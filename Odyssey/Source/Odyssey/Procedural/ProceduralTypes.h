// ProceduralTypes.h
// Shared type definitions for the Procedural Planet & Resource Generation System
// Centralizes enums, structs, and constants used across all procedural subsystems

#pragma once

#include "CoreMinimal.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "ProceduralTypes.generated.h"

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

class UOdysseyBiomeDefinitionSystem;
class UOdysseyResourceDistributionSystem;
class UOdysseyPlanetaryEconomyComponent;
class UExplorationRewardSystem;

// ============================================================================
// CONSTANTS
// ============================================================================

namespace ProceduralConstants
{
	// Noise generation
	constexpr int32 MaxOctaves = 8;
	constexpr float DefaultLacunarity = 2.0f;
	constexpr float DefaultPersistence = 0.5f;

	// Planet generation
	constexpr int32 MinBiomesPerPlanet = 2;
	constexpr int32 MaxBiomesPerPlanet = 12;
	constexpr int32 MinPOIsPerPlanet = 3;
	constexpr int32 MaxPOIsPerPlanet = 50;
	constexpr float MinWorldSize = 2000.0f;
	constexpr float MaxWorldSize = 50000.0f;

	// Resource distribution
	constexpr float MinClusterSpacing = 50.0f;
	constexpr int32 PoissonMaxRetries = 30;
	constexpr float ResourceDensityScale = 0.001f;

	// Exploration
	constexpr int32 BaseDiscoveryReward = 50;
	constexpr int32 RareDiscoveryMultiplier = 5;
	constexpr float ScanRevealRadius = 500.0f;
	constexpr float MaxExplorationProgress = 100.0f;

	// Economy
	constexpr float BaseMarketUpdateInterval = 60.0f;
	constexpr float MinPriceMultiplier = 0.25f;
	constexpr float MaxPriceMultiplier = 4.0f;
	constexpr float DefaultTaxRate = 0.05f;
}

// ============================================================================
// EXPLORATION ENUMERATIONS
// ============================================================================

/**
 * Discovery types for exploration rewards
 */
UENUM(BlueprintType)
enum class EDiscoveryType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	ResourceDeposit UMETA(DisplayName = "Resource Deposit"),
	AncientRuins UMETA(DisplayName = "Ancient Ruins"),
	AlienArtifact UMETA(DisplayName = "Alien Artifact"),
	NaturalWonder UMETA(DisplayName = "Natural Wonder"),
	AbandonedOutpost UMETA(DisplayName = "Abandoned Outpost"),
	BiologicalSpecimen UMETA(DisplayName = "Biological Specimen"),
	AnomalousSignal UMETA(DisplayName = "Anomalous Signal"),
	HiddenCache UMETA(DisplayName = "Hidden Cache"),
	WreckedShip UMETA(DisplayName = "Wrecked Ship"),
	PrecursorTechnology UMETA(DisplayName = "Precursor Technology"),
	QuantumAnomaly UMETA(DisplayName = "Quantum Anomaly"),
	RareMineral UMETA(DisplayName = "Rare Mineral"),
	GeothermalVent UMETA(DisplayName = "Geothermal Vent"),
	FrozenOrganism UMETA(DisplayName = "Frozen Organism"),
	CrystalFormation UMETA(DisplayName = "Crystal Formation")
};

/**
 * Discovery rarity tiers
 */
UENUM(BlueprintType)
enum class EDiscoveryRarity : uint8
{
	Common = 0 UMETA(DisplayName = "Common"),
	Uncommon = 1 UMETA(DisplayName = "Uncommon"),
	Rare = 2 UMETA(DisplayName = "Rare"),
	Epic = 3 UMETA(DisplayName = "Epic"),
	Legendary = 4 UMETA(DisplayName = "Legendary"),
	Mythic = 5 UMETA(DisplayName = "Mythic")
};

/**
 * Exploration scan modes
 */
UENUM(BlueprintType)
enum class EScanMode : uint8
{
	Passive = 0 UMETA(DisplayName = "Passive"),
	ActiveShort UMETA(DisplayName = "Active - Short Range"),
	ActiveLong UMETA(DisplayName = "Active - Long Range"),
	Deep UMETA(DisplayName = "Deep Scan"),
	Anomaly UMETA(DisplayName = "Anomaly Detection")
};

/**
 * Planet exploration status
 */
UENUM(BlueprintType)
enum class EExplorationStatus : uint8
{
	Uncharted = 0 UMETA(DisplayName = "Uncharted"),
	Surveyed UMETA(DisplayName = "Surveyed"),
	PartiallyExplored UMETA(DisplayName = "Partially Explored"),
	MostlyExplored UMETA(DisplayName = "Mostly Explored"),
	FullyExplored UMETA(DisplayName = "Fully Explored"),
	Mastered UMETA(DisplayName = "Mastered")
};

// ============================================================================
// EXPLORATION STRUCTURES
// ============================================================================

/**
 * Individual discovery data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDiscoveryData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	int32 DiscoveryID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	EDiscoveryType DiscoveryType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	EDiscoveryRarity Rarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	FText LoreText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	FVector WorldLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	int32 PlanetID;

	// Reward data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	int32 OMENReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	int32 ExperienceReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	TArray<FResourceStack> ResourceRewards;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	FName BlueprintUnlock;

	// Discovery state
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bDiscovered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bClaimed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	double DiscoveredTimestamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FString DiscovererPlayerID;

	// Scan requirements
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	EScanMode RequiredScanMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	float ScanDifficulty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	float DetectionRadius;

	FDiscoveryData()
		: DiscoveryID(0)
		, DiscoveryType(EDiscoveryType::None)
		, Rarity(EDiscoveryRarity::Common)
		, Name(FText::FromString(TEXT("Unknown Discovery")))
		, Description(FText::GetEmpty())
		, LoreText(FText::GetEmpty())
		, WorldLocation(FVector::ZeroVector)
		, PlanetID(0)
		, OMENReward(50)
		, ExperienceReward(100)
		, BlueprintUnlock(NAME_None)
		, bDiscovered(false)
		, bClaimed(false)
		, DiscoveredTimestamp(0.0)
		, RequiredScanMode(EScanMode::Passive)
		, ScanDifficulty(1.0f)
		, DetectionRadius(200.0f)
	{
	}
};

/**
 * Exploration progress data for a planet
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlanetExplorationData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	int32 PlanetID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	EExplorationStatus Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	float ExplorationPercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	int32 TotalDiscoveries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	int32 ClaimedDiscoveries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	int32 RegionsExplored;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	int32 TotalRegions;

	// Biomes discovered on this planet
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	TArray<EBiomeType> DiscoveredBiomes;

	// Fog of war grid (simplified for mobile)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	TArray<bool> ExploredGrid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	int32 GridResolution;

	// Timestamps
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	double FirstVisitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	double LastVisitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exploration")
	float TotalTimeSpent;

	FPlanetExplorationData()
		: PlanetID(0)
		, Status(EExplorationStatus::Uncharted)
		, ExplorationPercent(0.0f)
		, TotalDiscoveries(0)
		, ClaimedDiscoveries(0)
		, RegionsExplored(0)
		, TotalRegions(0)
		, GridResolution(32)
		, FirstVisitTime(0.0)
		, LastVisitTime(0.0)
		, TotalTimeSpent(0.0f)
	{
	}
};

/**
 * Exploration milestone with reward
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FExplorationMilestone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
	FName MilestoneID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
	float RequiredExplorationPercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
	int32 RequiredDiscoveryCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
	int32 OMENReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
	int32 ExperienceReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Milestone")
	bool bCompleted;

	FExplorationMilestone()
		: MilestoneID(NAME_None)
		, Name(FText::GetEmpty())
		, Description(FText::GetEmpty())
		, RequiredExplorationPercent(0.0f)
		, RequiredDiscoveryCount(0)
		, OMENReward(0)
		, ExperienceReward(0)
		, bCompleted(false)
	{
	}
};

/**
 * Scan result from an active scan
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FScanResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	bool bFoundSomething;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	FVector SignalLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	float SignalStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	EDiscoveryType HintedType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	EDiscoveryRarity HintedRarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	float DistanceToDiscovery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	int32 DiscoveryID;

	FScanResult()
		: bFoundSomething(false)
		, SignalLocation(FVector::ZeroVector)
		, SignalStrength(0.0f)
		, HintedType(EDiscoveryType::None)
		, HintedRarity(EDiscoveryRarity::Common)
		, DistanceToDiscovery(0.0f)
		, DiscoveryID(-1)
	{
	}
};

// ============================================================================
// PROCEDURAL NOISE UTILITY
// ============================================================================

/**
 * Lightweight noise parameters for procedural generation
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FNoiseParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	int32 Seed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "1", ClampMax = "8"))
	int32 Octaves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "0.001"))
	float Frequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float Lacunarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Persistence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	float Amplitude;

	FNoiseParameters()
		: Seed(0)
		, Octaves(4)
		, Frequency(0.01f)
		, Lacunarity(2.0f)
		, Persistence(0.5f)
		, Amplitude(1.0f)
	{
	}
};

// ============================================================================
// SERIALIZATION STRUCTURES
// ============================================================================

/**
 * Serializable planet save data (minimal footprint for mobile)
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlanetSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 PlanetID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 GenerationSeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	bool bDiscovered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	float ExplorationPercent;

	// Only store IDs of discovered/claimed discoveries (regenerate rest from seed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	TArray<int32> DiscoveredDiscoveryIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	TArray<int32> ClaimedDiscoveryIDs;

	// Depleted resource deposits (store only changes from generated state)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	TMap<int32, int32> DepositRemainingAmounts;

	// Economy state
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 WealthLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 Population;

	FPlanetSaveData()
		: PlanetID(0)
		, GenerationSeed(0)
		, bDiscovered(false)
		, ExplorationPercent(0.0f)
		, WealthLevel(50)
		, Population(0)
	{
	}
};

