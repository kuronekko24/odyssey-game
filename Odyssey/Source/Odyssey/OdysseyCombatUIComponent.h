// OdysseyCombatUIComponent.h
// Mobile-optimized UI component for combat feedback and targeting indicators
// Handles target indicators, health bars, weapon status, and touch feedback

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "OdysseyActionEvent.h"
#include "OdysseyCombatUIComponent.generated.h"

// Forward declarations
class UOdysseyCombatTargetingComponent;
class UOdysseyCombatWeaponComponent;
class UNPCHealthComponent;
class UUserWidget;
class UProgressBar;
class UTextBlock;
class UImage;

/**
 * UI element type for different combat feedback
 */
UENUM(BlueprintType)
enum class ECombatUIElement : uint8
{
	TargetIndicator,    // Circle around targeted enemy
	HealthBar,          // Health bar above enemies
	DamageNumber,       // Floating damage numbers
	WeaponStatus,       // Weapon charge/reload status
	Crosshair,          // Targeting crosshair
	HitMarker          // Hit confirmation
};

/**
 * Target indicator configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTargetIndicatorConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Indicator")
	float IndicatorSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Indicator")
	FLinearColor IndicatorColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Indicator")
	FLinearColor HostileColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Indicator")
	FLinearColor FriendlyColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Indicator")
	float PulseSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Indicator")
	bool bShowDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Indicator")
	bool bShowHealth;

	FTargetIndicatorConfig()
	{
		IndicatorSize = 64.0f;
		IndicatorColor = FLinearColor::Red;
		HostileColor = FLinearColor::Red;
		FriendlyColor = FLinearColor::Green;
		PulseSpeed = 2.0f;
		bShowDistance = true;
		bShowHealth = true;
	}
};

/**
 * Health bar configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FHealthBarConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FVector2D BarSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FVector OffsetFromActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor HealthyColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor DamagedColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	FLinearColor CriticalColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	bool bOnlyShowWhenDamaged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	float FadeOutDelay;

	FHealthBarConfig()
	{
		BarSize = FVector2D(60.0f, 8.0f);
		OffsetFromActor = FVector(0.0f, 0.0f, 100.0f);
		HealthyColor = FLinearColor::Green;
		DamagedColor = FLinearColor::Yellow;
		CriticalColor = FLinearColor::Red;
		bOnlyShowWhenDamaged = true;
		FadeOutDelay = 3.0f;
	}
};

/**
 * Damage number configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDamageNumberConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Numbers")
	float FontSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Numbers")
	FLinearColor NormalDamageColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Numbers")
	FLinearColor CriticalDamageColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Numbers")
	FLinearColor HealingColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Numbers")
	float AnimationDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Numbers")
	FVector AnimationDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Numbers")
	bool bShowDamageNumbers;

	FDamageNumberConfig()
	{
		FontSize = 24.0f;
		NormalDamageColor = FLinearColor::White;
		CriticalDamageColor = FLinearColor::Red;
		HealingColor = FLinearColor::Green;
		AnimationDuration = 1.5f;
		AnimationDirection = FVector(0.0f, 0.0f, 50.0f);
		bShowDamageNumbers = true;
	}
};

/**
 * Active UI element tracking
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCombatUIElement
{
	GENERATED_BODY()

	UPROPERTY()
	UUserWidget* Widget;

	UPROPERTY()
	AActor* TrackedActor;

	UPROPERTY()
	ECombatUIElement ElementType;

	UPROPERTY()
	float CreationTime;

	UPROPERTY()
	float LifeTime;

	UPROPERTY()
	bool bShouldDestroy;

	FCombatUIElement()
	{
		Widget = nullptr;
		TrackedActor = nullptr;
		ElementType = ECombatUIElement::TargetIndicator;
		CreationTime = 0.0f;
		LifeTime = 0.0f;
		bShouldDestroy = false;
	}
};

/**
 * Combat UI Component
 * 
 * Features:
 * - Touch-optimized target indicators with visual feedback
 * - Dynamic health bars above enemies
 * - Floating damage numbers with animations
 * - Weapon status indicators (charge, reload, ammo)
 * - Hit markers for touch feedback
 * - Performance-optimized UI pooling for mobile
 * - Integration with targeting and weapon systems
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCombatUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCombatUIComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Target Indicator Management
	// ============================================================================

	/**
	 * Show target indicator for an actor
	 * @param Target Actor to show indicator for
	 * @param bIsHostile Whether the target is hostile
	 */
	UFUNCTION(BlueprintCallable, Category = "Target Indicators")
	void ShowTargetIndicator(AActor* Target, bool bIsHostile = true);

	/**
	 * Hide target indicator for an actor
	 * @param Target Actor to hide indicator for
	 */
	UFUNCTION(BlueprintCallable, Category = "Target Indicators")
	void HideTargetIndicator(AActor* Target);

	/**
	 * Update target indicator position and appearance
	 * @param Target Actor being tracked
	 */
	UFUNCTION(BlueprintCallable, Category = "Target Indicators")
	void UpdateTargetIndicator(AActor* Target);

	/**
	 * Hide all target indicators
	 */
	UFUNCTION(BlueprintCallable, Category = "Target Indicators")
	void HideAllTargetIndicators();

	// ============================================================================
	// Health Bar Management
	// ============================================================================

	/**
	 * Show health bar for an actor
	 * @param Target Actor to show health bar for
	 */
	UFUNCTION(BlueprintCallable, Category = "Health Bars")
	void ShowHealthBar(AActor* Target);

	/**
	 * Hide health bar for an actor
	 * @param Target Actor to hide health bar for
	 */
	UFUNCTION(BlueprintCallable, Category = "Health Bars")
	void HideHealthBar(AActor* Target);

	/**
	 * Update health bar for an actor
	 * @param Target Actor whose health bar to update
	 * @param HealthPercentage Current health as percentage (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Health Bars")
	void UpdateHealthBar(AActor* Target, float HealthPercentage);

	/**
	 * Hide all health bars
	 */
	UFUNCTION(BlueprintCallable, Category = "Health Bars")
	void HideAllHealthBars();

	// ============================================================================
	// Damage Numbers
	// ============================================================================

	/**
	 * Show floating damage number
	 * @param Location World location to show damage
	 * @param Damage Damage amount to display
	 * @param bIsCritical Whether this was a critical hit
	 * @param bIsHealing Whether this is healing instead of damage
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Numbers")
	void ShowDamageNumber(FVector Location, float Damage, bool bIsCritical = false, bool bIsHealing = false);

	/**
	 * Show damage number at actor location
	 * @param Target Actor to show damage at
	 * @param Damage Damage amount to display
	 * @param bIsCritical Whether this was a critical hit
	 * @param bIsHealing Whether this is healing instead of damage
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Numbers")
	void ShowDamageNumberAtActor(AActor* Target, float Damage, bool bIsCritical = false, bool bIsHealing = false);

	// ============================================================================
	// Weapon Status UI
	// ============================================================================

	/**
	 * Update weapon charge indicator
	 * @param ChargePercentage Charge level (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Status")
	void UpdateWeaponCharge(float ChargePercentage);

	/**
	 * Update weapon reload indicator
	 * @param ReloadPercentage Reload progress (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Status")
	void UpdateWeaponReload(float ReloadPercentage);

	/**
	 * Update weapon energy/ammo indicator
	 * @param EnergyPercentage Available energy (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Status")
	void UpdateWeaponEnergy(float EnergyPercentage);

	/**
	 * Show weapon status indicators
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Status")
	void ShowWeaponStatus();

	/**
	 * Hide weapon status indicators
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon Status")
	void HideWeaponStatus();

	// ============================================================================
	// Hit Markers and Touch Feedback
	// ============================================================================

	/**
	 * Show hit marker at screen location
	 * @param ScreenLocation Screen coordinates for hit marker
	 * @param bWasCritical Whether this was a critical hit
	 */
	UFUNCTION(BlueprintCallable, Category = "Hit Markers")
	void ShowHitMarker(FVector2D ScreenLocation, bool bWasCritical = false);

	/**
	 * Show hit marker for world location
	 * @param WorldLocation World coordinates for hit marker
	 * @param bWasCritical Whether this was a critical hit
	 */
	UFUNCTION(BlueprintCallable, Category = "Hit Markers")
	void ShowHitMarkerAtLocation(FVector WorldLocation, bool bWasCritical = false);

	/**
	 * Show touch feedback at screen location
	 * @param TouchLocation Screen coordinates for touch feedback
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Feedback")
	void ShowTouchFeedback(FVector2D TouchLocation);

	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * Set target indicator configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetTargetIndicatorConfig(const FTargetIndicatorConfig& Config);

	/**
	 * Set health bar configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetHealthBarConfig(const FHealthBarConfig& Config);

	/**
	 * Set damage number configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetDamageNumberConfig(const FDamageNumberConfig& Config);

	/**
	 * Enable or disable specific UI elements
	 */
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetUIElementEnabled(ECombatUIElement ElementType, bool bEnabled);

	// ============================================================================
	// Integration with Combat Systems
	// ============================================================================

	/**
	 * Set the targeting component to monitor
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetTargetingComponent(UOdysseyCombatTargetingComponent* TargetingComp);

	/**
	 * Set the weapon component to monitor
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetWeaponComponent(UOdysseyCombatWeaponComponent* WeaponComp);

	// ============================================================================
	// Blueprint Events
	// ============================================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnTargetIndicatorShown(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnTargetIndicatorHidden(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnDamageNumberShown(float Damage, bool bIsCritical);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI Events")
	void OnHitMarkerShown(bool bWasCritical);

protected:
	// ============================================================================
	// Configuration Properties
	// ============================================================================

	/**
	 * Target indicator configuration
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Indicators")
	FTargetIndicatorConfig TargetIndicatorConfig;

	/**
	 * Health bar configuration
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bars")
	FHealthBarConfig HealthBarConfig;

	/**
	 * Damage number configuration
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Numbers")
	FDamageNumberConfig DamageNumberConfig;

	/**
	 * Widget classes for UI elements
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Classes")
	TSubclassOf<UUserWidget> TargetIndicatorWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Classes")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Classes")
	TSubclassOf<UUserWidget> DamageNumberWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Classes")
	TSubclassOf<UUserWidget> HitMarkerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Classes")
	TSubclassOf<UUserWidget> WeaponStatusWidgetClass;

	/**
	 * UI element enable flags
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Elements")
	bool bShowTargetIndicators;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Elements")
	bool bShowHealthBars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Elements")
	bool bShowDamageNumbers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Elements")
	bool bShowHitMarkers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Elements")
	bool bShowWeaponStatus;

	/**
	 * Performance settings
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	int32 MaxTargetIndicators;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	int32 MaxHealthBars;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	int32 MaxDamageNumbers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
	float UIUpdateFrequency;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * Active UI elements
	 */
	UPROPERTY()
	TArray<FCombatUIElement> ActiveUIElements;

	/**
	 * Widget pools for performance
	 */
	UPROPERTY()
	TArray<UUserWidget*> TargetIndicatorPool;

	UPROPERTY()
	TArray<UUserWidget*> HealthBarPool;

	UPROPERTY()
	TArray<UUserWidget*> DamageNumberPool;

	UPROPERTY()
	TArray<UUserWidget*> HitMarkerPool;

	/**
	 * Component references
	 */
	UPROPERTY()
	UOdysseyCombatTargetingComponent* TargetingComponent;

	UPROPERTY()
	UOdysseyCombatWeaponComponent* WeaponComponent;

	/**
	 * Weapon status widget instance
	 */
	UPROPERTY()
	UUserWidget* WeaponStatusWidget;

	/**
	 * Update timing
	 */
	float LastUIUpdateTime;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Initialize UI system
	 */
	void InitializeUI();

	/**
	 * Shutdown UI system
	 */
	void ShutdownUI();

	/**
	 * Update all active UI elements
	 */
	void UpdateActiveUIElements(float DeltaTime);

	/**
	 * Update UI element positions
	 */
	void UpdateUIElementPositions();

	/**
	 * Clean up expired UI elements
	 */
	void CleanupExpiredElements();

	/**
	 * Get widget from pool or create new one
	 */
	UUserWidget* GetPooledWidget(ECombatUIElement ElementType);

	/**
	 * Return widget to pool
	 */
	void ReturnWidgetToPool(UUserWidget* Widget, ECombatUIElement ElementType);

	/**
	 * Create widget pools
	 */
	void InitializeWidgetPools();

	/**
	 * Cleanup widget pools
	 */
	void CleanupWidgetPools();

	/**
	 * Convert world location to screen coordinates
	 */
	bool WorldToScreen(FVector WorldLocation, FVector2D& ScreenLocation);

	/**
	 * Find active UI element for actor
	 */
	FCombatUIElement* FindUIElement(AActor* Target, ECombatUIElement ElementType);

	/**
	 * Remove UI element
	 */
	void RemoveUIElement(AActor* Target, ECombatUIElement ElementType);

	/**
	 * Handle targeting events
	 */
	void OnTargetChanged(AActor* NewTarget);

	/**
	 * Handle health change events
	 */
	void OnHealthChanged(AActor* Actor, float HealthPercentage);

	/**
	 * Handle damage events
	 */
	void OnDamageDealt(AActor* Target, float Damage, bool bIsCritical);

	/**
	 * Handle weapon events
	 */
	void OnWeaponFired();
	void OnWeaponCharged(float ChargeLevel);
	void OnWeaponReloading(float ReloadProgress);
};
