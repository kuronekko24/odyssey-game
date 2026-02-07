// EconomyRippleEffectTest.cpp
// Comprehensive automation tests for UEconomyRippleEffect
// Tests ripple creation, propagation, dampening, and lifecycle

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.h"
#include "Economy/EconomyRippleEffect.h"

// ============================================================================
// Helper: creates a multi-market network with ripple system
// ============================================================================
namespace RippleTestHelpers
{
	struct FTestContext
	{
		AActor* Actor = nullptr;
		UEconomyRippleEffect* RippleSystem = nullptr;
		UTradeRouteAnalyzer* Analyzer = nullptr;
		TMap<FName, UMarketDataComponent*> MarketDataMap;
		TMap<FName, UPriceFluctuationSystem*> PriceSystemMap;
		TArray<FMarketId> MarketIds;

		void Destroy()
		{
			if (Actor) Actor->Destroy();
		}
	};

	FTestContext CreateNetworkContext(UWorld* World, int32 NumMarkets = 3)
	{
		FTestContext Ctx;
		Ctx.Actor = World->SpawnActor<AActor>();
		if (!Ctx.Actor) return Ctx;

		// Create markets
		for (int32 i = 0; i < NumMarkets; ++i)
		{
			FMarketId Id(FName(*FString::Printf(TEXT("RippleMarket_%d"), i)), 1);
			FName Key(*Id.ToString());

			auto* MD = NewObject<UMarketDataComponent>(Ctx.Actor);
			MD->RegisterComponent();
			MD->InitializeMarketData(Id, Id.MarketName.ToString());
			MD->AddSupply(EResourceType::IronOre, 500);
			MD->AddSupply(EResourceType::CopperOre, 500);
			MD->RecalculateAllMetrics();

			auto* PS = NewObject<UPriceFluctuationSystem>(Ctx.Actor);
			PS->RegisterComponent();
			PS->Initialize(MD);
			PS->UpdateAllPrices();

			Ctx.MarketDataMap.Add(Key, MD);
			Ctx.PriceSystemMap.Add(Key, PS);
			Ctx.MarketIds.Add(Id);
		}

		// Create trade route analyzer and register markets
		Ctx.Analyzer = NewObject<UTradeRouteAnalyzer>(Ctx.Actor);
		Ctx.Analyzer->RegisterComponent();

		for (int32 i = 0; i < NumMarkets; ++i)
		{
			FName Key(*Ctx.MarketIds[i].ToString());
			Ctx.Analyzer->RegisterMarket(Ctx.MarketIds[i], Ctx.MarketDataMap[Key], Ctx.PriceSystemMap[Key]);
		}

		// Create linear chain: 0 <-> 1 <-> 2 <-> ...
		for (int32 i = 0; i < NumMarkets - 1; ++i)
		{
			Ctx.Analyzer->DefineTradeRoute(
				Ctx.MarketIds[i], Ctx.MarketIds[i + 1],
				1000.0f, 2.0f, ETradeRouteRisk::Low);
			Ctx.Analyzer->DefineTradeRoute(
				Ctx.MarketIds[i + 1], Ctx.MarketIds[i],
				1000.0f, 2.0f, ETradeRouteRisk::Low);
		}

		// Create ripple system
		Ctx.RippleSystem = NewObject<UEconomyRippleEffect>(Ctx.Actor);
		Ctx.RippleSystem->RegisterComponent();

		FEconomyConfiguration Config;
		Config.MaxActiveRipples = 10;
		Config.RippleMinMagnitudeThreshold = 0.01f;
		Config.RippleMaxPropagationDepth = 4;
		Config.RippleDefaultDampening = 0.3f;

		Ctx.RippleSystem->InitializeRippleSystem(Config);
		Ctx.RippleSystem->SetMarketReferences(&Ctx.MarketDataMap, &Ctx.PriceSystemMap, Ctx.Analyzer);

		return Ctx;
	}
}

// ============================================================================
// 1. RIPPLE CREATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_CreateSupplyShock,
	"Odyssey.Economy.RippleEffect.Create.SupplyShockRippleCreated",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_CreateSupplyShock::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::IronOre);

	int32 RippleId = Ctx.RippleSystem->CreateSupplyShockRipple(
		Ctx.MarketIds[0], Resources, -0.5f, -1);

	TestTrue(TEXT("Supply shock ripple should return valid ID"), RippleId >= 0);
	TestEqual(TEXT("Active ripple count should be 1"), Ctx.RippleSystem->GetActiveRippleCount(), 1);

	FEconomicRipple Ripple = Ctx.RippleSystem->GetRipple(RippleId);
	TestEqual(TEXT("Ripple type should be SupplyShock"), Ripple.RippleType, ERippleType::SupplyShock);
	TestTrue(TEXT("Ripple should be active"), Ripple.bIsActive);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_CreateDemandShock,
	"Odyssey.Economy.RippleEffect.Create.DemandShockRippleCreated",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_CreateDemandShock::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::CopperOre);

	int32 RippleId = Ctx.RippleSystem->CreateDemandShockRipple(
		Ctx.MarketIds[1], Resources, 0.8f);

	TestTrue(TEXT("Demand shock ripple should return valid ID"), RippleId >= 0);

	FEconomicRipple Ripple = Ctx.RippleSystem->GetRipple(RippleId);
	TestEqual(TEXT("Ripple type should be DemandShock"), Ripple.RippleType, ERippleType::DemandShock);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_CreatePriceShock,
	"Odyssey.Economy.RippleEffect.Create.PriceShockRippleCreated",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_CreatePriceShock::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::IronOre);

	int32 RippleId = Ctx.RippleSystem->CreatePriceShockRipple(
		Ctx.MarketIds[0], Resources, 0.6f);

	TestTrue(TEXT("Price shock ripple should return valid ID"), RippleId >= 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_CreateTradeDisruption,
	"Odyssey.Economy.RippleEffect.Create.TradeDisruptionRippleCreated",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_CreateTradeDisruption::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	int32 RippleId = Ctx.RippleSystem->CreateTradeDisruptionRipple(Ctx.MarketIds[1], 0.7f);
	TestTrue(TEXT("Trade disruption ripple should return valid ID"), RippleId >= 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_CreateCombatZone,
	"Odyssey.Economy.RippleEffect.Create.CombatZoneRippleCreated",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_CreateCombatZone::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	int32 RippleId = Ctx.RippleSystem->CreateCombatZoneRipple(Ctx.MarketIds[0], 0.9f);
	TestTrue(TEXT("Combat zone ripple should return valid ID"), RippleId >= 0);

	FEconomicRipple Ripple = Ctx.RippleSystem->GetRipple(RippleId);
	TestEqual(TEXT("Ripple type should be CombatZone"), Ripple.RippleType, ERippleType::CombatZone);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_CreateCraftingDemand,
	"Odyssey.Economy.RippleEffect.Create.CraftingDemandRippleCreated",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_CreateCraftingDemand::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	TArray<EResourceType> Ingredients;
	Ingredients.Add(EResourceType::IronOre);
	Ingredients.Add(EResourceType::CopperOre);

	int32 RippleId = Ctx.RippleSystem->CreateCraftingDemandRipple(
		Ctx.MarketIds[2], Ingredients, 0.5f);

	TestTrue(TEXT("Crafting demand ripple should return valid ID"), RippleId >= 0);

	FEconomicRipple Ripple = Ctx.RippleSystem->GetRipple(RippleId);
	TestEqual(TEXT("Ripple type should be CraftingDemand"), Ripple.RippleType, ERippleType::CraftingDemand);
	TestTrue(TEXT("Should affect at least 2 resources"), Ripple.AffectedResources.Num() >= 2);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 2. RIPPLE PROPAGATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_PropagationDampening,
	"Odyssey.Economy.RippleEffect.Propagation.MagnitudeDecreasesByDampening",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_PropagationDampening::RunTest(const FString& Parameters)
{
	// Test the FEconomicRipple struct's dampening calculation directly
	FEconomicRipple Ripple;
	Ripple.BaseMagnitude = 1.0f;
	Ripple.DampeningFactor = 0.3f; // 30% lost per hop
	Ripple.CurrentDepth = 0;

	float Mag0 = Ripple.GetCurrentMagnitude();
	TestTrue(TEXT("Magnitude at depth 0 should be 1.0"), FMath::IsNearlyEqual(Mag0, 1.0f, 0.01f));

	Ripple.CurrentDepth = 1;
	float Mag1 = Ripple.GetCurrentMagnitude();
	TestTrue(TEXT("Magnitude at depth 1 should be 0.7"), FMath::IsNearlyEqual(Mag1, 0.7f, 0.01f));

	Ripple.CurrentDepth = 2;
	float Mag2 = Ripple.GetCurrentMagnitude();
	TestTrue(TEXT("Magnitude at depth 2 should be 0.49"), FMath::IsNearlyEqual(Mag2, 0.49f, 0.01f));

	Ripple.CurrentDepth = 3;
	float Mag3 = Ripple.GetCurrentMagnitude();
	TestTrue(TEXT("Magnitude at depth 3 should be 0.343"), FMath::IsNearlyEqual(Mag3, 0.343f, 0.01f));

	// Each step should be less than the previous
	TestTrue(TEXT("Magnitude should decrease with each hop"), Mag0 > Mag1 && Mag1 > Mag2 && Mag2 > Mag3);

	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_DissipationCheck,
	"Odyssey.Economy.RippleEffect.Propagation.RippleDissipatesAtThreshold",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_DissipationCheck::RunTest(const FString& Parameters)
{
	FEconomicRipple Ripple;
	Ripple.BaseMagnitude = 0.5f;
	Ripple.DampeningFactor = 0.5f; // 50% lost per hop
	Ripple.MaxDepth = 4;

	Ripple.CurrentDepth = 0;
	TestFalse(TEXT("Should not be dissipated at depth 0"), Ripple.HasDissipated(0.01f));

	Ripple.CurrentDepth = 3;
	// Magnitude at depth 3: 0.5 * 0.5^3 = 0.0625
	TestFalse(TEXT("Should not be dissipated at depth 3 (mag=0.0625)"), Ripple.HasDissipated(0.01f));

	Ripple.CurrentDepth = 4;
	// At MaxDepth, should dissipate regardless
	TestTrue(TEXT("Should be dissipated at MaxDepth"), Ripple.HasDissipated(0.01f));

	// Very low magnitude ripple
	FEconomicRipple WeakRipple;
	WeakRipple.BaseMagnitude = 0.001f;
	WeakRipple.DampeningFactor = 0.5f;
	WeakRipple.MaxDepth = 10;
	WeakRipple.CurrentDepth = 0;
	TestTrue(TEXT("Very weak ripple should dissipate at threshold 0.01"),
		WeakRipple.HasDissipated(0.01f));

	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_MultipleActiveRipples,
	"Odyssey.Economy.RippleEffect.Propagation.MultipleRipplesCoexist",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_MultipleActiveRipples::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World, 4);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::IronOre);

	int32 Ripple1 = Ctx.RippleSystem->CreateSupplyShockRipple(Ctx.MarketIds[0], Resources, -0.5f);
	int32 Ripple2 = Ctx.RippleSystem->CreateDemandShockRipple(Ctx.MarketIds[2], Resources, 0.8f);
	int32 Ripple3 = Ctx.RippleSystem->CreatePriceShockRipple(Ctx.MarketIds[3], Resources, 0.3f);

	TestEqual(TEXT("Should have 3 active ripples"), Ctx.RippleSystem->GetActiveRippleCount(), 3);

	TArray<FEconomicRipple> ActiveRipples = Ctx.RippleSystem->GetActiveRipples();
	TestEqual(TEXT("GetActiveRipples should return 3"), ActiveRipples.Num(), 3);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 3. RIPPLE LIFECYCLE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_CancelRipple,
	"Odyssey.Economy.RippleEffect.Lifecycle.CancelRippleRemovesFromActive",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_CancelRipple::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::IronOre);

	int32 RippleId = Ctx.RippleSystem->CreateSupplyShockRipple(Ctx.MarketIds[0], Resources, -0.5f);
	TestEqual(TEXT("Should have 1 active ripple"), Ctx.RippleSystem->GetActiveRippleCount(), 1);

	bool bCancelled = Ctx.RippleSystem->CancelRipple(RippleId);
	TestTrue(TEXT("CancelRipple should succeed"), bCancelled);
	TestEqual(TEXT("Should have 0 active ripples after cancel"), Ctx.RippleSystem->GetActiveRippleCount(), 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_CancelAllRipples,
	"Odyssey.Economy.RippleEffect.Lifecycle.CancelAllRipplesClearsAllActive",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_CancelAllRipples::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::IronOre);

	Ctx.RippleSystem->CreateSupplyShockRipple(Ctx.MarketIds[0], Resources, -0.5f);
	Ctx.RippleSystem->CreateDemandShockRipple(Ctx.MarketIds[1], Resources, 0.3f);
	Ctx.RippleSystem->CreatePriceShockRipple(Ctx.MarketIds[2], Resources, 0.7f);

	TestEqual(TEXT("Should have 3 active ripples"), Ctx.RippleSystem->GetActiveRippleCount(), 3);

	Ctx.RippleSystem->CancelAllRipples();
	TestEqual(TEXT("Should have 0 active ripples after cancel all"), Ctx.RippleSystem->GetActiveRippleCount(), 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_CancelInvalidId,
	"Odyssey.Economy.RippleEffect.Lifecycle.CancelInvalidIdReturnsFalse",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_CancelInvalidId::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	bool bCancelled = Ctx.RippleSystem->CancelRipple(99999);
	TestFalse(TEXT("Cancelling non-existent ripple should return false"), bCancelled);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 4. RIPPLE DATA INTEGRITY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_OriginMarketTracked,
	"Odyssey.Economy.RippleEffect.Data.OriginMarketIsTracked",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_OriginMarketTracked::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::IronOre);

	int32 RippleId = Ctx.RippleSystem->CreateSupplyShockRipple(Ctx.MarketIds[1], Resources, -0.3f, 42);

	FEconomicRipple Ripple = Ctx.RippleSystem->GetRipple(RippleId);
	TestEqual(TEXT("Origin market should match"), Ripple.OriginMarket, Ctx.MarketIds[1]);
	TestEqual(TEXT("Source event ID should match"), Ripple.SourceEventId, 42);
	TestTrue(TEXT("Visited markets should contain origin"), Ripple.VisitedMarkets.Contains(Ctx.MarketIds[1]));

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_GenericRippleCreation,
	"Odyssey.Economy.RippleEffect.Data.GenericRippleTemplateCreation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_GenericRippleCreation::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	FEconomicRipple Template;
	Template.RippleType = ERippleType::PriceShock;
	Template.OriginMarket = Ctx.MarketIds[0];
	Template.AffectedResources.Add(EResourceType::GoldOre);
	Template.BaseMagnitude = 0.75f;
	Template.DampeningFactor = 0.2f;
	Template.MaxDepth = 6;
	Template.PropagationSpeed = 2.0f;
	Template.bIsActive = true;

	int32 RippleId = Ctx.RippleSystem->CreateRipple(Template);
	TestTrue(TEXT("Generic ripple should return valid ID"), RippleId >= 0);

	FEconomicRipple Stored = Ctx.RippleSystem->GetRipple(RippleId);
	TestEqual(TEXT("Ripple type should match template"), Stored.RippleType, ERippleType::PriceShock);
	TestTrue(TEXT("Base magnitude should match"), FMath::IsNearlyEqual(Stored.BaseMagnitude, 0.75f, 0.01f));
	TestTrue(TEXT("Dampening factor should match"), FMath::IsNearlyEqual(Stored.DampeningFactor, 0.2f, 0.01f));
	TestEqual(TEXT("Max depth should match"), Stored.MaxDepth, 6);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 5. CONFIGURATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRipple_ConfigurationUpdate,
	"Odyssey.Economy.RippleEffect.Config.ConfigurationCanBeUpdated",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRipple_ConfigurationUpdate::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = RippleTestHelpers::CreateNetworkContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	FEconomyConfiguration NewConfig;
	NewConfig.MaxActiveRipples = 20;
	NewConfig.RippleMinMagnitudeThreshold = 0.05f;
	NewConfig.RippleMaxPropagationDepth = 8;
	NewConfig.RippleDefaultDampening = 0.5f;

	// Should not crash
	Ctx.RippleSystem->SetConfiguration(NewConfig);

	// Verify the system still works after reconfiguration
	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::IronOre);
	int32 RippleId = Ctx.RippleSystem->CreateSupplyShockRipple(Ctx.MarketIds[0], Resources, 0.5f);
	TestTrue(TEXT("System should still create ripples after config update"), RippleId >= 0);

	Ctx.Destroy();
	return true;
}
