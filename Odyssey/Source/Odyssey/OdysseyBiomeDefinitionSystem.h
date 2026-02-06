// OdysseyBiomeDefinitionSystem.h
// Biome definition and management for procedural planet generation
// Part of the Odyssey Procedural Planet & Resource Generation System

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyBiomeDefinitionSystem.generated.h"

// Forward declarations
class UOdysseyResourceDistributionSystem;

/**
 * Enum defining the primary biome types available in the universe
 */
UENUM(BlueprintType)
enum class EBiomeType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Desert = 1 UMETA(DisplayName = "Desert"),
	Ice = 2 UMETA(DisplayName = "Ice"),
	Forest = 3 UMETA(DisplayName = "Forest"),
	Volcanic = 4 UMETA(DisplayName = "Volcanic"),
	Ocean = 5 UMETA(DisplayName = "Ocean"),
	Crystalline = 6 UMETA(DisplayName = "Crystalline"),
	Toxic = 7 UMETA(DisplayName = "Toxic"),
	Barren = 8 UMETA(DisplayName = "Barren"),
	Lush = 9 UMETA(DisplayName = "Lush"),
	Radioactive = 10 UMETA(DisplayName = "Radioactive"),
	Metallic = 11 UMETA(DisplayName = "Metallic"),
	Anomalous = 12 UMETA(DisplayName = "Anomalous")
};

/**
 * Enum for environmental hazard types
 */
UENUM(BlueprintType)
enum class EEnvironmentalHazard : uint8
{
	None = 0,
	ExtremeHeat,
	ExtremeCold,
	ToxicAtmosphere,
	Radiation,
	HighGravity,
	LowGravity,
	AcidRain,
	ElectricalStorms,
	SeismicActivity,
	SolarFlares
};

/**
 * Struct defining resource weight for biome-specific resource distribution
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FBiomeResourceWeight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Resource")
	EResourceType ResourceType;

	// Weight determining spawn probability (0.0 - 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Resource", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnWeight;

	// Quality modifier for this resource in this biome (1.0 = normal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Resource", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float QualityModifier;

	// Abundance modifier (affects quantity found)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Resource", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float AbundanceModifier;

	FBiomeResourceWeight()
		: ResourceType(EResourceType::Silicate)
		, SpawnWeight(0.5f)
		, QualityModifier(1.0f)
		, AbundanceModifier(1.0f)
	{
	}

	FBiomeResourceWeight(EResourceType Type, float Weight, float Quality = 1.0f, float Abundance = 1.0f)
		: ResourceType(Type)
		, SpawnWeight(Weight)
		, QualityModifier(Quality)
		, AbundanceModifier(Abundance)
	{
	}
};

/**
 * Struct defining visual characteristics of a biome
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FBiomeVisualData
{
	GENERATED_BODY()

	// Primary terrain color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Visual")
	FLinearColor PrimaryColor;

	// Secondary/accent color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Visual")
	FLinearColor SecondaryColor;

	// Atmospheric tint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Visual")
	FLinearColor AtmosphericTint;

	// Fog density (0.0 - 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogDensity;

	// Ambient light intensity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Visual", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AmbientLightIntensity;

	// Particle effect intensity (dust, snow, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ParticleIntensity;

	FBiomeVisualData()
		: PrimaryColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
		, SecondaryColor(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f))
		, AtmosphericTint(FLinearColor(0.8f, 0.9f, 1.0f, 1.0f))
		, FogDensity(0.1f)
		, AmbientLightIntensity(1.0f)
		, ParticleIntensity(0.0f)
	{
	}
};

/**
 * Struct defining gameplay modifiers for a biome
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FBiomeGameplayModifiers
{
	GENERATED_BODY()

	// Movement speed modifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Gameplay", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float MovementSpeedModifier;

	// Mining speed modifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Gameplay", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float MiningSpeedModifier;

	// Energy consumption rate modifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Gameplay", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float EnergyConsumptionModifier;

	// Visibility range modifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Gameplay", meta = (ClampMin = "0.2", ClampMax = "2.0"))
	float VisibilityModifier;

	// Scan range modifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Gameplay", meta = (ClampMin = "0.2", ClampMax = "2.0"))
	float ScanRangeModifier;

	// Environmental damage per second (0 = none)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Gameplay", meta = (ClampMin = "0.0"))
	float EnvironmentalDamagePerSecond;

	// Shield drain rate modifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Gameplay", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float ShieldDrainModifier;

	FBiomeGameplayModifiers()
		: MovementSpeedModifier(1.0f)
		, MiningSpeedModifier(1.0f)
		, EnergyConsumptionModifier(1.0f)
		, VisibilityModifier(1.0f)
		, ScanRangeModifier(1.0f)
		, EnvironmentalDamagePerSecond(0.0f)
		, ShieldDrainModifier(0.0f)
	{
	}
};

/**
 * Complete biome definition structure
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FBiomeDefinition : public FTableRowBase
{
	GENERATED_BODY()

	// Biome identifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	EBiomeType BiomeType;

	// Display name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FText DisplayName;

	// Description for UI/lore
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FText Description;

	// Primary environmental hazard
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	EEnvironmentalHazard PrimaryHazard;

	// Secondary hazard (can be None)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	EEnvironmentalHazard SecondaryHazard;

	// Hazard intensity (0.0 - 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HazardIntensity;

	// Resources weighted for this biome
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Resources")
	TArray<FBiomeResourceWeight> ResourceWeights;

	// Visual characteristics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Visual")
	FBiomeVisualData VisualData;

	// Gameplay modifiers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Gameplay")
	FBiomeGameplayModifiers GameplayModifiers;

	// Base exploration score for this biome
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (ClampMin = "0"))
	int32 BaseExplorationScore;

	// Rarity weight (affects how often this biome appears)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RarityWeight;

	// Minimum temperature (for procedural variation)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Environment")
	float MinTemperature;

	// Maximum temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Environment")
	float MaxTemperature;

	// Gravity modifier (1.0 = Earth standard)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Environment", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float GravityModifier;

	FBiomeDefinition()
		: BiomeType(EBiomeType::None)
		, DisplayName(FText::FromString(TEXT("Unknown Biome")))
		, Description(FText::FromString(TEXT("An uncharted biome.")))
		, PrimaryHazard(EEnvironmentalHazard::None)
		, SecondaryHazard(EEnvironmentalHazard::None)
		, HazardIntensity(0.0f)
		, BaseExplorationScore(100)
		, RarityWeight(0.5f)
		, MinTemperature(-20.0f)
		, MaxTemperature(40.0f)
		, GravityModifier(1.0f)
	{
	}
};

/**
 * Biome transition data for blending between biomes
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FBiomeTransition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Transition")
	EBiomeType FromBiome;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Transition")
	EBiomeType ToBiome;

	// Transition width in world units
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Transition", meta = (ClampMin = "10.0"))
	float TransitionWidth;

	// Blend curve type (linear, smooth, sharp)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Transition", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float BlendExponent;

	FBiomeTransition()
		: FromBiome(EBiomeType::None)
		, ToBiome(EBiomeType::None)
		, TransitionWidth(100.0f)
		, BlendExponent(1.0f)
	{
	}
};

/**
 * UOdysseyBiomeDefinitionSystem
 * 
 * Manages biome definitions and provides biome-related utilities for
 * procedural planet generation. Handles biome selection, resource weighting,
 * and environmental characteristics.
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API UOdysseyBiomeDefinitionSystem : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyBiomeDefinitionSystem();

	// Initialization
	UFUNCTION(BlueprintCallable, Category = "Biome System")
	void Initialize(UDataTable* BiomeDataTable = nullptr);

	// Biome definition access
	UFUNCTION(BlueprintCallable, Category = "Biome System")
	FBiomeDefinition GetBiomeDefinition(EBiomeType BiomeType) const;

	UFUNCTION(BlueprintCallable, Category = "Biome System")
	TArray<FBiomeDefinition> GetAllBiomeDefinitions() const;

	UFUNCTION(BlueprintCallable, Category = "Biome System")
	bool HasBiomeDefinition(EBiomeType BiomeType) const;

	// Biome selection based on seed
	UFUNCTION(BlueprintCallable, Category = "Biome Generation")
	EBiomeType SelectBiomeFromSeed(int32 Seed, float TemperatureHint = 0.5f, float MoistureHint = 0.5f) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Generation")
	TArray<EBiomeType> GeneratePlanetBiomes(int32 PlanetSeed, int32 BiomeCount) const;

	// Resource weighting
	UFUNCTION(BlueprintCallable, Category = "Biome Resources")
	TArray<FBiomeResourceWeight> GetBiomeResources(EBiomeType BiomeType) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Resources")
	EResourceType SelectResourceFromBiome(EBiomeType BiomeType, int32 Seed) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Resources")
	float GetResourceQualityModifier(EBiomeType BiomeType, EResourceType ResourceType) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Resources")
	float GetResourceAbundanceModifier(EBiomeType BiomeType, EResourceType ResourceType) const;

	// Visual data
	UFUNCTION(BlueprintCallable, Category = "Biome Visual")
	FBiomeVisualData GetBiomeVisualData(EBiomeType BiomeType) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Visual")
	FBiomeVisualData BlendBiomeVisuals(EBiomeType BiomeA, EBiomeType BiomeB, float BlendFactor) const;

	// Gameplay modifiers
	UFUNCTION(BlueprintCallable, Category = "Biome Gameplay")
	FBiomeGameplayModifiers GetBiomeGameplayModifiers(EBiomeType BiomeType) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Gameplay")
	float GetEnvironmentalDamage(EBiomeType BiomeType) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Gameplay")
	EEnvironmentalHazard GetPrimaryHazard(EBiomeType BiomeType) const;

	// Biome transitions
	UFUNCTION(BlueprintCallable, Category = "Biome Transitions")
	FBiomeTransition GetTransitionData(EBiomeType FromBiome, EBiomeType ToBiome) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Transitions")
	bool AreBiomesCompatible(EBiomeType BiomeA, EBiomeType BiomeB) const;

	// Utility functions
	UFUNCTION(BlueprintCallable, Category = "Biome Utility")
	FText GetBiomeDisplayName(EBiomeType BiomeType) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Utility")
	FText GetBiomeDescription(EBiomeType BiomeType) const;

	UFUNCTION(BlueprintCallable, Category = "Biome Utility")
	int32 GetBiomeExplorationScore(EBiomeType BiomeType) const;

	// Seeded random helpers
	UFUNCTION(BlueprintCallable, Category = "Biome Utility")
	static float SeededRandom(int32 Seed);

	UFUNCTION(BlueprintCallable, Category = "Biome Utility")
	static int32 SeededRandomRange(int32 Seed, int32 Min, int32 Max);

protected:
	// Biome definitions storage
	UPROPERTY()
	TMap<EBiomeType, FBiomeDefinition> BiomeDefinitions;

	// Biome compatibility matrix (which biomes can be adjacent)
	UPROPERTY()
	TMap<EBiomeType, TArray<EBiomeType>> BiomeCompatibility;

	// Data table reference for external configuration
	UPROPERTY()
	UDataTable* BiomeDataTableRef;

private:
	// Initialize default biome definitions
	void InitializeDefaultBiomes();

	// Initialize biome compatibility rules
	void InitializeBiomeCompatibility();

	// Load biomes from data table if available
	void LoadBiomesFromDataTable();

	// Create a specific biome definition
	FBiomeDefinition CreateDesertBiome() const;
	FBiomeDefinition CreateIceBiome() const;
	FBiomeDefinition CreateForestBiome() const;
	FBiomeDefinition CreateVolcanicBiome() const;
	FBiomeDefinition CreateOceanBiome() const;
	FBiomeDefinition CreateCrystallineBiome() const;
	FBiomeDefinition CreateToxicBiome() const;
	FBiomeDefinition CreateBarrenBiome() const;
	FBiomeDefinition CreateLushBiome() const;
	FBiomeDefinition CreateRadioactiveBiome() const;
	FBiomeDefinition CreateMetallicBiome() const;
	FBiomeDefinition CreateAnomalousBiome() const;

	// Hash function for seeded randomness
	static uint32 HashSeed(int32 Seed);
};
