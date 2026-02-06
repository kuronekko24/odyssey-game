// OdysseyResourceDistributionSystem.cpp
// Implementation of strategic resource distribution system

#include "OdysseyResourceDistributionSystem.h"
#include "ResourceNode.h"
#include "Engine/World.h"

UOdysseyResourceDistributionSystem::UOdysseyResourceDistributionSystem()
	: BiomeDefinitionSystem(nullptr)
	, NextDepositID(1)
	, NextClusterID(1)
{
	InitializeDefaultParameters();
}

void UOdysseyResourceDistributionSystem::Initialize(UOdysseyBiomeDefinitionSystem* BiomeSystem)
{
	BiomeDefinitionSystem = BiomeSystem;
}

void UOdysseyResourceDistributionSystem::InitializeDefaultParameters()
{
	// Silicate - Common, found everywhere
	FResourceDistributionParams SilicateParams;
	SilicateParams.ResourceType = EResourceType::Silicate;
	SilicateParams.BaseDensity = 0.8f;
	SilicateParams.MinClusterSize = 2;
	SilicateParams.MaxClusterSize = 8;
	SilicateParams.MinClusterSpacing = 80.0f;
	SilicateParams.BaseQualityRange = FVector2D(0.3f, 0.9f);
	SilicateParams.BaseAmountRange = FIntPoint(30, 150);
	SilicateParams.RarityWeights = { 0.6f, 0.25f, 0.1f, 0.04f, 0.009f, 0.001f };
	DistributionParameters.Add(EResourceType::Silicate, SilicateParams);

	// Carbon - Common, organic-rich areas
	FResourceDistributionParams CarbonParams;
	CarbonParams.ResourceType = EResourceType::Carbon;
	CarbonParams.BaseDensity = 0.6f;
	CarbonParams.MinClusterSize = 2;
	CarbonParams.MaxClusterSize = 6;
	CarbonParams.MinClusterSpacing = 100.0f;
	CarbonParams.BaseQualityRange = FVector2D(0.35f, 0.85f);
	CarbonParams.BaseAmountRange = FIntPoint(25, 120);
	CarbonParams.RarityWeights = { 0.55f, 0.28f, 0.12f, 0.04f, 0.008f, 0.002f };
	DistributionParameters.Add(EResourceType::Carbon, CarbonParams);

	// Refined Silicate - Uncommon
	FResourceDistributionParams RefinedSilicateParams;
	RefinedSilicateParams.ResourceType = EResourceType::RefinedSilicate;
	RefinedSilicateParams.BaseDensity = 0.25f;
	RefinedSilicateParams.MinClusterSize = 1;
	RefinedSilicateParams.MaxClusterSize = 3;
	RefinedSilicateParams.MinClusterSpacing = 200.0f;
	RefinedSilicateParams.BaseQualityRange = FVector2D(0.5f, 0.95f);
	RefinedSilicateParams.BaseAmountRange = FIntPoint(10, 50);
	RefinedSilicateParams.RarityWeights = { 0.2f, 0.4f, 0.25f, 0.1f, 0.04f, 0.01f };
	DistributionParameters.Add(EResourceType::RefinedSilicate, RefinedSilicateParams);

	// Refined Carbon - Uncommon
	FResourceDistributionParams RefinedCarbonParams;
	RefinedCarbonParams.ResourceType = EResourceType::RefinedCarbon;
	RefinedCarbonParams.BaseDensity = 0.2f;
	RefinedCarbonParams.MinClusterSize = 1;
	RefinedCarbonParams.MaxClusterSize = 3;
	RefinedCarbonParams.MinClusterSpacing = 220.0f;
	RefinedCarbonParams.BaseQualityRange = FVector2D(0.5f, 0.9f);
	RefinedCarbonParams.BaseAmountRange = FIntPoint(8, 40);
	RefinedCarbonParams.RarityWeights = { 0.15f, 0.4f, 0.3f, 0.1f, 0.04f, 0.01f };
	DistributionParameters.Add(EResourceType::RefinedCarbon, RefinedCarbonParams);

	// Composite Material - Rare
	FResourceDistributionParams CompositeParams;
	CompositeParams.ResourceType = EResourceType::CompositeMaterial;
	CompositeParams.BaseDensity = 0.08f;
	CompositeParams.MinClusterSize = 1;
	CompositeParams.MaxClusterSize = 2;
	CompositeParams.MinClusterSpacing = 400.0f;
	CompositeParams.BaseQualityRange = FVector2D(0.6f, 1.0f);
	CompositeParams.BaseAmountRange = FIntPoint(5, 25);
	CompositeParams.RarityWeights = { 0.05f, 0.15f, 0.35f, 0.3f, 0.12f, 0.03f };
	DistributionParameters.Add(EResourceType::CompositeMaterial, CompositeParams);
}

TArray<FResourceDepositLocation> UOdysseyResourceDistributionSystem::GenerateResourceDeposits(
	int32 Seed,
	const FVector2D& AreaSize,
	const TArray<EBiomeType>& Biomes,
	int32 TargetDepositCount)
{
	TArray<FResourceDepositLocation> Result;
	
	if (Biomes.Num() == 0 || TargetDepositCount <= 0)
	{
		return Result;
	}

	// Calculate deposits per biome
	int32 DepositsPerBiome = TargetDepositCount / Biomes.Num();
	int32 ExtraDeposits = TargetDepositCount % Biomes.Num();

	// Generate deposits for each biome
	for (int32 BiomeIndex = 0; BiomeIndex < Biomes.Num(); ++BiomeIndex)
	{
		EBiomeType CurrentBiome = Biomes[BiomeIndex];
		int32 BiomeDepositCount = DepositsPerBiome + (BiomeIndex < ExtraDeposits ? 1 : 0);

		// Calculate biome area (divide area among biomes for simplicity)
		float BiomeStartX = (AreaSize.X / Biomes.Num()) * BiomeIndex;
		float BiomeEndX = (AreaSize.X / Biomes.Num()) * (BiomeIndex + 1);

		// Generate cluster centers for this biome
		float MinSpacing = 100.0f;
		int32 ClusterCount = FMath::Max(1, BiomeDepositCount / 3);
		
		TArray<FVector2D> ClusterCenters = GenerateClusterCenters(
			Seed + BiomeIndex * 1000,
			FVector2D(BiomeEndX - BiomeStartX, AreaSize.Y),
			MinSpacing,
			ClusterCount * 10
		);

		// Offset cluster centers to biome position
		for (FVector2D& Center : ClusterCenters)
		{
			Center.X += BiomeStartX;
		}

		// Distribute deposits among clusters
		int32 DepositsPlaced = 0;
		int32 ClusterIndex = 0;

		while (DepositsPlaced < BiomeDepositCount && ClusterCenters.Num() > 0)
		{
			FVector2D ClusterCenter = ClusterCenters[ClusterIndex % ClusterCenters.Num()];
			
			// Determine primary resource for this cluster
			EResourceType PrimaryResource = SelectResourceForBiome(Seed + DepositsPlaced, CurrentBiome);
			
			// Generate deposits in this cluster
			int32 ClusterSize = SeededRandomRange(Seed + DepositsPlaced + 100, 1, 4);
			ClusterSize = FMath::Min(ClusterSize, BiomeDepositCount - DepositsPlaced);

			TArray<FResourceDepositLocation> ClusterDeposits = GenerateClusterDeposits(
				Seed + DepositsPlaced + BiomeIndex * 500,
				FVector(ClusterCenter.X, ClusterCenter.Y, 0.0f),
				50.0f + SeededRandom(Seed + DepositsPlaced) * 50.0f,
				PrimaryResource,
				CurrentBiome,
				ClusterSize
			);

			Result.Append(ClusterDeposits);
			DepositsPlaced += ClusterDeposits.Num();
			ClusterIndex++;
		}
	}

	return Result;
}

TArray<FResourceCluster> UOdysseyResourceDistributionSystem::GenerateResourceClusters(
	int32 Seed,
	const FVector2D& AreaSize,
	EBiomeType Biome,
	int32 ClusterCount)
{
	TArray<FResourceCluster> Result;

	// Generate cluster centers using Poisson disk sampling
	float MinSpacing = 150.0f;
	TArray<FVector2D> Centers = GenerateClusterCenters(Seed, AreaSize, MinSpacing, ClusterCount * 5);

	// Limit to requested cluster count
	int32 ActualClusterCount = FMath::Min(Centers.Num(), ClusterCount);

	for (int32 i = 0; i < ActualClusterCount; ++i)
	{
		FResourceCluster Cluster;
		Cluster.ClusterID = NextClusterID++;
		Cluster.CenterLocation = FVector(Centers[i].X, Centers[i].Y, 0.0f);
		Cluster.Radius = 40.0f + SeededRandom(Seed + i * 50) * 80.0f;
		Cluster.PrimaryResource = SelectResourceForBiome(Seed + i, Biome);
		Cluster.Richness = SeededRandom(Seed + i * 100);

		// 30% chance for secondary resource
		if (SeededRandom(Seed + i * 200) < 0.3f)
		{
			Cluster.SecondaryResource = SelectResourceForBiome(Seed + i + 1000, Biome);
			if (Cluster.SecondaryResource == Cluster.PrimaryResource)
			{
				Cluster.SecondaryResource = EResourceType::None;
			}
		}
		else
		{
			Cluster.SecondaryResource = EResourceType::None;
		}

		Result.Add(Cluster);
	}

	return Result;
}

FResourceDepositLocation UOdysseyResourceDistributionSystem::GenerateSingleDeposit(
	int32 Seed,
	const FVector& Location,
	EBiomeType Biome,
	EResourceType PreferredResource)
{
	FResourceDepositLocation Deposit;
	Deposit.DepositID = NextDepositID++;
	Deposit.Location = Location;
	Deposit.BiomeType = Biome;

	// Determine resource type
	if (PreferredResource != EResourceType::None)
	{
		Deposit.ResourceType = PreferredResource;
	}
	else
	{
		Deposit.ResourceType = SelectResourceForBiome(Seed, Biome);
	}

	// Determine rarity
	Deposit.Rarity = DetermineRarity(Seed + 100, Biome, Deposit.ResourceType);

	// Calculate quality based on rarity and biome
	Deposit.Quality = CalculateQuality(Seed + 200, Deposit.Rarity, Biome);

	// Calculate amount based on rarity and resource type
	Deposit.TotalAmount = CalculateDepositAmount(Seed + 300, Deposit.Rarity, Deposit.ResourceType);
	Deposit.RemainingAmount = Deposit.TotalAmount;

	// Determine deposit type based on rarity
	float DepositTypeRandom = SeededRandom(Seed + 400);
	if (Deposit.Rarity >= EResourceRarity::Exotic)
	{
		Deposit.DepositType = EResourceDepositType::Anomalous;
	}
	else if (Deposit.Rarity >= EResourceRarity::Rare)
	{
		Deposit.DepositType = DepositTypeRandom < 0.5f ? EResourceDepositType::Deep : EResourceDepositType::Vein;
	}
	else if (Deposit.Rarity >= EResourceRarity::Uncommon)
	{
		Deposit.DepositType = DepositTypeRandom < 0.6f ? EResourceDepositType::Shallow : EResourceDepositType::Cluster;
	}
	else
	{
		Deposit.DepositType = EResourceDepositType::Surface;
	}

	// Calculate mining difficulty
	float RarityDifficultyBonus = static_cast<float>(static_cast<uint8>(Deposit.Rarity)) * 0.2f;
	float BiomeModifier = 1.0f;
	
	if (BiomeDefinitionSystem)
	{
		FBiomeGameplayModifiers Modifiers = BiomeDefinitionSystem->GetBiomeGameplayModifiers(Biome);
		BiomeModifier = 1.0f / FMath::Max(0.1f, Modifiers.MiningSpeedModifier);
	}

	Deposit.MiningDifficulty = (1.0f + RarityDifficultyBonus) * BiomeModifier;
	Deposit.bDiscovered = false;

	return Deposit;
}

AResourceNode* UOdysseyResourceDistributionSystem::SpawnResourceNode(
	UWorld* World,
	const FResourceDepositLocation& DepositData,
	TSubclassOf<AResourceNode> NodeClass)
{
	if (!World || !NodeClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AResourceNode* Node = World->SpawnActor<AResourceNode>(NodeClass, DepositData.Location, FRotator::ZeroRotator, SpawnParams);
	
	if (Node)
	{
		// Configure node with deposit data
		FResourceNodeData NodeData;
		NodeData.ResourceType = DepositData.ResourceType;
		NodeData.MaxResourceAmount = DepositData.TotalAmount;
		NodeData.CurrentResourceAmount = DepositData.RemainingAmount;
		NodeData.MiningDifficulty = DepositData.MiningDifficulty;
		NodeData.RegenerationRate = 0.05f * DepositData.Quality; // Better quality = faster regen
		NodeData.bCanRegenerate = DepositData.Rarity < EResourceRarity::Exotic; // Exotic doesn't regenerate

		Node->SetResourceData(NodeData);
	}

	return Node;
}

TArray<AResourceNode*> UOdysseyResourceDistributionSystem::SpawnResourceNodes(
	UWorld* World,
	const TArray<FResourceDepositLocation>& Deposits,
	TSubclassOf<AResourceNode> NodeClass)
{
	TArray<AResourceNode*> SpawnedNodes;
	
	for (const FResourceDepositLocation& Deposit : Deposits)
	{
		AResourceNode* Node = SpawnResourceNode(World, Deposit, NodeClass);
		if (Node)
		{
			SpawnedNodes.Add(Node);
		}
	}

	return SpawnedNodes;
}

EResourceRarity UOdysseyResourceDistributionSystem::DetermineRarity(int32 Seed, EBiomeType Biome, EResourceType ResourceType) const
{
	// Get distribution params for this resource
	const FResourceDistributionParams* Params = DistributionParameters.Find(ResourceType);
	if (!Params || Params->RarityWeights.Num() < 6)
	{
		return EResourceRarity::Common;
	}

	// Apply biome modifier to rarity chances
	float BiomeRarityBonus = 0.0f;
	if (BiomeDefinitionSystem)
	{
		// Hazardous biomes have better rare resources
		FBiomeDefinition BiomeDef = BiomeDefinitionSystem->GetBiomeDefinition(Biome);
		BiomeRarityBonus = BiomeDef.HazardIntensity * 0.15f;
	}

	// Calculate total weight
	float TotalWeight = 0.0f;
	TArray<float> AdjustedWeights;
	
	for (int32 i = 0; i < 6; ++i)
	{
		// Higher rarities get boosted in dangerous biomes
		float Adjustment = (i >= 2) ? BiomeRarityBonus * (i - 1) : -BiomeRarityBonus;
		float AdjustedWeight = FMath::Max(0.0f, Params->RarityWeights[i] + Adjustment);
		AdjustedWeights.Add(AdjustedWeight);
		TotalWeight += AdjustedWeight;
	}

	if (TotalWeight <= 0.0f)
	{
		return EResourceRarity::Common;
	}

	// Select rarity based on weighted random
	float RandomValue = SeededRandom(Seed) * TotalWeight;
	float AccumulatedWeight = 0.0f;

	for (int32 i = 0; i < 6; ++i)
	{
		AccumulatedWeight += AdjustedWeights[i];
		if (RandomValue <= AccumulatedWeight)
		{
			return static_cast<EResourceRarity>(i);
		}
	}

	return EResourceRarity::Common;
}

float UOdysseyResourceDistributionSystem::CalculateQuality(int32 Seed, EResourceRarity Rarity, EBiomeType Biome) const
{
	// Base quality influenced by rarity
	float MinQuality = 0.2f + static_cast<float>(static_cast<uint8>(Rarity)) * 0.1f;
	float MaxQuality = 0.5f + static_cast<float>(static_cast<uint8>(Rarity)) * 0.1f;
	MaxQuality = FMath::Min(MaxQuality, 1.0f);

	float BaseQuality = MinQuality + SeededRandom(Seed) * (MaxQuality - MinQuality);

	// Apply biome quality modifier
	if (BiomeDefinitionSystem)
	{
		// Use biome's exploration score as a proxy for quality bonus
		int32 ExplorationScore = BiomeDefinitionSystem->GetBiomeExplorationScore(Biome);
		float BiomeQualityBonus = (ExplorationScore - 100) * 0.001f; // +/- 10% for extreme biomes
		BaseQuality = FMath::Clamp(BaseQuality + BiomeQualityBonus, 0.1f, 1.0f);
	}

	return BaseQuality;
}

int32 UOdysseyResourceDistributionSystem::CalculateDepositAmount(int32 Seed, EResourceRarity Rarity, EResourceType ResourceType) const
{
	const FResourceDistributionParams* Params = DistributionParameters.Find(ResourceType);
	if (!Params)
	{
		return 50;
	}

	// Base amount from params
	int32 MinAmount = Params->BaseAmountRange.X;
	int32 MaxAmount = Params->BaseAmountRange.Y;

	// Rarity multiplier
	float RarityMultiplier = 1.0f;
	switch (Rarity)
	{
	case EResourceRarity::Uncommon:
		RarityMultiplier = 0.8f;
		break;
	case EResourceRarity::Rare:
		RarityMultiplier = 0.6f;
		break;
	case EResourceRarity::VeryRare:
		RarityMultiplier = 0.4f;
		break;
	case EResourceRarity::Exotic:
		RarityMultiplier = 0.25f;
		break;
	case EResourceRarity::Legendary:
		RarityMultiplier = 0.15f;
		break;
	default:
		RarityMultiplier = 1.0f;
		break;
	}

	// Adjust range based on rarity (rarer = smaller deposits but more valuable)
	MinAmount = FMath::Max(1, static_cast<int32>(MinAmount * RarityMultiplier));
	MaxAmount = FMath::Max(MinAmount, static_cast<int32>(MaxAmount * RarityMultiplier));

	return SeededRandomRange(Seed, MinAmount, MaxAmount);
}

TArray<FTradeRouteOpportunity> UOdysseyResourceDistributionSystem::AnalyzeTradeOpportunities(
	const TMap<int32, TArray<FResourceDepositLocation>>& LocationResources) const
{
	TArray<FTradeRouteOpportunity> Opportunities;

	// Calculate abundance at each location
	TMap<int32, TMap<EResourceType, float>> LocationAbundance;
	
	for (const auto& Pair : LocationResources)
	{
		int32 LocationID = Pair.Key;
		TMap<EResourceType, float> Abundance = CalculateResourceAbundance(Pair.Value);
		LocationAbundance.Add(LocationID, Abundance);
	}

	// Find trade opportunities between locations
	TArray<int32> LocationIDs;
	LocationResources.GetKeys(LocationIDs);

	for (int32 i = 0; i < LocationIDs.Num(); ++i)
	{
		for (int32 j = i + 1; j < LocationIDs.Num(); ++j)
		{
			int32 LocationA = LocationIDs[i];
			int32 LocationB = LocationIDs[j];

			const TMap<EResourceType, float>& AbundanceA = LocationAbundance[LocationA];
			const TMap<EResourceType, float>& AbundanceB = LocationAbundance[LocationB];

			// Check each resource type for trade potential
			for (const auto& ResourcePair : AbundanceA)
			{
				EResourceType Resource = ResourcePair.Key;
				float AbundanceAtA = ResourcePair.Value;
				float AbundanceAtB = AbundanceB.Contains(Resource) ? AbundanceB[Resource] : 0.0f;

				// Calculate abundance difference
				float Difference = AbundanceAtA - AbundanceAtB;

				if (FMath::Abs(Difference) > 0.2f)
				{
					FTradeRouteOpportunity Opportunity;
					
					if (Difference > 0)
					{
						// Resource abundant at A, scarce at B
						Opportunity.AbundantResource = Resource;
						Opportunity.ScarceResource = Resource;
						Opportunity.SourceLocationID = LocationA;
						Opportunity.DestinationLocationID = LocationB;
					}
					else
					{
						// Resource abundant at B, scarce at A
						Opportunity.AbundantResource = Resource;
						Opportunity.ScarceResource = Resource;
						Opportunity.SourceLocationID = LocationB;
						Opportunity.DestinationLocationID = LocationA;
					}

					// Calculate profit margin based on scarcity difference
					Opportunity.ProfitMargin = FMath::Abs(Difference) * 100.0f;
					Opportunity.VolumePotential = static_cast<int32>(FMath::Abs(Difference) * 1000.0f);
					Opportunity.RiskLevel = FMath::Clamp(1.0f - FMath::Abs(Difference), 0.0f, 1.0f);

					Opportunities.Add(Opportunity);
				}
			}
		}
	}

	// Sort by profit margin (descending)
	Opportunities.Sort([](const FTradeRouteOpportunity& A, const FTradeRouteOpportunity& B)
	{
		return A.ProfitMargin > B.ProfitMargin;
	});

	return Opportunities;
}

TMap<EResourceType, float> UOdysseyResourceDistributionSystem::CalculateResourceAbundance(
	const TArray<FResourceDepositLocation>& Deposits) const
{
	TMap<EResourceType, float> Abundance;
	TMap<EResourceType, int32> TotalAmounts;

	// Sum up total amounts per resource type
	for (const FResourceDepositLocation& Deposit : Deposits)
	{
		int32* CurrentAmount = TotalAmounts.Find(Deposit.ResourceType);
		if (CurrentAmount)
		{
			*CurrentAmount += Deposit.TotalAmount;
		}
		else
		{
			TotalAmounts.Add(Deposit.ResourceType, Deposit.TotalAmount);
		}
	}

	// Calculate total across all resources
	int32 GrandTotal = 0;
	for (const auto& Pair : TotalAmounts)
	{
		GrandTotal += Pair.Value;
	}

	// Normalize to 0-1 range
	if (GrandTotal > 0)
	{
		for (const auto& Pair : TotalAmounts)
		{
			Abundance.Add(Pair.Key, static_cast<float>(Pair.Value) / static_cast<float>(GrandTotal));
		}
	}

	return Abundance;
}

float UOdysseyResourceDistributionSystem::GetResourceScarcityScore(
	EResourceType ResourceType,
	const TArray<FResourceDepositLocation>& Deposits) const
{
	TMap<EResourceType, float> Abundance = CalculateResourceAbundance(Deposits);

	if (const float* AbundanceValue = Abundance.Find(ResourceType))
	{
		// Scarcity is inverse of abundance
		return 1.0f - *AbundanceValue;
	}

	// Resource not found = maximum scarcity
	return 1.0f;
}

void UOdysseyResourceDistributionSystem::SetDistributionParams(EResourceType ResourceType, const FResourceDistributionParams& Params)
{
	DistributionParameters.Add(ResourceType, Params);
}

FResourceDistributionParams UOdysseyResourceDistributionSystem::GetDistributionParams(EResourceType ResourceType) const
{
	if (const FResourceDistributionParams* Params = DistributionParameters.Find(ResourceType))
	{
		return *Params;
	}
	return FResourceDistributionParams();
}

TArray<FResourceDepositLocation> UOdysseyResourceDistributionSystem::FindDepositsInRadius(
	const FVector& Center,
	float Radius,
	const TArray<FResourceDepositLocation>& AllDeposits) const
{
	TArray<FResourceDepositLocation> Result;
	float RadiusSquared = Radius * Radius;

	for (const FResourceDepositLocation& Deposit : AllDeposits)
	{
		if (FVector::DistSquared(Center, Deposit.Location) <= RadiusSquared)
		{
			Result.Add(Deposit);
		}
	}

	return Result;
}

TArray<FResourceDepositLocation> UOdysseyResourceDistributionSystem::FindDepositsByType(
	EResourceType ResourceType,
	const TArray<FResourceDepositLocation>& AllDeposits) const
{
	TArray<FResourceDepositLocation> Result;

	for (const FResourceDepositLocation& Deposit : AllDeposits)
	{
		if (Deposit.ResourceType == ResourceType)
		{
			Result.Add(Deposit);
		}
	}

	return Result;
}

TArray<FResourceDepositLocation> UOdysseyResourceDistributionSystem::FindDepositsByRarity(
	EResourceRarity MinRarity,
	const TArray<FResourceDepositLocation>& AllDeposits) const
{
	TArray<FResourceDepositLocation> Result;

	for (const FResourceDepositLocation& Deposit : AllDeposits)
	{
		if (Deposit.Rarity >= MinRarity)
		{
			Result.Add(Deposit);
		}
	}

	return Result;
}

FResourceDepositLocation UOdysseyResourceDistributionSystem::FindNearestDeposit(
	const FVector& Location,
	const TArray<FResourceDepositLocation>& AllDeposits,
	EResourceType FilterType) const
{
	FResourceDepositLocation Nearest;
	float NearestDistSquared = FLT_MAX;

	for (const FResourceDepositLocation& Deposit : AllDeposits)
	{
		if (FilterType != EResourceType::None && Deposit.ResourceType != FilterType)
		{
			continue;
		}

		float DistSquared = FVector::DistSquared(Location, Deposit.Location);
		if (DistSquared < NearestDistSquared)
		{
			NearestDistSquared = DistSquared;
			Nearest = Deposit;
		}
	}

	return Nearest;
}

FString UOdysseyResourceDistributionSystem::GetRarityDisplayName(EResourceRarity Rarity)
{
	switch (Rarity)
	{
	case EResourceRarity::Common:
		return TEXT("Common");
	case EResourceRarity::Uncommon:
		return TEXT("Uncommon");
	case EResourceRarity::Rare:
		return TEXT("Rare");
	case EResourceRarity::VeryRare:
		return TEXT("Very Rare");
	case EResourceRarity::Exotic:
		return TEXT("Exotic");
	case EResourceRarity::Legendary:
		return TEXT("Legendary");
	default:
		return TEXT("Unknown");
	}
}

FLinearColor UOdysseyResourceDistributionSystem::GetRarityColor(EResourceRarity Rarity)
{
	switch (Rarity)
	{
	case EResourceRarity::Common:
		return FLinearColor(0.7f, 0.7f, 0.7f, 1.0f); // Gray
	case EResourceRarity::Uncommon:
		return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f); // Green
	case EResourceRarity::Rare:
		return FLinearColor(0.2f, 0.4f, 1.0f, 1.0f); // Blue
	case EResourceRarity::VeryRare:
		return FLinearColor(0.6f, 0.2f, 0.8f, 1.0f); // Purple
	case EResourceRarity::Exotic:
		return FLinearColor(1.0f, 0.6f, 0.0f, 1.0f); // Orange
	case EResourceRarity::Legendary:
		return FLinearColor(1.0f, 0.85f, 0.0f, 1.0f); // Gold
	default:
		return FLinearColor::White;
	}
}

float UOdysseyResourceDistributionSystem::GetRarityValueMultiplier(EResourceRarity Rarity)
{
	switch (Rarity)
	{
	case EResourceRarity::Common:
		return 1.0f;
	case EResourceRarity::Uncommon:
		return 1.5f;
	case EResourceRarity::Rare:
		return 2.5f;
	case EResourceRarity::VeryRare:
		return 5.0f;
	case EResourceRarity::Exotic:
		return 10.0f;
	case EResourceRarity::Legendary:
		return 25.0f;
	default:
		return 1.0f;
	}
}

TArray<FVector2D> UOdysseyResourceDistributionSystem::GenerateClusterCenters(
	int32 Seed,
	const FVector2D& AreaSize,
	float MinSpacing,
	int32 MaxAttempts) const
{
	TArray<FVector2D> Points;
	
	// Simple Poisson disk sampling approximation
	int32 Attempts = 0;
	int32 TargetPoints = static_cast<int32>((AreaSize.X * AreaSize.Y) / (MinSpacing * MinSpacing * 3.14159f));
	TargetPoints = FMath::Min(TargetPoints, MaxAttempts);

	while (Points.Num() < TargetPoints && Attempts < MaxAttempts)
	{
		FVector2D Candidate = SeededRandomPoint(Seed + Attempts, FVector2D::ZeroVector, AreaSize);

		// Check distance from all existing points
		bool bValid = true;
		for (const FVector2D& Existing : Points)
		{
			if (FVector2D::Distance(Candidate, Existing) < MinSpacing)
			{
				bValid = false;
				break;
			}
		}

		if (bValid)
		{
			Points.Add(Candidate);
		}

		Attempts++;
	}

	return Points;
}

TArray<FResourceDepositLocation> UOdysseyResourceDistributionSystem::GenerateClusterDeposits(
	int32 Seed,
	const FVector& ClusterCenter,
	float ClusterRadius,
	EResourceType PrimaryResource,
	EBiomeType Biome,
	int32 DepositCount)
{
	TArray<FResourceDepositLocation> Deposits;

	for (int32 i = 0; i < DepositCount; ++i)
	{
		// Generate position within cluster
		float Angle = SeededRandom(Seed + i * 10) * 2.0f * PI;
		float Distance = SeededRandom(Seed + i * 20) * ClusterRadius;
		
		FVector Offset(
			FMath::Cos(Angle) * Distance,
			FMath::Sin(Angle) * Distance,
			0.0f
		);

		FVector DepositLocation = ClusterCenter + Offset;

		// 70% chance of primary resource, 30% chance of random biome resource
		EResourceType ResourceType = PrimaryResource;
		if (SeededRandom(Seed + i * 30) > 0.7f)
		{
			ResourceType = SelectResourceForBiome(Seed + i * 40, Biome);
		}

		FResourceDepositLocation Deposit = GenerateSingleDeposit(
			Seed + i * 100,
			DepositLocation,
			Biome,
			ResourceType
		);

		Deposits.Add(Deposit);
	}

	return Deposits;
}

float UOdysseyResourceDistributionSystem::GetAdjustedDensity(EResourceType ResourceType, EBiomeType Biome) const
{
	const FResourceDistributionParams* Params = DistributionParameters.Find(ResourceType);
	if (!Params)
	{
		return 0.5f;
	}

	float BaseDensity = Params->BaseDensity;

	// Apply biome abundance modifier
	if (BiomeDefinitionSystem)
	{
		float AbundanceModifier = BiomeDefinitionSystem->GetResourceAbundanceModifier(Biome, ResourceType);
		BaseDensity *= AbundanceModifier;
	}

	return BaseDensity;
}

EResourceType UOdysseyResourceDistributionSystem::SelectResourceForBiome(int32 Seed, EBiomeType Biome) const
{
	if (BiomeDefinitionSystem)
	{
		return BiomeDefinitionSystem->SelectResourceFromBiome(Biome, Seed);
	}

	// Fallback: random selection from available resources
	TArray<EResourceType> AvailableResources;
	DistributionParameters.GetKeys(AvailableResources);

	if (AvailableResources.Num() > 0)
	{
		int32 Index = SeededRandomRange(Seed, 0, AvailableResources.Num() - 1);
		return AvailableResources[Index];
	}

	return EResourceType::Silicate;
}

uint32 UOdysseyResourceDistributionSystem::HashSeed(int32 Seed)
{
	uint32 Hash = static_cast<uint32>(Seed);
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = (Hash >> 16) ^ Hash;
	return Hash;
}

float UOdysseyResourceDistributionSystem::SeededRandom(int32 Seed)
{
	uint32 Hash = HashSeed(Seed);
	return static_cast<float>(Hash & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
}

int32 UOdysseyResourceDistributionSystem::SeededRandomRange(int32 Seed, int32 Min, int32 Max)
{
	if (Min >= Max) return Min;
	float Random = SeededRandom(Seed);
	return Min + static_cast<int32>(Random * (Max - Min + 1));
}

FVector2D UOdysseyResourceDistributionSystem::SeededRandomPoint(int32 Seed, const FVector2D& Min, const FVector2D& Max)
{
	return FVector2D(
		Min.X + SeededRandom(Seed) * (Max.X - Min.X),
		Min.Y + SeededRandom(Seed + 1) * (Max.Y - Min.Y)
	);
}
