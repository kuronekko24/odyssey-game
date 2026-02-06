// NPCShipHealthIntegration.cpp
// Implementation showing how the new health component integrates with NPCShip
// Phase 1: Health & Damage Foundation

#include "NPCShipHealthIntegration.h"
#include "NPCBehaviorComponent.h"
#include "Engine/Engine.h"

// ============================================================================
// Log Category
// ============================================================================

DEFINE_LOG_CATEGORY_STATIC(LogNPCShipHealth, Log, All);

// ============================================================================
// Constructor
// ============================================================================

ANPCShipEnhanced::ANPCShipEnhanced()
{
	AdvancedHealthComponent = CreateDefaultSubobject<UNPCHealthComponent>(TEXT("AdvancedHealthComponent"));
}

// ============================================================================
// Lifecycle
// ============================================================================

void ANPCShipEnhanced::BeginPlay()
{
	Super::BeginPlay();

	InitializeAdvancedHealthSystem();
	EnsureDamageProcessorConfigured();
	SynchronizeLegacyHealthVariables();

	UE_LOG(LogNPCShipHealth, Log, TEXT("[%s] Enhanced NPC ship initialized (Hull=%.0f, Shields=%.0f)"),
		*GetName(),
		AdvancedHealthComponent ? AdvancedHealthComponent->GetCurrentHealth() : 0.0f,
		AdvancedHealthComponent ? AdvancedHealthComponent->GetCurrentShields() : 0.0f);
}

void ANPCShipEnhanced::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

// ============================================================================
// Legacy Interface Override
// ============================================================================

void ANPCShipEnhanced::TakeDamage(float DamageAmount, AActor* DamageSource)
{
	if (AdvancedHealthComponent && !AdvancedHealthComponent->IsDead())
	{
		AdvancedHealthComponent->TakeDamage(DamageAmount, DamageSource, TEXT("Combat"));

		// Sync legacy variables for backward compatibility
		SynchronizeLegacyHealthVariables();
	}
	else
	{
		// Fallback to base class if component is missing
		Super::TakeDamage(DamageAmount, DamageSource);
		UE_LOG(LogNPCShipHealth, Warning, TEXT("[%s] Falling back to legacy damage system"), *GetName());
	}
}

// ============================================================================
// Enhanced Queries
// ============================================================================

EHealthState ANPCShipEnhanced::GetDetailedHealthState() const
{
	if (AdvancedHealthComponent)
	{
		return AdvancedHealthComponent->GetHealthState();
	}

	// Fallback estimation from legacy values
	float Percent = GetHealthPercentage();
	if (Percent <= 0.0f) return EHealthState::Dead;
	if (Percent <= 0.25f) return EHealthState::Dying;
	if (Percent <= 0.5f) return EHealthState::Critical;
	if (Percent <= 0.75f) return EHealthState::Damaged;
	return EHealthState::Healthy;
}

bool ANPCShipEnhanced::IsHealthRegenerating() const
{
	if (!AdvancedHealthComponent) return false;
	// Regen is active when not dead, not at full health, and past the regen delay
	return !AdvancedHealthComponent->IsDead()
		&& !AdvancedHealthComponent->IsAtFullHealth()
		&& !AdvancedHealthComponent->IsInCombat();
}

bool ANPCShipEnhanced::IsShieldRegenerating() const
{
	if (!AdvancedHealthComponent) return false;
	return AdvancedHealthComponent->GetCurrentShields() < AdvancedHealthComponent->GetMaxShields()
		&& !AdvancedHealthComponent->IsDead();
}

float ANPCShipEnhanced::GetDamageResistance(FName DamageType) const
{
	if (AdvancedHealthComponent)
	{
		return AdvancedHealthComponent->GetDamageResistance(DamageType);
	}
	return 0.0f;
}

// ============================================================================
// Ship-Type Configuration
// ============================================================================

void ANPCShipEnhanced::ConfigureShipResistances()
{
	if (!AdvancedHealthComponent) return;

	switch (ShipConfig.ShipType)
	{
		case ENPCShipType::Civilian:
			AdvancedHealthComponent->SetDamageResistance(TEXT("Energy"), 0.05f);
			AdvancedHealthComponent->SetFlatDamageReduction(0.0f);
			break;

		case ENPCShipType::Pirate:
			AdvancedHealthComponent->SetDamageResistance(TEXT("Kinetic"), 0.15f);
			AdvancedHealthComponent->SetDamageResistance(TEXT("Energy"), 0.05f);
			AdvancedHealthComponent->SetFlatDamageReduction(1.0f);
			break;

		case ENPCShipType::Security:
			AdvancedHealthComponent->SetDamageResistance(TEXT("Kinetic"), 0.20f);
			AdvancedHealthComponent->SetDamageResistance(TEXT("Energy"), 0.15f);
			AdvancedHealthComponent->SetDamageResistance(TEXT("Plasma"), 0.10f);
			AdvancedHealthComponent->SetFlatDamageReduction(2.0f);
			break;

		case ENPCShipType::Escort:
			AdvancedHealthComponent->SetDamageResistance(TEXT("Energy"), 0.25f);
			AdvancedHealthComponent->SetDamageResistance(TEXT("Plasma"), 0.20f);
			AdvancedHealthComponent->SetDamageResistance(TEXT("Kinetic"), 0.10f);
			AdvancedHealthComponent->SetFlatDamageReduction(1.5f);
			break;
	}

	UE_LOG(LogNPCShipHealth, Log, TEXT("[%s] Resistances configured for ship type %d"),
		*GetName(), static_cast<int32>(ShipConfig.ShipType));
}

void ANPCShipEnhanced::SetupHealthRegeneration()
{
	if (!AdvancedHealthComponent) return;

	switch (ShipConfig.ShipType)
	{
		case ENPCShipType::Civilian:
			AdvancedHealthComponent->SetHealthRegenEnabled(true);
			AdvancedHealthComponent->SetHealthRegenRate(2.0f);
			break;

		case ENPCShipType::Pirate:
			// Pirates have poor maintenance; no hull regen
			AdvancedHealthComponent->SetHealthRegenEnabled(false);
			break;

		case ENPCShipType::Security:
			AdvancedHealthComponent->SetHealthRegenEnabled(true);
			AdvancedHealthComponent->SetHealthRegenRate(4.0f);
			break;

		case ENPCShipType::Escort:
			AdvancedHealthComponent->SetHealthRegenEnabled(true);
			AdvancedHealthComponent->SetHealthRegenRate(6.0f);
			break;
	}

	UE_LOG(LogNPCShipHealth, Log, TEXT("[%s] Health regeneration configured"), *GetName());
}

void ANPCShipEnhanced::ConfigureShipShields()
{
	if (!AdvancedHealthComponent) return;

	switch (ShipConfig.ShipType)
	{
		case ENPCShipType::Civilian:
			AdvancedHealthComponent->SetMaxShields(20.0f);
			AdvancedHealthComponent->SetShieldRegenEnabled(true);
			AdvancedHealthComponent->SetShieldRegenRate(4.0f);
			break;

		case ENPCShipType::Pirate:
			AdvancedHealthComponent->SetMaxShields(30.0f);
			AdvancedHealthComponent->SetShieldRegenEnabled(true);
			AdvancedHealthComponent->SetShieldRegenRate(5.0f);
			break;

		case ENPCShipType::Security:
			AdvancedHealthComponent->SetMaxShields(60.0f);
			AdvancedHealthComponent->SetShieldRegenEnabled(true);
			AdvancedHealthComponent->SetShieldRegenRate(8.0f);
			break;

		case ENPCShipType::Escort:
			AdvancedHealthComponent->SetMaxShields(80.0f);
			AdvancedHealthComponent->SetShieldRegenEnabled(true);
			AdvancedHealthComponent->SetShieldRegenRate(12.0f);
			break;
	}

	// Initialize shields to full
	AdvancedHealthComponent->SetShields(AdvancedHealthComponent->GetMaxShields());

	UE_LOG(LogNPCShipHealth, Log, TEXT("[%s] Shields configured: %.0f max"),
		*GetName(), AdvancedHealthComponent->GetMaxShields());
}

// ============================================================================
// Event Handlers
// ============================================================================

void ANPCShipEnhanced::OnAdvancedHealthChanged(const FHealthEventPayload& HealthData)
{
	// Sync legacy variables
	SynchronizeLegacyHealthVariables();

	// Forward to existing Blueprint events for compatibility
	if (HealthData.DamageAmount > 0.0f)
	{
		OnDamageTaken(HealthData.DamageAmount, HealthData.DamageSource.Get());
	}

	OnHealthChanged(HealthData.PreviousHealth, HealthData.CurrentHealth);

	if (!FMath::IsNearlyEqual(HealthData.PreviousShields, HealthData.CurrentShields, 0.01f))
	{
		OnShieldChanged(HealthData.PreviousShields, HealthData.CurrentShields);
	}
}

void ANPCShipEnhanced::OnAdvancedHealthStateChanged(EHealthState NewState)
{
	UE_LOG(LogNPCShipHealth, Log, TEXT("[%s] Health state -> %d"), *GetName(), static_cast<int32>(NewState));

	// Map health states to behavior modifications
	if (BehaviorComponent)
	{
		switch (NewState)
		{
			case EHealthState::Critical:
				UE_LOG(LogNPCShipHealth, Warning, TEXT("[%s] CRITICAL: modifying combat behavior"), *GetName());
				break;

			case EHealthState::Dying:
				UE_LOG(LogNPCShipHealth, Warning, TEXT("[%s] DYING: emergency behavior activated"), *GetName());
				break;

			default:
				break;
		}
	}
}

void ANPCShipEnhanced::OnAdvancedActorDied(AActor* DiedActor)
{
	if (DiedActor != this) return;

	UE_LOG(LogNPCShipHealth, Warning, TEXT("[%s] Died via advanced health system"), *GetName());

	// Sync legacy death state
	bIsDead = true;
	CurrentHealth = 0.0f;
	CurrentShields = 0.0f;

	// Trigger existing death path
	Die();
	OnDeath();
}

void ANPCShipEnhanced::OnAdvancedShieldBroken(AActor* Owner, AActor* DamageSource)
{
	UE_LOG(LogNPCShipHealth, Warning, TEXT("[%s] Shields BROKEN by %s!"),
		*GetName(), DamageSource ? *DamageSource->GetName() : TEXT("Unknown"));

	// Sync legacy shield variable
	CurrentShields = 0.0f;
}

void ANPCShipEnhanced::OnAdvancedShieldRestored(AActor* Owner, float ShieldAmount)
{
	UE_LOG(LogNPCShipHealth, Log, TEXT("[%s] Shields fully restored (%.0f)"), *GetName(), ShieldAmount);

	// Sync legacy shield variable
	CurrentShields = ShieldAmount;
}

// ============================================================================
// Integration Helpers
// ============================================================================

void ANPCShipEnhanced::InitializeAdvancedHealthSystem()
{
	if (!AdvancedHealthComponent)
	{
		UE_LOG(LogNPCShipHealth, Error, TEXT("[%s] AdvancedHealthComponent is null!"), *GetName());
		return;
	}

	// Configure from ShipConfig
	AdvancedHealthComponent->SetMaxHealth(ShipConfig.MaxHealth);
	AdvancedHealthComponent->SetHealth(ShipConfig.MaxHealth);

	// Configure ship-type-specific settings
	ConfigureShipShields();
	ConfigureShipResistances();
	SetupHealthRegeneration();

	// Bind delegates
	AdvancedHealthComponent->OnHealthChanged.AddDynamic(this, &ANPCShipEnhanced::OnAdvancedHealthChanged);
	AdvancedHealthComponent->OnHealthStateChanged.AddDynamic(this, &ANPCShipEnhanced::OnAdvancedHealthStateChanged);
	AdvancedHealthComponent->OnActorDied.AddDynamic(this, &ANPCShipEnhanced::OnAdvancedActorDied);
	AdvancedHealthComponent->OnShieldBroken.AddDynamic(this, &ANPCShipEnhanced::OnAdvancedShieldBroken);
	AdvancedHealthComponent->OnShieldRestored.AddDynamic(this, &ANPCShipEnhanced::OnAdvancedShieldRestored);

	UE_LOG(LogNPCShipHealth, Log, TEXT("[%s] Advanced health system initialized (Hull=%.0f, Shields=%.0f)"),
		*GetName(), AdvancedHealthComponent->GetMaxHealth(), AdvancedHealthComponent->GetMaxShields());
}

void ANPCShipEnhanced::SynchronizeLegacyHealthVariables()
{
	if (!AdvancedHealthComponent) return;

	CurrentHealth = AdvancedHealthComponent->GetCurrentHealth();
	CurrentShields = AdvancedHealthComponent->GetCurrentShields();
	MaxShields = AdvancedHealthComponent->GetMaxShields();
}

void ANPCShipEnhanced::EnsureDamageProcessorConfigured()
{
	UOdysseyDamageProcessor* DP = UOdysseyDamageProcessor::Get();
	if (!DP) return;

	if (!DP->IsInitialized())
	{
		DP->Initialize();

		// Global combat tuning
		DP->SetGlobalDamageMultiplier(1.0f);
		DP->SetGlobalCriticalChance(0.05f);
		DP->SetGlobalCriticalMultiplier(2.0f);
		DP->SetCriticalHitsEnabled(true);
		DP->SetMinimumDamage(1.0f);

		// Damage type multipliers
		DP->SetDamageTypeMultiplier(TEXT("Kinetic"), 1.0f);
		DP->SetDamageTypeMultiplier(TEXT("Energy"), 1.2f);
		DP->SetDamageTypeMultiplier(TEXT("Plasma"), 1.5f);

		// Distance falloff (optional, disabled by default)
		DP->SetDistanceFalloffEnabled(false);
		DP->SetDistanceFalloffParams(500.0f, 2000.0f, 1.0f);

		UE_LOG(LogNPCShipHealth, Log, TEXT("OdysseyDamageProcessor configured for combat"));
	}
}
