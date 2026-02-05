// OdysseyCombatManager.cpp

#include "OdysseyCombatManager.h"
#include "OdysseyCombatTargetingComponent.h"
#include "OdysseyCombatWeaponComponent.h"
#include "OdysseyCombatUIComponent.h"
#include "OdysseyTouchInterface.h"
#include "OdysseyActionButton.h"
#include "NPCHealthComponent.h"
#include "Engine/World.h"

UOdysseyCombatManager::UOdysseyCombatManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f; // 20 FPS for responsive combat

	// Default configuration
	CombatConfig.bEnableAutoTargeting = true;
	CombatConfig.bEnableAutoFiring = true;
	CombatConfig.bShowTargetIndicators = true;
	CombatConfig.bShowHealthBars = true;
	CombatConfig.bShowDamageNumbers = true;
	CombatConfig.TargetingRange = 2000.0f;
	CombatConfig.WeaponRange = 1500.0f;

	bAutoInitialize = true;
	bAutoRegisterTouch = true;
	bAutoRegisterActions = true;
	CombatUpdateFrequency = 0.05f;

	// Initialize state
	CurrentState = ECombatSystemState::Inactive;
	CombatStartTime = 0.0f;
	LastUpdateTime = 0.0f;
	bSystemInitialized = false;
	bTouchRegistered = false;
	bActionsRegistered = false;
	LastTarget = nullptr;

	// Initialize component references
	TargetingComponent = nullptr;
	WeaponComponent = nullptr;
	UIComponent = nullptr;
	TouchInterface = nullptr;
	ActionButtonManager = nullptr;
}

void UOdysseyCombatManager::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitialize)
	{
		InitializeCombatSystem();
	}
}

void UOdysseyCombatManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownCombatSystem();
	Super::EndPlay(EndPlayReason);
}

void UOdysseyCombatManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bSystemInitialized && CurrentState != ECombatSystemState::Inactive)
	{
		float CurrentTime = FPlatformTime::Seconds();
		
		// Check if it's time for a combat update
		if (CurrentTime - LastUpdateTime >= CombatUpdateFrequency)
		{
			LastUpdateTime = CurrentTime;
			UpdateCombatSystem(DeltaTime);
		}
	}
}

// ============================================================================
// Combat System Control
// ============================================================================

void UOdysseyCombatManager::InitializeCombatSystem()
{
	if (bSystemInitialized)
	{
		return; // Already initialized
	}

	// Find and initialize components
	InitializeComponents();

	// Validate system
	if (!ValidateCombatSystem())
	{
		UE_LOG(LogTemp, Error, TEXT("Combat system validation failed!"));
		return;
	}

	// Apply configuration
	ApplyConfiguration();

	// Register input if enabled
	if (bAutoRegisterTouch)
	{
		RegisterTouchInput();
	}

	if (bAutoRegisterActions)
	{
		RegisterCombatActions();
	}

	// Set initial state
	SetCombatState(ECombatSystemState::Standby);
	
	bSystemInitialized = true;
	
	UE_LOG(LogTemp, Log, TEXT("Combat system initialized successfully"));
}

void UOdysseyCombatManager::ShutdownCombatSystem()
{
	if (!bSystemInitialized)
	{
		return;
	}

	// Unregister input
	UnregisterTouchInput();
	UnregisterCombatActions();

	// Clear target
	ClearTarget();

	// Set inactive state
	SetCombatState(ECombatSystemState::Inactive);

	bSystemInitialized = false;
}

void UOdysseyCombatManager::SetCombatEnabled(bool bEnabled)
{
	if (bEnabled)
	{
		if (!bSystemInitialized)
		{
			InitializeCombatSystem();
		}
		else if (CurrentState == ECombatSystemState::Disabled)
		{
			SetCombatState(ECombatSystemState::Standby);
		}
	}
	else
	{
		SetCombatState(ECombatSystemState::Disabled);
	}
}

void UOdysseyCombatManager::SetCombatState(ECombatSystemState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	ECombatSystemState OldState = CurrentState;
	CurrentState = NewState;

	// Handle state transitions
	switch (NewState)
	{
	case ECombatSystemState::Standby:
		if (WeaponComponent)
		{
			WeaponComponent->SetAutoFireEnabled(false);
		}
		break;

	case ECombatSystemState::Engaging:
		if (WeaponComponent && CombatConfig.bEnableAutoFiring)
		{
			WeaponComponent->SetAutoFireEnabled(true);
		}
		CombatStartTime = FPlatformTime::Seconds();
		OnCombatStarted();
		break;

	case ECombatSystemState::Inactive:
	case ECombatSystemState::Disabled:
		if (WeaponComponent)
		{
			WeaponComponent->SetAutoFireEnabled(false);
		}
		if (OldState == ECombatSystemState::Engaging)
		{
			OnCombatEnded();
		}
		break;
	}

	OnCombatStateChanged(OldState, NewState);
}

bool UOdysseyCombatManager::IsCombatActive() const
{
	return CurrentState == ECombatSystemState::Engaging || CurrentState == ECombatSystemState::Standby;
}

// ============================================================================
// Touch Input Integration
// ============================================================================

bool UOdysseyCombatManager::HandleCombatTouch(FVector2D TouchLocation)
{
	if (!IsCombatActive() || !TargetingComponent)
	{
		return false;
	}

	// Handle targeting touch
	FTouchTargetResult Result = TargetingComponent->HandleTouchTargeting(TouchLocation);
	
	if (Result.bValidTouch && Result.TouchedActor)
	{
		// Show touch feedback
		if (UIComponent)
		{
			UIComponent->ShowTouchFeedback(TouchLocation);
		}

		// If we have a valid target, switch to engaging state
		if (GetCurrentTarget())
		{
			SetCombatState(ECombatSystemState::Engaging);
		}

		return true;
	}

	return false;
}

bool UOdysseyCombatManager::HandleFireTouch(FVector2D TouchLocation)
{
	if (!IsCombatActive() || !WeaponComponent)
	{
		return false;
	}

	// Fire weapon
	FWeaponFireResult Result = FireWeapon();
	
	if (Result.bFireSuccessful)
	{
		// Show hit marker if we hit something
		if (Result.bHitTarget && UIComponent)
		{
			UIComponent->ShowHitMarker(TouchLocation, Result.bWasCritical);
		}
		
		return true;
	}

	return false;
}

void UOdysseyCombatManager::RegisterTouchInput()
{
	if (bTouchRegistered || !TouchInterface)
	{
		return;
	}

	// Note: In a full implementation, you'd register touch callbacks with the touch interface
	// For now, we'll rely on the touch interface calling our HandleCombatTouch method

	bTouchRegistered = true;
}

void UOdysseyCombatManager::UnregisterTouchInput()
{
	if (!bTouchRegistered || !TouchInterface)
	{
		return;
	}

	// Unregister touch callbacks
	bTouchRegistered = false;
}

// ============================================================================
// Action Button Integration
// ============================================================================

bool UOdysseyCombatManager::HandleAttackAction()
{
	if (!IsCombatActive())
	{
		return false;
	}

	// Try to auto-select target if we don't have one
	if (!GetCurrentTarget() && CombatConfig.bEnableAutoTargeting)
	{
		AutoSelectTarget();
	}

	// Fire weapon
	FWeaponFireResult Result = FireWeapon();
	
	if (Result.bFireSuccessful)
	{
		OnWeaponFired(GetCurrentTarget(), Result.bHitTarget);
		return true;
	}

	return false;
}

bool UOdysseyCombatManager::HandleSpecialAttackAction()
{
	if (!IsCombatActive() || !WeaponComponent)
	{
		return false;
	}

	// Start charging weapon if it supports it
	if (WeaponComponent->GetFireMode() == EWeaponFireMode::Charged)
	{
		return WeaponComponent->StartCharging();
	}

	// Otherwise, just fire a regular shot
	return HandleAttackAction();
}

void UOdysseyCombatManager::RegisterCombatActions()
{
	if (bActionsRegistered || !ActionButtonManager)
	{
		return;
	}

	// Note: In a full implementation, you'd register action handlers with the action manager
	// For now, we'll assume the action manager will call our Handle methods

	bActionsRegistered = true;
}

void UOdysseyCombatManager::UnregisterCombatActions()
{
	if (!bActionsRegistered || !ActionButtonManager)
	{
		return;
	}

	// Unregister action handlers
	bActionsRegistered = false;
}

// ============================================================================
// Targeting Control
// ============================================================================

void UOdysseyCombatManager::SetTargetingMode(ETargetingMode Mode)
{
	if (TargetingComponent)
	{
		TargetingComponent->SetTargetingMode(Mode);
	}
}

AActor* UOdysseyCombatManager::GetCurrentTarget() const
{
	return TargetingComponent ? TargetingComponent->GetCurrentTarget() : nullptr;
}

bool UOdysseyCombatManager::SelectTarget(AActor* Target)
{
	if (!TargetingComponent)
	{
		return false;
	}

	bool bResult = TargetingComponent->SelectTarget(Target);
	
	if (bResult && Target != LastTarget)
	{
		LastTarget = Target;
		OnTargetChanged();

		// Switch to engaging state if we have a target
		if (Target && IsCombatActive())
		{
			SetCombatState(ECombatSystemState::Engaging);
			OnTargetEngaged(Target);
		}
	}

	return bResult;
}

void UOdysseyCombatManager::ClearTarget()
{
	if (TargetingComponent)
	{
		TargetingComponent->ClearTarget();
	}

	LastTarget = nullptr;
	OnTargetChanged();

	// Switch to standby state if we were engaging
	if (CurrentState == ECombatSystemState::Engaging)
	{
		SetCombatState(ECombatSystemState::Standby);
	}
}

bool UOdysseyCombatManager::AutoSelectTarget()
{
	if (!TargetingComponent)
	{
		return false;
	}

	bool bResult = TargetingComponent->AutoSelectTarget();
	
	if (bResult)
	{
		AActor* NewTarget = GetCurrentTarget();
		if (NewTarget && NewTarget != LastTarget)
		{
			LastTarget = NewTarget;
			OnTargetChanged();

			if (IsCombatActive())
			{
				SetCombatState(ECombatSystemState::Engaging);
				OnTargetEngaged(NewTarget);
			}
		}
	}

	return bResult;
}

// ============================================================================
// Weapon Control
// ============================================================================

FWeaponFireResult UOdysseyCombatManager::FireWeapon()
{
	FWeaponFireResult Result;
	
	if (!WeaponComponent || !IsCombatActive())
	{
		Result.FailureReason = FName("CombatNotActive");
		return Result;
	}

	Result = WeaponComponent->FireWeapon();
	
	// Update statistics
	UpdateCombatStats(Result);

	// Handle events
	OnWeaponFiredInternal(Result);

	return Result;
}

void UOdysseyCombatManager::SetAutoFireEnabled(bool bEnabled)
{
	if (WeaponComponent)
	{
		WeaponComponent->SetAutoFireEnabled(bEnabled);
	}
	CombatConfig.bEnableAutoFiring = bEnabled;
}

bool UOdysseyCombatManager::CanFireWeapon() const
{
	return WeaponComponent ? WeaponComponent->CanFire() : false;
}

float UOdysseyCombatManager::GetWeaponChargeLevel() const
{
	return WeaponComponent ? WeaponComponent->GetChargeLevel() : 0.0f;
}

// ============================================================================
// Configuration
// ============================================================================

void UOdysseyCombatManager::SetCombatConfiguration(const FCombatConfiguration& Config)
{
	CombatConfig = Config;
	ApplyConfiguration();
}

void UOdysseyCombatManager::ApplyConfiguration()
{
	if (!bSystemInitialized)
	{
		return;
	}

	// Apply targeting configuration
	if (TargetingComponent)
	{
		TargetingComponent->SetTargetingMode(CombatConfig.bEnableAutoTargeting ? ETargetingMode::Assisted : ETargetingMode::Manual);
		TargetingComponent->SetMaxTargetingRange(CombatConfig.TargetingRange);
	}

	// Apply weapon configuration
	if (WeaponComponent)
	{
		WeaponComponent->SetAutoFireEnabled(CombatConfig.bEnableAutoFiring);
		FWeaponStats Stats = WeaponComponent->GetWeaponStats();
		Stats.Range = CombatConfig.WeaponRange;
		WeaponComponent->SetWeaponStats(Stats);
	}

	// Apply UI configuration
	if (UIComponent)
	{
		UIComponent->SetUIElementEnabled(ECombatUIElement::TargetIndicator, CombatConfig.bShowTargetIndicators);
		UIComponent->SetUIElementEnabled(ECombatUIElement::HealthBar, CombatConfig.bShowHealthBars);
		UIComponent->SetUIElementEnabled(ECombatUIElement::DamageNumber, CombatConfig.bShowDamageNumbers);
	}
}

// ============================================================================
// Statistics and Metrics
// ============================================================================

void UOdysseyCombatManager::ResetCombatStats()
{
	CombatStats = FCombatStats();
	CombatStartTime = FPlatformTime::Seconds();
}

void UOdysseyCombatManager::UpdateCombatStats(const FWeaponFireResult& FireResult)
{
	if (FireResult.bFireSuccessful)
	{
		CombatStats.ShotsFired++;

		if (FireResult.bHitTarget)
		{
			CombatStats.ShotsHit++;
			CombatStats.TotalDamageDealt += FireResult.DamageDealt;

			if (FireResult.bWasCritical)
			{
				CombatStats.CriticalHits++;
			}

			// Check if target was destroyed
			if (FireResult.HitActor)
			{
				UNPCHealthComponent* HealthComp = FireResult.HitActor->FindComponentByClass<UNPCHealthComponent>();
				if (HealthComp && HealthComp->IsDead())
				{
					CombatStats.EnemiesDestroyed++;
					OnTargetDestroyed(FireResult.HitActor);
				}
			}
		}
	}

	// Update combat time
	if (CurrentState == ECombatSystemState::Engaging)
	{
		CombatStats.CombatTime = FPlatformTime::Seconds() - CombatStartTime;
	}
}

// ============================================================================
// Internal Methods
// ============================================================================

void UOdysseyCombatManager::InitializeComponents()
{
	if (!GetOwner())
	{
		return;
	}

	// Find or create targeting component
	TargetingComponent = GetOwner()->FindComponentByClass<UOdysseyCombatTargetingComponent>();
	if (!TargetingComponent)
	{
		TargetingComponent = NewObject<UOdysseyCombatTargetingComponent>(GetOwner(), FName("CombatTargeting"));
		GetOwner()->AddInstanceComponent(TargetingComponent);
		TargetingComponent->RegisterComponent();
	}

	// Find or create weapon component
	WeaponComponent = GetOwner()->FindComponentByClass<UOdysseyCombatWeaponComponent>();
	if (!WeaponComponent)
	{
		WeaponComponent = NewObject<UOdysseyCombatWeaponComponent>(GetOwner(), FName("CombatWeapon"));
		GetOwner()->AddInstanceComponent(WeaponComponent);
		WeaponComponent->RegisterComponent();
	}

	// Find or create UI component
	UIComponent = GetOwner()->FindComponentByClass<UOdysseyCombatUIComponent>();
	if (!UIComponent)
	{
		UIComponent = NewObject<UOdysseyCombatUIComponent>(GetOwner(), FName("CombatUI"));
		GetOwner()->AddInstanceComponent(UIComponent);
		UIComponent->RegisterComponent();
	}

	// Find existing touch interface
	TouchInterface = GetOwner()->FindComponentByClass<UOdysseyTouchInterface>();

	// Find existing action button manager
	ActionButtonManager = GetOwner()->FindComponentByClass<UOdysseyActionButtonManager>();

	// Set up component relationships
	if (WeaponComponent && TargetingComponent)
	{
		WeaponComponent->SetTargetingComponent(TargetingComponent);
	}

	if (UIComponent)
	{
		if (TargetingComponent)
		{
			UIComponent->SetTargetingComponent(TargetingComponent);
		}
		if (WeaponComponent)
		{
			UIComponent->SetWeaponComponent(WeaponComponent);
		}
	}
}

void UOdysseyCombatManager::UpdateCombatSystem(float DeltaTime)
{
	// Update combat state based on current conditions
	UpdateCombatState();

	// Monitor target changes
	AActor* CurrentTarget = GetCurrentTarget();
	if (CurrentTarget != LastTarget)
	{
		LastTarget = CurrentTarget;
		OnTargetChanged();
	}
}

void UOdysseyCombatManager::UpdateCombatState()
{
	AActor* CurrentTarget = GetCurrentTarget();

	switch (CurrentState)
	{
	case ECombatSystemState::Standby:
		// Transition to engaging if we have a target
		if (CurrentTarget)
		{
			SetCombatState(ECombatSystemState::Engaging);
		}
		break;

	case ECombatSystemState::Engaging:
		// Transition to standby if we lose target
		if (!CurrentTarget)
		{
			SetCombatState(ECombatSystemState::Standby);
		}
		break;

	default:
		break;
	}
}

void UOdysseyCombatManager::OnTargetChanged()
{
	AActor* CurrentTarget = GetCurrentTarget();

	// Update UI
	if (UIComponent)
	{
		// Show/hide target indicators
		if (CurrentTarget)
		{
			UIComponent->ShowTargetIndicator(CurrentTarget, true); // Assume hostile for now
			UIComponent->ShowHealthBar(CurrentTarget);
		}
	}
}

void UOdysseyCombatManager::OnWeaponFiredInternal(const FWeaponFireResult& Result)
{
	// Update UI with damage numbers and hit markers
	if (UIComponent && Result.bHitTarget && Result.HitActor)
	{
		UIComponent->ShowDamageNumberAtActor(Result.HitActor, Result.DamageDealt, Result.bWasCritical);
		UIComponent->ShowHitMarkerAtLocation(Result.HitLocation, Result.bWasCritical);
	}

	// Check if target was destroyed
	if (Result.HitActor)
	{
		UNPCHealthComponent* HealthComp = Result.HitActor->FindComponentByClass<UNPCHealthComponent>();
		if (HealthComp && HealthComp->IsDead())
		{
			OnTargetDied(Result.HitActor);
		}
	}
}

void UOdysseyCombatManager::OnTargetDied(AActor* Target)
{
	// Clear target if it was our current target
	if (GetCurrentTarget() == Target)
	{
		ClearTarget();
	}

	// Hide UI elements for dead target
	if (UIComponent)
	{
		UIComponent->HideTargetIndicator(Target);
		UIComponent->HideHealthBar(Target);
	}

	OnTargetDestroyed(Target);
}

bool UOdysseyCombatManager::ValidateCombatSystem() const
{
	// Check that required components exist
	if (!TargetingComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Combat system missing targeting component"));
		return false;
	}

	if (!WeaponComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Combat system missing weapon component"));
		return false;
	}

	if (!UIComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("Combat system missing UI component"));
		return false;
	}

	return true;
}
