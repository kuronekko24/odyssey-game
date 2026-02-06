// OdysseyBiomeDefinitionSystem.cpp
// Implementation of the biome definition and management system

#include "OdysseyBiomeDefinitionSystem.h"
#include "Engine/DataTable.h"

UOdysseyBiomeDefinitionSystem::UOdysseyBiomeDefinitionSystem()
	: BiomeDataTableRef(nullptr)
{
}

void UOdysseyBiomeDefinitionSystem::Initialize(UDataTable* BiomeDataTable)
{
	BiomeDataTableRef = BiomeDataTable;
	
	// Initialize default biomes first
	InitializeDefaultBiomes();
	
	// Initialize compatibility rules
	InitializeBiomeCompatibility();
	
	// Override with data table if provided
	if (BiomeDataTableRef)
	{
		LoadBiomesFromDataTable();
	}
}

void UOdysseyBiomeDefinitionSystem::InitializeDefaultBiomes()
{
	BiomeDefinitions.Empty();
	
	BiomeDefinitions.Add(EBiomeType::Desert, CreateDesertBiome());
	BiomeDefinitions.Add(EBiomeType::Ice, CreateIceBiome());
	BiomeDefinitions.Add(EBiomeType::Forest, CreateForestBiome());
	BiomeDefinitions.Add(EBiomeType::Volcanic, CreateVolcanicBiome());
	BiomeDefinitions.Add(EBiomeType::Ocean, CreateOceanBiome());
	BiomeDefinitions.Add(EBiomeType::Crystalline, CreateCrystallineBiome());
	BiomeDefinitions.Add(EBiomeType::Toxic, CreateToxicBiome());
	BiomeDefinitions.Add(EBiomeType::Barren, CreateBarrenBiome());
	BiomeDefinitions.Add(EBiomeType::Lush, CreateLushBiome());
	BiomeDefinitions.Add(EBiomeType::Radioactive, CreateRadioactiveBiome());
	BiomeDefinitions.Add(EBiomeType::Metallic, CreateMetallicBiome());
	BiomeDefinitions.Add(EBiomeType::Anomalous, CreateAnomalousBiome());
}

void UOdysseyBiomeDefinitionSystem::InitializeBiomeCompatibility()
{
	BiomeCompatibility.Empty();
	
	// Desert compatible biomes
	BiomeCompatibility.Add(EBiomeType::Desert, {
		EBiomeType::Barren, EBiomeType::Volcanic, EBiomeType::Crystalline, EBiomeType::Metallic
	});
	
	// Ice compatible biomes
	BiomeCompatibility.Add(EBiomeType::Ice, {
		EBiomeType::Barren, EBiomeType::Ocean, EBiomeType::Crystalline
	});
	
	// Forest compatible biomes
	BiomeCompatibility.Add(EBiomeType::Forest, {
		EBiomeType::Lush, EBiomeType::Ocean, EBiomeType::Toxic
	});
	
	// Volcanic compatible biomes
	BiomeCompatibility.Add(EBiomeType::Volcanic, {
		EBiomeType::Desert, EBiomeType::Barren, EBiomeType::Metallic, EBiomeType::Radioactive
	});
	
	// Ocean compatible biomes
	BiomeCompatibility.Add(EBiomeType::Ocean, {
		EBiomeType::Ice, EBiomeType::Forest, EBiomeType::Lush, EBiomeType::Toxic
	});
	
	// Crystalline compatible biomes
	BiomeCompatibility.Add(EBiomeType::Crystalline, {
		EBiomeType::Desert, EBiomeType::Ice, EBiomeType::Anomalous, EBiomeType::Radioactive
	});
	
	// Toxic compatible biomes
	BiomeCompatibility.Add(EBiomeType::Toxic, {
		EBiomeType::Forest, EBiomeType::Ocean, EBiomeType::Radioactive, EBiomeType::Volcanic
	});
	
	// Barren compatible biomes
	BiomeCompatibility.Add(EBiomeType::Barren, {
		EBiomeType::Desert, EBiomeType::Ice, EBiomeType::Volcanic, EBiomeType::Metallic
	});
	
	// Lush compatible biomes
	BiomeCompatibility.Add(EBiomeType::Lush, {
		EBiomeType::Forest, EBiomeType::Ocean
	});
	
	// Radioactive compatible biomes
	BiomeCompatibility.Add(EBiomeType::Radioactive, {
		EBiomeType::Volcanic, EBiomeType::Crystalline, EBiomeType::Toxic, EBiomeType::Anomalous
	});
	
	// Metallic compatible biomes
	BiomeCompatibility.Add(EBiomeType::Metallic, {
		EBiomeType::Desert, EBiomeType::Volcanic, EBiomeType::Barren
	});
	
	// Anomalous compatible biomes (can transition to any)
	BiomeCompatibility.Add(EBiomeType::Anomalous, {
		EBiomeType::Crystalline, EBiomeType::Radioactive, EBiomeType::Desert,
		EBiomeType::Ice, EBiomeType::Forest, EBiomeType::Volcanic
	});
}

void UOdysseyBiomeDefinitionSystem::LoadBiomesFromDataTable()
{
	if (!BiomeDataTableRef) return;
	
	TArray<FBiomeDefinition*> Rows;
	BiomeDataTableRef->GetAllRows<FBiomeDefinition>(TEXT("LoadBiomesFromDataTable"), Rows);
	
	for (const FBiomeDefinition* Row : Rows)
	{
		if (Row && Row->BiomeType != EBiomeType::None)
		{
			BiomeDefinitions.Add(Row->BiomeType, *Row);
		}
	}
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::GetBiomeDefinition(EBiomeType BiomeType) const
{
	if (const FBiomeDefinition* Definition = BiomeDefinitions.Find(BiomeType))
	{
		return *Definition;
	}
	return FBiomeDefinition();
}

TArray<FBiomeDefinition> UOdysseyBiomeDefinitionSystem::GetAllBiomeDefinitions() const
{
	TArray<FBiomeDefinition> Result;
	BiomeDefinitions.GenerateValueArray(Result);
	return Result;
}

bool UOdysseyBiomeDefinitionSystem::HasBiomeDefinition(EBiomeType BiomeType) const
{
	return BiomeDefinitions.Contains(BiomeType);
}

EBiomeType UOdysseyBiomeDefinitionSystem::SelectBiomeFromSeed(int32 Seed, float TemperatureHint, float MoistureHint) const
{
	// Create weighted selection based on temperature and moisture hints
	TArray<TPair<EBiomeType, float>> WeightedBiomes;
	float TotalWeight = 0.0f;
	
	for (const auto& Pair : BiomeDefinitions)
	{
		const FBiomeDefinition& Biome = Pair.Value;
		float Weight = Biome.RarityWeight;
		
		// Adjust weight based on temperature compatibility
		float BiomeTemp = (Biome.MinTemperature + Biome.MaxTemperature) / 2.0f;
		float NormalizedBiomeTemp = FMath::GetMappedRangeValueClamped(FVector2D(-100.0f, 200.0f), FVector2D(0.0f, 1.0f), BiomeTemp);
		float TempDiff = FMath::Abs(NormalizedBiomeTemp - TemperatureHint);
		Weight *= FMath::Exp(-TempDiff * 2.0f);
		
		// Adjust weight based on moisture (ocean/forest like high moisture, desert/volcanic like low)
		float MoisturePref = 0.5f;
		if (Biome.BiomeType == EBiomeType::Ocean || Biome.BiomeType == EBiomeType::Lush || Biome.BiomeType == EBiomeType::Forest)
		{
			MoisturePref = 0.8f;
		}
		else if (Biome.BiomeType == EBiomeType::Desert || Biome.BiomeType == EBiomeType::Volcanic || Biome.BiomeType == EBiomeType::Barren)
		{
			MoisturePref = 0.2f;
		}
		float MoistureDiff = FMath::Abs(MoisturePref - MoistureHint);
		Weight *= FMath::Exp(-MoistureDiff * 1.5f);
		
		if (Weight > 0.001f)
		{
			WeightedBiomes.Add(TPair<EBiomeType, float>(Pair.Key, Weight));
			TotalWeight += Weight;
		}
	}
	
	if (WeightedBiomes.Num() == 0 || TotalWeight <= 0.0f)
	{
		return EBiomeType::Barren;
	}
	
	// Select based on seed
	float RandomValue = SeededRandom(Seed) * TotalWeight;
	float AccumulatedWeight = 0.0f;
	
	for (const auto& Pair : WeightedBiomes)
	{
		AccumulatedWeight += Pair.Value;
		if (RandomValue <= AccumulatedWeight)
		{
			return Pair.Key;
		}
	}
	
	return WeightedBiomes.Last().Key;
}

TArray<EBiomeType> UOdysseyBiomeDefinitionSystem::GeneratePlanetBiomes(int32 PlanetSeed, int32 BiomeCount) const
{
	TArray<EBiomeType> Result;
	BiomeCount = FMath::Clamp(BiomeCount, 1, 6);
	
	// Generate base temperature and moisture for the planet
	float BaseTemperature = SeededRandom(PlanetSeed);
	float BaseMoisture = SeededRandom(PlanetSeed + 1000);
	
	// Select primary biome
	EBiomeType PrimaryBiome = SelectBiomeFromSeed(PlanetSeed + 2000, BaseTemperature, BaseMoisture);
	Result.Add(PrimaryBiome);
	
	// Select additional compatible biomes
	for (int32 i = 1; i < BiomeCount; ++i)
	{
		EBiomeType LastBiome = Result.Last();
		
		// Get compatible biomes for the last added biome
		const TArray<EBiomeType>* CompatibleBiomes = BiomeCompatibility.Find(LastBiome);
		
		if (CompatibleBiomes && CompatibleBiomes->Num() > 0)
		{
			// Select from compatible biomes
			int32 Index = SeededRandomRange(PlanetSeed + i * 100, 0, CompatibleBiomes->Num() - 1);
			EBiomeType NewBiome = (*CompatibleBiomes)[Index];
			
			// Avoid duplicates
			if (!Result.Contains(NewBiome))
			{
				Result.Add(NewBiome);
			}
			else
			{
				// Try to find another compatible biome
				for (const EBiomeType& CompatBiome : *CompatibleBiomes)
				{
					if (!Result.Contains(CompatBiome))
					{
						Result.Add(CompatBiome);
						break;
					}
				}
			}
		}
		else
		{
			// Fallback: add variation of primary biome environment
			float TempVariation = SeededRandom(PlanetSeed + i * 50) * 0.4f - 0.2f;
			float MoistVariation = SeededRandom(PlanetSeed + i * 75) * 0.4f - 0.2f;
			EBiomeType NewBiome = SelectBiomeFromSeed(PlanetSeed + i * 200, 
				FMath::Clamp(BaseTemperature + TempVariation, 0.0f, 1.0f),
				FMath::Clamp(BaseMoisture + MoistVariation, 0.0f, 1.0f));
			
			if (!Result.Contains(NewBiome))
			{
				Result.Add(NewBiome);
			}
		}
	}
	
	return Result;
}

TArray<FBiomeResourceWeight> UOdysseyBiomeDefinitionSystem::GetBiomeResources(EBiomeType BiomeType) const
{
	if (const FBiomeDefinition* Definition = BiomeDefinitions.Find(BiomeType))
	{
		return Definition->ResourceWeights;
	}
	return TArray<FBiomeResourceWeight>();
}

EResourceType UOdysseyBiomeDefinitionSystem::SelectResourceFromBiome(EBiomeType BiomeType, int32 Seed) const
{
	TArray<FBiomeResourceWeight> Resources = GetBiomeResources(BiomeType);
	
	if (Resources.Num() == 0)
	{
		return EResourceType::Silicate; // Default fallback
	}
	
	// Calculate total weight
	float TotalWeight = 0.0f;
	for (const FBiomeResourceWeight& Resource : Resources)
	{
		TotalWeight += Resource.SpawnWeight;
	}
	
	if (TotalWeight <= 0.0f)
	{
		return Resources[0].ResourceType;
	}
	
	// Select based on weight
	float RandomValue = SeededRandom(Seed) * TotalWeight;
	float AccumulatedWeight = 0.0f;
	
	for (const FBiomeResourceWeight& Resource : Resources)
	{
		AccumulatedWeight += Resource.SpawnWeight;
		if (RandomValue <= AccumulatedWeight)
		{
			return Resource.ResourceType;
		}
	}
	
	return Resources.Last().ResourceType;
}

float UOdysseyBiomeDefinitionSystem::GetResourceQualityModifier(EBiomeType BiomeType, EResourceType ResourceType) const
{
	TArray<FBiomeResourceWeight> Resources = GetBiomeResources(BiomeType);
	
	for (const FBiomeResourceWeight& Resource : Resources)
	{
		if (Resource.ResourceType == ResourceType)
		{
			return Resource.QualityModifier;
		}
	}
	
	return 1.0f;
}

float UOdysseyBiomeDefinitionSystem::GetResourceAbundanceModifier(EBiomeType BiomeType, EResourceType ResourceType) const
{
	TArray<FBiomeResourceWeight> Resources = GetBiomeResources(BiomeType);
	
	for (const FBiomeResourceWeight& Resource : Resources)
	{
		if (Resource.ResourceType == ResourceType)
		{
			return Resource.AbundanceModifier;
		}
	}
	
	return 1.0f;
}

FBiomeVisualData UOdysseyBiomeDefinitionSystem::GetBiomeVisualData(EBiomeType BiomeType) const
{
	if (const FBiomeDefinition* Definition = BiomeDefinitions.Find(BiomeType))
	{
		return Definition->VisualData;
	}
	return FBiomeVisualData();
}

FBiomeVisualData UOdysseyBiomeDefinitionSystem::BlendBiomeVisuals(EBiomeType BiomeA, EBiomeType BiomeB, float BlendFactor) const
{
	FBiomeVisualData VisualA = GetBiomeVisualData(BiomeA);
	FBiomeVisualData VisualB = GetBiomeVisualData(BiomeB);
	
	FBiomeVisualData Result;
	Result.PrimaryColor = FMath::Lerp(VisualA.PrimaryColor, VisualB.PrimaryColor, BlendFactor);
	Result.SecondaryColor = FMath::Lerp(VisualA.SecondaryColor, VisualB.SecondaryColor, BlendFactor);
	Result.AtmosphericTint = FMath::Lerp(VisualA.AtmosphericTint, VisualB.AtmosphericTint, BlendFactor);
	Result.FogDensity = FMath::Lerp(VisualA.FogDensity, VisualB.FogDensity, BlendFactor);
	Result.AmbientLightIntensity = FMath::Lerp(VisualA.AmbientLightIntensity, VisualB.AmbientLightIntensity, BlendFactor);
	Result.ParticleIntensity = FMath::Lerp(VisualA.ParticleIntensity, VisualB.ParticleIntensity, BlendFactor);
	
	return Result;
}

FBiomeGameplayModifiers UOdysseyBiomeDefinitionSystem::GetBiomeGameplayModifiers(EBiomeType BiomeType) const
{
	if (const FBiomeDefinition* Definition = BiomeDefinitions.Find(BiomeType))
	{
		return Definition->GameplayModifiers;
	}
	return FBiomeGameplayModifiers();
}

float UOdysseyBiomeDefinitionSystem::GetEnvironmentalDamage(EBiomeType BiomeType) const
{
	FBiomeGameplayModifiers Modifiers = GetBiomeGameplayModifiers(BiomeType);
	return Modifiers.EnvironmentalDamagePerSecond;
}

EEnvironmentalHazard UOdysseyBiomeDefinitionSystem::GetPrimaryHazard(EBiomeType BiomeType) const
{
	if (const FBiomeDefinition* Definition = BiomeDefinitions.Find(BiomeType))
	{
		return Definition->PrimaryHazard;
	}
	return EEnvironmentalHazard::None;
}

FBiomeTransition UOdysseyBiomeDefinitionSystem::GetTransitionData(EBiomeType FromBiome, EBiomeType ToBiome) const
{
	FBiomeTransition Transition;
	Transition.FromBiome = FromBiome;
	Transition.ToBiome = ToBiome;
	
	// Default transition widths based on biome similarity
	if (AreBiomesCompatible(FromBiome, ToBiome))
	{
		Transition.TransitionWidth = 150.0f;
		Transition.BlendExponent = 1.0f;
	}
	else
	{
		Transition.TransitionWidth = 50.0f;
		Transition.BlendExponent = 2.0f; // Sharper transition for incompatible biomes
	}
	
	return Transition;
}

bool UOdysseyBiomeDefinitionSystem::AreBiomesCompatible(EBiomeType BiomeA, EBiomeType BiomeB) const
{
	if (BiomeA == BiomeB) return true;
	
	const TArray<EBiomeType>* CompatibleBiomes = BiomeCompatibility.Find(BiomeA);
	if (CompatibleBiomes)
	{
		return CompatibleBiomes->Contains(BiomeB);
	}
	
	return false;
}

FText UOdysseyBiomeDefinitionSystem::GetBiomeDisplayName(EBiomeType BiomeType) const
{
	if (const FBiomeDefinition* Definition = BiomeDefinitions.Find(BiomeType))
	{
		return Definition->DisplayName;
	}
	return FText::FromString(TEXT("Unknown"));
}

FText UOdysseyBiomeDefinitionSystem::GetBiomeDescription(EBiomeType BiomeType) const
{
	if (const FBiomeDefinition* Definition = BiomeDefinitions.Find(BiomeType))
	{
		return Definition->Description;
	}
	return FText::FromString(TEXT("An uncharted biome."));
}

int32 UOdysseyBiomeDefinitionSystem::GetBiomeExplorationScore(EBiomeType BiomeType) const
{
	if (const FBiomeDefinition* Definition = BiomeDefinitions.Find(BiomeType))
	{
		return Definition->BaseExplorationScore;
	}
	return 0;
}

float UOdysseyBiomeDefinitionSystem::SeededRandom(int32 Seed)
{
	uint32 Hash = HashSeed(Seed);
	return static_cast<float>(Hash & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
}

int32 UOdysseyBiomeDefinitionSystem::SeededRandomRange(int32 Seed, int32 Min, int32 Max)
{
	if (Min >= Max) return Min;
	float Random = SeededRandom(Seed);
	return Min + static_cast<int32>(Random * (Max - Min + 1));
}

uint32 UOdysseyBiomeDefinitionSystem::HashSeed(int32 Seed)
{
	// Simple hash function for seeded randomness
	uint32 Hash = static_cast<uint32>(Seed);
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = (Hash >> 16) ^ Hash;
	return Hash;
}

// Biome Creation Functions

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateDesertBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Desert;
	Biome.DisplayName = FText::FromString(TEXT("Desert"));
	Biome.Description = FText::FromString(TEXT("Arid wasteland with extreme temperature fluctuations. Rich in silicate deposits but scarce in organic materials."));
	Biome.PrimaryHazard = EEnvironmentalHazard::ExtremeHeat;
	Biome.SecondaryHazard = EEnvironmentalHazard::None;
	Biome.HazardIntensity = 0.6f;
	
	// Resources: high silicate, moderate carbon, low organic
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.7f, 1.2f, 1.5f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Carbon, 0.2f, 0.8f, 0.5f));
	
	// Visual data
	Biome.VisualData.PrimaryColor = FLinearColor(0.85f, 0.75f, 0.5f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.7f, 0.55f, 0.35f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(1.0f, 0.9f, 0.7f, 1.0f);
	Biome.VisualData.FogDensity = 0.15f;
	Biome.VisualData.AmbientLightIntensity = 1.3f;
	Biome.VisualData.ParticleIntensity = 0.4f; // Dust particles
	
	// Gameplay modifiers
	Biome.GameplayModifiers.MovementSpeedModifier = 0.9f;
	Biome.GameplayModifiers.MiningSpeedModifier = 1.1f; // Easy to mine
	Biome.GameplayModifiers.EnergyConsumptionModifier = 1.3f;
	Biome.GameplayModifiers.VisibilityModifier = 0.8f;
	Biome.GameplayModifiers.ScanRangeModifier = 1.2f;
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 2.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 0.5f;
	
	Biome.BaseExplorationScore = 80;
	Biome.RarityWeight = 0.6f;
	Biome.MinTemperature = 25.0f;
	Biome.MaxTemperature = 120.0f;
	Biome.GravityModifier = 1.0f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateIceBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Ice;
	Biome.DisplayName = FText::FromString(TEXT("Ice"));
	Biome.Description = FText::FromString(TEXT("Frozen landscape with valuable crystalline formations beneath the surface. Extreme cold requires thermal protection."));
	Biome.PrimaryHazard = EEnvironmentalHazard::ExtremeCold;
	Biome.SecondaryHazard = EEnvironmentalHazard::None;
	Biome.HazardIntensity = 0.7f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Carbon, 0.5f, 1.0f, 1.0f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.3f, 1.1f, 0.8f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.85f, 0.92f, 1.0f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.7f, 0.85f, 0.95f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.8f, 0.9f, 1.0f, 1.0f);
	Biome.VisualData.FogDensity = 0.3f;
	Biome.VisualData.AmbientLightIntensity = 0.9f;
	Biome.VisualData.ParticleIntensity = 0.6f; // Snow particles
	
	Biome.GameplayModifiers.MovementSpeedModifier = 0.75f;
	Biome.GameplayModifiers.MiningSpeedModifier = 0.8f;
	Biome.GameplayModifiers.EnergyConsumptionModifier = 1.5f;
	Biome.GameplayModifiers.VisibilityModifier = 0.6f;
	Biome.GameplayModifiers.ScanRangeModifier = 0.9f;
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 3.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 0.8f;
	
	Biome.BaseExplorationScore = 100;
	Biome.RarityWeight = 0.5f;
	Biome.MinTemperature = -120.0f;
	Biome.MaxTemperature = -20.0f;
	Biome.GravityModifier = 0.9f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateForestBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Forest;
	Biome.DisplayName = FText::FromString(TEXT("Forest"));
	Biome.Description = FText::FromString(TEXT("Dense alien vegetation with abundant organic resources. Watch for hostile fauna and difficult terrain."));
	Biome.PrimaryHazard = EEnvironmentalHazard::None;
	Biome.SecondaryHazard = EEnvironmentalHazard::None;
	Biome.HazardIntensity = 0.2f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Carbon, 0.8f, 1.3f, 1.5f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.2f, 0.9f, 0.6f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.2f, 0.5f, 0.25f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.15f, 0.35f, 0.2f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.7f, 0.9f, 0.7f, 1.0f);
	Biome.VisualData.FogDensity = 0.25f;
	Biome.VisualData.AmbientLightIntensity = 0.7f;
	Biome.VisualData.ParticleIntensity = 0.3f; // Spores/pollen
	
	Biome.GameplayModifiers.MovementSpeedModifier = 0.85f;
	Biome.GameplayModifiers.MiningSpeedModifier = 0.9f;
	Biome.GameplayModifiers.EnergyConsumptionModifier = 1.0f;
	Biome.GameplayModifiers.VisibilityModifier = 0.5f;
	Biome.GameplayModifiers.ScanRangeModifier = 0.7f;
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 0.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 0.0f;
	
	Biome.BaseExplorationScore = 120;
	Biome.RarityWeight = 0.4f;
	Biome.MinTemperature = 10.0f;
	Biome.MaxTemperature = 35.0f;
	Biome.GravityModifier = 1.0f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateVolcanicBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Volcanic;
	Biome.DisplayName = FText::FromString(TEXT("Volcanic"));
	Biome.Description = FText::FromString(TEXT("Highly active geological region with valuable mineral deposits. Extreme heat and unstable terrain pose significant dangers."));
	Biome.PrimaryHazard = EEnvironmentalHazard::ExtremeHeat;
	Biome.SecondaryHazard = EEnvironmentalHazard::SeismicActivity;
	Biome.HazardIntensity = 0.85f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.6f, 1.5f, 1.3f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::RefinedSilicate, 0.15f, 1.2f, 0.5f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.3f, 0.15f, 0.1f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.8f, 0.3f, 0.1f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(1.0f, 0.6f, 0.4f, 1.0f);
	Biome.VisualData.FogDensity = 0.35f;
	Biome.VisualData.AmbientLightIntensity = 0.6f;
	Biome.VisualData.ParticleIntensity = 0.7f; // Ash/ember particles
	
	Biome.GameplayModifiers.MovementSpeedModifier = 0.8f;
	Biome.GameplayModifiers.MiningSpeedModifier = 1.2f;
	Biome.GameplayModifiers.EnergyConsumptionModifier = 1.8f;
	Biome.GameplayModifiers.VisibilityModifier = 0.5f;
	Biome.GameplayModifiers.ScanRangeModifier = 0.6f;
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 5.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 1.2f;
	
	Biome.BaseExplorationScore = 150;
	Biome.RarityWeight = 0.35f;
	Biome.MinTemperature = 60.0f;
	Biome.MaxTemperature = 250.0f;
	Biome.GravityModifier = 1.1f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateOceanBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Ocean;
	Biome.DisplayName = FText::FromString(TEXT("Ocean"));
	Biome.Description = FText::FromString(TEXT("Vast liquid expanse with unique aquatic resources. Requires specialized equipment for deep exploration."));
	Biome.PrimaryHazard = EEnvironmentalHazard::HighGravity;
	Biome.SecondaryHazard = EEnvironmentalHazard::None;
	Biome.HazardIntensity = 0.3f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Carbon, 0.6f, 1.1f, 1.2f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.3f, 1.0f, 0.8f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.1f, 0.3f, 0.6f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.15f, 0.4f, 0.5f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.6f, 0.8f, 1.0f, 1.0f);
	Biome.VisualData.FogDensity = 0.4f;
	Biome.VisualData.AmbientLightIntensity = 0.6f;
	Biome.VisualData.ParticleIntensity = 0.2f;
	
	Biome.GameplayModifiers.MovementSpeedModifier = 0.6f;
	Biome.GameplayModifiers.MiningSpeedModifier = 0.7f;
	Biome.GameplayModifiers.EnergyConsumptionModifier = 1.4f;
	Biome.GameplayModifiers.VisibilityModifier = 0.4f;
	Biome.GameplayModifiers.ScanRangeModifier = 1.5f; // Sonar works well
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 1.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 0.3f;
	
	Biome.BaseExplorationScore = 130;
	Biome.RarityWeight = 0.3f;
	Biome.MinTemperature = 5.0f;
	Biome.MaxTemperature = 30.0f;
	Biome.GravityModifier = 1.0f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateCrystallineBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Crystalline;
	Biome.DisplayName = FText::FromString(TEXT("Crystalline"));
	Biome.Description = FText::FromString(TEXT("Rare geological formation with valuable crystal structures. High value resources but difficult extraction."));
	Biome.PrimaryHazard = EEnvironmentalHazard::Radiation;
	Biome.SecondaryHazard = EEnvironmentalHazard::None;
	Biome.HazardIntensity = 0.4f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::RefinedSilicate, 0.5f, 1.5f, 1.0f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.4f, 1.3f, 1.2f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.7f, 0.5f, 0.9f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.5f, 0.8f, 0.9f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.9f, 0.8f, 1.0f, 1.0f);
	Biome.VisualData.FogDensity = 0.1f;
	Biome.VisualData.AmbientLightIntensity = 1.2f;
	Biome.VisualData.ParticleIntensity = 0.5f; // Crystal dust
	
	Biome.GameplayModifiers.MovementSpeedModifier = 0.9f;
	Biome.GameplayModifiers.MiningSpeedModifier = 0.6f; // Hard to mine
	Biome.GameplayModifiers.EnergyConsumptionModifier = 1.1f;
	Biome.GameplayModifiers.VisibilityModifier = 1.3f;
	Biome.GameplayModifiers.ScanRangeModifier = 0.8f;
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 1.5f;
	Biome.GameplayModifiers.ShieldDrainModifier = 0.4f;
	
	Biome.BaseExplorationScore = 180;
	Biome.RarityWeight = 0.2f;
	Biome.MinTemperature = -30.0f;
	Biome.MaxTemperature = 50.0f;
	Biome.GravityModifier = 0.85f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateToxicBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Toxic;
	Biome.DisplayName = FText::FromString(TEXT("Toxic"));
	Biome.Description = FText::FromString(TEXT("Hazardous environment with corrosive atmosphere. Contains unique chemical compounds valuable for advanced synthesis."));
	Biome.PrimaryHazard = EEnvironmentalHazard::ToxicAtmosphere;
	Biome.SecondaryHazard = EEnvironmentalHazard::AcidRain;
	Biome.HazardIntensity = 0.75f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Carbon, 0.5f, 1.4f, 1.1f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::RefinedCarbon, 0.2f, 1.3f, 0.6f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.4f, 0.5f, 0.2f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.6f, 0.7f, 0.3f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.7f, 0.9f, 0.5f, 1.0f);
	Biome.VisualData.FogDensity = 0.5f;
	Biome.VisualData.AmbientLightIntensity = 0.5f;
	Biome.VisualData.ParticleIntensity = 0.8f; // Toxic spores
	
	Biome.GameplayModifiers.MovementSpeedModifier = 0.85f;
	Biome.GameplayModifiers.MiningSpeedModifier = 1.0f;
	Biome.GameplayModifiers.EnergyConsumptionModifier = 1.6f;
	Biome.GameplayModifiers.VisibilityModifier = 0.4f;
	Biome.GameplayModifiers.ScanRangeModifier = 0.5f;
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 4.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 1.5f;
	
	Biome.BaseExplorationScore = 140;
	Biome.RarityWeight = 0.35f;
	Biome.MinTemperature = 20.0f;
	Biome.MaxTemperature = 60.0f;
	Biome.GravityModifier = 1.0f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateBarrenBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Barren;
	Biome.DisplayName = FText::FromString(TEXT("Barren"));
	Biome.Description = FText::FromString(TEXT("Desolate landscape with minimal resources. Low hazard but equally low reward."));
	Biome.PrimaryHazard = EEnvironmentalHazard::None;
	Biome.SecondaryHazard = EEnvironmentalHazard::None;
	Biome.HazardIntensity = 0.1f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.6f, 0.8f, 0.5f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Carbon, 0.3f, 0.7f, 0.3f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.5f, 0.45f, 0.4f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.4f, 0.35f, 0.3f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.8f, 0.75f, 0.7f, 1.0f);
	Biome.VisualData.FogDensity = 0.05f;
	Biome.VisualData.AmbientLightIntensity = 1.0f;
	Biome.VisualData.ParticleIntensity = 0.1f;
	
	Biome.GameplayModifiers.MovementSpeedModifier = 1.1f;
	Biome.GameplayModifiers.MiningSpeedModifier = 1.0f;
	Biome.GameplayModifiers.EnergyConsumptionModifier = 0.9f;
	Biome.GameplayModifiers.VisibilityModifier = 1.2f;
	Biome.GameplayModifiers.ScanRangeModifier = 1.3f;
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 0.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 0.0f;
	
	Biome.BaseExplorationScore = 50;
	Biome.RarityWeight = 0.7f;
	Biome.MinTemperature = -50.0f;
	Biome.MaxTemperature = 80.0f;
	Biome.GravityModifier = 0.9f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateLushBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Lush;
	Biome.DisplayName = FText::FromString(TEXT("Lush"));
	Biome.Description = FText::FromString(TEXT("Paradise-like environment with abundant life and resources. Ideal conditions but potentially competitive."));
	Biome.PrimaryHazard = EEnvironmentalHazard::None;
	Biome.SecondaryHazard = EEnvironmentalHazard::None;
	Biome.HazardIntensity = 0.0f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Carbon, 0.7f, 1.5f, 2.0f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::RefinedCarbon, 0.15f, 1.2f, 0.8f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.15f, 1.0f, 0.5f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.2f, 0.7f, 0.3f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.3f, 0.8f, 0.5f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.8f, 1.0f, 0.9f, 1.0f);
	Biome.VisualData.FogDensity = 0.15f;
	Biome.VisualData.AmbientLightIntensity = 1.1f;
	Biome.VisualData.ParticleIntensity = 0.4f;
	
	Biome.GameplayModifiers.MovementSpeedModifier = 1.0f;
	Biome.GameplayModifiers.MiningSpeedModifier = 1.1f;
	Biome.GameplayModifiers.EnergyConsumptionModifier = 0.8f;
	Biome.GameplayModifiers.VisibilityModifier = 0.7f;
	Biome.GameplayModifiers.ScanRangeModifier = 0.9f;
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 0.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 0.0f;
	
	Biome.BaseExplorationScore = 160;
	Biome.RarityWeight = 0.15f;
	Biome.MinTemperature = 15.0f;
	Biome.MaxTemperature = 28.0f;
	Biome.GravityModifier = 1.0f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateRadioactiveBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Radioactive;
	Biome.DisplayName = FText::FromString(TEXT("Radioactive"));
	Biome.Description = FText::FromString(TEXT("Highly irradiated zone with unique isotopes. Extreme danger but contains rare energy resources."));
	Biome.PrimaryHazard = EEnvironmentalHazard::Radiation;
	Biome.SecondaryHazard = EEnvironmentalHazard::SolarFlares;
	Biome.HazardIntensity = 0.9f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::RefinedSilicate, 0.4f, 1.6f, 0.8f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.4f, 1.1f, 1.0f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.3f, 0.5f, 0.3f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.5f, 0.8f, 0.4f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.6f, 1.0f, 0.6f, 1.0f);
	Biome.VisualData.FogDensity = 0.2f;
	Biome.VisualData.AmbientLightIntensity = 0.8f;
	Biome.VisualData.ParticleIntensity = 0.6f;
	
	Biome.GameplayModifiers.MovementSpeedModifier = 0.9f;
	Biome.GameplayModifiers.MiningSpeedModifier = 0.9f;
	Biome.GameplayModifiers.EnergyConsumptionModifier = 2.0f;
	Biome.GameplayModifiers.VisibilityModifier = 0.8f;
	Biome.GameplayModifiers.ScanRangeModifier = 0.4f; // Interference
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 8.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 2.0f;
	
	Biome.BaseExplorationScore = 200;
	Biome.RarityWeight = 0.15f;
	Biome.MinTemperature = 10.0f;
	Biome.MaxTemperature = 45.0f;
	Biome.GravityModifier = 1.0f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateMetallicBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Metallic;
	Biome.DisplayName = FText::FromString(TEXT("Metallic"));
	Biome.Description = FText::FromString(TEXT("Metal-rich terrain formed from ancient asteroid impacts. Prime location for advanced material extraction."));
	Biome.PrimaryHazard = EEnvironmentalHazard::ElectricalStorms;
	Biome.SecondaryHazard = EEnvironmentalHazard::None;
	Biome.HazardIntensity = 0.5f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::Silicate, 0.5f, 1.4f, 1.5f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::RefinedSilicate, 0.3f, 1.3f, 0.9f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::CompositeMaterial, 0.1f, 1.2f, 0.3f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.5f, 0.5f, 0.55f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.7f, 0.65f, 0.6f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.85f, 0.85f, 0.9f, 1.0f);
	Biome.VisualData.FogDensity = 0.1f;
	Biome.VisualData.AmbientLightIntensity = 1.0f;
	Biome.VisualData.ParticleIntensity = 0.2f;
	
	Biome.GameplayModifiers.MovementSpeedModifier = 0.95f;
	Biome.GameplayModifiers.MiningSpeedModifier = 0.75f; // Hard materials
	Biome.GameplayModifiers.EnergyConsumptionModifier = 1.2f;
	Biome.GameplayModifiers.VisibilityModifier = 1.0f;
	Biome.GameplayModifiers.ScanRangeModifier = 0.7f; // Metallic interference
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 0.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 0.6f;
	
	Biome.BaseExplorationScore = 170;
	Biome.RarityWeight = 0.25f;
	Biome.MinTemperature = -20.0f;
	Biome.MaxTemperature = 70.0f;
	Biome.GravityModifier = 1.2f;
	
	return Biome;
}

FBiomeDefinition UOdysseyBiomeDefinitionSystem::CreateAnomalousBiome() const
{
	FBiomeDefinition Biome;
	Biome.BiomeType = EBiomeType::Anomalous;
	Biome.DisplayName = FText::FromString(TEXT("Anomalous"));
	Biome.Description = FText::FromString(TEXT("Reality-warping zone with unexplainable phenomena. Contains exotic matter but navigation is extremely difficult."));
	Biome.PrimaryHazard = EEnvironmentalHazard::LowGravity;
	Biome.SecondaryHazard = EEnvironmentalHazard::Radiation;
	Biome.HazardIntensity = 0.8f;
	
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::CompositeMaterial, 0.4f, 2.0f, 0.5f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::RefinedSilicate, 0.3f, 1.5f, 0.7f));
	Biome.ResourceWeights.Add(FBiomeResourceWeight(EResourceType::RefinedCarbon, 0.2f, 1.5f, 0.6f));
	
	Biome.VisualData.PrimaryColor = FLinearColor(0.4f, 0.2f, 0.6f, 1.0f);
	Biome.VisualData.SecondaryColor = FLinearColor(0.2f, 0.4f, 0.7f, 1.0f);
	Biome.VisualData.AtmosphericTint = FLinearColor(0.7f, 0.5f, 0.9f, 1.0f);
	Biome.VisualData.FogDensity = 0.35f;
	Biome.VisualData.AmbientLightIntensity = 0.7f;
	Biome.VisualData.ParticleIntensity = 0.9f;
	
	Biome.GameplayModifiers.MovementSpeedModifier = 0.7f;
	Biome.GameplayModifiers.MiningSpeedModifier = 0.5f;
	Biome.GameplayModifiers.EnergyConsumptionModifier = 2.2f;
	Biome.GameplayModifiers.VisibilityModifier = 0.5f;
	Biome.GameplayModifiers.ScanRangeModifier = 0.3f;
	Biome.GameplayModifiers.EnvironmentalDamagePerSecond = 3.0f;
	Biome.GameplayModifiers.ShieldDrainModifier = 1.0f;
	
	Biome.BaseExplorationScore = 300;
	Biome.RarityWeight = 0.05f;
	Biome.MinTemperature = -100.0f;
	Biome.MaxTemperature = 100.0f;
	Biome.GravityModifier = 0.5f;
	
	return Biome;
}
