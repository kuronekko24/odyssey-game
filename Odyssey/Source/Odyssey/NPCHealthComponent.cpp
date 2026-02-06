// NPCHealthComponent.cpp
// Implementation of component-based health tracking for NPC ships
// Phase 1: Health & Damage Foundation

#include "NPCHealthComponent.h"
#include "OdysseyEventBus.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

// ============================================================================
// Log Category
// ============================================================================

DEFINE_LOG_CATEGORY_STATIC(LogNPCHealth, Log, All);

// ============================================================================
// Constructor
// ============================================================================

UNPCHealthComponent::UNPCHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 10 Hz is sufficient for regen ticks and DOT processing on mobile
	PrimaryComponentTick.TickInterval = 0.1f;

	// Hull defaults
	MaxHealth = 100.0f;
	StartingHealthPercentage = 1.0f;
	CurrentHealth = MaxHealth;
	CurrentHealthState = EHealthState::Healthy;

	// Shield defaults
	MaxShields = 0.0f;
	StartingShieldPercentage = 1.0f;
	CurrentShields = 0.0f;
	ShieldBleedThroughRatio = 0.0f;

	// Health regeneration defaults
	bHealthRegenEnabled = false;
	HealthRegenRate = 5.0f;
	HealthRegenDelay = 3.0f;
	bOnlyRegenOutOfCombat = true;
	OutOfCombatTime = 5.0f;

	// Shield regeneration defaults
	bShieldRegenEnabled = true;
	ShieldRegenRate = 10.0f;
	ShieldRegenDelay = 3.0f;

	// Resistance defaults
	FlatDamageReduction = 0.0f;

	// Death & events
	bCanDie = true;
	bBroadcastToEventBus = true;

	// Visual defaults
	HealthBarVisibilityDuration = 5.0f;
	bOnlyShowHealthBarWhenDamaged = true;

	// Runtime state
	TimeSinceLastDamage = 999.0f;
	TimeSinceLastShieldDamage = 999.0f;
	bIsHealthRegenerating = false;
	bIsShieldRegenerating = false;
	bShieldsWereFull = true;
	EventBus = nullptr;
}

// ============================================================================
// Lifecycle
// ============================================================================

void UNPCHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize health and shields from starting percentages
	CurrentHealth = MaxHealth * StartingHealthPercentage;
	CurrentShields = MaxShields * StartingShieldPercentage;
	bShieldsWereFull = (MaxShields <= 0.0f) || FMath::IsNearlyEqual(CurrentShields, MaxShields, 0.1f);

	UpdateHealthState();
	InitializeEventBusSubscriptions();

	UE_LOG(LogNPCHealth, Log, TEXT("[%s] Initialized: Hull=%.0f/%.0f, Shields=%.0f/%.0f"),
		*GetOwner()->GetName(), CurrentHealth, MaxHealth, CurrentShields, MaxShields);
}

void UNPCHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupEventBusSubscriptions();
	ActiveDOTEffects.Empty();
	Super::EndPlay(EndPlayReason);
}

void UNPCHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsDead())
	{
		return;
	}

	// Advance combat timers
	TimeSinceLastDamage += DeltaTime;
	TimeSinceLastShieldDamage += DeltaTime;

	// Process systems
	ProcessDamageOverTime(DeltaTime);
	ProcessHealthRegeneration(DeltaTime);
	ProcessShieldRegeneration(DeltaTime);
}

// ============================================================================
// Core Damage Application
// ============================================================================

float UNPCHealthComponent::TakeDamage(float DamageAmount, AActor* DamageSource, FName DamageType)
{
	return TakeDamageEx(DamageAmount, DamageSource, DamageType, false);
}

float UNPCHealthComponent::TakeDamageEx(float DamageAmount, AActor* DamageSource, FName DamageType, bool bIsCritical)
{
	if (IsDead() || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// Apply resistances and flat reduction
	float ProcessedDamage = CalculateActualDamage(DamageAmount, DamageType);
	if (ProcessedDamage <= 0.0f)
	{
		return 0.0f;
	}

	// Snapshot pre-damage state
	const float PrevHealth = CurrentHealth;
	const float PrevShields = CurrentShields;
	const EHealthState PrevState = CurrentHealthState;

	// Route through shields then hull
	float HullDamage = ApplyDamageToShieldsAndHealth(ProcessedDamage, DamageSource, DamageType, bIsCritical);
	float ShieldAbsorbed = ProcessedDamage - HullDamage;

	// Reset combat timers
	TimeSinceLastDamage = 0.0f;
	if (ShieldAbsorbed > 0.0f)
	{
		TimeSinceLastShieldDamage = 0.0f;
	}
	bIsHealthRegenerating = false;
	bIsShieldRegenerating = false;

	// Update health state
	UpdateHealthState();

	// Broadcast events
	BroadcastHealthChangeEvent(PrevHealth, PrevShields, ProcessedDamage, ShieldAbsorbed,
		DamageSource, DamageType, PrevState, bIsCritical);

	// Handle death
	if (CurrentHealth <= 0.0f && bCanDie)
	{
		HandleDeath(DamageSource);
	}

	UE_LOG(LogNPCHealth, Log, TEXT("[%s] Took %.1f dmg (raw=%.1f, type=%s, crit=%s). Shields: %.0f/%.0f, Hull: %.0f/%.0f"),
		*GetOwner()->GetName(), ProcessedDamage, DamageAmount, *DamageType.ToString(),
		bIsCritical ? TEXT("Y") : TEXT("N"),
		CurrentShields, MaxShields, CurrentHealth, MaxHealth);

	return HullDamage;
}

float UNPCHealthComponent::CalculateActualDamage(float BaseDamage, FName DamageType) const
{
	float Damage = BaseDamage;

	// Apply percentage resistance (skip for True damage)
	if (DamageType != TEXT("True"))
	{
		float Resistance = GetDamageResistance(DamageType);
		if (Resistance > 0.0f)
		{
			Damage *= (1.0f - Resistance);
		}
	}

	// Apply flat reduction
	if (FlatDamageReduction > 0.0f && DamageType != TEXT("True"))
	{
		Damage -= FlatDamageReduction;
	}

	return FMath::Max(0.0f, Damage);
}

float UNPCHealthComponent::ApplyDamageToShieldsAndHealth(float ProcessedDamage, AActor* Source, FName DamageType, bool bIsCritical)
{
	float HullDamage = 0.0f;
	float RemainingDamage = ProcessedDamage;

	// Shields absorb damage first
	if (CurrentShields > 0.0f && RemainingDamage > 0.0f)
	{
		float ShieldDamage = FMath::Min(RemainingDamage, CurrentShields);
		CurrentShields -= ShieldDamage;
		RemainingDamage -= ShieldDamage;

		// Shield bleed-through: a portion of overflow damage passes through even if shields hold
		// This only applies when shields are broken (RemainingDamage > 0 means shields depleted)

		// Check if shields just broke
		if (CurrentShields <= 0.0f)
		{
			CurrentShields = 0.0f;
			OnShieldBroken.Broadcast(GetOwner(), Source);
			UE_LOG(LogNPCHealth, Warning, TEXT("[%s] Shields broken!"), *GetOwner()->GetName());
		}
	}

	// Apply bleed-through ratio if configured (percentage of shield-absorbed damage leaks to hull)
	if (ShieldBleedThroughRatio > 0.0f)
	{
		float BleedDamage = (ProcessedDamage - RemainingDamage) * ShieldBleedThroughRatio;
		RemainingDamage += BleedDamage;
	}

	// Remaining damage goes to hull
	if (RemainingDamage > 0.0f)
	{
		float PrevHealth = CurrentHealth;
		CurrentHealth = FMath::Max(0.0f, CurrentHealth - RemainingDamage);

		// Immortal actors clamp to 1 HP
		if (!bCanDie && CurrentHealth <= 0.0f)
		{
			CurrentHealth = 1.0f;
		}

		HullDamage = PrevHealth - CurrentHealth;
	}

	return HullDamage;
}

// ============================================================================
// Healing & Shields
// ============================================================================

float UNPCHealthComponent::Heal(float HealAmount, AActor* HealSource)
{
	if (IsDead() || HealAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float PrevHealth = CurrentHealth;
	const float PrevShields = CurrentShields;
	const EHealthState PrevState = CurrentHealthState;

	float ActualHealing = FMath::Min(HealAmount, MaxHealth - CurrentHealth);
	CurrentHealth += ActualHealing;

	UpdateHealthState();

	if (ActualHealing > 0.0f)
	{
		BroadcastHealthChangeEvent(PrevHealth, PrevShields, -ActualHealing, 0.0f,
			HealSource, TEXT("Healing"), PrevState, false);
	}

	UE_LOG(LogNPCHealth, Log, TEXT("[%s] Healed %.1f. Hull: %.0f/%.0f"),
		*GetOwner()->GetName(), ActualHealing, CurrentHealth, MaxHealth);

	return ActualHealing;
}

float UNPCHealthComponent::RestoreShields(float ShieldAmount, AActor* Source)
{
	if (IsDead() || ShieldAmount <= 0.0f || MaxShields <= 0.0f)
	{
		return 0.0f;
	}

	const float PrevShields = CurrentShields;
	float ActualRestore = FMath::Min(ShieldAmount, MaxShields - CurrentShields);
	CurrentShields += ActualRestore;

	// Check if shields are fully restored
	if (ActualRestore > 0.0f && FMath::IsNearlyEqual(CurrentShields, MaxShields, 0.1f) && !bShieldsWereFull)
	{
		bShieldsWereFull = true;
		OnShieldRestored.Broadcast(GetOwner(), CurrentShields);
		UE_LOG(LogNPCHealth, Log, TEXT("[%s] Shields fully restored"), *GetOwner()->GetName());
	}

	return ActualRestore;
}

// ============================================================================
// Health & Shield Setters
// ============================================================================

void UNPCHealthComponent::SetHealth(float NewHealth, bool bBroadcastEvent)
{
	const float PrevHealth = CurrentHealth;
	const float PrevShields = CurrentShields;
	const EHealthState PrevState = CurrentHealthState;

	CurrentHealth = FMath::Clamp(NewHealth, bCanDie ? 0.0f : 1.0f, MaxHealth);
	UpdateHealthState();

	if (bBroadcastEvent)
	{
		float Delta = CurrentHealth - PrevHealth;
		BroadcastHealthChangeEvent(PrevHealth, PrevShields, -Delta, 0.0f,
			nullptr, TEXT("SetHealth"), PrevState, false);
	}

	if (CurrentHealth <= 0.0f && bCanDie)
	{
		HandleDeath(nullptr);
	}
}

void UNPCHealthComponent::SetShields(float NewShields, bool bBroadcastEvent)
{
	const float PrevShields = CurrentShields;
	CurrentShields = FMath::Clamp(NewShields, 0.0f, MaxShields);

	if (bBroadcastEvent && !FMath::IsNearlyEqual(PrevShields, CurrentShields, 0.01f))
	{
		const float PrevHealth = CurrentHealth;
		const EHealthState PrevState = CurrentHealthState;
		BroadcastHealthChangeEvent(PrevHealth, PrevShields, 0.0f, 0.0f,
			nullptr, TEXT("SetShields"), PrevState, false);
	}
}

void UNPCHealthComponent::SetMaxHealth(float NewMaxHealth, bool bMaintainHealthPercentage)
{
	if (NewMaxHealth <= 0.0f) return;

	float OldMax = MaxHealth;
	MaxHealth = NewMaxHealth;

	if (bMaintainHealthPercentage && OldMax > 0.0f)
	{
		CurrentHealth = MaxHealth * (CurrentHealth / OldMax);
	}
	else
	{
		CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
	}

	UpdateHealthState();
}

void UNPCHealthComponent::SetMaxShields(float NewMaxShields, bool bMaintainShieldPercentage)
{
	float OldMax = MaxShields;
	MaxShields = FMath::Max(0.0f, NewMaxShields);

	if (bMaintainShieldPercentage && OldMax > 0.0f)
	{
		CurrentShields = MaxShields * (CurrentShields / OldMax);
	}
	else
	{
		CurrentShields = FMath::Min(CurrentShields, MaxShields);
	}
}

void UNPCHealthComponent::Kill(AActor* KillerActor)
{
	if (IsDead()) return;

	const float PrevHealth = CurrentHealth;
	const float PrevShields = CurrentShields;
	const EHealthState PrevState = CurrentHealthState;

	CurrentShields = 0.0f;
	CurrentHealth = 0.0f;
	UpdateHealthState();

	BroadcastHealthChangeEvent(PrevHealth, PrevShields, PrevHealth + PrevShields, PrevShields,
		KillerActor, TEXT("Kill"), PrevState, false);

	HandleDeath(KillerActor);
}

// ============================================================================
// Damage Over Time
// ============================================================================

void UNPCHealthComponent::ApplyDamageOverTime(float DamagePerTick, float TickInterval, float Duration, FName DamageType, AActor* Source)
{
	if (IsDead() || DamagePerTick <= 0.0f || Duration <= 0.0f || TickInterval <= 0.0f)
	{
		return;
	}

	FDamageOverTimeEffect NewDOT;
	NewDOT.DamagePerTick = DamagePerTick;
	NewDOT.TickInterval = TickInterval;
	NewDOT.RemainingDuration = Duration;
	NewDOT.DamageType = DamageType;
	NewDOT.Source = Source;
	NewDOT.TickAccumulator = 0.0f;

	ActiveDOTEffects.Add(NewDOT);

	UE_LOG(LogNPCHealth, Log, TEXT("[%s] DOT applied: %.1f per %.1fs for %.1fs (%s)"),
		*GetOwner()->GetName(), DamagePerTick, TickInterval, Duration, *DamageType.ToString());
}

void UNPCHealthComponent::ClearAllDamageOverTime()
{
	ActiveDOTEffects.Empty();
}

void UNPCHealthComponent::ProcessDamageOverTime(float DeltaTime)
{
	if (ActiveDOTEffects.Num() == 0)
	{
		return;
	}

	// Iterate in reverse for safe removal
	for (int32 i = ActiveDOTEffects.Num() - 1; i >= 0; --i)
	{
		FDamageOverTimeEffect& DOT = ActiveDOTEffects[i];
		DOT.RemainingDuration -= DeltaTime;
		DOT.TickAccumulator += DeltaTime;

		// Apply tick damage when accumulator exceeds interval
		while (DOT.TickAccumulator >= DOT.TickInterval && !IsDead())
		{
			DOT.TickAccumulator -= DOT.TickInterval;
			TakeDamage(DOT.DamagePerTick, DOT.Source.Get(), DOT.DamageType);
		}

		// Remove expired effects
		if (DOT.RemainingDuration <= 0.0f)
		{
			ActiveDOTEffects.RemoveAtSwap(i);
		}
	}
}

// ============================================================================
// Health Queries
// ============================================================================

float UNPCHealthComponent::GetHealthPercentage() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

float UNPCHealthComponent::GetShieldPercentage() const
{
	return MaxShields > 0.0f ? CurrentShields / MaxShields : 0.0f;
}

float UNPCHealthComponent::GetEffectiveHealthPercentage() const
{
	float MaxEffective = MaxHealth + MaxShields;
	return MaxEffective > 0.0f ? (CurrentHealth + CurrentShields) / MaxEffective : 0.0f;
}

bool UNPCHealthComponent::IsAtFullHealth() const
{
	bool bHullFull = FMath::IsNearlyEqual(CurrentHealth, MaxHealth, 0.1f);
	bool bShieldsFull = MaxShields <= 0.0f || FMath::IsNearlyEqual(CurrentShields, MaxShields, 0.1f);
	return bHullFull && bShieldsFull;
}

bool UNPCHealthComponent::IsInCombat() const
{
	return TimeSinceLastDamage < OutOfCombatTime;
}

// ============================================================================
// Resistance Management
// ============================================================================

void UNPCHealthComponent::SetDamageResistance(FName DamageType, float ResistancePercentage)
{
	ResistancePercentage = FMath::Clamp(ResistancePercentage, 0.0f, 1.0f);

	if (ResistancePercentage > 0.0f)
	{
		DamageResistances.Add(DamageType, ResistancePercentage);
	}
	else
	{
		DamageResistances.Remove(DamageType);
	}
}

float UNPCHealthComponent::GetDamageResistance(FName DamageType) const
{
	const float* Resistance = DamageResistances.Find(DamageType);
	return Resistance ? *Resistance : 0.0f;
}

void UNPCHealthComponent::SetFlatDamageReduction(float ReductionAmount)
{
	FlatDamageReduction = FMath::Max(0.0f, ReductionAmount);
}

// ============================================================================
// Regeneration Configuration
// ============================================================================

void UNPCHealthComponent::SetHealthRegenEnabled(bool bEnabled)
{
	bHealthRegenEnabled = bEnabled;
	if (!bEnabled)
	{
		bIsHealthRegenerating = false;
	}
}

void UNPCHealthComponent::SetHealthRegenRate(float RegenPerSecond)
{
	HealthRegenRate = FMath::Max(0.0f, RegenPerSecond);
}

void UNPCHealthComponent::SetShieldRegenEnabled(bool bEnabled)
{
	bShieldRegenEnabled = bEnabled;
	if (!bEnabled)
	{
		bIsShieldRegenerating = false;
	}
}

void UNPCHealthComponent::SetShieldRegenRate(float RegenPerSecond)
{
	ShieldRegenRate = FMath::Max(0.0f, RegenPerSecond);
}

// ============================================================================
// Health Regeneration
// ============================================================================

void UNPCHealthComponent::ProcessHealthRegeneration(float DeltaTime)
{
	if (!bHealthRegenEnabled || CurrentHealth >= MaxHealth)
	{
		if (bIsHealthRegenerating)
		{
			bIsHealthRegenerating = false;
		}
		return;
	}

	// Check regen delay
	if (TimeSinceLastDamage < HealthRegenDelay)
	{
		return;
	}

	// Check combat restriction
	if (bOnlyRegenOutOfCombat && IsInCombat())
	{
		return;
	}

	if (!bIsHealthRegenerating)
	{
		bIsHealthRegenerating = true;
		UE_LOG(LogNPCHealth, Verbose, TEXT("[%s] Health regeneration started"), *GetOwner()->GetName());
	}

	float RegenAmount = HealthRegenRate * DeltaTime;
	if (RegenAmount > 0.0f)
	{
		Heal(RegenAmount, GetOwner());
	}
}

// ============================================================================
// Shield Regeneration
// ============================================================================

void UNPCHealthComponent::ProcessShieldRegeneration(float DeltaTime)
{
	if (!bShieldRegenEnabled || MaxShields <= 0.0f || CurrentShields >= MaxShields || IsDead())
	{
		if (bIsShieldRegenerating)
		{
			bIsShieldRegenerating = false;
		}
		return;
	}

	// Check regen delay after shield damage
	if (TimeSinceLastShieldDamage < ShieldRegenDelay)
	{
		return;
	}

	if (!bIsShieldRegenerating)
	{
		bIsShieldRegenerating = true;
		bShieldsWereFull = false;
		UE_LOG(LogNPCHealth, Verbose, TEXT("[%s] Shield regeneration started"), *GetOwner()->GetName());
	}

	float RegenAmount = ShieldRegenRate * DeltaTime;
	if (RegenAmount > 0.0f)
	{
		RestoreShields(RegenAmount, GetOwner());
	}
}

// ============================================================================
// Health State Management
// ============================================================================

void UNPCHealthComponent::UpdateHealthState()
{
	EHealthState NewState;
	float EffectivePercent = GetEffectiveHealthPercentage();

	if (CurrentHealth <= 0.0f && bCanDie)
	{
		NewState = EHealthState::Dead;
	}
	else if (EffectivePercent <= 0.25f)
	{
		NewState = EHealthState::Dying;
	}
	else if (EffectivePercent <= 0.5f)
	{
		NewState = EHealthState::Critical;
	}
	else if (EffectivePercent <= 0.75f)
	{
		NewState = EHealthState::Damaged;
	}
	else
	{
		NewState = EHealthState::Healthy;
	}

	if (NewState != CurrentHealthState)
	{
		EHealthState PrevState = CurrentHealthState;
		CurrentHealthState = NewState;
		OnHealthStateChanged.Broadcast(NewState);

		UE_LOG(LogNPCHealth, Log, TEXT("[%s] State: %d -> %d (effective=%.0f%%)"),
			*GetOwner()->GetName(), (int32)PrevState, (int32)NewState, EffectivePercent * 100.0f);
	}
}

// ============================================================================
// Event Broadcasting
// ============================================================================

void UNPCHealthComponent::BroadcastHealthChangeEvent(
	float PrevHealth, float PrevShields, float DamageAmount, float ShieldAbsorbed,
	AActor* Source, FName DamageType, EHealthState PrevState, bool bIsCritical)
{
	FHealthEventPayload Payload;
	Payload.Initialize(EOdysseyEventType::DamageReceived, GetOwner());
	Payload.PreviousHealth = PrevHealth;
	Payload.CurrentHealth = CurrentHealth;
	Payload.MaxHealth = MaxHealth;
	Payload.PreviousShields = PrevShields;
	Payload.CurrentShields = CurrentShields;
	Payload.MaxShields = MaxShields;
	Payload.DamageAmount = DamageAmount;
	Payload.ShieldDamageAbsorbed = ShieldAbsorbed;
	Payload.PreviousState = PrevState;
	Payload.CurrentState = CurrentHealthState;
	Payload.DamageSource = Source;
	Payload.DamageType = DamageType;
	Payload.bWasKillingBlow = (CurrentHealth <= 0.0f && PrevHealth > 0.0f);
	Payload.bWasCritical = bIsCritical;

	// Local delegate broadcast
	OnHealthChanged.Broadcast(Payload);

	// Global event bus broadcast
	if (bBroadcastToEventBus && EventBus)
	{
		TSharedPtr<FHealthEventPayload> SharedPayload = MakeShared<FHealthEventPayload>(Payload);
		EventBus->PublishEvent(StaticCastSharedPtr<FOdysseyEventPayload>(SharedPayload));
	}
}

void UNPCHealthComponent::HandleDeath(AActor* KillerActor)
{
	if (CurrentHealthState != EHealthState::Dead)
	{
		return;
	}

	// Clear DOT effects on death
	ActiveDOTEffects.Empty();

	// Broadcast death event
	OnActorDied.Broadcast(GetOwner());

	UE_LOG(LogNPCHealth, Warning, TEXT("[%s] DIED. Killer: %s"),
		*GetOwner()->GetName(),
		KillerActor ? *KillerActor->GetName() : TEXT("Unknown"));
}

// ============================================================================
// Visual Health Bar Helpers
// ============================================================================

FLinearColor UNPCHealthComponent::GetHealthBarColor() const
{
	float Percent = GetHealthPercentage();

	// Green -> Yellow -> Red gradient
	if (Percent > 0.5f)
	{
		// Green to Yellow (1.0 -> 0.5)
		float T = (Percent - 0.5f) * 2.0f;
		return FLinearColor::LerpUsingHSV(FLinearColor::Yellow, FLinearColor::Green, T);
	}
	else
	{
		// Yellow to Red (0.5 -> 0.0)
		float T = Percent * 2.0f;
		return FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Yellow, T);
	}
}

FLinearColor UNPCHealthComponent::GetShieldBarColor() const
{
	// Cyan/blue shield bar with intensity based on shield percentage
	float Percent = GetShieldPercentage();
	FLinearColor ShieldColor(0.2f, 0.6f, 1.0f, 1.0f); // Light blue
	ShieldColor.A = FMath::Lerp(0.4f, 1.0f, Percent);
	return ShieldColor;
}

bool UNPCHealthComponent::ShouldShowHealthBar() const
{
	if (IsDead())
	{
		return false;
	}

	if (!bOnlyShowHealthBarWhenDamaged)
	{
		return true;
	}

	// Show if not at full health, or if recently damaged
	return !IsAtFullHealth() || TimeSinceLastDamage < HealthBarVisibilityDuration;
}

// ============================================================================
// Event Bus Integration
// ============================================================================

void UNPCHealthComponent::InitializeEventBusSubscriptions()
{
	EventBus = UOdysseyEventBus::Get();
	if (!EventBus)
	{
		UE_LOG(LogNPCHealth, Warning, TEXT("[%s] Could not find OdysseyEventBus instance"),
			*GetOwner()->GetName());
		return;
	}

	// Subscribe to DamageReceived events targeted at this actor
	FOdysseyEventFilter Filter;
	Filter.AllowedEventTypes.Add(EOdysseyEventType::DamageReceived);
	Filter.RequiredSource = GetOwner();

	DamageSubscriptionHandle = EventBus->Subscribe(
		EOdysseyEventType::DamageReceived,
		[this](const FOdysseyEventPayload& Payload)
		{
			OnDamageEventReceived(Payload);
		},
		Filter,
		100 // High priority for health components
	);
}

void UNPCHealthComponent::CleanupEventBusSubscriptions()
{
	if (EventBus && DamageSubscriptionHandle.IsValid())
	{
		EventBus->Unsubscribe(DamageSubscriptionHandle);
		DamageSubscriptionHandle.Reset();
	}
}

void UNPCHealthComponent::OnDamageEventReceived(const FOdysseyEventPayload& Payload)
{
	// Handle incoming damage events from the global event bus
	// This allows other systems to deal damage via event publishing
	const FCombatEventPayload* CombatEvent = static_cast<const FCombatEventPayload*>(&Payload);
	if (CombatEvent && CombatEvent->Target == GetOwner())
	{
		TakeDamageEx(
			CombatEvent->DamageAmount,
			CombatEvent->Attacker.Get(),
			CombatEvent->DamageType,
			CombatEvent->bIsCritical
		);
	}
}
