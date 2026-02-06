// CombatTypes.h
// Core types, enumerations, and data structures for the Phase 3 Combat & Targeting System
// Provides shared definitions used across all combat subsystems

#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

// ============================================================================
// Forward Declarations
// ============================================================================

class AActor;
class UNPCHealthComponent;

// ============================================================================
// Combat Enumerations
// ============================================================================

/**
 * Combat engagement state for the overall system
 */
UENUM(BlueprintType)
enum class ECombatEngagementState : uint8
{
	Idle = 0,           // No combat activity
	Scanning,           // Looking for targets
	Locked,             // Target acquired, weapons tracking
	Firing,             // Actively engaging target
	Cooldown,           // Between engagement bursts
	Disengaging         // Breaking off combat
};

/**
 * Weapon slot identifiers for multi-weapon loadouts
 */
UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Primary = 0,
	Secondary,
	Tertiary,
	Special
};

/**
 * Visual effect intensity levels for mobile performance scaling
 */
UENUM(BlueprintType)
enum class EEffectQuality : uint8
{
	Minimal = 0,    // Text-only damage, no particles
	Low,            // Basic particles, no trails
	Medium,         // Standard particles and trails
	High            // Full effects with screen shake
};

/**
 * Targeting reticle visual states
 */
UENUM(BlueprintType)
enum class EReticleState : uint8
{
	Hidden = 0,
	Searching,      // Pulsing circle, no lock
	Locking,        // Contracting circle, acquiring lock
	Locked,         // Solid reticle, target locked
	Firing,         // Red flash, weapons hot
	OutOfRange      // Dashed circle, target too far
};

// ============================================================================
// Combat Data Structures
// ============================================================================

/**
 * Snapshot of a potential or current target's combat-relevant data.
 * Designed for value semantics -- cheap to copy, no heap allocations.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCombatTargetSnapshot
{
	GENERATED_BODY()

	/** The target actor (weak ref to avoid preventing GC) */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	TWeakObjectPtr<AActor> Actor;

	/** World-space position at the time of the snapshot */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FVector WorldLocation;

	/** Velocity for lead-target prediction */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FVector Velocity;

	/** Distance from player ship */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	float Distance;

	/** Health as 0..1 fraction */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	float HealthFraction;

	/** Whether NPC behavior reports hostile */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	bool bIsHostile;

	/** Whether a clear line of fire exists */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	bool bHasLineOfSight;

	/** Computed priority score (higher = better target) */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	float PriorityScore;

	/** Game-time when this snapshot was taken */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	double SnapshotTime;

	FCombatTargetSnapshot()
		: WorldLocation(FVector::ZeroVector)
		, Velocity(FVector::ZeroVector)
		, Distance(TNumericLimits<float>::Max())
		, HealthFraction(1.0f)
		, bIsHostile(false)
		, bHasLineOfSight(false)
		, PriorityScore(0.0f)
		, SnapshotTime(0.0)
	{
	}

	bool IsValid() const { return Actor.IsValid(); }
	AActor* GetActor() const { return Actor.Get(); }
};

/**
 * Result of a single weapon discharge
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCombatFireResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	bool bFired;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	bool bHit;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	bool bCritical;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	bool bKillingBlow;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	float DamageDealt;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	FVector ImpactLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	TWeakObjectPtr<AActor> HitActor;

	UPROPERTY(BlueprintReadOnly, Category = "Fire Result")
	FName FailReason;

	FCombatFireResult()
		: bFired(false)
		, bHit(false)
		, bCritical(false)
		, bKillingBlow(false)
		, DamageDealt(0.0f)
		, ImpactLocation(FVector::ZeroVector)
	{
	}
};

/**
 * Configuration block for the targeting subsystem.
 * Exposed to Blueprints and the editor for tuning.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTargetingConfig
{
	GENERATED_BODY()

	/** Maximum distance at which an enemy can be selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "500.0"))
	float MaxRange;

	/** Radius (in screen pixels) around a touch that counts as hitting an enemy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "20.0"))
	float TouchRadiusPixels;

	/** How often (seconds) auto-targeting re-evaluates */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.1"))
	float AutoTargetInterval;

	/** Weight for distance in priority scoring (0 = ignore distance) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float DistanceWeight;

	/** Weight for low-health bias in priority scoring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float LowHealthWeight;

	/** Weight for hostility in priority scoring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float HostilityWeight;

	/** Tags that make an actor a valid target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	TArray<FName> ValidTargetTags;

	FTargetingConfig()
		: MaxRange(3000.0f)
		, TouchRadiusPixels(60.0f)
		, AutoTargetInterval(0.4f)
		, DistanceWeight(1.0f)
		, LowHealthWeight(0.6f)
		, HostilityWeight(1.5f)
	{
		ValidTargetTags.Add(FName("Enemy"));
		ValidTargetTags.Add(FName("NPC"));
	}
};

/**
 * Configuration block for the auto-weapon subsystem.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FAutoWeaponConfig
{
	GENERATED_BODY()

	/** Base damage per shot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1.0"))
	float BaseDamage;

	/** Shots per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.1"))
	float FireRate;

	/** Maximum weapon engagement range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "100.0"))
	float EngagementRange;

	/** Accuracy as 0..1 fraction (1 = perfect) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Accuracy;

	/** Critical hit probability */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CritChance;

	/** Damage multiplier on critical hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1.0"))
	float CritMultiplier;

	/** Energy cost per shot (0 = free firing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	int32 EnergyCost;

	/** Projectile speed for lead calculation (0 = hitscan) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.0"))
	float ProjectileSpeed;

	/** Offset from owner origin to muzzle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FVector MuzzleOffset;

	FAutoWeaponConfig()
		: BaseDamage(20.0f)
		, FireRate(3.0f)
		, EngagementRange(2000.0f)
		, Accuracy(0.92f)
		, CritChance(0.08f)
		, CritMultiplier(2.0f)
		, EnergyCost(5)
		, ProjectileSpeed(0.0f)
		, MuzzleOffset(FVector(120.0f, 0.0f, 0.0f))
	{
	}
};

/**
 * Configuration block for combat visual feedback.
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCombatFeedbackConfig
{
	GENERATED_BODY()

	/** Quality level for visual effects (scaled for device) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	EEffectQuality EffectQuality;

	/** Reticle base size in screen pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "32.0"))
	float ReticleSize;

	/** Duration (seconds) for floating damage numbers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0.3"))
	float DamageNumberLifetime;

	/** Maximum simultaneous floating damage numbers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "1"))
	int32 MaxDamageNumbers;

	/** Whether to show health bars above enemies in range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	bool bShowEnemyHealthBars;

	/** Whether to show hit markers on successful hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	bool bShowHitMarkers;

	/** Duration (seconds) for hit marker flash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback", meta = (ClampMin = "0.05"))
	float HitMarkerDuration;

	/** Color for normal damage numbers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	FLinearColor NormalDamageColor;

	/** Color for critical damage numbers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	FLinearColor CritDamageColor;

	/** Color for the targeting reticle when locked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	FLinearColor ReticleLockedColor;

	/** Color for the targeting reticle when out of range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Feedback")
	FLinearColor ReticleOutOfRangeColor;

	FCombatFeedbackConfig()
		: EffectQuality(EEffectQuality::Medium)
		, ReticleSize(72.0f)
		, DamageNumberLifetime(1.2f)
		, MaxDamageNumbers(8)
		, bShowEnemyHealthBars(true)
		, bShowHitMarkers(true)
		, HitMarkerDuration(0.25f)
		, NormalDamageColor(FLinearColor::White)
		, CritDamageColor(FLinearColor(1.0f, 0.2f, 0.1f, 1.0f))
		, ReticleLockedColor(FLinearColor::Red)
		, ReticleOutOfRangeColor(FLinearColor(0.6f, 0.6f, 0.6f, 0.5f))
	{
	}
};

/**
 * Aggregate combat session statistics
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCombatSessionStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ShotsFired;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ShotsHit;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 CriticalHits;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float TotalDamageDealt;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 EnemiesDestroyed;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float EngagementDuration;

	FCombatSessionStats()
		: ShotsFired(0)
		, ShotsHit(0)
		, CriticalHits(0)
		, TotalDamageDealt(0.0f)
		, EnemiesDestroyed(0)
		, EngagementDuration(0.0f)
	{
	}

	void Reset()
	{
		ShotsFired = 0;
		ShotsHit = 0;
		CriticalHits = 0;
		TotalDamageDealt = 0.0f;
		EnemiesDestroyed = 0;
		EngagementDuration = 0.0f;
	}

	float GetAccuracy() const
	{
		return ShotsFired > 0 ? static_cast<float>(ShotsHit) / static_cast<float>(ShotsFired) : 0.0f;
	}

	float GetCritRate() const
	{
		return ShotsHit > 0 ? static_cast<float>(CriticalHits) / static_cast<float>(ShotsHit) : 0.0f;
	}

	float GetDPS() const
	{
		return EngagementDuration > 0.0f ? TotalDamageDealt / EngagementDuration : 0.0f;
	}
};
