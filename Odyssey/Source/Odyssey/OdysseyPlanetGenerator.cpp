// OdysseyPlanetGenerator.cpp
// Implementation of procedural planet generation system

#include "OdysseyPlanetGenerator.h"

UOdysseyPlanetGenerator::UOdysseyPlanetGenerator()
	: BiomeDefinitionSystem(nullptr)
	, ResourceDistributionSystem(nullptr)
	, NextPlanetID(1)
	, NextSystemID(1)
	, NextRegionID(1)
	, NextPOIID(1)
{
	InitializeNameGenerators();
}

void UOdysseyPlanetGenerator::Initialize(
	UOdysseyBiomeDefinitionSystem* BiomeSystem,
	UOdysseyResourceDistributionSystem* ResourceSystem)
{
	BiomeDefinitionSystem = BiomeSystem;
	ResourceDistributionSystem = ResourceSystem;
}

void UOdysseyPlanetGenerator::InitializeNameGenerators()
{
	// Planet name prefixes
	PlanetNamePrefixes = {
		TEXT("Nova"), TEXT("Stellar"), TEXT("Astra"), TEXT("Cosmos"), TEXT("Nebula"),
		TEXT("Solar"), TEXT("Lunar"), TEXT("Orbital"), TEXT("Galactic"), TEXT("Void"),
		TEXT("Prime"), TEXT("Alpha"), TEXT("Beta"), TEXT("Gamma"), TEXT("Delta"),
		TEXT("Zenith"), TEXT("Apex"), TEXT("Echo"), TEXT("Phantom"), TEXT("Shadow"),
		TEXT("Crystal"), TEXT("Iron"), TEXT("Golden"), TEXT("Silver"), TEXT("Crimson")
	};

	// Planet name suffixes
	PlanetNameSuffixes = {
		TEXT("Prime"), TEXT("Major"), TEXT("Minor"), TEXT("Alpha"), TEXT("Beta"),
		TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"),
		TEXT("Proxima"), TEXT("Ultima"), TEXT("Magnus"), TEXT("Vertex"), TEXT("Nexus")
	};

	// System name parts
	SystemNameParts = {
		TEXT("Kepler"), TEXT("Gliese"), TEXT("Trappist"), TEXT("Proxima"), TEXT("Sirius"),
		TEXT("Vega"), TEXT("Rigel"), TEXT("Altair"), TEXT("Deneb"), TEXT("Arcturus"),
		TEXT("Polaris"), TEXT("Canopus"), TEXT("Capella"), TEXT("Aldebaran"), TEXT("Antares"),
		TEXT("Betelgeuse"), TEXT("Achernar"), TEXT("Procyon"), TEXT("Regulus"), TEXT("Spica")
	};
}

FGeneratedPlanetData UOdysseyPlanetGenerator::GeneratePlanet(int32 Seed, EPlanetSize PreferredSize)
{
	FGeneratedPlanetData Planet;
	Planet.PlanetID = NextPlanetID++;
	Planet.GenerationSeed = Seed;

	// Determine size (use preferred or random)
	if (PreferredSize != EPlanetSize::Medium || SeededRandom(Seed) < 0.7f)
	{
		Planet.PlanetSize = PreferredSize;
	}
	else
	{
		Planet.PlanetSize = DeterminePlanetSize(Seed);
	}

	// Determine planet type based on random factors
	Planet.PlanetType = DeterminePlanetType(Seed + 100, 1.0f, 5778.0f);

	// Generate name
	Planet.PlanetName = GeneratePlanetName(Seed);

	// Set world size based on planet size
	Planet.WorldSize = GetWorldSizeForPlanetSize(Planet.PlanetSize);

	// Generate physical and orbital data
	Planet.PhysicalData = GeneratePhysicalData(Seed + 200, Planet.PlanetType, Planet.PlanetSize);
	Planet.OrbitData = GenerateOrbitData(Seed + 300, 3, 5778.0f);

	// Generate biome regions
	int32 BiomeCount = GetBiomeCountForPlanetSize(Planet.PlanetSize);
	Planet.BiomeRegions = GenerateBiomeRegions(Seed + 400, Planet.PlanetType, Planet.WorldSize, BiomeCount);

	// Generate resource deposits
	if (ResourceDistributionSystem)
	{
		TArray<EBiomeType> Biomes;
		for (const FPlanetBiomeRegion& Region : Planet.BiomeRegions)
		{
			Biomes.AddUnique(Region.BiomeType);
		}

		int32 ResourceCount = GetResourceCountForPlanetSize(Planet.PlanetSize);
		Planet.ResourceDeposits = ResourceDistributionSystem->GenerateResourceDeposits(
			Seed + 500,
			Planet.WorldSize,
			Biomes,
			ResourceCount
		);
	}

	// Generate points of interest
	int32 POICount = GetPOICountForPlanetSize(Planet.PlanetSize);
	Planet.PointsOfInterest = GeneratePointsOfInterest(Seed + 600, Planet.BiomeRegions, POICount);

	// Calculate ratings
	Planet.EconomicRating = CalculateEconomicRating(Planet);
	Planet.DangerRating = CalculateDangerRating(Planet);

	return Planet;
}

FGeneratedPlanetData UOdysseyPlanetGenerator::GeneratePlanetWithType(int32 Seed, EPlanetType Type, EPlanetSize Size)
{
	FGeneratedPlanetData Planet;
	Planet.PlanetID = NextPlanetID++;
	Planet.GenerationSeed = Seed;
	Planet.PlanetType = Type;
	Planet.PlanetSize = Size;
	Planet.PlanetName = GeneratePlanetName(Seed);
	Planet.WorldSize = GetWorldSizeForPlanetSize(Size);

	// Generate data based on specified type
	Planet.PhysicalData = GeneratePhysicalData(Seed + 200, Type, Size);
	Planet.OrbitData = GenerateOrbitData(Seed + 300, 3, 5778.0f);

	// Generate biome regions based on planet type
	int32 BiomeCount = GetBiomeCountForPlanetSize(Size);
	Planet.BiomeRegions = GenerateBiomeRegions(Seed + 400, Type, Planet.WorldSize, BiomeCount);

	// Generate resources
	if (ResourceDistributionSystem)
	{
		TArray<EBiomeType> Biomes;
		for (const FPlanetBiomeRegion& Region : Planet.BiomeRegions)
		{
			Biomes.AddUnique(Region.BiomeType);
		}

		int32 ResourceCount = GetResourceCountForPlanetSize(Size);
		Planet.ResourceDeposits = ResourceDistributionSystem->GenerateResourceDeposits(
			Seed + 500,
			Planet.WorldSize,
			Biomes,
			ResourceCount
		);
	}

	// Generate POIs
	int32 POICount = GetPOICountForPlanetSize(Size);
	Planet.PointsOfInterest = GeneratePointsOfInterest(Seed + 600, Planet.BiomeRegions, POICount);

	Planet.EconomicRating = CalculateEconomicRating(Planet);
	Planet.DangerRating = CalculateDangerRating(Planet);

	return Planet;
}

FGeneratedPlanetData UOdysseyPlanetGenerator::RegeneratePlanet(const FGeneratedPlanetData& ExistingPlanet)
{
	// Regenerate using the same seed to get identical results
	return GeneratePlanetWithType(
		ExistingPlanet.GenerationSeed,
		ExistingPlanet.PlanetType,
		ExistingPlanet.PlanetSize
	);
}

FStarSystemData UOdysseyPlanetGenerator::GenerateStarSystem(int32 Seed, int32 MinPlanets, int32 MaxPlanets)
{
	FStarSystemData System;
	System.SystemID = NextSystemID++;
	System.GenerationSeed = Seed;
	System.SystemName = GenerateStarSystemName(Seed);

	// Determine star type
	float StarTypeRandom = SeededRandom(Seed);
	if (StarTypeRandom < 0.1f)
	{
		System.StarType = TEXT("O"); // Blue giant
		System.StarTemperature = 30000.0f + SeededRandom(Seed + 1) * 20000.0f;
	}
	else if (StarTypeRandom < 0.2f)
	{
		System.StarType = TEXT("B");
		System.StarTemperature = 10000.0f + SeededRandom(Seed + 1) * 20000.0f;
	}
	else if (StarTypeRandom < 0.35f)
	{
		System.StarType = TEXT("A");
		System.StarTemperature = 7500.0f + SeededRandom(Seed + 1) * 2500.0f;
	}
	else if (StarTypeRandom < 0.5f)
	{
		System.StarType = TEXT("F");
		System.StarTemperature = 6000.0f + SeededRandom(Seed + 1) * 1500.0f;
	}
	else if (StarTypeRandom < 0.7f)
	{
		System.StarType = TEXT("G"); // Sun-like
		System.StarTemperature = 5200.0f + SeededRandom(Seed + 1) * 800.0f;
	}
	else if (StarTypeRandom < 0.85f)
	{
		System.StarType = TEXT("K");
		System.StarTemperature = 3700.0f + SeededRandom(Seed + 1) * 1500.0f;
	}
	else
	{
		System.StarType = TEXT("M"); // Red dwarf
		System.StarTemperature = 2400.0f + SeededRandom(Seed + 1) * 1300.0f;
	}

	// Generate planets
	int32 PlanetCount = SeededRandomRange(Seed + 100, MinPlanets, MaxPlanets);

	for (int32 i = 0; i < PlanetCount; ++i)
	{
		int32 PlanetSeed = Seed + (i + 1) * 1000;

		// Determine planet size based on orbital position
		EPlanetSize Size;
		float SizeRandom = SeededRandom(PlanetSeed);
		if (i < 2) // Inner planets tend to be smaller
		{
			Size = SizeRandom < 0.6f ? EPlanetSize::Small : 
				   SizeRandom < 0.9f ? EPlanetSize::Medium : EPlanetSize::Tiny;
		}
		else if (i < 4) // Middle planets can be any size
		{
			Size = static_cast<EPlanetSize>(SeededRandomRange(PlanetSeed + 1, 1, 4));
		}
		else // Outer planets tend to be larger
		{
			Size = SizeRandom < 0.3f ? EPlanetSize::Large :
				   SizeRandom < 0.6f ? EPlanetSize::Huge : EPlanetSize::Giant;
		}

		// Calculate orbital distance
		float OrbitalDistance = 0.3f + i * 0.4f + SeededRandom(PlanetSeed + 2) * 0.3f;

		// Determine planet type based on orbital distance and star temperature
		EPlanetType Type = DeterminePlanetType(PlanetSeed, OrbitalDistance, System.StarTemperature);

		FGeneratedPlanetData Planet = GeneratePlanetWithType(PlanetSeed, Type, Size);
		Planet.OrbitData = GenerateOrbitData(PlanetSeed + 300, i, System.StarTemperature);
		Planet.OrbitData.OrbitalDistance = OrbitalDistance;

		System.Planets.Add(Planet);
	}

	return System;
}

TArray<FStarSystemData> UOdysseyPlanetGenerator::GenerateGalaxyRegion(
	int32 Seed,
	int32 SystemCount,
	const FVector& RegionCenter,
	float RegionRadius)
{
	TArray<FStarSystemData> Systems;

	for (int32 i = 0; i < SystemCount; ++i)
	{
		int32 SystemSeed = Seed + i * 10000;

		// Generate random position within region
		float Angle1 = SeededRandom(SystemSeed) * 2.0f * PI;
		float Angle2 = SeededRandom(SystemSeed + 1) * PI - PI / 2.0f;
		float Distance = SeededRandom(SystemSeed + 2) * RegionRadius;

		FVector Position = RegionCenter + FVector(
			Distance * FMath::Cos(Angle1) * FMath::Cos(Angle2),
			Distance * FMath::Sin(Angle1) * FMath::Cos(Angle2),
			Distance * FMath::Sin(Angle2)
		);

		FStarSystemData System = GenerateStarSystem(SystemSeed, 1, 6);
		System.GalacticPosition = Position;

		Systems.Add(System);
	}

	return Systems;
}

TArray<FPlanetBiomeRegion> UOdysseyPlanetGenerator::GenerateBiomeRegions(
	int32 Seed,
	EPlanetType PlanetType,
	const FVector2D& WorldSize,
	int32 BiomeCount)
{
	TArray<FPlanetBiomeRegion> Regions;

	// Select biomes appropriate for this planet type
	TArray<EBiomeType> SelectedBiomes = SelectBiomesForPlanetType(Seed, PlanetType, BiomeCount);

	// Create regions for each biome
	for (int32 i = 0; i < SelectedBiomes.Num(); ++i)
	{
		FPlanetBiomeRegion Region;
		Region.RegionID = NextRegionID++;
		Region.BiomeType = SelectedBiomes[i];

		// Initial region center (will be adjusted by layout)
		Region.RegionCenter = FVector2D(
			SeededRandom(Seed + i * 100),
			SeededRandom(Seed + i * 100 + 1)
		);

		Region.RegionSize = 1.0f / BiomeCount + SeededRandom(Seed + i * 100 + 2) * 0.2f;

		Regions.Add(Region);
	}

	// Layout regions to fill the world
	LayoutBiomeRegions(Regions, WorldSize, Seed + 1000);

	return Regions;
}

TArray<FPlanetPointOfInterest> UOdysseyPlanetGenerator::GeneratePointsOfInterest(
	int32 Seed,
	const TArray<FPlanetBiomeRegion>& Regions,
	int32 POICount)
{
	TArray<FPlanetPointOfInterest> POIs;

	if (Regions.Num() == 0)
	{
		return POIs;
	}

	// Distribute POIs across regions
	int32 POIsPerRegion = POICount / Regions.Num();
	int32 ExtraPOIs = POICount % Regions.Num();

	for (int32 RegionIndex = 0; RegionIndex < Regions.Num(); ++RegionIndex)
	{
		const FPlanetBiomeRegion& Region = Regions[RegionIndex];
		int32 RegionPOICount = POIsPerRegion + (RegionIndex < ExtraPOIs ? 1 : 0);

		for (int32 POIIndex = 0; POIIndex < RegionPOICount; ++POIIndex)
		{
			int32 POISeed = Seed + RegionIndex * 1000 + POIIndex * 100;
			FPlanetPointOfInterest POI = GeneratePOI(POISeed, Region);
			POIs.Add(POI);
		}
	}

	return POIs;
}

FText UOdysseyPlanetGenerator::GeneratePlanetName(int32 Seed) const
{
	FString Name;

	// 60% chance of prefix + number
	// 40% chance of prefix + suffix
	if (SeededRandom(Seed) < 0.6f)
	{
		int32 PrefixIndex = SeededRandomRange(Seed + 1, 0, PlanetNamePrefixes.Num() - 1);
		int32 Number = SeededRandomRange(Seed + 2, 1, 999);
		Name = FString::Printf(TEXT("%s-%d"), *PlanetNamePrefixes[PrefixIndex], Number);
	}
	else
	{
		int32 PrefixIndex = SeededRandomRange(Seed + 1, 0, PlanetNamePrefixes.Num() - 1);
		int32 SuffixIndex = SeededRandomRange(Seed + 2, 0, PlanetNameSuffixes.Num() - 1);
		Name = FString::Printf(TEXT("%s %s"), *PlanetNamePrefixes[PrefixIndex], *PlanetNameSuffixes[SuffixIndex]);
	}

	return FText::FromString(Name);
}

FText UOdysseyPlanetGenerator::GenerateStarSystemName(int32 Seed) const
{
	FString Name;

	int32 PartIndex = SeededRandomRange(Seed, 0, SystemNameParts.Num() - 1);
	int32 Number = SeededRandomRange(Seed + 1, 1, 9999);

	// Format: Name-Number or Name Number-Letter
	if (SeededRandom(Seed + 2) < 0.5f)
	{
		Name = FString::Printf(TEXT("%s-%d"), *SystemNameParts[PartIndex], Number);
	}
	else
	{
		TCHAR Letter = 'A' + SeededRandomRange(Seed + 3, 0, 25);
		Name = FString::Printf(TEXT("%s %d%c"), *SystemNameParts[PartIndex], Number, Letter);
	}

	return FText::FromString(Name);
}

EBiomeType UOdysseyPlanetGenerator::GetBiomeAtLocation(const FGeneratedPlanetData& Planet, const FVector& Location) const
{
	FPlanetBiomeRegion Region = GetRegionAtLocation(Planet, Location);
	return Region.BiomeType;
}

FPlanetBiomeRegion UOdysseyPlanetGenerator::GetRegionAtLocation(const FGeneratedPlanetData& Planet, const FVector& Location) const
{
	// Find the region that contains this location
	for (const FPlanetBiomeRegion& Region : Planet.BiomeRegions)
	{
		if (Location.X >= Region.WorldMin.X && Location.X <= Region.WorldMax.X &&
			Location.Y >= Region.WorldMin.Y && Location.Y <= Region.WorldMax.Y)
		{
			return Region;
		}
	}

	// Fallback: return first region or empty
	if (Planet.BiomeRegions.Num() > 0)
	{
		return Planet.BiomeRegions[0];
	}

	return FPlanetBiomeRegion();
}

TArray<FResourceDepositLocation> UOdysseyPlanetGenerator::GetResourcesInRegion(
	const FGeneratedPlanetData& Planet,
	const FPlanetBiomeRegion& Region) const
{
	TArray<FResourceDepositLocation> Result;

	for (const FResourceDepositLocation& Deposit : Planet.ResourceDeposits)
	{
		if (Deposit.Location.X >= Region.WorldMin.X && Deposit.Location.X <= Region.WorldMax.X &&
			Deposit.Location.Y >= Region.WorldMin.Y && Deposit.Location.Y <= Region.WorldMax.Y)
		{
			Result.Add(Deposit);
		}
	}

	return Result;
}

FVector2D UOdysseyPlanetGenerator::GetWorldSizeForPlanetSize(EPlanetSize Size)
{
	switch (Size)
	{
	case EPlanetSize::Tiny:
		return FVector2D(2000.0f, 2000.0f);
	case EPlanetSize::Small:
		return FVector2D(5000.0f, 5000.0f);
	case EPlanetSize::Medium:
		return FVector2D(10000.0f, 10000.0f);
	case EPlanetSize::Large:
		return FVector2D(20000.0f, 20000.0f);
	case EPlanetSize::Huge:
		return FVector2D(35000.0f, 35000.0f);
	case EPlanetSize::Giant:
		return FVector2D(50000.0f, 50000.0f);
	default:
		return FVector2D(10000.0f, 10000.0f);
	}
}

int32 UOdysseyPlanetGenerator::GetBiomeCountForPlanetSize(EPlanetSize Size)
{
	switch (Size)
	{
	case EPlanetSize::Tiny:
		return 1;
	case EPlanetSize::Small:
		return 2;
	case EPlanetSize::Medium:
		return 3;
	case EPlanetSize::Large:
		return 4;
	case EPlanetSize::Huge:
		return 5;
	case EPlanetSize::Giant:
		return 6;
	default:
		return 3;
	}
}

int32 UOdysseyPlanetGenerator::GetResourceCountForPlanetSize(EPlanetSize Size)
{
	switch (Size)
	{
	case EPlanetSize::Tiny:
		return 10;
	case EPlanetSize::Small:
		return 25;
	case EPlanetSize::Medium:
		return 50;
	case EPlanetSize::Large:
		return 100;
	case EPlanetSize::Huge:
		return 175;
	case EPlanetSize::Giant:
		return 250;
	default:
		return 50;
	}
}

int32 UOdysseyPlanetGenerator::GetPOICountForPlanetSize(EPlanetSize Size)
{
	switch (Size)
	{
	case EPlanetSize::Tiny:
		return 2;
	case EPlanetSize::Small:
		return 5;
	case EPlanetSize::Medium:
		return 10;
	case EPlanetSize::Large:
		return 18;
	case EPlanetSize::Huge:
		return 30;
	case EPlanetSize::Giant:
		return 45;
	default:
		return 10;
	}
}

int32 UOdysseyPlanetGenerator::CalculateEconomicRating(const FGeneratedPlanetData& Planet)
{
	int32 Rating = 0;

	// Base rating from resource count
	Rating += FMath::Min(30, Planet.ResourceDeposits.Num() / 2);

	// Bonus for rare resources
	int32 RareCount = 0;
	for (const FResourceDepositLocation& Deposit : Planet.ResourceDeposits)
	{
		if (Deposit.Rarity >= EResourceRarity::Rare)
		{
			RareCount++;
		}
	}
	Rating += FMath::Min(30, RareCount * 3);

	// Bonus for diverse biomes
	TSet<EBiomeType> UniqueBiomes;
	for (const FPlanetBiomeRegion& Region : Planet.BiomeRegions)
	{
		UniqueBiomes.Add(Region.BiomeType);
	}
	Rating += UniqueBiomes.Num() * 5;

	// Bonus for POIs
	Rating += FMath::Min(20, Planet.PointsOfInterest.Num());

	return FMath::Clamp(Rating, 0, 100);
}

int32 UOdysseyPlanetGenerator::CalculateDangerRating(const FGeneratedPlanetData& Planet)
{
	int32 Rating = 0;

	// Check biome hazards
	for (const FPlanetBiomeRegion& Region : Planet.BiomeRegions)
	{
		switch (Region.BiomeType)
		{
		case EBiomeType::Volcanic:
		case EBiomeType::Radioactive:
			Rating += 15;
			break;
		case EBiomeType::Toxic:
			Rating += 12;
			break;
		case EBiomeType::Ice:
		case EBiomeType::Desert:
			Rating += 8;
			break;
		case EBiomeType::Anomalous:
			Rating += 20;
			break;
		default:
			Rating += 3;
			break;
		}
	}

	// Physical conditions
	if (Planet.PhysicalData.AtmosphereType == EAtmosphereType::Toxic ||
		Planet.PhysicalData.AtmosphereType == EAtmosphereType::Corrosive)
	{
		Rating += 15;
	}

	if (Planet.PhysicalData.SurfaceGravity > 1.5f || Planet.PhysicalData.SurfaceGravity < 0.5f)
	{
		Rating += 10;
	}

	if (Planet.PhysicalData.AverageTemperature > 60.0f || Planet.PhysicalData.AverageTemperature < -40.0f)
	{
		Rating += 10;
	}

	return FMath::Clamp(Rating, 0, 100);
}

EPlanetType UOdysseyPlanetGenerator::DeterminePlanetType(int32 Seed, float OrbitalDistance, float StarTemperature) const
{
	// Calculate habitable zone based on star temperature
	float HabitableZoneCenter = (StarTemperature / 5778.0f) * 1.0f; // 1 AU for Sun-like stars
	float HabitableZoneWidth = 0.5f;

	float DistanceFromHabitable = FMath::Abs(OrbitalDistance - HabitableZoneCenter);

	// Close to star = hot planets
	if (OrbitalDistance < HabitableZoneCenter - HabitableZoneWidth)
	{
		float Random = SeededRandom(Seed);
		if (Random < 0.4f) return EPlanetType::Volcanic;
		if (Random < 0.7f) return EPlanetType::Desert;
		return EPlanetType::Barren;
	}
	// Habitable zone = diverse planets
	else if (DistanceFromHabitable <= HabitableZoneWidth)
	{
		float Random = SeededRandom(Seed);
		if (Random < 0.25f) return EPlanetType::Terrestrial;
		if (Random < 0.45f) return EPlanetType::Oceanic;
		if (Random < 0.60f) return EPlanetType::Jungle;
		if (Random < 0.75f) return EPlanetType::Desert;
		if (Random < 0.90f) return EPlanetType::Arctic;
		return EPlanetType::Exotic;
	}
	// Far from star = cold planets
	else
	{
		float Random = SeededRandom(Seed);
		if (Random < 0.5f) return EPlanetType::Arctic;
		if (Random < 0.8f) return EPlanetType::Barren;
		return EPlanetType::Exotic;
	}
}

EPlanetSize UOdysseyPlanetGenerator::DeterminePlanetSize(int32 Seed) const
{
	float Random = SeededRandom(Seed);

	// Weighted distribution favoring medium sizes
	if (Random < 0.05f) return EPlanetSize::Tiny;
	if (Random < 0.20f) return EPlanetSize::Small;
	if (Random < 0.55f) return EPlanetSize::Medium;
	if (Random < 0.80f) return EPlanetSize::Large;
	if (Random < 0.95f) return EPlanetSize::Huge;
	return EPlanetSize::Giant;
}

FPlanetPhysicalData UOdysseyPlanetGenerator::GeneratePhysicalData(int32 Seed, EPlanetType Type, EPlanetSize Size) const
{
	FPlanetPhysicalData Data;

	// Radius based on size
	switch (Size)
	{
	case EPlanetSize::Tiny:
		Data.Radius = 1000.0f + SeededRandom(Seed) * 1000.0f;
		break;
	case EPlanetSize::Small:
		Data.Radius = 2000.0f + SeededRandom(Seed) * 2000.0f;
		break;
	case EPlanetSize::Medium:
		Data.Radius = 4000.0f + SeededRandom(Seed) * 4000.0f;
		break;
	case EPlanetSize::Large:
		Data.Radius = 8000.0f + SeededRandom(Seed) * 8000.0f;
		break;
	case EPlanetSize::Huge:
		Data.Radius = 15000.0f + SeededRandom(Seed) * 15000.0f;
		break;
	case EPlanetSize::Giant:
		Data.Radius = 30000.0f + SeededRandom(Seed) * 40000.0f;
		break;
	}

	// Gravity related to size
	Data.SurfaceGravity = 0.5f + (static_cast<float>(Size) / 5.0f) * 1.5f + SeededRandom(Seed + 1) * 0.3f;

	// Temperature and atmosphere based on type
	switch (Type)
	{
	case EPlanetType::Volcanic:
		Data.AverageTemperature = 80.0f + SeededRandom(Seed + 2) * 120.0f;
		Data.TemperatureVariation = 30.0f;
		Data.AtmosphereType = SeededRandom(Seed + 3) < 0.5f ? EAtmosphereType::Toxic : EAtmosphereType::Dense;
		Data.AtmospherePressure = 1.5f + SeededRandom(Seed + 4) * 2.0f;
		Data.WaterCoverage = SeededRandom(Seed + 5) * 10.0f;
		break;

	case EPlanetType::Desert:
		Data.AverageTemperature = 30.0f + SeededRandom(Seed + 2) * 40.0f;
		Data.TemperatureVariation = 50.0f;
		Data.AtmosphereType = SeededRandom(Seed + 3) < 0.7f ? EAtmosphereType::Thin : EAtmosphereType::Standard;
		Data.AtmospherePressure = 0.3f + SeededRandom(Seed + 4) * 0.7f;
		Data.WaterCoverage = SeededRandom(Seed + 5) * 15.0f;
		break;

	case EPlanetType::Arctic:
		Data.AverageTemperature = -60.0f + SeededRandom(Seed + 2) * 40.0f;
		Data.TemperatureVariation = 20.0f;
		Data.AtmosphereType = EAtmosphereType::Standard;
		Data.AtmospherePressure = 0.8f + SeededRandom(Seed + 4) * 0.4f;
		Data.WaterCoverage = 10.0f + SeededRandom(Seed + 5) * 40.0f; // Ice coverage
		break;

	case EPlanetType::Oceanic:
		Data.AverageTemperature = 10.0f + SeededRandom(Seed + 2) * 20.0f;
		Data.TemperatureVariation = 15.0f;
		Data.AtmosphereType = EAtmosphereType::Standard;
		Data.AtmospherePressure = 0.9f + SeededRandom(Seed + 4) * 0.3f;
		Data.WaterCoverage = 80.0f + SeededRandom(Seed + 5) * 18.0f;
		break;

	case EPlanetType::Jungle:
		Data.AverageTemperature = 20.0f + SeededRandom(Seed + 2) * 15.0f;
		Data.TemperatureVariation = 10.0f;
		Data.AtmosphereType = EAtmosphereType::Dense;
		Data.AtmospherePressure = 1.0f + SeededRandom(Seed + 4) * 0.5f;
		Data.WaterCoverage = 40.0f + SeededRandom(Seed + 5) * 30.0f;
		break;

	case EPlanetType::Barren:
		Data.AverageTemperature = -20.0f + SeededRandom(Seed + 2) * 60.0f;
		Data.TemperatureVariation = 80.0f;
		Data.AtmosphereType = SeededRandom(Seed + 3) < 0.7f ? EAtmosphereType::None : EAtmosphereType::Thin;
		Data.AtmospherePressure = SeededRandom(Seed + 4) * 0.3f;
		Data.WaterCoverage = SeededRandom(Seed + 5) * 5.0f;
		break;

	case EPlanetType::Exotic:
		Data.AverageTemperature = -100.0f + SeededRandom(Seed + 2) * 200.0f;
		Data.TemperatureVariation = 60.0f;
		Data.AtmosphereType = static_cast<EAtmosphereType>(SeededRandomRange(Seed + 3, 0, 5));
		Data.AtmospherePressure = SeededRandom(Seed + 4) * 3.0f;
		Data.WaterCoverage = SeededRandom(Seed + 5) * 100.0f;
		break;

	default: // Terrestrial
		Data.AverageTemperature = 5.0f + SeededRandom(Seed + 2) * 25.0f;
		Data.TemperatureVariation = 30.0f;
		Data.AtmosphereType = EAtmosphereType::Standard;
		Data.AtmospherePressure = 0.8f + SeededRandom(Seed + 4) * 0.4f;
		Data.WaterCoverage = 30.0f + SeededRandom(Seed + 5) * 40.0f;
		break;
	}

	// Magnetic field
	Data.MagneticFieldStrength = SeededRandom(Seed + 6);

	return Data;
}

FPlanetOrbitData UOdysseyPlanetGenerator::GenerateOrbitData(int32 Seed, int32 OrbitIndex, float StarTemperature) const
{
	FPlanetOrbitData Data;

	// Orbital distance increases with index (simplified Titius-Bode)
	Data.OrbitalDistance = 0.4f + OrbitIndex * 0.3f + SeededRandom(Seed) * 0.2f;

	// Orbital period based on distance (Kepler's third law approximation)
	Data.OrbitalPeriod = FMath::Pow(Data.OrbitalDistance, 1.5f) * 365.0f;

	// Eccentricity
	Data.Eccentricity = SeededRandom(Seed + 1) * 0.3f;

	// Current angle (random starting position)
	Data.CurrentAngle = SeededRandom(Seed + 2) * 360.0f;

	// Axial tilt
	Data.AxialTilt = SeededRandom(Seed + 3) * 45.0f;

	// Day length
	Data.DayLength = 10.0f + SeededRandom(Seed + 4) * 50.0f;

	return Data;
}

TArray<EBiomeType> UOdysseyPlanetGenerator::SelectBiomesForPlanetType(int32 Seed, EPlanetType Type, int32 Count) const
{
	TArray<EBiomeType> Result;
	TArray<EBiomeType> Candidates;

	// Select candidate biomes based on planet type
	switch (Type)
	{
	case EPlanetType::Volcanic:
		Candidates = { EBiomeType::Volcanic, EBiomeType::Desert, EBiomeType::Barren, EBiomeType::Metallic };
		break;
	case EPlanetType::Desert:
		Candidates = { EBiomeType::Desert, EBiomeType::Barren, EBiomeType::Crystalline, EBiomeType::Metallic };
		break;
	case EPlanetType::Arctic:
		Candidates = { EBiomeType::Ice, EBiomeType::Barren, EBiomeType::Ocean, EBiomeType::Crystalline };
		break;
	case EPlanetType::Oceanic:
		Candidates = { EBiomeType::Ocean, EBiomeType::Ice, EBiomeType::Forest, EBiomeType::Lush };
		break;
	case EPlanetType::Jungle:
		Candidates = { EBiomeType::Lush, EBiomeType::Forest, EBiomeType::Toxic, EBiomeType::Ocean };
		break;
	case EPlanetType::Barren:
		Candidates = { EBiomeType::Barren, EBiomeType::Desert, EBiomeType::Metallic, EBiomeType::Radioactive };
		break;
	case EPlanetType::Exotic:
		Candidates = { EBiomeType::Anomalous, EBiomeType::Crystalline, EBiomeType::Radioactive, EBiomeType::Toxic };
		break;
	default: // Terrestrial
		Candidates = { EBiomeType::Forest, EBiomeType::Desert, EBiomeType::Ocean, EBiomeType::Ice, EBiomeType::Lush, EBiomeType::Barren };
		break;
	}

	// Select biomes from candidates
	for (int32 i = 0; i < Count && Candidates.Num() > 0; ++i)
	{
		int32 Index = SeededRandomRange(Seed + i, 0, Candidates.Num() - 1);
		Result.Add(Candidates[Index]);
		Candidates.RemoveAt(Index);
	}

	// Fill remaining slots with any biome if needed
	if (Result.Num() < Count && BiomeDefinitionSystem)
	{
		TArray<FBiomeDefinition> AllBiomes = BiomeDefinitionSystem->GetAllBiomeDefinitions();
		for (int32 i = Result.Num(); i < Count && AllBiomes.Num() > 0; ++i)
		{
			int32 Index = SeededRandomRange(Seed + Count + i, 0, AllBiomes.Num() - 1);
			if (!Result.Contains(AllBiomes[Index].BiomeType))
			{
				Result.Add(AllBiomes[Index].BiomeType);
			}
		}
	}

	return Result;
}

void UOdysseyPlanetGenerator::LayoutBiomeRegions(
	TArray<FPlanetBiomeRegion>& Regions,
	const FVector2D& WorldSize,
	int32 Seed) const
{
	if (Regions.Num() == 0) return;

	// Simple grid-based layout with some randomization
	int32 GridSize = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Regions.Num())));
	float CellWidth = WorldSize.X / GridSize;
	float CellHeight = WorldSize.Y / FMath::CeilToInt(static_cast<float>(Regions.Num()) / GridSize);

	for (int32 i = 0; i < Regions.Num(); ++i)
	{
		int32 GridX = i % GridSize;
		int32 GridY = i / GridSize;

		// Add some randomization to boundaries
		float RandomOffsetX = SeededRandomFloatRange(Seed + i * 10, -CellWidth * 0.1f, CellWidth * 0.1f);
		float RandomOffsetY = SeededRandomFloatRange(Seed + i * 10 + 1, -CellHeight * 0.1f, CellHeight * 0.1f);

		Regions[i].WorldMin = FVector2D(
			FMath::Max(0.0f, GridX * CellWidth + RandomOffsetX),
			FMath::Max(0.0f, GridY * CellHeight + RandomOffsetY)
		);

		Regions[i].WorldMax = FVector2D(
			FMath::Min(WorldSize.X, (GridX + 1) * CellWidth + RandomOffsetX),
			FMath::Min(WorldSize.Y, (GridY + 1) * CellHeight + RandomOffsetY)
		);

		Regions[i].RegionCenter = FVector2D(
			(Regions[i].WorldMin.X + Regions[i].WorldMax.X) / 2.0f / WorldSize.X,
			(Regions[i].WorldMin.Y + Regions[i].WorldMax.Y) / 2.0f / WorldSize.Y
		);
	}
}

FPlanetPointOfInterest UOdysseyPlanetGenerator::GeneratePOI(int32 Seed, const FPlanetBiomeRegion& Region) const
{
	FPlanetPointOfInterest POI;
	POI.POIID = NextPOIID++;
	POI.Biome = Region.BiomeType;

	// Determine POI type based on biome
	TArray<FString> POITypes;
	switch (Region.BiomeType)
	{
	case EBiomeType::Desert:
	case EBiomeType::Barren:
		POITypes = { TEXT("Ancient Ruins"), TEXT("Crashed Ship"), TEXT("Underground Cave"), TEXT("Mineral Vein") };
		break;
	case EBiomeType::Forest:
	case EBiomeType::Lush:
		POITypes = { TEXT("Hidden Grove"), TEXT("Ancient Tree"), TEXT("Wildlife Den"), TEXT("Natural Spring") };
		break;
	case EBiomeType::Volcanic:
		POITypes = { TEXT("Lava Tube"), TEXT("Obsidian Formation"), TEXT("Thermal Vent"), TEXT("Magma Chamber") };
		break;
	case EBiomeType::Ice:
		POITypes = { TEXT("Ice Cave"), TEXT("Frozen Lake"), TEXT("Crystal Formation"), TEXT("Buried Structure") };
		break;
	case EBiomeType::Ocean:
		POITypes = { TEXT("Underwater Ruins"), TEXT("Deep Trench"), TEXT("Coral Formation"), TEXT("Shipwreck") };
		break;
	case EBiomeType::Crystalline:
		POITypes = { TEXT("Crystal Cave"), TEXT("Energy Nexus"), TEXT("Resonance Chamber"), TEXT("Prism Formation") };
		break;
	case EBiomeType::Toxic:
		POITypes = { TEXT("Toxic Pool"), TEXT("Chemical Deposit"), TEXT("Mutant Nest"), TEXT("Processing Ruin") };
		break;
	case EBiomeType::Radioactive:
		POITypes = { TEXT("Reactor Ruin"), TEXT("Isotope Deposit"), TEXT("Anomaly Zone"), TEXT("Containment Breach") };
		break;
	case EBiomeType::Metallic:
		POITypes = { TEXT("Metal Spire"), TEXT("Ore Deposit"), TEXT("Ancient Machine"), TEXT("Processing Plant") };
		break;
	case EBiomeType::Anomalous:
		POITypes = { TEXT("Reality Tear"), TEXT("Time Distortion"), TEXT("Void Gate"), TEXT("Impossible Structure") };
		break;
	default:
		POITypes = { TEXT("Unknown Structure"), TEXT("Resource Cache"), TEXT("Abandoned Camp"), TEXT("Survey Marker") };
		break;
	}

	int32 TypeIndex = SeededRandomRange(Seed, 0, POITypes.Num() - 1);
	POI.POIType = POITypes[TypeIndex];
	POI.Name = GeneratePOIName(Seed + 100, POI.POIType);

	// Generate location within region
	POI.Location = FVector(
		Region.WorldMin.X + SeededRandom(Seed + 200) * (Region.WorldMax.X - Region.WorldMin.X),
		Region.WorldMin.Y + SeededRandom(Seed + 201) * (Region.WorldMax.Y - Region.WorldMin.Y),
		0.0f
	);

	// Discovery reward based on biome danger
	int32 BaseReward = 50;
	if (Region.BiomeType == EBiomeType::Anomalous) BaseReward = 200;
	else if (Region.BiomeType == EBiomeType::Radioactive || Region.BiomeType == EBiomeType::Volcanic) BaseReward = 100;
	else if (Region.BiomeType == EBiomeType::Toxic || Region.BiomeType == EBiomeType::Crystalline) BaseReward = 75;

	POI.DiscoveryReward = BaseReward + SeededRandomRange(Seed + 300, 0, BaseReward / 2);

	// Generate description
	POI.Description = FText::FromString(FString::Printf(
		TEXT("A %s discovered in the %s region. Exploration may yield valuable discoveries."),
		*POI.POIType.ToLower(),
		*UEnum::GetDisplayValueAsText(Region.BiomeType).ToString().ToLower()
	));

	return POI;
}

FText UOdysseyPlanetGenerator::GeneratePOIName(int32 Seed, const FString& POIType) const
{
	TArray<FString> Adjectives = {
		TEXT("Ancient"), TEXT("Hidden"), TEXT("Lost"), TEXT("Forgotten"), TEXT("Mysterious"),
		TEXT("Remote"), TEXT("Isolated"), TEXT("Strange"), TEXT("Peculiar"), TEXT("Enigmatic")
	};

	int32 AdjIndex = SeededRandomRange(Seed, 0, Adjectives.Num() - 1);
	int32 Number = SeededRandomRange(Seed + 1, 1, 99);

	return FText::FromString(FString::Printf(TEXT("%s %s %d"), *Adjectives[AdjIndex], *POIType, Number));
}

uint32 UOdysseyPlanetGenerator::HashSeed(int32 Seed)
{
	uint32 Hash = static_cast<uint32>(Seed);
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = (Hash >> 16) ^ Hash;
	return Hash;
}

float UOdysseyPlanetGenerator::SeededRandom(int32 Seed)
{
	uint32 Hash = HashSeed(Seed);
	return static_cast<float>(Hash & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
}

int32 UOdysseyPlanetGenerator::SeededRandomRange(int32 Seed, int32 Min, int32 Max)
{
	if (Min >= Max) return Min;
	float Random = SeededRandom(Seed);
	return Min + static_cast<int32>(Random * (Max - Min + 1));
}

float UOdysseyPlanetGenerator::SeededRandomFloatRange(int32 Seed, float Min, float Max)
{
	return Min + SeededRandom(Seed) * (Max - Min);
}
