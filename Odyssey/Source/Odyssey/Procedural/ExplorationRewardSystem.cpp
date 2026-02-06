// ExplorationRewardSystem.cpp
// Implementation of discovery mechanics, scanning, and exploration rewards

#include "ExplorationRewardSystem.h"

// ============================================================================
// CONSTRUCTOR & INITIALIZATION
// ============================================================================

UExplorationRewardSystem::UExplorationRewardSystem()
	: BiomeDefinitionSystem(nullptr)
	, NextDiscoveryID(1)
{
	InitializeNamePools();
	InitializeBiomeDiscoveryAffinity();
}

void UExplorationRewardSystem::Initialize(UOdysseyBiomeDefinitionSystem* BiomeSystem)
{
	BiomeDefinitionSystem = BiomeSystem;
}

void UExplorationRewardSystem::InitializeNamePools()
{
	RuinsPrefixes = {
		TEXT("Ancient"), TEXT("Forgotten"), TEXT("Sunken"), TEXT("Crumbling"),
		TEXT("Hidden"), TEXT("Lost"), TEXT("Shattered"), TEXT("Buried"),
		TEXT("Overgrown"), TEXT("Petrified"), TEXT("Crystallized"), TEXT("Fossilized")
	};

	ArtifactNames = {
		TEXT("Resonance Core"), TEXT("Phase Prism"), TEXT("Void Compass"),
		TEXT("Stellar Fragment"), TEXT("Quantum Shard"), TEXT("Neural Matrix"),
		TEXT("Temporal Lens"), TEXT("Gravity Seed"), TEXT("Harmonic Crystal"),
		TEXT("Dark Matter Capsule"), TEXT("Plasma Conduit"), TEXT("Zero-Point Cell"),
		TEXT("Precursor Tablet"), TEXT("Entropy Key"), TEXT("Singularity Map")
	};

	WonderAdjectives = {
		TEXT("Towering"), TEXT("Luminous"), TEXT("Crystalline"), TEXT("Floating"),
		TEXT("Enormous"), TEXT("Iridescent"), TEXT("Pulsating"), TEXT("Spiral"),
		TEXT("Cascading"), TEXT("Prismatic"), TEXT("Colossal"), TEXT("Ethereal")
	};

	OutpostDesignations = {
		TEXT("Sigma"), TEXT("Theta"), TEXT("Lambda"), TEXT("Omega"),
		TEXT("Zeta"), TEXT("Epsilon"), TEXT("Kappa"), TEXT("Rho"),
		TEXT("Tau"), TEXT("Psi"), TEXT("Phi"), TEXT("Chi")
	};
}

void UExplorationRewardSystem::InitializeBiomeDiscoveryAffinity()
{
	BiomeDiscoveryAffinity.Empty();

	BiomeDiscoveryAffinity.Add(EBiomeType::Desert, {
		EDiscoveryType::AncientRuins, EDiscoveryType::HiddenCache,
		EDiscoveryType::RareMineral, EDiscoveryType::AbandonedOutpost,
		EDiscoveryType::AlienArtifact
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Ice, {
		EDiscoveryType::FrozenOrganism, EDiscoveryType::CrystalFormation,
		EDiscoveryType::WreckedShip, EDiscoveryType::AnomalousSignal,
		EDiscoveryType::HiddenCache
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Forest, {
		EDiscoveryType::BiologicalSpecimen, EDiscoveryType::NaturalWonder,
		EDiscoveryType::AncientRuins, EDiscoveryType::ResourceDeposit,
		EDiscoveryType::HiddenCache
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Volcanic, {
		EDiscoveryType::GeothermalVent, EDiscoveryType::RareMineral,
		EDiscoveryType::ResourceDeposit, EDiscoveryType::AnomalousSignal,
		EDiscoveryType::PrecursorTechnology
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Ocean, {
		EDiscoveryType::WreckedShip, EDiscoveryType::BiologicalSpecimen,
		EDiscoveryType::NaturalWonder, EDiscoveryType::AncientRuins,
		EDiscoveryType::HiddenCache
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Crystalline, {
		EDiscoveryType::CrystalFormation, EDiscoveryType::RareMineral,
		EDiscoveryType::QuantumAnomaly, EDiscoveryType::AlienArtifact,
		EDiscoveryType::PrecursorTechnology
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Toxic, {
		EDiscoveryType::BiologicalSpecimen, EDiscoveryType::AbandonedOutpost,
		EDiscoveryType::ResourceDeposit, EDiscoveryType::HiddenCache,
		EDiscoveryType::WreckedShip
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Barren, {
		EDiscoveryType::WreckedShip, EDiscoveryType::AbandonedOutpost,
		EDiscoveryType::ResourceDeposit, EDiscoveryType::AncientRuins,
		EDiscoveryType::RareMineral
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Lush, {
		EDiscoveryType::BiologicalSpecimen, EDiscoveryType::NaturalWonder,
		EDiscoveryType::AncientRuins, EDiscoveryType::ResourceDeposit,
		EDiscoveryType::AlienArtifact
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Radioactive, {
		EDiscoveryType::AnomalousSignal, EDiscoveryType::QuantumAnomaly,
		EDiscoveryType::PrecursorTechnology, EDiscoveryType::RareMineral,
		EDiscoveryType::WreckedShip
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Metallic, {
		EDiscoveryType::ResourceDeposit, EDiscoveryType::RareMineral,
		EDiscoveryType::AbandonedOutpost, EDiscoveryType::PrecursorTechnology,
		EDiscoveryType::WreckedShip
	});

	BiomeDiscoveryAffinity.Add(EBiomeType::Anomalous, {
		EDiscoveryType::QuantumAnomaly, EDiscoveryType::PrecursorTechnology,
		EDiscoveryType::AlienArtifact, EDiscoveryType::AnomalousSignal,
		EDiscoveryType::NaturalWonder
	});
}

// ============================================================================
// DISCOVERY GENERATION
// ============================================================================

TArray<FDiscoveryData> UExplorationRewardSystem::GenerateDiscoveriesForPlanet(
	const FGeneratedPlanetData& PlanetData,
	int32 DiscoveryCount)
{
	TArray<FDiscoveryData> AllDiscoveries;

	if (PlanetData.BiomeRegions.Num() == 0)
	{
		return AllDiscoveries;
	}

	// Distribute discoveries across biome regions proportionally
	int32 DiscoveriesPerRegion = FMath::Max(1, DiscoveryCount / PlanetData.BiomeRegions.Num());
	int32 ExtraDiscoveries = DiscoveryCount - (DiscoveriesPerRegion * PlanetData.BiomeRegions.Num());

	for (int32 i = 0; i < PlanetData.BiomeRegions.Num(); ++i)
	{
		const FPlanetBiomeRegion& Region = PlanetData.BiomeRegions[i];
		int32 RegionCount = DiscoveriesPerRegion + (i < ExtraDiscoveries ? 1 : 0);

		TArray<FDiscoveryData> RegionDiscoveries = GenerateDiscoveriesForRegion(
			PlanetData.GenerationSeed + i * 777,
			PlanetData.PlanetID,
			Region,
			RegionCount);

		AllDiscoveries.Append(RegionDiscoveries);
	}

	// Store in our tracking map
	PlanetDiscoveries.Add(PlanetData.PlanetID, AllDiscoveries);

	return AllDiscoveries;
}

FDiscoveryData UExplorationRewardSystem::GenerateDiscovery(
	int32 Seed,
	int32 PlanetID,
	const FVector& Location,
	EBiomeType Biome)
{
	FDiscoveryData Discovery;
	Discovery.DiscoveryID = NextDiscoveryID++;
	Discovery.PlanetID = PlanetID;
	Discovery.WorldLocation = Location;

	// Determine type and rarity
	Discovery.DiscoveryType = SelectDiscoveryType(Seed, Biome);
	Discovery.Rarity = DetermineDiscoveryRarity(Seed + 100, Biome);

	// Generate text content
	Discovery.Name = GenerateDiscoveryName(Seed + 200, Discovery.DiscoveryType, Biome);
	Discovery.Description = GenerateDiscoveryDescription(Discovery.DiscoveryType, Discovery.Rarity, Biome);
	Discovery.LoreText = GenerateLoreText(Seed + 300, Discovery.DiscoveryType);

	// Calculate rewards
	Discovery.OMENReward = CalculateDiscoveryOMENValue(Discovery.DiscoveryType, Discovery.Rarity);
	Discovery.ExperienceReward = Discovery.OMENReward * 2;
	Discovery.ResourceRewards = GenerateResourceRewards(Seed + 400, Discovery.DiscoveryType, Discovery.Rarity, Biome);

	// Blueprint unlock for epic+ discoveries
	if (Discovery.Rarity >= EDiscoveryRarity::Epic)
	{
		FString UnlockName = FString::Printf(TEXT("Blueprint_%s_%d"),
			*Discovery.Name.ToString().Replace(TEXT(" "), TEXT("_")),
			Discovery.DiscoveryID);
		Discovery.BlueprintUnlock = FName(*UnlockName);
	}

	// Scan requirements scale with rarity
	Discovery.RequiredScanMode = DetermineScanRequirement(Discovery.Rarity);
	Discovery.ScanDifficulty = 0.5f + static_cast<float>(static_cast<uint8>(Discovery.Rarity)) * 0.3f;
	Discovery.DetectionRadius = 300.0f - static_cast<float>(static_cast<uint8>(Discovery.Rarity)) * 40.0f;
	Discovery.DetectionRadius = FMath::Max(Discovery.DetectionRadius, 50.0f);

	Discovery.bDiscovered = false;
	Discovery.bClaimed = false;

	return Discovery;
}

TArray<FDiscoveryData> UExplorationRewardSystem::GenerateDiscoveriesForRegion(
	int32 Seed,
	int32 PlanetID,
	const FPlanetBiomeRegion& Region,
	int32 Count)
{
	TArray<FDiscoveryData> Discoveries;

	FVector2D RegionMin = Region.WorldMin;
	FVector2D RegionMax = Region.WorldMax;
	float RegionWidth = RegionMax.X - RegionMin.X;
	float RegionHeight = RegionMax.Y - RegionMin.Y;

	// Minimum spacing between discoveries
	float MinSpacing = FMath::Max(100.0f, FMath::Sqrt(RegionWidth * RegionHeight / static_cast<float>(Count + 1)) * 0.5f);

	TArray<FVector> PlacedLocations;

	for (int32 i = 0; i < Count; ++i)
	{
		int32 LocalSeed = Seed + i * 137;
		FVector Location = FVector::ZeroVector;
		bool bValidLocation = false;

		// Try to find a valid location with spacing constraints
		for (int32 Attempt = 0; Attempt < 30; ++Attempt)
		{
			float X = RegionMin.X + SeededRandom(LocalSeed + Attempt * 2) * RegionWidth;
			float Y = RegionMin.Y + SeededRandom(LocalSeed + Attempt * 2 + 1) * RegionHeight;
			FVector Candidate(X, Y, 0.0f);

			bValidLocation = true;
			for (const FVector& Placed : PlacedLocations)
			{
				if (FVector::Dist2D(Candidate, Placed) < MinSpacing)
				{
					bValidLocation = false;
					break;
				}
			}

			if (bValidLocation)
			{
				Location = Candidate;
				break;
			}
		}

		if (!bValidLocation)
		{
			// Fallback: place randomly even if spacing is violated
			float X = RegionMin.X + SeededRandom(LocalSeed + 999) * RegionWidth;
			float Y = RegionMin.Y + SeededRandom(LocalSeed + 1000) * RegionHeight;
			Location = FVector(X, Y, 0.0f);
		}

		PlacedLocations.Add(Location);

		FDiscoveryData Discovery = GenerateDiscovery(LocalSeed, PlanetID, Location, Region.BiomeType);
		Discoveries.Add(Discovery);
	}

	return Discoveries;
}

// ============================================================================
// SCANNING
// ============================================================================

TArray<FScanResult> UExplorationRewardSystem::PerformScan(
	int32 PlanetID,
	const FVector& ScanOrigin,
	EScanMode ScanMode,
	float ScannerPower)
{
	TArray<FScanResult> Results;

	const TArray<FDiscoveryData>* PlanetDiscs = PlanetDiscoveries.Find(PlanetID);
	if (!PlanetDiscs)
	{
		return Results;
	}

	float ScanRadius = GetEffectiveScanRadius(ScanMode, ScannerPower);

	for (const FDiscoveryData& Discovery : *PlanetDiscs)
	{
		if (Discovery.bDiscovered)
		{
			continue; // Already discovered, skip
		}

		float Distance = FVector::Dist(ScanOrigin, Discovery.WorldLocation);

		if (CanDetectDiscovery(Discovery, ScanMode, Distance, ScannerPower))
		{
			FScanResult Result;
			Result.bFoundSomething = true;
			Result.DiscoveryID = Discovery.DiscoveryID;
			Result.DistanceToDiscovery = Distance;

			// Signal strength falls off with distance
			Result.SignalStrength = FMath::Clamp(1.0f - (Distance / ScanRadius), 0.0f, 1.0f);

			// Accuracy of type/rarity hints depends on signal strength
			if (Result.SignalStrength > 0.7f)
			{
				Result.HintedType = Discovery.DiscoveryType;
				Result.HintedRarity = Discovery.Rarity;
				Result.SignalLocation = Discovery.WorldLocation;
			}
			else if (Result.SignalStrength > 0.4f)
			{
				Result.HintedType = Discovery.DiscoveryType;
				Result.HintedRarity = EDiscoveryRarity::Common; // Rarity hidden
				// Fuzzy location
				float Fuzz = Distance * 0.1f;
				Result.SignalLocation = Discovery.WorldLocation + FVector(
					FMath::RandRange(-Fuzz, Fuzz),
					FMath::RandRange(-Fuzz, Fuzz),
					0.0f);
			}
			else
			{
				Result.HintedType = EDiscoveryType::None; // Type hidden
				Result.HintedRarity = EDiscoveryRarity::Common;
				// Very fuzzy location
				float Fuzz = Distance * 0.3f;
				Result.SignalLocation = Discovery.WorldLocation + FVector(
					FMath::RandRange(-Fuzz, Fuzz),
					FMath::RandRange(-Fuzz, Fuzz),
					0.0f);
			}

			Results.Add(Result);
		}
	}

	// Sort by signal strength descending
	Results.Sort([](const FScanResult& A, const FScanResult& B)
	{
		return A.SignalStrength > B.SignalStrength;
	});

	// Broadcast results
	for (const FScanResult& Result : Results)
	{
		OnScanComplete.Broadcast(Result);
	}

	return Results;
}

float UExplorationRewardSystem::GetEffectiveScanRadius(EScanMode Mode, float ScannerPower) const
{
	float BaseRadius = 0.0f;

	switch (Mode)
	{
	case EScanMode::Passive:
		BaseRadius = 200.0f;
		break;
	case EScanMode::ActiveShort:
		BaseRadius = 500.0f;
		break;
	case EScanMode::ActiveLong:
		BaseRadius = 1500.0f;
		break;
	case EScanMode::Deep:
		BaseRadius = 800.0f;
		break;
	case EScanMode::Anomaly:
		BaseRadius = 2000.0f;
		break;
	}

	return BaseRadius * FMath::Max(0.1f, ScannerPower);
}

bool UExplorationRewardSystem::CanDetectDiscovery(
	const FDiscoveryData& Discovery,
	EScanMode Mode,
	float Distance,
	float ScannerPower) const
{
	float ScanRadius = GetEffectiveScanRadius(Mode, ScannerPower);

	if (Distance > ScanRadius)
	{
		return false;
	}

	// Check scan mode capability
	int32 ModeLevel = static_cast<int32>(Mode);
	int32 RequiredLevel = static_cast<int32>(Discovery.RequiredScanMode);

	// Anomaly scanner can detect anomaly-type discoveries at any level
	if (Mode == EScanMode::Anomaly &&
		(Discovery.DiscoveryType == EDiscoveryType::QuantumAnomaly ||
		 Discovery.DiscoveryType == EDiscoveryType::AnomalousSignal))
	{
		return true;
	}

	// Deep scan can find anything within range
	if (Mode == EScanMode::Deep)
	{
		return true;
	}

	// Other modes must meet minimum level
	if (ModeLevel < RequiredLevel)
	{
		return false;
	}

	// Scanner power affects detection probability at longer distances
	float DetectionChance = ScannerPower * (1.0f - (Distance / ScanRadius) * 0.5f);
	DetectionChance -= Discovery.ScanDifficulty * 0.2f;

	return DetectionChance > 0.3f;
}

// ============================================================================
// DISCOVERY CLAIMING
// ============================================================================

bool UExplorationRewardSystem::TryDiscoverAtLocation(
	int32 PlanetID,
	const FVector& PlayerLocation,
	float InteractionRadius,
	FDiscoveryData& OutDiscovery)
{
	TArray<FDiscoveryData>* PlanetDiscs = PlanetDiscoveries.Find(PlanetID);
	if (!PlanetDiscs)
	{
		return false;
	}

	float ClosestDist = FLT_MAX;
	int32 ClosestIndex = -1;

	for (int32 i = 0; i < PlanetDiscs->Num(); ++i)
	{
		FDiscoveryData& Disc = (*PlanetDiscs)[i];
		if (Disc.bDiscovered)
		{
			continue;
		}

		float Dist = FVector::Dist(PlayerLocation, Disc.WorldLocation);
		if (Dist <= InteractionRadius && Dist < ClosestDist)
		{
			ClosestDist = Dist;
			ClosestIndex = i;
		}
	}

	if (ClosestIndex >= 0)
	{
		FDiscoveryData& Disc = (*PlanetDiscs)[ClosestIndex];
		Disc.bDiscovered = true;
		Disc.DiscoveredTimestamp = FPlatformTime::Seconds();
		OutDiscovery = Disc;

		// Update exploration progress
		FPlanetExplorationData* ExplData = ExplorationProgress.Find(PlanetID);
		if (ExplData)
		{
			ExplData->TotalDiscoveries++;
		}

		OnDiscoveryMade.Broadcast(OutDiscovery);
		return true;
	}

	return false;
}

bool UExplorationRewardSystem::ClaimDiscoveryRewards(
	int32 DiscoveryID,
	const FString& PlayerID,
	int32& OutOMEN,
	int32& OutExperience,
	TArray<FResourceStack>& OutResources)
{
	// Search across all planets for the discovery
	for (auto& Pair : PlanetDiscoveries)
	{
		for (FDiscoveryData& Disc : Pair.Value)
		{
			if (Disc.DiscoveryID == DiscoveryID)
			{
				if (!Disc.bDiscovered)
				{
					return false; // Must be discovered first
				}
				if (Disc.bClaimed)
				{
					return false; // Already claimed
				}

				Disc.bClaimed = true;
				Disc.DiscovererPlayerID = PlayerID;

				OutOMEN = Disc.OMENReward;
				OutExperience = Disc.ExperienceReward;
				OutResources = Disc.ResourceRewards;

				// Update exploration data
				FPlanetExplorationData* ExplData = ExplorationProgress.Find(Disc.PlanetID);
				if (ExplData)
				{
					ExplData->ClaimedDiscoveries++;
				}

				OnDiscoveryClaimed.Broadcast(Disc);

				// Check milestones
				CheckMilestones(Disc.PlanetID);

				return true;
			}
		}
	}

	return false;
}

bool UExplorationRewardSystem::IsDiscoveryClaimed(int32 DiscoveryID) const
{
	for (const auto& Pair : PlanetDiscoveries)
	{
		for (const FDiscoveryData& Disc : Pair.Value)
		{
			if (Disc.DiscoveryID == DiscoveryID)
			{
				return Disc.bClaimed;
			}
		}
	}
	return false;
}

bool UExplorationRewardSystem::IsDiscoveryRevealed(int32 DiscoveryID) const
{
	for (const auto& Pair : PlanetDiscoveries)
	{
		for (const FDiscoveryData& Disc : Pair.Value)
		{
			if (Disc.DiscoveryID == DiscoveryID)
			{
				return Disc.bDiscovered;
			}
		}
	}
	return false;
}

// ============================================================================
// EXPLORATION PROGRESS
// ============================================================================

void UExplorationRewardSystem::RegisterPlanet(int32 PlanetID, int32 TotalRegions, int32 GridResolution)
{
	if (ExplorationProgress.Contains(PlanetID))
	{
		return;
	}

	FPlanetExplorationData Data;
	Data.PlanetID = PlanetID;
	Data.TotalRegions = TotalRegions;
	Data.GridResolution = GridResolution;
	Data.Status = EExplorationStatus::Uncharted;
	Data.ExplorationPercent = 0.0f;
	Data.FirstVisitTime = FPlatformTime::Seconds();
	Data.LastVisitTime = Data.FirstVisitTime;

	// Initialize grid (all unexplored)
	Data.ExploredGrid.SetNum(GridResolution * GridResolution);
	for (int32 i = 0; i < Data.ExploredGrid.Num(); ++i)
	{
		Data.ExploredGrid[i] = false;
	}

	ExplorationProgress.Add(PlanetID, Data);

	// Initialize milestones
	InitializeDefaultMilestones(PlanetID);
}

void UExplorationRewardSystem::UpdateExploration(
	int32 PlanetID,
	const FVector& PlayerLocation,
	float RevealRadius,
	const FVector2D& WorldSize)
{
	FPlanetExplorationData* Data = ExplorationProgress.Find(PlanetID);
	if (!Data)
	{
		return;
	}

	Data->LastVisitTime = FPlatformTime::Seconds();

	// Convert world location to grid coordinates
	int32 GridRes = Data->GridResolution;
	float CellWidth = WorldSize.X / static_cast<float>(GridRes);
	float CellHeight = WorldSize.Y / static_cast<float>(GridRes);

	int32 CenterGridX = FMath::Clamp(static_cast<int32>(PlayerLocation.X / CellWidth), 0, GridRes - 1);
	int32 CenterGridY = FMath::Clamp(static_cast<int32>(PlayerLocation.Y / CellHeight), 0, GridRes - 1);
	int32 GridRadius = FMath::CeilToInt32(RevealRadius / FMath::Min(CellWidth, CellHeight));

	bool bNewCellRevealed = false;

	// Reveal cells within radius
	for (int32 dy = -GridRadius; dy <= GridRadius; ++dy)
	{
		for (int32 dx = -GridRadius; dx <= GridRadius; ++dx)
		{
			int32 GX = CenterGridX + dx;
			int32 GY = CenterGridY + dy;

			if (GX < 0 || GX >= GridRes || GY < 0 || GY >= GridRes)
			{
				continue;
			}

			// Check if within circular radius
			float DistSq = static_cast<float>(dx * dx + dy * dy);
			if (DistSq > static_cast<float>(GridRadius * GridRadius))
			{
				continue;
			}

			int32 Index = GY * GridRes + GX;
			if (!Data->ExploredGrid[Index])
			{
				Data->ExploredGrid[Index] = true;
				bNewCellRevealed = true;
			}
		}
	}

	if (bNewCellRevealed)
	{
		// Recalculate exploration percentage
		int32 ExploredCount = 0;
		for (bool bExplored : Data->ExploredGrid)
		{
			if (bExplored) ExploredCount++;
		}

		float OldPercent = Data->ExplorationPercent;
		Data->ExplorationPercent = (static_cast<float>(ExploredCount) / static_cast<float>(Data->ExploredGrid.Num())) * 100.0f;

		// Update status
		if (Data->ExplorationPercent >= 100.0f)
		{
			Data->Status = EExplorationStatus::FullyExplored;
		}
		else if (Data->ExplorationPercent >= 75.0f)
		{
			Data->Status = EExplorationStatus::MostlyExplored;
		}
		else if (Data->ExplorationPercent >= 25.0f)
		{
			Data->Status = EExplorationStatus::PartiallyExplored;
		}
		else if (Data->ExplorationPercent > 0.0f)
		{
			Data->Status = EExplorationStatus::Surveyed;
		}

		// Broadcast progress if significant change
		if (FMath::Abs(Data->ExplorationPercent - OldPercent) >= 0.5f)
		{
			OnExplorationProgress.Broadcast(PlanetID, Data->ExplorationPercent);
		}

		// Check milestones
		CheckMilestones(PlanetID);
	}
}

FPlanetExplorationData UExplorationRewardSystem::GetExplorationData(int32 PlanetID) const
{
	if (const FPlanetExplorationData* Data = ExplorationProgress.Find(PlanetID))
	{
		return *Data;
	}
	return FPlanetExplorationData();
}

EExplorationStatus UExplorationRewardSystem::GetExplorationStatus(int32 PlanetID) const
{
	if (const FPlanetExplorationData* Data = ExplorationProgress.Find(PlanetID))
	{
		return Data->Status;
	}
	return EExplorationStatus::Uncharted;
}

float UExplorationRewardSystem::GetExplorationPercent(int32 PlanetID) const
{
	if (const FPlanetExplorationData* Data = ExplorationProgress.Find(PlanetID))
	{
		return Data->ExplorationPercent;
	}
	return 0.0f;
}

void UExplorationRewardSystem::MarkRegionExplored(int32 PlanetID, int32 RegionIndex, EBiomeType BiomeType)
{
	FPlanetExplorationData* Data = ExplorationProgress.Find(PlanetID);
	if (!Data)
	{
		return;
	}

	Data->RegionsExplored = FMath::Min(Data->RegionsExplored + 1, Data->TotalRegions);

	if (!Data->DiscoveredBiomes.Contains(BiomeType))
	{
		Data->DiscoveredBiomes.Add(BiomeType);
	}
}

// ============================================================================
// MILESTONES
// ============================================================================

void UExplorationRewardSystem::InitializeDefaultMilestones(int32 PlanetID)
{
	TArray<FExplorationMilestone> Milestones;

	// First steps
	{
		FExplorationMilestone M;
		M.MilestoneID = FName(TEXT("FirstSteps"));
		M.Name = FText::FromString(TEXT("First Steps"));
		M.Description = FText::FromString(TEXT("Begin exploring this planet."));
		M.RequiredExplorationPercent = 5.0f;
		M.RequiredDiscoveryCount = 0;
		M.OMENReward = 100;
		M.ExperienceReward = 200;
		M.bCompleted = false;
		Milestones.Add(M);
	}

	// Surveyor
	{
		FExplorationMilestone M;
		M.MilestoneID = FName(TEXT("Surveyor"));
		M.Name = FText::FromString(TEXT("Surveyor"));
		M.Description = FText::FromString(TEXT("Survey 25% of the planet and make 3 discoveries."));
		M.RequiredExplorationPercent = 25.0f;
		M.RequiredDiscoveryCount = 3;
		M.OMENReward = 500;
		M.ExperienceReward = 750;
		M.bCompleted = false;
		Milestones.Add(M);
	}

	// Explorer
	{
		FExplorationMilestone M;
		M.MilestoneID = FName(TEXT("Explorer"));
		M.Name = FText::FromString(TEXT("Explorer"));
		M.Description = FText::FromString(TEXT("Explore 50% of the planet and make 7 discoveries."));
		M.RequiredExplorationPercent = 50.0f;
		M.RequiredDiscoveryCount = 7;
		M.OMENReward = 1500;
		M.ExperienceReward = 2000;
		M.bCompleted = false;
		Milestones.Add(M);
	}

	// Cartographer
	{
		FExplorationMilestone M;
		M.MilestoneID = FName(TEXT("Cartographer"));
		M.Name = FText::FromString(TEXT("Cartographer"));
		M.Description = FText::FromString(TEXT("Map 75% of the planet and make 12 discoveries."));
		M.RequiredExplorationPercent = 75.0f;
		M.RequiredDiscoveryCount = 12;
		M.OMENReward = 3000;
		M.ExperienceReward = 5000;
		M.bCompleted = false;
		Milestones.Add(M);
	}

	// Planetmaster
	{
		FExplorationMilestone M;
		M.MilestoneID = FName(TEXT("Planetmaster"));
		M.Name = FText::FromString(TEXT("Planetmaster"));
		M.Description = FText::FromString(TEXT("Fully explore the planet and claim all discoveries."));
		M.RequiredExplorationPercent = 100.0f;
		M.RequiredDiscoveryCount = -1; // All discoveries
		M.OMENReward = 10000;
		M.ExperienceReward = 15000;
		M.bCompleted = false;
		Milestones.Add(M);
	}

	PlanetMilestones.Add(PlanetID, Milestones);
}

TArray<FExplorationMilestone> UExplorationRewardSystem::GetMilestones(int32 PlanetID) const
{
	if (const TArray<FExplorationMilestone>* MS = PlanetMilestones.Find(PlanetID))
	{
		return *MS;
	}
	return TArray<FExplorationMilestone>();
}

TArray<FExplorationMilestone> UExplorationRewardSystem::GetPendingMilestones(int32 PlanetID) const
{
	TArray<FExplorationMilestone> Pending;
	if (const TArray<FExplorationMilestone>* MS = PlanetMilestones.Find(PlanetID))
	{
		for (const FExplorationMilestone& M : *MS)
		{
			if (!M.bCompleted)
			{
				Pending.Add(M);
			}
		}
	}
	return Pending;
}

TArray<FExplorationMilestone> UExplorationRewardSystem::CheckMilestones(int32 PlanetID)
{
	TArray<FExplorationMilestone> NewlyCompleted;

	TArray<FExplorationMilestone>* MS = PlanetMilestones.Find(PlanetID);
	FPlanetExplorationData* ExplData = ExplorationProgress.Find(PlanetID);
	if (!MS || !ExplData)
	{
		return NewlyCompleted;
	}

	for (FExplorationMilestone& M : *MS)
	{
		if (M.bCompleted)
		{
			continue;
		}

		bool bExplorationMet = ExplData->ExplorationPercent >= M.RequiredExplorationPercent;

		bool bDiscoveryMet = true;
		if (M.RequiredDiscoveryCount > 0)
		{
			bDiscoveryMet = ExplData->ClaimedDiscoveries >= M.RequiredDiscoveryCount;
		}
		else if (M.RequiredDiscoveryCount == -1)
		{
			// Requires all discoveries
			const TArray<FDiscoveryData>* Discs = PlanetDiscoveries.Find(PlanetID);
			if (Discs)
			{
				bDiscoveryMet = ExplData->ClaimedDiscoveries >= Discs->Num();
			}
		}

		if (bExplorationMet && bDiscoveryMet)
		{
			M.bCompleted = true;
			NewlyCompleted.Add(M);
			OnMilestoneReached.Broadcast(M);
		}
	}

	// Check for mastered status
	bool bAllMilestonesComplete = true;
	for (const FExplorationMilestone& M : *MS)
	{
		if (!M.bCompleted)
		{
			bAllMilestonesComplete = false;
			break;
		}
	}
	if (bAllMilestonesComplete)
	{
		ExplData->Status = EExplorationStatus::Mastered;
	}

	return NewlyCompleted;
}

// ============================================================================
// QUERIES
// ============================================================================

TArray<FDiscoveryData> UExplorationRewardSystem::GetPlanetDiscoveries(int32 PlanetID) const
{
	if (const TArray<FDiscoveryData>* Discs = PlanetDiscoveries.Find(PlanetID))
	{
		return *Discs;
	}
	return TArray<FDiscoveryData>();
}

TArray<FDiscoveryData> UExplorationRewardSystem::GetUndiscoveredItems(int32 PlanetID) const
{
	TArray<FDiscoveryData> Result;
	if (const TArray<FDiscoveryData>* Discs = PlanetDiscoveries.Find(PlanetID))
	{
		for (const FDiscoveryData& D : *Discs)
		{
			if (!D.bDiscovered)
			{
				Result.Add(D);
			}
		}
	}
	return Result;
}

TArray<FDiscoveryData> UExplorationRewardSystem::GetDiscoveriesByType(int32 PlanetID, EDiscoveryType Type) const
{
	TArray<FDiscoveryData> Result;
	if (const TArray<FDiscoveryData>* Discs = PlanetDiscoveries.Find(PlanetID))
	{
		for (const FDiscoveryData& D : *Discs)
		{
			if (D.DiscoveryType == Type)
			{
				Result.Add(D);
			}
		}
	}
	return Result;
}

TArray<FDiscoveryData> UExplorationRewardSystem::GetDiscoveriesByRarity(int32 PlanetID, EDiscoveryRarity MinRarity) const
{
	TArray<FDiscoveryData> Result;
	if (const TArray<FDiscoveryData>* Discs = PlanetDiscoveries.Find(PlanetID))
	{
		for (const FDiscoveryData& D : *Discs)
		{
			if (D.Rarity >= MinRarity)
			{
				Result.Add(D);
			}
		}
	}
	return Result;
}

FDiscoveryData UExplorationRewardSystem::FindNearestUndiscovered(
	int32 PlanetID,
	const FVector& FromLocation,
	float MaxDistance) const
{
	FDiscoveryData Nearest;
	float BestDist = (MaxDistance > 0.0f) ? MaxDistance : FLT_MAX;

	if (const TArray<FDiscoveryData>* Discs = PlanetDiscoveries.Find(PlanetID))
	{
		for (const FDiscoveryData& D : *Discs)
		{
			if (D.bDiscovered)
			{
				continue;
			}

			float Dist = FVector::Dist(FromLocation, D.WorldLocation);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				Nearest = D;
			}
		}
	}

	return Nearest;
}

int32 UExplorationRewardSystem::GetTotalExplorationRewards(int32 PlanetID) const
{
	int32 Total = 0;
	if (const TArray<FDiscoveryData>* Discs = PlanetDiscoveries.Find(PlanetID))
	{
		for (const FDiscoveryData& D : *Discs)
		{
			if (D.bClaimed)
			{
				Total += D.OMENReward;
			}
		}
	}

	// Add milestone rewards
	if (const TArray<FExplorationMilestone>* MS = PlanetMilestones.Find(PlanetID))
	{
		for (const FExplorationMilestone& M : *MS)
		{
			if (M.bCompleted)
			{
				Total += M.OMENReward;
			}
		}
	}

	return Total;
}

// ============================================================================
// SERIALIZATION
// ============================================================================

void UExplorationRewardSystem::ExportPlanetSaveData(
	int32 PlanetID,
	TArray<int32>& OutDiscoveredIDs,
	TArray<int32>& OutClaimedIDs) const
{
	OutDiscoveredIDs.Empty();
	OutClaimedIDs.Empty();

	if (const TArray<FDiscoveryData>* Discs = PlanetDiscoveries.Find(PlanetID))
	{
		for (const FDiscoveryData& D : *Discs)
		{
			if (D.bDiscovered)
			{
				OutDiscoveredIDs.Add(D.DiscoveryID);
			}
			if (D.bClaimed)
			{
				OutClaimedIDs.Add(D.DiscoveryID);
			}
		}
	}
}

void UExplorationRewardSystem::ImportPlanetSaveData(
	int32 PlanetID,
	const TArray<int32>& DiscoveredIDs,
	const TArray<int32>& ClaimedIDs)
{
	TArray<FDiscoveryData>* Discs = PlanetDiscoveries.Find(PlanetID);
	if (!Discs)
	{
		return;
	}

	for (FDiscoveryData& D : *Discs)
	{
		D.bDiscovered = DiscoveredIDs.Contains(D.DiscoveryID);
		D.bClaimed = ClaimedIDs.Contains(D.DiscoveryID);
	}

	// Recalculate exploration data
	FPlanetExplorationData* ExplData = ExplorationProgress.Find(PlanetID);
	if (ExplData)
	{
		int32 DiscoveredCount = 0;
		int32 ClaimedCount = 0;
		for (const FDiscoveryData& D : *Discs)
		{
			if (D.bDiscovered) DiscoveredCount++;
			if (D.bClaimed) ClaimedCount++;
		}
		ExplData->TotalDiscoveries = DiscoveredCount;
		ExplData->ClaimedDiscoveries = ClaimedCount;
	}
}

// ============================================================================
// UTILITY (STATIC)
// ============================================================================

FText UExplorationRewardSystem::GetDiscoveryTypeDisplayName(EDiscoveryType Type)
{
	switch (Type)
	{
	case EDiscoveryType::ResourceDeposit:      return FText::FromString(TEXT("Resource Deposit"));
	case EDiscoveryType::AncientRuins:         return FText::FromString(TEXT("Ancient Ruins"));
	case EDiscoveryType::AlienArtifact:        return FText::FromString(TEXT("Alien Artifact"));
	case EDiscoveryType::NaturalWonder:        return FText::FromString(TEXT("Natural Wonder"));
	case EDiscoveryType::AbandonedOutpost:     return FText::FromString(TEXT("Abandoned Outpost"));
	case EDiscoveryType::BiologicalSpecimen:   return FText::FromString(TEXT("Biological Specimen"));
	case EDiscoveryType::AnomalousSignal:      return FText::FromString(TEXT("Anomalous Signal"));
	case EDiscoveryType::HiddenCache:          return FText::FromString(TEXT("Hidden Cache"));
	case EDiscoveryType::WreckedShip:          return FText::FromString(TEXT("Wrecked Ship"));
	case EDiscoveryType::PrecursorTechnology:  return FText::FromString(TEXT("Precursor Technology"));
	case EDiscoveryType::QuantumAnomaly:       return FText::FromString(TEXT("Quantum Anomaly"));
	case EDiscoveryType::RareMineral:          return FText::FromString(TEXT("Rare Mineral"));
	case EDiscoveryType::GeothermalVent:       return FText::FromString(TEXT("Geothermal Vent"));
	case EDiscoveryType::FrozenOrganism:       return FText::FromString(TEXT("Frozen Organism"));
	case EDiscoveryType::CrystalFormation:     return FText::FromString(TEXT("Crystal Formation"));
	default:                                   return FText::FromString(TEXT("Unknown"));
	}
}

FLinearColor UExplorationRewardSystem::GetDiscoveryRarityColor(EDiscoveryRarity Rarity)
{
	switch (Rarity)
	{
	case EDiscoveryRarity::Common:     return FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
	case EDiscoveryRarity::Uncommon:   return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
	case EDiscoveryRarity::Rare:       return FLinearColor(0.2f, 0.4f, 1.0f, 1.0f);
	case EDiscoveryRarity::Epic:       return FLinearColor(0.6f, 0.2f, 0.9f, 1.0f);
	case EDiscoveryRarity::Legendary:  return FLinearColor(1.0f, 0.7f, 0.0f, 1.0f);
	case EDiscoveryRarity::Mythic:     return FLinearColor(1.0f, 0.2f, 0.3f, 1.0f);
	default:                           return FLinearColor::White;
	}
}

FText UExplorationRewardSystem::GetExplorationStatusDisplayName(EExplorationStatus Status)
{
	switch (Status)
	{
	case EExplorationStatus::Uncharted:         return FText::FromString(TEXT("Uncharted"));
	case EExplorationStatus::Surveyed:          return FText::FromString(TEXT("Surveyed"));
	case EExplorationStatus::PartiallyExplored: return FText::FromString(TEXT("Partially Explored"));
	case EExplorationStatus::MostlyExplored:    return FText::FromString(TEXT("Mostly Explored"));
	case EExplorationStatus::FullyExplored:     return FText::FromString(TEXT("Fully Explored"));
	case EExplorationStatus::Mastered:          return FText::FromString(TEXT("Mastered"));
	default:                                    return FText::FromString(TEXT("Unknown"));
	}
}

int32 UExplorationRewardSystem::CalculateDiscoveryOMENValue(EDiscoveryType Type, EDiscoveryRarity Rarity)
{
	// Base value by type
	int32 BaseValue = 50;
	switch (Type)
	{
	case EDiscoveryType::ResourceDeposit:      BaseValue = 30; break;
	case EDiscoveryType::AncientRuins:         BaseValue = 100; break;
	case EDiscoveryType::AlienArtifact:        BaseValue = 150; break;
	case EDiscoveryType::NaturalWonder:        BaseValue = 80; break;
	case EDiscoveryType::AbandonedOutpost:     BaseValue = 70; break;
	case EDiscoveryType::BiologicalSpecimen:   BaseValue = 60; break;
	case EDiscoveryType::AnomalousSignal:      BaseValue = 90; break;
	case EDiscoveryType::HiddenCache:          BaseValue = 120; break;
	case EDiscoveryType::WreckedShip:          BaseValue = 110; break;
	case EDiscoveryType::PrecursorTechnology:  BaseValue = 200; break;
	case EDiscoveryType::QuantumAnomaly:       BaseValue = 250; break;
	case EDiscoveryType::RareMineral:          BaseValue = 40; break;
	case EDiscoveryType::GeothermalVent:       BaseValue = 45; break;
	case EDiscoveryType::FrozenOrganism:       BaseValue = 75; break;
	case EDiscoveryType::CrystalFormation:     BaseValue = 55; break;
	default: break;
	}

	// Rarity multiplier
	float RarityMult = 1.0f;
	switch (Rarity)
	{
	case EDiscoveryRarity::Common:     RarityMult = 1.0f; break;
	case EDiscoveryRarity::Uncommon:   RarityMult = 2.0f; break;
	case EDiscoveryRarity::Rare:       RarityMult = 4.0f; break;
	case EDiscoveryRarity::Epic:       RarityMult = 8.0f; break;
	case EDiscoveryRarity::Legendary:  RarityMult = 15.0f; break;
	case EDiscoveryRarity::Mythic:     RarityMult = 30.0f; break;
	}

	return static_cast<int32>(static_cast<float>(BaseValue) * RarityMult);
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

EDiscoveryType UExplorationRewardSystem::SelectDiscoveryType(int32 Seed, EBiomeType Biome) const
{
	const TArray<EDiscoveryType>* AffinityList = BiomeDiscoveryAffinity.Find(Biome);
	if (AffinityList && AffinityList->Num() > 0)
	{
		// 80% chance to pick from biome affinity, 20% fully random
		if (SeededRandom(Seed) < 0.8f)
		{
			int32 Index = SeededRandomRange(Seed + 50, 0, AffinityList->Num() - 1);
			return (*AffinityList)[Index];
		}
	}

	// Random selection from all types
	TArray<EDiscoveryType> AllTypes = {
		EDiscoveryType::ResourceDeposit, EDiscoveryType::AncientRuins,
		EDiscoveryType::AlienArtifact, EDiscoveryType::NaturalWonder,
		EDiscoveryType::AbandonedOutpost, EDiscoveryType::BiologicalSpecimen,
		EDiscoveryType::AnomalousSignal, EDiscoveryType::HiddenCache,
		EDiscoveryType::WreckedShip, EDiscoveryType::PrecursorTechnology,
		EDiscoveryType::QuantumAnomaly, EDiscoveryType::RareMineral,
		EDiscoveryType::GeothermalVent, EDiscoveryType::FrozenOrganism,
		EDiscoveryType::CrystalFormation
	};

	int32 Index = SeededRandomRange(Seed + 100, 0, AllTypes.Num() - 1);
	return AllTypes[Index];
}

EDiscoveryRarity UExplorationRewardSystem::DetermineDiscoveryRarity(int32 Seed, EBiomeType Biome) const
{
	// Base rarity weights: Common=40%, Uncommon=30%, Rare=18%, Epic=8%, Legendary=3%, Mythic=1%
	TArray<float> Weights = { 0.40f, 0.30f, 0.18f, 0.08f, 0.03f, 0.01f };

	// Hazardous and rare biomes boost rarer discoveries
	float RarityBoost = 0.0f;
	if (BiomeDefinitionSystem)
	{
		FBiomeDefinition BiomeDef = BiomeDefinitionSystem->GetBiomeDefinition(Biome);
		RarityBoost = BiomeDef.HazardIntensity * 0.1f;

		// Anomalous biome gives extra rarity boost
		if (Biome == EBiomeType::Anomalous)
		{
			RarityBoost += 0.15f;
		}
	}

	// Shift weights towards rarer tiers
	if (RarityBoost > 0.0f)
	{
		Weights[0] -= RarityBoost;
		Weights[0] = FMath::Max(Weights[0], 0.1f);
		for (int32 i = 2; i < Weights.Num(); ++i)
		{
			Weights[i] += RarityBoost / static_cast<float>(Weights.Num() - 2);
		}
	}

	float TotalWeight = 0.0f;
	for (float W : Weights) TotalWeight += W;

	float Roll = SeededRandom(Seed) * TotalWeight;
	float Accumulated = 0.0f;
	for (int32 i = 0; i < Weights.Num(); ++i)
	{
		Accumulated += Weights[i];
		if (Roll <= Accumulated)
		{
			return static_cast<EDiscoveryRarity>(i);
		}
	}

	return EDiscoveryRarity::Common;
}

FText UExplorationRewardSystem::GenerateDiscoveryName(int32 Seed, EDiscoveryType Type, EBiomeType Biome) const
{
	FString Name;

	switch (Type)
	{
	case EDiscoveryType::AncientRuins:
	{
		int32 PrefixIdx = SeededRandomRange(Seed, 0, RuinsPrefixes.Num() - 1);
		Name = FString::Printf(TEXT("%s Ruins"), *RuinsPrefixes[PrefixIdx]);
		break;
	}
	case EDiscoveryType::AlienArtifact:
	{
		int32 ArtIdx = SeededRandomRange(Seed, 0, ArtifactNames.Num() - 1);
		Name = ArtifactNames[ArtIdx];
		break;
	}
	case EDiscoveryType::NaturalWonder:
	{
		int32 AdjIdx = SeededRandomRange(Seed, 0, WonderAdjectives.Num() - 1);
		TArray<FString> WonderNouns = {
			TEXT("Arch"), TEXT("Cavern"), TEXT("Spire"), TEXT("Chasm"),
			TEXT("Geyser"), TEXT("Falls"), TEXT("Canyon"), TEXT("Pinnacle")
		};
		int32 NounIdx = SeededRandomRange(Seed + 10, 0, WonderNouns.Num() - 1);
		Name = FString::Printf(TEXT("The %s %s"), *WonderAdjectives[AdjIdx], *WonderNouns[NounIdx]);
		break;
	}
	case EDiscoveryType::AbandonedOutpost:
	{
		int32 DesigIdx = SeededRandomRange(Seed, 0, OutpostDesignations.Num() - 1);
		int32 NumID = SeededRandomRange(Seed + 5, 1, 99);
		Name = FString::Printf(TEXT("Outpost %s-%d"), *OutpostDesignations[DesigIdx], NumID);
		break;
	}
	case EDiscoveryType::WreckedShip:
	{
		TArray<FString> ShipPrefixes = { TEXT("ISS"), TEXT("HMS"), TEXT("OSV"), TEXT("DSV"), TEXT("TSV") };
		TArray<FString> ShipNames = {
			TEXT("Wanderer"), TEXT("Horizon"), TEXT("Intrepid"), TEXT("Pioneer"),
			TEXT("Voyager"), TEXT("Seeker"), TEXT("Pathfinder"), TEXT("Endeavor")
		};
		int32 PIdx = SeededRandomRange(Seed, 0, ShipPrefixes.Num() - 1);
		int32 NIdx = SeededRandomRange(Seed + 10, 0, ShipNames.Num() - 1);
		Name = FString::Printf(TEXT("Wreck of the %s %s"), *ShipPrefixes[PIdx], *ShipNames[NIdx]);
		break;
	}
	case EDiscoveryType::QuantumAnomaly:
	{
		int32 NumID = SeededRandomRange(Seed, 100, 999);
		Name = FString::Printf(TEXT("Quantum Anomaly QA-%d"), NumID);
		break;
	}
	case EDiscoveryType::PrecursorTechnology:
	{
		TArray<FString> TechNames = {
			TEXT("Dimensional Gateway"), TEXT("Stasis Chamber"), TEXT("Terraformer Core"),
			TEXT("Neural Beacon"), TEXT("Gravity Forge"), TEXT("Void Engine")
		};
		int32 TIdx = SeededRandomRange(Seed, 0, TechNames.Num() - 1);
		Name = TechNames[TIdx];
		break;
	}
	default:
	{
		Name = FString::Printf(TEXT("%s #%d"),
			*GetDiscoveryTypeDisplayName(Type).ToString(),
			SeededRandomRange(Seed, 1, 999));
		break;
	}
	}

	return FText::FromString(Name);
}

FText UExplorationRewardSystem::GenerateDiscoveryDescription(
	EDiscoveryType Type, EDiscoveryRarity Rarity, EBiomeType Biome) const
{
	FString RarityStr;
	switch (Rarity)
	{
	case EDiscoveryRarity::Common:     RarityStr = TEXT("a common"); break;
	case EDiscoveryRarity::Uncommon:   RarityStr = TEXT("an uncommon"); break;
	case EDiscoveryRarity::Rare:       RarityStr = TEXT("a rare"); break;
	case EDiscoveryRarity::Epic:       RarityStr = TEXT("an extraordinary"); break;
	case EDiscoveryRarity::Legendary:  RarityStr = TEXT("a legendary"); break;
	case EDiscoveryRarity::Mythic:     RarityStr = TEXT("a mythic"); break;
	default:                           RarityStr = TEXT("an unknown"); break;
	}

	FString BiomeName = TEXT("unknown terrain");
	if (BiomeDefinitionSystem)
	{
		BiomeName = BiomeDefinitionSystem->GetBiomeDisplayName(Biome).ToString().ToLower();
	}

	FString TypeDesc;
	switch (Type)
	{
	case EDiscoveryType::ResourceDeposit:
		TypeDesc = FString::Printf(TEXT("Scanners detect %s resource deposit embedded in the %s."), *RarityStr, *BiomeName);
		break;
	case EDiscoveryType::AncientRuins:
		TypeDesc = FString::Printf(TEXT("The remains of %s ancient structure emerge from the %s, origin unknown."), *RarityStr, *BiomeName);
		break;
	case EDiscoveryType::AlienArtifact:
		TypeDesc = FString::Printf(TEXT("A %s artifact of alien design, found within the %s biome. Its purpose remains unclear."), *RarityStr, *BiomeName);
		break;
	case EDiscoveryType::NaturalWonder:
		TypeDesc = FString::Printf(TEXT("A %s natural formation of breathtaking scale rises from the %s."), *RarityStr, *BiomeName);
		break;
	case EDiscoveryType::HiddenCache:
		TypeDesc = FString::Printf(TEXT("A %s supply cache concealed within the %s. Someone left this here deliberately."), *RarityStr, *BiomeName);
		break;
	default:
		TypeDesc = FString::Printf(TEXT("A %s discovery found in the %s biome."), *RarityStr, *BiomeName);
		break;
	}

	return FText::FromString(TypeDesc);
}

FText UExplorationRewardSystem::GenerateLoreText(int32 Seed, EDiscoveryType Type) const
{
	TArray<FString> LoreEntries;

	switch (Type)
	{
	case EDiscoveryType::AncientRuins:
		LoreEntries = {
			TEXT("Carbon dating suggests this structure predates known spacefaring civilizations by millennia."),
			TEXT("The architectural style matches no known species. The walls hum with residual energy."),
			TEXT("Inscriptions cover the interior walls. The language has no known translation.")
		};
		break;
	case EDiscoveryType::PrecursorTechnology:
		LoreEntries = {
			TEXT("This technology operates on principles that challenge our understanding of physics."),
			TEXT("The device appears to manipulate spacetime at a quantum level. Handle with extreme caution."),
			TEXT("Power readings are off the charts. This could revolutionize interstellar travel.")
		};
		break;
	case EDiscoveryType::QuantumAnomaly:
		LoreEntries = {
			TEXT("Local spacetime is folded in ways that should not be possible. Sensors are unreliable here."),
			TEXT("Matter and energy behave unpredictably. Brief temporal echoes have been observed."),
			TEXT("The anomaly pulses with a rhythm that some crew members find hypnotic.")
		};
		break;
	default:
		LoreEntries = {
			TEXT("This discovery warrants further study by qualified researchers."),
			TEXT("Initial scans reveal intriguing properties that defy easy classification."),
			TEXT("The significance of this find may not be fully appreciated for years to come.")
		};
		break;
	}

	if (LoreEntries.Num() > 0)
	{
		int32 Index = SeededRandomRange(Seed, 0, LoreEntries.Num() - 1);
		return FText::FromString(LoreEntries[Index]);
	}

	return FText::GetEmpty();
}

TArray<FResourceStack> UExplorationRewardSystem::GenerateResourceRewards(
	int32 Seed,
	EDiscoveryType Type,
	EDiscoveryRarity Rarity,
	EBiomeType Biome) const
{
	TArray<FResourceStack> Rewards;

	int32 RarityBonus = static_cast<int32>(static_cast<uint8>(Rarity));

	switch (Type)
	{
	case EDiscoveryType::ResourceDeposit:
	case EDiscoveryType::RareMineral:
	{
		// Resource-heavy reward
		EResourceType Resource = EResourceType::Silicate;
		if (BiomeDefinitionSystem)
		{
			Resource = BiomeDefinitionSystem->SelectResourceFromBiome(Biome, Seed);
		}
		int32 Amount = 20 + RarityBonus * 15;
		Rewards.Add(FResourceStack(Resource, Amount));
		break;
	}
	case EDiscoveryType::HiddenCache:
	{
		// Mixed resources
		Rewards.Add(FResourceStack(EResourceType::Silicate, 10 + RarityBonus * 5));
		Rewards.Add(FResourceStack(EResourceType::Carbon, 10 + RarityBonus * 5));
		if (RarityBonus >= 2)
		{
			Rewards.Add(FResourceStack(EResourceType::RefinedSilicate, 5 + RarityBonus * 3));
		}
		break;
	}
	case EDiscoveryType::WreckedShip:
	{
		// Refined materials
		Rewards.Add(FResourceStack(EResourceType::RefinedSilicate, 5 + RarityBonus * 4));
		Rewards.Add(FResourceStack(EResourceType::RefinedCarbon, 5 + RarityBonus * 3));
		if (RarityBonus >= 3)
		{
			Rewards.Add(FResourceStack(EResourceType::CompositeMaterial, 2 + RarityBonus));
		}
		break;
	}
	case EDiscoveryType::PrecursorTechnology:
	case EDiscoveryType::AlienArtifact:
	{
		// High-value materials
		Rewards.Add(FResourceStack(EResourceType::CompositeMaterial, 3 + RarityBonus * 2));
		if (RarityBonus >= 2)
		{
			Rewards.Add(FResourceStack(EResourceType::RefinedSilicate, 10 + RarityBonus * 5));
		}
		break;
	}
	default:
	{
		// Generic small reward
		if (RarityBonus >= 1)
		{
			Rewards.Add(FResourceStack(EResourceType::Carbon, 5 + RarityBonus * 3));
		}
		break;
	}
	}

	return Rewards;
}

EScanMode UExplorationRewardSystem::DetermineScanRequirement(EDiscoveryRarity Rarity) const
{
	switch (Rarity)
	{
	case EDiscoveryRarity::Common:
	case EDiscoveryRarity::Uncommon:
		return EScanMode::Passive;
	case EDiscoveryRarity::Rare:
		return EScanMode::ActiveShort;
	case EDiscoveryRarity::Epic:
		return EScanMode::ActiveLong;
	case EDiscoveryRarity::Legendary:
		return EScanMode::Deep;
	case EDiscoveryRarity::Mythic:
		return EScanMode::Anomaly;
	default:
		return EScanMode::Passive;
	}
}

// ============================================================================
// SEEDED RANDOM HELPERS
// ============================================================================

uint32 UExplorationRewardSystem::HashSeed(int32 Seed)
{
	uint32 Hash = static_cast<uint32>(Seed);
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = (Hash >> 16) ^ Hash;
	return Hash;
}

float UExplorationRewardSystem::SeededRandom(int32 Seed)
{
	uint32 Hash = HashSeed(Seed);
	return static_cast<float>(Hash & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
}

int32 UExplorationRewardSystem::SeededRandomRange(int32 Seed, int32 Min, int32 Max)
{
	if (Min >= Max) return Min;
	float Random = SeededRandom(Seed);
	return Min + static_cast<int32>(Random * static_cast<float>(Max - Min + 1));
}

float UExplorationRewardSystem::SeededRandomFloat(int32 Seed, float Min, float Max)
{
	return Min + SeededRandom(Seed) * (Max - Min);
}

