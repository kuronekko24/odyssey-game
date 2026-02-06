// ProceduralPlanetManager.cpp
// Master coordinator implementation for Procedural Planet & Resource Generation

#include "ProceduralPlanetManager.h"

// ============================================================================
// CONSTRUCTOR & LIFECYCLE
// ============================================================================

UProceduralPlanetManager::UProceduralPlanetManager()
	: UniverseSeed(42)
	, MaxLoadedPlanets(20)
	, ExplorationGenerationDistance(5000.0f)
	, DefaultDiscoveriesPerPlanet(15)
	, PlanetGenerator(nullptr)
	, BiomeSystem(nullptr)
	, ResourceSystem(nullptr)
	, ExplorationSystem(nullptr)
	, bIsInitialized(false)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // Tick twice per second
}

void UProceduralPlanetManager::BeginPlay()
{
	Super::BeginPlay();

	if (!bIsInitialized)
	{
		InitializeProceduralSystem(UniverseSeed);
	}
}

void UProceduralPlanetManager::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Memory management: unload distant planets if over limit
	if (RegisteredPlanets.Num() > MaxLoadedPlanets)
	{
		// In a full implementation, we would track which planets are
		// closest to the player and unload the rest, keeping only
		// their seeds for regeneration. For now, we keep all loaded.
	}
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void UProceduralPlanetManager::InitializeProceduralSystem(int32 InUniverseSeed)
{
	if (bIsInitialized)
	{
		return;
	}

	UniverseSeed = InUniverseSeed;
	CreateSubsystems();
	bIsInitialized = true;
}

void UProceduralPlanetManager::CreateSubsystems()
{
	// Create biome system
	BiomeSystem = NewObject<UOdysseyBiomeDefinitionSystem>(this, TEXT("BiomeSystem"));
	BiomeSystem->Initialize(nullptr);

	// Create resource distribution system
	ResourceSystem = NewObject<UOdysseyResourceDistributionSystem>(this, TEXT("ResourceSystem"));
	ResourceSystem->Initialize(BiomeSystem);

	// Create planet generator
	PlanetGenerator = NewObject<UOdysseyPlanetGenerator>(this, TEXT("PlanetGenerator"));
	PlanetGenerator->Initialize(BiomeSystem, ResourceSystem);

	// Create exploration reward system
	ExplorationSystem = NewObject<UExplorationRewardSystem>(this, TEXT("ExplorationSystem"));
	ExplorationSystem->Initialize(BiomeSystem);
}

// ============================================================================
// PLANET GENERATION
// ============================================================================

FGeneratedPlanetData UProceduralPlanetManager::GenerateAndRegisterPlanet(int32 Seed, EPlanetSize PreferredSize)
{
	if (!bIsInitialized)
	{
		InitializeProceduralSystem(UniverseSeed);
	}

	FGeneratedPlanetData Planet = PlanetGenerator->GeneratePlanet(Seed, PreferredSize);

	// Register in our tracking map
	RegisteredPlanets.Add(Planet.PlanetID, Planet);

	return Planet;
}

FStarSystemData UProceduralPlanetManager::GenerateAndRegisterStarSystem(int32 Seed, int32 MinPlanets, int32 MaxPlanets)
{
	if (!bIsInitialized)
	{
		InitializeProceduralSystem(UniverseSeed);
	}

	FStarSystemData System = PlanetGenerator->GenerateStarSystem(Seed, MinPlanets, MaxPlanets);

	// Register system and all its planets
	RegisteredSystems.Add(System.SystemID, System);
	for (const FGeneratedPlanetData& Planet : System.Planets)
	{
		RegisteredPlanets.Add(Planet.PlanetID, Planet);
	}

	return System;
}

TArray<FStarSystemData> UProceduralPlanetManager::GenerateGalaxyRegion(
	int32 Seed,
	int32 SystemCount,
	const FVector& RegionCenter,
	float RegionRadius)
{
	if (!bIsInitialized)
	{
		InitializeProceduralSystem(UniverseSeed);
	}

	TArray<FStarSystemData> Systems = PlanetGenerator->GenerateGalaxyRegion(
		Seed, SystemCount, RegionCenter, RegionRadius);

	for (const FStarSystemData& System : Systems)
	{
		RegisteredSystems.Add(System.SystemID, System);
		for (const FGeneratedPlanetData& Planet : System.Planets)
		{
			RegisteredPlanets.Add(Planet.PlanetID, Planet);
		}
	}

	return Systems;
}

void UProceduralPlanetManager::GenerateExplorationContent(int32 PlanetID)
{
	if (PlanetsWithExploration.Contains(PlanetID))
	{
		return; // Already generated
	}

	const FGeneratedPlanetData* Planet = RegisteredPlanets.Find(PlanetID);
	if (!Planet)
	{
		return;
	}

	// Calculate discovery count based on planet size
	int32 DiscoveryCount = CalculateDiscoveryCount(*Planet);

	// Generate discoveries
	ExplorationSystem->GenerateDiscoveriesForPlanet(*Planet, DiscoveryCount);

	// Register planet in exploration tracker
	ExplorationSystem->RegisterPlanet(PlanetID, Planet->BiomeRegions.Num());

	PlanetsWithExploration.Add(PlanetID);
}

void UProceduralPlanetManager::GenerateEconomyForPlanet(AActor* PlanetActor, int32 PlanetID)
{
	if (!PlanetActor)
	{
		return;
	}

	const FGeneratedPlanetData* Planet = RegisteredPlanets.Find(PlanetID);
	if (!Planet)
	{
		return;
	}

	// Add or get economy component
	UOdysseyPlanetaryEconomyComponent* EconComp = PlanetActor->FindComponentByClass<UOdysseyPlanetaryEconomyComponent>();
	if (!EconComp)
	{
		EconComp = NewObject<UOdysseyPlanetaryEconomyComponent>(PlanetActor, TEXT("EconomyComponent"));
		EconComp->RegisterComponent();
	}

	EconComp->InitializeFromPlanetData(*Planet, Planet->GenerationSeed);
}

int32 UProceduralPlanetManager::CalculateDiscoveryCount(const FGeneratedPlanetData& Planet) const
{
	int32 BaseCount = DefaultDiscoveriesPerPlanet;

	// Scale with planet size
	switch (Planet.PlanetSize)
	{
	case EPlanetSize::Tiny:   BaseCount = FMath::Max(3, BaseCount / 3); break;
	case EPlanetSize::Small:  BaseCount = FMath::Max(5, BaseCount / 2); break;
	case EPlanetSize::Medium: break; // Default
	case EPlanetSize::Large:  BaseCount = static_cast<int32>(static_cast<float>(BaseCount) * 1.5f); break;
	case EPlanetSize::Huge:   BaseCount *= 2; break;
	case EPlanetSize::Giant:  BaseCount *= 3; break;
	}

	// Exotic planets get more discoveries
	if (Planet.PlanetType == EPlanetType::Exotic)
	{
		BaseCount = static_cast<int32>(static_cast<float>(BaseCount) * 1.5f);
	}

	return BaseCount;
}

// ============================================================================
// PLANET QUERIES
// ============================================================================

FGeneratedPlanetData UProceduralPlanetManager::GetPlanetData(int32 PlanetID) const
{
	if (const FGeneratedPlanetData* Planet = RegisteredPlanets.Find(PlanetID))
	{
		return *Planet;
	}
	return FGeneratedPlanetData();
}

bool UProceduralPlanetManager::DoesPlanetExist(int32 PlanetID) const
{
	return RegisteredPlanets.Contains(PlanetID);
}

TArray<int32> UProceduralPlanetManager::GetAllPlanetIDs() const
{
	TArray<int32> IDs;
	RegisteredPlanets.GetKeys(IDs);
	return IDs;
}

FStarSystemData UProceduralPlanetManager::GetStarSystem(int32 SystemID) const
{
	if (const FStarSystemData* System = RegisteredSystems.Find(SystemID))
	{
		return *System;
	}
	return FStarSystemData();
}

TArray<int32> UProceduralPlanetManager::GetAllSystemIDs() const
{
	TArray<int32> IDs;
	RegisteredSystems.GetKeys(IDs);
	return IDs;
}

TArray<FGeneratedPlanetData> UProceduralPlanetManager::FindPlanetsByType(EPlanetType Type) const
{
	TArray<FGeneratedPlanetData> Result;
	for (const auto& Pair : RegisteredPlanets)
	{
		if (Pair.Value.PlanetType == Type)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FGeneratedPlanetData> UProceduralPlanetManager::FindPlanetsWithBiome(EBiomeType Biome) const
{
	TArray<FGeneratedPlanetData> Result;
	for (const auto& Pair : RegisteredPlanets)
	{
		for (const FPlanetBiomeRegion& Region : Pair.Value.BiomeRegions)
		{
			if (Region.BiomeType == Biome)
			{
				Result.Add(Pair.Value);
				break;
			}
		}
	}
	return Result;
}

TArray<FGeneratedPlanetData> UProceduralPlanetManager::FindPlanetsWithResource(EResourceType Resource) const
{
	TArray<FGeneratedPlanetData> Result;
	for (const auto& Pair : RegisteredPlanets)
	{
		for (const FResourceDepositLocation& Deposit : Pair.Value.ResourceDeposits)
		{
			if (Deposit.ResourceType == Resource)
			{
				Result.Add(Pair.Value);
				break;
			}
		}
	}
	return Result;
}

// ============================================================================
// EXPLORATION
// ============================================================================

TArray<FScanResult> UProceduralPlanetManager::ScanPlanet(
	int32 PlanetID,
	const FVector& Origin,
	EScanMode Mode,
	float Power)
{
	// Ensure exploration content is generated
	GenerateExplorationContent(PlanetID);

	return ExplorationSystem->PerformScan(PlanetID, Origin, Mode, Power);
}

void UProceduralPlanetManager::UpdatePlayerExploration(
	int32 PlanetID,
	const FVector& PlayerLocation,
	float RevealRadius)
{
	// Ensure exploration content is generated
	GenerateExplorationContent(PlanetID);

	const FGeneratedPlanetData* Planet = RegisteredPlanets.Find(PlanetID);
	if (Planet)
	{
		ExplorationSystem->UpdateExploration(PlanetID, PlayerLocation, RevealRadius, Planet->WorldSize);
	}
}

bool UProceduralPlanetManager::TryDiscover(
	int32 PlanetID,
	const FVector& PlayerLocation,
	float Radius,
	FDiscoveryData& OutDiscovery)
{
	GenerateExplorationContent(PlanetID);
	return ExplorationSystem->TryDiscoverAtLocation(PlanetID, PlayerLocation, Radius, OutDiscovery);
}

bool UProceduralPlanetManager::ClaimRewards(
	int32 DiscoveryID,
	const FString& PlayerID,
	int32& OutOMEN,
	int32& OutXP,
	TArray<FResourceStack>& OutResources)
{
	return ExplorationSystem->ClaimDiscoveryRewards(DiscoveryID, PlayerID, OutOMEN, OutXP, OutResources);
}

// ============================================================================
// TRADE ROUTE ANALYSIS
// ============================================================================

TArray<FTradeRouteOpportunity> UProceduralPlanetManager::AnalyzeAllTradeRoutes() const
{
	if (!ResourceSystem)
	{
		return TArray<FTradeRouteOpportunity>();
	}

	// Build resource map per planet
	TMap<int32, TArray<FResourceDepositLocation>> PlanetResources;
	for (const auto& Pair : RegisteredPlanets)
	{
		PlanetResources.Add(Pair.Key, Pair.Value.ResourceDeposits);
	}

	return ResourceSystem->AnalyzeTradeOpportunities(PlanetResources);
}

FTradeRouteOpportunity UProceduralPlanetManager::FindBestTradeRoute(EResourceType Resource) const
{
	TArray<FTradeRouteOpportunity> AllRoutes = AnalyzeAllTradeRoutes();

	for (const FTradeRouteOpportunity& Route : AllRoutes)
	{
		if (Route.AbundantResource == Resource)
		{
			return Route;
		}
	}

	return FTradeRouteOpportunity();
}

TMap<EResourceType, float> UProceduralPlanetManager::GetUniverseResourceScarcity() const
{
	TMap<EResourceType, float> Scarcity;

	// Aggregate all deposits
	TArray<FResourceDepositLocation> AllDeposits;
	for (const auto& Pair : RegisteredPlanets)
	{
		AllDeposits.Append(Pair.Value.ResourceDeposits);
	}

	if (ResourceSystem && AllDeposits.Num() > 0)
	{
		TMap<EResourceType, float> Abundance = ResourceSystem->CalculateResourceAbundance(AllDeposits);
		for (const auto& Pair : Abundance)
		{
			Scarcity.Add(Pair.Key, 1.0f - Pair.Value);
		}
	}

	return Scarcity;
}

// ============================================================================
// PERSISTENCE
// ============================================================================

TArray<FPlanetSaveData> UProceduralPlanetManager::ExportSaveData() const
{
	TArray<FPlanetSaveData> SaveData;

	for (const auto& Pair : RegisteredPlanets)
	{
		const FGeneratedPlanetData& Planet = Pair.Value;

		FPlanetSaveData Data;
		Data.PlanetID = Planet.PlanetID;
		Data.GenerationSeed = Planet.GenerationSeed;
		Data.bDiscovered = Planet.bDiscovered;
		Data.ExplorationPercent = Planet.ExplorationProgress;

		// Export exploration state
		if (ExplorationSystem && PlanetsWithExploration.Contains(Planet.PlanetID))
		{
			ExplorationSystem->ExportPlanetSaveData(
				Planet.PlanetID,
				Data.DiscoveredDiscoveryIDs,
				Data.ClaimedDiscoveryIDs);
		}

		// Export depleted resources
		for (const FResourceDepositLocation& Deposit : Planet.ResourceDeposits)
		{
			if (Deposit.RemainingAmount < Deposit.TotalAmount)
			{
				Data.DepositRemainingAmounts.Add(Deposit.DepositID, Deposit.RemainingAmount);
			}
		}

		SaveData.Add(Data);
	}

	return SaveData;
}

void UProceduralPlanetManager::ImportSaveData(const TArray<FPlanetSaveData>& SaveData)
{
	for (const FPlanetSaveData& Data : SaveData)
	{
		// Regenerate planet from seed
		FGeneratedPlanetData Planet = PlanetGenerator->GeneratePlanet(Data.GenerationSeed);
		Planet.PlanetID = Data.PlanetID;
		Planet.bDiscovered = Data.bDiscovered;
		Planet.ExplorationProgress = Data.ExplorationPercent;

		// Restore resource depletion
		for (FResourceDepositLocation& Deposit : Planet.ResourceDeposits)
		{
			if (const int32* RemainingAmount = Data.DepositRemainingAmounts.Find(Deposit.DepositID))
			{
				Deposit.RemainingAmount = *RemainingAmount;
			}
		}

		RegisteredPlanets.Add(Planet.PlanetID, Planet);

		// Restore exploration state
		if (Data.DiscoveredDiscoveryIDs.Num() > 0 || Data.ClaimedDiscoveryIDs.Num() > 0)
		{
			GenerateExplorationContent(Planet.PlanetID);
			ExplorationSystem->ImportPlanetSaveData(
				Planet.PlanetID,
				Data.DiscoveredDiscoveryIDs,
				Data.ClaimedDiscoveryIDs);
		}
	}
}

