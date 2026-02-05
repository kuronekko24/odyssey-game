#include "NPCShip.h"
#include "NPCBehaviorComponent.h"
#include "OdysseyEventBus.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

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

	// Configure for NPC behavior
	ConfigureMovementForNPC();

	UE_LOG(LogTemp, Warning, TEXT("ANPCShip constructor called"));
}

void ANPCShip::BeginPlay()
{
	Super::BeginPlay();

	// Initialize NPC-specific systems
	InitializeNPCShip();
	SetupComponentReferences();
	RegisterWithEventBus();

	// Set respawn location to spawn location
	RespawnLocation = GetActorLocation();
	RespawnRotation = GetActorRotation();

	// Apply ship configuration
	CurrentHealth = ShipConfig.MaxHealth;
	
	// Configure movement speed
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = ShipConfig.MovementSpeed;
	}

	UE_LOG(LogTemp, Warning, TEXT("NPCShip %s initialized - Type: %d, Health: %f"), 
		*GetName(), 
		(int32)ShipConfig.ShipType, 
		CurrentHealth);
}

void ANPCShip::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromEventBus();
	Super::EndPlay(EndPlayReason);
}

void ANPCShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle shield regeneration timing
	if (CurrentShields < MaxShields && GetWorld()->GetTimeSeconds() - LastDamageTime >= ShieldRegenDelay)
	{
		if (!ShieldRegenTimerHandle.IsValid())
		{
			StartShieldRegeneration();
		}
	}
}

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

	// Apply damage after shields
	float RemainingDamage = CalculateDamageAfterShields(DamageAmount);
	
	if (RemainingDamage > 0.0f)
	{
		ApplyDamageToHealth(RemainingDamage);
	}

	// Fire events
	OnDamageTaken(DamageAmount, DamageSource);
	
	if (CurrentHealth != OldHealth)
	{
		OnHealthChanged(OldHealth, CurrentHealth);
	}
	
	if (CurrentShields != OldShields)
	{
		OnShieldChanged(OldShields, CurrentShields);
	}

	// Check for death
	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		Die();
	}

	// Broadcast damage event
	TMap<FString, FString> EventData;
	EventData.Add(TEXT("DamageAmount"), FString::Printf(TEXT("%.2f"), DamageAmount));
	EventData.Add(TEXT("SourceActor"), DamageSource ? DamageSource->GetName() : TEXT("Unknown"));
	EventData.Add(TEXT("CurrentHealth"), FString::Printf(TEXT("%.2f"), CurrentHealth));
	BroadcastNPCEvent(TEXT("DamageTaken"), EventData);

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s took %.2f damage from %s. Health: %.2f/%.2f"), 
		*GetName(), 
		DamageAmount, 
		DamageSource ? *DamageSource->GetName() : TEXT("Unknown"), 
		CurrentHealth, 
		ShipConfig.MaxHealth);
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

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s healed for %.2f. Health: %.2f/%.2f"), 
		*GetName(), HealAmount, CurrentHealth, ShipConfig.MaxHealth);
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

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s shields restored by %.2f. Shields: %.2f/%.2f"), 
		*GetName(), ShieldAmount, CurrentShields, MaxShields);
}

float ANPCShip::GetHealthPercentage() const
{
	return ShipConfig.MaxHealth > 0.0f ? (CurrentHealth / ShipConfig.MaxHealth) : 0.0f;
}

float ANPCShip::GetShieldPercentage() const
{
	return MaxShields > 0.0f ? (CurrentShields / MaxShields) : 0.0f;
}

void ANPCShip::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	CurrentHealth = 0.0f;

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

	// Fire death event
	OnDeath();

	// Broadcast death event
	TMap<FString, FString> EventData;
	EventData.Add(TEXT("DeathTime"), FString::Printf(TEXT("%.2f"), GetWorld()->GetTimeSeconds()));
	EventData.Add(TEXT("ShipType"), FString::Printf(TEXT("%d"), (int32)ShipConfig.ShipType));
	BroadcastNPCEvent(TEXT("Death"), EventData);

	// Schedule respawn if enabled
	if (ShipConfig.bCanRespawn)
	{
		GetWorld()->GetTimerManager().SetTimer(
			RespawnTimerHandle,
			this,
			&ANPCShip::OnRespawnTimerExpired,
			ShipConfig.RespawnDelay,
			false
		);

		UE_LOG(LogTemp, Warning, TEXT("NPCShip %s died. Respawning in %.2f seconds"), 
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

	// Reset position
	SetActorLocation(RespawnLocation);
	SetActorRotation(RespawnRotation);

	// Restore movement
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->SetMovementMode(MOVE_Walking);
	}

	// Reset AI state
	if (BehaviorComponent)
	{
		BehaviorComponent->ClearTarget();
		BehaviorComponent->ChangeState(ENPCState::Idle);
	}

	// Clear timers
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle);
	StopShieldRegeneration();

	// Fire respawn event
	OnRespawned();

	// Broadcast respawn event
	TMap<FString, FString> EventData;
	EventData.Add(TEXT("RespawnTime"), FString::Printf(TEXT("%.2f"), GetWorld()->GetTimeSeconds()));
	BroadcastNPCEvent(TEXT("Respawn"), EventData);

	UE_LOG(LogTemp, Warning, TEXT("NPCShip %s respawned"), *GetName());
}

void ANPCShip::SetRespawnLocation(const FVector& Location, const FRotator& Rotation)
{
	RespawnLocation = Location;
	RespawnRotation = Rotation;
}

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

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s configuration updated"), *GetName());
}

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

	// Fire attack event
	OnAttackPerformed(Target, Damage);

	// Broadcast attack event
	TMap<FString, FString> EventData;
	EventData.Add(TEXT("Target"), Target->GetName());
	EventData.Add(TEXT("Damage"), FString::Printf(TEXT("%.2f"), Damage));
	BroadcastNPCEvent(TEXT("Attack"), EventData);

	UE_LOG(LogTemp, Log, TEXT("NPCShip %s attacked %s for %.2f damage"), 
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

ANPCShip* ANPCShip::CreateNPCShip(UWorld* World, ENPCShipType ShipType, const FVector& Location, const FRotator& Rotation)
{
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
				break;
				
			case ENPCShipType::Pirate:
				Config.ShipName = TEXT("Pirate Ship");
				Config.MaxHealth = 120.0f;
				Config.AttackDamage = 35.0f;
				Config.MovementSpeed = 450.0f;
				Config.bCanRespawn = false;
				break;
				
			case ENPCShipType::Security:
				Config.ShipName = TEXT("Security Patrol");
				Config.MaxHealth = 150.0f;
				Config.AttackDamage = 30.0f;
				Config.MovementSpeed = 400.0f;
				Config.bCanRespawn = true;
				break;
				
			case ENPCShipType::Escort:
				Config.ShipName = TEXT("Escort Ship");
				Config.MaxHealth = 100.0f;
				Config.AttackDamage = 25.0f;
				Config.MovementSpeed = 500.0f;
				Config.bCanRespawn = false;
				break;
		}

		NewShip->SetShipConfig(Config);
	}

	return NewShip;
}

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
		return FText::FromString(TEXT("Destroyed"));
	}

	FString Status = FString::Printf(TEXT("Health: %.0f%% | Shields: %.0f%%"), 
		GetHealthPercentage() * 100.0f,
		GetShieldPercentage() * 100.0f);

	if (BehaviorComponent)
	{
		switch (BehaviorComponent->GetCurrentState())
		{
			case ENPCState::Idle:
				Status += TEXT(" | Idle");
				break;
			case ENPCState::Patrolling:
				Status += TEXT(" | Patrolling");
				break;
			case ENPCState::Engaging:
				Status += TEXT(" | In Combat");
				break;
			case ENPCState::Dead:
				Status += TEXT(" | Destroyed");
				break;
		}
	}

	return FText::FromString(Status);
}

void ANPCShip::InitializeNPCShip()
{
	// Any additional NPC-specific initialization
	SetCanBeDamaged(true);
	
	// Set up collision for NPCs
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
}

void ANPCShip::SetupComponentReferences()
{
	// Link behavior component to this ship
	if (BehaviorComponent)
	{
		// Additional setup if needed
	}
}

void ANPCShip::ConfigureMovementForNPC()
{
	// Configure movement component for NPC behavior
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->bOrientRotationToMovement = true;
		MovementComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f); // Faster rotation for NPCs
		MovementComp->bConstrainToPlane = true;
		MovementComp->SetPlaneConstraintNormal(FVector(0, 0, 1));
		MovementComp->MaxWalkSpeed = 400.0f; // Default NPC speed
	}
}

void ANPCShip::StartShieldRegeneration()
{
	if (bIsDead || ShieldRegenTimerHandle.IsValid())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		ShieldRegenTimerHandle,
		this,
		&ANPCShip::OnShieldRegenTick,
		0.1f, // 10 times per second
		true
	);
}

void ANPCShip::StopShieldRegeneration()
{
	GetWorld()->GetTimerManager().ClearTimer(ShieldRegenTimerHandle);
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
	float RegenAmount = ShieldRegenRate * 0.1f; // Per tick amount
	CurrentShields = FMath::Min(CurrentShields + RegenAmount, MaxShields);

	if (CurrentShields != OldShields)
	{
		OnShieldChanged(OldShields, CurrentShields);
	}

	// Stop when shields are full
	if (CurrentShields >= MaxShields)
	{
		StopShieldRegeneration();
	}
}

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
		// Shields absorb some damage, remainder goes to health
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

void ANPCShip::RegisterWithEventBus()
{
	// Find or create event bus
	EventBus = UOdysseyEventBus::Get();
	
	if (EventBus)
	{
		UE_LOG(LogTemp, Log, TEXT("NPCShip %s registered with event bus"), *GetName());
	}
}

void ANPCShip::UnregisterFromEventBus()
{
	EventBus = nullptr;
}

void ANPCShip::BroadcastNPCEvent(const FString& EventType, const TMap<FString, FString>& EventData)
{
	if (EventBus)
	{
		// Create event data with NPC information
		TMap<FString, FString> FullEventData = EventData;
		FullEventData.Add(TEXT("NPCShipName"), GetName());
		FullEventData.Add(TEXT("ShipType"), FString::Printf(TEXT("%d"), (int32)ShipConfig.ShipType));
		FullEventData.Add(TEXT("EventType"), EventType);
		
		// This would integrate with your event bus system
		// EventBus->BroadcastEvent(FString::Printf(TEXT("NPC_%s"), *EventType), FullEventData);
	}
}
