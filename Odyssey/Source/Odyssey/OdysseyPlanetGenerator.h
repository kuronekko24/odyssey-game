// OdysseyPlanetGenerator.h
// Procedural planet generation with seed-based consistency
// Part of the Odyssey Procedural Planet & Resource Generation System

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyResourceDistributionSystem.h"
#include "OdysseyPlanetGenerator.generated.h"

// Forward declarations
class UOdysseyPlanetaryEconomyComponent;
class UOdysseyExplorationRewardSystem;

/**
 * Enum for planet size categories
 */
UENUM(BlueprintType)
enum class EPlanetSize : uint8
{
	Tiny = 0 UMETA(DisplayName = "Tiny"),
	Small = 1 UMETA(DisplayName = "Small"),
	Medium = 2 UMETA(DisplayName = "Medium"),
	Large = 3 UMETA(DisplayName = "Large"),
	Huge = 4 UMETA(DisplayName = "Huge"),
	Giant = 5 UMETA(DisplayName = "Giant")
};

/**
 * Enum for planet type/class
 */
UENUM(BlueprintType)
enum class EPlanetType : uint8
{
	Terrestrial = 0 UMETA(DisplayName = "Terrestrial"),
	Oceanic = 1 UMETA(DisplayName = "Oceanic"),
	Desert = 2 UMETA(DisplayName = "Desert"),
	Arctic = 3 UMETA(DisplayName = "Arctic"),
	Volcanic = 4 UMETA(DisplayName = "Volcanic"),
	Jungle = 5 UMETA(DisplayName = "Jungle"),
	Barren = 6 UMETA(DisplayName = "Barren"),
	Exotic = 7 UMETA(DisplayName = "Exotic"),
	Artificial = 8 UMETA(DisplayName = "Artificial")
};

/**
 * Enum for atmosphere type
 */
UENUM(BlueprintType)
enum class EAtmosphereType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Thin = 1 UMETA(DisplayName = "Thin"),
	Standard = 2 UMETA(DisplayName = "Standard"),
	Dense = 3 UMETA(DisplayName = "Dense"),
	Toxic = 4 UMETA(DisplayName = "Toxic"),
	Corrosive = 5 UMETA(DisplayName = "Corrosive")
};

/**
 * Struct for planet orbital data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlanetOrbitData
{
	GENERATED_BODY()

	// Distance from star (AU)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float OrbitalDistance;

	// Orbital period (Earth days)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float OrbitalPeriod;

	// Orbital eccentricity (0 = circular, 1 = parabolic)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float Eccentricity;

	// Current orbital angle (0-360)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float CurrentAngle;

	// Axial tilt (degrees)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float AxialTilt;

	// Day length (Earth hours)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit")
	float DayLength;

	FPlanetOrbitData()
		: OrbitalDistance(1.0f)
		, OrbitalPeriod(365.0f)
		, Eccentricity(0.0f)
		, CurrentAngle(0.0f)
		, AxialTilt(23.5f)
		, DayLength(24.0f)
	{
	}
};

/**
 * Struct for planet physical characteristics
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlanetPhysicalData
{
	GENERATED_BODY()

	// Planet radius (km)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical")
	float Radius;

	// Surface gravity (Earth = 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical")
	float SurfaceGravity;

	// Average surface temperature (Celsius)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical")
	float AverageTemperature;

	// Temperature variation (day/night, seasonal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical")
	float TemperatureVariation;

	// Atmosphere type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical")
	EAtmosphereType AtmosphereType;

	// Atmosphere pressure (Earth = 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical")
	float AtmospherePressure;

	// Magnetic field strength (0-1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MagneticFieldStrength;

	// Water coverage percentage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physical", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float WaterCoverage;

	FPlanetPhysicalData()
		: Radius(6371.0f)
		, SurfaceGravity(1.0f)
		, AverageTemperature(15.0f)
		, TemperatureVariation(30.0f)
		, AtmosphereType(EAtmosphereType::Standard)
		, AtmospherePressure(1.0f)
		, MagneticFieldStrength(0.5f)
		, WaterCoverage(70.0f)
	{
	}
};

/**
 * Struct for a biome region on a planet
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlanetBiomeRegion
{
	GENERATED_BODY()

	// Biome type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Region")
	EBiomeType BiomeType;

	// Region center (normalized 0-1 coordinates)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Region")
	FVector2D RegionCenter;

	// Region size (normalized)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Region")
	float RegionSize;

	// Actual world bounds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Region")
	FVector2D WorldMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Region")
	FVector2D WorldMax;

	// Region ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Region")
	int32 RegionID;

	FPlanetBiomeRegion()
		: BiomeType(EBiomeType::Barren)
		, RegionCenter(FVector2D(0.5f, 0.5f))
		, RegionSize(0.25f)
		, WorldMin(FVector2D::ZeroVector)
		, WorldMax(FVector2D(1000.0f, 1000.0f))
		, RegionID(0)
	{
	}
};

/**
 * Struct for point of interest on a planet
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlanetPointOfInterest
{
	GENERATED_BODY()

	// POI name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "POI")
	FText Name;

	// POI description
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "POI")
	FText Description;

	// World location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "POI")
	FVector Location;

	// POI type (for categorization)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "POI")
	FString POIType;

	// Whether discovered
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "POI")
	bool bDiscovered;

	// Discovery reward (OMEN)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "POI")
	int32 DiscoveryReward;

	// Associated biome
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "POI")
	EBiomeType Biome;

	// Unique ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "POI")
	int32 POIID;

	FPlanetPointOfInterest()
		: Name(FText::FromString(TEXT("Unknown Location")))
		, Description(FText::FromString(TEXT("")))
		, Location(FVector::ZeroVector)
		, POIType(TEXT("Generic"))
		, bDiscovered(false)
		, DiscoveryReward(50)
		, Biome(EBiomeType::Barren)
		, POIID(0)
	{
	}
};

/**
 * Complete planet data structure
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGeneratedPlanetData
{
	GENERATED_BODY()

	// Basic info
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FText PlanetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	int32 PlanetID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	int32 GenerationSeed;

	// Classification
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	EPlanetType PlanetType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	EPlanetSize PlanetSize;

	// Physical and orbital data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FPlanetPhysicalData PhysicalData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FPlanetOrbitData OrbitData;

	// Biomes and regions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	TArray<FPlanetBiomeRegion> BiomeRegions;

	// Resource deposits
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	TArray<FResourceDepositLocation> ResourceDeposits;

	// Points of interest
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	TArray<FPlanetPointOfInterest> PointsOfInterest;

	// World dimensions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	FVector2D WorldSize;

	// Discovery status
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	bool bDiscovered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	float ExplorationProgress;

	// Economic rating (0-100)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	int32 EconomicRating;

	// Danger rating (0-100)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	int32 DangerRating;

	FGeneratedPlanetData()
		: PlanetName(FText::FromString(TEXT("Unknown Planet")))
		, PlanetID(0)
		, GenerationSeed(0)
		, PlanetType(EPlanetType::Terrestrial)
		, PlanetSize(EPlanetSize::Medium)
		, WorldSize(FVector2D(10000.0f, 10000.0f))
		, bDiscovered(false)
		, ExplorationProgress(0.0f)
		, EconomicRating(50)
		, DangerRating(25)
	{
	}
};

/**
 * Struct for star system data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FStarSystemData
{
	GENERATED_BODY()

	// System name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	FText SystemName;

	// System ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	int32 SystemID;

	// Generation seed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	int32 GenerationSeed;

	// Star type (for visual/gameplay effects)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	FString StarType;

	// Star temperature (affects habitable zone)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	float StarTemperature;

	// Planets in this system
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	TArray<FGeneratedPlanetData> Planets;

	// System position in galaxy
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	FVector GalacticPosition;

	// Discovery status
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Star System")
	bool bDiscovered;

	FStarSystemData()
		: SystemName(FText::FromString(TEXT("Unknown System")))
		, SystemID(0)
		, GenerationSeed(0)
		, StarType(TEXT("G"))
		, StarTemperature(5778.0f)
		, GalacticPosition(FVector::ZeroVector)
		, bDiscovered(false)
	{
	}
};

/**
 * UOdysseyPlanetGenerator
 * 
 * Generates procedural planets with diverse biomes, resource distributions,
 * and exploration opportunities. Uses seed-based generation for consistency
 * across game sessions.
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API UOdysseyPlanetGenerator : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyPlanetGenerator();

	// Initialization
	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	void Initialize(
		UOdysseyBiomeDefinitionSystem* BiomeSystem,
		UOdysseyResourceDistributionSystem* ResourceSystem
	);

	// Planet generation
	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	FGeneratedPlanetData GeneratePlanet(int32 Seed, EPlanetSize PreferredSize = EPlanetSize::Medium);

	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	FGeneratedPlanetData GeneratePlanetWithType(int32 Seed, EPlanetType Type, EPlanetSize Size);

	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	FGeneratedPlanetData RegeneratePlanet(const FGeneratedPlanetData& ExistingPlanet);

	// Star system generation
	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	FStarSystemData GenerateStarSystem(int32 Seed, int32 MinPlanets = 2, int32 MaxPlanets = 8);

	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	TArray<FStarSystemData> GenerateGalaxyRegion(
		int32 Seed,
		int32 SystemCount,
		const FVector& RegionCenter,
		float RegionRadius
	);

	// Biome region generation
	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	TArray<FPlanetBiomeRegion> GenerateBiomeRegions(
		int32 Seed,
		EPlanetType PlanetType,
		const FVector2D& WorldSize,
		int32 BiomeCount
	);

	// Points of interest generation
	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	TArray<FPlanetPointOfInterest> GeneratePointsOfInterest(
		int32 Seed,
		const TArray<FPlanetBiomeRegion>& Regions,
		int32 POICount
	);

	// Planet naming
	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	FText GeneratePlanetName(int32 Seed) const;

	UFUNCTION(BlueprintCallable, Category = "Planet Generator")
	FText GenerateStarSystemName(int32 Seed) const;

	// Planet queries
	UFUNCTION(BlueprintCallable, Category = "Planet Query")
	EBiomeType GetBiomeAtLocation(const FGeneratedPlanetData& Planet, const FVector& Location) const;

	UFUNCTION(BlueprintCallable, Category = "Planet Query")
	FPlanetBiomeRegion GetRegionAtLocation(const FGeneratedPlanetData& Planet, const FVector& Location) const;

	UFUNCTION(BlueprintCallable, Category = "Planet Query")
	TArray<FResourceDepositLocation> GetResourcesInRegion(
		const FGeneratedPlanetData& Planet,
		const FPlanetBiomeRegion& Region
	) const;

	// Size and type utilities
	UFUNCTION(BlueprintCallable, Category = "Planet Utility")
	static FVector2D GetWorldSizeForPlanetSize(EPlanetSize Size);

	UFUNCTION(BlueprintCallable, Category = "Planet Utility")
	static int32 GetBiomeCountForPlanetSize(EPlanetSize Size);

	UFUNCTION(BlueprintCallable, Category = "Planet Utility")
	static int32 GetResourceCountForPlanetSize(EPlanetSize Size);

	UFUNCTION(BlueprintCallable, Category = "Planet Utility")
	static int32 GetPOICountForPlanetSize(EPlanetSize Size);

	// Rating calculations
	UFUNCTION(BlueprintCallable, Category = "Planet Utility")
	static int32 CalculateEconomicRating(const FGeneratedPlanetData& Planet);

	UFUNCTION(BlueprintCallable, Category = "Planet Utility")
	static int32 CalculateDangerRating(const FGeneratedPlanetData& Planet);

protected:
	// Subsystem references
	UPROPERTY()
	UOdysseyBiomeDefinitionSystem* BiomeDefinitionSystem;

	UPROPERTY()
	UOdysseyResourceDistributionSystem* ResourceDistributionSystem;

	// ID counters
	UPROPERTY()
	int32 NextPlanetID;

	UPROPERTY()
	int32 NextSystemID;

	UPROPERTY()
	int32 NextRegionID;

	UPROPERTY()
	int32 NextPOIID;

private:
	// Internal generation helpers
	EPlanetType DeterminePlanetType(int32 Seed, float OrbitalDistance, float StarTemperature) const;
	EPlanetSize DeterminePlanetSize(int32 Seed) const;
	FPlanetPhysicalData GeneratePhysicalData(int32 Seed, EPlanetType Type, EPlanetSize Size) const;
	FPlanetOrbitData GenerateOrbitData(int32 Seed, int32 OrbitIndex, float StarTemperature) const;

	// Biome-related helpers
	TArray<EBiomeType> SelectBiomesForPlanetType(int32 Seed, EPlanetType Type, int32 Count) const;
	void LayoutBiomeRegions(
		TArray<FPlanetBiomeRegion>& Regions,
		const FVector2D& WorldSize,
		int32 Seed
	) const;

	// POI generation helpers
	FPlanetPointOfInterest GeneratePOI(int32 Seed, const FPlanetBiomeRegion& Region) const;
	FText GeneratePOIName(int32 Seed, const FString& POIType) const;

	// Name generation helpers
	TArray<FString> PlanetNamePrefixes;
	TArray<FString> PlanetNameSuffixes;
	TArray<FString> SystemNameParts;
	void InitializeNameGenerators();

	// Seeded random helpers
	static uint32 HashSeed(int32 Seed);
	static float SeededRandom(int32 Seed);
	static int32 SeededRandomRange(int32 Seed, int32 Min, int32 Max);
	static float SeededRandomFloatRange(int32 Seed, float Min, float Max);
};
