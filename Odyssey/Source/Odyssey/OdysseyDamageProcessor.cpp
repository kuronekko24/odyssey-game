// OdysseyDamageProcessor.cpp
// Implementation of the central damage processing system
// Phase 1: Health & Damage Foundation

#include "OdysseyDamageProcessor.h"
#include "NPCHealthComponent.h"
#include "OdysseyEventBus.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"

// ============================================================================
// Log Category
// ============================================================================

DEFINE_LOG_CATEGORY_STATIC(LogDamageProcessor, Log, All);

// ============================================================================
// Singleton
// ============================================================================

UOdysseyDamageProcessor* UOdysseyDamageProcessor::GlobalInstance = nullptr;

UOdysseyDamageProcessor* UOdysseyDamageProcessor::Get()
{
	if (!GlobalInstance)
	{
		GlobalInstance = NewObject<UOdysseyDamageProcessor>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		if (GlobalInstance)
		{
			GlobalInstance->Initialize();
		}
	}
	return GlobalInstance;
}

// ============================================================================
// Constructor
// ============================================================================

UOdysseyDamageProcessor::UOdysseyDamageProcessor()
{
	// Defaults
	GlobalDamageMultiplier = 1.0f;
	bCriticalHitsEnabled = true;
	GlobalCriticalChance = 0.05f;
	GlobalCriticalMultiplier = 2.0f;
	bVerboseLogging = false;

	// Distance falloff defaults
	bDistanceFalloffEnabled = false;
	FalloffMinRange = 500.0f;
	FalloffMaxRange = 2000.0f;
	FalloffExponent = 1.0f;

	// Minimum damage floor
	MinimumDamage = 1.0f;

	// Runtime state
	bIsInitialized = false;
	EventBus = nullptr;
	TotalProcessingTime = 0.0;
	ProcessingTimeSamples = 0;
	ProcessorStats = FDamageProcessorStats();
}

// ============================================================================
// Lifecycle
// ============================================================================

void UOdysseyDamageProcessor::Initialize()
{
	if (bIsInitialized)
	{
		UE_LOG(LogDamageProcessor, Warning, TEXT("DamageProcessor already initialized"));
		return;
	}

	GlobalInstance = this;
	InitializeEventSubscriptions();
	bIsInitialized = true;

	UE_LOG(LogDamageProcessor, Log, TEXT("OdysseyDamageProcessor initialized (GlobalMult=%.2f, Crit=%.0f%% @ %.1fx, Falloff=%s)"),
		GlobalDamageMultiplier, GlobalCriticalChance * 100.0f, GlobalCriticalMultiplier,
		bDistanceFalloffEnabled ? TEXT("ON") : TEXT("OFF"));
}

void UOdysseyDamageProcessor::Shutdown()
{
	if (!bIsInitialized) return;

	CleanupEventSubscriptions();
	bIsInitialized = false;

	if (GlobalInstance == this)
	{
		GlobalInstance = nullptr;
	}

	UE_LOG(LogDamageProcessor, Log, TEXT("OdysseyDamageProcessor shut down. Events processed: %lld, Kills: %lld"),
		ProcessorStats.TotalDamageEventsProcessed, ProcessorStats.KillsProcessed);
}

// ============================================================================
// Core: ProcessAttackHit
// ============================================================================

bool UOdysseyDamageProcessor::ProcessAttackHit(const FCombatEventPayload& AttackEvent)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogDamageProcessor, Error, TEXT("ProcessAttackHit called before initialization"));
		return false;
	}

	double StartTime = FPlatformTime::Seconds();

	// Validate actors
	if (!AttackEvent.Attacker.IsValid() || !AttackEvent.Target.IsValid())
	{
		UE_LOG(LogDamageProcessor, Warning, TEXT("ProcessAttackHit: Invalid attacker or target"));
		return false;
	}

	AActor* AttackerActor = AttackEvent.Attacker.Get();
	AActor* TargetActor = AttackEvent.Target.Get();

	// Build calculation parameters from the event
	FDamageCalculationParams Params;
	Params.BaseDamage = AttackEvent.DamageAmount;
	Params.DamageType = AttackEvent.DamageType;
	Params.Attacker = AttackerActor;
	Params.Target = TargetActor;
	Params.HitLocation = AttackEvent.HitLocation;

	// Calculate distance if actors are valid
	if (AttackerActor && TargetActor)
	{
		Params.Distance = FVector::Dist(AttackerActor->GetActorLocation(), TargetActor->GetActorLocation());
	}

	// Calculate damage
	FDamageCalculationResult Result = CalculateDamage(Params);

	// Apply to target
	float ActualHullDamage = ApplyDamageToTarget(TargetActor, Result, AttackerActor, Params.DamageType);

	// Get shield absorption for stats
	float ShieldAbsorbed = Result.FinalDamage - ActualHullDamage;
	if (ShieldAbsorbed < 0.0f) ShieldAbsorbed = 0.0f;

	// Update statistics
	double ProcessingTimeMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
	UpdateStatistics(Result, ShieldAbsorbed, ProcessingTimeMs);

	// Publish events
	PublishDamageDealtEvent(AttackerActor, TargetActor, Result);
	OnDamageProcessed.Broadcast(AttackerActor, TargetActor, Result);

	if (bVerboseLogging)
	{
		UE_LOG(LogDamageProcessor, Log,
			TEXT("AttackHit: %s -> %s | Base=%.1f Final=%.1f Hull=%.1f Shield=%.1f Crit=%s Blocked=%s Falloff=%.2f [%.3fms]"),
			*AttackerActor->GetName(), *TargetActor->GetName(),
			Params.BaseDamage, Result.FinalDamage, ActualHullDamage, ShieldAbsorbed,
			Result.bIsCritical ? TEXT("Y") : TEXT("N"),
			Result.bWasBlocked ? TEXT("Y") : TEXT("N"),
			Result.DistanceFalloff,
			ProcessingTimeMs);
	}

	return ActualHullDamage > 0.0f;
}

// ============================================================================
// Core: CalculateDamage
// ============================================================================

FDamageCalculationResult UOdysseyDamageProcessor::CalculateDamage(const FDamageCalculationParams& Params)
{
	FDamageCalculationResult Result;
	float Damage = Params.BaseDamage;
	float TotalMultiplier = 1.0f;

	// 1. Global damage multiplier
	Damage *= GlobalDamageMultiplier;
	TotalMultiplier *= GlobalDamageMultiplier;

	// 2. Damage type multiplier
	const float* TypeMult = DamageTypeMultipliers.Find(Params.DamageType);
	if (TypeMult)
	{
		Damage *= *TypeMult;
		TotalMultiplier *= *TypeMult;
	}

	// 3. Per-attack named modifiers
	for (const auto& Mod : Params.DamageModifiers)
	{
		Damage *= Mod.Value;
		TotalMultiplier *= Mod.Value;
	}

	// 4. Distance falloff
	Result.DistanceFalloff = 1.0f;
	if (bDistanceFalloffEnabled && Params.Distance > 0.0f)
	{
		Result.DistanceFalloff = CalculateDistanceFalloff(Params.Distance);
		Damage *= Result.DistanceFalloff;
		TotalMultiplier *= Result.DistanceFalloff;
	}

	// 5. Critical hit
	if (bCriticalHitsEnabled)
	{
		Result.bIsCritical = RollCriticalHit(Params);
		if (Result.bIsCritical)
		{
			float CritMult = (Params.CriticalMultiplier > 0.0f) ? Params.CriticalMultiplier : GlobalCriticalMultiplier;
			Damage *= CritMult;
			TotalMultiplier *= CritMult;
		}
	}

	// 6. Blocking (placeholder for future shield/armor systems)
	Result.bWasBlocked = false; // Will be expanded in Phase 2

	// 7. Enforce minimum damage floor
	if (Damage > 0.0f && Damage < MinimumDamage)
	{
		Damage = MinimumDamage;
	}

	// Final result
	Result.FinalDamage = FMath::Max(0.0f, Damage);
	Result.DamageMultiplier = TotalMultiplier;

	// Debug details string
	if (bVerboseLogging)
	{
		Result.CalculationDetails = FString::Printf(
			TEXT("Base=%.1f GlobalMult=%.2f TypeMult=%.2f Falloff=%.2f Crit=%s(%.1fx) Final=%.1f"),
			Params.BaseDamage,
			GlobalDamageMultiplier,
			TypeMult ? *TypeMult : 1.0f,
			Result.DistanceFalloff,
			Result.bIsCritical ? TEXT("Y") : TEXT("N"),
			Result.bIsCritical ? ((Params.CriticalMultiplier > 0.0f) ? Params.CriticalMultiplier : GlobalCriticalMultiplier) : 1.0f,
			Result.FinalDamage);
	}

	return Result;
}

// ============================================================================
// Core: ApplyDamageToTarget
// ============================================================================

float UOdysseyDamageProcessor::ApplyDamageToTarget(AActor* Target, const FDamageCalculationResult& DamageResult, AActor* Attacker, FName DamageType)
{
	if (!Target || DamageResult.FinalDamage <= 0.0f)
	{
		return 0.0f;
	}

	// Look for our health component first
	UNPCHealthComponent* HealthComp = FindHealthComponent(Target);
	if (HealthComp)
	{
		float HullDamage = HealthComp->TakeDamageEx(
			DamageResult.FinalDamage,
			Attacker,
			DamageType.IsNone() ? TEXT("Combat") : DamageType,
			DamageResult.bIsCritical);

		// Check for kill
		if (HealthComp->IsDead() && HullDamage > 0.0f)
		{
			HandleActorKilled(Attacker, Target);
		}

		return HullDamage;
	}

	// Fallback: UE built-in damage system for actors without our component
	if (APawn* TargetPawn = Cast<APawn>(Target))
	{
		float AppliedDamage = TargetPawn->TakeDamage(DamageResult.FinalDamage, FDamageEvent(), nullptr, Attacker);

		if (bVerboseLogging)
		{
			UE_LOG(LogDamageProcessor, Log, TEXT("Fallback UE damage on %s: %.1f"), *Target->GetName(), AppliedDamage);
		}

		return AppliedDamage;
	}

	UE_LOG(LogDamageProcessor, Warning, TEXT("Target %s has no health component and is not a Pawn"), *Target->GetName());
	return 0.0f;
}

// ============================================================================
// Convenience: DealDamage
// ============================================================================

float UOdysseyDamageProcessor::DealDamage(AActor* Target, float DamageAmount, FName DamageType, AActor* Attacker)
{
	if (!Target || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	FDamageCalculationParams Params;
	Params.BaseDamage = DamageAmount;
	Params.DamageType = DamageType;
	Params.Attacker = Attacker;
	Params.Target = Target;

	if (Attacker && Target)
	{
		Params.Distance = FVector::Dist(Attacker->GetActorLocation(), Target->GetActorLocation());
	}

	FDamageCalculationResult Result = CalculateDamage(Params);
	float ActualDamage = ApplyDamageToTarget(Target, Result, Attacker, DamageType);

	// Broadcast
	PublishDamageDealtEvent(Attacker, Target, Result);
	OnDamageProcessed.Broadcast(Attacker, Target, Result);

	return ActualDamage;
}

// ============================================================================
// Configuration Setters
// ============================================================================

void UOdysseyDamageProcessor::SetGlobalDamageMultiplier(float Multiplier)
{
	GlobalDamageMultiplier = FMath::Max(0.0f, Multiplier);
}

void UOdysseyDamageProcessor::SetDamageTypeMultiplier(FName DamageType, float Multiplier)
{
	Multiplier = FMath::Max(0.0f, Multiplier);
	if (FMath::IsNearlyEqual(Multiplier, 1.0f, KINDA_SMALL_NUMBER))
	{
		DamageTypeMultipliers.Remove(DamageType);
	}
	else
	{
		DamageTypeMultipliers.Add(DamageType, Multiplier);
	}
}

void UOdysseyDamageProcessor::SetCriticalHitsEnabled(bool bEnabled)
{
	bCriticalHitsEnabled = bEnabled;
}

void UOdysseyDamageProcessor::SetGlobalCriticalChance(float CriticalChance)
{
	GlobalCriticalChance = FMath::Clamp(CriticalChance, 0.0f, 1.0f);
}

void UOdysseyDamageProcessor::SetGlobalCriticalMultiplier(float CritMult)
{
	GlobalCriticalMultiplier = FMath::Max(1.0f, CritMult);
}

void UOdysseyDamageProcessor::SetDistanceFalloffEnabled(bool bEnabled)
{
	bDistanceFalloffEnabled = bEnabled;
}

void UOdysseyDamageProcessor::SetDistanceFalloffParams(float MinRange, float MaxRange, float Exponent)
{
	FalloffMinRange = FMath::Max(0.0f, MinRange);
	FalloffMaxRange = FMath::Max(FalloffMinRange + 1.0f, MaxRange);
	FalloffExponent = FMath::Max(0.1f, Exponent);
}

void UOdysseyDamageProcessor::SetMinimumDamage(float MinDamage)
{
	MinimumDamage = FMath::Max(0.0f, MinDamage);
}

// ============================================================================
// Statistics
// ============================================================================

void UOdysseyDamageProcessor::ResetStatistics()
{
	ProcessorStats = FDamageProcessorStats();
	TotalProcessingTime = 0.0;
	ProcessingTimeSamples = 0;
}

void UOdysseyDamageProcessor::UpdateStatistics(const FDamageCalculationResult& Result, float ShieldAbsorbed, double ProcessingTimeMs)
{
	ProcessorStats.TotalDamageEventsProcessed++;
	ProcessorStats.TotalDamageDealt += static_cast<int64>(Result.FinalDamage);
	ProcessorStats.TotalShieldDamageAbsorbed += static_cast<int64>(ShieldAbsorbed);

	if (Result.bIsCritical)
	{
		ProcessorStats.CriticalHits++;
	}
	if (Result.bWasBlocked)
	{
		ProcessorStats.BlockedAttacks++;
	}

	TotalProcessingTime += ProcessingTimeMs;
	ProcessingTimeSamples++;
	ProcessorStats.AverageProcessingTimeMs = (ProcessingTimeSamples > 0) ? (TotalProcessingTime / ProcessingTimeSamples) : 0.0;
}

// ============================================================================
// Internal: Critical Hit Roll
// ============================================================================

bool UOdysseyDamageProcessor::RollCriticalHit(const FDamageCalculationParams& Params) const
{
	float CritChance = (Params.CriticalChance >= 0.0f) ? Params.CriticalChance : GlobalCriticalChance;
	return FMath::FRand() <= CritChance;
}

// ============================================================================
// Internal: Distance Falloff
// ============================================================================

float UOdysseyDamageProcessor::CalculateDistanceFalloff(float Distance) const
{
	if (!bDistanceFalloffEnabled || Distance <= FalloffMinRange)
	{
		return 1.0f;
	}

	if (Distance >= FalloffMaxRange)
	{
		return 0.0f;
	}

	// Normalized distance within falloff range [0, 1]
	float T = (Distance - FalloffMinRange) / (FalloffMaxRange - FalloffMinRange);
	// Apply exponent for curve shaping
	float Falloff = 1.0f - FMath::Pow(T, FalloffExponent);
	return FMath::Clamp(Falloff, 0.0f, 1.0f);
}

// ============================================================================
// Internal: Find Health Component
// ============================================================================

UNPCHealthComponent* UOdysseyDamageProcessor::FindHealthComponent(AActor* Target) const
{
	if (!Target) return nullptr;
	return Target->FindComponentByClass<UNPCHealthComponent>();
}

// ============================================================================
// Internal: Event Bus Integration
// ============================================================================

void UOdysseyDamageProcessor::InitializeEventSubscriptions()
{
	EventBus = UOdysseyEventBus::Get();
	if (!EventBus)
	{
		UE_LOG(LogDamageProcessor, Error, TEXT("Could not find OdysseyEventBus instance"));
		return;
	}

	// Subscribe to AttackHit events (no filter -- process all attacks)
	FOdysseyEventHandle AttackHandle = EventBus->Subscribe(
		EOdysseyEventType::AttackHit,
		[this](const FOdysseyEventPayload& Payload)
		{
			OnAttackHitEvent(Payload);
		},
		FOdysseyEventFilter(),
		150 // Very high priority for damage processing
	);

	if (AttackHandle.IsValid())
	{
		EventSubscriptionHandles.Add(AttackHandle);
	}

	UE_LOG(LogDamageProcessor, Log, TEXT("Event subscriptions initialized"));
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
	const FCombatEventPayload* CombatEvent = static_cast<const FCombatEventPayload*>(&Payload);
	if (CombatEvent)
	{
		ProcessAttackHit(*CombatEvent);
	}
}

void UOdysseyDamageProcessor::PublishDamageDealtEvent(AActor* Attacker, AActor* Target, const FDamageCalculationResult& Result)
{
	if (!EventBus || Result.FinalDamage <= 0.0f)
	{
		return;
	}

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

void UOdysseyDamageProcessor::HandleActorKilled(AActor* Killer, AActor* Victim)
{
	ProcessorStats.KillsProcessed++;
	OnActorKilled.Broadcast(Killer, Victim);

	UE_LOG(LogDamageProcessor, Log, TEXT("Kill: %s -> %s"),
		Killer ? *Killer->GetName() : TEXT("Unknown"),
		Victim ? *Victim->GetName() : TEXT("Unknown"));
}
