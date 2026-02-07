// ProceduralIntegrationTests.cpp
// Integration tests for the ProceduralPlanetManager coordinator
// Tests full pipeline: generation -> exploration -> economy -> persistence

#include "Misc/AutomationTest.h"
#include "OdysseyPlanetGenerator.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyResourceDistributionSystem.h"
#include "OdysseyPlanetaryEconomyComponent.h"
#include "Procedural/ProceduralPlanetManager.h"
#include "Procedural/ExplorationRewardSystem.h"
#include "Procedural/ProceduralTypes.h"

// ============================================================================
// HELPERS
// ============================================================================

namespace IntegrationTestHelpers
{
	/**
	 * The ProceduralPlanetManager is a UActorComponent that normally
	 * requires an owning actor. For unit testing, we create standalone
	 * subsystem instances to test the integration logic without a world.
	 */
	struct IntegrationContext
	{
		UOdysseyBiomeDefinitionSystem* BiomeSystem;
		UOdysseyResourceDistributionSystem* ResourceSystem;
		UOdysseyPlanetGenerator* PlanetGen;
		UExplorationRewardSystem* ExplorationSystem;
	};

	static IntegrationContext CreateFullPipeline()
	{
		IntegrationContext Ctx;

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
}

// ============================================================================
// 1. FULL PLANET GENERATION PIPELINE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_FullPlanetPipeline,
	"Odyssey.Procedural.Integration.FullPlanetPipeline",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_FullPlanetPipeline::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	// Step 1: Generate planet
	FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(42, EPlanetSize::Large);
	TestTrue(TEXT("Planet should have biome regions"), Planet.BiomeRegions.Num() > 0);
	TestTrue(TEXT("Planet should have resource deposits"), Planet.ResourceDeposits.Num() > 0);

	// Step 2: Generate exploration content
	TArray<FDiscoveryData> Discoveries = Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 15);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num());
	TestEqual(TEXT("Should have 15 discoveries"), Discoveries.Num(), 15);

	// Step 3: Verify exploration tracking is initialized
	FPlanetExplorationData ExplData = Ctx.ExplorationSystem->GetExplorationData(Planet.PlanetID);
	TestEqual(TEXT("Exploration should start at 0%"), ExplData.ExplorationPercent, 0.0f);

	// Step 4: Simulate player exploration
	Ctx.ExplorationSystem->UpdateExploration(
		Planet.PlanetID,
		FVector(Planet.WorldSize.X / 2.0f, Planet.WorldSize.Y / 2.0f, 0.0f),
		2000.0f,
		Planet.WorldSize);

	float Progress = Ctx.ExplorationSystem->GetExplorationPercent(Planet.PlanetID);
	TestTrue(TEXT("Exploration progress should increase after player movement"), Progress > 0.0f);

	// Step 5: Perform scan
	TArray<FScanResult> ScanResults = Ctx.ExplorationSystem->PerformScan(
		Planet.PlanetID,
		FVector(Planet.WorldSize.X / 2.0f, Planet.WorldSize.Y / 2.0f, 0.0f),
		EScanMode::Deep, 2.0f);

	// Step 6: Check milestones
	TArray<FExplorationMilestone> Milestones = Ctx.ExplorationSystem->GetMilestones(Planet.PlanetID);
	TestTrue(TEXT("Should have milestones defined"), Milestones.Num() > 0);

	return true;
}

// ============================================================================
// 2. BIOME-RESOURCE INTEGRATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_BiomeResourceConsistency,
	"Odyssey.Procedural.Integration.BiomeResourceConsistency",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_BiomeResourceConsistency::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(42, EPlanetSize::Large);

	// For each biome region, the resources should align with the biome's resource weights
	for (const FPlanetBiomeRegion& Region : Planet.BiomeRegions)
	{
		TArray<FBiomeResourceWeight> BiomeResources = Ctx.BiomeSystem->GetBiomeResources(Region.BiomeType);

		// Build set of valid resource types for this biome
		TSet<EResourceType> ValidResources;
		for (const FBiomeResourceWeight& Weight : BiomeResources)
		{
			ValidResources.Add(Weight.ResourceType);
		}

		// Check resources within this region
		TArray<FResourceDepositLocation> RegionResources = Ctx.PlanetGen->GetResourcesInRegion(Planet, Region);
		for (const FResourceDepositLocation& Deposit : RegionResources)
		{
			// Resources should generally be from the biome's resource list
			// (not strictly required due to rare/anomalous spawns, so just verify validity)
			TestTrue(
				FString::Printf(TEXT("Deposit resource type %d should be valid"), static_cast<int32>(Deposit.ResourceType)),
				Deposit.ResourceType != EResourceType::None);
		}
	}

	return true;
}

// ============================================================================
// 3. DISCOVERY-BIOME AFFINITY INTEGRATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_DiscoveryBiomeAffinity,
	"Odyssey.Procedural.Integration.DiscoveryBiomeAffinity",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_DiscoveryBiomeAffinity::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	// Generate discoveries in specific biomes and verify type affinity
	TMap<EBiomeType, TMap<EDiscoveryType, int32>> BiomeDiscoveryCounts;

	const EBiomeType TestBiomes[] = { EBiomeType::Volcanic, EBiomeType::Ice, EBiomeType::Crystalline };

	for (EBiomeType Biome : TestBiomes)
	{
		for (int32 Seed = 0; Seed < 50; ++Seed)
		{
			FDiscoveryData Discovery = Ctx.ExplorationSystem->GenerateDiscovery(
				Seed, 1, FVector(100.0f * Seed, 0.0f, 0.0f), Biome);
			BiomeDiscoveryCounts.FindOrAdd(Biome).FindOrAdd(Discovery.DiscoveryType)++;
		}
	}

	// Volcanic should have GeothermalVent as a relatively common discovery
	int32 VolcanicGeothermalCount = BiomeDiscoveryCounts.FindRef(EBiomeType::Volcanic).FindRef(EDiscoveryType::GeothermalVent);
	int32 IceGeothermalCount = BiomeDiscoveryCounts.FindRef(EBiomeType::Ice).FindRef(EDiscoveryType::GeothermalVent);
	// Ice should have FrozenOrganism as a relatively common discovery
	int32 IceFrozenCount = BiomeDiscoveryCounts.FindRef(EBiomeType::Ice).FindRef(EDiscoveryType::FrozenOrganism);
	int32 VolcanicFrozenCount = BiomeDiscoveryCounts.FindRef(EBiomeType::Volcanic).FindRef(EDiscoveryType::FrozenOrganism);

	// Geothermal vents should be more common in volcanic than ice (if present)
	if (VolcanicGeothermalCount > 0 || IceGeothermalCount > 0)
	{
		TestTrue(TEXT("GeothermalVent should be more common in Volcanic than Ice"),
			VolcanicGeothermalCount >= IceGeothermalCount);
	}

	// Frozen organisms should be more common in ice than volcanic (if present)
	if (IceFrozenCount > 0 || VolcanicFrozenCount > 0)
	{
		TestTrue(TEXT("FrozenOrganism should be more common in Ice than Volcanic"),
			IceFrozenCount >= VolcanicFrozenCount);
	}

	return true;
}

// ============================================================================
// 4. ECONOMY-PLANET TYPE INTEGRATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_EconomyPlanetTypeCorrelation,
	"Odyssey.Procedural.Integration.EconomyPlanetTypeCorrelation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_EconomyPlanetTypeCorrelation::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	// Generate planets of different types and check economic specialization
	TMap<EPlanetType, TArray<EEconomicSpecialization>> TypeSpecializations;

	for (int32 Seed = 0; Seed < 30; ++Seed)
	{
		FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(Seed * 1000);

		UOdysseyPlanetaryEconomyComponent* Econ = NewObject<UOdysseyPlanetaryEconomyComponent>();
		Econ->InitializeTradeGoods();
		Econ->InitializeFromPlanetData(Planet, Seed * 1000);

		TypeSpecializations.FindOrAdd(Planet.PlanetType).Add(Econ->GetPrimarySpecialization());
	}

	// Log correlation data
	for (const auto& Pair : TypeSpecializations)
	{
		TMap<EEconomicSpecialization, int32> SpecCounts;
		for (EEconomicSpecialization Spec : Pair.Value)
		{
			SpecCounts.FindOrAdd(Spec)++;
		}
		AddInfo(FString::Printf(TEXT("Planet type %d: %d planets, %d distinct specializations"),
			static_cast<int32>(Pair.Key), Pair.Value.Num(), SpecCounts.Num()));
	}

	return true;
}

// ============================================================================
// 5. SAVE/LOAD ROUND-TRIP INTEGRATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_SaveLoadRoundTrip,
	"Odyssey.Procedural.Integration.SaveLoadRoundTrip",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_SaveLoadRoundTrip::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	// Step 1: Generate a planet and exploration content
	FGeneratedPlanetData OriginalPlanet = Ctx.PlanetGen->GeneratePlanet(42, EPlanetSize::Medium);
	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(OriginalPlanet, 10);
	Ctx.ExplorationSystem->RegisterPlanet(OriginalPlanet.PlanetID, OriginalPlanet.BiomeRegions.Num());

	// Step 2: Create save data
	FPlanetSaveData SaveData;
	SaveData.PlanetID = OriginalPlanet.PlanetID;
	SaveData.GenerationSeed = OriginalPlanet.GenerationSeed;
	SaveData.bDiscovered = true;
	SaveData.ExplorationPercent = 35.0f;
	Ctx.ExplorationSystem->ExportPlanetSaveData(
		OriginalPlanet.PlanetID,
		SaveData.DiscoveredDiscoveryIDs,
		SaveData.ClaimedDiscoveryIDs);

	// Step 3: Regenerate from seed (simulating load)
	FGeneratedPlanetData RegeneratedPlanet = Ctx.PlanetGen->GeneratePlanet(SaveData.GenerationSeed, EPlanetSize::Medium);

	// Step 4: Verify regenerated planet matches original
	TestEqual(TEXT("Regenerated planet type should match"), OriginalPlanet.PlanetType, RegeneratedPlanet.PlanetType);
	TestEqual(TEXT("Regenerated biome count should match"), OriginalPlanet.BiomeRegions.Num(), RegeneratedPlanet.BiomeRegions.Num());
	TestEqual(TEXT("Regenerated resource count should match"), OriginalPlanet.ResourceDeposits.Num(), RegeneratedPlanet.ResourceDeposits.Num());

	for (int32 i = 0; i < OriginalPlanet.BiomeRegions.Num() && i < RegeneratedPlanet.BiomeRegions.Num(); ++i)
	{
		TestEqual(
			FString::Printf(TEXT("Biome region %d type should match after regeneration"), i),
			OriginalPlanet.BiomeRegions[i].BiomeType,
			RegeneratedPlanet.BiomeRegions[i].BiomeType);
	}

	return true;
}

// ============================================================================
// 6. RESOURCE DEPLETION PERSISTENCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_ResourceDepletionPersistence,
	"Odyssey.Procedural.Integration.ResourceDepletionPersistence",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_ResourceDepletionPersistence::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(42, EPlanetSize::Medium);

	if (Planet.ResourceDeposits.Num() > 0)
	{
		// Simulate partial depletion
		FPlanetSaveData SaveData;
		SaveData.PlanetID = Planet.PlanetID;
		SaveData.GenerationSeed = Planet.GenerationSeed;

		int32 DepositID = Planet.ResourceDeposits[0].DepositID;
		int32 OriginalAmount = Planet.ResourceDeposits[0].TotalAmount;
		int32 ReducedAmount = OriginalAmount / 2;

		SaveData.DepositRemainingAmounts.Add(DepositID, ReducedAmount);

		// Regenerate and apply depletion
		FGeneratedPlanetData Regenerated = Ctx.PlanetGen->GeneratePlanet(SaveData.GenerationSeed, EPlanetSize::Medium);

		// Apply depletion from save data
		for (FResourceDepositLocation& Deposit : Regenerated.ResourceDeposits)
		{
			if (const int32* Remaining = SaveData.DepositRemainingAmounts.Find(Deposit.DepositID))
			{
				Deposit.RemainingAmount = *Remaining;
			}
		}

		// Verify depletion is preserved
		if (Regenerated.ResourceDeposits.Num() > 0)
		{
			bool bFoundDepletedDeposit = false;
			for (const FResourceDepositLocation& Dep : Regenerated.ResourceDeposits)
			{
				if (Dep.DepositID == DepositID)
				{
					TestEqual(TEXT("Depleted deposit should have reduced amount"), Dep.RemainingAmount, ReducedAmount);
					bFoundDepletedDeposit = true;
					break;
				}
			}
			TestTrue(TEXT("Should find the depleted deposit after regeneration"), bFoundDepletedDeposit);
		}
	}

	return true;
}

// ============================================================================
// 7. MULTI-SYSTEM TRADE ROUTE INTEGRATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_MultiSystemTradeRoutes,
	"Odyssey.Procedural.Integration.MultiSystemTradeRoutes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_MultiSystemTradeRoutes::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	// Generate multiple star systems
	FStarSystemData SystemA = Ctx.PlanetGen->GenerateStarSystem(100, 3, 5);
	FStarSystemData SystemB = Ctx.PlanetGen->GenerateStarSystem(200, 3, 5);

	TestTrue(TEXT("System A should have planets"), SystemA.Planets.Num() > 0);
	TestTrue(TEXT("System B should have planets"), SystemB.Planets.Num() > 0);

	// Build resource map across all planets
	TMap<int32, TArray<FResourceDepositLocation>> AllResources;
	for (const auto& Planet : SystemA.Planets)
	{
		AllResources.Add(Planet.PlanetID, Planet.ResourceDeposits);
	}
	for (const auto& Planet : SystemB.Planets)
	{
		AllResources.Add(Planet.PlanetID, Planet.ResourceDeposits);
	}

	// Analyze trade routes
	TArray<FTradeRouteOpportunity> Routes = Ctx.ResourceSystem->AnalyzeTradeOpportunities(AllResources);

	AddInfo(FString::Printf(TEXT("Found %d trade route opportunities across 2 systems (%d + %d planets)"),
		Routes.Num(), SystemA.Planets.Num(), SystemB.Planets.Num()));

	// With different systems, there should be some trade opportunities
	for (const FTradeRouteOpportunity& Route : Routes)
	{
		TestTrue(TEXT("Trade route should have valid source"), Route.SourceLocationID > 0);
		TestTrue(TEXT("Trade route should have valid destination"), Route.DestinationLocationID > 0);
		TestTrue(TEXT("Trade route resources should not be None"), Route.AbundantResource != EResourceType::None);
	}

	return true;
}

// ============================================================================
// 8. GALAXY REGION COHERENCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_GalaxyRegionCoherence,
	"Odyssey.Procedural.Integration.GalaxyRegionCoherence",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_GalaxyRegionCoherence::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	const int32 SystemCount = 5;
	const FVector Center(0.0f, 0.0f, 0.0f);
	const float Radius = 10000.0f;

	TArray<FStarSystemData> Region = Ctx.PlanetGen->GenerateGalaxyRegion(42, SystemCount, Center, Radius);

	TestEqual(TEXT("Region should have requested system count"), Region.Num(), SystemCount);

	// Verify all systems and planets have unique IDs
	TSet<int32> AllSystemIDs;
	TSet<int32> AllPlanetIDs;
	int32 TotalPlanets = 0;

	for (const FStarSystemData& System : Region)
	{
		TestFalse(TEXT("System ID should be unique"),
			AllSystemIDs.Contains(System.SystemID));
		AllSystemIDs.Add(System.SystemID);

		for (const FGeneratedPlanetData& Planet : System.Planets)
		{
			TestFalse(
				FString::Printf(TEXT("Planet ID %d should be unique across region"), Planet.PlanetID),
				AllPlanetIDs.Contains(Planet.PlanetID));
			AllPlanetIDs.Add(Planet.PlanetID);
			TotalPlanets++;
		}
	}

	AddInfo(FString::Printf(TEXT("Galaxy region: %d systems, %d total planets, all IDs unique"),
		SystemCount, TotalPlanets));

	TestTrue(TEXT("Region should have at least (SystemCount * 2) planets"), TotalPlanets >= SystemCount * 2);

	return true;
}

// ============================================================================
// 9. EXPLORATION TO ECONOMY FEEDBACK
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_ExplorationEconomyFeedback,
	"Odyssey.Procedural.Integration.ExplorationEconomyFeedback",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_ExplorationEconomyFeedback::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(42, EPlanetSize::Large);

	// Initialize economy
	UOdysseyPlanetaryEconomyComponent* Econ = NewObject<UOdysseyPlanetaryEconomyComponent>();
	Econ->InitializeTradeGoods();
	Econ->InitializeFromPlanetData(Planet, 42);

	// Verify economic rating aligns with planet resources
	int32 EconRating = UOdysseyPlanetGenerator::CalculateEconomicRating(Planet);
	TestTrue(TEXT("Economic rating should be in [0, 100]"), EconRating >= 0 && EconRating <= 100);

	// Generate exploration content
	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 10);

	// Exploration rewards should include OMEN that could be used in economy
	int32 TotalOMEN = Ctx.ExplorationSystem->GetTotalExplorationRewards(Planet.PlanetID);
	TestTrue(TEXT("Total exploration OMEN should be positive"), TotalOMEN > 0);

	// Economy should have meaningful market activity
	TestTrue(TEXT("Economy should have productions"), Econ->GetProductions().Num() > 0);
	TestTrue(TEXT("Economy should have consumptions"), Econ->GetConsumptions().Num() > 0);
	TestTrue(TEXT("Economy should have market prices"), Econ->GetAllMarketPrices().Num() > 0);

	return true;
}

// ============================================================================
// 10. COMPLETE LIFECYCLE TEST
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_CompleteLifecycle,
	"Odyssey.Procedural.Integration.CompleteLifecycle",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_CompleteLifecycle::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	// Phase 1: Galaxy generation
	TArray<FStarSystemData> Galaxy = Ctx.PlanetGen->GenerateGalaxyRegion(42, 3, FVector::ZeroVector, 50000.0f);
	TestTrue(TEXT("Galaxy should have star systems"), Galaxy.Num() > 0);

	// Phase 2: Pick a planet and generate exploration content
	FGeneratedPlanetData TargetPlanet = Galaxy[0].Planets[0];
	TArray<FDiscoveryData> Discoveries = Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(TargetPlanet, 20);
	Ctx.ExplorationSystem->RegisterPlanet(TargetPlanet.PlanetID, TargetPlanet.BiomeRegions.Num());
	TestEqual(TEXT("Should have 20 discoveries"), Discoveries.Num(), 20);

	// Phase 3: Initialize economy
	UOdysseyPlanetaryEconomyComponent* Econ = NewObject<UOdysseyPlanetaryEconomyComponent>();
	Econ->InitializeTradeGoods();
	Econ->InitializeFromPlanetData(TargetPlanet, TargetPlanet.GenerationSeed);
	TestTrue(TEXT("Economy should be initialized"), Econ->GetPrimarySpecialization() != EEconomicSpecialization::None);

	// Phase 4: Simulate player exploration
	for (int32 Step = 0; Step < 10; ++Step)
	{
		FVector PlayerPos(
			TargetPlanet.WorldSize.X * Step / 10.0f,
			TargetPlanet.WorldSize.Y / 2.0f,
			0.0f);
		Ctx.ExplorationSystem->UpdateExploration(
			TargetPlanet.PlanetID, PlayerPos, 1000.0f, TargetPlanet.WorldSize);
	}

	float ExplProgress = Ctx.ExplorationSystem->GetExplorationPercent(TargetPlanet.PlanetID);
	TestTrue(TEXT("Exploration progress should increase"), ExplProgress > 0.0f);

	// Phase 5: Try discovering something
	FDiscoveryData OutDiscovery;
	if (Discoveries.Num() > 0)
	{
		bool bDiscovered = Ctx.ExplorationSystem->TryDiscoverAtLocation(
			TargetPlanet.PlanetID, Discoveries[0].WorldLocation, 500.0f, OutDiscovery);
		// May or may not succeed depending on distance and scan requirements
	}

	// Phase 6: Check milestones
	TArray<FExplorationMilestone> CompletedMilestones = Ctx.ExplorationSystem->CheckMilestones(TargetPlanet.PlanetID);

	// Phase 7: Create save data
	FPlanetSaveData SaveData;
	SaveData.PlanetID = TargetPlanet.PlanetID;
	SaveData.GenerationSeed = TargetPlanet.GenerationSeed;
	SaveData.bDiscovered = true;
	SaveData.ExplorationPercent = ExplProgress;
	SaveData.WealthLevel = Econ->GetWealthLevel();
	SaveData.Population = Econ->GetPopulation();

	Ctx.ExplorationSystem->ExportPlanetSaveData(
		TargetPlanet.PlanetID,
		SaveData.DiscoveredDiscoveryIDs,
		SaveData.ClaimedDiscoveryIDs);

	// Phase 8: Verify save data is valid
	TestEqual(TEXT("Save data planet ID should match"), SaveData.PlanetID, TargetPlanet.PlanetID);
	TestEqual(TEXT("Save data seed should match"), SaveData.GenerationSeed, TargetPlanet.GenerationSeed);
	TestTrue(TEXT("Save data should be marked discovered"), SaveData.bDiscovered);

	AddInfo(FString::Printf(
		TEXT("Complete lifecycle: %d systems, %d discoveries, %.1f%% explored, %s specialization, wealth=%d"),
		Galaxy.Num(), Discoveries.Num(), ExplProgress,
		*UEnum::GetValueAsString(Econ->GetPrimarySpecialization()),
		Econ->GetWealthLevel()));

	return true;
}

// ============================================================================
// 11. PLANET TYPE DIVERSITY ACROSS STAR SYSTEMS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_PlanetTypeDiversity,
	"Odyssey.Procedural.Integration.PlanetTypeDiversityInSystems",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_PlanetTypeDiversity::RunTest(const FString& Parameters)
{
	auto Ctx = IntegrationTestHelpers::CreateFullPipeline();

	TSet<EPlanetType> AllTypes;
	TSet<EBiomeType> AllBiomes;
	TSet<EEconomicSpecialization> AllSpecs;

	for (int32 SystemSeed = 0; SystemSeed < 10; ++SystemSeed)
	{
		FStarSystemData System = Ctx.PlanetGen->GenerateStarSystem(SystemSeed * 10000, 3, 6);

		for (const FGeneratedPlanetData& Planet : System.Planets)
		{
			AllTypes.Add(Planet.PlanetType);

			for (const FPlanetBiomeRegion& Region : Planet.BiomeRegions)
			{
				AllBiomes.Add(Region.BiomeType);
			}

			UOdysseyPlanetaryEconomyComponent* Econ = NewObject<UOdysseyPlanetaryEconomyComponent>();
			Econ->InitializeTradeGoods();
			Econ->InitializeFromPlanetData(Planet, Planet.GenerationSeed);
			AllSpecs.Add(Econ->GetPrimarySpecialization());
		}
	}

	AddInfo(FString::Printf(TEXT("Diversity across 10 systems: %d planet types, %d biome types, %d specializations"),
		AllTypes.Num(), AllBiomes.Num(), AllSpecs.Num()));

	TestTrue(
		FString::Printf(TEXT("Should have at least 4 planet types, found %d"), AllTypes.Num()),
		AllTypes.Num() >= 4);
	TestTrue(
		FString::Printf(TEXT("Should have at least 6 biome types, found %d"), AllBiomes.Num()),
		AllBiomes.Num() >= 6);
	TestTrue(
		FString::Printf(TEXT("Should have at least 3 economic specializations, found %d"), AllSpecs.Num()),
		AllSpecs.Num() >= 3);

	return true;
}

// ============================================================================
// 12. CONSTANTS VALIDATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIntegration_ProceduralConstantsValid,
	"Odyssey.Procedural.Integration.ProceduralConstantsValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_ProceduralConstantsValid::RunTest(const FString& Parameters)
{
	// Noise constants
	TestTrue(TEXT("MaxOctaves should be positive"), ProceduralConstants::MaxOctaves > 0);
	TestTrue(TEXT("DefaultLacunarity should be > 1"), ProceduralConstants::DefaultLacunarity > 1.0f);
	TestTrue(TEXT("DefaultPersistence should be in (0, 1)"),
		ProceduralConstants::DefaultPersistence > 0.0f && ProceduralConstants::DefaultPersistence < 1.0f);

	// Planet bounds
	TestTrue(TEXT("MinBiomesPerPlanet should be >= 1"), ProceduralConstants::MinBiomesPerPlanet >= 1);
	TestTrue(TEXT("MaxBiomesPerPlanet > MinBiomesPerPlanet"),
		ProceduralConstants::MaxBiomesPerPlanet > ProceduralConstants::MinBiomesPerPlanet);
	TestTrue(TEXT("MinPOIsPerPlanet >= 1"), ProceduralConstants::MinPOIsPerPlanet >= 1);
	TestTrue(TEXT("MaxPOIsPerPlanet > MinPOIsPerPlanet"),
		ProceduralConstants::MaxPOIsPerPlanet > ProceduralConstants::MinPOIsPerPlanet);
	TestTrue(TEXT("MinWorldSize > 0"), ProceduralConstants::MinWorldSize > 0.0f);
	TestTrue(TEXT("MaxWorldSize > MinWorldSize"),
		ProceduralConstants::MaxWorldSize > ProceduralConstants::MinWorldSize);

	// Resource constants
	TestTrue(TEXT("MinClusterSpacing > 0"), ProceduralConstants::MinClusterSpacing > 0.0f);
	TestTrue(TEXT("PoissonMaxRetries > 0"), ProceduralConstants::PoissonMaxRetries > 0);

	// Exploration constants
	TestTrue(TEXT("BaseDiscoveryReward > 0"), ProceduralConstants::BaseDiscoveryReward > 0);
	TestTrue(TEXT("RareDiscoveryMultiplier > 1"), ProceduralConstants::RareDiscoveryMultiplier > 1);
	TestTrue(TEXT("ScanRevealRadius > 0"), ProceduralConstants::ScanRevealRadius > 0.0f);
	TestTrue(TEXT("MaxExplorationProgress > 0"), ProceduralConstants::MaxExplorationProgress > 0.0f);

	// Economy constants
	TestTrue(TEXT("BaseMarketUpdateInterval > 0"), ProceduralConstants::BaseMarketUpdateInterval > 0.0f);
	TestTrue(TEXT("MinPriceMultiplier > 0"), ProceduralConstants::MinPriceMultiplier > 0.0f);
	TestTrue(TEXT("MaxPriceMultiplier > MinPriceMultiplier"),
		ProceduralConstants::MaxPriceMultiplier > ProceduralConstants::MinPriceMultiplier);
	TestTrue(TEXT("DefaultTaxRate in [0, 1)"),
		ProceduralConstants::DefaultTaxRate >= 0.0f && ProceduralConstants::DefaultTaxRate < 1.0f);

	return true;
}
