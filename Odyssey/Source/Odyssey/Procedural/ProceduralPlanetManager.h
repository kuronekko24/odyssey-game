// ProceduralPlanetManager.h
// Master coordinator for the Procedural Planet & Resource Generation System
// Ties together planet generation, biomes, resources, economy, and exploration

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProceduralTypes.h"
#include "OdysseyPlanetGenerator.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyResourceDistributionSystem.h"
#include "OdysseyPlanetaryEconomyComponent.h"
#include "ExplorationRewardSystem.h"
#include "ProceduralPlanetManager.generated.h"

/**
 * UProceduralPlanetManager
 *
 * The master coordinator for all procedural generation subsystems.
 * Manages the lifecycle of planets, star systems, and the galaxy.
 * Provides a unified API for generating, querying, and persisting
 * procedurally generated content.
 *
 * Designed for mobile performance with lazy generation, LOD support,
 * and minimal memory footprint through seed-based regeneration.
 */
UCLASS(ClassGroup=(Procedural), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UProceduralPlanetManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UProceduralPlanetManager();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========================================================================
	// INITIALIZATION
	// ========================================================================

	/** Initialize all subsystems */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Setup")
	void InitializeProceduralSystem(int32 UniverseSeed = 42);

	/** Check if system is initialized */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Setup")
	bool IsInitialized() const { return bIsInitialized; }

	// ========================================================================
	// PLANET GENERATION
	// ========================================================================

	/** Generate a new planet and register it in the universe */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Generation")
	FGeneratedPlanetData GenerateAndRegisterPlanet(int32 Seed, EPlanetSize PreferredSize = EPlanetSize::Medium);

	/** Generate a complete star system */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Generation")
	FStarSystemData GenerateAndRegisterStarSystem(int32 Seed, int32 MinPlanets = 2, int32 MaxPlanets = 8);

	/** Generate a galaxy region with multiple star systems */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Generation")
	TArray<FStarSystemData> GenerateGalaxyRegion(
		int32 Seed,
		int32 SystemCount,
		const FVector& RegionCenter,
		float RegionRadius);

	/** Generate exploration content for a planet (lazy loading) */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Generation")
	void GenerateExplorationContent(int32 PlanetID);

	/** Generate economy data for a planet */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Generation")
	void GenerateEconomyForPlanet(AActor* PlanetActor, int32 PlanetID);

	// ========================================================================
	// PLANET QUERIES
	// ========================================================================

	/** Get planet data by ID */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Query")
	FGeneratedPlanetData GetPlanetData(int32 PlanetID) const;

	/** Check if planet exists */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Query")
	bool DoesPlanetExist(int32 PlanetID) const;

	/** Get all registered planet IDs */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Query")
	TArray<int32> GetAllPlanetIDs() const;

	/** Get star system by ID */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Query")
	FStarSystemData GetStarSystem(int32 SystemID) const;

	/** Get all registered star system IDs */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Query")
	TArray<int32> GetAllSystemIDs() const;

	/** Find planets by type */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Query")
	TArray<FGeneratedPlanetData> FindPlanetsByType(EPlanetType Type) const;

	/** Find planets by biome */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Query")
	TArray<FGeneratedPlanetData> FindPlanetsWithBiome(EBiomeType Biome) const;

	/** Find planets by resource availability */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Query")
	TArray<FGeneratedPlanetData> FindPlanetsWithResource(EResourceType Resource) const;

	// ========================================================================
	// EXPLORATION
	// ========================================================================

	/** Get exploration system */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Exploration")
	UExplorationRewardSystem* GetExplorationSystem() const { return ExplorationSystem; }

	/** Perform scan on current planet */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Exploration")
	TArray<FScanResult> ScanPlanet(int32 PlanetID, const FVector& Origin, EScanMode Mode, float Power = 1.0f);

	/** Update player exploration on current planet */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Exploration")
	void UpdatePlayerExploration(int32 PlanetID, const FVector& PlayerLocation, float RevealRadius);

	/** Try to discover something at player location */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Exploration")
	bool TryDiscover(int32 PlanetID, const FVector& PlayerLocation, float Radius, FDiscoveryData& OutDiscovery);

	/** Claim discovery rewards */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Exploration")
	bool ClaimRewards(int32 DiscoveryID, const FString& PlayerID, int32& OutOMEN, int32& OutXP, TArray<FResourceStack>& OutResources);

	// ========================================================================
	// TRADE ROUTE ANALYSIS
	// ========================================================================

	/** Analyze trade opportunities between all known planets */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Trade")
	TArray<FTradeRouteOpportunity> AnalyzeAllTradeRoutes() const;

	/** Find best trade route for a specific resource */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Trade")
	FTradeRouteOpportunity FindBestTradeRoute(EResourceType Resource) const;

	/** Get resource scarcity across the known universe */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Trade")
	TMap<EResourceType, float> GetUniverseResourceScarcity() const;

	// ========================================================================
	// PERSISTENCE
	// ========================================================================

	/** Export minimal save data for all planets */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Save")
	TArray<FPlanetSaveData> ExportSaveData() const;

	/** Import save data and restore universe state */
	UFUNCTION(BlueprintCallable, Category = "Procedural|Save")
	void ImportSaveData(const TArray<FPlanetSaveData>& SaveData);

	// ========================================================================
	// SUBSYSTEM ACCESS
	// ========================================================================

	UFUNCTION(BlueprintCallable, Category = "Procedural|Systems")
	UOdysseyPlanetGenerator* GetPlanetGenerator() const { return PlanetGenerator; }

	UFUNCTION(BlueprintCallable, Category = "Procedural|Systems")
	UOdysseyBiomeDefinitionSystem* GetBiomeSystem() const { return BiomeSystem; }

	UFUNCTION(BlueprintCallable, Category = "Procedural|Systems")
	UOdysseyResourceDistributionSystem* GetResourceSystem() const { return ResourceSystem; }

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	/** Base universe seed (all generation derives from this) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 UniverseSeed;

	/** Maximum planets to keep fully loaded in memory */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Performance")
	int32 MaxLoadedPlanets;

	/** Distance at which to generate exploration content (lazy loading) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Performance")
	float ExplorationGenerationDistance;

	/** Default number of discoveries per planet */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Generation")
	int32 DefaultDiscoveriesPerPlanet;

protected:
	// Subsystem instances
	UPROPERTY()
	UOdysseyPlanetGenerator* PlanetGenerator;

	UPROPERTY()
	UOdysseyBiomeDefinitionSystem* BiomeSystem;

	UPROPERTY()
	UOdysseyResourceDistributionSystem* ResourceSystem;

	UPROPERTY()
	UExplorationRewardSystem* ExplorationSystem;

	// Universe state
	TMap<int32, FGeneratedPlanetData> RegisteredPlanets;
	TMap<int32, FStarSystemData> RegisteredSystems;

	// Planets with generated exploration content
	TSet<int32> PlanetsWithExploration;

	bool bIsInitialized;

private:
	void CreateSubsystems();
	int32 CalculateDiscoveryCount(const FGeneratedPlanetData& Planet) const;
};

