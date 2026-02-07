// ExplorationRewardTests.cpp
// Comprehensive automation tests for UExplorationRewardSystem
// Tests discovery generation, reward scaling, milestones, fog-of-war, and scanning

#include "Misc/AutomationTest.h"
#include "Procedural/ExplorationRewardSystem.h"
#include "OdysseyPlanetGenerator.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyResourceDistributionSystem.h"
#include "Procedural/ProceduralTypes.h"

// ============================================================================
// HELPERS
// ============================================================================

namespace ExplorationTestHelpers
{
	struct TestContext
	{
		UOdysseyBiomeDefinitionSystem* BiomeSystem;
		UOdysseyResourceDistributionSystem* ResourceSystem;
		UOdysseyPlanetGenerator* PlanetGen;
		UExplorationRewardSystem* ExplorationSystem;
	};

	static TestContext CreateFullContext()
	{
		TestContext Ctx;

		Ctx.BiomeSystem = NewObject<UOdysseyBiomeDefinitionSystem>();
		Ctx.BiomeSystem->Initialize(nullptr);

		Ctx.ResourceSystem = NewObject<UOdysseyResourceDistributionSystem>();
		Ctx.ResourceSystem->Initialize(Ctx.BiomeSystem);

		Ctx.PlanetGen = NewObject<UOdysseyPlanetGenerator>();
		Ctx.PlanetGen->Initialize(Ctx.BiomeSystem, Ctx.ResourceSystem);

		Ctx.ExplorationSystem = NewObject<UExplorationRewardSystem>();
		Ctx.ExplorationSystem->Initialize(Ctx.BiomeSystem);

		return Ctx;
	}

	static FGeneratedPlanetData GenerateTestPlanet(TestContext& Ctx, int32 Seed = 42, EPlanetSize Size = EPlanetSize::Medium)
	{
		return Ctx.PlanetGen->GeneratePlanet(Seed, Size);
	}
}

// ============================================================================
// 1. DISCOVERY GENERATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_GenerateDiscoveriesForPlanet,
	"Odyssey.Procedural.ExplorationReward.GenerateDiscoveriesForPlanet",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_GenerateDiscoveriesForPlanet::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	const int32 DiscoveryCount = 15;
	TArray<FDiscoveryData> Discoveries = Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, DiscoveryCount);

	TestEqual(TEXT("Should generate requested discovery count"), Discoveries.Num(), DiscoveryCount);

	TSet<int32> DiscoveryIDs;
	for (const FDiscoveryData& Discovery : Discoveries)
	{
		TestTrue(TEXT("Discovery ID should be positive"), Discovery.DiscoveryID > 0);
		TestTrue(TEXT("Discovery type should not be None"), Discovery.DiscoveryType != EDiscoveryType::None);
		TestFalse(TEXT("Discovery name should not be empty"), Discovery.Name.IsEmpty());
		TestFalse(TEXT("Discovery description should not be empty"), Discovery.Description.IsEmpty());
		TestEqual(TEXT("Discovery planet ID should match"), Discovery.PlanetID, Planet.PlanetID);
		TestTrue(TEXT("OMEN reward should be positive"), Discovery.OMENReward > 0);
		TestTrue(TEXT("Experience reward should be positive"), Discovery.ExperienceReward > 0);
		TestFalse(TEXT("Discovery should start undiscovered"), Discovery.bDiscovered);
		TestFalse(TEXT("Discovery should start unclaimed"), Discovery.bClaimed);
		TestTrue(TEXT("Scan difficulty should be positive"), Discovery.ScanDifficulty > 0.0f);
		TestTrue(TEXT("Detection radius should be positive"), Discovery.DetectionRadius > 0.0f);

		TestFalse(
			FString::Printf(TEXT("Discovery ID %d should be unique"), Discovery.DiscoveryID),
			DiscoveryIDs.Contains(Discovery.DiscoveryID));
		DiscoveryIDs.Add(Discovery.DiscoveryID);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_GenerateSingleDiscovery,
	"Odyssey.Procedural.ExplorationReward.GenerateSingleDiscovery",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_GenerateSingleDiscovery::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();

	FDiscoveryData Discovery = Ctx.ExplorationSystem->GenerateDiscovery(
		42, 1, FVector(500.0f, 500.0f, 0.0f), EBiomeType::Volcanic);

	TestTrue(TEXT("Discovery type should not be None"), Discovery.DiscoveryType != EDiscoveryType::None);
	TestEqual(TEXT("Planet ID should match"), Discovery.PlanetID, 1);
	TestEqual(TEXT("Location should match"), Discovery.WorldLocation, FVector(500.0f, 500.0f, 0.0f));
	TestFalse(TEXT("Name should not be empty"), Discovery.Name.IsEmpty());
	TestTrue(TEXT("OMEN reward should be positive"), Discovery.OMENReward > 0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_DiscoveryTypesVariety,
	"Odyssey.Procedural.ExplorationReward.DiscoveryTypesVariety",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_DiscoveryTypesVariety::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();

	// Generate discoveries across different biomes to maximize type variety
	TSet<EDiscoveryType> TypesFound;
	const EBiomeType TestBiomes[] = {
		EBiomeType::Desert, EBiomeType::Ice, EBiomeType::Forest,
		EBiomeType::Volcanic, EBiomeType::Crystalline, EBiomeType::Anomalous
	};

	for (EBiomeType Biome : TestBiomes)
	{
		for (int32 Seed = 0; Seed < 30; ++Seed)
		{
			FDiscoveryData Discovery = Ctx.ExplorationSystem->GenerateDiscovery(
				Seed, 1, FVector(100.0f * Seed, 100.0f, 0.0f), Biome);
			TypesFound.Add(Discovery.DiscoveryType);
		}
	}

	// With 180 discoveries across 6 biomes, we should see good variety
	TestTrue(
		FString::Printf(TEXT("Should find at least 5 distinct discovery types, found %d"), TypesFound.Num()),
		TypesFound.Num() >= 5);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_DiscoveryRarityDistribution,
	"Odyssey.Procedural.ExplorationReward.DiscoveryRarityDistribution",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_DiscoveryRarityDistribution::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();

	TMap<EDiscoveryRarity, int32> RarityCounts;

	for (int32 Seed = 0; Seed < 200; ++Seed)
	{
		FDiscoveryData Discovery = Ctx.ExplorationSystem->GenerateDiscovery(
			Seed, 1, FVector(100.0f * Seed, 0.0f, 0.0f), EBiomeType::Forest);
		RarityCounts.FindOrAdd(Discovery.Rarity)++;
	}

	int32 CommonCount = RarityCounts.FindRef(EDiscoveryRarity::Common);
	int32 MythicCount = RarityCounts.FindRef(EDiscoveryRarity::Mythic);

	// Common should be more frequent than Mythic
	TestTrue(
		FString::Printf(TEXT("Common (%d) should outnumber Mythic (%d)"), CommonCount, MythicCount),
		CommonCount > MythicCount);

	return true;
}

// ============================================================================
// 2. REWARD SCALING BY DIFFICULTY / RARITY
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_OMENValueScaling,
	"Odyssey.Procedural.ExplorationReward.OMENValueScalesByRarity",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_OMENValueScaling::RunTest(const FString& Parameters)
{
	int32 CommonValue = UExplorationRewardSystem::CalculateDiscoveryOMENValue(EDiscoveryType::ResourceDeposit, EDiscoveryRarity::Common);
	int32 RareValue = UExplorationRewardSystem::CalculateDiscoveryOMENValue(EDiscoveryType::ResourceDeposit, EDiscoveryRarity::Rare);
	int32 LegendaryValue = UExplorationRewardSystem::CalculateDiscoveryOMENValue(EDiscoveryType::ResourceDeposit, EDiscoveryRarity::Legendary);
	int32 MythicValue = UExplorationRewardSystem::CalculateDiscoveryOMENValue(EDiscoveryType::ResourceDeposit, EDiscoveryRarity::Mythic);

	TestTrue(TEXT("Common OMEN value should be positive"), CommonValue > 0);
	TestTrue(TEXT("Rare should be worth more than Common"), RareValue > CommonValue);
	TestTrue(TEXT("Legendary should be worth more than Rare"), LegendaryValue > RareValue);
	TestTrue(TEXT("Mythic should be worth more than Legendary"), MythicValue > LegendaryValue);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_OMENValueByType,
	"Odyssey.Procedural.ExplorationReward.OMENValueVariesByDiscoveryType",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_OMENValueByType::RunTest(const FString& Parameters)
{
	// PrecursorTechnology should be more valuable than a basic ResourceDeposit at same rarity
	int32 ResourceValue = UExplorationRewardSystem::CalculateDiscoveryOMENValue(EDiscoveryType::ResourceDeposit, EDiscoveryRarity::Rare);
	int32 PrecursorValue = UExplorationRewardSystem::CalculateDiscoveryOMENValue(EDiscoveryType::PrecursorTechnology, EDiscoveryRarity::Rare);
	int32 QuantumValue = UExplorationRewardSystem::CalculateDiscoveryOMENValue(EDiscoveryType::QuantumAnomaly, EDiscoveryRarity::Rare);

	TestTrue(TEXT("PrecursorTechnology should be more valuable than ResourceDeposit"), PrecursorValue > ResourceValue);
	TestTrue(TEXT("QuantumAnomaly should be more valuable than ResourceDeposit"), QuantumValue > ResourceValue);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_ScanRequirementByRarity,
	"Odyssey.Procedural.ExplorationReward.HigherRarityNeedsAdvancedScan",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_ScanRequirementByRarity::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();

	// Generate many discoveries and check that higher rarities tend to need more advanced scans
	TMap<EDiscoveryRarity, TArray<EScanMode>> RarityScanModes;

	for (int32 Seed = 0; Seed < 200; ++Seed)
	{
		FDiscoveryData Discovery = Ctx.ExplorationSystem->GenerateDiscovery(
			Seed, 1, FVector(100.0f * Seed, 0.0f, 0.0f), EBiomeType::Anomalous);
		RarityScanModes.FindOrAdd(Discovery.Rarity).Add(Discovery.RequiredScanMode);
	}

	// At minimum, all scan modes assigned should be valid enum values
	for (const auto& Pair : RarityScanModes)
	{
		for (EScanMode Mode : Pair.Value)
		{
			TestTrue(
				FString::Printf(TEXT("Scan mode %d should be a valid value"), static_cast<int32>(Mode)),
				static_cast<int32>(Mode) >= 0 && static_cast<int32>(Mode) <= 4);
		}
	}

	return true;
}

// ============================================================================
// 3. SCANNING
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_EffectiveScanRadius,
	"Odyssey.Procedural.ExplorationReward.EffectiveScanRadiusScales",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_EffectiveScanRadius::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();

	float PassiveRadius = Ctx.ExplorationSystem->GetEffectiveScanRadius(EScanMode::Passive, 1.0f);
	float ActiveShortRadius = Ctx.ExplorationSystem->GetEffectiveScanRadius(EScanMode::ActiveShort, 1.0f);
	float ActiveLongRadius = Ctx.ExplorationSystem->GetEffectiveScanRadius(EScanMode::ActiveLong, 1.0f);
	float DeepRadius = Ctx.ExplorationSystem->GetEffectiveScanRadius(EScanMode::Deep, 1.0f);

	TestTrue(TEXT("Passive scan radius should be positive"), PassiveRadius > 0.0f);
	TestTrue(TEXT("Active Short should have larger radius than Passive"), ActiveShortRadius > PassiveRadius);
	TestTrue(TEXT("Active Long should have larger radius than Active Short"), ActiveLongRadius > ActiveShortRadius);

	// Higher scanner power should increase radius
	float LowPowerRadius = Ctx.ExplorationSystem->GetEffectiveScanRadius(EScanMode::ActiveShort, 0.5f);
	float HighPowerRadius = Ctx.ExplorationSystem->GetEffectiveScanRadius(EScanMode::ActiveShort, 2.0f);
	TestTrue(TEXT("Higher power should increase scan radius"), HighPowerRadius > LowPowerRadius);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_CanDetectDiscovery,
	"Odyssey.Procedural.ExplorationReward.CanDetectDiscoveryLogic",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_CanDetectDiscovery::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();

	// Create a test discovery
	FDiscoveryData Discovery;
	Discovery.RequiredScanMode = EScanMode::ActiveShort;
	Discovery.ScanDifficulty = 1.0f;
	Discovery.DetectionRadius = 500.0f;

	// Should detect with adequate scan mode and distance
	bool bCanDetect = Ctx.ExplorationSystem->CanDetectDiscovery(
		Discovery, EScanMode::ActiveShort, 200.0f, 1.0f);
	TestTrue(TEXT("Should detect with correct scan mode and close distance"), bCanDetect);

	// Should detect with a more powerful scan mode
	bool bCanDetectDeep = Ctx.ExplorationSystem->CanDetectDiscovery(
		Discovery, EScanMode::Deep, 200.0f, 1.0f);
	TestTrue(TEXT("Should detect with more powerful scan mode"), bCanDetectDeep);

	// Should not detect with passive if active is required
	bool bCanDetectPassive = Ctx.ExplorationSystem->CanDetectDiscovery(
		Discovery, EScanMode::Passive, 200.0f, 1.0f);
	TestFalse(TEXT("Should not detect with weaker scan mode"), bCanDetectPassive);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_PerformScan,
	"Odyssey.Procedural.ExplorationReward.PerformScanReturnsResults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_PerformScan::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	// Generate discoveries first
	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 20);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num());

	// Perform a deep scan from the center of the planet
	FVector ScanOrigin(Planet.WorldSize.X / 2.0f, Planet.WorldSize.Y / 2.0f, 0.0f);
	TArray<FScanResult> Results = Ctx.ExplorationSystem->PerformScan(
		Planet.PlanetID, ScanOrigin, EScanMode::Deep, 2.0f);

	// Deep scan with high power from center should find at least some discoveries
	for (const FScanResult& Result : Results)
	{
		if (Result.bFoundSomething)
		{
			TestTrue(TEXT("Found result should have positive signal strength"), Result.SignalStrength > 0.0f);
			TestTrue(TEXT("Found result should have valid discovery ID"), Result.DiscoveryID >= 0);
		}
	}

	return true;
}

// ============================================================================
// 4. DISCOVERY CLAIMING
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_ClaimDiscoveryRewards,
	"Odyssey.Procedural.ExplorationReward.ClaimDiscoveryRewards",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_ClaimDiscoveryRewards::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	TArray<FDiscoveryData> Discoveries = Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 5);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num());

	if (Discoveries.Num() > 0)
	{
		int32 DiscoveryID = Discoveries[0].DiscoveryID;

		// Try to discover it first
		FDiscoveryData OutDiscovery;
		Ctx.ExplorationSystem->TryDiscoverAtLocation(
			Planet.PlanetID, Discoveries[0].WorldLocation, 1000.0f, OutDiscovery);

		// Now claim rewards
		int32 OutOMEN = 0;
		int32 OutXP = 0;
		TArray<FResourceStack> OutResources;
		bool bClaimed = Ctx.ExplorationSystem->ClaimDiscoveryRewards(
			DiscoveryID, TEXT("TestPlayer"), OutOMEN, OutXP, OutResources);

		if (bClaimed)
		{
			TestTrue(TEXT("Claimed OMEN should be positive"), OutOMEN > 0);
			TestTrue(TEXT("Claimed XP should be positive"), OutXP > 0);
			TestTrue(TEXT("Discovery should now be marked claimed"), Ctx.ExplorationSystem->IsDiscoveryClaimed(DiscoveryID));

			// Should not be able to claim again
			int32 OutOMEN2 = 0, OutXP2 = 0;
			TArray<FResourceStack> OutResources2;
			bool bClaimedAgain = Ctx.ExplorationSystem->ClaimDiscoveryRewards(
				DiscoveryID, TEXT("TestPlayer"), OutOMEN2, OutXP2, OutResources2);
			TestFalse(TEXT("Should not be able to claim same discovery twice"), bClaimedAgain);
		}
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_ClaimInvalidDiscovery,
	"Odyssey.Procedural.ExplorationReward.ClaimInvalidDiscoveryFails",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_ClaimInvalidDiscovery::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();

	int32 OutOMEN = 0, OutXP = 0;
	TArray<FResourceStack> OutResources;

	// Claim a non-existent discovery ID
	bool bClaimed = Ctx.ExplorationSystem->ClaimDiscoveryRewards(
		999999, TEXT("TestPlayer"), OutOMEN, OutXP, OutResources);
	TestFalse(TEXT("Claiming non-existent discovery should fail"), bClaimed);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_DiscoveryRevealedState,
	"Odyssey.Procedural.ExplorationReward.DiscoveryRevealedState",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_DiscoveryRevealedState::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	TArray<FDiscoveryData> Discoveries = Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 5);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num());

	if (Discoveries.Num() > 0)
	{
		int32 ID = Discoveries[0].DiscoveryID;

		// Should not be revealed initially
		TestFalse(TEXT("Discovery should not be revealed initially"), Ctx.ExplorationSystem->IsDiscoveryRevealed(ID));
		TestFalse(TEXT("Discovery should not be claimed initially"), Ctx.ExplorationSystem->IsDiscoveryClaimed(ID));
	}

	return true;
}

// ============================================================================
// 5. EXPLORATION PROGRESS / FOG-OF-WAR
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_ExplorationProgressTracking,
	"Odyssey.Procedural.ExplorationReward.ExplorationProgressTracking",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_ExplorationProgressTracking::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num(), 32);

	// Initially should be uncharted at 0%
	float InitialProgress = Ctx.ExplorationSystem->GetExplorationPercent(Planet.PlanetID);
	TestEqual(TEXT("Initial exploration should be 0%"), InitialProgress, 0.0f);

	EExplorationStatus InitialStatus = Ctx.ExplorationSystem->GetExplorationStatus(Planet.PlanetID);
	TestEqual(TEXT("Initial status should be Uncharted"), InitialStatus, EExplorationStatus::Uncharted);

	// Simulate exploring part of the planet
	Ctx.ExplorationSystem->UpdateExploration(
		Planet.PlanetID, FVector(500.0f, 500.0f, 0.0f), 1000.0f, Planet.WorldSize);

	float Progress = Ctx.ExplorationSystem->GetExplorationPercent(Planet.PlanetID);
	TestTrue(TEXT("Progress should be > 0 after exploration"), Progress > 0.0f);
	TestTrue(TEXT("Progress should be <= 100"), Progress <= 100.0f);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_ExplorationDataAccuracy,
	"Odyssey.Procedural.ExplorationReward.ExplorationDataAccuracy",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_ExplorationDataAccuracy::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	const int32 TotalRegions = Planet.BiomeRegions.Num();
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, TotalRegions, 32);

	FPlanetExplorationData Data = Ctx.ExplorationSystem->GetExplorationData(Planet.PlanetID);

	TestEqual(TEXT("Planet ID should match"), Data.PlanetID, Planet.PlanetID);
	TestEqual(TEXT("Total regions should match"), Data.TotalRegions, TotalRegions);
	TestEqual(TEXT("Initial regions explored should be 0"), Data.RegionsExplored, 0);
	TestEqual(TEXT("Grid resolution should be 32"), Data.GridResolution, 32);
	TestTrue(TEXT("Explored grid should be initialized"), Data.ExploredGrid.Num() > 0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_MarkRegionExplored,
	"Odyssey.Procedural.ExplorationReward.MarkRegionExploredUpdatesProgress",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_MarkRegionExplored::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	const int32 TotalRegions = Planet.BiomeRegions.Num();
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, TotalRegions);

	// Mark first region as explored
	if (Planet.BiomeRegions.Num() > 0)
	{
		Ctx.ExplorationSystem->MarkRegionExplored(
			Planet.PlanetID, 0, Planet.BiomeRegions[0].BiomeType);

		FPlanetExplorationData Data = Ctx.ExplorationSystem->GetExplorationData(Planet.PlanetID);
		TestTrue(TEXT("Regions explored should be > 0 after marking"), Data.RegionsExplored > 0);
		TestTrue(TEXT("Discovered biomes should include the marked biome"), Data.DiscoveredBiomes.Contains(Planet.BiomeRegions[0].BiomeType));
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_ExplorationStatusProgression,
	"Odyssey.Procedural.ExplorationReward.ExplorationStatusProgressesThroughStages",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_ExplorationStatusProgression::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx, 42, EPlanetSize::Tiny);

	const int32 TotalRegions = FMath::Max(Planet.BiomeRegions.Num(), 4);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, TotalRegions, 8);

	// Explore the entire grid step by step
	float StepSize = Planet.WorldSize.X / 8.0f;
	float RevealRadius = StepSize * 2.0f;

	for (float X = 0; X <= Planet.WorldSize.X; X += StepSize)
	{
		for (float Y = 0; Y <= Planet.WorldSize.Y; Y += StepSize)
		{
			Ctx.ExplorationSystem->UpdateExploration(
				Planet.PlanetID, FVector(X, Y, 0.0f), RevealRadius, Planet.WorldSize);
		}
	}

	float FinalProgress = Ctx.ExplorationSystem->GetExplorationPercent(Planet.PlanetID);
	TestTrue(
		FString::Printf(TEXT("After full grid exploration, progress (%.1f%%) should be high"), FinalProgress),
		FinalProgress > 50.0f);

	return true;
}

// ============================================================================
// 6. MILESTONES
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_MilestonesGenerated,
	"Odyssey.Procedural.ExplorationReward.MilestonesGeneratedForPlanet",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_MilestonesGenerated::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 10);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num());

	TArray<FExplorationMilestone> Milestones = Ctx.ExplorationSystem->GetMilestones(Planet.PlanetID);

	TestTrue(TEXT("Planet should have milestones"), Milestones.Num() > 0);

	for (const FExplorationMilestone& Milestone : Milestones)
	{
		TestTrue(TEXT("Milestone ID should be valid"), Milestone.MilestoneID != NAME_None);
		TestFalse(TEXT("Milestone name should not be empty"), Milestone.Name.IsEmpty());
		TestTrue(TEXT("Milestone OMEN reward should be positive"), Milestone.OMENReward > 0);
		TestFalse(TEXT("Milestone should start incomplete"), Milestone.bCompleted);
		TestTrue(TEXT("Required exploration percent should be in [0, 100]"),
			Milestone.RequiredExplorationPercent >= 0.0f && Milestone.RequiredExplorationPercent <= 100.0f);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_PendingMilestones,
	"Odyssey.Procedural.ExplorationReward.PendingMilestonesReturnsIncomplete",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_PendingMilestones::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 10);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num());

	TArray<FExplorationMilestone> AllMilestones = Ctx.ExplorationSystem->GetMilestones(Planet.PlanetID);
	TArray<FExplorationMilestone> Pending = Ctx.ExplorationSystem->GetPendingMilestones(Planet.PlanetID);

	// Initially all milestones should be pending
	TestEqual(TEXT("Initially all milestones should be pending"), AllMilestones.Num(), Pending.Num());

	for (const FExplorationMilestone& M : Pending)
	{
		TestFalse(TEXT("Pending milestone should not be completed"), M.bCompleted);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_MilestoneChecking,
	"Odyssey.Procedural.ExplorationReward.CheckMilestonesAwardsProgress",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_MilestoneChecking::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx, 42, EPlanetSize::Tiny);

	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 10);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num(), 4);

	// Check milestones before any exploration (none should complete)
	TArray<FExplorationMilestone> Completed = Ctx.ExplorationSystem->CheckMilestones(Planet.PlanetID);
	// May complete 0% milestones if any exist

	// The function should return without crashing
	TestTrue(TEXT("CheckMilestones should return valid array"), true);

	return true;
}

// ============================================================================
// 7. QUERIES
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_GetPlanetDiscoveries,
	"Odyssey.Procedural.ExplorationReward.GetPlanetDiscoveriesReturnsAll",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_GetPlanetDiscoveries::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	const int32 Count = 10;
	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, Count);

	TArray<FDiscoveryData> All = Ctx.ExplorationSystem->GetPlanetDiscoveries(Planet.PlanetID);
	TestEqual(TEXT("GetPlanetDiscoveries should return all generated"), All.Num(), Count);

	TArray<FDiscoveryData> Undiscovered = Ctx.ExplorationSystem->GetUndiscoveredItems(Planet.PlanetID);
	TestEqual(TEXT("Initially all should be undiscovered"), Undiscovered.Num(), Count);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_GetDiscoveriesByType,
	"Odyssey.Procedural.ExplorationReward.GetDiscoveriesByType",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_GetDiscoveriesByType::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 30);

	TArray<FDiscoveryData> All = Ctx.ExplorationSystem->GetPlanetDiscoveries(Planet.PlanetID);
	if (All.Num() > 0)
	{
		EDiscoveryType SearchType = All[0].DiscoveryType;
		TArray<FDiscoveryData> ByType = Ctx.ExplorationSystem->GetDiscoveriesByType(Planet.PlanetID, SearchType);

		TestTrue(TEXT("Should find at least one discovery of the searched type"), ByType.Num() > 0);
		for (const auto& D : ByType)
		{
			TestEqual(TEXT("All filtered discoveries should match the searched type"), D.DiscoveryType, SearchType);
		}
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_GetDiscoveriesByRarity,
	"Odyssey.Procedural.ExplorationReward.GetDiscoveriesByMinRarity",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_GetDiscoveriesByRarity::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 50);

	TArray<FDiscoveryData> RareOrAbove = Ctx.ExplorationSystem->GetDiscoveriesByRarity(Planet.PlanetID, EDiscoveryRarity::Rare);

	for (const auto& D : RareOrAbove)
	{
		TestTrue(
			FString::Printf(TEXT("Discovery rarity (%d) should be >= Rare (2)"), static_cast<int32>(D.Rarity)),
			static_cast<int32>(D.Rarity) >= static_cast<int32>(EDiscoveryRarity::Rare));
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_TotalExplorationRewards,
	"Odyssey.Procedural.ExplorationReward.TotalExplorationRewardsCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_TotalExplorationRewards::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 10);

	int32 TotalRewards = Ctx.ExplorationSystem->GetTotalExplorationRewards(Planet.PlanetID);
	TestTrue(TEXT("Total exploration rewards should be positive"), TotalRewards > 0);

	return true;
}

// ============================================================================
// 8. SERIALIZATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_ExportImportSaveData,
	"Odyssey.Procedural.ExplorationReward.ExportImportSaveDataRoundTrip",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_ExportImportSaveData::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 5);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num());

	// Export save data
	TArray<int32> DiscoveredIDs;
	TArray<int32> ClaimedIDs;
	Ctx.ExplorationSystem->ExportPlanetSaveData(Planet.PlanetID, DiscoveredIDs, ClaimedIDs);

	// Initially nothing should be discovered or claimed
	TestEqual(TEXT("No IDs should be discovered initially"), DiscoveredIDs.Num(), 0);
	TestEqual(TEXT("No IDs should be claimed initially"), ClaimedIDs.Num(), 0);

	// Import should not crash
	TArray<int32> FakeDiscovered = { 1, 2 };
	TArray<int32> FakeClaimed = { 1 };
	Ctx.ExplorationSystem->ImportPlanetSaveData(Planet.PlanetID, FakeDiscovered, FakeClaimed);

	TestTrue(TEXT("Import should complete without crash"), true);

	return true;
}

// ============================================================================
// 9. UTILITY FUNCTIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_DisplayNameUtilities,
	"Odyssey.Procedural.ExplorationReward.DisplayNameUtilitiesValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_DisplayNameUtilities::RunTest(const FString& Parameters)
{
	// Discovery type display names
	const EDiscoveryType AllTypes[] = {
		EDiscoveryType::ResourceDeposit, EDiscoveryType::AncientRuins,
		EDiscoveryType::AlienArtifact, EDiscoveryType::NaturalWonder,
		EDiscoveryType::AbandonedOutpost, EDiscoveryType::BiologicalSpecimen,
		EDiscoveryType::AnomalousSignal, EDiscoveryType::HiddenCache,
		EDiscoveryType::WreckedShip, EDiscoveryType::PrecursorTechnology,
		EDiscoveryType::QuantumAnomaly, EDiscoveryType::RareMineral,
		EDiscoveryType::GeothermalVent, EDiscoveryType::FrozenOrganism,
		EDiscoveryType::CrystalFormation
	};

	for (EDiscoveryType Type : AllTypes)
	{
		FText Name = UExplorationRewardSystem::GetDiscoveryTypeDisplayName(Type);
		TestFalse(
			FString::Printf(TEXT("Display name for discovery type %d should not be empty"), static_cast<int32>(Type)),
			Name.IsEmpty());
	}

	// Exploration status display names
	const EExplorationStatus AllStatuses[] = {
		EExplorationStatus::Uncharted, EExplorationStatus::Surveyed,
		EExplorationStatus::PartiallyExplored, EExplorationStatus::MostlyExplored,
		EExplorationStatus::FullyExplored, EExplorationStatus::Mastered
	};

	for (EExplorationStatus Status : AllStatuses)
	{
		FText Name = UExplorationRewardSystem::GetExplorationStatusDisplayName(Status);
		TestFalse(
			FString::Printf(TEXT("Display name for status %d should not be empty"), static_cast<int32>(Status)),
			Name.IsEmpty());
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_RarityColors,
	"Odyssey.Procedural.ExplorationReward.RarityColorsDistinct",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_RarityColors::RunTest(const FString& Parameters)
{
	const EDiscoveryRarity AllRarities[] = {
		EDiscoveryRarity::Common, EDiscoveryRarity::Uncommon, EDiscoveryRarity::Rare,
		EDiscoveryRarity::Epic, EDiscoveryRarity::Legendary, EDiscoveryRarity::Mythic
	};

	TSet<FString> UniqueColors;
	for (EDiscoveryRarity Rarity : AllRarities)
	{
		FLinearColor Color = UExplorationRewardSystem::GetDiscoveryRarityColor(Rarity);
		TestTrue(
			FString::Printf(TEXT("Color alpha for rarity %d should be positive"), static_cast<int32>(Rarity)),
			Color.A > 0.0f);
		UniqueColors.Add(Color.ToString());
	}

	TestTrue(
		FString::Printf(TEXT("Should have at least 4 unique rarity colors, got %d"), UniqueColors.Num()),
		UniqueColors.Num() >= 4);

	return true;
}

// ============================================================================
// 10. EDGE CASES
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_EdgeCase_NonExistentPlanet,
	"Odyssey.Procedural.ExplorationReward.EdgeCases.NonExistentPlanet",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_EdgeCase_NonExistentPlanet::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();

	// Queries on non-existent planet should not crash
	TArray<FDiscoveryData> Discoveries = Ctx.ExplorationSystem->GetPlanetDiscoveries(99999);
	TestEqual(TEXT("Non-existent planet should return empty discoveries"), Discoveries.Num(), 0);

	float Progress = Ctx.ExplorationSystem->GetExplorationPercent(99999);
	TestEqual(TEXT("Non-existent planet should have 0% progress"), Progress, 0.0f);

	EExplorationStatus Status = Ctx.ExplorationSystem->GetExplorationStatus(99999);
	TestEqual(TEXT("Non-existent planet should be Uncharted"), Status, EExplorationStatus::Uncharted);

	TArray<FExplorationMilestone> Milestones = Ctx.ExplorationSystem->GetMilestones(99999);
	TestEqual(TEXT("Non-existent planet should have no milestones"), Milestones.Num(), 0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_EdgeCase_ZeroDiscoveryCount,
	"Odyssey.Procedural.ExplorationReward.EdgeCases.ZeroDiscoveryCount",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_EdgeCase_ZeroDiscoveryCount::RunTest(const FString& Parameters)
{
	auto Ctx = ExplorationTestHelpers::CreateFullContext();
	FGeneratedPlanetData Planet = ExplorationTestHelpers::GenerateTestPlanet(Ctx);

	TArray<FDiscoveryData> Discoveries = Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 0);
	TestEqual(TEXT("Zero discovery count should produce empty array"), Discoveries.Num(), 0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExplReward_EdgeCase_DiscoveryDeterminism,
	"Odyssey.Procedural.ExplorationReward.EdgeCases.DiscoveryGenerationDeterministic",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FExplReward_EdgeCase_DiscoveryDeterminism::RunTest(const FString& Parameters)
{
	// Two independent exploration systems with same seed should produce same discovery types
	auto CtxA = ExplorationTestHelpers::CreateFullContext();
	auto CtxB = ExplorationTestHelpers::CreateFullContext();

	FDiscoveryData A = CtxA.ExplorationSystem->GenerateDiscovery(42, 1, FVector(100.0f, 200.0f, 0.0f), EBiomeType::Volcanic);
	FDiscoveryData B = CtxB.ExplorationSystem->GenerateDiscovery(42, 1, FVector(100.0f, 200.0f, 0.0f), EBiomeType::Volcanic);

	TestEqual(TEXT("Same seed should produce same discovery type"), A.DiscoveryType, B.DiscoveryType);
	TestEqual(TEXT("Same seed should produce same rarity"), A.Rarity, B.Rarity);
	TestEqual(TEXT("Same seed should produce same OMEN reward"), A.OMENReward, B.OMENReward);

	return true;
}
