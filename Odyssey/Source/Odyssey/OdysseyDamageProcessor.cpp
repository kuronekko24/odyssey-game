// OdysseyDamageProcessor.cpp
// Implementation of the central damage processing system

#include "OdysseyDamageProcessor.h"
#include "NPCHealthComponent.h"
#include "OdysseyEventBus.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"

// Static instance
UOdysseyDamageProcessor* UOdysseyDamageProcessor::GlobalInstance = nullptr;

UOdysseyDamageProcessor::UOdysseyDamageProcessor()
{
	// Configuration defaults
	GlobalDamageMultiplier = 1.0f;
	bCriticalHitsEnabled = true;
	GlobalCriticalChance = 0.05f;
	GlobalCriticalMultiplier = 2.0f;
	bVerboseLogging = false;

	// Runtime state
	bIsInitialized = false;
	EventBus = nullptr;
	TotalProcessingTime = 0.0;
	ProcessingTimeSamples = 0;

	// Initialize stats
	ProcessorStats = FDamageProcessorStats();
}

void UOdysseyDamageProcessor::Initialize()
{
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("OdysseyDamageProcessor already initialized"));
		return;
	}

	// Set global instance
	GlobalInstance = this;

	// Initialize event bus subscriptions
	InitializeEventSubscriptions();

	bIsInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("OdysseyDamageProcessor initialized successfully"));
}

void UOdysseyDamageProcessor::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	// Clean up event subscriptions
	CleanupEventSubscriptions();

	bIsInitialized = false;

	// Clear global instance if this is it
	if (GlobalInstance == this)
	{
		GlobalInstance = nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("OdysseyDamageProcessor shut down"));
}

bool UOdysseyDamageProcessor::ProcessAttackHit(const FCombatEventPayload& AttackEvent)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("DamageProcessor not initialized"));
		return false;
	}

	double StartTime = FPlatformTime::Seconds();

	// Validate attack event
	if (!AttackEvent.Attacker.IsValid() || !AttackEvent.Target.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid attacker or target in attack event"));
		return false;
	}

	// Set up damage calculation parameters
	FDamageCalculationParams DamageParams;
	DamageParams.BaseDamage = AttackEvent.DamageAmount;
	DamageParams.DamageType = AttackEvent.DamageType;
	DamageParams.Attacker = AttackEvent.Attacker;
	DamageParams.Target = AttackEvent.Target;
	DamageParams.HitLocation = AttackEvent.HitLocation;
	// Critical chance and multiplier can be customized per attack if needed

	// Calculate damage
	FDamageCalculationResult DamageResult = CalculateDamage(DamageParams);

	// Apply damage to target
	float ActualDamage = ApplyDamageToTarget(AttackEvent.Target.Get(), DamageResult, AttackEvent.Attacker.Get());

	// Update statistics
	double ProcessingTime = (FPlatformTime::Seconds() - StartTime) * 1000.0; // Convert to milliseconds
	UpdateStatistics(DamageResult, ProcessingTime);

	// Broadcast events
	BroadcastDamageEvent(AttackEvent.Attacker.Get(), AttackEvent.Target.Get(), DamageResult);

	if (bVerboseLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("Processed attack: %s -> %s, Damage: %f (Base: %f), Critical: %s, Blocked: %s"),
			*AttackEvent.Attacker->GetName(),
			*AttackEvent.Target->GetName(),
			DamageResult.FinalDamage,
			DamageParams.BaseDamage,
			DamageResult.bIsCritical ? TEXT("Yes") : TEXT("No"),
			DamageResult.bWasBlocked ? TEXT("Yes") : TEXT("No")
		);
	}

	return ActualDamage > 0.0f;
}

FDamageCalculationResult UOdysseyDamageProcessor::CalculateDamage(const FDamageCalculationParams& Params)
{
	FDamageCalculationResult Result;

	// Start with base damage
	float CalculatedDamage = Params.BaseDamage;
	Result.DamageMultiplier = 1.0f;

	// Apply global damage multiplier
	CalculatedDamage *= GlobalDamageMultiplier;
	Result.DamageMultiplier *= GlobalDamageMultiplier;

	// Apply damage type multipliers
	const float* TypeMultiplier = DamageTypeMultipliers.Find(Params.DamageType);
	if (TypeMultiplier)
	{
		CalculatedDamage *= *TypeMultiplier;
		Result.DamageMultiplier *= *TypeMultiplier;
	}

	// Apply custom damage modifiers from parameters
	for (const auto& Modifier : Params.DamageModifiers)
	{
		CalculatedDamage *= Modifier.Value;
		Result.DamageMultiplier *= Modifier.Value;
	}

	// Check for critical hit
	if (bCriticalHitsEnabled)
	{
		Result.bIsCritical = CalculateCriticalHit(Params);
		if (Result.bIsCritical)
		{
			float CritMultiplier = Params.CriticalMultiplier > 0.0f ? Params.CriticalMultiplier : GlobalCriticalMultiplier;
			CalculatedDamage *= CritMultiplier;
			Result.DamageMultiplier *= CritMultiplier;
		}
	}

	// Check for blocking (could be expanded for shield systems, etc.)
	Result.bWasBlocked = CalculateBlocking(Params);
	if (Result.bWasBlocked)
	{
		// For now, blocking reduces damage by 50%. This could be made configurable.
		CalculatedDamage *= 0.5f;
		Result.DamageMultiplier *= 0.5f;
	}

	// Ensure minimum damage of 0
	Result.FinalDamage = FMath::Max(0.0f, CalculatedDamage);

	// Generate calculation details for debugging
	if (bVerboseLogging)
	{
		Result.CalculationDetails = FString::Printf(
			TEXT("Base: %.1f, Global: %.2f, Type: %.2f, Critical: %s (%.2fx), Blocked: %s, Final: %.1f"),
			Params.BaseDamage,
			GlobalDamageMultiplier,
			TypeMultiplier ? *TypeMultiplier : 1.0f,
			Result.bIsCritical ? TEXT("Yes") : TEXT("No"),
			Result.bIsCritical ? (Params.CriticalMultiplier > 0.0f ? Params.CriticalMultiplier : GlobalCriticalMultiplier) : 1.0f,
			Result.bWasBlocked ? TEXT("Yes") : TEXT("No"),
			Result.FinalDamage
		);
	}

	return Result;
}

float UOdysseyDamageProcessor::ApplyDamageToTarget(AActor* Target, const FDamageCalculationResult& DamageResult, AActor* Attacker)
{
	if (!Target)
	{
		return 0.0f;
	}

	// Find health component on target
	UNPCHealthComponent* HealthComponent = FindHealthComponent(Target);
	if (!HealthComponent)
	{
		// Target doesn't have a health component, try UE5 built-in damage system as fallback
		if (APawn* TargetPawn = Cast<APawn>(Target))
		{
			// Apply damage through UE5 damage system
			float ActualDamage = TargetPawn->TakeDamage(DamageResult.FinalDamage, FDamageEvent(), nullptr, Attacker);
			
			if (bVerboseLogging)
			{
				UE_LOG(LogTemp, Log, TEXT("Applied %f damage to %s via UE5 damage system"), ActualDamage, *Target->GetName());
			}
			
			return ActualDamage;
		}
		
		UE_LOG(LogTemp, Warning, TEXT("Target %s has no health component and is not a Pawn"), *Target->GetName());
		return 0.0f;
	}

	// Apply damage through our health component system
	FName DamageTypeName = TEXT("Combat"); // Default damage type for combat
	float ActualDamage = HealthComponent->TakeDamage(DamageResult.FinalDamage, Attacker, DamageTypeName);

	// Check if target was killed
	if (HealthComponent->IsDead() && ActualDamage > 0.0f)
	{
		HandleActorKilled(Attacker, Target);
	}

	return ActualDamage;
}

float UOdysseyDamageProcessor::DealDamage(AActor* Target, float DamageAmount, FName DamageType, AActor* Attacker)
{
	if (!Target || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// Create damage calculation parameters
	FDamageCalculationParams DamageParams;
	DamageParams.BaseDamage = DamageAmount;
	DamageParams.DamageType = DamageType;
	DamageParams.Attacker = Attacker;
	DamageParams.Target = Target;

	// Calculate and apply damage
	FDamageCalculationResult Result = CalculateDamage(DamageParams);
	float ActualDamage = ApplyDamageToTarget(Target, Result, Attacker);

	// Broadcast events
	BroadcastDamageEvent(Attacker, Target, Result);

	return ActualDamage;
}

void UOdysseyDamageProcessor::SetGlobalDamageMultiplier(float Multiplier)
{
	GlobalDamageMultiplier = FMath::Max(0.0f, Multiplier);
	UE_LOG(LogTemp, Log, TEXT("Global damage multiplier set to %f"), GlobalDamageMultiplier);
}

void UOdysseyDamageProcessor::SetDamageTypeMultiplier(FName DamageType, float Multiplier)
{
	Multiplier = FMath::Max(0.0f, Multiplier);
	
	if (Multiplier == 1.0f)
	{
		DamageTypeMultipliers.Remove(DamageType);
	}
	else
	{
		DamageTypeMultipliers.Add(DamageType, Multiplier);
	}
	
	UE_LOG(LogTemp, Log, TEXT("Damage multiplier for %s set to %f"), *DamageType.ToString(), Multiplier);
}

void UOdysseyDamageProcessor::SetCriticalHitsEnabled(bool bEnabled)
{
	bCriticalHitsEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("Critical hits %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UOdysseyDamageProcessor::SetGlobalCriticalChance(float CriticalChance)
{
	GlobalCriticalChance = FMath::Clamp(CriticalChance, 0.0f, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("Global critical chance set to %f"), GlobalCriticalChance);
}

void UOdysseyDamageProcessor::SetGlobalCriticalMultiplier(float CriticalMultiplier)
{
	GlobalCriticalMultiplier = FMath::Max(1.0f, CriticalMultiplier);
	UE_LOG(LogTemp, Log, TEXT("Global critical multiplier set to %f"), GlobalCriticalMultiplier);
}

void UOdysseyDamageProcessor::ResetStatistics()
{
	ProcessorStats = FDamageProcessorStats();
	TotalProcessingTime = 0.0;
	ProcessingTimeSamples = 0;
	
	UE_LOG(LogTemp, Log, TEXT("Damage processor statistics reset"));
}

UOdysseyDamageProcessor* UOdysseyDamageProcessor::Get()
{
	if (!GlobalInstance)
	{
		// Create a new instance if none exists
		GlobalInstance = NewObject<UOdysseyDamageProcessor>();
		if (GlobalInstance)
		{
			GlobalInstance->AddToRoot(); // Prevent garbage collection
			GlobalInstance->Initialize();
		}
	}
	
	return GlobalInstance;
}

void UOdysseyDamageProcessor::InitializeEventSubscriptions()
{
	EventBus = UOdysseyEventBus::Get();
	if (!EventBus)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find OdysseyEventBus instance"));
		return;
	}

	// Subscribe to attack hit events
	FOdysseyEventHandle AttackHitHandle = EventBus->Subscribe(
		EOdysseyEventType::AttackHit,
		[this](const FOdysseyEventPayload& Payload)
		{
			OnAttackHitEvent(Payload);
		},
		FOdysseyEventFilter(), // No filter - process all attack hits
		150 // Very high priority for damage processing
	);

	if (AttackHitHandle.IsValid())
	{
		EventSubscriptionHandles.Add(AttackHitHandle);
	}

	UE_LOG(LogTemp, Log, TEXT("Damage processor event subscriptions initialized"));
}

void UOdysseyDamageProcessor::CleanupEventSubscriptions()
{
	if (EventBus)
	{
		for (FOdysseyEventHandle& Handle : EventSubscriptionHandles)
		{
			EventBus->Unsubscribe(Handle);
		}
	}
	
	EventSubscriptionHandles.Empty();
}

void UOdysseyDamageProcessor::OnAttackHitEvent(const FOdysseyEventPayload& Payload)
{
	if (const FCombatEventPayload* CombatEvent = static_cast<const FCombatEventPayload*>(&Payload))
	{
		ProcessAttackHit(*CombatEvent);
	}
}

bool UOdysseyDamageProcessor::CalculateCriticalHit(const FDamageCalculationParams& Params) const
{
	// Use parameter-specific critical chance if provided, otherwise use global
	float CritChance = Params.CriticalChance >= 0.0f ? Params.CriticalChance : GlobalCriticalChance;
	
	// Add any attacker-specific critical chance modifiers here
	// For example, you could query the attacker for critical chance bonuses
	
	return FMath::RandRange(0.0f, 1.0f) <= CritChance;
}

bool UOdysseyDamageProcessor::CalculateBlocking(const FDamageCalculationParams& Params) const
{
	// Basic blocking calculation - can be expanded to include:
	// - Shield systems
	// - Armor mechanics
	// - Directional blocking
	// - Block chance based on target stats
	
	// For now, just return false (no blocking)
	// This can be enhanced later when shield/armor systems are implemented
	return false;
}

float UOdysseyDamageProcessor::ApplyDamageModifiers(float BaseDamage, const FDamageCalculationParams& Params) const
{
	float ModifiedDamage = BaseDamage;
	
	// Apply each modifier
	for (const auto& Modifier : Params.DamageModifiers)
	{
		ModifiedDamage *= Modifier.Value;
	}
	
	return ModifiedDamage;
}

UNPCHealthComponent* UOdysseyDamageProcessor::FindHealthComponent(AActor* Target) const
{
	if (!Target)
	{
		return nullptr;
	}

	// Look for NPCHealthComponent specifically
	return Target->FindComponentByClass<UNPCHealthComponent>();
}

void UOdysseyDamageProcessor::UpdateStatistics(const FDamageCalculationResult& Result, double ProcessingTimeMs)
{
	ProcessorStats.TotalDamageEventsProcessed++;
	ProcessorStats.TotalDamageDealt += static_cast<int64>(Result.FinalDamage);
	
	if (Result.bIsCritical)
	{
		ProcessorStats.CriticalHits++;
	}
	
	if (Result.bWasBlocked)
	{
		ProcessorStats.BlockedAttacks++;
	}
	
	// Update processing time averages
	TotalProcessingTime += ProcessingTimeMs;
	ProcessingTimeSamples++;
	ProcessorStats.AverageProcessingTimeMs = TotalProcessingTime / ProcessingTimeSamples;
}

void UOdysseyDamageProcessor::BroadcastDamageEvent(AActor* Attacker, AActor* Target, const FDamageCalculationResult& Result)
{
	// Broadcast to Blueprint delegates
	OnDamageProcessed.Broadcast(Attacker, Target, Result);

	// Create and publish damage dealt event to the event bus
	if (EventBus && Result.FinalDamage > 0.0f)
	{
		FCombatEventPayload DamageEvent;
		DamageEvent.Initialize(EOdysseyEventType::DamageDealt, Attacker);
		DamageEvent.Attacker = Attacker;
		DamageEvent.Target = Target;
		DamageEvent.DamageAmount = Result.FinalDamage;
		DamageEvent.bIsCritical = Result.bIsCritical;
		DamageEvent.bWasBlocked = Result.bWasBlocked;

		TSharedPtr<FCombatEventPayload> SharedPayload = MakeShared<FCombatEventPayload>(DamageEvent);
		EventBus->PublishEvent(StaticCastSharedPtr<FOdysseyEventPayload>(SharedPayload));
	}
}

void UOdysseyDamageProcessor::HandleActorKilled(AActor* Killer, AActor* Victim)
{
	ProcessorStats.KillsProcessed++;
	
	// Broadcast kill event
	OnActorKilled.Broadcast(Killer, Victim);
	
	UE_LOG(LogTemp, Log, TEXT("%s killed %s"), 
		Killer ? *Killer->GetName() : TEXT("Unknown"),
		Victim ? *Victim->GetName() : TEXT("Unknown"));
}
