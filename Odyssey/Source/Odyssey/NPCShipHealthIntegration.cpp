// NPCShipHealthIntegration.cpp
// Implementation showing how to integrate the new health system with NPCShip

#include "NPCShipHealthIntegration.h"
#include "NPCBehaviorComponent.h"
#include "Engine/Engine.h"

ANPCShipEnhanced::ANPCShipEnhanced()
{
	// Create the advanced health component
	AdvancedHealthComponent = CreateDefaultSubobject<UNPCHealthComponent>(TEXT("AdvancedHealthComponent"));

	UE_LOG(LogTemp, Log, TEXT("ANPCShipEnhanced constructor: Advanced health component created"));
}

void ANPCShipEnhanced::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize the advanced health system
	InitializeAdvancedHealthSystem();
	
	// Configure damage processor (singleton, only needs to be done once)
	ConfigureDamageProcessor();
	
	// Synchronize with existing health system if needed
	SynchronizeHealthSystems();
	
	UE_LOG(LogTemp, Log, TEXT("NPCShipEnhanced %s initialized with advanced health system"), *GetName());
}

void ANPCShipEnhanced::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Cleanup is handled automatically by the components
	Super::EndPlay(EndPlayReason);
}

void ANPCShipEnhanced::TakeDamage(float DamageAmount, AActor* DamageSource)
{
	// Use the advanced health component instead of the basic system
	if (AdvancedHealthComponent && !AdvancedHealthComponent->IsDead())
	{
		// Let the advanced health component handle damage with all its features
		AdvancedHealthComponent->TakeDamage(DamageAmount, DamageSource, TEXT("Combat"));
		
		// Also update legacy health variables for backward compatibility
		CurrentHealth = AdvancedHealthComponent->GetCurrentHealth();
		
		UE_LOG(LogTemp, Log, TEXT("NPCShipEnhanced %s took damage via advanced system: %.1f damage, %.1f/%.1f health"), 
			*GetName(), DamageAmount, CurrentHealth, AdvancedHealthComponent->GetMaxHealth());
	}
	else
	{
		// Fallback to basic system if component is not available
		Super::TakeDamage(DamageAmount, DamageSource);
		UE_LOG(LogTemp, Warning, TEXT("NPCShipEnhanced %s falling back to basic damage system"), *GetName());
	}
}

EHealthState ANPCShipEnhanced::GetDetailedHealthState() const
{
	if (AdvancedHealthComponent)
	{
		return AdvancedHealthComponent->GetHealthState();
	}
	
	// Fallback: convert basic health to health state
	float HealthPercent = GetHealthPercentage();
	if (HealthPercent <= 0.0f) return EHealthState::Dead;
	if (HealthPercent <= 0.25f) return EHealthState::Dying;
	if (HealthPercent <= 0.5f) return EHealthState::Critical;
	if (HealthPercent <= 0.75f) return EHealthState::Damaged;
	return EHealthState::Healthy;
}

bool ANPCShipEnhanced::IsHealthRegenerating() const
{
	// This would require adding a getter to NPCHealthComponent
	// For now, return a reasonable estimate
	return AdvancedHealthComponent && 
		   !AdvancedHealthComponent->IsDead() && 
		   !AdvancedHealthComponent->IsAtFullHealth();
}

float ANPCShipEnhanced::GetDamageResistance(FName DamageType) const
{
	if (AdvancedHealthComponent)
	{
		return AdvancedHealthComponent->GetDamageResistance(DamageType);
	}
	return 0.0f;
}

void ANPCShipEnhanced::ConfigureShipResistances()
{
	if (!AdvancedHealthComponent)
	{
		return;
	}

	// Configure resistances based on ship type
	switch (ShipConfig.ShipType)
	{
		case ENPCShipType::Civilian:
			// Civilian ships have minimal resistances
			AdvancedHealthComponent->SetDamageResistance(TEXT("Energy"), 0.05f);  // 5% energy resistance
			break;
			
		case ENPCShipType::Pirate:
			// Pirate ships have moderate kinetic resistance (improvised armor)
			AdvancedHealthComponent->SetDamageResistance(TEXT("Kinetic"), 0.15f); // 15% kinetic resistance
			AdvancedHealthComponent->SetDamageResistance(TEXT("Energy"), 0.05f);  // 5% energy resistance
			break;
			
		case ENPCShipType::Security:
			// Security ships have balanced resistances
			AdvancedHealthComponent->SetDamageResistance(TEXT("Kinetic"), 0.20f); // 20% kinetic resistance
			AdvancedHealthComponent->SetDamageResistance(TEXT("Energy"), 0.15f);  // 15% energy resistance
			AdvancedHealthComponent->SetDamageResistance(TEXT("Plasma"), 0.10f);  // 10% plasma resistance
			break;
			
		case ENPCShipType::Escort:
			// Escort ships have high energy resistance (advanced shields)
			AdvancedHealthComponent->SetDamageResistance(TEXT("Energy"), 0.25f);  // 25% energy resistance
			AdvancedHealthComponent->SetDamageResistance(TEXT("Plasma"), 0.20f);  // 20% plasma resistance
			AdvancedHealthComponent->SetDamageResistance(TEXT("Kinetic"), 0.10f); // 10% kinetic resistance
			break;
	}
	
	UE_LOG(LogTemp, Log, TEXT("NPCShipEnhanced %s configured resistances for ship type %d"), 
		*GetName(), (int32)ShipConfig.ShipType);
}

void ANPCShipEnhanced::SetupHealthRegeneration()
{
	if (!AdvancedHealthComponent)
	{
		return;
	}

	// Configure regeneration based on ship type
	switch (ShipConfig.ShipType)
	{
		case ENPCShipType::Civilian:
			// Civilian ships have slow regeneration
			AdvancedHealthComponent->SetHealthRegenEnabled(true);
			AdvancedHealthComponent->SetHealthRegenRate(2.0f);    // 2 HP/sec
			break;
			
		case ENPCShipType::Pirate:
			// Pirate ships have no regeneration (poor maintenance)
			AdvancedHealthComponent->SetHealthRegenEnabled(false);
			break;
			
		case ENPCShipType::Security:
			// Security ships have moderate regeneration
			AdvancedHealthComponent->SetHealthRegenEnabled(true);
			AdvancedHealthComponent->SetHealthRegenRate(4.0f);    // 4 HP/sec
			break;
			
		case ENPCShipType::Escort:
			// Escort ships have fast regeneration (advanced tech)
			AdvancedHealthComponent->SetHealthRegenEnabled(true);
			AdvancedHealthComponent->SetHealthRegenRate(6.0f);    // 6 HP/sec
			break;
	}
	
	UE_LOG(LogTemp, Log, TEXT("NPCShipEnhanced %s configured health regeneration"), *GetName());
}

void ANPCShipEnhanced::OnAdvancedHealthChanged(const FHealthEventPayload& HealthData)
{
	// Update UI, effects, etc.
	UE_LOG(LogTemp, Log, TEXT("NPCShip %s health changed: %.1f -> %.1f (%.1f%%)"), 
		*GetName(), 
		HealthData.PreviousHealth, 
		HealthData.CurrentHealth,
		HealthData.GetHealthPercentage() * 100.0f);
	
	// Trigger existing Blueprint events for compatibility
	OnHealthChanged(HealthData.PreviousHealth, HealthData.CurrentHealth);
	
	// Add visual effects based on damage amount
	if (HealthData.DamageAmount > 0.0f)
	{
		// Trigger damage effects, screen shake, etc.
		OnDamageTaken(HealthData.DamageAmount, HealthData.DamageSource.Get());
	}
}

void ANPCShipEnhanced::OnAdvancedHealthStateChanged(EHealthState NewState)
{
	UE_LOG(LogTemp, Warning, TEXT("NPCShip %s health state changed to %d"), *GetName(), (int32)NewState);
	
	// Update AI behavior based on health state
	if (BehaviorComponent)
	{
		switch (NewState)
		{
			case EHealthState::Critical:
				// Ship becomes more aggressive or starts retreating
				UE_LOG(LogTemp, Warning, TEXT("NPCShip %s entering critical health state - modifying behavior"), *GetName());
				break;
				
			case EHealthState::Dying:
				// Ship tries to escape or becomes desperate
				UE_LOG(LogTemp, Warning, TEXT("NPCShip %s is dying - emergency behavior activated"), *GetName());
				break;
				
			case EHealthState::Dead:
				// This will be handled by OnAdvancedActorDied
				break;
				
			default:
				// Normal behavior
				break;
		}
	}
	
	// Trigger existing state change events for compatibility
	if (BehaviorComponent)
	{
		// You might want to map health states to existing NPC states
		ENPCState NewNPCState = BehaviorComponent->GetCurrentState(); // Keep current state by default
		
		switch (NewState)
		{
			case EHealthState::Critical:
			case EHealthState::Dying:
				// Could set to a retreat or defensive state if available
				break;
		}
	}
}

void ANPCShipEnhanced::OnAdvancedActorDied(AActor* DiedActor)
{
	if (DiedActor == this)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCShip %s died via advanced health system"), *GetName());
		
		// Update legacy death flag for compatibility
		bIsDead = true;
		CurrentHealth = 0.0f;
		
		// Trigger existing death logic
		Die();
		
		// Trigger Blueprint event
		OnDeath();
	}
}

void ANPCShipEnhanced::InitializeAdvancedHealthSystem()
{
	if (!AdvancedHealthComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("NPCShipEnhanced %s: AdvancedHealthComponent is null!"), *GetName());
		return;
	}
	
	// Set up health component with ship configuration
	AdvancedHealthComponent->SetMaxHealth(ShipConfig.MaxHealth);
	AdvancedHealthComponent->SetHealth(ShipConfig.MaxHealth * 1.0f); // Start at full health
	
	// Configure health regeneration and resistances
	ConfigureShipResistances();
	SetupHealthRegeneration();
	
	// Bind to health events
	AdvancedHealthComponent->OnHealthChanged.AddDynamic(this, &ANPCShipEnhanced::OnAdvancedHealthChanged);
	AdvancedHealthComponent->OnHealthStateChanged.AddDynamic(this, &ANPCShipEnhanced::OnAdvancedHealthStateChanged);
	AdvancedHealthComponent->OnActorDied.AddDynamic(this, &ANPCShipEnhanced::OnAdvancedActorDied);
	
	UE_LOG(LogTemp, Log, TEXT("NPCShipEnhanced %s: Advanced health system initialized with %.1f max health"), 
		*GetName(), AdvancedHealthComponent->GetMaxHealth());
}

void ANPCShipEnhanced::SynchronizeHealthSystems()
{
	// Synchronize the legacy health variables with the new health component
	// This ensures backward compatibility
	
	if (AdvancedHealthComponent)
	{
		// Update legacy variables to match advanced component
		CurrentHealth = AdvancedHealthComponent->GetCurrentHealth();
		// Note: ShipConfig.MaxHealth should already match since we set it during initialization
	}
	
	UE_LOG(LogTemp, Log, TEXT("NPCShipEnhanced %s: Health systems synchronized"), *GetName());
}

void ANPCShipEnhanced::ConfigureDamageProcessor()
{
	// Get the global damage processor instance
	UOdysseyDamageProcessor* DamageProcessor = UOdysseyDamageProcessor::Get();
	if (!DamageProcessor)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not get OdysseyDamageProcessor instance"));
		return;
	}
	
	// Initialize if not already done
	if (!DamageProcessor->IsInitialized())
	{
		DamageProcessor->Initialize();
		
		// Configure global damage settings
		DamageProcessor->SetGlobalDamageMultiplier(1.0f);           // Normal damage
		DamageProcessor->SetGlobalCriticalChance(0.05f);           // 5% critical chance
		DamageProcessor->SetGlobalCriticalMultiplier(2.0f);        // 2x critical damage
		DamageProcessor->SetCriticalHitsEnabled(true);
		
		// Configure damage type multipliers
		DamageProcessor->SetDamageTypeMultiplier(TEXT("Kinetic"), 1.0f);  // Normal kinetic damage
		DamageProcessor->SetDamageTypeMultiplier(TEXT("Energy"), 1.2f);   // Energy weapons do 20% more damage
		DamageProcessor->SetDamageTypeMultiplier(TEXT("Plasma"), 1.5f);   // Plasma weapons do 50% more damage
		
		UE_LOG(LogTemp, Log, TEXT("OdysseyDamageProcessor configured for combat system"));
	}
}
