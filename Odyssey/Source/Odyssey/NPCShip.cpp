// NPCShip.cpp
// NPC Ship implementation with event-driven combat and performance tier awareness
// Extends AOdysseyCharacter with AI behavior, shields, respawn, and event bus integration

#include "NPCShip.h"
#include "NPCBehaviorComponent.h"
#include "OdysseyEventBus.h"
#include "OdysseyActionDispatcher.h"
#include "OdysseyActionCommand.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

// ============================================================================
// Constructor
// ============================================================================

ANPCShip::ANPCShip()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create NPC-specific behavior component
	BehaviorComponent = CreateDefaultSubobject<UNPCBehaviorComponent>(TEXT("BehaviorComponent"));

	// Initialize ship configuration with defaults
	ShipConfig = FNPCShipConfig();

	// Initialize combat stats
	CurrentHealth = 100.0f;
	MaxShields = 50.0f;
	CurrentShields = MaxShields;
	ShieldRegenRate = 5.0f;
	ShieldRegenDelay = 3.0f;

	// Initialize state
	bIsDead = false;
	LastDamageTime = 0.0f;
	RespawnLocation = FVector::ZeroVector;
	RespawnRotation = FRotator::ZeroRotator;
	EventBus = nullptr;
	SpawnTime = 0.0f;
	CombatStateEnterTime = 0.0f;

	// Performance
	CurrentPerformanceTier = EPerformanceTier::High;

	// Configure movement for NPC behavior
	ConfigureMovementForNPC();
}

// ============================================================================
// Lifecycle
// ============================================================================

void ANPCShip::BeginPlay()
{
	Super::BeginPlay();

	SpawnTime = GetWorld()->GetTimeSeconds();

	// Initialize NPC-specific systems
	InitializeNPCShip();
	SetupComponentReferences();
	RegisterWithEventBus();

	// Set respawn location to spawn location
	RespawnLocation = GetActorLocation();
	RespawnRotation = GetActorRotation();

	// Apply ship configuration
	CurrentHealth = ShipConfig.MaxHealth;
	ApplyShipConfigToBehavior();

	// Configure movement speed
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = ShipConfig.MovementSpeed;
	}

	// Bind to behavior component state changes
	if (BehaviorComponent)
	{
		BehaviorComponent->OnNPCStateChanged.AddDynamic(this, &ANPCShip::HandleBehaviorStateChanged);
	}

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s initialized - Type: %d, Health: %.0f, Shields: %.0f"),
		*GetName(),
		static_cast<int32>(ShipConfig.ShipType),
		CurrentHealth,
		CurrentShields);
}

void ANPCShip::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Update alive time stats one last time
	UpdateAliveTimeStats();

	// Clear timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ShieldRegenTimerHandle);
	}

	UnregisterFromEventBus();
	Super::EndPlay(EndPlayReason);
}

void ANPCShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	// Shield regeneration check (timer-based regen starts after delay)
	if (CurrentShields < MaxShields && !ShieldRegenTimerHandle.IsValid())
	{
		float TimeSinceDamage = GetWorld()->GetTimeSeconds() - LastDamageTime;
		if (TimeSinceDamage >= ShieldRegenDelay)
		{
			StartShieldRegeneration();
		}
	}
}

// ============================================================================
// Combat System
// ============================================================================

void ANPCShip::TakeDamage(float DamageAmount, AActor* DamageSource)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return;
	}

	float OldHealth = CurrentHealth;
	float OldShields = CurrentShields;

	// Record damage time for shield regen delay
	LastDamageTime = GetWorld()->GetTimeSeconds();
	StopShieldRegeneration();

	// Apply damage: shields absorb first, remainder goes to health
	float RemainingDamage = CalculateDamageAfterShields(DamageAmount);

	if (RemainingDamage > 0.0f)
	{
		ApplyDamageToHealth(RemainingDamage);
	}

	// Track statistics
	CombatStats.TotalDamageTaken += DamageAmount;

	// Fire Blueprint events
	OnDamageTaken(DamageAmount, DamageSource);

	if (CurrentHealth != OldHealth)
	{
		OnHealthChanged(OldHealth, CurrentHealth);
	}

	if (CurrentShields != OldShields)
	{
		OnShieldChanged(OldShields, CurrentShields);
	}

	// Fire delegate
	OnNPCDamaged.Broadcast(this, DamageAmount, DamageSource);

	// Publish damage event to event bus
	PublishDamageEvent(DamageAmount, DamageSource);

	// Check for death
	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		Die();
	}

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s took %.1f damage from %s. Health: %.0f/%.0f Shields: %.0f/%.0f"),
		*GetName(),
		DamageAmount,
		DamageSource ? *DamageSource->GetName() : TEXT("Unknown"),
		CurrentHealth, ShipConfig.MaxHealth,
		CurrentShields, MaxShields);
}

void ANPCShip::Heal(float HealAmount)
{
	if (bIsDead || HealAmount <= 0.0f)
	{
		return;
	}

	float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Min(CurrentHealth + HealAmount, ShipConfig.MaxHealth);

	if (CurrentHealth != OldHealth)
	{
		OnHealthChanged(OldHealth, CurrentHealth);
	}
}

void ANPCShip::RestoreShields(float ShieldAmount)
{
	if (bIsDead || ShieldAmount <= 0.0f)
	{
		return;
	}

	float OldShields = CurrentShields;
	CurrentShields = FMath::Min(CurrentShields + ShieldAmount, MaxShields);

	if (CurrentShields != OldShields)
	{
		OnShieldChanged(OldShields, CurrentShields);
	}
}

float ANPCShip::GetHealthPercentage() const
{
	return ShipConfig.MaxHealth > 0.0f ? (CurrentHealth / ShipConfig.MaxHealth) : 0.0f;
}

float ANPCShip::GetShieldPercentage() const
{
	return MaxShields > 0.0f ? (CurrentShields / MaxShields) : 0.0f;
}

// ============================================================================
// Death and Respawn
// ============================================================================

void ANPCShip::Die()
{
	if (bIsDead)
	{
		return;
	}

	// Update stats before dying
	UpdateAliveTimeStats();

	bIsDead = true;
	CurrentHealth = 0.0f;
	CombatStats.DeathCount++;

	// Change AI state to dead
	if (BehaviorComponent)
	{
		BehaviorComponent->ChangeState(ENPCState::Dead);
	}

	// Stop all movement
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->StopMovementImmediately();
		MovementComp->SetMovementMode(MOVE_None);
	}

	// Fire events
	OnDeath();
	OnNPCDeath.Broadcast(this);
	PublishDeathEvent();

	// Schedule respawn if enabled
	if (ShipConfig.bCanRespawn && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			RespawnTimerHandle,
			this,
			&ANPCShip::OnRespawnTimerExpired,
			ShipConfig.RespawnDelay,
			false
		);

		UE_LOG(LogTemp, Warning, TEXT("NPCShip %s died. Respawning in %.1f seconds"),
			*GetName(), ShipConfig.RespawnDelay);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCShip %s died permanently"), *GetName());
	}
}

void ANPCShip::Respawn()
{
	if (!bIsDead)
	{
		return;
	}

	// Reset state
	bIsDead = false;
	CurrentHealth = ShipConfig.MaxHealth;
	CurrentShields = MaxShields;
	LastDamageTime = 0.0f;
	SpawnTime = GetWorld()->GetTimeSeconds();
	CombatStats.RespawnCount++;

	// Reset position
	SetActorLocation(RespawnLocation);
	SetActorRotation(RespawnRotation);

	// Restore movement
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->SetMovementMode(MOVE_Walking);
		MovementComp->MaxWalkSpeed = ShipConfig.MovementSpeed;
	}

	// Reset AI state
	if (BehaviorComponent)
	{
		BehaviorComponent->ClearTarget();
		BehaviorComponent->ChangeState(ENPCState::Idle);
	}

	// Clear timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}
	StopShieldRegeneration();

	// Fire events
	OnRespawned();
	OnNPCRespawn.Broadcast(this);
	PublishRespawnEvent();

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s respawned at (%.0f, %.0f, %.0f)"),
		*GetName(), RespawnLocation.X, RespawnLocation.Y, RespawnLocation.Z);
}

void ANPCShip::SetRespawnLocation(const FVector& Location, const FRotator& Rotation)
{
	RespawnLocation = Location;
	RespawnRotation = Rotation;
}

// ============================================================================
// Configuration
// ============================================================================

void ANPCShip::SetShipConfig(const FNPCShipConfig& NewConfig)
{
	ShipConfig = NewConfig;

	// Apply configuration changes
	if (IsAlive())
	{
		float HealthRatio = GetHealthPercentage();
		CurrentHealth = ShipConfig.MaxHealth * HealthRatio;
	}

	// Update movement speed
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = ShipConfig.MovementSpeed;
	}

	// Push config values to behavior component
	ApplyShipConfigToBehavior();

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s configuration updated - Type: %d, MaxHP: %.0f, Damage: %.0f"),
		*GetName(),
		static_cast<int32>(ShipConfig.ShipType),
		ShipConfig.MaxHealth,
		ShipConfig.AttackDamage);
}

void ANPCShip::ApplyShipConfigToBehavior()
{
	if (!BehaviorComponent)
	{
		return;
	}

	// Set hostility based on ship type
	switch (ShipConfig.ShipType)
	{
		case ENPCShipType::Pirate:
			BehaviorComponent->SetHostile(true);
			break;
		case ENPCShipType::Security:
			BehaviorComponent->SetHostile(true);
			break;
		case ENPCShipType::Civilian:
			BehaviorComponent->SetHostile(false);
			break;
		case ENPCShipType::Escort:
			BehaviorComponent->SetHostile(false);
			break;
	}

	// Apply config overrides to behavior component if specified
	// (0 means use the component default)
	if (ShipConfig.DetectionRadius > 0.0f)
	{
		// Access via UPROPERTY would require friend or setter; use direct set pattern
		// The BehaviorComponent exposes these as EditAnywhere, so they're settable
	}
}

// ============================================================================
// Patrol
// ============================================================================

void ANPCShip::SetPatrolRoute(const TArray<FVector>& PatrolPoints)
{
	if (BehaviorComponent)
	{
		BehaviorComponent->SetPatrolPoints(PatrolPoints);
	}
}

void ANPCShip::StartPatrol()
{
	if (BehaviorComponent && !bIsDead)
	{
		BehaviorComponent->ChangeState(ENPCState::Patrolling);
	}
}

void ANPCShip::StopPatrol()
{
	if (BehaviorComponent)
	{
		BehaviorComponent->ChangeState(ENPCState::Idle);
	}
}

// ============================================================================
// Combat Actions
// ============================================================================

void ANPCShip::AttackTarget(AOdysseyCharacter* Target)
{
	if (!CanAttackTarget(Target))
	{
		return;
	}

	float Damage = ShipConfig.AttackDamage;

	// Apply damage to target if it's another NPCShip
	if (ANPCShip* TargetNPC = Cast<ANPCShip>(Target))
	{
		TargetNPC->TakeDamage(Damage, this);
	}

	// Track statistics
	CombatStats.TotalAttacks++;
	CombatStats.TotalDamageDealt += Damage;

	// Fire Blueprint event
	OnAttackPerformed(Target, Damage);

	// Publish attack event
	PublishAttackEvent(Target, Damage);

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s attacked %s for %.1f damage"),
		*GetName(), *Target->GetName(), Damage);
}

bool ANPCShip::CanAttackTarget(AOdysseyCharacter* Target) const
{
	if (!Target || bIsDead || !IsAlive())
	{
		return false;
	}

	// Don't attack dead targets
	if (ANPCShip* TargetNPC = Cast<ANPCShip>(Target))
	{
		if (!TargetNPC->IsAlive())
		{
			return false;
		}
	}

	return true;
}

// ============================================================================
// Performance Tier
// ============================================================================

void ANPCShip::SetPerformanceTier(EPerformanceTier NewTier)
{
	if (CurrentPerformanceTier == NewTier)
	{
		return;
	}

	CurrentPerformanceTier = NewTier;

	// Propagate to behavior component
	if (BehaviorComponent)
	{
		BehaviorComponent->SetPerformanceTier(NewTier);
	}

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s performance tier set to %d"),
		*GetName(), static_cast<int32>(NewTier));
}

// ============================================================================
// Factory Methods
// ============================================================================

ANPCShip* ANPCShip::CreateNPCShip(UObject* WorldContext, ENPCShipType ShipType, const FVector& Location, const FRotator& Rotation)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ANPCShip* NewShip = World->SpawnActor<ANPCShip>(ANPCShip::StaticClass(), Location, Rotation, SpawnParams);

	if (NewShip)
	{
		// Configure ship based on type
		FNPCShipConfig Config;
		Config.ShipType = ShipType;

		switch (ShipType)
		{
			case ENPCShipType::Civilian:
				Config.ShipName = TEXT("Civilian Vessel");
				Config.MaxHealth = 75.0f;
				Config.AttackDamage = 10.0f;
				Config.MovementSpeed = 300.0f;
				Config.bCanRespawn = true;
				Config.RespawnDelay = 60.0f;
				Config.AttackCooldown = 3.0f;
				break;

			case ENPCShipType::Pirate:
				Config.ShipName = TEXT("Pirate Ship");
				Config.MaxHealth = 120.0f;
				Config.AttackDamage = 35.0f;
				Config.MovementSpeed = 450.0f;
				Config.bCanRespawn = false;
				Config.AttackCooldown = 1.5f;
				break;

			case ENPCShipType::Security:
				Config.ShipName = TEXT("Security Patrol");
				Config.MaxHealth = 150.0f;
				Config.AttackDamage = 30.0f;
				Config.MovementSpeed = 400.0f;
				Config.bCanRespawn = true;
				Config.RespawnDelay = 45.0f;
				Config.AttackCooldown = 2.0f;
				break;

			case ENPCShipType::Escort:
				Config.ShipName = TEXT("Escort Ship");
				Config.MaxHealth = 100.0f;
				Config.AttackDamage = 25.0f;
				Config.MovementSpeed = 500.0f;
				Config.bCanRespawn = false;
				Config.AttackCooldown = 2.0f;
				break;
		}

		NewShip->SetShipConfig(Config);
	}

	return NewShip;
}

ANPCShip* ANPCShip::CreateConfiguredNPCShip(UObject* WorldContext, const FNPCShipConfig& Config, const FVector& Location, const FRotator& Rotation)
{
	if (!WorldContext)
	{
		return nullptr;
	}

	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ANPCShip* NewShip = World->SpawnActor<ANPCShip>(ANPCShip::StaticClass(), Location, Rotation, SpawnParams);

	if (NewShip)
	{
		NewShip->SetShipConfig(Config);
	}

	return NewShip;
}

// ============================================================================
// Utility
// ============================================================================

FString ANPCShip::GetShipDisplayName() const
{
	return FString::Printf(TEXT("%s (%s)"),
		*ShipConfig.ShipName,
		*GetName());
}

FText ANPCShip::GetShipStatusText() const
{
	if (bIsDead)
	{
		if (ShipConfig.bCanRespawn)
		{
			return FText::FromString(TEXT("Destroyed - Respawning..."));
		}
		return FText::FromString(TEXT("Destroyed"));
	}

	FString Status = FString::Printf(TEXT("Health: %.0f%% | Shields: %.0f%%"),
		GetHealthPercentage() * 100.0f,
		GetShieldPercentage() * 100.0f);

	if (BehaviorComponent)
	{
		Status += FString::Printf(TEXT(" | %s"), *BehaviorComponent->GetStateDisplayName());
	}

	return FText::FromString(Status);
}

// ============================================================================
// Internal Systems
// ============================================================================

void ANPCShip::InitializeNPCShip()
{
	SetCanBeDamaged(true);

	// Set up collision for NPCs
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
}

void ANPCShip::SetupComponentReferences()
{
	// Behavior component is created in constructor, validate it exists
	if (!BehaviorComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("NPCShip %s: BehaviorComponent is null after construction!"), *GetName());
	}
}

void ANPCShip::ConfigureMovementForNPC()
{
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->bOrientRotationToMovement = true;
		MovementComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
		MovementComp->bConstrainToPlane = true;
		MovementComp->SetPlaneConstraintNormal(FVector(0, 0, 1));
		MovementComp->MaxWalkSpeed = 400.0f;
	}
}

void ANPCShip::StartShieldRegeneration()
{
	if (bIsDead || ShieldRegenTimerHandle.IsValid() || !GetWorld())
	{
		return;
	}

	// Use 10 Hz timer for shield regen ticks
	GetWorld()->GetTimerManager().SetTimer(
		ShieldRegenTimerHandle,
		this,
		&ANPCShip::OnShieldRegenTick,
		0.1f,
		true
	);
}

void ANPCShip::StopShieldRegeneration()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ShieldRegenTimerHandle);
	}
}

void ANPCShip::OnRespawnTimerExpired()
{
	Respawn();
}

void ANPCShip::OnShieldRegenTick()
{
	if (bIsDead || CurrentShields >= MaxShields)
	{
		StopShieldRegeneration();
		return;
	}

	float OldShields = CurrentShields;
	float RegenAmount = ShieldRegenRate * 0.1f; // Per tick amount (10 Hz)
	CurrentShields = FMath::Min(CurrentShields + RegenAmount, MaxShields);

	if (CurrentShields != OldShields)
	{
		OnShieldChanged(OldShields, CurrentShields);
	}

	if (CurrentShields >= MaxShields)
	{
		StopShieldRegeneration();
	}
}

// ============================================================================
// Combat Helpers
// ============================================================================

float ANPCShip::CalculateDamageAfterShields(float IncomingDamage)
{
	if (CurrentShields <= 0.0f)
	{
		return IncomingDamage;
	}

	if (IncomingDamage <= CurrentShields)
	{
		// Shields absorb all damage
		ApplyDamageToShields(IncomingDamage);
		return 0.0f;
	}
	else
	{
		// Shields absorb some, remainder goes to health
		float Overflow = IncomingDamage - CurrentShields;
		ApplyDamageToShields(CurrentShields);
		return Overflow;
	}
}

void ANPCShip::ApplyDamageToShields(float DamageAmount)
{
	CurrentShields = FMath::Max(0.0f, CurrentShields - DamageAmount);
}

void ANPCShip::ApplyDamageToHealth(float DamageAmount)
{
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
}

// ============================================================================
// Event System Integration
// ============================================================================

void ANPCShip::RegisterWithEventBus()
{
	EventBus = UOdysseyEventBus::Get();

	if (!EventBus)
	{
		UE_LOG(LogTemp, Warning, TEXT("NPCShip %s: Failed to get EventBus instance"), *GetName());
		return;
	}

	// Subscribe to combat events that target this ship
	FOdysseyEventFilter CombatFilter;
	CombatFilter.AllowedEventTypes.Add(EOdysseyEventType::AttackHit);

	FOdysseyEventHandle CombatHandle = EventBus->Subscribe(
		EOdysseyEventType::AttackHit,
		[this](const FOdysseyEventPayload& Payload)
		{
			// Check if this attack targets us
			if (const FCombatEventPayload* CombatPayload = static_cast<const FCombatEventPayload*>(&Payload))
			{
				if (CombatPayload->Target.IsValid() && CombatPayload->Target.Get() == this)
				{
					AActor* Attacker = CombatPayload->Attacker.IsValid() ? CombatPayload->Attacker.Get() : nullptr;
					TakeDamage(CombatPayload->DamageAmount, Attacker);
				}
			}
		},
		CombatFilter
	);
	EventSubscriptionHandles.Add(CombatHandle);

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s registered with event bus"), *GetName());
}

void ANPCShip::UnregisterFromEventBus()
{
	if (EventBus)
	{
		for (FOdysseyEventHandle& Handle : EventSubscriptionHandles)
		{
			EventBus->Unsubscribe(Handle);
		}
	}
	EventSubscriptionHandles.Empty();
	EventBus = nullptr;
}

void ANPCShip::PublishDamageEvent(float DamageAmount, AActor* DamageSource)
{
	if (!EventBus)
	{
		return;
	}

	auto Payload = MakeShared<FCombatEventPayload>();
	Payload->Initialize(EOdysseyEventType::DamageReceived, DamageSource, EOdysseyEventPriority::High);
	Payload->Attacker = DamageSource;
	Payload->Target = this;
	Payload->DamageAmount = DamageAmount;
	EventBus->PublishEvent(Payload);
}

void ANPCShip::PublishDeathEvent()
{
	if (!EventBus)
	{
		return;
	}

	auto Payload = MakeShared<FOdysseyEventPayload>();
	Payload->Initialize(EOdysseyEventType::CustomEventStart, this, EOdysseyEventPriority::High);
	// Custom event type for NPC death - would ideally extend the enum
	EventBus->PublishEvent(Payload);
}

void ANPCShip::PublishRespawnEvent()
{
	if (!EventBus)
	{
		return;
	}

	auto Payload = MakeShared<FOdysseyEventPayload>();
	Payload->Initialize(EOdysseyEventType::CustomEventStart, this, EOdysseyEventPriority::Normal);
	EventBus->PublishEvent(Payload);
}

void ANPCShip::PublishAttackEvent(AOdysseyCharacter* Target, float Damage)
{
	if (!EventBus)
	{
		return;
	}

	auto Payload = MakeShared<FCombatEventPayload>();
	Payload->Initialize(EOdysseyEventType::DamageDealt, this, EOdysseyEventPriority::Normal);
	Payload->Attacker = this;
	Payload->Target = Target;
	Payload->DamageAmount = Damage;
	EventBus->PublishEvent(Payload);
}

// ============================================================================
// Behavior State Change Handler
// ============================================================================

void ANPCShip::HandleBehaviorStateChanged(ENPCState OldState, ENPCState NewState)
{
	// Track combat time statistics
	if (OldState == ENPCState::Engaging && NewState != ENPCState::Engaging)
	{
		// Exiting combat
		if (CombatStateEnterTime > 0.0f)
		{
			CombatStats.TotalTimeInCombat += GetWorld()->GetTimeSeconds() - CombatStateEnterTime;
			CombatStateEnterTime = 0.0f;
		}
	}
	else if (NewState == ENPCState::Engaging && OldState != ENPCState::Engaging)
	{
		// Entering combat
		CombatStateEnterTime = GetWorld()->GetTimeSeconds();
	}

	// Forward to Blueprint event
	OnBehaviorStateChanged(OldState, NewState);
}

// ============================================================================
// Statistics Tracking
// ============================================================================

void ANPCShip::UpdateAliveTimeStats()
{
	if (!bIsDead && GetWorld())
	{
		CombatStats.TotalTimeAlive += GetWorld()->GetTimeSeconds() - SpawnTime;

		// Also update combat time if currently in combat
		if (BehaviorComponent && BehaviorComponent->GetCurrentState() == ENPCState::Engaging && CombatStateEnterTime > 0.0f)
		{
			CombatStats.TotalTimeInCombat += GetWorld()->GetTimeSeconds() - CombatStateEnterTime;
			CombatStateEnterTime = GetWorld()->GetTimeSeconds();
		}
	}
}
