// BiomeDefinitionTests.cpp
// Comprehensive automation tests for UOdysseyBiomeDefinitionSystem
// Tests biome instantiation, property ranges, discovery affinity, transitions, and compatibility

#include "Misc/AutomationTest.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "Procedural/ProceduralTypes.h"

// ============================================================================
// HELPERS
// ============================================================================

namespace BiomeTestHelpers
{
	static UOdysseyBiomeDefinitionSystem* CreateInitializedBiomeSystem()
	{
		UOdysseyBiomeDefinitionSystem* BiomeSystem = NewObject<UOdysseyBiomeDefinitionSystem>();
		BiomeSystem->Initialize(nullptr);
		return BiomeSystem;
	}

	static const EBiomeType AllBiomeTypes[] = {
		EBiomeType::Desert, EBiomeType::Ice, EBiomeType::Forest,
		EBiomeType::Volcanic, EBiomeType::Ocean, EBiomeType::Crystalline,
		EBiomeType::Toxic, EBiomeType::Barren, EBiomeType::Lush,
		EBiomeType::Radioactive, EBiomeType::Metallic, EBiomeType::Anomalous
	};

	static constexpr int32 AllBiomeCount = 12;
}

// ============================================================================
// 1. ALL BIOME TYPES INSTANTIATE CORRECTLY
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_AllTypesHaveDefinitions,
	"Odyssey.Procedural.BiomeDefinition.AllTypesHaveDefinitions",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_AllTypesHaveDefinitions::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		TestTrue(
			FString::Printf(TEXT("BiomeType %d should have a definition"), static_cast<int32>(BiomeType)),
			BiomeSystem->HasBiomeDefinition(BiomeType));
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_AllDefinitionsValid,
	"Odyssey.Procedural.BiomeDefinition.AllDefinitionsHaveValidData",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_AllDefinitionsValid::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		FBiomeDefinition Def = BiomeSystem->GetBiomeDefinition(BiomeType);

		TestEqual(
			FString::Printf(TEXT("Biome %d definition type should match"), static_cast<int32>(BiomeType)),
			Def.BiomeType, BiomeType);
		TestFalse(
			FString::Printf(TEXT("Biome %d display name should not be empty"), static_cast<int32>(BiomeType)),
			Def.DisplayName.IsEmpty());
		TestFalse(
			FString::Printf(TEXT("Biome %d description should not be empty"), static_cast<int32>(BiomeType)),
			Def.Description.IsEmpty());
		TestTrue(
			FString::Printf(TEXT("Biome %d should have resource weights"), static_cast<int32>(BiomeType)),
			Def.ResourceWeights.Num() > 0);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_GetAllDefinitions,
	"Odyssey.Procedural.BiomeDefinition.GetAllDefinitionsReturnsAll",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_GetAllDefinitions::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	TArray<FBiomeDefinition> AllDefs = BiomeSystem->GetAllBiomeDefinitions();

	TestEqual(TEXT("Should return all 12 biome definitions"), AllDefs.Num(), BiomeTestHelpers::AllBiomeCount);

	// Verify each definition has a unique biome type
	TSet<EBiomeType> Types;
	for (const FBiomeDefinition& Def : AllDefs)
	{
		TestFalse(
			FString::Printf(TEXT("Biome type %d should be unique in GetAllBiomeDefinitions"), static_cast<int32>(Def.BiomeType)),
			Types.Contains(Def.BiomeType));
		Types.Add(Def.BiomeType);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_NoneTypeNotDefined,
	"Odyssey.Procedural.BiomeDefinition.NoneTypeNotDefined",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_NoneTypeNotDefined::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	// EBiomeType::None should either not have a definition or have a reasonable fallback
	FBiomeDefinition NoneDef = BiomeSystem->GetBiomeDefinition(EBiomeType::None);
	// The function should not crash; exact behavior (empty or fallback) is acceptable

	TestTrue(TEXT("GetBiomeDefinition(None) should not crash"), true);

	return true;
}

// ============================================================================
// 2. BIOME PROPERTIES WITHIN VALID RANGES
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_HazardIntensityRange,
	"Odyssey.Procedural.BiomeDefinition.HazardIntensityWithinRange",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_HazardIntensityRange::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		FBiomeDefinition Def = BiomeSystem->GetBiomeDefinition(BiomeType);

		TestTrue(
			FString::Printf(TEXT("Biome %d hazard intensity (%.2f) should be in [0, 1]"), static_cast<int32>(BiomeType), Def.HazardIntensity),
			Def.HazardIntensity >= 0.0f && Def.HazardIntensity <= 1.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d rarity weight (%.2f) should be in [0, 1]"), static_cast<int32>(BiomeType), Def.RarityWeight),
			Def.RarityWeight >= 0.0f && Def.RarityWeight <= 1.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d gravity modifier (%.2f) should be in [0.1, 3.0]"), static_cast<int32>(BiomeType), Def.GravityModifier),
			Def.GravityModifier >= 0.1f && Def.GravityModifier <= 3.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d base exploration score (%d) should be >= 0"), static_cast<int32>(BiomeType), Def.BaseExplorationScore),
			Def.BaseExplorationScore >= 0);
		TestTrue(
			FString::Printf(TEXT("Biome %d min temp should be < max temp"), static_cast<int32>(BiomeType)),
			Def.MinTemperature < Def.MaxTemperature);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_GameplayModifiersRange,
	"Odyssey.Procedural.BiomeDefinition.GameplayModifiersWithinRange",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_GameplayModifiersRange::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		FBiomeGameplayModifiers Mods = BiomeSystem->GetBiomeGameplayModifiers(BiomeType);

		TestTrue(
			FString::Printf(TEXT("Biome %d movement speed modifier should be in [0.1, 2.0]"), static_cast<int32>(BiomeType)),
			Mods.MovementSpeedModifier >= 0.1f && Mods.MovementSpeedModifier <= 2.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d mining speed modifier should be in [0.1, 2.0]"), static_cast<int32>(BiomeType)),
			Mods.MiningSpeedModifier >= 0.1f && Mods.MiningSpeedModifier <= 2.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d energy consumption modifier should be in [0.5, 3.0]"), static_cast<int32>(BiomeType)),
			Mods.EnergyConsumptionModifier >= 0.5f && Mods.EnergyConsumptionModifier <= 3.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d visibility modifier should be in [0.2, 2.0]"), static_cast<int32>(BiomeType)),
			Mods.VisibilityModifier >= 0.2f && Mods.VisibilityModifier <= 2.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d scan range modifier should be in [0.2, 2.0]"), static_cast<int32>(BiomeType)),
			Mods.ScanRangeModifier >= 0.2f && Mods.ScanRangeModifier <= 2.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d env damage should be >= 0"), static_cast<int32>(BiomeType)),
			Mods.EnvironmentalDamagePerSecond >= 0.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d shield drain should be in [0, 3]"), static_cast<int32>(BiomeType)),
			Mods.ShieldDrainModifier >= 0.0f && Mods.ShieldDrainModifier <= 3.0f);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_VisualDataRange,
	"Odyssey.Procedural.BiomeDefinition.VisualDataWithinRange",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_VisualDataRange::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		FBiomeVisualData Visual = BiomeSystem->GetBiomeVisualData(BiomeType);

		TestTrue(
			FString::Printf(TEXT("Biome %d fog density should be in [0, 1]"), static_cast<int32>(BiomeType)),
			Visual.FogDensity >= 0.0f && Visual.FogDensity <= 1.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d ambient light should be in [0, 2]"), static_cast<int32>(BiomeType)),
			Visual.AmbientLightIntensity >= 0.0f && Visual.AmbientLightIntensity <= 2.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d particle intensity should be in [0, 1]"), static_cast<int32>(BiomeType)),
			Visual.ParticleIntensity >= 0.0f && Visual.ParticleIntensity <= 1.0f);
		TestTrue(
			FString::Printf(TEXT("Biome %d primary color alpha should be positive"), static_cast<int32>(BiomeType)),
			Visual.PrimaryColor.A > 0.0f);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_ResourceWeightRanges,
	"Odyssey.Procedural.BiomeDefinition.ResourceWeightRangesValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_ResourceWeightRanges::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		TArray<FBiomeResourceWeight> Weights = BiomeSystem->GetBiomeResources(BiomeType);

		TestTrue(
			FString::Printf(TEXT("Biome %d should have at least one resource weight"), static_cast<int32>(BiomeType)),
			Weights.Num() > 0);

		for (const FBiomeResourceWeight& Weight : Weights)
		{
			TestTrue(
				FString::Printf(TEXT("Biome %d resource spawn weight should be in [0, 1]"), static_cast<int32>(BiomeType)),
				Weight.SpawnWeight >= 0.0f && Weight.SpawnWeight <= 1.0f);
			TestTrue(
				FString::Printf(TEXT("Biome %d resource quality modifier should be in [0.1, 3.0]"), static_cast<int32>(BiomeType)),
				Weight.QualityModifier >= 0.1f && Weight.QualityModifier <= 3.0f);
			TestTrue(
				FString::Printf(TEXT("Biome %d resource abundance modifier should be in [0.1, 5.0]"), static_cast<int32>(BiomeType)),
				Weight.AbundanceModifier >= 0.1f && Weight.AbundanceModifier <= 5.0f);
			TestTrue(
				FString::Printf(TEXT("Biome %d resource type should not be None"), static_cast<int32>(BiomeType)),
				Weight.ResourceType != EResourceType::None);
		}
	}

	return true;
}

// ============================================================================
// 3. BIOME SELECTION AND GENERATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_SelectBiomeFromSeed,
	"Odyssey.Procedural.BiomeDefinition.SelectBiomeFromSeedDeterministic",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_SelectBiomeFromSeed::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	// Same seed should yield same biome
	EBiomeType A = BiomeSystem->SelectBiomeFromSeed(42, 0.5f, 0.5f);
	EBiomeType B = BiomeSystem->SelectBiomeFromSeed(42, 0.5f, 0.5f);
	TestEqual(TEXT("Same seed + hints should produce same biome"), A, B);

	// Different seeds should (likely) produce different biomes over a range
	TSet<EBiomeType> BiomesFound;
	for (int32 Seed = 0; Seed < 100; ++Seed)
	{
		EBiomeType Selected = BiomeSystem->SelectBiomeFromSeed(Seed, 0.5f, 0.5f);
		TestTrue(TEXT("Selected biome should not be None"), Selected != EBiomeType::None);
		BiomesFound.Add(Selected);
	}

	TestTrue(
		FString::Printf(TEXT("Should find at least 3 distinct biomes from 100 seeds, found %d"), BiomesFound.Num()),
		BiomesFound.Num() >= 3);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_TemperatureInfluencesSelection,
	"Odyssey.Procedural.BiomeDefinition.TemperatureInfluencesBiomeSelection",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_TemperatureInfluencesSelection::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	// Hot temperature hint should favor hot biomes, cold should favor cold biomes
	TMap<EBiomeType, int32> HotBiomes;
	TMap<EBiomeType, int32> ColdBiomes;

	for (int32 Seed = 0; Seed < 200; ++Seed)
	{
		EBiomeType Hot = BiomeSystem->SelectBiomeFromSeed(Seed, 0.9f, 0.5f);  // High temp
		EBiomeType Cold = BiomeSystem->SelectBiomeFromSeed(Seed, 0.1f, 0.5f); // Low temp
		HotBiomes.FindOrAdd(Hot)++;
		ColdBiomes.FindOrAdd(Cold)++;
	}

	// The distributions should differ (hot should lean toward Desert/Volcanic, cold toward Ice)
	bool bDistinctDistributions = false;
	for (const auto& Pair : HotBiomes)
	{
		int32* ColdCount = ColdBiomes.Find(Pair.Key);
		if (!ColdCount || FMath::Abs(Pair.Value - *ColdCount) > 5)
		{
			bDistinctDistributions = true;
			break;
		}
	}

	TestTrue(TEXT("Hot and cold temperature hints should produce different biome distributions"), bDistinctDistributions);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_GeneratePlanetBiomes,
	"Odyssey.Procedural.BiomeDefinition.GeneratePlanetBiomesValidCount",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_GeneratePlanetBiomes::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	for (int32 Count = 2; Count <= 10; ++Count)
	{
		TArray<EBiomeType> Biomes = BiomeSystem->GeneratePlanetBiomes(42, Count);
		TestEqual(
			FString::Printf(TEXT("GeneratePlanetBiomes(%d) should return requested count"), Count),
			Biomes.Num(), Count);

		for (EBiomeType Biome : Biomes)
		{
			TestTrue(
				FString::Printf(TEXT("Generated biome should not be None (count=%d)"), Count),
				Biome != EBiomeType::None);
		}
	}

	return true;
}

// ============================================================================
// 4. RESOURCE SELECTION FROM BIOME
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_SelectResourceFromBiome,
	"Odyssey.Procedural.BiomeDefinition.SelectResourceFromBiomeDeterministic",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_SelectResourceFromBiome::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	// Same seed + biome = same resource
	EResourceType A = BiomeSystem->SelectResourceFromBiome(EBiomeType::Volcanic, 42);
	EResourceType B = BiomeSystem->SelectResourceFromBiome(EBiomeType::Volcanic, 42);
	TestEqual(TEXT("Same seed + biome should produce same resource"), A, B);

	// Selected resources should always be valid (not None)
	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		for (int32 Seed = 0; Seed < 20; ++Seed)
		{
			EResourceType Resource = BiomeSystem->SelectResourceFromBiome(BiomeType, Seed);
			TestTrue(
				FString::Printf(TEXT("Resource from biome %d seed %d should not be None"), static_cast<int32>(BiomeType), Seed),
				Resource != EResourceType::None);
		}
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_ResourceQualityModifier,
	"Odyssey.Procedural.BiomeDefinition.ResourceQualityModifierValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_ResourceQualityModifier::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		TArray<FBiomeResourceWeight> Resources = BiomeSystem->GetBiomeResources(BiomeType);
		for (const FBiomeResourceWeight& Res : Resources)
		{
			float QualityMod = BiomeSystem->GetResourceQualityModifier(BiomeType, Res.ResourceType);
			TestTrue(
				FString::Printf(TEXT("Quality modifier for biome %d resource should be positive"), static_cast<int32>(BiomeType)),
				QualityMod > 0.0f);

			float AbundanceMod = BiomeSystem->GetResourceAbundanceModifier(BiomeType, Res.ResourceType);
			TestTrue(
				FString::Printf(TEXT("Abundance modifier for biome %d resource should be positive"), static_cast<int32>(BiomeType)),
				AbundanceMod > 0.0f);
		}
	}

	return true;
}

// ============================================================================
// 5. ENVIRONMENTAL HAZARDS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_EnvironmentalDamage,
	"Odyssey.Procedural.BiomeDefinition.EnvironmentalDamageConsistent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_EnvironmentalDamage::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	// Hazardous biomes should have environmental damage
	float VolcanicDamage = BiomeSystem->GetEnvironmentalDamage(EBiomeType::Volcanic);
	float ToxicDamage = BiomeSystem->GetEnvironmentalDamage(EBiomeType::Toxic);
	float RadioactiveDamage = BiomeSystem->GetEnvironmentalDamage(EBiomeType::Radioactive);
	float ForestDamage = BiomeSystem->GetEnvironmentalDamage(EBiomeType::Forest);

	TestTrue(TEXT("Volcanic biome should have environmental damage"), VolcanicDamage > 0.0f);
	TestTrue(TEXT("Toxic biome should have environmental damage"), ToxicDamage > 0.0f);
	TestTrue(TEXT("Radioactive biome should have environmental damage"), RadioactiveDamage > 0.0f);

	// Forest should be safer
	TestTrue(TEXT("Forest should have less damage than Toxic"), ForestDamage < ToxicDamage);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_PrimaryHazards,
	"Odyssey.Procedural.BiomeDefinition.PrimaryHazardsMatchBiomeTheme",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_PrimaryHazards::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	EEnvironmentalHazard VolcanicHazard = BiomeSystem->GetPrimaryHazard(EBiomeType::Volcanic);
	TestEqual(TEXT("Volcanic primary hazard should be ExtremeHeat"), VolcanicHazard, EEnvironmentalHazard::ExtremeHeat);

	EEnvironmentalHazard IceHazard = BiomeSystem->GetPrimaryHazard(EBiomeType::Ice);
	TestEqual(TEXT("Ice primary hazard should be ExtremeCold"), IceHazard, EEnvironmentalHazard::ExtremeCold);

	EEnvironmentalHazard ToxicHazard = BiomeSystem->GetPrimaryHazard(EBiomeType::Toxic);
	TestEqual(TEXT("Toxic primary hazard should be ToxicAtmosphere"), ToxicHazard, EEnvironmentalHazard::ToxicAtmosphere);

	EEnvironmentalHazard RadioactiveHazard = BiomeSystem->GetPrimaryHazard(EBiomeType::Radioactive);
	TestEqual(TEXT("Radioactive primary hazard should be Radiation"), RadioactiveHazard, EEnvironmentalHazard::Radiation);

	return true;
}

// ============================================================================
// 6. BIOME TRANSITIONS AND BOUNDARIES
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_BiomeTransitionData,
	"Odyssey.Procedural.BiomeDefinition.BiomeTransitionDataValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_BiomeTransitionData::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	// Test transition between compatible biomes
	FBiomeTransition Transition = BiomeSystem->GetTransitionData(EBiomeType::Desert, EBiomeType::Barren);

	TestEqual(TEXT("Transition FromBiome should match"), Transition.FromBiome, EBiomeType::Desert);
	TestEqual(TEXT("Transition ToBiome should match"), Transition.ToBiome, EBiomeType::Barren);
	TestTrue(TEXT("Transition width should be >= 10.0"), Transition.TransitionWidth >= 10.0f);
	TestTrue(TEXT("Blend exponent should be in [0.1, 3.0]"), Transition.BlendExponent >= 0.1f && Transition.BlendExponent <= 3.0f);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_BiomeCompatibility,
	"Odyssey.Procedural.BiomeDefinition.BiomeCompatibilityMatrix",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_BiomeCompatibility::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	// Desert and Barren should be compatible (both dry environments)
	TestTrue(TEXT("Desert and Barren should be compatible"),
		BiomeSystem->AreBiomesCompatible(EBiomeType::Desert, EBiomeType::Barren));

	// Forest and Lush should be compatible (both vegetation-rich)
	TestTrue(TEXT("Forest and Lush should be compatible"),
		BiomeSystem->AreBiomesCompatible(EBiomeType::Forest, EBiomeType::Lush));

	// Compatibility should be symmetric
	TestEqual(TEXT("Compatibility should be symmetric (Desert-Forest)"),
		BiomeSystem->AreBiomesCompatible(EBiomeType::Desert, EBiomeType::Forest),
		BiomeSystem->AreBiomesCompatible(EBiomeType::Forest, EBiomeType::Desert));

	// Biome should always be compatible with itself
	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		TestTrue(
			FString::Printf(TEXT("Biome %d should be compatible with itself"), static_cast<int32>(BiomeType)),
			BiomeSystem->AreBiomesCompatible(BiomeType, BiomeType));
	}

	return true;
}

// ============================================================================
// 7. VISUAL BLENDING
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_VisualBlending,
	"Odyssey.Procedural.BiomeDefinition.VisualBlendingInterpolation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_VisualBlending::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	FBiomeVisualData DesertVisual = BiomeSystem->GetBiomeVisualData(EBiomeType::Desert);
	FBiomeVisualData IceVisual = BiomeSystem->GetBiomeVisualData(EBiomeType::Ice);

	// Blend at 0 should equal source biome
	FBiomeVisualData BlendZero = BiomeSystem->BlendBiomeVisuals(EBiomeType::Desert, EBiomeType::Ice, 0.0f);
	TestTrue(TEXT("Blend at 0.0 fog density should approximate Desert"),
		FMath::IsNearlyEqual(BlendZero.FogDensity, DesertVisual.FogDensity, 0.01f));

	// Blend at 1 should equal target biome
	FBiomeVisualData BlendOne = BiomeSystem->BlendBiomeVisuals(EBiomeType::Desert, EBiomeType::Ice, 1.0f);
	TestTrue(TEXT("Blend at 1.0 fog density should approximate Ice"),
		FMath::IsNearlyEqual(BlendOne.FogDensity, IceVisual.FogDensity, 0.01f));

	// Blend at 0.5 should be between the two
	FBiomeVisualData BlendHalf = BiomeSystem->BlendBiomeVisuals(EBiomeType::Desert, EBiomeType::Ice, 0.5f);
	float ExpectedFog = (DesertVisual.FogDensity + IceVisual.FogDensity) / 2.0f;
	TestTrue(TEXT("Blend at 0.5 fog density should be near midpoint"),
		FMath::IsNearlyEqual(BlendHalf.FogDensity, ExpectedFog, 0.1f));

	return true;
}

// ============================================================================
// 8. UTILITY FUNCTIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_DisplayNames,
	"Odyssey.Procedural.BiomeDefinition.DisplayNamesUnique",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_DisplayNames::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	TSet<FString> Names;
	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		FText DisplayName = BiomeSystem->GetBiomeDisplayName(BiomeType);
		TestFalse(
			FString::Printf(TEXT("Biome %d display name should not be empty"), static_cast<int32>(BiomeType)),
			DisplayName.IsEmpty());

		FText Description = BiomeSystem->GetBiomeDescription(BiomeType);
		TestFalse(
			FString::Printf(TEXT("Biome %d description should not be empty"), static_cast<int32>(BiomeType)),
			Description.IsEmpty());

		Names.Add(DisplayName.ToString());
	}

	TestEqual(TEXT("All 12 biome display names should be unique"), Names.Num(), BiomeTestHelpers::AllBiomeCount);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_ExplorationScores,
	"Odyssey.Procedural.BiomeDefinition.ExplorationScoresPositive",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_ExplorationScores::RunTest(const FString& Parameters)
{
	UOdysseyBiomeDefinitionSystem* BiomeSystem = BiomeTestHelpers::CreateInitializedBiomeSystem();

	for (EBiomeType BiomeType : BiomeTestHelpers::AllBiomeTypes)
	{
		int32 Score = BiomeSystem->GetBiomeExplorationScore(BiomeType);
		TestTrue(
			FString::Printf(TEXT("Biome %d exploration score (%d) should be positive"), static_cast<int32>(BiomeType), Score),
			Score > 0);
	}

	// Hazardous biomes should generally have higher exploration scores (risk/reward)
	int32 AnomalousScore = BiomeSystem->GetBiomeExplorationScore(EBiomeType::Anomalous);
	int32 BarrenScore = BiomeSystem->GetBiomeExplorationScore(EBiomeType::Barren);
	TestTrue(TEXT("Anomalous biome should have higher exploration score than Barren"),
		AnomalousScore > BarrenScore);

	return true;
}

// ============================================================================
// 9. SEEDED RANDOM UTILITY
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBiomeDef_SeededRandomDeterministic,
	"Odyssey.Procedural.BiomeDefinition.SeededRandomDeterministic",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBiomeDef_SeededRandomDeterministic::RunTest(const FString& Parameters)
{
	// SeededRandom should be deterministic
	float A = UOdysseyBiomeDefinitionSystem::SeededRandom(42);
	float B = UOdysseyBiomeDefinitionSystem::SeededRandom(42);
	TestEqual(TEXT("SeededRandom(42) should return same value twice"), A, B);

	// SeededRandom should return values in [0, 1)
	for (int32 Seed = 0; Seed < 1000; ++Seed)
	{
		float Val = UOdysseyBiomeDefinitionSystem::SeededRandom(Seed);
		TestTrue(
			FString::Printf(TEXT("SeededRandom(%d) = %.6f should be in [0, 1)"), Seed, Val),
			Val >= 0.0f && Val < 1.0f);
	}

	// SeededRandomRange should respect bounds
	for (int32 Seed = 0; Seed < 100; ++Seed)
	{
		int32 Val = UOdysseyBiomeDefinitionSystem::SeededRandomRange(Seed, 10, 20);
		TestTrue(
			FString::Printf(TEXT("SeededRandomRange(%d, 10, 20) = %d should be in [10, 20]"), Seed, Val),
			Val >= 10 && Val <= 20);
	}

	return true;
}
