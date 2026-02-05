// OdysseyTouchInterface.cpp
// Touch interface implementation with event-driven action routing

#include "OdysseyTouchInterface.h"
#include "OdysseyActionButton.h"
#include "OdysseyEventBus.h"
#include "OdysseyActionDispatcher.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

UOdysseyTouchInterface::UOdysseyTouchInterface()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Default touch control settings
	JoystickRadius = 100.0f;
	JoystickDeadZone = 0.2f;
	TouchSensitivity = 1.0f;
	bShowTouchControls = true;

	// Event system settings
	bUseEventDispatcher = true;
	bQueueTouchEvents = true;
	MaxQueuedTouchEvents = 64;

	// Initialize touch state
	bIsJoystickActive = false;
	JoystickFingerIndex = -1;
	JoystickCenter = FVector2D::ZeroVector;
	JoystickPosition = FVector2D::ZeroVector;
	MovementInput = FVector2D::ZeroVector;

	// Initialize references
	ActionButtonManager = nullptr;
	EventBus = nullptr;
	ActionDispatcher = nullptr;

	// Set up default touch controls
	FOdysseyTouchControl JoystickControl;
	JoystickControl.ControlName = TEXT("Movement");
	JoystickControl.Position = FVector2D(150.0f, 700.0f);
	JoystickControl.Size = FVector2D(200.0f, 200.0f);
	JoystickControl.Opacity = 0.6f;
	JoystickControl.bVisible = true;
	TouchControls.Add(JoystickControl);

	FOdysseyTouchControl InteractButton;
	InteractButton.ControlName = TEXT("Interact");
	InteractButton.Position = FVector2D(1700.0f, 700.0f);
	InteractButton.Size = FVector2D(120.0f, 120.0f);
	InteractButton.Opacity = 0.7f;
	InteractButton.bVisible = true;
	TouchControls.Add(InteractButton);
}

void UOdysseyTouchInterface::BeginPlay()
{
	Super::BeginPlay();

	// Enable touch events on mobile
	FString Platform = UGameplayStatics::GetPlatformName();
	if (Platform == TEXT("Android") || Platform == TEXT("IOS"))
	{
		bShowTouchControls = true;
	}
	else
	{
		// Hide touch controls on desktop for testing (can be enabled manually)
		bShowTouchControls = false;
	}

	// Initialize event system
	if (bUseEventDispatcher)
	{
		InitializeEventSystem();
	}

	UE_LOG(LogTemp, Log, TEXT("Touch interface initialized with %d controls (EventDispatcher: %s)"),
		TouchControls.Num(), bUseEventDispatcher ? TEXT("Enabled") : TEXT("Disabled"));
}

void UOdysseyTouchInterface::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownEventSystem();

	Super::EndPlay(EndPlayReason);
}

void UOdysseyTouchInterface::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Process queued touch events
	if (bQueueTouchEvents)
	{
		ProcessQueuedTouchEvents();
	}

	// Update movement input based on joystick state
	if (bIsJoystickActive)
	{
		FVector2D NormalizedInput = GetNormalizedJoystickInput();
		if (!NormalizedInput.Equals(MovementInput, 0.01f))
		{
			MovementInput = NormalizedInput;
			OnMovementInputChanged(MovementInput);
		}
	}
}

// ============================================================================
// Event System Initialization
// ============================================================================

void UOdysseyTouchInterface::InitializeEventSystem()
{
	// Get event bus
	EventBus = UOdysseyEventBus::Get();

	// Get action dispatcher
	ActionDispatcher = UOdysseyActionDispatcher::Get();

	// Subscribe to action failed events for feedback
	if (EventBus)
	{
		EventHandles.Add(EventBus->Subscribe(
			EOdysseyEventType::ActionFailed,
			[this](const FOdysseyEventPayload& Payload) { OnActionFailedEvent(Payload); }
		));
	}
}

void UOdysseyTouchInterface::ShutdownEventSystem()
{
	// Unsubscribe from all events
	if (EventBus)
	{
		for (FOdysseyEventHandle& Handle : EventHandles)
		{
			EventBus->Unsubscribe(Handle);
		}
	}

	EventHandles.Empty();
}

void UOdysseyTouchInterface::OnActionFailedEvent(const FOdysseyEventPayload& Payload)
{
	const FActionFailedEventPayload* FailedPayload = static_cast<const FActionFailedEventPayload*>(&Payload);
	if (FailedPayload)
	{
		// Forward to Blueprint event
		OnActionButtonFailed(FailedPayload->ActionType, FailedPayload->FailureReason);
	}
}

// ============================================================================
// Touch Input Handling
// ============================================================================

void UOdysseyTouchInterface::HandleTouchStarted(FVector2D TouchLocation, int32 FingerIndex)
{
	if (bQueueTouchEvents)
	{
		// Queue touch event for thread-safe processing
		FQueuedTouchEvent Event;
		Event.Location = TouchLocation;
		Event.FingerIndex = FingerIndex;
		Event.Timestamp = FPlatformTime::Seconds();
		Event.EventType = 0;	// Start

		{
			FScopeLock Lock(&TouchQueueLock);

			// Enforce queue limit
			if (TouchEventQueue.Num() >= MaxQueuedTouchEvents)
			{
				UE_LOG(LogTemp, Warning, TEXT("Touch event queue full, dropping oldest event"));
				TouchEventQueue.RemoveAt(0);
			}

			TouchEventQueue.Add(Event);
		}
	}
	else
	{
		// Process immediately
		ProcessTouchStarted(TouchLocation, FingerIndex);
	}
}

void UOdysseyTouchInterface::HandleTouchMoved(FVector2D TouchLocation, int32 FingerIndex)
{
	if (bQueueTouchEvents)
	{
		FQueuedTouchEvent Event;
		Event.Location = TouchLocation;
		Event.FingerIndex = FingerIndex;
		Event.Timestamp = FPlatformTime::Seconds();
		Event.EventType = 1;	// Move

		{
			FScopeLock Lock(&TouchQueueLock);

			if (TouchEventQueue.Num() >= MaxQueuedTouchEvents)
			{
				// For move events, remove oldest move event of same finger
				for (int32 i = 0; i < TouchEventQueue.Num(); ++i)
				{
					if (TouchEventQueue[i].FingerIndex == FingerIndex && TouchEventQueue[i].EventType == 1)
					{
						TouchEventQueue.RemoveAt(i);
						break;
					}
				}
			}

			TouchEventQueue.Add(Event);
		}
	}
	else
	{
		ProcessTouchMoved(TouchLocation, FingerIndex);
	}
}

void UOdysseyTouchInterface::HandleTouchEnded(FVector2D TouchLocation, int32 FingerIndex)
{
	if (bQueueTouchEvents)
	{
		FQueuedTouchEvent Event;
		Event.Location = TouchLocation;
		Event.FingerIndex = FingerIndex;
		Event.Timestamp = FPlatformTime::Seconds();
		Event.EventType = 2;	// End

		{
			FScopeLock Lock(&TouchQueueLock);

			if (TouchEventQueue.Num() >= MaxQueuedTouchEvents)
			{
				TouchEventQueue.RemoveAt(0);
			}

			TouchEventQueue.Add(Event);
		}
	}
	else
	{
		ProcessTouchEnded(TouchLocation, FingerIndex);
	}
}

int32 UOdysseyTouchInterface::ProcessQueuedTouchEvents(int32 MaxEvents)
{
	TArray<FQueuedTouchEvent> EventsToProcess;

	{
		FScopeLock Lock(&TouchQueueLock);

		int32 NumToProcess = MaxEvents > 0 ? FMath::Min(MaxEvents, TouchEventQueue.Num()) : TouchEventQueue.Num();

		for (int32 i = 0; i < NumToProcess; ++i)
		{
			EventsToProcess.Add(TouchEventQueue[i]);
		}

		TouchEventQueue.RemoveAt(0, NumToProcess);
	}

	// Process outside lock
	for (const FQueuedTouchEvent& Event : EventsToProcess)
	{
		ProcessTouchEvent(Event);
	}

	return EventsToProcess.Num();
}

void UOdysseyTouchInterface::ProcessTouchEvent(const FQueuedTouchEvent& TouchEvent)
{
	switch (TouchEvent.EventType)
	{
		case 0:	// Start
			ProcessTouchStarted(TouchEvent.Location, TouchEvent.FingerIndex);
			break;
		case 1:	// Move
			ProcessTouchMoved(TouchEvent.Location, TouchEvent.FingerIndex);
			break;
		case 2:	// End
			ProcessTouchEnded(TouchEvent.Location, TouchEvent.FingerIndex);
			break;
	}
}

void UOdysseyTouchInterface::ProcessTouchStarted(FVector2D TouchLocation, int32 FingerIndex)
{
	ActiveTouches.Add(FingerIndex, TouchLocation);

	// First check if touch is in action button area
	if (IsActionButtonArea(TouchLocation))
	{
		bool ButtonHandled = HandleActionButtonTouch(TouchLocation);
		if (ButtonHandled)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Action button handled touch at %s with finger %d"), *TouchLocation.ToString(), FingerIndex);
			return;
		}
	}

	// Check which control was touched
	for (const FOdysseyTouchControl& Control : TouchControls)
	{
		if (Control.bVisible && IsPointInControl(TouchLocation, Control))
		{
			if (Control.ControlName == TEXT("Movement"))
			{
				StartJoystickInput(TouchLocation);
				JoystickFingerIndex = FingerIndex;
			}
			else if (Control.ControlName == TEXT("Interact"))
			{
				OnInteractPressed();

				// Also trigger interact action if we have a button manager
				if (ActionButtonManager)
				{
					ActionButtonManager->ActivateButton(EActionButtonType::Interact);
				}
			}
			else
			{
				OnTouchControlPressed(Control.ControlName);
			}
			break;
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("Touch started at %s with finger %d"), *TouchLocation.ToString(), FingerIndex);
}

void UOdysseyTouchInterface::ProcessTouchMoved(FVector2D TouchLocation, int32 FingerIndex)
{
	ActiveTouches.Add(FingerIndex, TouchLocation);

	// Update joystick if this is the joystick finger
	if (bIsJoystickActive && FingerIndex == JoystickFingerIndex)
	{
		UpdateJoystickInput(TouchLocation);
	}
}

void UOdysseyTouchInterface::ProcessTouchEnded(FVector2D TouchLocation, int32 FingerIndex)
{
	ActiveTouches.Remove(FingerIndex);

	// End joystick input if this was the joystick finger
	if (bIsJoystickActive && FingerIndex == JoystickFingerIndex)
	{
		EndJoystickInput();
		JoystickFingerIndex = -1;
	}

	UE_LOG(LogTemp, Verbose, TEXT("Touch ended at %s with finger %d"), *TouchLocation.ToString(), FingerIndex);
}

// ============================================================================
// Virtual Joystick Functions
// ============================================================================

void UOdysseyTouchInterface::StartJoystickInput(FVector2D TouchLocation)
{
	bIsJoystickActive = true;
	JoystickCenter = TouchLocation;
	JoystickPosition = TouchLocation;

	UE_LOG(LogTemp, Verbose, TEXT("Joystick started at %s"), *TouchLocation.ToString());
}

void UOdysseyTouchInterface::UpdateJoystickInput(FVector2D TouchLocation)
{
	if (!bIsJoystickActive)
		return;

	JoystickPosition = TouchLocation;

	// Clamp to joystick radius
	FVector2D Delta = JoystickPosition - JoystickCenter;
	float Distance = Delta.Size();

	if (Distance > JoystickRadius)
	{
		Delta = Delta.GetSafeNormal() * JoystickRadius;
		JoystickPosition = JoystickCenter + Delta;
	}
}

void UOdysseyTouchInterface::EndJoystickInput()
{
	if (!bIsJoystickActive)
		return;

	bIsJoystickActive = false;
	JoystickPosition = JoystickCenter;
	MovementInput = FVector2D::ZeroVector;

	// Notify of zero movement
	OnMovementInputChanged(MovementInput);

	UE_LOG(LogTemp, Verbose, TEXT("Joystick ended"));
}

FVector2D UOdysseyTouchInterface::GetNormalizedJoystickInput() const
{
	if (!bIsJoystickActive)
		return FVector2D::ZeroVector;

	FVector2D Delta = JoystickPosition - JoystickCenter;
	float Distance = Delta.Size();

	// Apply dead zone
	if (Distance < JoystickRadius * JoystickDeadZone)
		return FVector2D::ZeroVector;

	// Normalize and apply sensitivity
	FVector2D NormalizedInput = Delta.GetSafeNormal();
	float Magnitude = FMath::Clamp((Distance - JoystickRadius * JoystickDeadZone) / (JoystickRadius * (1.0f - JoystickDeadZone)), 0.0f, 1.0f);

	return NormalizedInput * Magnitude * TouchSensitivity;
}

// ============================================================================
// Touch Controls Management
// ============================================================================

void UOdysseyTouchInterface::SetTouchControlsVisible(bool bVisible)
{
	bShowTouchControls = bVisible;

	for (FOdysseyTouchControl& Control : TouchControls)
	{
		Control.bVisible = bVisible;
	}

	UE_LOG(LogTemp, Log, TEXT("Touch controls visibility set to %s"), bVisible ? TEXT("true") : TEXT("false"));
}

void UOdysseyTouchInterface::UpdateControlOpacity(float NewOpacity)
{
	for (FOdysseyTouchControl& Control : TouchControls)
	{
		Control.Opacity = FMath::Clamp(NewOpacity, 0.0f, 1.0f);
	}
}

bool UOdysseyTouchInterface::IsPointInControl(FVector2D Point, const FOdysseyTouchControl& Control)
{
	if (!Control.bVisible)
		return false;

	FVector2D ControlMin = Control.Position - (Control.Size * 0.5f);
	FVector2D ControlMax = Control.Position + (Control.Size * 0.5f);

	return (Point.X >= ControlMin.X && Point.X <= ControlMax.X &&
			Point.Y >= ControlMin.Y && Point.Y <= ControlMax.Y);
}

// ============================================================================
// Action Button Integration
// ============================================================================

void UOdysseyTouchInterface::SetActionButtonManager(UOdysseyActionButtonManager* NewManager)
{
	ActionButtonManager = NewManager;
	UE_LOG(LogTemp, Log, TEXT("Action Button Manager set on Touch Interface"));
}

bool UOdysseyTouchInterface::HandleActionButtonTouch(FVector2D TouchLocation)
{
	return HandleActionButtonTouchWithTarget(TouchLocation, nullptr);
}

bool UOdysseyTouchInterface::HandleActionButtonTouchWithTarget(FVector2D TouchLocation, AActor* Target)
{
	if (!ActionButtonManager)
	{
		return false;
	}

	// Get the button type at this position
	EActionButtonType ButtonType = ActionButtonManager->GetButtonAtPosition(TouchLocation);
	if (ButtonType == EActionButtonType::None)
	{
		return false;
	}

	bool bSuccess = false;

	if (bUseEventDispatcher && ActionDispatcher)
	{
		// Route through the event dispatcher
		bSuccess = ActionDispatcher->DispatchAction(
			static_cast<uint8>(ButtonType),
			GetOwner(),
			Target
		);

		if (bSuccess)
		{
			OnActionButtonPressed(static_cast<int32>(ButtonType));
		}
	}
	else
	{
		// Direct activation through button manager
		if (Target)
		{
			bSuccess = ActionButtonManager->ActivateButtonWithTarget(ButtonType, Target);
		}
		else
		{
			bSuccess = ActionButtonManager->ActivateButton(ButtonType);
		}

		if (bSuccess)
		{
			OnActionButtonPressed(static_cast<int32>(ButtonType));
		}
	}

	return bSuccess;
}

bool UOdysseyTouchInterface::IsActionButtonArea(FVector2D TouchLocation) const
{
	if (!ActionButtonManager)
		return false;

	// Check if touch location is in any action button area
	EActionButtonType ButtonType = ActionButtonManager->GetButtonAtPosition(TouchLocation);
	return ButtonType != EActionButtonType::None;
}

// ============================================================================
// Event System Access
// ============================================================================

UOdysseyEventBus* UOdysseyTouchInterface::GetEventBus()
{
	if (!EventBus)
	{
		EventBus = UOdysseyEventBus::Get();
	}
	return EventBus;
}

UOdysseyActionDispatcher* UOdysseyTouchInterface::GetActionDispatcher()
{
	if (!ActionDispatcher)
	{
		ActionDispatcher = UOdysseyActionDispatcher::Get();
	}
	return ActionDispatcher;
}

void UOdysseyTouchInterface::SetUseEventDispatcher(bool bEnabled)
{
	if (bUseEventDispatcher == bEnabled)
	{
		return;
	}

	bUseEventDispatcher = bEnabled;

	if (bEnabled)
	{
		InitializeEventSystem();
	}
	else
	{
		ShutdownEventSystem();
	}
}
