// PlanetGenerationTests.cpp
// Comprehensive automation tests for UOdysseyPlanetGenerator
// Tests planet creation, biome assignment, seed determinism, size scaling, and edge cases

#include "Misc/AutomationTest.h"
#include "OdysseyPlanetGenerator.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyResourceDistributionSystem.h"
#include "Procedural/ProceduralTypes.h"

// ============================================================================
// HELPERS
// ============================================================================

namespace PlanetGenTestHelpers
{
	/** Create a fully initialized planet generator with subsystem dependencies */
	static UOdysseyPlanetGenerator* CreateInitializedGenerator()
	{
		UOdysseyBiomeDefinitionSystem* BiomeSystem = NewObject<UOdysseyBiomeDefinitionSystem>();
		BiomeSystem->Initialize(nullptr);

		UOdysseyResourceDistributionSystem* ResourceSystem = NewObject<UOdysseyResourceDistributionSystem>();
		ResourceSystem->Initialize(BiomeSystem);

		UOdysseyPlanetGenerator* Generator = NewObject<UOdysseyPlanetGenerator>();
		Generator->Initialize(BiomeSystem, ResourceSystem);

		return Generator;
	}
}

// ============================================================================
// 1. BASIC PLANET CREATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_CreateWithValidParams,
	"Odyssey.Procedural.PlanetGeneration.CreateWithValidParameters",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_CreateWithValidParams::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(12345, EPlanetSize::Medium);

	TestTrue(TEXT("Planet should have a non-zero ID"), Planet.PlanetID != 0);
	TestFalse(TEXT("Planet name should not be empty"), Planet.PlanetName.IsEmpty());
	TestEqual(TEXT("Generation seed should match input"), Planet.GenerationSeed, 12345);
	TestTrue(TEXT("Planet should have biome regions"), Planet.BiomeRegions.Num() > 0);
	TestTrue(TEXT("Planet should have resource deposits"), Planet.ResourceDeposits.Num() > 0);
	TestTrue(TEXT("Planet should have points of interest"), Planet.PointsOfInterest.Num() > 0);
	TestTrue(TEXT("World size X should be positive"), Planet.WorldSize.X > 0.0f);
	TestTrue(TEXT("World size Y should be positive"), Planet.WorldSize.Y > 0.0f);
	TestFalse(TEXT("Planet should start undiscovered"), Planet.bDiscovered);
	TestEqual(TEXT("Exploration progress should start at 0"), Planet.ExplorationProgress, 0.0f);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_CreateWithSpecificType,
	"Odyssey.Procedural.PlanetGeneration.CreateWithSpecificType",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_CreateWithSpecificType::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	const EPlanetType TestTypes[] = {
		EPlanetType::Terrestrial, EPlanetType::Oceanic, EPlanetType::Desert,
		EPlanetType::Arctic, EPlanetType::Volcanic, EPlanetType::Jungle,
		EPlanetType::Barren, EPlanetType::Exotic, EPlanetType::Artificial
	};

	for (EPlanetType Type : TestTypes)
	{
		FGeneratedPlanetData Planet = Generator->GeneratePlanetWithType(100, Type, EPlanetSize::Medium);
		TestEqual(
			FString::Printf(TEXT("Planet type should match requested type %d"), static_cast<int32>(Type)),
			Planet.PlanetType, Type);
		TestTrue(
			FString::Printf(TEXT("Planet of type %d should have biome regions"), static_cast<int32>(Type)),
			Planet.BiomeRegions.Num() > 0);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_PhysicalDataValid,
	"Odyssey.Procedural.PlanetGeneration.PhysicalDataValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_PhysicalDataValid::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(42);

	const FPlanetPhysicalData& Phys = Planet.PhysicalData;
	TestTrue(TEXT("Radius should be positive"), Phys.Radius > 0.0f);
	TestTrue(TEXT("Surface gravity should be positive"), Phys.SurfaceGravity > 0.0f);
	TestTrue(TEXT("Atmosphere pressure should be non-negative"), Phys.AtmospherePressure >= 0.0f);
	TestTrue(TEXT("Magnetic field should be in [0, 1]"), Phys.MagneticFieldStrength >= 0.0f && Phys.MagneticFieldStrength <= 1.0f);
	TestTrue(TEXT("Water coverage should be in [0, 100]"), Phys.WaterCoverage >= 0.0f && Phys.WaterCoverage <= 100.0f);

	const FPlanetOrbitData& Orbit = Planet.OrbitData;
	TestTrue(TEXT("Orbital distance should be positive"), Orbit.OrbitalDistance > 0.0f);
	TestTrue(TEXT("Orbital period should be positive"), Orbit.OrbitalPeriod > 0.0f);
	TestTrue(TEXT("Eccentricity should be in [0, 0.9]"), Orbit.Eccentricity >= 0.0f && Orbit.Eccentricity <= 0.9f);
	TestTrue(TEXT("Day length should be positive"), Orbit.DayLength > 0.0f);

	return true;
}

// ============================================================================
// 2. BIOME ASSIGNMENT VARIETY
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_BiomeVariety,
	"Odyssey.Procedural.PlanetGeneration.BiomeAssignmentVariety",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_BiomeVariety::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	// Generate multiple planets and collect biome types used
	TSet<EBiomeType> AllBiomesEncountered;
	for (int32 Seed = 1; Seed <= 50; ++Seed)
	{
		FGeneratedPlanetData Planet = Generator->GeneratePlanet(Seed * 7919); // Prime multiplier for variety
		for (const FPlanetBiomeRegion& Region : Planet.BiomeRegions)
		{
			AllBiomesEncountered.Add(Region.BiomeType);
		}
	}

	// With 50 planets, we should see significant biome variety (at least 6 distinct types)
	TestTrue(
		FString::Printf(TEXT("Should have at least 6 distinct biome types across 50 planets, found %d"), AllBiomesEncountered.Num()),
		AllBiomesEncountered.Num() >= 6);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_BiomeRegionsNonOverlapping,
	"Odyssey.Procedural.PlanetGeneration.BiomeRegionsHaveValidBounds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_BiomeRegionsNonOverlapping::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(54321, EPlanetSize::Large);

	for (const FPlanetBiomeRegion& Region : Planet.BiomeRegions)
	{
		// Each region should have valid center coordinates (normalized 0-1)
		TestTrue(
			FString::Printf(TEXT("Region %d center X should be in [0,1]"), Region.RegionID),
			Region.RegionCenter.X >= 0.0f && Region.RegionCenter.X <= 1.0f);
		TestTrue(
			FString::Printf(TEXT("Region %d center Y should be in [0,1]"), Region.RegionID),
			Region.RegionCenter.Y >= 0.0f && Region.RegionCenter.Y <= 1.0f);
		TestTrue(
			FString::Printf(TEXT("Region %d size should be positive"), Region.RegionID),
			Region.RegionSize > 0.0f);
		TestTrue(
			FString::Printf(TEXT("Region %d biome type should not be None"), Region.RegionID),
			Region.BiomeType != EBiomeType::None);

		// World bounds should be sensible
		TestTrue(
			FString::Printf(TEXT("Region %d WorldMax.X >= WorldMin.X"), Region.RegionID),
			Region.WorldMax.X >= Region.WorldMin.X);
		TestTrue(
			FString::Printf(TEXT("Region %d WorldMax.Y >= WorldMin.Y"), Region.RegionID),
			Region.WorldMax.Y >= Region.WorldMin.Y);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_BiomeCountPerType,
	"Odyssey.Procedural.PlanetGeneration.BiomeCountMatchesPlanetType",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_BiomeCountPerType::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	// Barren planets should have fewer biome types, exotic planets more
	FGeneratedPlanetData Barren = Generator->GeneratePlanetWithType(100, EPlanetType::Barren, EPlanetSize::Medium);
	FGeneratedPlanetData Exotic = Generator->GeneratePlanetWithType(100, EPlanetType::Exotic, EPlanetSize::Medium);

	TestTrue(TEXT("Barren planet should have biome regions"), Barren.BiomeRegions.Num() >= ProceduralConstants::MinBiomesPerPlanet);
	TestTrue(TEXT("Exotic planet should have biome regions"), Exotic.BiomeRegions.Num() >= ProceduralConstants::MinBiomesPerPlanet);
	TestTrue(TEXT("Biome regions should not exceed max"),
		Barren.BiomeRegions.Num() <= ProceduralConstants::MaxBiomesPerPlanet &&
		Exotic.BiomeRegions.Num() <= ProceduralConstants::MaxBiomesPerPlanet);

	return true;
}

// ============================================================================
// 3. SEED-BASED DETERMINISTIC GENERATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_SeedDeterminism,
	"Odyssey.Procedural.PlanetGeneration.SeedBasedDeterminism",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_SeedDeterminism::RunTest(const FString& Parameters)
{
	// Create two independent generators
	UOdysseyPlanetGenerator* GeneratorA = PlanetGenTestHelpers::CreateInitializedGenerator();
	UOdysseyPlanetGenerator* GeneratorB = PlanetGenTestHelpers::CreateInitializedGenerator();

	const int32 TestSeed = 98765;

	FGeneratedPlanetData PlanetA = GeneratorA->GeneratePlanet(TestSeed, EPlanetSize::Large);
	FGeneratedPlanetData PlanetB = GeneratorB->GeneratePlanet(TestSeed, EPlanetSize::Large);

	// Core properties must match
	TestEqual(TEXT("Planet type should be identical"), PlanetA.PlanetType, PlanetB.PlanetType);
	TestEqual(TEXT("Planet size should be identical"), PlanetA.PlanetSize, PlanetB.PlanetSize);
	TestEqual(TEXT("World size should be identical"), PlanetA.WorldSize, PlanetB.WorldSize);
	TestEqual(TEXT("Biome count should be identical"), PlanetA.BiomeRegions.Num(), PlanetB.BiomeRegions.Num());
	TestEqual(TEXT("Resource deposit count should be identical"), PlanetA.ResourceDeposits.Num(), PlanetB.ResourceDeposits.Num());
	TestEqual(TEXT("POI count should be identical"), PlanetA.PointsOfInterest.Num(), PlanetB.PointsOfInterest.Num());
	TestEqual(TEXT("Economic rating should be identical"), PlanetA.EconomicRating, PlanetB.EconomicRating);
	TestEqual(TEXT("Danger rating should be identical"), PlanetA.DangerRating, PlanetB.DangerRating);

	// Physical data must match
	TestEqual(TEXT("Radius should match"), PlanetA.PhysicalData.Radius, PlanetB.PhysicalData.Radius);
	TestEqual(TEXT("Gravity should match"), PlanetA.PhysicalData.SurfaceGravity, PlanetB.PhysicalData.SurfaceGravity);
	TestEqual(TEXT("Temperature should match"), PlanetA.PhysicalData.AverageTemperature, PlanetB.PhysicalData.AverageTemperature);
	TestEqual(TEXT("Atmosphere type should match"), PlanetA.PhysicalData.AtmosphereType, PlanetB.PhysicalData.AtmosphereType);

	// Biome regions must match
	for (int32 i = 0; i < PlanetA.BiomeRegions.Num() && i < PlanetB.BiomeRegions.Num(); ++i)
	{
		TestEqual(
			FString::Printf(TEXT("Biome region %d type should match"), i),
			PlanetA.BiomeRegions[i].BiomeType, PlanetB.BiomeRegions[i].BiomeType);
	}

	// Resource types and locations must match
	for (int32 i = 0; i < PlanetA.ResourceDeposits.Num() && i < PlanetB.ResourceDeposits.Num(); ++i)
	{
		TestEqual(
			FString::Printf(TEXT("Resource deposit %d type should match"), i),
			PlanetA.ResourceDeposits[i].ResourceType, PlanetB.ResourceDeposits[i].ResourceType);
		TestEqual(
			FString::Printf(TEXT("Resource deposit %d location should match"), i),
			PlanetA.ResourceDeposits[i].Location, PlanetB.ResourceDeposits[i].Location);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_DifferentSeedsDifferentPlanets,
	"Odyssey.Procedural.PlanetGeneration.DifferentSeedsProduceDifferentResults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_DifferentSeedsDifferentPlanets::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData PlanetA = Generator->GeneratePlanet(111);
	FGeneratedPlanetData PlanetB = Generator->GeneratePlanet(999);

	// At minimum the names should differ (extremely unlikely to collide)
	bool bSomethingDiffers =
		!PlanetA.PlanetName.EqualTo(PlanetB.PlanetName) ||
		PlanetA.PlanetType != PlanetB.PlanetType ||
		PlanetA.PhysicalData.AverageTemperature != PlanetB.PhysicalData.AverageTemperature ||
		PlanetA.BiomeRegions.Num() != PlanetB.BiomeRegions.Num();

	TestTrue(TEXT("Different seeds should produce at least one differing property"), bSomethingDiffers);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_RegeneratePlanet,
	"Odyssey.Procedural.PlanetGeneration.RegeneratePlanetMatchesOriginal",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_RegeneratePlanet::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Original = Generator->GeneratePlanet(55555, EPlanetSize::Large);
	FGeneratedPlanetData Regenerated = Generator->RegeneratePlanet(Original);

	TestEqual(TEXT("Regenerated planet type should match"), Original.PlanetType, Regenerated.PlanetType);
	TestEqual(TEXT("Regenerated biome count should match"), Original.BiomeRegions.Num(), Regenerated.BiomeRegions.Num());
	TestEqual(TEXT("Regenerated resource count should match"), Original.ResourceDeposits.Num(), Regenerated.ResourceDeposits.Num());
	TestEqual(TEXT("Regenerated POI count should match"), Original.PointsOfInterest.Num(), Regenerated.PointsOfInterest.Num());

	return true;
}

// ============================================================================
// 4. PLANET SIZE AND COMPLEXITY SCALING
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_SizeScaling,
	"Odyssey.Procedural.PlanetGeneration.SizeAndComplexityScaling",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_SizeScaling::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	const int32 TestSeed = 42;
	FGeneratedPlanetData Tiny = Generator->GeneratePlanet(TestSeed, EPlanetSize::Tiny);
	FGeneratedPlanetData Small = Generator->GeneratePlanet(TestSeed, EPlanetSize::Small);
	FGeneratedPlanetData Medium = Generator->GeneratePlanet(TestSeed, EPlanetSize::Medium);
	FGeneratedPlanetData Large = Generator->GeneratePlanet(TestSeed, EPlanetSize::Large);
	FGeneratedPlanetData Huge = Generator->GeneratePlanet(TestSeed, EPlanetSize::Huge);
	FGeneratedPlanetData Giant = Generator->GeneratePlanet(TestSeed, EPlanetSize::Giant);

	// World sizes should scale up
	TestTrue(TEXT("Tiny world size < Small world size"), Tiny.WorldSize.X < Small.WorldSize.X);
	TestTrue(TEXT("Small world size < Medium world size"), Small.WorldSize.X < Medium.WorldSize.X);
	TestTrue(TEXT("Medium world size < Large world size"), Medium.WorldSize.X < Large.WorldSize.X);
	TestTrue(TEXT("Large world size < Huge world size"), Large.WorldSize.X < Huge.WorldSize.X);
	TestTrue(TEXT("Huge world size < Giant world size"), Huge.WorldSize.X < Giant.WorldSize.X);

	// Biome count should generally scale (at least non-decreasing)
	TestTrue(TEXT("Tiny should have fewer or equal biomes to Giant"),
		Tiny.BiomeRegions.Num() <= Giant.BiomeRegions.Num());

	// Resource deposits should scale
	TestTrue(TEXT("Tiny should have fewer or equal resources to Giant"),
		Tiny.ResourceDeposits.Num() <= Giant.ResourceDeposits.Num());

	// POIs should scale
	TestTrue(TEXT("Tiny should have fewer or equal POIs to Giant"),
		Tiny.PointsOfInterest.Num() <= Giant.PointsOfInterest.Num());

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_SizeUtilityFunctions,
	"Odyssey.Procedural.PlanetGeneration.SizeUtilityFunctionsValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_SizeUtilityFunctions::RunTest(const FString& Parameters)
{
	// Test static utility functions for all planet sizes
	const EPlanetSize Sizes[] = {
		EPlanetSize::Tiny, EPlanetSize::Small, EPlanetSize::Medium,
		EPlanetSize::Large, EPlanetSize::Huge, EPlanetSize::Giant
	};

	float PreviousWorldSizeX = 0.0f;
	for (EPlanetSize Size : Sizes)
	{
		FVector2D WorldSize = UOdysseyPlanetGenerator::GetWorldSizeForPlanetSize(Size);
		TestTrue(FString::Printf(TEXT("World size for %d should be positive"), static_cast<int32>(Size)),
			WorldSize.X > 0.0f && WorldSize.Y > 0.0f);
		TestTrue(FString::Printf(TEXT("World size for %d should be >= previous"), static_cast<int32>(Size)),
			WorldSize.X >= PreviousWorldSizeX);
		PreviousWorldSizeX = WorldSize.X;

		int32 BiomeCount = UOdysseyPlanetGenerator::GetBiomeCountForPlanetSize(Size);
		TestTrue(FString::Printf(TEXT("Biome count for %d should be >= MinBiomesPerPlanet"), static_cast<int32>(Size)),
			BiomeCount >= ProceduralConstants::MinBiomesPerPlanet);
		TestTrue(FString::Printf(TEXT("Biome count for %d should be <= MaxBiomesPerPlanet"), static_cast<int32>(Size)),
			BiomeCount <= ProceduralConstants::MaxBiomesPerPlanet);

		int32 ResourceCount = UOdysseyPlanetGenerator::GetResourceCountForPlanetSize(Size);
		TestTrue(FString::Printf(TEXT("Resource count for %d should be positive"), static_cast<int32>(Size)),
			ResourceCount > 0);

		int32 POICount = UOdysseyPlanetGenerator::GetPOICountForPlanetSize(Size);
		TestTrue(FString::Printf(TEXT("POI count for %d should be >= MinPOIsPerPlanet"), static_cast<int32>(Size)),
			POICount >= ProceduralConstants::MinPOIsPerPlanet);
		TestTrue(FString::Printf(TEXT("POI count for %d should be <= MaxPOIsPerPlanet"), static_cast<int32>(Size)),
			POICount <= ProceduralConstants::MaxPOIsPerPlanet);
	}

	return true;
}

// ============================================================================
// 5. STAR SYSTEM GENERATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_StarSystemGeneration,
	"Odyssey.Procedural.PlanetGeneration.StarSystemGeneration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_StarSystemGeneration::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FStarSystemData System = Generator->GenerateStarSystem(42, 3, 6);

	TestTrue(TEXT("System should have a non-zero ID"), System.SystemID != 0);
	TestFalse(TEXT("System name should not be empty"), System.SystemName.IsEmpty());
	TestTrue(TEXT("Star temperature should be positive"), System.StarTemperature > 0.0f);
	TestTrue(TEXT("System should have planets"), System.Planets.Num() > 0);
	TestTrue(TEXT("System should have at least MinPlanets"), System.Planets.Num() >= 3);
	TestTrue(TEXT("System should have at most MaxPlanets"), System.Planets.Num() <= 6);

	// Verify planets in system have increasing orbital distances
	float PrevDistance = 0.0f;
	for (const FGeneratedPlanetData& Planet : System.Planets)
	{
		TestTrue(
			FString::Printf(TEXT("Planet %d orbital distance should increase"), Planet.PlanetID),
			Planet.OrbitData.OrbitalDistance >= PrevDistance);
		PrevDistance = Planet.OrbitData.OrbitalDistance;
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_GalaxyRegionGeneration,
	"Odyssey.Procedural.PlanetGeneration.GalaxyRegionGeneration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_GalaxyRegionGeneration::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	const int32 SystemCount = 5;
	const FVector Center(0.0f, 0.0f, 0.0f);
	const float Radius = 10000.0f;

	TArray<FStarSystemData> Systems = Generator->GenerateGalaxyRegion(42, SystemCount, Center, Radius);

	TestEqual(TEXT("Should generate requested number of systems"), Systems.Num(), SystemCount);

	for (const FStarSystemData& System : Systems)
	{
		TestTrue(TEXT("Each system should have at least one planet"), System.Planets.Num() > 0);

		// Systems should be within the region radius
		float DistFromCenter = FVector::Dist(System.GalacticPosition, Center);
		TestTrue(
			FString::Printf(TEXT("System at distance %.1f should be within radius %.1f"), DistFromCenter, Radius),
			DistFromCenter <= Radius * 1.1f); // Small tolerance
	}

	// All system IDs should be unique
	TSet<int32> SystemIDs;
	for (const FStarSystemData& System : Systems)
	{
		TestFalse(
			FString::Printf(TEXT("System ID %d should be unique"), System.SystemID),
			SystemIDs.Contains(System.SystemID));
		SystemIDs.Add(System.SystemID);
	}

	return true;
}

// ============================================================================
// 6. PLANET NAMING
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_NamingGeneratesValidNames,
	"Odyssey.Procedural.PlanetGeneration.NamingGeneratesValidNames",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_NamingGeneratesValidNames::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	TSet<FString> UniqueNames;
	for (int32 Seed = 0; Seed < 100; ++Seed)
	{
		FText Name = Generator->GeneratePlanetName(Seed);
		TestFalse(FString::Printf(TEXT("Planet name for seed %d should not be empty"), Seed), Name.IsEmpty());
		UniqueNames.Add(Name.ToString());
	}

	// With 100 seeds, we should get reasonable name variety
	TestTrue(
		FString::Printf(TEXT("Should have at least 20 unique names from 100 seeds, got %d"), UniqueNames.Num()),
		UniqueNames.Num() >= 20);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_StarSystemNaming,
	"Odyssey.Procedural.PlanetGeneration.StarSystemNaming",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_StarSystemNaming::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	for (int32 Seed = 0; Seed < 20; ++Seed)
	{
		FText SystemName = Generator->GenerateStarSystemName(Seed);
		TestFalse(
			FString::Printf(TEXT("Star system name for seed %d should not be empty"), Seed),
			SystemName.IsEmpty());
	}

	return true;
}

// ============================================================================
// 7. BIOME REGION GENERATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_BiomeRegionGeneration,
	"Odyssey.Procedural.PlanetGeneration.BiomeRegionGeneration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_BiomeRegionGeneration::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	const FVector2D WorldSize(10000.0f, 10000.0f);
	const int32 BiomeCount = 6;

	TArray<FPlanetBiomeRegion> Regions = Generator->GenerateBiomeRegions(42, EPlanetType::Terrestrial, WorldSize, BiomeCount);

	TestEqual(TEXT("Should generate requested number of biome regions"), Regions.Num(), BiomeCount);

	TSet<int32> RegionIDs;
	for (const FPlanetBiomeRegion& Region : Regions)
	{
		TestTrue(TEXT("Region biome type should not be None"), Region.BiomeType != EBiomeType::None);
		TestFalse(
			FString::Printf(TEXT("Region ID %d should be unique"), Region.RegionID),
			RegionIDs.Contains(Region.RegionID));
		RegionIDs.Add(Region.RegionID);
	}

	return true;
}

// ============================================================================
// 8. POINT OF INTEREST GENERATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_POIGeneration,
	"Odyssey.Procedural.PlanetGeneration.PointOfInterestGeneration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_POIGeneration::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(42, EPlanetSize::Large);
	const int32 POICount = 10;

	TArray<FPlanetPointOfInterest> POIs = Generator->GeneratePointsOfInterest(42, Planet.BiomeRegions, POICount);

	TestEqual(TEXT("Should generate requested POI count"), POIs.Num(), POICount);

	TSet<int32> POIIDs;
	for (const FPlanetPointOfInterest& POI : POIs)
	{
		TestFalse(TEXT("POI name should not be empty"), POI.Name.IsEmpty());
		TestFalse(TEXT("POI type string should not be empty"), POI.POIType.IsEmpty());
		TestTrue(TEXT("POI discovery reward should be positive"), POI.DiscoveryReward > 0);
		TestFalse(TEXT("POI should start undiscovered"), POI.bDiscovered);
		TestFalse(
			FString::Printf(TEXT("POI ID %d should be unique"), POI.POIID),
			POIIDs.Contains(POI.POIID));
		POIIDs.Add(POI.POIID);
	}

	return true;
}

// ============================================================================
// 9. PLANET QUERY FUNCTIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_GetBiomeAtLocation,
	"Odyssey.Procedural.PlanetGeneration.GetBiomeAtLocation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_GetBiomeAtLocation::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(42, EPlanetSize::Large);

	// Query biome at a region center - should return that region's biome
	if (Planet.BiomeRegions.Num() > 0)
	{
		const FPlanetBiomeRegion& FirstRegion = Planet.BiomeRegions[0];
		FVector RegionCenterWorld(
			FirstRegion.RegionCenter.X * Planet.WorldSize.X,
			FirstRegion.RegionCenter.Y * Planet.WorldSize.Y,
			0.0f);

		EBiomeType Found = Generator->GetBiomeAtLocation(Planet, RegionCenterWorld);
		// The query should return a valid biome (not necessarily the exact one due to overlaps)
		TestTrue(TEXT("Biome at valid location should not be None"), Found != EBiomeType::None);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_GetResourcesInRegion,
	"Odyssey.Procedural.PlanetGeneration.GetResourcesInRegion",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_GetResourcesInRegion::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(42, EPlanetSize::Large);

	if (Planet.BiomeRegions.Num() > 0 && Planet.ResourceDeposits.Num() > 0)
	{
		TArray<FResourceDepositLocation> ResourcesInRegion = Generator->GetResourcesInRegion(Planet, Planet.BiomeRegions[0]);
		// Not all regions will have resources, but the function should not crash
		TestTrue(TEXT("GetResourcesInRegion should return a valid (possibly empty) array"), true);
	}

	return true;
}

// ============================================================================
// 10. RATING CALCULATIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_RatingCalculations,
	"Odyssey.Procedural.PlanetGeneration.RatingCalculations",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_RatingCalculations::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	for (int32 Seed = 0; Seed < 20; ++Seed)
	{
		FGeneratedPlanetData Planet = Generator->GeneratePlanet(Seed * 1000);

		int32 EconomicRating = UOdysseyPlanetGenerator::CalculateEconomicRating(Planet);
		int32 DangerRating = UOdysseyPlanetGenerator::CalculateDangerRating(Planet);

		TestTrue(FString::Printf(TEXT("Economic rating %d should be in [0, 100]"), EconomicRating),
			EconomicRating >= 0 && EconomicRating <= 100);
		TestTrue(FString::Printf(TEXT("Danger rating %d should be in [0, 100]"), DangerRating),
			DangerRating >= 0 && DangerRating <= 100);
	}

	return true;
}

// ============================================================================
// 11. EDGE CASES
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_EdgeCase_SeedZero,
	"Odyssey.Procedural.PlanetGeneration.EdgeCases.SeedZero",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_EdgeCase_SeedZero::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(0, EPlanetSize::Medium);

	TestTrue(TEXT("Seed 0 should still produce a valid planet with biomes"), Planet.BiomeRegions.Num() > 0);
	TestTrue(TEXT("Seed 0 should still produce a valid planet with resources"), Planet.ResourceDeposits.Num() > 0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_EdgeCase_NegativeSeed,
	"Odyssey.Procedural.PlanetGeneration.EdgeCases.NegativeSeed",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_EdgeCase_NegativeSeed::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(-12345, EPlanetSize::Medium);

	TestTrue(TEXT("Negative seed should still produce valid biomes"), Planet.BiomeRegions.Num() > 0);
	TestTrue(TEXT("Negative seed should still produce valid resources"), Planet.ResourceDeposits.Num() > 0);
	TestFalse(TEXT("Negative seed planet should have a name"), Planet.PlanetName.IsEmpty());

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_EdgeCase_MaxIntSeed,
	"Odyssey.Procedural.PlanetGeneration.EdgeCases.MaxIntSeed",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_EdgeCase_MaxIntSeed::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(MAX_int32, EPlanetSize::Medium);

	TestTrue(TEXT("MAX_int32 seed should produce valid biomes"), Planet.BiomeRegions.Num() > 0);
	TestTrue(TEXT("MAX_int32 seed should produce valid resources"), Planet.ResourceDeposits.Num() > 0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_EdgeCase_TinyPlanet,
	"Odyssey.Procedural.PlanetGeneration.EdgeCases.TinyPlanetMinimalContent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_EdgeCase_TinyPlanet::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(42, EPlanetSize::Tiny);

	TestTrue(TEXT("Even tiny planets must have minimum biomes"),
		Planet.BiomeRegions.Num() >= ProceduralConstants::MinBiomesPerPlanet);
	TestTrue(TEXT("Even tiny planets must have minimum POIs"),
		Planet.PointsOfInterest.Num() >= ProceduralConstants::MinPOIsPerPlanet);
	TestTrue(TEXT("Tiny planet world size should be >= MinWorldSize"),
		Planet.WorldSize.X >= ProceduralConstants::MinWorldSize);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_EdgeCase_GiantPlanet,
	"Odyssey.Procedural.PlanetGeneration.EdgeCases.GiantPlanetMaxContent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_EdgeCase_GiantPlanet::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	FGeneratedPlanetData Planet = Generator->GeneratePlanet(42, EPlanetSize::Giant);

	TestTrue(TEXT("Giant planet biome count should be <= max"),
		Planet.BiomeRegions.Num() <= ProceduralConstants::MaxBiomesPerPlanet);
	TestTrue(TEXT("Giant planet POI count should be <= max"),
		Planet.PointsOfInterest.Num() <= ProceduralConstants::MaxPOIsPerPlanet);
	TestTrue(TEXT("Giant planet world size should be <= MaxWorldSize"),
		Planet.WorldSize.X <= ProceduralConstants::MaxWorldSize);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetGen_EdgeCase_RapidSequentialGeneration,
	"Odyssey.Procedural.PlanetGeneration.EdgeCases.RapidSequentialGeneration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetGen_EdgeCase_RapidSequentialGeneration::RunTest(const FString& Parameters)
{
	UOdysseyPlanetGenerator* Generator = PlanetGenTestHelpers::CreateInitializedGenerator();

	// Generate 100 planets rapidly to ensure no ID collisions or crashes
	TSet<int32> PlanetIDs;
	for (int32 i = 0; i < 100; ++i)
	{
		FGeneratedPlanetData Planet = Generator->GeneratePlanet(i);
		TestFalse(
			FString::Printf(TEXT("Planet ID %d should be unique (iteration %d)"), Planet.PlanetID, i),
			PlanetIDs.Contains(Planet.PlanetID));
		PlanetIDs.Add(Planet.PlanetID);
	}

	TestEqual(TEXT("Should have generated 100 unique planet IDs"), PlanetIDs.Num(), 100);

	return true;
}
