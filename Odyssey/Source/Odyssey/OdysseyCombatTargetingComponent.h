// OdysseyCombatTargetingComponent.h
// Touch-based targeting system for mobile combat
// Integrates with existing action button system and event architecture

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Camera/CameraComponent.h"
#include "OdysseyActionEvent.h"
#include "OdysseyCombatTargetingComponent.generated.h"

// Forward declarations
class UOdysseyEventBus;
class UOdysseyCombatWeaponComponent;
class UNPCHealthComponent;
class AOdysseyCharacter;

/**
 * Target priority enumeration for automatic target selection
 */
UENUM(BlueprintType)
enum class ETargetPriority : uint8
{
	None = 0,
	Low = 1,        // Neutral NPCs, resources
	Medium = 2,     // Hostile NPCs at medium range
	High = 3,       // Hostile NPCs in close range
	Critical = 4    // Attacking NPCs, low health enemies
};

/**
 * Targeting mode for different combat situations
 */
UENUM(BlueprintType)
enum class ETargetingMode : uint8
{
	Manual,         // Player must touch to select targets
	Assisted,       // Auto-target nearby enemies, manual selection for specific targets
	Automatic       // Fully automatic targeting of nearest valid target
};

/**
 * Target validation result
 */
UENUM(BlueprintType)
enum class ETargetValidation : uint8
{
	Valid,
	OutOfRange,
	NoLineOfSight,
	InvalidActor,
	SameTeam,
	Dead,
	Custom
};

/**
 * Target information structure
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTargetInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
	AActor* TargetActor;

	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
	FVector TargetLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
	float DistanceToTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
	ETargetPriority Priority;

	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
	float HealthPercentage;

	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
	bool bHasLineOfSight;

	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
	bool bIsHostile;

	UPROPERTY(BlueprintReadOnly, Category = "Target Info")
	double SelectionTime;

	FTargetInfo()
	{
		TargetActor = nullptr;
		TargetLocation = FVector::ZeroVector;
		DistanceToTarget = 0.0f;
		Priority = ETargetPriority::None;
		HealthPercentage = 1.0f;
		bHasLineOfSight = false;
		bIsHostile = false;
		SelectionTime = 0.0;
	}
};

/**
 * Touch targeting result
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTouchTargetResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Touch Result")
	bool bValidTouch;

	UPROPERTY(BlueprintReadOnly, Category = "Touch Result")
	AActor* TouchedActor;

	UPROPERTY(BlueprintReadOnly, Category = "Touch Result")
	FVector TouchWorldLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Touch Result")
	FVector2D TouchScreenLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Touch Result")
	ETargetValidation ValidationResult;

	FTouchTargetResult()
	{
		bValidTouch = false;
		TouchedActor = nullptr;
		TouchWorldLocation = FVector::ZeroVector;
		TouchScreenLocation = FVector2D::ZeroVector;
		ValidationResult = ETargetValidation::InvalidActor;
	}
};

/**
 * Target change event payload
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTargetChangeEventPayload : public FOdysseyEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Target Event")
	AActor* PreviousTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Target Event")
	AActor* NewTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Target Event")
	ETargetingMode TargetingMode;

	UPROPERTY(BlueprintReadOnly, Category = "Target Event")
	bool bIsAutoTarget;

	FTargetChangeEventPayload()
	{
		PreviousTarget = nullptr;
		NewTarget = nullptr;
		TargetingMode = ETargetingMode::Manual;
		bIsAutoTarget = false;
	}
};

/**
 * Combat Targeting Component
 * 
 * Manages touch-based targeting for mobile combat:
 * - Touch-to-select enemy ships
 * - Automatic target prioritization 
 * - Line of sight validation
 * - Integration with action button system
 * - Event-driven architecture for weapon systems
 * - Mobile performance optimization
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyCombatTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyCombatTargetingComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Touch Targeting Interface
	// ============================================================================

	/**
	 * Handle touch input for target selection
	 * @param TouchLocation Screen coordinates of touch
	 * @return Result of touch targeting attempt
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Targeting")
	FTouchTargetResult HandleTouchTargeting(FVector2D TouchLocation);

	/**
	 * Perform screen-to-world raycast for touch targeting
	 * @param ScreenLocation Screen coordinates
	 * @param WorldLocation Out parameter for world location
	 * @param HitActor Out parameter for hit actor
	 * @return True if raycast hit something
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Targeting")
	bool ScreenToWorldRaycast(FVector2D ScreenLocation, FVector& WorldLocation, AActor*& HitActor);

	/**
	 * Select target actor directly
	 * @param TargetActor Actor to target
	 * @param bValidateTarget Whether to run validation checks
	 * @return True if target was successfully selected
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	bool SelectTarget(AActor* TargetActor, bool bValidateTarget = true);

	/**
	 * Clear current target
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void ClearTarget();

	// ============================================================================
	// Automatic Targeting
	// ============================================================================

	/**
	 * Find and select the best available target automatically
	 * @return True if a target was selected
	 */
	UFUNCTION(BlueprintCallable, Category = "Auto Targeting")
	bool AutoSelectTarget();

	/**
	 * Get the best target from available candidates
	 * @param Candidates List of potential targets
	 * @return Best target, or nullptr if none suitable
	 */
	UFUNCTION(BlueprintCallable, Category = "Auto Targeting")
	AActor* GetBestTarget(const TArray<AActor*>& Candidates);

	/**
	 * Find all potential targets within range
	 * @return Array of potential target actors
	 */
	UFUNCTION(BlueprintCallable, Category = "Auto Targeting")
	TArray<AActor*> FindPotentialTargets();

	/**
	 * Calculate target priority for automatic selection
	 * @param TargetActor Actor to evaluate
	 * @return Priority level for the target
	 */
	UFUNCTION(BlueprintCallable, Category = "Auto Targeting")
	ETargetPriority CalculateTargetPriority(AActor* TargetActor);

	// ============================================================================
	// Target Validation
	// ============================================================================

	/**
	 * Validate if an actor can be targeted
	 * @param TargetActor Actor to validate
	 * @return Validation result
	 */
	UFUNCTION(BlueprintCallable, Category = "Target Validation")
	ETargetValidation ValidateTarget(AActor* TargetActor);

	/**
	 * Check if target is within maximum range
	 * @param TargetActor Actor to check
	 * @return True if within range
	 */
	UFUNCTION(BlueprintCallable, Category = "Target Validation")
	bool IsTargetInRange(AActor* TargetActor);

	/**
	 * Check line of sight to target
	 * @param TargetActor Actor to check
	 * @return True if clear line of sight
	 */
	UFUNCTION(BlueprintCallable, Category = "Target Validation")
	bool HasLineOfSightToTarget(AActor* TargetActor);

	/**
	 * Check if target is on the same team
	 * @param TargetActor Actor to check
	 * @return True if same team (should not target)
	 */
	UFUNCTION(BlueprintCallable, Category = "Target Validation")
	bool IsSameTeam(AActor* TargetActor);

	// ============================================================================
	// Target Information
	// ============================================================================

	/**
	 * Get current target actor
	 */
	UFUNCTION(BlueprintPure, Category = "Targeting")
	AActor* GetCurrentTarget() const { return CurrentTarget.TargetActor; }

	/**
	 * Get current target information
	 */
	UFUNCTION(BlueprintPure, Category = "Targeting")
	FTargetInfo GetCurrentTargetInfo() const { return CurrentTarget; }

	/**
	 * Check if we have a valid target
	 */
	UFUNCTION(BlueprintPure, Category = "Targeting")
	bool HasValidTarget() const;

	/**
	 * Get distance to current target
	 */
	UFUNCTION(BlueprintPure, Category = "Targeting")
	float GetDistanceToTarget() const { return CurrentTarget.DistanceToTarget; }

	/**
	 * Update target information (distance, line of sight, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	void UpdateTargetInformation();

	// ============================================================================
	// Targeting Configuration
	// ============================================================================

	/**
	 * Set targeting mode
	 */
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetTargetingMode(ETargetingMode NewMode);

	/**
	 * Get current targeting mode
	 */
	UFUNCTION(BlueprintPure, Category = "Configuration")
	ETargetingMode GetTargetingMode() const { return TargetingMode; }

	/**
	 * Set maximum targeting range
	 */
	UFUNCTION(BlueprintCallable, Category = "Configuration")
	void SetMaxTargetingRange(float NewRange);

	/**
	 * Get maximum targeting range
	 */
	UFUNCTION(BlueprintPure, Category = "Configuration")
	float GetMaxTargetingRange() const { return MaxTargetingRange; }

	// ============================================================================
	// Event System Integration
	// ============================================================================

	/**
	 * Get the event bus
	 */
	UFUNCTION(BlueprintCallable, Category = "Event System")
	UOdysseyEventBus* GetEventBus();

	// ============================================================================
	// Mobile UI Integration
	// ============================================================================

	/**
	 * Get screen position of current target (for UI indicators)
	 * @param ScreenPosition Out parameter for screen coordinates
	 * @return True if target is on screen
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool GetTargetScreenPosition(FVector2D& ScreenPosition);

	/**
	 * Check if target is visible on screen
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool IsTargetOnScreen();

	// ============================================================================
	// Blueprint Events
	// ============================================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Targeting Events")
	void OnTargetSelected(AActor* NewTarget);

	UFUNCTION(BlueprintImplementableEvent, Category = "Targeting Events")
	void OnTargetCleared(AActor* PreviousTarget);

	UFUNCTION(BlueprintImplementableEvent, Category = "Targeting Events")
	void OnAutoTargetFound(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Targeting Events")
	void OnTargetInvalidated(AActor* Target, ETargetValidation Reason);

	UFUNCTION(BlueprintImplementableEvent, Category = "Targeting Events")
	void OnTargetingModeChanged(ETargetingMode OldMode, ETargetingMode NewMode);

protected:
	// ============================================================================
	// Configuration Properties
	// ============================================================================

	/**
	 * Current targeting mode
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	ETargetingMode TargetingMode;

	/**
	 * Maximum range for targeting
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "100.0"))
	float MaxTargetingRange;

	/**
	 * Auto-targeting update frequency (seconds)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Targeting", meta = (ClampMin = "0.1"))
	float AutoTargetUpdateFrequency;

	/**
	 * Priority bias for closer targets
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Targeting", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DistancePriorityBias;

	/**
	 * Priority bias for low health targets
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Targeting", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float HealthPriorityBias;

	/**
	 * Collision channels to check for line of sight
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	TArray<TEnumAsByte<ECollisionChannel>> LineOfSightChannels;

	/**
	 * Actor tags to treat as valid targets
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	TArray<FName> ValidTargetTags;

	/**
	 * Actor tags to treat as invalid targets
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	TArray<FName> InvalidTargetTags;

	/**
	 * Team ID of the owning player
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	int32 PlayerTeamId;

	/**
	 * Whether to broadcast targeting events
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event System")
	bool bBroadcastTargetingEvents;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * Current target information
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	FTargetInfo CurrentTarget;

	/**
	 * Time of last auto-targeting update
	 */
	float LastAutoTargetUpdateTime;

	/**
	 * Cached camera component for screen-to-world conversion
	 */
	UPROPERTY()
	UCameraComponent* CachedCameraComponent;

	/**
	 * Event bus reference
	 */
	UPROPERTY()
	UOdysseyEventBus* EventBus;

	/**
	 * Event subscription handles
	 */
	TArray<FOdysseyEventHandle> EventHandles;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Initialize targeting system
	 */
	void InitializeTargeting();

	/**
	 * Shutdown targeting system
	 */
	void ShutdownTargeting();

	/**
	 * Update automatic targeting (called periodically)
	 */
	void UpdateAutoTargeting(float DeltaTime);

	/**
	 * Get camera component for screen-to-world conversion
	 */
	UCameraComponent* GetCameraComponent();

	/**
	 * Calculate target score for auto-targeting
	 */
	float CalculateTargetScore(AActor* TargetActor);

	/**
	 * Broadcast target change event
	 */
	void BroadcastTargetChangeEvent(AActor* PreviousTarget, AActor* NewTarget);

	/**
	 * Handle target death event
	 */
	void OnTargetDied(AActor* DiedActor);

	/**
	 * Subscribe to relevant events
	 */
	void InitializeEventSubscriptions();

	/**
	 * Clean up event subscriptions
	 */
	void CleanupEventSubscriptions();

	/**
	 * Handle actor died event
	 */
	void OnActorDiedEvent(const FOdysseyEventPayload& Payload);
};
