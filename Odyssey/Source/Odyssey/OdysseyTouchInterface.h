// OdysseyTouchInterface.h
// Touch interface component with event-driven action routing
// Properly integrates with the Odyssey event system for thread-safe multi-touch

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/TouchInterface.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyActionEvent.h"
#include "OdysseyTouchInterface.generated.h"

// Forward declarations
class UOdysseyActionButtonManager;
class UOdysseyEventBus;
class UOdysseyActionDispatcher;

/**
 * Touch input event for queuing
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FQueuedTouchEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Touch Event")
	FVector2D Location;

	UPROPERTY(BlueprintReadOnly, Category = "Touch Event")
	int32 FingerIndex;

	UPROPERTY(BlueprintReadOnly, Category = "Touch Event")
	double Timestamp;

	UPROPERTY(BlueprintReadOnly, Category = "Touch Event")
	uint8 EventType;	// 0 = Start, 1 = Move, 2 = End

	FQueuedTouchEvent()
		: Location(FVector2D::ZeroVector)
		, FingerIndex(-1)
		, Timestamp(0.0)
		, EventType(0)
	{
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FOdysseyTouchControl
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
	FString ControlName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
	FVector2D Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
	FVector2D Size;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
	float Opacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Control")
	bool bVisible;

	FOdysseyTouchControl()
	{
		ControlName = TEXT("DefaultControl");
		Position = FVector2D(0.0f, 0.0f);
		Size = FVector2D(100.0f, 100.0f);
		Opacity = 0.7f;
		bVisible = true;
	}
};

/**
 * Touch Interface Component
 *
 * Refactored to use event-driven action routing:
 * - Publishes touch events to the event bus
 * - Routes action button touches through the dispatcher
 * - Thread-safe touch event queuing for multi-touch scenarios
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API UOdysseyTouchInterface : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyTouchInterface();

protected:
	// Touch controls configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Controls")
	TArray<FOdysseyTouchControl> TouchControls;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Controls")
	float JoystickRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Controls")
	float JoystickDeadZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Controls")
	float TouchSensitivity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Touch Controls")
	bool bShowTouchControls;

	// Event system settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event System")
	bool bUseEventDispatcher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event System")
	bool bQueueTouchEvents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event System")
	int32 MaxQueuedTouchEvents;

	// Current touch state
	UPROPERTY(BlueprintReadOnly, Category = "Touch State")
	bool bIsJoystickActive;

	UPROPERTY(BlueprintReadOnly, Category = "Touch State")
	FVector2D JoystickCenter;

	UPROPERTY(BlueprintReadOnly, Category = "Touch State")
	FVector2D JoystickPosition;

	UPROPERTY(BlueprintReadOnly, Category = "Touch State")
	FVector2D MovementInput;

	// Component references
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	UOdysseyActionButtonManager* ActionButtonManager;

	UPROPERTY()
	UOdysseyEventBus* EventBus;

	UPROPERTY()
	UOdysseyActionDispatcher* ActionDispatcher;

	// Touch event queue for thread-safe processing
	TArray<FQueuedTouchEvent> TouchEventQueue;
	mutable FCriticalSection TouchQueueLock;

	// Event subscription handles
	TArray<FOdysseyEventHandle> EventHandles;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Touch Input Handling
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	void HandleTouchStarted(FVector2D TouchLocation, int32 FingerIndex);

	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	void HandleTouchMoved(FVector2D TouchLocation, int32 FingerIndex);

	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	void HandleTouchEnded(FVector2D TouchLocation, int32 FingerIndex);

	/**
	 * Process queued touch events (call from game thread)
	 * @param MaxEvents Maximum events to process (0 = all)
	 * @return Number of events processed
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	int32 ProcessQueuedTouchEvents(int32 MaxEvents = 0);

	// ============================================================================
	// Virtual Joystick Functions
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Virtual Joystick")
	void StartJoystickInput(FVector2D TouchLocation);

	UFUNCTION(BlueprintCallable, Category = "Virtual Joystick")
	void UpdateJoystickInput(FVector2D TouchLocation);

	UFUNCTION(BlueprintCallable, Category = "Virtual Joystick")
	void EndJoystickInput();

	UFUNCTION(BlueprintCallable, Category = "Virtual Joystick")
	FVector2D GetNormalizedJoystickInput() const;

	// ============================================================================
	// Touch Controls Management
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Touch Controls")
	void SetTouchControlsVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Touch Controls")
	void UpdateControlOpacity(float NewOpacity);

	UFUNCTION(BlueprintCallable, Category = "Touch Controls")
	bool IsPointInControl(FVector2D Point, const FOdysseyTouchControl& Control);

	// ============================================================================
	// Action Button Integration
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	void SetActionButtonManager(UOdysseyActionButtonManager* NewManager);

	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool HandleActionButtonTouch(FVector2D TouchLocation);

	/**
	 * Handle action button touch with target (for targeted abilities)
	 */
	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool HandleActionButtonTouchWithTarget(FVector2D TouchLocation, AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Action Buttons")
	bool IsActionButtonArea(FVector2D TouchLocation) const;

	// ============================================================================
	// Event System Integration
	// ============================================================================

	/**
	 * Get the event bus
	 */
	UFUNCTION(BlueprintCallable, Category = "Event System")
	UOdysseyEventBus* GetEventBus();

	/**
	 * Get the action dispatcher
	 */
	UFUNCTION(BlueprintCallable, Category = "Event System")
	UOdysseyActionDispatcher* GetActionDispatcher();

	/**
	 * Enable/disable event dispatcher routing
	 */
	UFUNCTION(BlueprintCallable, Category = "Event System")
	void SetUseEventDispatcher(bool bEnabled);

	// ============================================================================
	// Blueprint Events
	// ============================================================================

	UFUNCTION(BlueprintImplementableEvent, Category = "Touch Events")
	void OnMovementInputChanged(FVector2D NewInput);

	UFUNCTION(BlueprintImplementableEvent, Category = "Touch Events")
	void OnInteractPressed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Touch Events")
	void OnTouchControlPressed(const FString& ControlName);

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Button Events")
	void OnActionButtonPressed(int32 ButtonType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Action Button Events")
	void OnActionButtonFailed(int32 ButtonType, EActionFailureReason Reason);

private:
	// Internal touch processing
	void ProcessTouchEvent(const FQueuedTouchEvent& TouchEvent);
	void ProcessTouchStarted(FVector2D TouchLocation, int32 FingerIndex);
	void ProcessTouchMoved(FVector2D TouchLocation, int32 FingerIndex);
	void ProcessTouchEnded(FVector2D TouchLocation, int32 FingerIndex);

	// Event system initialization
	void InitializeEventSystem();
	void ShutdownEventSystem();

	// Event handlers
	void OnActionFailedEvent(const FOdysseyEventPayload& Payload);

	// Touch state tracking
	int32 JoystickFingerIndex;
	TMap<int32, FVector2D> ActiveTouches;
};
