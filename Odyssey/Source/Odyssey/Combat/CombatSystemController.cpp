// CombatSystemController.cpp
// Phase 3: Master combat system coordinator implementation

#include "CombatSystemController.h"
#include "TouchTargetingSystem.h"
#include "AutoWeaponSystem.h"
#include "CombatFeedbackSystem.h"
#include "OdysseyTouchInterface.h"
#include "OdysseyActionButton.h"
#include "Engine/World.h"

UCombatSystemController::UCombatSystemController()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10 Hz for state monitoring

	bInitialized = false;
	bCombatEnabled = false;
	bActionsRegistered = false;
	bAutoEnable = true;
	bAutoRegisterActions = true;

	TargetingSystem = nullptr;
	WeaponSystem = nullptr;
	FeedbackSystem = nullptr;
	TouchInterface = nullptr;
	ActionButtonManager = nullptr;
}

void UCombatSystemController::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoEnable)
	{
		InitializeCombat();
	}
}

void UCombatSystemController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownCombat();
	Super::EndPlay(EndPlayReason);
}

void UCombatSystemController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Light-weight state monitoring -- subsystems handle their own ticking
}

// ============================================================================
// System Lifecycle
// ============================================================================

void UCombatSystemController::InitializeCombat()
{
	if (bInitialized)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("CombatSystemController: Initializing combat system"));

	EnsureSubsystems();
	WireSubsystems();
	PushConfiguration();
	FindExternalSystems();

	if (bAutoRegisterActions)
	{
		RegisterCombatActions();
	}

	bInitialized = true;
	bCombatEnabled = true;

	// Enable subsystems
	if (WeaponSystem)
	{
		WeaponSystem->SetWeaponEnabled(true);
		WeaponSystem->SetAutoFireEnabled(true);
	}

	OnCombatModeChanged.Broadcast(true, ECombatEngagementState::Scanning);

	UE_LOG(LogTemp, Log, TEXT("CombatSystemController: Combat system initialized successfully"));
}

void UCombatSystemController::ShutdownCombat()
{
	if (!bInitialized)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("CombatSystemController: Shutting down combat system"));

	// Disable weapon
	if (WeaponSystem)
	{
		WeaponSystem->SetWeaponEnabled(false);
	}

	// Clear target
	if (TargetingSystem)
	{
		TargetingSystem->ClearTarget();
	}

	// Unregister actions
	if (ActionButtonManager && bActionsRegistered)
	{
		ActionButtonManager->UnregisterButton(EActionButtonType::Attack);
		ActionButtonManager->UnregisterButton(EActionButtonType::SpecialAttack);
		bActionsRegistered = false;
	}

	bCombatEnabled = false;
	bInitialized = false;

	OnCombatModeChanged.Broadcast(false, ECombatEngagementState::Idle);
}

void UCombatSystemController::SetCombatEnabled(bool bEnabled)
{
	if (bEnabled && !bInitialized)
	{
		InitializeCombat();
		return;
	}

	if (!bEnabled && bInitialized)
	{
		bCombatEnabled = false;
		
		if (WeaponSystem)
		{
			WeaponSystem->SetWeaponEnabled(false);
		}
		if (TargetingSystem)
		{
			TargetingSystem->ClearTarget();
		}

		OnCombatModeChanged.Broadcast(false, ECombatEngagementState::Idle);
		return;
	}

	if (bEnabled && bInitialized && !bCombatEnabled)
	{
		bCombatEnabled = true;
		
		if (WeaponSystem)
		{
			WeaponSystem->SetWeaponEnabled(true);
		}

		OnCombatModeChanged.Broadcast(true, ECombatEngagementState::Scanning);
	}
}

// ============================================================================
// Touch Input Integration
// ============================================================================

bool UCombatSystemController::HandleCombatTouch(FVector2D ScreenPosition)
{
	if (!IsCombatEnabled() || !TargetingSystem)
	{
		return false;
	}

	return TargetingSystem->HandleTouch(ScreenPosition);
}

// ============================================================================
// Action Button Integration
// ============================================================================

bool UCombatSystemController::HandleAttackAction()
{
	if (!IsCombatEnabled())
	{
		return false;
	}

	// If no target, try auto-select
	if (TargetingSystem && !TargetingSystem->HasValidTarget())
	{
		TargetingSystem->AutoSelectBestTarget();
	}

	// Fire weapon
	if (WeaponSystem)
	{
		FCombatFireResult Result = WeaponSystem->FireOnce();
		return Result.bFired;
	}

	return false;
}

bool UCombatSystemController::HandleSpecialAttackAction()
{
	if (!IsCombatEnabled())
	{
		return false;
	}

	// Auto-select if needed
	if (TargetingSystem && !TargetingSystem->HasValidTarget())
	{
		TargetingSystem->AutoSelectBestTarget();
	}

	// For now, special attack fires a double-damage shot
	if (WeaponSystem && TargetingSystem && TargetingSystem->HasValidTarget())
	{
		// Temporarily boost damage
		const float OrigDamage = WeaponSystem->Config.BaseDamage;
		WeaponSystem->Config.BaseDamage *= 2.5f;

		FCombatFireResult Result = WeaponSystem->FireOnce();

		WeaponSystem->Config.BaseDamage = OrigDamage;
		return Result.bFired;
	}

	return false;
}

void UCombatSystemController::RegisterCombatActions()
{
	if (!ActionButtonManager || bActionsRegistered)
	{
		return;
	}

	// Register Attack button
	FActionButtonData AttackData;
	AttackData.ButtonType = EActionButtonType::Attack;
	AttackData.ButtonName = TEXT("Attack");
	AttackData.Description = TEXT("Fire weapons at the targeted enemy");
	AttackData.Position = FVector2D(200, 100);
	AttackData.Size = FVector2D(80, 80);
	AttackData.CooldownDuration = 0.3f;
	AttackData.EnergyCost = WeaponConfig.EnergyCost;
	AttackData.bRequiresTarget = false; // Will auto-target
	AttackData.ButtonColor = FLinearColor(0.9f, 0.15f, 0.1f, 1.0f);
	AttackData.CustomHandlerName = FName("CombatAttack");
	ActionButtonManager->RegisterButton(AttackData);

	// Register Special Attack button
	FActionButtonData SpecialData;
	SpecialData.ButtonType = EActionButtonType::SpecialAttack;
	SpecialData.ButtonName = TEXT("Heavy Strike");
	SpecialData.Description = TEXT("Powerful charged attack dealing 2.5x damage");
	SpecialData.Position = FVector2D(290, 100);
	SpecialData.Size = FVector2D(80, 80);
	SpecialData.CooldownDuration = 3.0f;
	SpecialData.EnergyCost = WeaponConfig.EnergyCost * 4;
	SpecialData.bRequiresTarget = false;
	SpecialData.ButtonColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);
	SpecialData.CustomHandlerName = FName("CombatSpecialAttack");
	ActionButtonManager->RegisterButton(SpecialData);

	bActionsRegistered = true;

	UE_LOG(LogTemp, Log, TEXT("CombatSystemController: Combat actions registered"));
}

// ============================================================================
// Statistics
// ============================================================================

FCombatSessionStats UCombatSystemController::GetSessionStats() const
{
	return WeaponSystem ? WeaponSystem->GetSessionStats() : FCombatSessionStats();
}

void UCombatSystemController::ResetSessionStats()
{
	if (WeaponSystem)
	{
		WeaponSystem->ResetSessionStats();
	}
}

// ============================================================================
// Configuration
// ============================================================================

void UCombatSystemController::ApplyConfiguration()
{
	PushConfiguration();
}

// ============================================================================
// Internal
// ============================================================================

void UCombatSystemController::EnsureSubsystems()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Find or create Targeting System
	TargetingSystem = Owner->FindComponentByClass<UTouchTargetingSystem>();
	if (!TargetingSystem)
	{
		TargetingSystem = NewObject<UTouchTargetingSystem>(Owner, FName("CombatTargeting"));
		Owner->AddInstanceComponent(TargetingSystem);
		TargetingSystem->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("CombatSystemController: Created UTouchTargetingSystem"));
	}

	// Find or create Weapon System
	WeaponSystem = Owner->FindComponentByClass<UAutoWeaponSystem>();
	if (!WeaponSystem)
	{
		WeaponSystem = NewObject<UAutoWeaponSystem>(Owner, FName("CombatWeapon"));
		Owner->AddInstanceComponent(WeaponSystem);
		WeaponSystem->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("CombatSystemController: Created UAutoWeaponSystem"));
	}

	// Find or create Feedback System
	FeedbackSystem = Owner->FindComponentByClass<UCombatFeedbackSystem>();
	if (!FeedbackSystem)
	{
		FeedbackSystem = NewObject<UCombatFeedbackSystem>(Owner, FName("CombatFeedback"));
		Owner->AddInstanceComponent(FeedbackSystem);
		FeedbackSystem->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("CombatSystemController: Created UCombatFeedbackSystem"));
	}
}

void UCombatSystemController::WireSubsystems()
{
	// Weapon -> Targeting
	if (WeaponSystem && TargetingSystem)
	{
		WeaponSystem->SetTargetingSystem(TargetingSystem);
	}

	// Feedback -> Targeting + Weapon
	if (FeedbackSystem)
	{
		if (TargetingSystem)
		{
			FeedbackSystem->SetTargetingSystem(TargetingSystem);
		}
		if (WeaponSystem)
		{
			FeedbackSystem->SetWeaponSystem(WeaponSystem);
		}
	}

	// Subscribe to engagement state changes
	if (WeaponSystem)
	{
		WeaponSystem->OnEngagementStateChanged.AddDynamic(this, &UCombatSystemController::OnEngagementStateChanged);
	}
}

void UCombatSystemController::PushConfiguration()
{
	if (TargetingSystem)
	{
		TargetingSystem->Config = TargetingConfig;
	}

	if (WeaponSystem)
	{
		WeaponSystem->Config = WeaponConfig;
	}

	if (FeedbackSystem)
	{
		FeedbackSystem->FeedbackConfig = FeedbackConfig;
	}
}

void UCombatSystemController::FindExternalSystems()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TouchInterface = Owner->FindComponentByClass<UOdysseyTouchInterface>();
	ActionButtonManager = Owner->FindComponentByClass<UOdysseyActionButtonManager>();

	if (TouchInterface)
	{
		UE_LOG(LogTemp, Log, TEXT("CombatSystemController: Found OdysseyTouchInterface"));
	}

	if (ActionButtonManager)
	{
		UE_LOG(LogTemp, Log, TEXT("CombatSystemController: Found OdysseyActionButtonManager"));
	}
}

void UCombatSystemController::OnEngagementStateChanged(ECombatEngagementState OldState, ECombatEngagementState NewState)
{
	OnCombatModeChanged.Broadcast(bCombatEnabled, NewState);
}
