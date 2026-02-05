// OdysseyCombatIntegration.cpp

#include "OdysseyCombatIntegration.h"
#include "OdysseyCombatManager.h"
#include "OdysseyActionButton.h"
#include "OdysseyTouchInterface.h"
#include "Engine/World.h"

UOdysseyCombatIntegration::UOdysseyCombatIntegration()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Initialize references
	CombatManager = nullptr;
	ActionButtonManager = nullptr;
	TouchInterface = nullptr;
	bActionsRegistered = false;
	bTouchIntegrationActive = false;
}

void UOdysseyCombatIntegration::BeginPlay()
{
	Super::BeginPlay();

	// Initialize component references
	InitializeReferences();

	// Setup integrations
	if (CombatManager && ActionButtonManager)
	{
		SetupCombatActions();
	}

	if (CombatManager && TouchInterface)
	{
		SetupTouchIntegration();
	}
}

void UOdysseyCombatIntegration::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupIntegrations();
	Super::EndPlay(EndPlayReason);
}

// ============================================================================
// Action Button Integration
// ============================================================================

void UOdysseyCombatIntegration::SetupCombatActions()
{
	if (!ActionButtonManager || bActionsRegistered)
	{
		return;
	}

	// Create attack button data
	FActionButtonData AttackButtonData;
	AttackButtonData.ButtonType = EActionButtonType::Attack;
	AttackButtonData.ButtonName = TEXT("Attack");
	AttackButtonData.Description = TEXT("Fire weapon at current target");
	AttackButtonData.Position = FVector2D(200, 100); // Position on screen
	AttackButtonData.Size = FVector2D(80, 80);
	AttackButtonData.CooldownDuration = 0.5f;
	AttackButtonData.EnergyCost = 10;
	AttackButtonData.bRequiresTarget = false; // Can auto-target
	AttackButtonData.ButtonColor = FLinearColor::Red;
	AttackButtonData.CustomHandlerName = FName("CombatAttack");

	// Register attack button
	ActionButtonManager->RegisterButton(AttackButtonData);

	// Create special attack button data  
	FActionButtonData SpecialAttackButtonData;
	SpecialAttackButtonData.ButtonType = EActionButtonType::SpecialAttack;
	SpecialAttackButtonData.ButtonName = TEXT("Special Attack");
	SpecialAttackButtonData.Description = TEXT("Charged weapon attack");
	SpecialAttackButtonData.Position = FVector2D(290, 100);
	SpecialAttackButtonData.Size = FVector2D(80, 80);
	SpecialAttackButtonData.CooldownDuration = 2.0f;
	SpecialAttackButtonData.EnergyCost = 25;
	SpecialAttackButtonData.ChargeDuration = 2.0f;
	SpecialAttackButtonData.bRequiresTarget = false;
	SpecialAttackButtonData.ButtonColor = FLinearColor::Orange;
	SpecialAttackButtonData.CustomHandlerName = FName("CombatSpecialAttack");

	// Register special attack button
	ActionButtonManager->RegisterButton(SpecialAttackButtonData);

	bActionsRegistered = true;

	UE_LOG(LogTemp, Log, TEXT("Combat actions registered successfully"));
}

void UOdysseyCombatIntegration::OnAttackButtonPressed()
{
	if (CombatManager)
	{
		CombatManager->HandleAttackAction();
	}
}

void UOdysseyCombatIntegration::OnSpecialAttackButtonPressed()
{
	if (CombatManager)
	{
		CombatManager->HandleSpecialAttackAction();
	}
}

// ============================================================================
// Touch Interface Integration
// ============================================================================

void UOdysseyCombatIntegration::SetupTouchIntegration()
{
	if (!TouchInterface || bTouchIntegrationActive)
	{
		return;
	}

	// Note: In a full implementation, you would bind to touch events here
	// For this example, we'll assume the touch interface will call our methods

	bTouchIntegrationActive = true;

	UE_LOG(LogTemp, Log, TEXT("Combat touch integration setup successfully"));
}

void UOdysseyCombatIntegration::OnCombatTouch(FVector2D TouchLocation, int32 FingerIndex)
{
	if (CombatManager)
	{
		// Check if this touch should be handled by combat system
		// For example, touches in the upper part of screen could be for targeting
		FVector2D ViewportSize;
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->GetViewportSize(ViewportSize);
			
			// If touch is in upper 2/3 of screen, treat as targeting
			if (TouchLocation.Y < ViewportSize.Y * 0.67f)
			{
				CombatManager->HandleCombatTouch(TouchLocation);
			}
		}
	}
}

// ============================================================================
// Private Methods
// ============================================================================

void UOdysseyCombatIntegration::InitializeReferences()
{
	if (!GetOwner())
	{
		return;
	}

	// Find combat manager
	CombatManager = GetOwner()->FindComponentByClass<UOdysseyCombatManager>();
	if (!CombatManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat integration: Combat manager not found"));
	}

	// Find action button manager
	ActionButtonManager = GetOwner()->FindComponentByClass<UOdysseyActionButtonManager>();
	if (!ActionButtonManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat integration: Action button manager not found"));
	}

	// Find touch interface
	TouchInterface = GetOwner()->FindComponentByClass<UOdysseyTouchInterface>();
	if (!TouchInterface)
	{
		UE_LOG(LogTemp, Warning, TEXT("Combat integration: Touch interface not found"));
	}
}

void UOdysseyCombatIntegration::CleanupIntegrations()
{
	// Unregister combat actions
	if (ActionButtonManager && bActionsRegistered)
	{
		ActionButtonManager->UnregisterButton(EActionButtonType::Attack);
		ActionButtonManager->UnregisterButton(EActionButtonType::SpecialAttack);
		bActionsRegistered = false;
	}

	// Clean up touch integration
	if (bTouchIntegrationActive)
	{
		// Unbind touch events here
		bTouchIntegrationActive = false;
	}
}
