// OdysseyActionButton.h
// Action button manager component with event-driven architecture
// Refactored to use the Odyssey event system for extensible action dispatch

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyActionEvent.h"
#include "OdysseyActionButton.generated.h"

// Forward declarations
class UOdysseyEventBus;
class UOdysseyActionDispatcher;

UENUM(BlueprintType)
enum class EActionButtonType : uint8
{
	None = 0,
	Interact = 1,
	Cargo = 2,
	Scout = 3,
	Attack = 4,
	SpecialAttack = 5,
	ThrusterBoost = 6,

	// Extended action types for scalability
	Custom1 = 10,
	Custom2 = 11,
	Custom3 = 12,
	Custom4 = 13,
	Custom5 = 14,

	Max = 255
};

UENUM(BlueprintType)
enum class EButtonState : uint8
{
	Available,
	OnCooldown,
	Disabled,
	Charging,
	Executing	// New state for actions in progress
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FActionButtonData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	EActionButtonType ButtonType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	FString ButtonName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	FVector2D Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	FVector2D Size;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	float CooldownDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	float ChargeDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	int32 EnergyCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	bool bRequiresTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	bool bIsToggle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	FLinearColor ButtonColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	FLinearColor CooldownColor;

	// New: Handler name for custom action mapping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Button")
	FName CustomHandlerName;

	FActionButtonData()
	{
		ButtonType = EActionButtonType::None;
		ButtonName = TEXT("Action");
		Description = TEXT("Perform action");
		Position = FVector2D(0, 0);
		Size = FVector2D(100, 100);
		CooldownDuration = 1.0f;
		ChargeDuration = 0.0f;
		EnergyCost = 10;
		bRequiresTarget = false;
		bIsToggle = false;
		ButtonColor = FLinearColor::Blue;
		CooldownColor = FLinearColor::Gray;
		CustomHandlerName = NAME_None;
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FActionButtonState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Button State")
	EActionButtonType ButtonType;

	UPROPERTY(BlueprintReadOnly, Category = "Button State")
	EButtonState State;

	UPROPERTY(BlueprintReadOnly, Category = "Button State")
	float RemainingCooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Button State")
	float ChargeProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Button State")
	bool bIsPressed;

	UPROPERTY(BlueprintReadOnly, Category = "Button State")
	bool bIsToggled;

	// New: Track cooldown total for progress calculation
	UPROPERTY(BlueprintReadOnly, Category = "Button State")
	float TotalCooldownDuration;

	FActionButtonState()
	{
		ButtonType = EActionButtonType::None;
		State = EButtonState::Available;
		RemainingCooldown = 0.0f;
		ChargeProgress = 0.0f;
		bIsPressed = false;
		bIsToggled = false;
		TotalCooldownDuration = 0.0f;
	}
};

/**
 * Action Button Manager Component
 *
 * Refactored to use the event-driven action system:
 * - Replaces switch-case dispatch with event publishing
 * - Uses O(1) button lookup via hash map
 * - Integrates with OdysseyEventBus for thread-safe action handling
 * - Event-driven cooldown and energy management
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyActionButtonManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyActionButtonManager();

protected:
	// Button configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Buttons")
	TArray<FActionButtonData> ActionButtons;

	// Runtime state - now using hash map for O(1) lookup
	UPROPERTY(BlueprintReadOnly, Category = "Button State")
	TArray<FActionButtonState> ButtonStates;

	// O(1) lookup map: ButtonType -> Index in ButtonStates array
	TMap<uint8, int32> ButtonTypeToIndex;

	// Player resources
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	float CurrentEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	float MaxEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	float EnergyRegenRate;

	// Active abilities
	UPROPERTY(BlueprintReadOnly, Category = "Active Abilities")
	bool bThrusterBoostActive;

	UPROPERTY(BlueprintReadOnly, Category = "Active Abilities")
	float ThrusterBoostTimeRemaining;

	UPROPERTY(BlueprintReadOnly, Category = "Active Abilities")
	bool bScoutModeActive;

	UPROPERTY(BlueprintReadOnly, Category = "Active Abilities")
	bool bAttackModeActive;

	// Event system integration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event System")
	bool bUseEventSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event System")
	bool bUseEventDrivenCooldowns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event System")
	bool bPublishCooldownTickEvents;

	// Event bus reference
	UPROPERTY()
	UOdysseyEventBus* EventBus;

	// Action dispatcher reference
	UPROPERTY()
	UOdysseyActionDispatcher* ActionDispatcher;

	// Event subscription handles
	TArray<FOdysseyEventHandle> EventHandles;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Button Management
	// ============================================================================

	/**
	 * Activate a button action
	 * Now uses event system for dispatch instead of switch-case
	 */
	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool ActivateButton(EActionButtonType ButtonType);

	/**
	 * Activate button with target
	 */
	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool ActivateButtonWithTarget(EActionButtonType ButtonType, AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	void StartCooldown(EActionButtonType ButtonType);

	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool IsButtonAvailable(EActionButtonType ButtonType) const;

	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	float GetButtonCooldownProgress(EActionButtonType ButtonType) const;

	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	FActionButtonState GetButtonState(EActionButtonType ButtonType) const;

	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	TArray<FActionButtonData> GetActionButtons() const { return ActionButtons; }

	/**
	 * Register a new button at runtime
	 */
	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool RegisterButton(const FActionButtonData& ButtonData);

	/**
	 * Unregister a button at runtime
	 */
	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool UnregisterButton(EActionButtonType ButtonType);

	// ============================================================================
	// Energy Management (Event-Driven)
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Energy")
	bool CanAffordAction(EActionButtonType ButtonType) const;

	UFUNCTION(BlueprintCallable, Category = "Energy")
	bool SpendEnergy(int32 Amount);

	/**
	 * Add energy (from pickups, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Energy")
	void AddEnergy(float Amount, FName Reason = NAME_None);

	/**
	 * Set energy directly (with event publishing)
	 */
	UFUNCTION(BlueprintCallable, Category = "Energy")
	void SetEnergy(float NewEnergy, FName Reason = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Energy")
	float GetEnergyPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Energy")
	float GetCurrentEnergy() const { return CurrentEnergy; }

	UFUNCTION(BlueprintCallable, Category = "Energy")
	float GetMaxEnergy() const { return MaxEnergy; }

	// ============================================================================
	// Action Implementations (kept for backward compatibility)
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Ship Actions")
	void ExecuteInteract();

	UFUNCTION(BlueprintCallable, Category = "Ship Actions")
	void ExecuteOpenCargo();

	UFUNCTION(BlueprintCallable, Category = "Ship Actions")
	void ExecuteScoutMode();

	UFUNCTION(BlueprintCallable, Category = "Ship Actions")
	void ExecuteAttack();

	UFUNCTION(BlueprintCallable, Category = "Ship Actions")
	void ExecuteSpecialAttack();

	UFUNCTION(BlueprintCallable, Category = "Ship Actions")
	void ExecuteThrusterBoost();

	// ============================================================================
	// Ability State Getters
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool IsThrusterBoostActive() const { return bThrusterBoostActive; }

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool IsScoutModeActive() const { return bScoutModeActive; }

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool IsAttackModeActive() const { return bAttackModeActive; }

	// ============================================================================
	// Touch Input Integration
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	EActionButtonType GetButtonAtPosition(FVector2D TouchPosition) const;

	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	bool HandleButtonTouch(FVector2D TouchPosition);

	// ============================================================================
	// Event System Integration
	// ============================================================================

	/**
	 * Get the event bus (creates if needed)
	 */
	UFUNCTION(BlueprintCallable, Category = "Event System")
	UOdysseyEventBus* GetEventBus();

	/**
	 * Get the action dispatcher (creates if needed)
	 */
	UFUNCTION(BlueprintCallable, Category = "Event System")
	UOdysseyActionDispatcher* GetActionDispatcher();

	/**
	 * Enable/disable the event system
	 */
	UFUNCTION(BlueprintCallable, Category = "Event System")
	void SetUseEventSystem(bool bEnabled);

	// ============================================================================
	// Blueprint Events (kept for backward compatibility)
	// ============================================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Events")
	void OnButtonPressed(EActionButtonType ButtonType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Events")
	void OnButtonCooldownStarted(EActionButtonType ButtonType, float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Events")
	void OnButtonCooldownCompleted(EActionButtonType ButtonType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Events")
	void OnThrusterBoostActivated(float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Events")
	void OnThrusterBoostDeactivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Events")
	void OnScoutModeToggled(bool bActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Events")
	void OnAttackModeToggled(bool bActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Events")
	void OnEnergyChanged(float NewEnergy, float OldMaxEnergy);

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Events")
	void OnActionFailed(EActionButtonType ButtonType, EActionFailureReason Reason);

private:
	// Initialization
	void InitializeButtons();
	void InitializeEventSystem();
	void ShutdownEventSystem();
	void BuildButtonLookupMap();

	// Event handlers
	void OnActionExecutedEvent(const FOdysseyEventPayload& Payload);
	void OnActionFailedEvent(const FOdysseyEventPayload& Payload);
	void OnCooldownStartedEvent(const FOdysseyEventPayload& Payload);
	void OnCooldownCompletedEvent(const FOdysseyEventPayload& Payload);
	void OnEnergyChangedEvent(const FOdysseyEventPayload& Payload);
	void OnAbilityStateChangedEvent(const FOdysseyEventPayload& Payload);

	// Legacy update methods (used when event system is disabled)
	void UpdateCooldowns(float DeltaTime);
	void UpdateActiveAbilities(float DeltaTime);
	void RegenerateEnergy(float DeltaTime);

	// Legacy dispatch (used when event system is disabled)
	bool DispatchActionLegacy(EActionButtonType ButtonType);

	// O(1) lookup
	int32 FindButtonIndex(EActionButtonType ButtonType) const;
	FActionButtonData GetButtonData(EActionButtonType ButtonType) const;

	// Energy event publishing
	void PublishEnergyChange(float OldEnergy, FName Reason);
};
