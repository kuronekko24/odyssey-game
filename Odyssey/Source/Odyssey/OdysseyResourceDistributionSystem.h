// OdysseyResourceDistributionSystem.h
// Strategic resource distribution for procedural planet generation
// Part of the Odyssey Procedural Planet & Resource Generation System

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyResourceDistributionSystem.generated.h"

// Forward declarations
class AResourceNode;

/**
 * Enum defining resource rarity tiers
 */
UENUM(BlueprintType)
enum class EResourceRarity : uint8
{
	Common = 0 UMETA(DisplayName = "Common"),
	Uncommon = 1 UMETA(DisplayName = "Uncommon"),
	Rare = 2 UMETA(DisplayName = "Rare"),
	VeryRare = 3 UMETA(DisplayName = "Very Rare"),
	Exotic = 4 UMETA(DisplayName = "Exotic"),
	Legendary = 5 UMETA(DisplayName = "Legendary")
};

/**
 * Enum for resource deposit types
 */
UENUM(BlueprintType)
enum class EResourceDepositType : uint8
{
	Surface = 0 UMETA(DisplayName = "Surface"),
	Shallow = 1 UMETA(DisplayName = "Shallow"),
	Deep = 2 UMETA(DisplayName = "Deep"),
	Vein = 3 UMETA(DisplayName = "Vein"),
	Cluster = 4 UMETA(DisplayName = "Cluster"),
	Anomalous = 5 UMETA(DisplayName = "Anomalous")
};

/**
 * Struct defining a resource deposit location
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FResourceDepositLocation
{
	GENERATED_BODY()

	// World position of the deposit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit")
	FVector Location;

	// Type of resource
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit")
	EResourceType ResourceType;

	// Rarity tier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit")
	EResourceRarity Rarity;

	// Deposit type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit")
	EResourceDepositType DepositType;

	// Quality rating (0.0 - 1.0, affects yield and purity)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Quality;

	// Total resource amount in this deposit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit", meta = (ClampMin = "1"))
	int32 TotalAmount;

	// Remaining amount (for tracking depletion)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit")
	int32 RemainingAmount;

	// Mining difficulty modifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MiningDifficulty;

	// Whether this deposit has been discovered by the player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit")
	bool bDiscovered;

	// Unique identifier for this deposit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit")
	int32 DepositID;

	// Biome this deposit is located in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Deposit")
	EBiomeType BiomeType;

	FResourceDepositLocation()
		: Location(FVector::ZeroVector)
		, ResourceType(EResourceType::Silicate)
		, Rarity(EResourceRarity::Common)
		, DepositType(EResourceDepositType::Surface)
		, Quality(0.5f)
		, TotalAmount(50)
		, RemainingAmount(50)
		, MiningDifficulty(1.0f)
		, bDiscovered(false)
		, DepositID(0)
		, BiomeType(EBiomeType::Barren)
	{
	}
};

/**
 * Struct defining distribution parameters for a resource type
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FResourceDistributionParams
{
	GENERATED_BODY()

	// Resource type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distribution")
	EResourceType ResourceType;

	// Base spawn density (deposits per 1000 square units)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distribution", meta = (ClampMin = "0.0"))
	float BaseDensity;

	// Minimum cluster size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distribution", meta = (ClampMin = "1"))
	int32 MinClusterSize;

	// Maximum cluster size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distribution", meta = (ClampMin = "1"))
	int32 MaxClusterSize;

	// Minimum distance between clusters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distribution", meta = (ClampMin = "10.0"))
	float MinClusterSpacing;

	// Base quality range (min, max)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distribution")
	FVector2D BaseQualityRange;

	// Base amount range per deposit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distribution")
	FIntPoint BaseAmountRange;

	// Rarity weight distribution (Common to Legendary)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distribution")
	TArray<float> RarityWeights;

	FResourceDistributionParams()
		: ResourceType(EResourceType::Silicate)
		, BaseDensity(0.5f)
		, MinClusterSize(1)
		, MaxClusterSize(5)
		, MinClusterSpacing(100.0f)
		, BaseQualityRange(FVector2D(0.3f, 0.8f))
		, BaseAmountRange(FIntPoint(20, 100))
	{
		// Default rarity weights: Common most likely, Legendary very rare
		RarityWeights = { 0.5f, 0.25f, 0.15f, 0.07f, 0.025f, 0.005f };
	}
};

/**
 * Struct for a resource cluster (group of nearby deposits)
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FResourceCluster
{
	GENERATED_BODY()

	// Center position of the cluster
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Cluster")
	FVector CenterLocation;

	// Radius of the cluster
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Cluster")
	float Radius;

	// Primary resource type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Cluster")
	EResourceType PrimaryResource;

	// Secondary resource type (can be None)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Cluster")
	EResourceType SecondaryResource;

	// Deposit IDs in this cluster
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Cluster")
	TArray<int32> DepositIDs;

	// Cluster richness (affects overall quality)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Cluster", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Richness;

	// Unique cluster ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Cluster")
	int32 ClusterID;

	FResourceCluster()
		: CenterLocation(FVector::ZeroVector)
		, Radius(100.0f)
		, PrimaryResource(EResourceType::Silicate)
		, SecondaryResource(EResourceType::None)
		, Richness(0.5f)
		, ClusterID(0)
	{
	}
};

/**
 * Struct for economic trade route opportunity
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTradeRouteOpportunity
{
	GENERATED_BODY()

	// Resource that is abundant at source
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Route")
	EResourceType AbundantResource;

	// Resource that is scarce at destination
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Route")
	EResourceType ScarceResource;

	// Source location/planet ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Route")
	int32 SourceLocationID;

	// Destination location/planet ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Route")
	int32 DestinationLocationID;

	// Estimated profit margin (percentage)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Route")
	float ProfitMargin;

	// Trade volume potential
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Route")
	int32 VolumePotential;

	// Risk level (0.0 - 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade Route")
	float RiskLevel;

	FTradeRouteOpportunity()
		: AbundantResource(EResourceType::None)
		, ScarceResource(EResourceType::None)
		, SourceLocationID(0)
		, DestinationLocationID(0)
		, ProfitMargin(0.0f)
		, VolumePotential(0)
		, RiskLevel(0.0f)
	{
	}
};

/**
 * UOdysseyResourceDistributionSystem
 * 
 * Handles strategic resource placement across planets and regions.
 * Creates economic opportunities through varied resource distribution
 * and generates trade route potential.
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API UOdysseyResourceDistributionSystem : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyResourceDistributionSystem();

	// Initialization
	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	void Initialize(UOdysseyBiomeDefinitionSystem* BiomeSystem);

	// Resource distribution generation
	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	TArray<FResourceDepositLocation> GenerateResourceDeposits(
		int32 Seed,
		const FVector2D& AreaSize,
		const TArray<EBiomeType>& Biomes,
		int32 TargetDepositCount
	);

	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	TArray<FResourceCluster> GenerateResourceClusters(
		int32 Seed,
		const FVector2D& AreaSize,
		EBiomeType Biome,
		int32 ClusterCount
	);

	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	FResourceDepositLocation GenerateSingleDeposit(
		int32 Seed,
		const FVector& Location,
		EBiomeType Biome,
		EResourceType PreferredResource = EResourceType::None
	);

	// Resource node spawning
	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	AResourceNode* SpawnResourceNode(
		UWorld* World,
		const FResourceDepositLocation& DepositData,
		TSubclassOf<AResourceNode> NodeClass
	);

	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	TArray<AResourceNode*> SpawnResourceNodes(
		UWorld* World,
		const TArray<FResourceDepositLocation>& Deposits,
		TSubclassOf<AResourceNode> NodeClass
	);

	// Rarity and quality
	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	EResourceRarity DetermineRarity(int32 Seed, EBiomeType Biome, EResourceType ResourceType) const;

	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	float CalculateQuality(int32 Seed, EResourceRarity Rarity, EBiomeType Biome) const;

	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	int32 CalculateDepositAmount(int32 Seed, EResourceRarity Rarity, EResourceType ResourceType) const;

	// Economic analysis
	UFUNCTION(BlueprintCallable, Category = "Economic Analysis")
	TArray<FTradeRouteOpportunity> AnalyzeTradeOpportunities(
		const TMap<int32, TArray<FResourceDepositLocation>>& LocationResources
	) const;

	UFUNCTION(BlueprintCallable, Category = "Economic Analysis")
	TMap<EResourceType, float> CalculateResourceAbundance(
		const TArray<FResourceDepositLocation>& Deposits
	) const;

	UFUNCTION(BlueprintCallable, Category = "Economic Analysis")
	float GetResourceScarcityScore(
		EResourceType ResourceType,
		const TArray<FResourceDepositLocation>& Deposits
	) const;

	// Distribution parameters
	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	void SetDistributionParams(EResourceType ResourceType, const FResourceDistributionParams& Params);

	UFUNCTION(BlueprintCallable, Category = "Resource Distribution")
	FResourceDistributionParams GetDistributionParams(EResourceType ResourceType) const;

	// Query functions
	UFUNCTION(BlueprintCallable, Category = "Resource Query")
	TArray<FResourceDepositLocation> FindDepositsInRadius(
		const FVector& Center,
		float Radius,
		const TArray<FResourceDepositLocation>& AllDeposits
	) const;

	UFUNCTION(BlueprintCallable, Category = "Resource Query")
	TArray<FResourceDepositLocation> FindDepositsByType(
		EResourceType ResourceType,
		const TArray<FResourceDepositLocation>& AllDeposits
	) const;

	UFUNCTION(BlueprintCallable, Category = "Resource Query")
	TArray<FResourceDepositLocation> FindDepositsByRarity(
		EResourceRarity MinRarity,
		const TArray<FResourceDepositLocation>& AllDeposits
	) const;

	UFUNCTION(BlueprintCallable, Category = "Resource Query")
	FResourceDepositLocation FindNearestDeposit(
		const FVector& Location,
		const TArray<FResourceDepositLocation>& AllDeposits,
		EResourceType FilterType = EResourceType::None
	) const;

	// Utility
	UFUNCTION(BlueprintCallable, Category = "Resource Utility")
	static FString GetRarityDisplayName(EResourceRarity Rarity);

	UFUNCTION(BlueprintCallable, Category = "Resource Utility")
	static FLinearColor GetRarityColor(EResourceRarity Rarity);

	UFUNCTION(BlueprintCallable, Category = "Resource Utility")
	static float GetRarityValueMultiplier(EResourceRarity Rarity);

protected:
	// Reference to biome system
	UPROPERTY()
	UOdysseyBiomeDefinitionSystem* BiomeDefinitionSystem;

	// Distribution parameters per resource type
	UPROPERTY()
	TMap<EResourceType, FResourceDistributionParams> DistributionParameters;

	// Deposit ID counter
	UPROPERTY()
	int32 NextDepositID;

	// Cluster ID counter
	UPROPERTY()
	int32 NextClusterID;

private:
	// Initialize default distribution parameters
	void InitializeDefaultParameters();

	// Generate cluster center positions using Poisson disk sampling
	TArray<FVector2D> GenerateClusterCenters(
		int32 Seed,
		const FVector2D& AreaSize,
		float MinSpacing,
		int32 MaxAttempts
	) const;

	// Generate deposits within a cluster
	TArray<FResourceDepositLocation> GenerateClusterDeposits(
		int32 Seed,
		const FVector& ClusterCenter,
		float ClusterRadius,
		EResourceType PrimaryResource,
		EBiomeType Biome,
		int32 DepositCount
	);

	// Calculate biome-adjusted density
	float GetAdjustedDensity(EResourceType ResourceType, EBiomeType Biome) const;

	// Select resource type based on biome weights
	EResourceType SelectResourceForBiome(int32 Seed, EBiomeType Biome) const;

	// Hash function for seeded randomness
	static uint32 HashSeed(int32 Seed);
	static float SeededRandom(int32 Seed);
	static int32 SeededRandomRange(int32 Seed, int32 Min, int32 Max);
	static FVector2D SeededRandomPoint(int32 Seed, const FVector2D& Min, const FVector2D& Max);
};
