// OdysseyCombatUIComponent.cpp

#include "OdysseyCombatUIComponent.h"
#include "OdysseyCombatTargetingComponent.h"
#include "OdysseyCombatWeaponComponent.h"
#include "NPCHealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

UOdysseyCombatUIComponent::UOdysseyCombatUIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.033f; // 30 FPS for smooth UI updates

	// Default configuration
	bShowTargetIndicators = true;
	bShowHealthBars = true;
	bShowDamageNumbers = true;
	bShowHitMarkers = true;
	bShowWeaponStatus = true;

	// Performance limits
	MaxTargetIndicators = 5;
	MaxHealthBars = 8;
	MaxDamageNumbers = 10;
	UIUpdateFrequency = 0.033f;

	// Initialize runtime state
	TargetingComponent = nullptr;
	WeaponComponent = nullptr;
	WeaponStatusWidget = nullptr;
	LastUIUpdateTime = 0.0f;
}

void UOdysseyCombatUIComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeUI();
}

void UOdysseyCombatUIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownUI();
	Super::EndPlay(EndPlayReason);
}

void UOdysseyCombatUIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	float CurrentTime = FPlatformTime::Seconds();
	
	// Check if it's time for a UI update
	if (CurrentTime - LastUIUpdateTime >= UIUpdateFrequency)
	{
		LastUIUpdateTime = CurrentTime;
		UpdateActiveUIElements(DeltaTime);
	}
}

// ============================================================================
// Target Indicator Management
// ============================================================================

void UOdysseyCombatUIComponent::ShowTargetIndicator(AActor* Target, bool bIsHostile)
{
	if (!Target || !bShowTargetIndicators || !TargetIndicatorWidgetClass)
	{
		return;
	}

	// Check if we already have an indicator for this target
	FCombatUIElement* ExistingElement = FindUIElement(Target, ECombatUIElement::TargetIndicator);
	if (ExistingElement)
	{
		return; // Already showing indicator
	}

	// Check if we've reached the maximum number of indicators
	int32 IndicatorCount = 0;
	for (const FCombatUIElement& Element : ActiveUIElements)
	{
		if (Element.ElementType == ECombatUIElement::TargetIndicator)
		{
			IndicatorCount++;
		}
	}

	if (IndicatorCount >= MaxTargetIndicators)
	{
		return; // Too many indicators
	}

	// Create new indicator widget
	UUserWidget* IndicatorWidget = GetPooledWidget(ECombatUIElement::TargetIndicator);
	if (!IndicatorWidget)
	{
		return;
	}

	// Add to viewport
	IndicatorWidget->AddToViewport(10); // High Z-order for indicators

	// Configure indicator appearance based on hostility
	if (UImage* IndicatorImage = Cast<UImage>(IndicatorWidget->GetWidgetFromName(FName("IndicatorImage"))))
	{
		FLinearColor IndicatorColor = bIsHostile ? TargetIndicatorConfig.HostileColor : TargetIndicatorConfig.FriendlyColor;
		IndicatorImage->SetColorAndOpacity(IndicatorColor);
	}

	// Create UI element entry
	FCombatUIElement NewElement;
	NewElement.Widget = IndicatorWidget;
	NewElement.TrackedActor = Target;
	NewElement.ElementType = ECombatUIElement::TargetIndicator;
	NewElement.CreationTime = FPlatformTime::Seconds();
	NewElement.LifeTime = 0.0f; // Persistent until manually removed
	NewElement.bShouldDestroy = false;

	ActiveUIElements.Add(NewElement);
	OnTargetIndicatorShown(Target);
}

void UOdysseyCombatUIComponent::HideTargetIndicator(AActor* Target)
{
	RemoveUIElement(Target, ECombatUIElement::TargetIndicator);
	OnTargetIndicatorHidden(Target);
}

void UOdysseyCombatUIComponent::UpdateTargetIndicator(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	FCombatUIElement* Element = FindUIElement(Target, ECombatUIElement::TargetIndicator);
	if (!Element || !Element->Widget)
	{
		return;
	}

	// Update indicator position
	FVector2D ScreenPosition;
	if (WorldToScreen(Target->GetActorLocation(), ScreenPosition))
	{
		// Center the indicator on the target
		FVector2D IndicatorSize = FVector2D(TargetIndicatorConfig.IndicatorSize, TargetIndicatorConfig.IndicatorSize);
		FVector2D Position = ScreenPosition - (IndicatorSize * 0.5f);
		Element->Widget->SetPositionInViewport(Position);
		Element->Widget->SetVisibility(ESlateVisibility::Visible);

		// Update distance text if enabled
		if (TargetIndicatorConfig.bShowDistance)
		{
			if (UTextBlock* DistanceText = Cast<UTextBlock>(Element->Widget->GetWidgetFromName(FName("DistanceText"))))
			{
				float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
				FString DistanceString = FString::Printf(TEXT("%.0f m"), Distance / 100.0f); // Convert to meters
				DistanceText->SetText(FText::FromString(DistanceString));
			}
		}

		// Update health text if enabled
		if (TargetIndicatorConfig.bShowHealth)
		{
			if (UTextBlock* HealthText = Cast<UTextBlock>(Element->Widget->GetWidgetFromName(FName("HealthText"))))
			{
				UNPCHealthComponent* HealthComp = Target->FindComponentByClass<UNPCHealthComponent>();
				if (HealthComp)
				{
					float HealthPercent = HealthComp->GetHealthPercentage() * 100.0f;
					FString HealthString = FString::Printf(TEXT("%.0f%%"), HealthPercent);
					HealthText->SetText(FText::FromString(HealthString));
				}
			}
		}
	}
	else
	{
		// Target is off-screen, hide indicator
		Element->Widget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UOdysseyCombatUIComponent::HideAllTargetIndicators()
{
	for (int32 i = ActiveUIElements.Num() - 1; i >= 0; i--)
	{
		if (ActiveUIElements[i].ElementType == ECombatUIElement::TargetIndicator)
		{
			if (ActiveUIElements[i].Widget)
			{
				ReturnWidgetToPool(ActiveUIElements[i].Widget, ECombatUIElement::TargetIndicator);
			}
			ActiveUIElements.RemoveAt(i);
		}
	}
}

// ============================================================================
// Health Bar Management
// ============================================================================

void UOdysseyCombatUIComponent::ShowHealthBar(AActor* Target)
{
	if (!Target || !bShowHealthBars || !HealthBarWidgetClass)
	{
		return;
	}

	// Check if we already have a health bar for this target
	FCombatUIElement* ExistingElement = FindUIElement(Target, ECombatUIElement::HealthBar);
	if (ExistingElement)
	{
		return; // Already showing health bar
	}

	// Check if we've reached the maximum number of health bars
	int32 HealthBarCount = 0;
	for (const FCombatUIElement& Element : ActiveUIElements)
	{
		if (Element.ElementType == ECombatUIElement::HealthBar)
		{
			HealthBarCount++;
		}
	}

	if (HealthBarCount >= MaxHealthBars)
	{
		return; // Too many health bars
	}

	// Create new health bar widget
	UUserWidget* HealthBarWidget = GetPooledWidget(ECombatUIElement::HealthBar);
	if (!HealthBarWidget)
	{
		return;
	}

	// Add to viewport
	HealthBarWidget->AddToViewport(5); // Lower Z-order than indicators

	// Create UI element entry
	FCombatUIElement NewElement;
	NewElement.Widget = HealthBarWidget;
	NewElement.TrackedActor = Target;
	NewElement.ElementType = ECombatUIElement::HealthBar;
	NewElement.CreationTime = FPlatformTime::Seconds();
	NewElement.LifeTime = HealthBarConfig.FadeOutDelay;
	NewElement.bShouldDestroy = false;

	ActiveUIElements.Add(NewElement);

	// Update health bar immediately
	UNPCHealthComponent* HealthComp = Target->FindComponentByClass<UNPCHealthComponent>();
	if (HealthComp)
	{
		UpdateHealthBar(Target, HealthComp->GetHealthPercentage());
	}
}

void UOdysseyCombatUIComponent::HideHealthBar(AActor* Target)
{
	RemoveUIElement(Target, ECombatUIElement::HealthBar);
}

void UOdysseyCombatUIComponent::UpdateHealthBar(AActor* Target, float HealthPercentage)
{
	if (!Target)
	{
		return;
	}

	FCombatUIElement* Element = FindUIElement(Target, ECombatUIElement::HealthBar);
	if (!Element || !Element->Widget)
	{
		return;
	}

	// Update health bar position
	FVector TargetLocation = Target->GetActorLocation() + HealthBarConfig.OffsetFromActor;
	FVector2D ScreenPosition;
	if (WorldToScreen(TargetLocation, ScreenPosition))
	{
		// Center the health bar
		FVector2D Position = ScreenPosition - (HealthBarConfig.BarSize * 0.5f);
		Element->Widget->SetPositionInViewport(Position);
		Element->Widget->SetVisibility(ESlateVisibility::Visible);

		// Update progress bar
		if (UProgressBar* HealthProgress = Cast<UProgressBar>(Element->Widget->GetWidgetFromName(FName("HealthProgress"))))
		{
			HealthProgress->SetPercent(HealthPercentage);

			// Update color based on health level
			FLinearColor BarColor;
			if (HealthPercentage > 0.6f)
			{
				BarColor = HealthBarConfig.HealthyColor;
			}
			else if (HealthPercentage > 0.3f)
			{
				BarColor = HealthBarConfig.DamagedColor;
			}
			else
			{
				BarColor = HealthBarConfig.CriticalColor;
			}
			HealthProgress->SetFillColorAndOpacity(BarColor);
		}

		// Reset lifetime for fade out
		Element->LifeTime = HealthBarConfig.FadeOutDelay;
		Element->bShouldDestroy = false;
	}
	else
	{
		// Target is off-screen, hide health bar
		Element->Widget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UOdysseyCombatUIComponent::HideAllHealthBars()
{
	for (int32 i = ActiveUIElements.Num() - 1; i >= 0; i--)
	{
		if (ActiveUIElements[i].ElementType == ECombatUIElement::HealthBar)
		{
			if (ActiveUIElements[i].Widget)
			{
				ReturnWidgetToPool(ActiveUIElements[i].Widget, ECombatUIElement::HealthBar);
			}
			ActiveUIElements.RemoveAt(i);
		}
	}
}

// ============================================================================
// Damage Numbers
// ============================================================================

void UOdysseyCombatUIComponent::ShowDamageNumber(FVector Location, float Damage, bool bIsCritical, bool bIsHealing)
{
	if (!bShowDamageNumbers || !DamageNumberWidgetClass || !DamageNumberConfig.bShowDamageNumbers)
	{
		return;
	}

	// Check damage number limit
	int32 DamageNumberCount = 0;
	for (const FCombatUIElement& Element : ActiveUIElements)
	{
		if (Element.ElementType == ECombatUIElement::DamageNumber)
		{
			DamageNumberCount++;
		}
	}

	if (DamageNumberCount >= MaxDamageNumbers)
	{
		return; // Too many damage numbers
	}

	// Create damage number widget
	UUserWidget* DamageWidget = GetPooledWidget(ECombatUIElement::DamageNumber);
	if (!DamageWidget)
	{
		return;
	}

	// Add to viewport
	DamageWidget->AddToViewport(15); // Highest Z-order

	// Configure damage number
	if (UTextBlock* DamageText = Cast<UTextBlock>(DamageWidget->GetWidgetFromName(FName("DamageText"))))
	{
		// Format damage text
		FString DamageString;
		if (bIsHealing)
		{
			DamageString = FString::Printf(TEXT("+%.0f"), Damage);
		}
		else if (bIsCritical)
		{
			DamageString = FString::Printf(TEXT("%.0f!"), Damage);
		}
		else
		{
			DamageString = FString::Printf(TEXT("%.0f"), Damage);
		}

		DamageText->SetText(FText::FromString(DamageString));

		// Set color
		FLinearColor TextColor;
		if (bIsHealing)
		{
			TextColor = DamageNumberConfig.HealingColor;
		}
		else if (bIsCritical)
		{
			TextColor = DamageNumberConfig.CriticalDamageColor;
		}
		else
		{
			TextColor = DamageNumberConfig.NormalDamageColor;
		}
		DamageText->SetColorAndOpacity(TextColor);

		// Set font size
		FSlateFontInfo FontInfo = DamageText->GetFont();
		FontInfo.Size = bIsCritical ? DamageNumberConfig.FontSize * 1.5f : DamageNumberConfig.FontSize;
		DamageText->SetFont(FontInfo);
	}

	// Position damage number
	FVector2D ScreenPosition;
	if (WorldToScreen(Location, ScreenPosition))
	{
		DamageWidget->SetPositionInViewport(ScreenPosition);
	}

	// Create UI element entry
	FCombatUIElement NewElement;
	NewElement.Widget = DamageWidget;
	NewElement.TrackedActor = nullptr; // Damage numbers don't track actors
	NewElement.ElementType = ECombatUIElement::DamageNumber;
	NewElement.CreationTime = FPlatformTime::Seconds();
	NewElement.LifeTime = DamageNumberConfig.AnimationDuration;
	NewElement.bShouldDestroy = true; // Auto-destroy after animation

	ActiveUIElements.Add(NewElement);
	OnDamageNumberShown(Damage, bIsCritical);
}

void UOdysseyCombatUIComponent::ShowDamageNumberAtActor(AActor* Target, float Damage, bool bIsCritical, bool bIsHealing)
{
	if (Target)
	{
		FVector DamageLocation = Target->GetActorLocation() + FVector(0, 0, 50); // Slightly above actor
		ShowDamageNumber(DamageLocation, Damage, bIsCritical, bIsHealing);
	}
}

// ============================================================================
// Weapon Status UI
// ============================================================================

void UOdysseyCombatUIComponent::UpdateWeaponCharge(float ChargePercentage)
{
	if (!WeaponStatusWidget)
	{
		return;
	}

	if (UProgressBar* ChargeProgress = Cast<UProgressBar>(WeaponStatusWidget->GetWidgetFromName(FName("ChargeProgress"))))
	{
		ChargeProgress->SetPercent(ChargePercentage);
		ChargeProgress->SetVisibility(ChargePercentage > 0.0f ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UOdysseyCombatUIComponent::UpdateWeaponReload(float ReloadPercentage)
{
	if (!WeaponStatusWidget)
	{
		return;
	}

	if (UProgressBar* ReloadProgress = Cast<UProgressBar>(WeaponStatusWidget->GetWidgetFromName(FName("ReloadProgress"))))
	{
		ReloadProgress->SetPercent(ReloadPercentage);
		ReloadProgress->SetVisibility(ReloadPercentage < 1.0f ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UOdysseyCombatUIComponent::UpdateWeaponEnergy(float EnergyPercentage)
{
	if (!WeaponStatusWidget)
	{
		return;
	}

	if (UProgressBar* EnergyProgress = Cast<UProgressBar>(WeaponStatusWidget->GetWidgetFromName(FName("EnergyProgress"))))
	{
		EnergyProgress->SetPercent(EnergyPercentage);

		// Change color based on energy level
		FLinearColor EnergyColor;
		if (EnergyPercentage > 0.6f)
		{
			EnergyColor = FLinearColor::Green;
		}
		else if (EnergyPercentage > 0.3f)
		{
			EnergyColor = FLinearColor::Yellow;
		}
		else
		{
			EnergyColor = FLinearColor::Red;
		}
		EnergyProgress->SetFillColorAndOpacity(EnergyColor);
	}
}

void UOdysseyCombatUIComponent::ShowWeaponStatus()
{
	if (!bShowWeaponStatus || !WeaponStatusWidgetClass)
	{
		return;
	}

	if (!WeaponStatusWidget)
	{
		WeaponStatusWidget = CreateWidget<UUserWidget>(GetWorld(), WeaponStatusWidgetClass);
	}

	if (WeaponStatusWidget && !WeaponStatusWidget->IsInViewport())
	{
		WeaponStatusWidget->AddToViewport(1); // Lower Z-order for HUD elements
	}
}

void UOdysseyCombatUIComponent::HideWeaponStatus()
{
	if (WeaponStatusWidget && WeaponStatusWidget->IsInViewport())
	{
		WeaponStatusWidget->RemoveFromViewport();
	}
}

// ============================================================================
// Hit Markers and Touch Feedback
// ============================================================================

void UOdysseyCombatUIComponent::ShowHitMarker(FVector2D ScreenLocation, bool bWasCritical)
{
	if (!bShowHitMarkers || !HitMarkerWidgetClass)
	{
		return;
	}

	// Create hit marker widget
	UUserWidget* HitMarkerWidget = GetPooledWidget(ECombatUIElement::HitMarker);
	if (!HitMarkerWidget)
	{
		return;
	}

	// Add to viewport
	HitMarkerWidget->AddToViewport(20); // Very high Z-order

	// Configure hit marker appearance
	if (UImage* HitMarkerImage = Cast<UImage>(HitMarkerWidget->GetWidgetFromName(FName("HitMarkerImage"))))
	{
		FLinearColor MarkerColor = bWasCritical ? FLinearColor::Red : FLinearColor::White;
		HitMarkerImage->SetColorAndOpacity(MarkerColor);
	}

	// Position hit marker
	HitMarkerWidget->SetPositionInViewport(ScreenLocation);

	// Create UI element entry
	FCombatUIElement NewElement;
	NewElement.Widget = HitMarkerWidget;
	NewElement.TrackedActor = nullptr;
	NewElement.ElementType = ECombatUIElement::HitMarker;
	NewElement.CreationTime = FPlatformTime::Seconds();
	NewElement.LifeTime = 0.5f; // Short duration
	NewElement.bShouldDestroy = true;

	ActiveUIElements.Add(NewElement);
	OnHitMarkerShown(bWasCritical);
}

void UOdysseyCombatUIComponent::ShowHitMarkerAtLocation(FVector WorldLocation, bool bWasCritical)
{
	FVector2D ScreenPosition;
	if (WorldToScreen(WorldLocation, ScreenPosition))
	{
		ShowHitMarker(ScreenPosition, bWasCritical);
	}
}

void UOdysseyCombatUIComponent::ShowTouchFeedback(FVector2D TouchLocation)
{
	// Simple touch feedback - could show a ripple effect or highlight
	// For now, we'll use a brief hit marker
	ShowHitMarker(TouchLocation, false);
}

// ============================================================================
// Configuration
// ============================================================================

void UOdysseyCombatUIComponent::SetTargetIndicatorConfig(const FTargetIndicatorConfig& Config)
{
	TargetIndicatorConfig = Config;
}

void UOdysseyCombatUIComponent::SetHealthBarConfig(const FHealthBarConfig& Config)
{
	HealthBarConfig = Config;
}

void UOdysseyCombatUIComponent::SetDamageNumberConfig(const FDamageNumberConfig& Config)
{
	DamageNumberConfig = Config;
}

void UOdysseyCombatUIComponent::SetUIElementEnabled(ECombatUIElement ElementType, bool bEnabled)
{
	switch (ElementType)
	{
	case ECombatUIElement::TargetIndicator:
		bShowTargetIndicators = bEnabled;
		if (!bEnabled)
		{
			HideAllTargetIndicators();
		}
		break;
		
	case ECombatUIElement::HealthBar:
		bShowHealthBars = bEnabled;
		if (!bEnabled)
		{
			HideAllHealthBars();
		}
		break;
		
	case ECombatUIElement::DamageNumber:
		bShowDamageNumbers = bEnabled;
		break;
		
	case ECombatUIElement::HitMarker:
		bShowHitMarkers = bEnabled;
		break;
		
	case ECombatUIElement::WeaponStatus:
		bShowWeaponStatus = bEnabled;
		if (bEnabled)
		{
			ShowWeaponStatus();
		}
		else
		{
			HideWeaponStatus();
		}
		break;
		
	default:
		break;
	}
}

// ============================================================================
// Integration with Combat Systems
// ============================================================================

void UOdysseyCombatUIComponent::SetTargetingComponent(UOdysseyCombatTargetingComponent* TargetingComp)
{
	TargetingComponent = TargetingComp;
}

void UOdysseyCombatUIComponent::SetWeaponComponent(UOdysseyCombatWeaponComponent* WeaponComp)
{
	WeaponComponent = WeaponComp;
}

// ============================================================================
// Internal Methods
// ============================================================================

void UOdysseyCombatUIComponent::InitializeUI()
{
	// Initialize widget pools
	InitializeWidgetPools();

	// Find components on owner
	if (GetOwner())
	{
		if (!TargetingComponent)
		{
			TargetingComponent = GetOwner()->FindComponentByClass<UOdysseyCombatTargetingComponent>();
		}
		
		if (!WeaponComponent)
		{
			WeaponComponent = GetOwner()->FindComponentByClass<UOdysseyCombatWeaponComponent>();
		}
	}

	// Show weapon status if enabled
	if (bShowWeaponStatus)
	{
		ShowWeaponStatus();
	}
}

void UOdysseyCombatUIComponent::ShutdownUI()
{
	// Hide all UI elements
	HideAllTargetIndicators();
	HideAllHealthBars();
	HideWeaponStatus();

	// Clean up widget pools
	CleanupWidgetPools();

	// Clean up active elements
	ActiveUIElements.Empty();
}

void UOdysseyCombatUIComponent::UpdateActiveUIElements(float DeltaTime)
{
	float CurrentTime = FPlatformTime::Seconds();

	// Update positions and handle lifetime
	for (int32 i = ActiveUIElements.Num() - 1; i >= 0; i--)
	{
		FCombatUIElement& Element = ActiveUIElements[i];

		// Update position-based elements
		if (Element.TrackedActor && IsValid(Element.TrackedActor))
		{
			switch (Element.ElementType)
			{
			case ECombatUIElement::TargetIndicator:
				UpdateTargetIndicator(Element.TrackedActor);
				break;
				
			case ECombatUIElement::HealthBar:
				{
					UNPCHealthComponent* HealthComp = Element.TrackedActor->FindComponentByClass<UNPCHealthComponent>();
					if (HealthComp)
					{
						UpdateHealthBar(Element.TrackedActor, HealthComp->GetHealthPercentage());
					}
				}
				break;
				
			default:
				break;
			}
		}

		// Handle lifetime for temporary elements
		if (Element.bShouldDestroy && Element.LifeTime > 0.0f)
		{
			float ElapsedTime = CurrentTime - Element.CreationTime;
			if (ElapsedTime >= Element.LifeTime)
			{
				// Time to destroy
				if (Element.Widget)
				{
					ReturnWidgetToPool(Element.Widget, Element.ElementType);
				}
				ActiveUIElements.RemoveAt(i);
			}
			else
			{
				// Update damage number animation
				if (Element.ElementType == ECombatUIElement::DamageNumber && Element.Widget)
				{
					float AnimationProgress = ElapsedTime / Element.LifeTime;
					
					// Move damage number up
					FVector2D CurrentPosition = Element.Widget->GetCachedGeometry().GetAbsolutePosition();
					FVector2D NewPosition = CurrentPosition + FVector2D(0, -DamageNumberConfig.AnimationDirection.Z * DeltaTime);
					Element.Widget->SetPositionInViewport(NewPosition);
					
					// Fade out
					float Alpha = 1.0f - AnimationProgress;
					Element.Widget->SetRenderOpacity(Alpha);
				}
			}
		}

		// Remove elements with invalid actors
		if (Element.TrackedActor && !IsValid(Element.TrackedActor))
		{
			if (Element.Widget)
			{
				ReturnWidgetToPool(Element.Widget, Element.ElementType);
			}
			ActiveUIElements.RemoveAt(i);
		}
	}

	// Update weapon status
	if (WeaponComponent && WeaponStatusWidget && WeaponStatusWidget->IsInViewport())
	{
		UpdateWeaponCharge(WeaponComponent->GetChargeLevel());
		UpdateWeaponReload(WeaponComponent->GetReloadProgress());
		
		// Update energy from action button manager
		if (UOdysseyActionButtonManager* ActionManager = GetOwner()->FindComponentByClass<UOdysseyActionButtonManager>())
		{
			UpdateWeaponEnergy(ActionManager->GetEnergyPercentage());
		}
	}
}

UUserWidget* UOdysseyCombatUIComponent::GetPooledWidget(ECombatUIElement ElementType)
{
	TArray<UUserWidget*>* TargetPool = nullptr;
	TSubclassOf<UUserWidget> WidgetClass = nullptr;

	switch (ElementType)
	{
	case ECombatUIElement::TargetIndicator:
		TargetPool = &TargetIndicatorPool;
		WidgetClass = TargetIndicatorWidgetClass;
		break;
		
	case ECombatUIElement::HealthBar:
		TargetPool = &HealthBarPool;
		WidgetClass = HealthBarWidgetClass;
		break;
		
	case ECombatUIElement::DamageNumber:
		TargetPool = &DamageNumberPool;
		WidgetClass = DamageNumberWidgetClass;
		break;
		
	case ECombatUIElement::HitMarker:
		TargetPool = &HitMarkerPool;
		WidgetClass = HitMarkerWidgetClass;
		break;
		
	default:
		return nullptr;
	}

	if (!TargetPool || !WidgetClass)
	{
		return nullptr;
	}

	// Try to get widget from pool
	if (TargetPool->Num() > 0)
	{
		UUserWidget* Widget = (*TargetPool)[0];
		TargetPool->RemoveAt(0);
		Widget->SetVisibility(ESlateVisibility::Visible);
		Widget->SetRenderOpacity(1.0f);
		return Widget;
	}

	// Create new widget if pool is empty
	return CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
}

void UOdysseyCombatUIComponent::ReturnWidgetToPool(UUserWidget* Widget, ECombatUIElement ElementType)
{
	if (!Widget)
	{
		return;
	}

	// Remove from viewport
	Widget->RemoveFromViewport();
	Widget->SetVisibility(ESlateVisibility::Hidden);

	// Return to appropriate pool
	switch (ElementType)
	{
	case ECombatUIElement::TargetIndicator:
		if (TargetIndicatorPool.Num() < MaxTargetIndicators)
		{
			TargetIndicatorPool.Add(Widget);
		}
		break;
		
	case ECombatUIElement::HealthBar:
		if (HealthBarPool.Num() < MaxHealthBars)
		{
			HealthBarPool.Add(Widget);
		}
		break;
		
	case ECombatUIElement::DamageNumber:
		if (DamageNumberPool.Num() < MaxDamageNumbers)
		{
			DamageNumberPool.Add(Widget);
		}
		break;
		
	case ECombatUIElement::HitMarker:
		if (HitMarkerPool.Num() < 5) // Small pool for hit markers
		{
			HitMarkerPool.Add(Widget);
		}
		break;
		
	default:
		break;
	}
}

void UOdysseyCombatUIComponent::InitializeWidgetPools()
{
	// Pools will be populated as needed
	TargetIndicatorPool.Reserve(MaxTargetIndicators);
	HealthBarPool.Reserve(MaxHealthBars);
	DamageNumberPool.Reserve(MaxDamageNumbers);
	HitMarkerPool.Reserve(5);
}

void UOdysseyCombatUIComponent::CleanupWidgetPools()
{
	// Clear all pools
	TargetIndicatorPool.Empty();
	HealthBarPool.Empty();
	DamageNumberPool.Empty();
	HitMarkerPool.Empty();
}

bool UOdysseyCombatUIComponent::WorldToScreen(FVector WorldLocation, FVector2D& ScreenLocation)
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return false;
	}

	return UGameplayStatics::ProjectWorldToScreen(PlayerController, WorldLocation, ScreenLocation, false);
}

FCombatUIElement* UOdysseyCombatUIComponent::FindUIElement(AActor* Target, ECombatUIElement ElementType)
{
	for (FCombatUIElement& Element : ActiveUIElements)
	{
		if (Element.TrackedActor == Target && Element.ElementType == ElementType)
		{
			return &Element;
		}
	}
	return nullptr;
}

void UOdysseyCombatUIComponent::RemoveUIElement(AActor* Target, ECombatUIElement ElementType)
{
	for (int32 i = ActiveUIElements.Num() - 1; i >= 0; i--)
	{
		FCombatUIElement& Element = ActiveUIElements[i];
		if (Element.TrackedActor == Target && Element.ElementType == ElementType)
		{
			if (Element.Widget)
			{
				ReturnWidgetToPool(Element.Widget, ElementType);
			}
			ActiveUIElements.RemoveAt(i);
			break;
		}
	}
}
