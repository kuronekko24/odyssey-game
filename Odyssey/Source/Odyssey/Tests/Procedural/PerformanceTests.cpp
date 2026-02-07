// PerformanceTests.cpp
// Performance and stress tests for the Procedural Planet Generation System
// Tests generation time benchmarks, memory considerations, and concurrent generation

#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"
#include "OdysseyPlanetGenerator.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyResourceDistributionSystem.h"
#include "Procedural/ProceduralPlanetManager.h"
#include "Procedural/ExplorationRewardSystem.h"
#include "Procedural/ProceduralTypes.h"

// ============================================================================
// HELPERS
// ============================================================================

namespace PerfTestHelpers
{
	struct PerfTestContext
	{
		UOdysseyBiomeDefinitionSystem* BiomeSystem;
		UOdysseyResourceDistributionSystem* ResourceSystem;
		UOdysseyPlanetGenerator* PlanetGen;
		UExplorationRewardSystem* ExplorationSystem;
	};

	static PerfTestContext CreateFullContext()
	{
		PerfTestContext Ctx;

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
// 1. PLANET GENERATION TIME BENCHMARKS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_SinglePlanetGenTime,
	"Odyssey.Procedural.Performance.SinglePlanetGenerationTime",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_SinglePlanetGenTime::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	// Warm up
	Ctx.PlanetGen->GeneratePlanet(0, EPlanetSize::Medium);

	// Benchmark single planet generation
	const int32 Iterations = 20;
	double TotalTime = 0.0;
	double MaxTime = 0.0;
	double MinTime = DBL_MAX;

	for (int32 i = 0; i < Iterations; ++i)
	{
		double StartTime = FPlatformTime::Seconds();
		FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(i * 1000, EPlanetSize::Medium);
		double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

		TotalTime += ElapsedMs;
		MaxTime = FMath::Max(MaxTime, ElapsedMs);
		MinTime = FMath::Min(MinTime, ElapsedMs);
	}

	double AvgTime = TotalTime / Iterations;

	AddInfo(FString::Printf(TEXT("Single planet generation (Medium): Avg=%.2fms, Min=%.2fms, Max=%.2fms"),
		AvgTime, MinTime, MaxTime));

	// Mobile target: should complete within 100ms per planet
	TestTrue(
		FString::Printf(TEXT("Average generation time (%.2fms) should be under 100ms for mobile"), AvgTime),
		AvgTime < 100.0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_PlanetGenTimeBySizeCategory,
	"Odyssey.Procedural.Performance.PlanetGenTimeBySize",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_PlanetGenTimeBySizeCategory::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	const EPlanetSize Sizes[] = {
		EPlanetSize::Tiny, EPlanetSize::Small, EPlanetSize::Medium,
		EPlanetSize::Large, EPlanetSize::Huge, EPlanetSize::Giant
	};
	const TCHAR* SizeNames[] = {
		TEXT("Tiny"), TEXT("Small"), TEXT("Medium"),
		TEXT("Large"), TEXT("Huge"), TEXT("Giant")
	};

	const int32 SamplesPerSize = 10;

	for (int32 SizeIdx = 0; SizeIdx < 6; ++SizeIdx)
	{
		double TotalTime = 0.0;
		for (int32 i = 0; i < SamplesPerSize; ++i)
		{
			double StartTime = FPlatformTime::Seconds();
			Ctx.PlanetGen->GeneratePlanet(i * 1000, Sizes[SizeIdx]);
			TotalTime += (FPlatformTime::Seconds() - StartTime) * 1000.0;
		}

		double AvgTime = TotalTime / SamplesPerSize;
		AddInfo(FString::Printf(TEXT("%s planet: Avg=%.2fms"), SizeNames[SizeIdx], AvgTime));

		// Even Giant planets should generate within 500ms
		TestTrue(
			FString::Printf(TEXT("%s planet generation (%.2fms) should be under 500ms"), SizeNames[SizeIdx], AvgTime),
			AvgTime < 500.0);
	}

	return true;
}

// ============================================================================
// 2. STAR SYSTEM GENERATION BENCHMARKS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_StarSystemGenTime,
	"Odyssey.Procedural.Performance.StarSystemGenerationTime",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_StarSystemGenTime::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	const int32 Iterations = 10;
	double TotalTime = 0.0;

	for (int32 i = 0; i < Iterations; ++i)
	{
		double StartTime = FPlatformTime::Seconds();
		FStarSystemData System = Ctx.PlanetGen->GenerateStarSystem(i * 1000, 3, 8);
		double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
		TotalTime += ElapsedMs;
	}

	double AvgTime = TotalTime / Iterations;
	AddInfo(FString::Printf(TEXT("Star system generation (3-8 planets): Avg=%.2fms"), AvgTime));

	// Star system with 3-8 planets should complete within 1 second
	TestTrue(
		FString::Printf(TEXT("Star system generation (%.2fms) should be under 1000ms"), AvgTime),
		AvgTime < 1000.0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_GalaxyRegionGenTime,
	"Odyssey.Procedural.Performance.GalaxyRegionGenerationTime",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_GalaxyRegionGenTime::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	const int32 SystemCount = 10;
	const FVector Center(0.0f, 0.0f, 0.0f);
	const float Radius = 50000.0f;

	double StartTime = FPlatformTime::Seconds();
	TArray<FStarSystemData> Systems = Ctx.PlanetGen->GenerateGalaxyRegion(42, SystemCount, Center, Radius);
	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	int32 TotalPlanets = 0;
	for (const FStarSystemData& Sys : Systems)
	{
		TotalPlanets += Sys.Planets.Num();
	}

	AddInfo(FString::Printf(TEXT("Galaxy region (%d systems, %d planets): %.2fms"),
		SystemCount, TotalPlanets, ElapsedMs));

	// 10 star systems should generate within 5 seconds
	TestTrue(
		FString::Printf(TEXT("Galaxy region generation (%.2fms) should be under 5000ms"), ElapsedMs),
		ElapsedMs < 5000.0);

	return true;
}

// ============================================================================
// 3. EXPLORATION CONTENT GENERATION BENCHMARKS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_DiscoveryGenTime,
	"Odyssey.Procedural.Performance.DiscoveryGenerationTime",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_DiscoveryGenTime::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(42, EPlanetSize::Large);

	const int32 DiscoveryCounts[] = { 10, 25, 50, 100 };

	for (int32 Count : DiscoveryCounts)
	{
		// Fresh exploration system each time to avoid accumulation
		UExplorationRewardSystem* FreshExplorer = NewObject<UExplorationRewardSystem>();
		FreshExplorer->Initialize(Ctx.BiomeSystem);

		double StartTime = FPlatformTime::Seconds();
		TArray<FDiscoveryData> Discoveries = FreshExplorer->GenerateDiscoveriesForPlanet(Planet, Count);
		double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

		AddInfo(FString::Printf(TEXT("Generate %d discoveries: %.2fms (%.3fms each)"),
			Count, ElapsedMs, Count > 0 ? ElapsedMs / Count : 0.0));

		// 100 discoveries should generate within 200ms
		TestTrue(
			FString::Printf(TEXT("%d discoveries (%.2fms) should be under 200ms"), Count, ElapsedMs),
			ElapsedMs < 200.0);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_ScanPerformance,
	"Odyssey.Procedural.Performance.ScanPerformance",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_ScanPerformance::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(42, EPlanetSize::Large);
	Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 50);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num());

	FVector ScanOrigin(Planet.WorldSize.X / 2.0f, Planet.WorldSize.Y / 2.0f, 0.0f);

	// Benchmark scans
	const int32 ScanIterations = 50;
	double TotalTime = 0.0;

	for (int32 i = 0; i < ScanIterations; ++i)
	{
		FVector Offset(
			FMath::FRandRange(-1000.0f, 1000.0f),
			FMath::FRandRange(-1000.0f, 1000.0f),
			0.0f);

		double StartTime = FPlatformTime::Seconds();
		TArray<FScanResult> Results = Ctx.ExplorationSystem->PerformScan(
			Planet.PlanetID, ScanOrigin + Offset, EScanMode::Deep, 1.0f);
		TotalTime += (FPlatformTime::Seconds() - StartTime) * 1000.0;
	}

	double AvgScanTime = TotalTime / ScanIterations;
	AddInfo(FString::Printf(TEXT("Scan performance (50 discoveries, Deep mode): Avg=%.3fms"), AvgScanTime));

	// Scans should be very fast (under 5ms each)
	TestTrue(
		FString::Printf(TEXT("Average scan time (%.3fms) should be under 5ms"), AvgScanTime),
		AvgScanTime < 5.0);

	return true;
}

// ============================================================================
// 4. RESOURCE DISTRIBUTION BENCHMARKS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_ResourceDistributionTime,
	"Odyssey.Procedural.Performance.ResourceDistributionTime",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_ResourceDistributionTime::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	const FVector2D AreaSize(20000.0f, 20000.0f);
	TArray<EBiomeType> Biomes = {
		EBiomeType::Desert, EBiomeType::Forest, EBiomeType::Volcanic,
		EBiomeType::Crystalline, EBiomeType::Metallic
	};

	const int32 DepositCounts[] = { 20, 50, 100, 200 };

	for (int32 Count : DepositCounts)
	{
		double StartTime = FPlatformTime::Seconds();
		TArray<FResourceDepositLocation> Deposits = Ctx.ResourceSystem->GenerateResourceDeposits(
			42, AreaSize, Biomes, Count);
		double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

		AddInfo(FString::Printf(TEXT("Generate %d resource deposits: %.2fms"), Count, ElapsedMs));

		// 200 deposits should generate within 500ms
		TestTrue(
			FString::Printf(TEXT("%d deposits (%.2fms) should be under 500ms"), Count, ElapsedMs),
			ElapsedMs < 500.0);
	}

	return true;
}

// ============================================================================
// 5. MEMORY USAGE FOR GENERATED PLANETS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_PlanetDataMemoryEstimate,
	"Odyssey.Procedural.Performance.PlanetDataMemoryEstimate",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_PlanetDataMemoryEstimate::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	// Generate planets of each size and estimate memory footprint
	const EPlanetSize Sizes[] = {
		EPlanetSize::Tiny, EPlanetSize::Small, EPlanetSize::Medium,
		EPlanetSize::Large, EPlanetSize::Huge, EPlanetSize::Giant
	};
	const TCHAR* SizeNames[] = {
		TEXT("Tiny"), TEXT("Small"), TEXT("Medium"),
		TEXT("Large"), TEXT("Huge"), TEXT("Giant")
	};

	for (int32 SizeIdx = 0; SizeIdx < 6; ++SizeIdx)
	{
		FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(42, Sizes[SizeIdx]);

		// Estimate memory based on struct member counts
		int32 EstimatedBytes = sizeof(FGeneratedPlanetData);
		EstimatedBytes += Planet.BiomeRegions.Num() * sizeof(FPlanetBiomeRegion);
		EstimatedBytes += Planet.ResourceDeposits.Num() * sizeof(FResourceDepositLocation);
		EstimatedBytes += Planet.PointsOfInterest.Num() * sizeof(FPlanetPointOfInterest);

		float EstimatedKB = EstimatedBytes / 1024.0f;

		AddInfo(FString::Printf(
			TEXT("%s planet: ~%.1f KB (%d biomes, %d deposits, %d POIs)"),
			SizeNames[SizeIdx], EstimatedKB,
			Planet.BiomeRegions.Num(), Planet.ResourceDeposits.Num(), Planet.PointsOfInterest.Num()));

		// Each planet should be under 1MB in memory (mobile constraint)
		TestTrue(
			FString::Printf(TEXT("%s planet memory estimate (%.1f KB) should be under 1024 KB"), SizeNames[SizeIdx], EstimatedKB),
			EstimatedKB < 1024.0f);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_ExplorationGridMemory,
	"Odyssey.Procedural.Performance.ExplorationGridMemoryEstimate",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_ExplorationGridMemory::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	const int32 GridResolutions[] = { 16, 32, 64, 128 };

	for (int32 Resolution : GridResolutions)
	{
		int32 GridCells = Resolution * Resolution;
		int32 GridBytes = GridCells * sizeof(bool);
		float GridKB = GridBytes / 1024.0f;

		AddInfo(FString::Printf(TEXT("Exploration grid %dx%d: %d cells, %.1f KB"), Resolution, Resolution, GridCells, GridKB));

		// Default 32x32 grid should be very small
		if (Resolution == 32)
		{
			TestTrue(
				FString::Printf(TEXT("Default grid memory (%.1f KB) should be under 4 KB"), GridKB),
				GridKB < 4.0f);
		}
	}

	return true;
}

// ============================================================================
// 6. MULTIPLE SIMULTANEOUS PLANET GENERATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_BatchPlanetGeneration,
	"Odyssey.Procedural.Performance.BatchPlanetGeneration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_BatchPlanetGeneration::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	const int32 BatchSizes[] = { 10, 25, 50 };

	for (int32 BatchSize : BatchSizes)
	{
		TArray<FGeneratedPlanetData> Planets;
		Planets.Reserve(BatchSize);

		double StartTime = FPlatformTime::Seconds();
		for (int32 i = 0; i < BatchSize; ++i)
		{
			Planets.Add(Ctx.PlanetGen->GeneratePlanet(i, EPlanetSize::Medium));
		}
		double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

		AddInfo(FString::Printf(TEXT("Batch generate %d planets: %.2fms (%.2fms each)"),
			BatchSize, ElapsedMs, ElapsedMs / BatchSize));

		TestEqual(FString::Printf(TEXT("Should generate all %d planets"), BatchSize), Planets.Num(), BatchSize);

		// Verify all planets are unique
		TSet<int32> UniqueIDs;
		for (const auto& Planet : Planets)
		{
			UniqueIDs.Add(Planet.PlanetID);
		}
		TestEqual(TEXT("All planet IDs should be unique"), UniqueIDs.Num(), BatchSize);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_FullPlanetWithExploration,
	"Odyssey.Procedural.Performance.FullPlanetWithExplorationContent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_FullPlanetWithExploration::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	// Time the full pipeline: planet + discoveries + registration
	double StartTime = FPlatformTime::Seconds();

	FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(42, EPlanetSize::Large);
	TArray<FDiscoveryData> Discoveries = Ctx.ExplorationSystem->GenerateDiscoveriesForPlanet(Planet, 30);
	Ctx.ExplorationSystem->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num(), 32);

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	AddInfo(FString::Printf(
		TEXT("Full planet pipeline (Large + 30 discoveries): %.2fms"), ElapsedMs));

	// Full pipeline should complete within 500ms
	TestTrue(
		FString::Printf(TEXT("Full planet pipeline (%.2fms) should be under 500ms"), ElapsedMs),
		ElapsedMs < 500.0);

	return true;
}

// ============================================================================
// 7. BIOME SYSTEM INITIALIZATION BENCHMARK
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_BiomeSystemInit,
	"Odyssey.Procedural.Performance.BiomeSystemInitialization",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_BiomeSystemInit::RunTest(const FString& Parameters)
{
	const int32 Iterations = 20;
	double TotalTime = 0.0;

	for (int32 i = 0; i < Iterations; ++i)
	{
		double StartTime = FPlatformTime::Seconds();

		UOdysseyBiomeDefinitionSystem* BiomeSystem = NewObject<UOdysseyBiomeDefinitionSystem>();
		BiomeSystem->Initialize(nullptr);

		TotalTime += (FPlatformTime::Seconds() - StartTime) * 1000.0;
	}

	double AvgTime = TotalTime / Iterations;
	AddInfo(FString::Printf(TEXT("Biome system initialization: Avg=%.2fms"), AvgTime));

	// Initialization should be fast (under 10ms)
	TestTrue(
		FString::Printf(TEXT("Biome system init (%.2fms) should be under 10ms"), AvgTime),
		AvgTime < 10.0);

	return true;
}

// ============================================================================
// 8. SAVE DATA EFFICIENCY
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_SaveDataSize,
	"Odyssey.Procedural.Performance.SaveDataSizeEstimate",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_SaveDataSize::RunTest(const FString& Parameters)
{
	// Estimate save data size for a planet
	FPlanetSaveData SaveData;
	SaveData.PlanetID = 1;
	SaveData.GenerationSeed = 42;
	SaveData.bDiscovered = true;
	SaveData.ExplorationPercent = 45.0f;

	// Simulate discovered/claimed IDs
	for (int32 i = 0; i < 15; ++i)
	{
		SaveData.DiscoveredDiscoveryIDs.Add(i);
	}
	for (int32 i = 0; i < 5; ++i)
	{
		SaveData.ClaimedDiscoveryIDs.Add(i);
	}
	// Simulate depleted deposits
	for (int32 i = 0; i < 10; ++i)
	{
		SaveData.DepositRemainingAmounts.Add(i, 25);
	}

	// Rough estimate of serialized size
	int32 EstimatedBytes = sizeof(FPlanetSaveData);
	EstimatedBytes += SaveData.DiscoveredDiscoveryIDs.Num() * sizeof(int32);
	EstimatedBytes += SaveData.ClaimedDiscoveryIDs.Num() * sizeof(int32);
	EstimatedBytes += SaveData.DepositRemainingAmounts.Num() * (sizeof(int32) * 2);

	float EstimatedKB = EstimatedBytes / 1024.0f;
	AddInfo(FString::Printf(TEXT("Planet save data estimate: %.2f KB"), EstimatedKB));

	// Save data per planet should be very small (under 1KB)
	TestTrue(
		FString::Printf(TEXT("Save data (%.2f KB) should be under 1 KB"), EstimatedKB),
		EstimatedKB < 1.0f);

	// 100 planets worth of save data should be under 100KB
	float HundredPlanetsKB = EstimatedKB * 100;
	AddInfo(FString::Printf(TEXT("100 planets save data estimate: %.2f KB"), HundredPlanetsKB));
	TestTrue(
		FString::Printf(TEXT("100 planets save data (%.2f KB) should be under 100 KB"), HundredPlanetsKB),
		HundredPlanetsKB < 100.0f);

	return true;
}

// ============================================================================
// 9. QUERY PERFORMANCE WITH MANY PLANETS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_QueryPerformanceWithManyPlanets,
	"Odyssey.Procedural.Performance.QueryPerformanceWithManyPlanets",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_QueryPerformanceWithManyPlanets::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	// Generate 50 planets and store them
	TArray<FGeneratedPlanetData> AllPlanets;
	for (int32 i = 0; i < 50; ++i)
	{
		AllPlanets.Add(Ctx.PlanetGen->GeneratePlanet(i * 1000));
	}

	// Benchmark resource queries
	TArray<FResourceDepositLocation> AllDeposits;
	for (const auto& Planet : AllPlanets)
	{
		AllDeposits.Append(Planet.ResourceDeposits);
	}

	double StartTime = FPlatformTime::Seconds();

	// Run queries
	TArray<FResourceDepositLocation> InRadius = Ctx.ResourceSystem->FindDepositsInRadius(
		FVector(5000.0f, 5000.0f, 0.0f), 2000.0f, AllDeposits);
	TArray<FResourceDepositLocation> ByType = Ctx.ResourceSystem->FindDepositsByType(
		EResourceType::Silicate, AllDeposits);
	TArray<FResourceDepositLocation> ByRarity = Ctx.ResourceSystem->FindDepositsByRarity(
		EResourceRarity::Rare, AllDeposits);
	TMap<EResourceType, float> Abundance = Ctx.ResourceSystem->CalculateResourceAbundance(AllDeposits);

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	AddInfo(FString::Printf(TEXT("4 queries across %d deposits from 50 planets: %.2fms"),
		AllDeposits.Num(), ElapsedMs));

	// All queries should complete within 50ms
	TestTrue(
		FString::Printf(TEXT("Query batch (%.2fms) should be under 50ms"), ElapsedMs),
		ElapsedMs < 50.0);

	return true;
}

// ============================================================================
// 10. STRESS TEST - RAPID SEQUENTIAL OPERATIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerfTest_StressSequentialOperations,
	"Odyssey.Procedural.Performance.StressSequentialOperations",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPerfTest_StressSequentialOperations::RunTest(const FString& Parameters)
{
	auto Ctx = PerfTestHelpers::CreateFullContext();

	double StartTime = FPlatformTime::Seconds();

	// Rapidly create planets, generate exploration content, and query
	for (int32 i = 0; i < 30; ++i)
	{
		FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(i, EPlanetSize::Medium);

		UExplorationRewardSystem* TempExplorer = NewObject<UExplorationRewardSystem>();
		TempExplorer->Initialize(Ctx.BiomeSystem);
		TArray<FDiscoveryData> Discoveries = TempExplorer->GenerateDiscoveriesForPlanet(Planet, 10);
		TempExplorer->RegisterPlanet(Planet.PlanetID, Planet.BiomeRegions.Num());

		// Query operations
		TempExplorer->GetPlanetDiscoveries(Planet.PlanetID);
		TempExplorer->GetExplorationPercent(Planet.PlanetID);
		TempExplorer->GetMilestones(Planet.PlanetID);
	}

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	AddInfo(FString::Printf(TEXT("30 full planet operations (gen + explore + query): %.2fms (%.2fms each)"),
		ElapsedMs, ElapsedMs / 30.0));

	// 30 full cycles should complete within 10 seconds
	TestTrue(
		FString::Printf(TEXT("Stress test (%.2fms) should complete within 10000ms"), ElapsedMs),
		ElapsedMs < 10000.0);

	return true;
}
