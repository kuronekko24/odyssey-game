// NPCHealthComponent.cpp
// Implementation of health management component for NPC ships

#include "NPCHealthComponent.h"
#include "OdysseyEventBus.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

UNPCHealthComponent::UNPCHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10 FPS for health regen is sufficient

	// Default configuration
	MaxHealth = 100.0f;
	StartingHealthPercentage = 1.0f;
	bHealthRegenEnabled = false;
	HealthRegenRate = 5.0f;
	HealthRegenDelay = 3.0f;
	bOnlyRegenOutOfCombat = true;
	OutOfCombatTime = 5.0f;
	bBroadcastHealthEvents = true;
	bCanDie = true;

	// Runtime state
	CurrentHealth = MaxHealth;
	CurrentHealthState = EHealthState::Healthy;
	TimeSinceLastDamage = 0.0f;
	bIsRegenerating = false;
	EventBus = nullptr;
}

void UNPCHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize health
	CurrentHealth = MaxHealth * StartingHealthPercentage;
	UpdateHealthState();

	// Initialize event bus integration
	InitializeEventBusSubscriptions();

	UE_LOG(LogTemp, Log, TEXT("NPCHealthComponent initialized for %s with %f/%f health"), 
		*GetOwner()->GetName(), CurrentHealth, MaxHealth);
}

void UNPCHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupEventBusSubscriptions();
	Super::EndPlay(EndPlayReason);
}

void UNPCHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update combat timing
	TimeSinceLastDamage += DeltaTime;

	// Process health regeneration
	if (bHealthRegenEnabled && !IsDead())
	{
		ProcessHealthRegeneration(DeltaTime);
	}
}

float UNPCHealthComponent::TakeDamage(float DamageAmount, AActor* DamageSource, FName DamageType)
{
	// Can't damage dead actors
	if (IsDead())
	{
		return 0.0f;
	}

	// Calculate actual damage after resistances
	float ActualDamage = CalculateActualDamage(DamageAmount, DamageType);

	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	// Store previous values for event
	float PreviousHealth = CurrentHealth;
	EHealthState PreviousState = CurrentHealthState;

	// Apply damage
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);

	// If actor can't die, ensure health stays above 0
	if (!bCanDie && CurrentHealth <= 0.0f)
	{
		CurrentHealth = 1.0f;
		ActualDamage = PreviousHealth - CurrentHealth;
	}

	// Update state and reset combat timer
	UpdateHealthState();
	TimeSinceLastDamage = 0.0f;
	bIsRegenerating = false;

	// Broadcast events
	BroadcastHealthChangeEvent(PreviousHealth, ActualDamage, DamageSource, DamageType, PreviousState);

	// Handle death if health reached 0
	if (CurrentHealth <= 0.0f && bCanDie)
	{
		HandleDeath(DamageSource);
	}

	UE_LOG(LogTemp, Log, TEXT("%s took %f damage (%f after resistances). Health: %f/%f"), 
		*GetOwner()->GetName(), DamageAmount, ActualDamage, CurrentHealth, MaxHealth);

	return ActualDamage;
}

float UNPCHealthComponent::Heal(float HealAmount, AActor* HealSource)
{
	if (IsDead() || HealAmount <= 0.0f)
	{
		return 0.0f;
	}

	float PreviousHealth = CurrentHealth;
	EHealthState PreviousState = CurrentHealthState;

	// Apply healing
	float ActualHealing = FMath::Min(HealAmount, MaxHealth - CurrentHealth);
	CurrentHealth += ActualHealing;

	// Update state
	UpdateHealthState();

	// Broadcast heal event (negative damage)
	if (ActualHealing > 0.0f)
	{
		BroadcastHealthChangeEvent(PreviousHealth, -ActualHealing, HealSource, TEXT("Healing"), PreviousState);
	}

	UE_LOG(LogTemp, Log, TEXT("%s healed for %f. Health: %f/%f"), 
		*GetOwner()->GetName(), ActualHealing, CurrentHealth, MaxHealth);

	return ActualHealing;
}

void UNPCHealthComponent::SetHealth(float NewHealth, bool bBroadcastEvent)
{
	float PreviousHealth = CurrentHealth;
	EHealthState PreviousState = CurrentHealthState;

	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	
	// If actor can't die, ensure health stays above 0
	if (!bCanDie && CurrentHealth <= 0.0f)
	{
		CurrentHealth = 1.0f;
	}

	UpdateHealthState();

	if (bBroadcastEvent)
	{
		float HealthDelta = CurrentHealth - PreviousHealth;
		BroadcastHealthChangeEvent(PreviousHealth, -HealthDelta, nullptr, TEXT("SetHealth"), PreviousState);
	}

	// Handle death if applicable
	if (CurrentHealth <= 0.0f && bCanDie)
	{
		HandleDeath(nullptr);
	}
}

void UNPCHealthComponent::SetMaxHealth(float NewMaxHealth, bool bMaintainHealthPercentage)
{
	if (NewMaxHealth <= 0.0f)
	{
		return;
	}

	float OldMaxHealth = MaxHealth;
	MaxHealth = NewMaxHealth;

	if (bMaintainHealthPercentage)
	{
		// Keep the same health percentage
		float HealthPercentage = OldMaxHealth > 0.0f ? CurrentHealth / OldMaxHealth : 1.0f;
		CurrentHealth = MaxHealth * HealthPercentage;
	}
	else
	{
		// Clamp current health to new maximum
		CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
	}

	UpdateHealthState();

	UE_LOG(LogTemp, Log, TEXT("%s max health changed to %f. Current health: %f"), 
		*GetOwner()->GetName(), MaxHealth, CurrentHealth);
}

void UNPCHealthComponent::Kill(AActor* KillerActor)
{
	if (!IsDead())
	{
		float PreviousHealth = CurrentHealth;
		EHealthState PreviousState = CurrentHealthState;

		CurrentHealth = 0.0f;
		UpdateHealthState();

		BroadcastHealthChangeEvent(PreviousHealth, PreviousHealth, KillerActor, TEXT("Kill"), PreviousState);
		HandleDeath(KillerActor);
	}
}

float UNPCHealthComponent::GetHealthPercentage() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

void UNPCHealthComponent::SetHealthRegenEnabled(bool bEnabled)
{
	bHealthRegenEnabled = bEnabled;
	if (!bEnabled)
	{
		bIsRegenerating = false;
	}
}

void UNPCHealthComponent::SetHealthRegenRate(float RegenPerSecond)
{
	HealthRegenRate = FMath::Max(0.0f, RegenPerSecond);
}

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

void UNPCHealthComponent::UpdateHealthState()
{
	EHealthState NewState;
	float HealthPercent = GetHealthPercentage();

	if (HealthPercent <= 0.0f)
	{
		NewState = EHealthState::Dead;
	}
	else if (HealthPercent <= 0.25f)
	{
		NewState = EHealthState::Dying;
	}
	else if (HealthPercent <= 0.5f)
	{
		NewState = EHealthState::Critical;
	}
	else if (HealthPercent <= 0.75f)
	{
		NewState = EHealthState::Damaged;
	}
	else
	{
		NewState = EHealthState::Healthy;
	}

	if (NewState != CurrentHealthState)
	{
		EHealthState PreviousState = CurrentHealthState;
		CurrentHealthState = NewState;

		// Broadcast state change
		OnHealthStateChanged.Broadcast(NewState);

		UE_LOG(LogTemp, Log, TEXT("%s health state changed from %d to %d"), 
			*GetOwner()->GetName(), (int32)PreviousState, (int32)NewState);
	}
}

float UNPCHealthComponent::CalculateActualDamage(float BaseDamage, FName DamageType) const
{
	float ActualDamage = BaseDamage;

	// Apply damage resistance
	float Resistance = GetDamageResistance(DamageType);
	if (Resistance > 0.0f)
	{
		ActualDamage *= (1.0f - Resistance);
	}

	return FMath::Max(0.0f, ActualDamage);
}

void UNPCHealthComponent::BroadcastHealthChangeEvent(float PreviousHealth, float DamageAmount, AActor* Source, FName DamageType, EHealthState PreviousState)
{
	// Create and populate health event payload
	FHealthEventPayload HealthEvent;
	HealthEvent.Initialize(EOdysseyEventType::DamageReceived, GetOwner());
	HealthEvent.PreviousHealth = PreviousHealth;
	HealthEvent.CurrentHealth = CurrentHealth;
	HealthEvent.MaxHealth = MaxHealth;
	HealthEvent.DamageAmount = DamageAmount;
	HealthEvent.PreviousState = PreviousState;
	HealthEvent.CurrentState = CurrentHealthState;
	HealthEvent.DamageSource = Source;
	HealthEvent.DamageType = DamageType;
	HealthEvent.bWasKillingBlow = (CurrentHealth <= 0.0f && PreviousHealth > 0.0f);

	// Broadcast to Blueprint delegates
	OnHealthChanged.Broadcast(HealthEvent);

	// Broadcast to global event bus if enabled
	if (bBroadcastHealthEvents && EventBus)
	{
		TSharedPtr<FHealthEventPayload> SharedPayload = MakeShared<FHealthEventPayload>(HealthEvent);
		EventBus->PublishEvent(StaticCastSharedPtr<FOdysseyEventPayload>(SharedPayload));
	}
}

void UNPCHealthComponent::HandleDeath(AActor* KillerActor)
{
	if (CurrentHealthState != EHealthState::Dead)
	{
		return; // Already handled
	}

	// Broadcast death event
	OnActorDied.Broadcast(GetOwner());

	UE_LOG(LogTemp, Warning, TEXT("%s has died"), *GetOwner()->GetName());

	// Here you might want to trigger death animations, drop loot, etc.
	// For now, just log the event
}

void UNPCHealthComponent::ProcessHealthRegeneration(float DeltaTime)
{
	// Check if we should start/continue regenerating
	bool bShouldRegen = (TimeSinceLastDamage >= HealthRegenDelay) && 
						(!bOnlyRegenOutOfCombat || !IsInCombat()) &&
						!IsAtFullHealth();

	if (bShouldRegen)
	{
		if (!bIsRegenerating)
		{
			bIsRegenerating = true;
			UE_LOG(LogTemp, Log, TEXT("%s started health regeneration"), *GetOwner()->GetName());
		}

		// Apply regeneration
		float RegenAmount = HealthRegenRate * DeltaTime;
		if (RegenAmount > 0.0f)
		{
			Heal(RegenAmount, GetOwner());
		}
	}
	else if (bIsRegenerating)
	{
		bIsRegenerating = false;
		UE_LOG(LogTemp, Log, TEXT("%s stopped health regeneration"), *GetOwner()->GetName());
	}
}

bool UNPCHealthComponent::IsInCombat() const
{
	return TimeSinceLastDamage < OutOfCombatTime;
}

void UNPCHealthComponent::InitializeEventBusSubscriptions()
{
	EventBus = UOdysseyEventBus::Get();
	if (!EventBus)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCHealthComponent could not find OdysseyEventBus instance"));
		return;
	}

	// Subscribe to damage events targeted at this actor
	FOdysseyEventFilter Filter;
	Filter.AllowedEventTypes.Add(EOdysseyEventType::DamageReceived);
	Filter.RequiredSource = GetOwner();

	DamageSubscriptionHandle = EventBus->Subscribe(
		EOdysseyEventType::DamageReceived,
		[this](const FOdysseyEventPayload& Payload)
		{
			OnDamageReceived(Payload);
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

void UNPCHealthComponent::OnDamageReceived(const FOdysseyEventPayload& Payload)
{
	// This method can be used for additional processing of damage events
	// from the event bus if needed. Currently, direct damage application
	// through TakeDamage() is the primary method, but this provides
	// integration point for event-driven damage from other systems.

	if (const FCombatEventPayload* CombatEvent = static_cast<const FCombatEventPayload*>(&Payload))
	{
		if (CombatEvent->Target == GetOwner())
		{
			// Apply damage from combat event
			TakeDamage(CombatEvent->DamageAmount, CombatEvent->Attacker.Get(), CombatEvent->DamageType);
		}
	}
}
