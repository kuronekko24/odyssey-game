// OdysseyActionButton.cpp
// Action button manager implementation with event-driven architecture

#include "OdysseyActionButton.h"
#include "OdysseyEventBus.h"
#include "OdysseyActionDispatcher.h"
#include "OdysseyActionCommand.h"
#include "Engine/Engine.h"

UOdysseyActionButtonManager::UOdysseyActionButtonManager()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Energy system defaults
	MaxEnergy = 100.0f;
	CurrentEnergy = MaxEnergy;
	EnergyRegenRate = 10.0f;

	// Active abilities
	bThrusterBoostActive = false;
	ThrusterBoostTimeRemaining = 0.0f;
	bScoutModeActive = false;
	bAttackModeActive = false;

	// Event system settings
	bUseEventSystem = true;
	bUseEventDrivenCooldowns = true;
	bPublishCooldownTickEvents = false;

	// References
	EventBus = nullptr;
	ActionDispatcher = nullptr;
}

void UOdysseyActionButtonManager::BeginPlay()
{
	Super::BeginPlay();

	InitializeButtons();

	if (bUseEventSystem)
	{
		InitializeEventSystem();
	}

	UE_LOG(LogTemp, Log, TEXT("Action Button Manager initialized with %d buttons (EventSystem: %s)"),
		ActionButtons.Num(), bUseEventSystem ? TEXT("Enabled") : TEXT("Disabled"));
}

void UOdysseyActionButtonManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownEventSystem();

	Super::EndPlay(EndPlayReason);
}

void UOdysseyActionButtonManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUseEventSystem)
	{
		// Process event bus
		if (EventBus)
		{
			EventBus->ProcessEvents();
		}

		// Tick dispatcher (handles cooldowns)
		if (ActionDispatcher)
		{
			ActionDispatcher->Tick(DeltaTime);
		}

		// Regenerate energy (event-driven)
		if (CurrentEnergy < MaxEnergy)
		{
			float OldEnergy = CurrentEnergy;
			CurrentEnergy = FMath::Min(MaxEnergy, CurrentEnergy + (EnergyRegenRate * DeltaTime));

			if (FMath::Abs(CurrentEnergy - OldEnergy) > KINDA_SMALL_NUMBER)
			{
				PublishEnergyChange(OldEnergy, FName("Regeneration"));
			}
		}

		// Update active abilities
		UpdateActiveAbilities(DeltaTime);
	}
	else
	{
		// Legacy tick behavior
		UpdateCooldowns(DeltaTime);
		UpdateActiveAbilities(DeltaTime);
		RegenerateEnergy(DeltaTime);
	}
}

// ============================================================================
// Event System Initialization
// ============================================================================

void UOdysseyActionButtonManager::InitializeEventSystem()
{
	// Get or create event bus
	EventBus = UOdysseyEventBus::Get();
	if (!EventBus)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get OdysseyEventBus"));
		return;
	}

	// Get or create dispatcher
	ActionDispatcher = UOdysseyActionDispatcher::Get();
	if (ActionDispatcher)
	{
		ActionDispatcher->SetButtonManager(this);
		ActionDispatcher->RegisterDefaultHandlers();
	}

	// Subscribe to events
	EventHandles.Add(EventBus->Subscribe(
		EOdysseyEventType::ActionExecuted,
		[this](const FOdysseyEventPayload& Payload) { OnActionExecutedEvent(Payload); }
	));

	EventHandles.Add(EventBus->Subscribe(
		EOdysseyEventType::ActionFailed,
		[this](const FOdysseyEventPayload& Payload) { OnActionFailedEvent(Payload); }
	));

	EventHandles.Add(EventBus->Subscribe(
		EOdysseyEventType::CooldownStarted,
		[this](const FOdysseyEventPayload& Payload) { OnCooldownStartedEvent(Payload); }
	));

	EventHandles.Add(EventBus->Subscribe(
		EOdysseyEventType::CooldownCompleted,
		[this](const FOdysseyEventPayload& Payload) { OnCooldownCompletedEvent(Payload); }
	));

	EventHandles.Add(EventBus->Subscribe(
		EOdysseyEventType::EnergyChanged,
		[this](const FOdysseyEventPayload& Payload) { OnEnergyChangedEvent(Payload); }
	));

	EventHandles.Add(EventBus->Subscribe(
		EOdysseyEventType::AbilityToggled,
		[this](const FOdysseyEventPayload& Payload) { OnAbilityStateChangedEvent(Payload); }
	));

	EventHandles.Add(EventBus->Subscribe(
		EOdysseyEventType::AbilityActivated,
		[this](const FOdysseyEventPayload& Payload) { OnAbilityStateChangedEvent(Payload); }
	));

	EventHandles.Add(EventBus->Subscribe(
		EOdysseyEventType::AbilityDeactivated,
		[this](const FOdysseyEventPayload& Payload) { OnAbilityStateChangedEvent(Payload); }
	));

	UE_LOG(LogTemp, Log, TEXT("Event system initialized with %d subscriptions"), EventHandles.Num());
}

void UOdysseyActionButtonManager::ShutdownEventSystem()
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

// ============================================================================
// Button Management
// ============================================================================

bool UOdysseyActionButtonManager::ActivateButton(EActionButtonType ButtonType)
{
	return ActivateButtonWithTarget(ButtonType, nullptr);
}

bool UOdysseyActionButtonManager::ActivateButtonWithTarget(EActionButtonType ButtonType, AActor* Target)
{
	// Pre-validation (same for both systems)
	if (!IsButtonAvailable(ButtonType))
	{
		if (bUseEventSystem && EventBus)
		{
			// Publish failure event
			auto Payload = MakeShared<FActionFailedEventPayload>();
			Payload->Initialize(EOdysseyEventType::ActionFailed, GetOwner());
			Payload->ActionType = static_cast<uint8>(ButtonType);
			Payload->FailureReason = EActionFailureReason::OnCooldown;
			Payload->RemainingCooldown = GetButtonState(ButtonType).RemainingCooldown;
			EventBus->PublishEvent(Payload);
		}
		return false;
	}

	if (!CanAffordAction(ButtonType))
	{
		if (bUseEventSystem && EventBus)
		{
			auto Payload = MakeShared<FActionFailedEventPayload>();
			Payload->Initialize(EOdysseyEventType::ActionFailed, GetOwner());
			Payload->ActionType = static_cast<uint8>(ButtonType);
			Payload->FailureReason = EActionFailureReason::InsufficientEnergy;
			FActionButtonData ButtonData = GetButtonData(ButtonType);
			Payload->RequiredEnergy = ButtonData.EnergyCost;
			Payload->CurrentEnergy = static_cast<int32>(CurrentEnergy);
			EventBus->PublishEvent(Payload);
		}
		return false;
	}

	if (bUseEventSystem && ActionDispatcher)
	{
		// Use event-driven dispatch
		FActionButtonData ButtonData = GetButtonData(ButtonType);

		// Spend energy first
		if (!SpendEnergy(ButtonData.EnergyCost))
		{
			return false;
		}

		// Dispatch through the action dispatcher
		bool bSuccess = ActionDispatcher->DispatchAction(
			static_cast<uint8>(ButtonType),
			GetOwner(),
			Target
		);

		if (bSuccess)
		{
			// Start cooldown via dispatcher
			ActionDispatcher->StartCooldown(
				static_cast<uint8>(ButtonType),
				FName(*ButtonData.ButtonName),
				ButtonData.CooldownDuration,
				bPublishCooldownTickEvents
			);

			OnButtonPressed(ButtonType);
		}

		return bSuccess;
	}
	else
	{
		// Legacy dispatch
		return DispatchActionLegacy(ButtonType);
	}
}

bool UOdysseyActionButtonManager::DispatchActionLegacy(EActionButtonType ButtonType)
{
	// Get button data for energy cost
	FActionButtonData ButtonData = GetButtonData(ButtonType);
	if (!SpendEnergy(ButtonData.EnergyCost))
	{
		return false;
	}

	// Execute the action (legacy switch-case pattern)
	switch (ButtonType)
	{
		case EActionButtonType::Interact:
			ExecuteInteract();
			break;
		case EActionButtonType::Cargo:
			ExecuteOpenCargo();
			break;
		case EActionButtonType::Scout:
			ExecuteScoutMode();
			break;
		case EActionButtonType::Attack:
			ExecuteAttack();
			break;
		case EActionButtonType::SpecialAttack:
			ExecuteSpecialAttack();
			break;
		case EActionButtonType::ThrusterBoost:
			ExecuteThrusterBoost();
			break;
		default:
			UE_LOG(LogTemp, Warning, TEXT("Unknown action type: %d"), static_cast<int32>(ButtonType));
			return false;
	}

	// Start cooldown
	StartCooldown(ButtonType);

	OnButtonPressed(ButtonType);

	UE_LOG(LogTemp, Log, TEXT("Activated button (legacy): %d"), static_cast<int32>(ButtonType));

	return true;
}

void UOdysseyActionButtonManager::StartCooldown(EActionButtonType ButtonType)
{
	int32 ButtonIndex = FindButtonIndex(ButtonType);
	if (ButtonIndex == -1)
	{
		return;
	}

	FActionButtonState& ButtonState = ButtonStates[ButtonIndex];
	FActionButtonData ButtonData = ActionButtons[ButtonIndex];

	ButtonState.State = EButtonState::OnCooldown;
	ButtonState.RemainingCooldown = ButtonData.CooldownDuration;
	ButtonState.TotalCooldownDuration = ButtonData.CooldownDuration;

	// Publish event if using event system
	if (bUseEventSystem && EventBus)
	{
		EventBus->PublishCooldownEvent(
			EOdysseyEventType::CooldownStarted,
			static_cast<uint8>(ButtonType),
			ButtonData.CooldownDuration,
			ButtonData.CooldownDuration
		);
	}

	OnButtonCooldownStarted(ButtonType, ButtonData.CooldownDuration);
}

bool UOdysseyActionButtonManager::IsButtonAvailable(EActionButtonType ButtonType) const
{
	// O(1) lookup using hash map
	const int32* IndexPtr = ButtonTypeToIndex.Find(static_cast<uint8>(ButtonType));
	if (IndexPtr && *IndexPtr >= 0 && *IndexPtr < ButtonStates.Num())
	{
		return ButtonStates[*IndexPtr].State == EButtonState::Available;
	}
	return false;
}

float UOdysseyActionButtonManager::GetButtonCooldownProgress(EActionButtonType ButtonType) const
{
	int32 ButtonIndex = FindButtonIndex(ButtonType);
	if (ButtonIndex != -1)
	{
		const FActionButtonState& ButtonState = ButtonStates[ButtonIndex];

		if (ButtonState.State == EButtonState::OnCooldown && ButtonState.TotalCooldownDuration > 0)
		{
			return 1.0f - (ButtonState.RemainingCooldown / ButtonState.TotalCooldownDuration);
		}
	}
	return 1.0f;
}

FActionButtonState UOdysseyActionButtonManager::GetButtonState(EActionButtonType ButtonType) const
{
	int32 ButtonIndex = FindButtonIndex(ButtonType);
	if (ButtonIndex != -1)
	{
		return ButtonStates[ButtonIndex];
	}
	return FActionButtonState();
}

bool UOdysseyActionButtonManager::RegisterButton(const FActionButtonData& ButtonData)
{
	// Check if button type already exists
	if (ButtonTypeToIndex.Contains(static_cast<uint8>(ButtonData.ButtonType)))
	{
		UE_LOG(LogTemp, Warning, TEXT("Button type %d already registered"), static_cast<int32>(ButtonData.ButtonType));
		return false;
	}

	// Add button data
	int32 NewIndex = ActionButtons.Add(ButtonData);

	// Add button state
	FActionButtonState NewState;
	NewState.ButtonType = ButtonData.ButtonType;
	NewState.State = EButtonState::Available;
	ButtonStates.Add(NewState);

	// Update lookup map
	ButtonTypeToIndex.Add(static_cast<uint8>(ButtonData.ButtonType), NewIndex);

	UE_LOG(LogTemp, Log, TEXT("Registered button: %s (type %d)"), *ButtonData.ButtonName, static_cast<int32>(ButtonData.ButtonType));

	return true;
}

bool UOdysseyActionButtonManager::UnregisterButton(EActionButtonType ButtonType)
{
	int32* IndexPtr = ButtonTypeToIndex.Find(static_cast<uint8>(ButtonType));
	if (!IndexPtr)
	{
		return false;
	}

	int32 Index = *IndexPtr;

	// Remove from arrays
	ActionButtons.RemoveAt(Index);
	ButtonStates.RemoveAt(Index);

	// Rebuild lookup map (indices shifted)
	BuildButtonLookupMap();

	UE_LOG(LogTemp, Log, TEXT("Unregistered button type %d"), static_cast<int32>(ButtonType));

	return true;
}

// ============================================================================
// Energy Management
// ============================================================================

bool UOdysseyActionButtonManager::CanAffordAction(EActionButtonType ButtonType) const
{
	FActionButtonData ButtonData = GetButtonData(ButtonType);
	return CurrentEnergy >= ButtonData.EnergyCost;
}

bool UOdysseyActionButtonManager::SpendEnergy(int32 Amount)
{
	if (CurrentEnergy >= Amount)
	{
		float OldEnergy = CurrentEnergy;
		CurrentEnergy = FMath::Max(0.0f, CurrentEnergy - Amount);

		if (FMath::Abs(CurrentEnergy - OldEnergy) > KINDA_SMALL_NUMBER)
		{
			PublishEnergyChange(OldEnergy, FName("ActionCost"));
			OnEnergyChanged(CurrentEnergy, MaxEnergy);
		}

		return true;
	}
	return false;
}

void UOdysseyActionButtonManager::AddEnergy(float Amount, FName Reason)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	float OldEnergy = CurrentEnergy;
	CurrentEnergy = FMath::Min(MaxEnergy, CurrentEnergy + Amount);

	if (FMath::Abs(CurrentEnergy - OldEnergy) > KINDA_SMALL_NUMBER)
	{
		PublishEnergyChange(OldEnergy, Reason.IsNone() ? FName("Pickup") : Reason);
		OnEnergyChanged(CurrentEnergy, MaxEnergy);
	}
}

void UOdysseyActionButtonManager::SetEnergy(float NewEnergy, FName Reason)
{
	float OldEnergy = CurrentEnergy;
	CurrentEnergy = FMath::Clamp(NewEnergy, 0.0f, MaxEnergy);

	if (FMath::Abs(CurrentEnergy - OldEnergy) > KINDA_SMALL_NUMBER)
	{
		PublishEnergyChange(OldEnergy, Reason.IsNone() ? FName("Set") : Reason);
		OnEnergyChanged(CurrentEnergy, MaxEnergy);
	}
}

float UOdysseyActionButtonManager::GetEnergyPercentage() const
{
	return MaxEnergy > 0.0f ? CurrentEnergy / MaxEnergy : 0.0f;
}

void UOdysseyActionButtonManager::PublishEnergyChange(float OldEnergy, FName Reason)
{
	if (bUseEventSystem && EventBus)
	{
		EventBus->PublishEnergyEvent(OldEnergy, CurrentEnergy, MaxEnergy, Reason);
	}
}

// ============================================================================
// Action Implementations (Legacy, kept for backward compatibility)
// ============================================================================

void UOdysseyActionButtonManager::ExecuteInteract()
{
	UE_LOG(LogTemp, Log, TEXT("Executing Interact"));
}

void UOdysseyActionButtonManager::ExecuteOpenCargo()
{
	UE_LOG(LogTemp, Log, TEXT("Opening Cargo Bay"));
}

void UOdysseyActionButtonManager::ExecuteScoutMode()
{
	bScoutModeActive = !bScoutModeActive;
	OnScoutModeToggled(bScoutModeActive);

	UE_LOG(LogTemp, Log, TEXT("Scout Mode: %s"), bScoutModeActive ? TEXT("ON") : TEXT("OFF"));
}

void UOdysseyActionButtonManager::ExecuteAttack()
{
	bAttackModeActive = !bAttackModeActive;
	OnAttackModeToggled(bAttackModeActive);

	UE_LOG(LogTemp, Log, TEXT("Attack Mode: %s"), bAttackModeActive ? TEXT("ON") : TEXT("OFF"));
}

void UOdysseyActionButtonManager::ExecuteSpecialAttack()
{
	UE_LOG(LogTemp, Log, TEXT("Executing Special Attack"));
}

void UOdysseyActionButtonManager::ExecuteThrusterBoost()
{
	bThrusterBoostActive = true;
	ThrusterBoostTimeRemaining = 3.0f;

	OnThrusterBoostActivated(ThrusterBoostTimeRemaining);

	UE_LOG(LogTemp, Log, TEXT("Thruster Boost Activated"));
}

// ============================================================================
// Touch Input
// ============================================================================

EActionButtonType UOdysseyActionButtonManager::GetButtonAtPosition(FVector2D TouchPosition) const
{
	for (const FActionButtonData& Button : ActionButtons)
	{
		FVector2D ButtonMin = Button.Position - (Button.Size * 0.5f);
		FVector2D ButtonMax = Button.Position + (Button.Size * 0.5f);

		if (TouchPosition.X >= ButtonMin.X && TouchPosition.X <= ButtonMax.X &&
			TouchPosition.Y >= ButtonMin.Y && TouchPosition.Y <= ButtonMax.Y)
		{
			return Button.ButtonType;
		}
	}
	return EActionButtonType::None;
}

bool UOdysseyActionButtonManager::HandleButtonTouch(FVector2D TouchPosition)
{
	EActionButtonType ButtonType = GetButtonAtPosition(TouchPosition);
	if (ButtonType != EActionButtonType::None)
	{
		return ActivateButton(ButtonType);
	}
	return false;
}

// ============================================================================
// Event System Access
// ============================================================================

UOdysseyEventBus* UOdysseyActionButtonManager::GetEventBus()
{
	if (!EventBus)
	{
		EventBus = UOdysseyEventBus::Get();
	}
	return EventBus;
}

UOdysseyActionDispatcher* UOdysseyActionButtonManager::GetActionDispatcher()
{
	if (!ActionDispatcher)
	{
		ActionDispatcher = UOdysseyActionDispatcher::Get();
	}
	return ActionDispatcher;
}

void UOdysseyActionButtonManager::SetUseEventSystem(bool bEnabled)
{
	if (bUseEventSystem == bEnabled)
	{
		return;
	}

	bUseEventSystem = bEnabled;

	if (bEnabled)
	{
		InitializeEventSystem();
	}
	else
	{
		ShutdownEventSystem();
	}
}

// ============================================================================
// Event Handlers
// ============================================================================

void UOdysseyActionButtonManager::OnActionExecutedEvent(const FOdysseyEventPayload& Payload)
{
	const FActionEventPayload* ActionPayload = static_cast<const FActionEventPayload*>(&Payload);
	if (ActionPayload)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Action executed event received: %d"), ActionPayload->ActionType);
	}
}

void UOdysseyActionButtonManager::OnActionFailedEvent(const FOdysseyEventPayload& Payload)
{
	const FActionFailedEventPayload* FailedPayload = static_cast<const FActionFailedEventPayload*>(&Payload);
	if (FailedPayload)
	{
		// Forward to Blueprint event
		OnActionFailed(static_cast<EActionButtonType>(FailedPayload->ActionType), FailedPayload->FailureReason);
	}
}

void UOdysseyActionButtonManager::OnCooldownStartedEvent(const FOdysseyEventPayload& Payload)
{
	const FCooldownEventPayload* CooldownPayload = static_cast<const FCooldownEventPayload*>(&Payload);
	if (CooldownPayload)
	{
		int32 ButtonIndex = FindButtonIndex(static_cast<EActionButtonType>(CooldownPayload->ActionType));
		if (ButtonIndex != -1)
		{
			ButtonStates[ButtonIndex].State = EButtonState::OnCooldown;
			ButtonStates[ButtonIndex].RemainingCooldown = CooldownPayload->RemainingTime;
			ButtonStates[ButtonIndex].TotalCooldownDuration = CooldownPayload->TotalDuration;
		}
	}
}

void UOdysseyActionButtonManager::OnCooldownCompletedEvent(const FOdysseyEventPayload& Payload)
{
	const FCooldownEventPayload* CooldownPayload = static_cast<const FCooldownEventPayload*>(&Payload);
	if (CooldownPayload)
	{
		int32 ButtonIndex = FindButtonIndex(static_cast<EActionButtonType>(CooldownPayload->ActionType));
		if (ButtonIndex != -1)
		{
			ButtonStates[ButtonIndex].State = EButtonState::Available;
			ButtonStates[ButtonIndex].RemainingCooldown = 0.0f;

			OnButtonCooldownCompleted(static_cast<EActionButtonType>(CooldownPayload->ActionType));
		}
	}
}

void UOdysseyActionButtonManager::OnEnergyChangedEvent(const FOdysseyEventPayload& Payload)
{
	// Energy changes are already handled locally
	// This handler is for external energy changes
}

void UOdysseyActionButtonManager::OnAbilityStateChangedEvent(const FOdysseyEventPayload& Payload)
{
	const FAbilityEventPayload* AbilityPayload = static_cast<const FAbilityEventPayload*>(&Payload);
	if (!AbilityPayload)
	{
		return;
	}

	EActionButtonType AbilityType = static_cast<EActionButtonType>(AbilityPayload->AbilityType);

	switch (AbilityType)
	{
		case EActionButtonType::Scout:
			bScoutModeActive = AbilityPayload->bIsActive;
			OnScoutModeToggled(bScoutModeActive);
			break;

		case EActionButtonType::Attack:
			bAttackModeActive = AbilityPayload->bIsActive;
			OnAttackModeToggled(bAttackModeActive);
			break;

		case EActionButtonType::ThrusterBoost:
			if (AbilityPayload->bIsActive)
			{
				bThrusterBoostActive = true;
				ThrusterBoostTimeRemaining = AbilityPayload->Duration;
				OnThrusterBoostActivated(AbilityPayload->Duration);
			}
			else
			{
				bThrusterBoostActive = false;
				ThrusterBoostTimeRemaining = 0.0f;
				OnThrusterBoostDeactivated();
			}
			break;

		default:
			break;
	}
}

// ============================================================================
// Initialization
// ============================================================================

void UOdysseyActionButtonManager::InitializeButtons()
{
	ActionButtons.Empty();
	ButtonStates.Empty();
	ButtonTypeToIndex.Empty();

	// Interact Button
	FActionButtonData InteractButton;
	InteractButton.ButtonType = EActionButtonType::Interact;
	InteractButton.ButtonName = TEXT("Interact");
	InteractButton.Description = TEXT("Mine, craft, or trade");
	InteractButton.Position = FVector2D(1600.0f, 800.0f);
	InteractButton.Size = FVector2D(120.0f, 120.0f);
	InteractButton.CooldownDuration = 0.5f;
	InteractButton.EnergyCost = 5;
	InteractButton.ButtonColor = FLinearColor::Green;

	// Cargo Button
	FActionButtonData CargoButton;
	CargoButton.ButtonType = EActionButtonType::Cargo;
	CargoButton.ButtonName = TEXT("Cargo");
	CargoButton.Description = TEXT("Open inventory");
	CargoButton.Position = FVector2D(1750.0f, 800.0f);
	CargoButton.Size = FVector2D(120.0f, 120.0f);
	CargoButton.CooldownDuration = 1.0f;
	CargoButton.EnergyCost = 0;
	CargoButton.ButtonColor = FLinearColor::Blue;

	// Scout Button
	FActionButtonData ScoutButton;
	ScoutButton.ButtonType = EActionButtonType::Scout;
	ScoutButton.ButtonName = TEXT("Scout");
	ScoutButton.Description = TEXT("Enhanced sensors");
	ScoutButton.Position = FVector2D(1600.0f, 650.0f);
	ScoutButton.Size = FVector2D(120.0f, 120.0f);
	ScoutButton.CooldownDuration = 2.0f;
	ScoutButton.EnergyCost = 15;
	ScoutButton.bIsToggle = true;
	ScoutButton.ButtonColor = FLinearColor::Cyan;

	// Attack Button
	FActionButtonData AttackButton;
	AttackButton.ButtonType = EActionButtonType::Attack;
	AttackButton.ButtonName = TEXT("Attack");
	AttackButton.Description = TEXT("Combat mode");
	AttackButton.Position = FVector2D(1750.0f, 650.0f);
	AttackButton.Size = FVector2D(120.0f, 120.0f);
	AttackButton.CooldownDuration = 1.5f;
	AttackButton.EnergyCost = 20;
	AttackButton.bIsToggle = true;
	AttackButton.ButtonColor = FLinearColor::Red;

	// Special Attack Button
	FActionButtonData SpecialAttackButton;
	SpecialAttackButton.ButtonType = EActionButtonType::SpecialAttack;
	SpecialAttackButton.ButtonName = TEXT("Special");
	SpecialAttackButton.Description = TEXT("Powerful attack");
	SpecialAttackButton.Position = FVector2D(1675.0f, 500.0f);
	SpecialAttackButton.Size = FVector2D(120.0f, 120.0f);
	SpecialAttackButton.CooldownDuration = 10.0f;
	SpecialAttackButton.EnergyCost = 40;
	SpecialAttackButton.ButtonColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);

	// Thruster Boost Button
	FActionButtonData ThrusterButton;
	ThrusterButton.ButtonType = EActionButtonType::ThrusterBoost;
	ThrusterButton.ButtonName = TEXT("Boost");
	ThrusterButton.Description = TEXT("Speed boost");
	ThrusterButton.Position = FVector2D(1675.0f, 950.0f);
	ThrusterButton.Size = FVector2D(120.0f, 120.0f);
	ThrusterButton.CooldownDuration = 8.0f;
	ThrusterButton.EnergyCost = 25;
	ThrusterButton.ButtonColor = FLinearColor::Yellow;

	// Add all buttons
	ActionButtons.Add(InteractButton);
	ActionButtons.Add(CargoButton);
	ActionButtons.Add(ScoutButton);
	ActionButtons.Add(AttackButton);
	ActionButtons.Add(SpecialAttackButton);
	ActionButtons.Add(ThrusterButton);

	// Initialize button states and build lookup map
	for (int32 i = 0; i < ActionButtons.Num(); i++)
	{
		const FActionButtonData& Button = ActionButtons[i];

		FActionButtonState ButtonState;
		ButtonState.ButtonType = Button.ButtonType;
		ButtonState.State = EButtonState::Available;
		ButtonState.TotalCooldownDuration = Button.CooldownDuration;
		ButtonStates.Add(ButtonState);

		// O(1) lookup map
		ButtonTypeToIndex.Add(static_cast<uint8>(Button.ButtonType), i);
	}
}

void UOdysseyActionButtonManager::BuildButtonLookupMap()
{
	ButtonTypeToIndex.Empty();

	for (int32 i = 0; i < ActionButtons.Num(); i++)
	{
		ButtonTypeToIndex.Add(static_cast<uint8>(ActionButtons[i].ButtonType), i);
	}
}

// ============================================================================
// Legacy Update Methods
// ============================================================================

void UOdysseyActionButtonManager::UpdateCooldowns(float DeltaTime)
{
	for (int32 i = 0; i < ButtonStates.Num(); i++)
	{
		FActionButtonState& ButtonState = ButtonStates[i];

		if (ButtonState.State == EButtonState::OnCooldown)
		{
			ButtonState.RemainingCooldown -= DeltaTime;

			if (ButtonState.RemainingCooldown <= 0.0f)
			{
				ButtonState.RemainingCooldown = 0.0f;
				ButtonState.State = EButtonState::Available;

				OnButtonCooldownCompleted(ButtonState.ButtonType);
			}
		}
	}
}

void UOdysseyActionButtonManager::UpdateActiveAbilities(float DeltaTime)
{
	// Update thruster boost
	if (bThrusterBoostActive)
	{
		ThrusterBoostTimeRemaining -= DeltaTime;
		if (ThrusterBoostTimeRemaining <= 0.0f)
		{
			ThrusterBoostTimeRemaining = 0.0f;
			bThrusterBoostActive = false;

			// Publish deactivation event
			if (bUseEventSystem && EventBus)
			{
				EventBus->PublishAbilityEvent(
					EOdysseyEventType::AbilityDeactivated,
					static_cast<uint8>(EActionButtonType::ThrusterBoost),
					FName("ThrusterBoost"),
					false,
					0.0f
				);
			}

			OnThrusterBoostDeactivated();
			UE_LOG(LogTemp, Log, TEXT("Thruster Boost Ended"));
		}
	}
}

void UOdysseyActionButtonManager::RegenerateEnergy(float DeltaTime)
{
	if (CurrentEnergy < MaxEnergy)
	{
		float OldEnergy = CurrentEnergy;
		CurrentEnergy = FMath::Min(MaxEnergy, CurrentEnergy + (EnergyRegenRate * DeltaTime));

		if (CurrentEnergy != OldEnergy)
		{
			OnEnergyChanged(CurrentEnergy, MaxEnergy);
		}
	}
}

// ============================================================================
// O(1) Lookup Helpers
// ============================================================================

int32 UOdysseyActionButtonManager::FindButtonIndex(EActionButtonType ButtonType) const
{
	// O(1) lookup using hash map
	const int32* IndexPtr = ButtonTypeToIndex.Find(static_cast<uint8>(ButtonType));
	return IndexPtr ? *IndexPtr : -1;
}

FActionButtonData UOdysseyActionButtonManager::GetButtonData(EActionButtonType ButtonType) const
{
	int32 Index = FindButtonIndex(ButtonType);
	if (Index != -1 && Index < ActionButtons.Num())
	{
		return ActionButtons[Index];
	}
	return FActionButtonData();
}
