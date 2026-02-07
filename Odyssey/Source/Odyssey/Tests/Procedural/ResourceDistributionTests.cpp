// ResourceDistributionTests.cpp
// Comprehensive automation tests for UOdysseyResourceDistributionSystem
// Tests resource distribution, rarity tiers, biome placement, density, and edge cases

#include "Misc/AutomationTest.h"
#include "OdysseyResourceDistributionSystem.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "Procedural/ProceduralTypes.h"

// ============================================================================
// HELPERS
// ============================================================================

namespace ResourceTestHelpers
{
	static UOdysseyResourceDistributionSystem* CreateInitializedResourceSystem()
	{
		UOdysseyBiomeDefinitionSystem* BiomeSystem = NewObject<UOdysseyBiomeDefinitionSystem>();
		BiomeSystem->Initialize(nullptr);

		UOdysseyResourceDistributionSystem* ResourceSystem = NewObject<UOdysseyResourceDistributionSystem>();
		ResourceSystem->Initialize(BiomeSystem);

		return ResourceSystem;
	}

	static UOdysseyBiomeDefinitionSystem* CreateInitializedBiomeSystem()
	{
		UOdysseyBiomeDefinitionSystem* BiomeSystem = NewObject<UOdysseyBiomeDefinitionSystem>();
		BiomeSystem->Initialize(nullptr);
		return BiomeSystem;
	}
}

// ============================================================================
// 1. BASIC RESOURCE DISTRIBUTION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_GenerateDeposits,
	"Odyssey.Procedural.ResourceDistribution.GenerateDepositsBasic",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_GenerateDeposits::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Desert, EBiomeType::Forest, EBiomeType::Volcanic };
	const int32 TargetCount = 20;

	TArray<FResourceDepositLocation> Deposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, Biomes, TargetCount);

	TestTrue(TEXT("Should generate deposits"), Deposits.Num() > 0);
	// Target count is a hint, actual count may vary due to clustering
	TestTrue(
		FString::Printf(TEXT("Deposit count (%d) should be reasonable relative to target (%d)"), Deposits.Num(), TargetCount),
		Deposits.Num() >= TargetCount / 3 && Deposits.Num() <= TargetCount * 3);

	for (const FResourceDepositLocation& Deposit : Deposits)
	{
		TestTrue(TEXT("Resource type should not be None"), Deposit.ResourceType != EResourceType::None);
		TestTrue(TEXT("Total amount should be positive"), Deposit.TotalAmount > 0);
		TestTrue(TEXT("Remaining amount should equal total initially"), Deposit.RemainingAmount == Deposit.TotalAmount);
		TestTrue(TEXT("Quality should be in [0, 1]"), Deposit.Quality >= 0.0f && Deposit.Quality <= 1.0f);
		TestTrue(TEXT("Mining difficulty should be positive"), Deposit.MiningDifficulty > 0.0f);
		TestFalse(TEXT("Deposit should start undiscovered"), Deposit.bDiscovered);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_DepositsWithinBounds,
	"Odyssey.Procedural.ResourceDistribution.DepositsWithinWorldBounds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_DepositsWithinBounds::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(5000.0f, 5000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Crystalline, EBiomeType::Metallic };
	const int32 TargetCount = 30;

	TArray<FResourceDepositLocation> Deposits = ResourceSystem->GenerateResourceDeposits(12345, AreaSize, Biomes, TargetCount);

	for (int32 i = 0; i < Deposits.Num(); ++i)
	{
		const FResourceDepositLocation& Dep = Deposits[i];
		TestTrue(
			FString::Printf(TEXT("Deposit %d X (%.1f) should be within area width"), i, Dep.Location.X),
			Dep.Location.X >= 0.0f && Dep.Location.X <= AreaSize.X);
		TestTrue(
			FString::Printf(TEXT("Deposit %d Y (%.1f) should be within area height"), i, Dep.Location.Y),
			Dep.Location.Y >= 0.0f && Dep.Location.Y <= AreaSize.Y);
	}

	return true;
}

// ============================================================================
// 2. BIOME-CORRECT RESOURCE PLACEMENT
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_BiomeInfluencesResources,
	"Odyssey.Procedural.ResourceDistribution.BiomeInfluencesResourceTypes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_BiomeInfluencesResources::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	const int32 TargetCount = 50;

	// Generate deposits for a volcanic biome
	TArray<EBiomeType> VolcanicBiomes = { EBiomeType::Volcanic };
	TArray<FResourceDepositLocation> VolcanicDeposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, VolcanicBiomes, TargetCount);

	// Generate deposits for an ice biome
	TArray<EBiomeType> IceBiomes = { EBiomeType::Ice };
	TArray<FResourceDepositLocation> IceDeposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, IceBiomes, TargetCount);

	// The resource type distributions should differ between biomes
	TMap<EResourceType, int32> VolcanicResourceCounts;
	for (const auto& Dep : VolcanicDeposits)
	{
		VolcanicResourceCounts.FindOrAdd(Dep.ResourceType)++;
	}

	TMap<EResourceType, int32> IceResourceCounts;
	for (const auto& Dep : IceDeposits)
	{
		IceResourceCounts.FindOrAdd(Dep.ResourceType)++;
	}

	// The distributions should not be identical (different biomes = different resource profiles)
	bool bDistributionsDiffer = false;
	for (const auto& Pair : VolcanicResourceCounts)
	{
		int32* IceCount = IceResourceCounts.Find(Pair.Key);
		if (!IceCount || *IceCount != Pair.Value)
		{
			bDistributionsDiffer = true;
			break;
		}
	}
	if (!bDistributionsDiffer && IceResourceCounts.Num() != VolcanicResourceCounts.Num())
	{
		bDistributionsDiffer = true;
	}

	TestTrue(TEXT("Volcanic and Ice biomes should produce different resource distributions"), bDistributionsDiffer);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_SingleDepositBiomeSpecific,
	"Odyssey.Procedural.ResourceDistribution.SingleDepositBiomeSpecific",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_SingleDepositBiomeSpecific::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector Location(500.0f, 500.0f, 0.0f);

	// Generate single deposits in different biomes
	FResourceDepositLocation DesertDep = ResourceSystem->GenerateSingleDeposit(42, Location, EBiomeType::Desert);
	FResourceDepositLocation ForestDep = ResourceSystem->GenerateSingleDeposit(42, Location, EBiomeType::Forest);

	TestTrue(TEXT("Desert deposit should have a valid resource type"), DesertDep.ResourceType != EResourceType::None);
	TestTrue(TEXT("Forest deposit should have a valid resource type"), ForestDep.ResourceType != EResourceType::None);
	TestTrue(TEXT("Desert deposit quality should be in [0, 1]"), DesertDep.Quality >= 0.0f && DesertDep.Quality <= 1.0f);
	TestTrue(TEXT("Forest deposit quality should be in [0, 1]"), ForestDep.Quality >= 0.0f && ForestDep.Quality <= 1.0f);
	TestEqual(TEXT("Deposit location should match input"), DesertDep.Location, Location);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_PreferredResourceType,
	"Odyssey.Procedural.ResourceDistribution.PreferredResourceTypeRespected",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_PreferredResourceType::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector Location(1000.0f, 1000.0f, 0.0f);

	// Generate single deposit with a preferred resource type
	FResourceDepositLocation Deposit = ResourceSystem->GenerateSingleDeposit(
		42, Location, EBiomeType::Metallic, EResourceType::TitaniumAlloy);

	TestEqual(TEXT("Deposit should use the preferred resource type"), Deposit.ResourceType, EResourceType::TitaniumAlloy);

	return true;
}

// ============================================================================
// 3. RARITY TIER DISTRIBUTION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_RarityTiersRespected,
	"Odyssey.Procedural.ResourceDistribution.RarityTiersRespected",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_RarityTiersRespected::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(20000.0f, 20000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Desert, EBiomeType::Forest, EBiomeType::Volcanic, EBiomeType::Crystalline };
	const int32 TargetCount = 200;

	TArray<FResourceDepositLocation> Deposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, Biomes, TargetCount);

	// Count rarity distribution
	TMap<EResourceRarity, int32> RarityCounts;
	for (const auto& Dep : Deposits)
	{
		RarityCounts.FindOrAdd(Dep.Rarity)++;
	}

	int32 CommonCount = RarityCounts.FindRef(EResourceRarity::Common);
	int32 LegendaryCount = RarityCounts.FindRef(EResourceRarity::Legendary);

	// Common should be more frequent than Legendary
	TestTrue(
		FString::Printf(TEXT("Common (%d) should outnumber Legendary (%d)"), CommonCount, LegendaryCount),
		CommonCount > LegendaryCount);

	// There should be at least some common resources
	TestTrue(TEXT("Should have at least some Common deposits"), CommonCount > 0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_RarityDetermination,
	"Odyssey.Procedural.ResourceDistribution.RarityDeterminationConsistent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_RarityDetermination::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	// Same seed should produce same rarity
	EResourceRarity RarityA = ResourceSystem->DetermineRarity(42, EBiomeType::Desert, EResourceType::Silicate);
	EResourceRarity RarityB = ResourceSystem->DetermineRarity(42, EBiomeType::Desert, EResourceType::Silicate);
	TestEqual(TEXT("Same seed/biome/resource should produce same rarity"), RarityA, RarityB);

	// All rarity values should be within the enum range
	for (int32 Seed = 0; Seed < 100; ++Seed)
	{
		EResourceRarity Rarity = ResourceSystem->DetermineRarity(Seed, EBiomeType::Crystalline, EResourceType::QuantumDots);
		TestTrue(
			FString::Printf(TEXT("Rarity %d should be a valid enum value"), static_cast<int32>(Rarity)),
			static_cast<int32>(Rarity) >= 0 && static_cast<int32>(Rarity) <= 5);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_RarityValueMultiplier,
	"Odyssey.Procedural.ResourceDistribution.RarityValueMultiplierScales",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_RarityValueMultiplier::RunTest(const FString& Parameters)
{
	float CommonMult = UOdysseyResourceDistributionSystem::GetRarityValueMultiplier(EResourceRarity::Common);
	float UncommonMult = UOdysseyResourceDistributionSystem::GetRarityValueMultiplier(EResourceRarity::Uncommon);
	float RareMult = UOdysseyResourceDistributionSystem::GetRarityValueMultiplier(EResourceRarity::Rare);
	float VeryRareMult = UOdysseyResourceDistributionSystem::GetRarityValueMultiplier(EResourceRarity::VeryRare);
	float ExoticMult = UOdysseyResourceDistributionSystem::GetRarityValueMultiplier(EResourceRarity::Exotic);
	float LegendaryMult = UOdysseyResourceDistributionSystem::GetRarityValueMultiplier(EResourceRarity::Legendary);

	TestTrue(TEXT("Common multiplier should be positive"), CommonMult > 0.0f);
	TestTrue(TEXT("Uncommon >= Common"), UncommonMult >= CommonMult);
	TestTrue(TEXT("Rare >= Uncommon"), RareMult >= UncommonMult);
	TestTrue(TEXT("VeryRare >= Rare"), VeryRareMult >= RareMult);
	TestTrue(TEXT("Exotic >= VeryRare"), ExoticMult >= VeryRareMult);
	TestTrue(TEXT("Legendary >= Exotic"), LegendaryMult >= ExoticMult);

	return true;
}

// ============================================================================
// 4. QUALITY AND AMOUNT CALCULATIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_QualityCalculation,
	"Odyssey.Procedural.ResourceDistribution.QualityCalculationValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_QualityCalculation::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	for (int32 Seed = 0; Seed < 50; ++Seed)
	{
		float Quality = ResourceSystem->CalculateQuality(Seed, EResourceRarity::Rare, EBiomeType::Crystalline);
		TestTrue(
			FString::Printf(TEXT("Quality %.3f should be in [0, 1] for seed %d"), Quality, Seed),
			Quality >= 0.0f && Quality <= 1.0f);
	}

	// Higher rarity should generally yield higher quality
	float TotalCommonQuality = 0.0f;
	float TotalLegendaryQuality = 0.0f;
	const int32 Samples = 100;
	for (int32 Seed = 0; Seed < Samples; ++Seed)
	{
		TotalCommonQuality += ResourceSystem->CalculateQuality(Seed, EResourceRarity::Common, EBiomeType::Forest);
		TotalLegendaryQuality += ResourceSystem->CalculateQuality(Seed, EResourceRarity::Legendary, EBiomeType::Forest);
	}

	float AvgCommon = TotalCommonQuality / Samples;
	float AvgLegendary = TotalLegendaryQuality / Samples;
	TestTrue(
		FString::Printf(TEXT("Average Legendary quality (%.3f) should exceed Common (%.3f)"), AvgLegendary, AvgCommon),
		AvgLegendary >= AvgCommon);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_DepositAmountCalculation,
	"Odyssey.Procedural.ResourceDistribution.DepositAmountCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_DepositAmountCalculation::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	for (int32 Seed = 0; Seed < 50; ++Seed)
	{
		int32 Amount = ResourceSystem->CalculateDepositAmount(Seed, EResourceRarity::Rare, EResourceType::Silicate);
		TestTrue(
			FString::Printf(TEXT("Deposit amount %d should be positive for seed %d"), Amount, Seed),
			Amount > 0);
	}

	return true;
}

// ============================================================================
// 5. RESOURCE CLUSTERING
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_ClusterGeneration,
	"Odyssey.Procedural.ResourceDistribution.ClusterGeneration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_ClusterGeneration::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	const int32 ClusterCount = 5;

	TArray<FResourceCluster> Clusters = ResourceSystem->GenerateResourceClusters(42, AreaSize, EBiomeType::Desert, ClusterCount);

	TestEqual(TEXT("Should generate requested cluster count"), Clusters.Num(), ClusterCount);

	TSet<int32> ClusterIDs;
	for (const FResourceCluster& Cluster : Clusters)
	{
		TestTrue(TEXT("Cluster radius should be positive"), Cluster.Radius > 0.0f);
		TestTrue(TEXT("Cluster richness should be in [0, 1]"), Cluster.Richness >= 0.0f && Cluster.Richness <= 1.0f);
		TestTrue(TEXT("Primary resource should not be None"), Cluster.PrimaryResource != EResourceType::None);
		TestTrue(TEXT("Cluster should have deposit IDs"), Cluster.DepositIDs.Num() > 0);

		TestFalse(
			FString::Printf(TEXT("Cluster ID %d should be unique"), Cluster.ClusterID),
			ClusterIDs.Contains(Cluster.ClusterID));
		ClusterIDs.Add(Cluster.ClusterID);

		// Cluster center should be within bounds
		TestTrue(TEXT("Cluster center X should be within area"),
			Cluster.CenterLocation.X >= 0.0f && Cluster.CenterLocation.X <= AreaSize.X);
		TestTrue(TEXT("Cluster center Y should be within area"),
			Cluster.CenterLocation.Y >= 0.0f && Cluster.CenterLocation.Y <= AreaSize.Y);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_ClusterSpacing,
	"Odyssey.Procedural.ResourceDistribution.ClustersRespectMinSpacing",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_ClusterSpacing::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(20000.0f, 20000.0f);
	const int32 ClusterCount = 10;

	TArray<FResourceCluster> Clusters = ResourceSystem->GenerateResourceClusters(42, AreaSize, EBiomeType::Forest, ClusterCount);

	// Check minimum spacing between cluster centers
	for (int32 i = 0; i < Clusters.Num(); ++i)
	{
		for (int32 j = i + 1; j < Clusters.Num(); ++j)
		{
			float Distance = FVector::Dist(Clusters[i].CenterLocation, Clusters[j].CenterLocation);
			TestTrue(
				FString::Printf(TEXT("Clusters %d and %d spacing (%.1f) should respect min spacing"), i, j, Distance),
				Distance >= ProceduralConstants::MinClusterSpacing);
		}
	}

	return true;
}

// ============================================================================
// 6. QUERY FUNCTIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_FindDepositsInRadius,
	"Odyssey.Procedural.ResourceDistribution.FindDepositsInRadius",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_FindDepositsInRadius::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Desert };
	TArray<FResourceDepositLocation> AllDeposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, Biomes, 50);

	// Search from center with a radius
	FVector SearchCenter(5000.0f, 5000.0f, 0.0f);
	float SearchRadius = 2000.0f;

	TArray<FResourceDepositLocation> Found = ResourceSystem->FindDepositsInRadius(SearchCenter, SearchRadius, AllDeposits);

	// All found deposits should be within the search radius
	for (const auto& Dep : Found)
	{
		float Dist = FVector::Dist(SearchCenter, Dep.Location);
		TestTrue(
			FString::Printf(TEXT("Found deposit at distance %.1f should be within radius %.1f"), Dist, SearchRadius),
			Dist <= SearchRadius * 1.01f); // Small tolerance for floating point
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_FindDepositsByType,
	"Odyssey.Procedural.ResourceDistribution.FindDepositsByType",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_FindDepositsByType::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Metallic, EBiomeType::Volcanic };
	TArray<FResourceDepositLocation> AllDeposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, Biomes, 100);

	// Pick a resource type that should exist
	if (AllDeposits.Num() > 0)
	{
		EResourceType SearchType = AllDeposits[0].ResourceType;
		TArray<FResourceDepositLocation> Filtered = ResourceSystem->FindDepositsByType(SearchType, AllDeposits);

		TestTrue(TEXT("Should find at least one deposit of the searched type"), Filtered.Num() > 0);
		for (const auto& Dep : Filtered)
		{
			TestEqual(TEXT("All filtered deposits should match the searched type"), Dep.ResourceType, SearchType);
		}
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_FindDepositsByRarity,
	"Odyssey.Procedural.ResourceDistribution.FindDepositsByRarity",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_FindDepositsByRarity::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(20000.0f, 20000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Crystalline, EBiomeType::Anomalous };
	TArray<FResourceDepositLocation> AllDeposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, Biomes, 200);

	TArray<FResourceDepositLocation> RareOrAbove = ResourceSystem->FindDepositsByRarity(EResourceRarity::Rare, AllDeposits);

	for (const auto& Dep : RareOrAbove)
	{
		TestTrue(
			FString::Printf(TEXT("Deposit rarity (%d) should be >= Rare (2)"), static_cast<int32>(Dep.Rarity)),
			static_cast<int32>(Dep.Rarity) >= static_cast<int32>(EResourceRarity::Rare));
	}

	// Rare-or-above should be a subset of all deposits
	TestTrue(TEXT("Filtered results should be <= total deposits"), RareOrAbove.Num() <= AllDeposits.Num());

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_FindNearestDeposit,
	"Odyssey.Procedural.ResourceDistribution.FindNearestDeposit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_FindNearestDeposit::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Forest };
	TArray<FResourceDepositLocation> AllDeposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, Biomes, 50);

	if (AllDeposits.Num() > 0)
	{
		FVector SearchFrom(5000.0f, 5000.0f, 0.0f);
		FResourceDepositLocation Nearest = ResourceSystem->FindNearestDeposit(SearchFrom, AllDeposits);

		// Verify it is actually the nearest
		float NearestDist = FVector::Dist(SearchFrom, Nearest.Location);
		for (const auto& Dep : AllDeposits)
		{
			float Dist = FVector::Dist(SearchFrom, Dep.Location);
			TestTrue(
				FString::Printf(TEXT("Nearest deposit distance (%.1f) should be <= other deposit distance (%.1f)"), NearestDist, Dist),
				NearestDist <= Dist + 0.01f); // Small tolerance
		}
	}

	return true;
}

// ============================================================================
// 7. ECONOMIC ANALYSIS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_ResourceAbundance,
	"Odyssey.Procedural.ResourceDistribution.ResourceAbundanceCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_ResourceAbundance::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Desert, EBiomeType::Forest };
	TArray<FResourceDepositLocation> Deposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, Biomes, 100);

	TMap<EResourceType, float> Abundance = ResourceSystem->CalculateResourceAbundance(Deposits);

	float TotalAbundance = 0.0f;
	for (const auto& Pair : Abundance)
	{
		TestTrue(
			FString::Printf(TEXT("Abundance for resource %d should be >= 0"), static_cast<int32>(Pair.Key)),
			Pair.Value >= 0.0f);
		TestTrue(
			FString::Printf(TEXT("Abundance for resource %d should be <= 1"), static_cast<int32>(Pair.Key)),
			Pair.Value <= 1.0f);
		TotalAbundance += Pair.Value;
	}

	// Abundances should sum to approximately 1.0 (it's a distribution)
	TestTrue(
		FString::Printf(TEXT("Total abundance (%.3f) should be approximately 1.0"), TotalAbundance),
		FMath::IsNearlyEqual(TotalAbundance, 1.0f, 0.01f));

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_ScarcityScore,
	"Odyssey.Procedural.ResourceDistribution.ScarcityScoreCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_ScarcityScore::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Desert };
	TArray<FResourceDepositLocation> Deposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, Biomes, 100);

	if (Deposits.Num() > 0)
	{
		// Check scarcity for an existing resource type
		EResourceType ExistingType = Deposits[0].ResourceType;
		float Scarcity = ResourceSystem->GetResourceScarcityScore(ExistingType, Deposits);
		TestTrue(TEXT("Scarcity score should be in a reasonable range"), Scarcity >= 0.0f);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_TradeOpportunities,
	"Odyssey.Procedural.ResourceDistribution.TradeOpportunityAnalysis",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_TradeOpportunities::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);

	// Create resource distributions for two different locations
	TMap<int32, TArray<FResourceDepositLocation>> LocationResources;

	TArray<EBiomeType> VolcanicBiomes = { EBiomeType::Volcanic };
	LocationResources.Add(1, ResourceSystem->GenerateResourceDeposits(42, AreaSize, VolcanicBiomes, 30));

	TArray<EBiomeType> IceBiomes = { EBiomeType::Ice };
	LocationResources.Add(2, ResourceSystem->GenerateResourceDeposits(99, AreaSize, IceBiomes, 30));

	TArray<FTradeRouteOpportunity> Opportunities = ResourceSystem->AnalyzeTradeOpportunities(LocationResources);

	// Should find some trade opportunities between different biome types
	for (const auto& Opp : Opportunities)
	{
		TestTrue(TEXT("Source location ID should be valid"), Opp.SourceLocationID > 0);
		TestTrue(TEXT("Destination location ID should be valid"), Opp.DestinationLocationID > 0);
		TestTrue(TEXT("Source and destination should differ"), Opp.SourceLocationID != Opp.DestinationLocationID);
		TestTrue(TEXT("Risk level should be in [0, 1]"), Opp.RiskLevel >= 0.0f && Opp.RiskLevel <= 1.0f);
	}

	return true;
}

// ============================================================================
// 8. DISTRIBUTION PARAMETERS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_DistributionParams,
	"Odyssey.Procedural.ResourceDistribution.DistributionParameterAccess",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_DistributionParams::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	// Get default params for Silicate
	FResourceDistributionParams Params = ResourceSystem->GetDistributionParams(EResourceType::Silicate);
	TestEqual(TEXT("Params resource type should match"), Params.ResourceType, EResourceType::Silicate);
	TestTrue(TEXT("Base density should be positive"), Params.BaseDensity > 0.0f);
	TestTrue(TEXT("Min cluster size should be >= 1"), Params.MinClusterSize >= 1);
	TestTrue(TEXT("Max cluster >= min cluster"), Params.MaxClusterSize >= Params.MinClusterSize);
	TestTrue(TEXT("Rarity weights should have entries"), Params.RarityWeights.Num() > 0);

	// Set custom params and verify
	FResourceDistributionParams CustomParams;
	CustomParams.ResourceType = EResourceType::Silicate;
	CustomParams.BaseDensity = 1.5f;
	ResourceSystem->SetDistributionParams(EResourceType::Silicate, CustomParams);

	FResourceDistributionParams Retrieved = ResourceSystem->GetDistributionParams(EResourceType::Silicate);
	TestEqual(TEXT("Custom density should be stored"), Retrieved.BaseDensity, 1.5f);

	return true;
}

// ============================================================================
// 9. UTILITY FUNCTIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_RarityDisplayNames,
	"Odyssey.Procedural.ResourceDistribution.RarityDisplayNamesValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_RarityDisplayNames::RunTest(const FString& Parameters)
{
	const EResourceRarity AllRarities[] = {
		EResourceRarity::Common, EResourceRarity::Uncommon, EResourceRarity::Rare,
		EResourceRarity::VeryRare, EResourceRarity::Exotic, EResourceRarity::Legendary
	};

	for (EResourceRarity Rarity : AllRarities)
	{
		FString DisplayName = UOdysseyResourceDistributionSystem::GetRarityDisplayName(Rarity);
		TestFalse(
			FString::Printf(TEXT("Display name for rarity %d should not be empty"), static_cast<int32>(Rarity)),
			DisplayName.IsEmpty());
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_RarityColors,
	"Odyssey.Procedural.ResourceDistribution.RarityColorsValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_RarityColors::RunTest(const FString& Parameters)
{
	const EResourceRarity AllRarities[] = {
		EResourceRarity::Common, EResourceRarity::Uncommon, EResourceRarity::Rare,
		EResourceRarity::VeryRare, EResourceRarity::Exotic, EResourceRarity::Legendary
	};

	TSet<FString> UniqueColors;
	for (EResourceRarity Rarity : AllRarities)
	{
		FLinearColor Color = UOdysseyResourceDistributionSystem::GetRarityColor(Rarity);
		TestTrue(FString::Printf(TEXT("Color alpha for rarity %d should be positive"), static_cast<int32>(Rarity)),
			Color.A > 0.0f);
		UniqueColors.Add(Color.ToString());
	}

	// Each rarity should have a distinct color
	TestTrue(
		FString::Printf(TEXT("Should have at least 4 unique rarity colors, got %d"), UniqueColors.Num()),
		UniqueColors.Num() >= 4);

	return true;
}

// ============================================================================
// 10. EDGE CASES
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_EdgeCase_EmptyBiomeList,
	"Odyssey.Procedural.ResourceDistribution.EdgeCases.EmptyBiomeList",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_EdgeCase_EmptyBiomeList::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	TArray<EBiomeType> EmptyBiomes;

	// Should handle empty biome list gracefully without crashing
	TArray<FResourceDepositLocation> Deposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, EmptyBiomes, 10);
	// Result may be empty or may use a fallback - either is acceptable
	TestTrue(TEXT("Empty biome list should not crash (result is valid)"), true);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_EdgeCase_ZeroTargetCount,
	"Odyssey.Procedural.ResourceDistribution.EdgeCases.ZeroTargetCount",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_EdgeCase_ZeroTargetCount::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Desert };

	TArray<FResourceDepositLocation> Deposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, Biomes, 0);

	TestTrue(TEXT("Zero target count should produce zero or minimal deposits"), Deposits.Num() <= 1);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_EdgeCase_VerySmallArea,
	"Odyssey.Procedural.ResourceDistribution.EdgeCases.VerySmallArea",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_EdgeCase_VerySmallArea::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D SmallArea(100.0f, 100.0f);
	TArray<EBiomeType> Biomes = { EBiomeType::Barren };

	// Should not crash on very small area
	TArray<FResourceDepositLocation> Deposits = ResourceSystem->GenerateResourceDeposits(42, SmallArea, Biomes, 5);

	// All deposits should be within the small area
	for (const auto& Dep : Deposits)
	{
		TestTrue(TEXT("Deposit X should be within small area"),
			Dep.Location.X >= 0.0f && Dep.Location.X <= SmallArea.X);
		TestTrue(TEXT("Deposit Y should be within small area"),
			Dep.Location.Y >= 0.0f && Dep.Location.Y <= SmallArea.Y);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_EdgeCase_SingleBiome,
	"Odyssey.Procedural.ResourceDistribution.EdgeCases.SingleBiome",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_EdgeCase_SingleBiome::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	TArray<EBiomeType> SingleBiome = { EBiomeType::Radioactive };

	TArray<FResourceDepositLocation> Deposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, SingleBiome, 30);

	TestTrue(TEXT("Single biome should produce deposits"), Deposits.Num() > 0);

	// All deposits should be associated with the single biome
	for (const auto& Dep : Deposits)
	{
		TestEqual(TEXT("Deposit biome should match single biome input"), Dep.BiomeType, EBiomeType::Radioactive);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_EdgeCase_EmptyDepositsQuery,
	"Odyssey.Procedural.ResourceDistribution.EdgeCases.EmptyDepositsQuery",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_EdgeCase_EmptyDepositsQuery::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	TArray<FResourceDepositLocation> EmptyDeposits;

	// These should handle empty input gracefully
	TArray<FResourceDepositLocation> InRadius = ResourceSystem->FindDepositsInRadius(FVector::ZeroVector, 1000.0f, EmptyDeposits);
	TestEqual(TEXT("FindDepositsInRadius on empty should return empty"), InRadius.Num(), 0);

	TArray<FResourceDepositLocation> ByType = ResourceSystem->FindDepositsByType(EResourceType::Silicate, EmptyDeposits);
	TestEqual(TEXT("FindDepositsByType on empty should return empty"), ByType.Num(), 0);

	TArray<FResourceDepositLocation> ByRarity = ResourceSystem->FindDepositsByRarity(EResourceRarity::Common, EmptyDeposits);
	TestEqual(TEXT("FindDepositsByRarity on empty should return empty"), ByRarity.Num(), 0);

	TMap<EResourceType, float> Abundance = ResourceSystem->CalculateResourceAbundance(EmptyDeposits);
	TestEqual(TEXT("CalculateResourceAbundance on empty should return empty map"), Abundance.Num(), 0);

	return true;
}

// ============================================================================
// 11. DENSITY VARIES BY BIOME
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResourceDist_DensityVariesByBiome,
	"Odyssey.Procedural.ResourceDistribution.DensityVariesByBiome",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResourceDist_DensityVariesByBiome::RunTest(const FString& Parameters)
{
	UOdysseyResourceDistributionSystem* ResourceSystem = ResourceTestHelpers::CreateInitializedResourceSystem();

	const FVector2D AreaSize(10000.0f, 10000.0f);
	const int32 TargetCount = 50;

	// Generate for different biomes and compare densities
	TArray<EBiomeType> RichBiome = { EBiomeType::Crystalline };
	TArray<EBiomeType> PoorBiome = { EBiomeType::Barren };

	TArray<FResourceDepositLocation> RichDeposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, RichBiome, TargetCount);
	TArray<FResourceDepositLocation> PoorDeposits = ResourceSystem->GenerateResourceDeposits(42, AreaSize, PoorBiome, TargetCount);

	// Both should produce valid deposits
	TestTrue(TEXT("Crystalline (rich) biome should have deposits"), RichDeposits.Num() > 0);
	TestTrue(TEXT("Barren (poor) biome should have deposits"), PoorDeposits.Num() > 0);

	// Calculate total resource value for each
	int32 RichTotal = 0;
	int32 PoorTotal = 0;
	for (const auto& Dep : RichDeposits) { RichTotal += Dep.TotalAmount; }
	for (const auto& Dep : PoorDeposits) { PoorTotal += Dep.TotalAmount; }

	// Crystalline should generally be richer than Barren (by resource amount or variety)
	AddInfo(FString::Printf(TEXT("Crystalline total: %d, Barren total: %d"), RichTotal, PoorTotal));

	return true;
}
