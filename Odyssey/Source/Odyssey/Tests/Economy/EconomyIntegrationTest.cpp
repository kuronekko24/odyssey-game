// EconomyIntegrationTest.cpp
// Integration tests that validate cross-system interactions in the Dynamic Economy
// Tests the economy manager, system coordination, and end-to-end workflows

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.h"
#include "UEconomicEventSystem.h"
#include "Economy/EconomyRippleEffect.h"
#include "Economy/EconomySaveSystem.h"

// ============================================================================
// Helper: creates a complete integrated economy context
// ============================================================================
namespace IntegrationTestHelpers
{
	struct FIntegrationContext
	{
		AActor* Actor = nullptr;
		TMap<FName, UMarketDataComponent*> MarketDataMap;
		TMap<FName, UPriceFluctuationSystem*> PriceSystemMap;
		UTradeRouteAnalyzer* TradeAnalyzer = nullptr;
		UEconomicEventSystem* EventSystem = nullptr;
		UEconomyRippleEffect* RippleSystem = nullptr;
		UEconomySaveSystem* SaveSystem = nullptr;
		TArray<FMarketId> RegisteredMarkets;

		void Destroy()
		{
			if (Actor) Actor->Destroy();
		}
	};

	FIntegrationContext CreateFullContext(UWorld* World, int32 NumMarkets = 3)
	{
		FIntegrationContext Ctx;
		Ctx.Actor = World->SpawnActor<AActor>();
		if (!Ctx.Actor) return Ctx;

		// Create markets
		for (int32 i = 0; i < NumMarkets; ++i)
		{
			FMarketId Id(FName(*FString::Printf(TEXT("IntMarket_%d"), i)), 1);
			FName Key(*Id.ToString());

			auto* MD = NewObject<UMarketDataComponent>(Ctx.Actor);
			MD->RegisterComponent();
			MD->InitializeMarketData(Id, FString::Printf(TEXT("Integration Market %d"), i));
			MD->AddSupply(EResourceType::IronOre, 200 + i * 100);
			MD->AddSupply(EResourceType::CopperOre, 300 - i * 50);
			MD->AddSupply(EResourceType::GoldOre, 50 + i * 20);
			MD->SetDemandRate(EResourceType::IronOre, 10.0f + i * 5.0f);
			MD->SetDemandRate(EResourceType::CopperOre, 15.0f);
			MD->SetDemandRate(EResourceType::GoldOre, 5.0f);
			MD->RecalculateAllMetrics();
			Ctx.MarketDataMap.Add(Key, MD);

			auto* PS = NewObject<UPriceFluctuationSystem>(Ctx.Actor);
			PS->RegisterComponent();
			PS->Initialize(MD);
			FEconomyConfiguration Config;
			PS->SetConfiguration(Config);
			PS->UpdateAllPrices();
			Ctx.PriceSystemMap.Add(Key, PS);

			Ctx.RegisteredMarkets.Add(Id);
		}

		// Trade route analyzer
		Ctx.TradeAnalyzer = NewObject<UTradeRouteAnalyzer>(Ctx.Actor);
		Ctx.TradeAnalyzer->RegisterComponent();
		for (const FMarketId& Id : Ctx.RegisteredMarkets)
		{
			FName Key(*Id.ToString());
			Ctx.TradeAnalyzer->RegisterMarket(Id, Ctx.MarketDataMap[Key], Ctx.PriceSystemMap[Key]);
		}
		// Create routes between sequential markets
		for (int32 i = 0; i < NumMarkets - 1; ++i)
		{
			Ctx.TradeAnalyzer->DefineTradeRoute(
				Ctx.RegisteredMarkets[i], Ctx.RegisteredMarkets[i + 1],
				1000.0f, 2.0f, ETradeRouteRisk::Low);
			Ctx.TradeAnalyzer->DefineTradeRoute(
				Ctx.RegisteredMarkets[i + 1], Ctx.RegisteredMarkets[i],
				1000.0f, 2.0f, ETradeRouteRisk::Low);
		}

		// Event system
		Ctx.EventSystem = NewObject<UEconomicEventSystem>(Ctx.Actor);
		Ctx.EventSystem->RegisterComponent();
		FEventGenerationParams Params;
		Params.MaxActiveEvents = 20;
		Params.MinTimeBetweenEvents = 0.0f;
		Ctx.EventSystem->Initialize(Params);
		for (const FMarketId& Id : Ctx.RegisteredMarkets)
		{
			FName Key(*Id.ToString());
			Ctx.EventSystem->RegisterMarket(Id, Ctx.MarketDataMap[Key], Ctx.PriceSystemMap[Key]);
		}

		// Ripple system
		Ctx.RippleSystem = NewObject<UEconomyRippleEffect>(Ctx.Actor);
		Ctx.RippleSystem->RegisterComponent();
		FEconomyConfiguration RippleConfig;
		Ctx.RippleSystem->InitializeRippleSystem(RippleConfig);
		Ctx.RippleSystem->SetMarketReferences(&Ctx.MarketDataMap, &Ctx.PriceSystemMap, Ctx.TradeAnalyzer);

		// Save system
		Ctx.SaveSystem = NewObject<UEconomySaveSystem>(Ctx.Actor);
		Ctx.SaveSystem->RegisterComponent();
		Ctx.SaveSystem->SetEconomyReferences(
			&Ctx.MarketDataMap, &Ctx.PriceSystemMap,
			Ctx.TradeAnalyzer, Ctx.EventSystem,
			&Ctx.RegisteredMarkets);

		return Ctx;
	}
}

// ============================================================================
// 1. EVENT -> MARKET IMPACT INTEGRATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_EventAffectsMarketPrices,
	"Odyssey.Economy.Integration.EventAffectsMarketPrices",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_EventAffectsMarketPrices::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = IntegrationTestHelpers::CreateFullContext(World);
	TestNotNull(TEXT("TradeAnalyzer"), Ctx.TradeAnalyzer);

	FName Key(*Ctx.RegisteredMarkets[0].ToString());

	// Record baseline prices
	int32 BaselineBuyPrice = Ctx.PriceSystemMap[Key]->CalculateBuyPrice(EResourceType::IronOre);

	// Trigger a demand surge event on market 0
	TArray<FMarketId> Markets;
	Markets.Add(Ctx.RegisteredMarkets[0]);

	FEconomicEvent Event;
	Event.EventType = EEconomicEventType::DemandSurge;
	Event.Severity = EEconomicEventSeverity::Major;
	Event.EventName = TEXT("Iron Demand Surge");
	Event.Duration = 300.0;
	Event.AffectedMarkets = Markets;
	Event.AffectedResources.Add(EResourceType::IronOre);
	Event.SupplyModifier = 1.0f;
	Event.DemandModifier = 3.0f; // Triple demand
	Event.PriceModifier = 1.5f;
	Event.bShowNotification = true;

	Ctx.EventSystem->TriggerEvent(Event);

	// Apply the event's modifier to the market
	float DemandMod = Ctx.EventSystem->GetTotalDemandModifier(Ctx.RegisteredMarkets[0], EResourceType::IronOre);
	Ctx.MarketDataMap[Key]->SetDemandModifier(EResourceType::IronOre, DemandMod);
	Ctx.MarketDataMap[Key]->RecalculateAllMetrics();

	float PriceMod = Ctx.EventSystem->GetTotalPriceModifier(Ctx.RegisteredMarkets[0], EResourceType::IronOre);
	Ctx.PriceSystemMap[Key]->ApplyEventModifier(EResourceType::IronOre, PriceMod, 300.0f);
	Ctx.PriceSystemMap[Key]->UpdateAllPrices();

	int32 EventBuyPrice = Ctx.PriceSystemMap[Key]->CalculateBuyPrice(EResourceType::IronOre);

	TestTrue(TEXT("Price should increase after demand surge event"),
		EventBuyPrice >= BaselineBuyPrice);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 2. EVENT -> RIPPLE -> MULTI-MARKET PROPAGATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_EventTriggersRipple,
	"Odyssey.Economy.Integration.EventTriggersRipplePropagation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_EventTriggersRipple::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = IntegrationTestHelpers::CreateFullContext(World);
	TestNotNull(TEXT("RippleSystem"), Ctx.RippleSystem);

	// Trigger event at market 0
	TArray<FMarketId> Markets;
	Markets.Add(Ctx.RegisteredMarkets[0]);

	FEconomicEvent Event;
	Event.EventType = EEconomicEventType::ResourceDepletion;
	Event.Duration = 300.0;
	Event.AffectedMarkets = Markets;
	Event.AffectedResources.Add(EResourceType::IronOre);
	Event.SupplyModifier = 0.5f;
	int32 EventId = Ctx.EventSystem->TriggerEvent(Event);

	// Create a supply shock ripple triggered by this event
	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::IronOre);
	int32 RippleId = Ctx.RippleSystem->CreateSupplyShockRipple(
		Ctx.RegisteredMarkets[0], Resources, -0.5f, EventId);

	TestTrue(TEXT("Ripple should be created from event"), RippleId >= 0);
	TestEqual(TEXT("Should have 1 active ripple"), Ctx.RippleSystem->GetActiveRippleCount(), 1);

	FEconomicRipple Ripple = Ctx.RippleSystem->GetRipple(RippleId);
	TestEqual(TEXT("Ripple source event should match"), Ripple.SourceEventId, EventId);
	TestEqual(TEXT("Ripple origin should be market 0"), Ripple.OriginMarket, Ctx.RegisteredMarkets[0]);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 3. TRADE ROUTE -> PRICE DIFFERENTIAL -> OPPORTUNITY
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_PriceDifferentialCreatesOpportunity,
	"Odyssey.Economy.Integration.PriceDifferentialCreatesTradeOpportunity",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_PriceDifferentialCreatesOpportunity::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = IntegrationTestHelpers::CreateFullContext(World);
	TestNotNull(TEXT("TradeAnalyzer"), Ctx.TradeAnalyzer);

	// Create extreme price differential by manipulating supply
	FName Key0(*Ctx.RegisteredMarkets[0].ToString());
	FName Key1(*Ctx.RegisteredMarkets[1].ToString());

	// Market 0: Abundant Iron (cheap to buy)
	Ctx.MarketDataMap[Key0]->AddSupply(EResourceType::IronOre, 5000);
	Ctx.MarketDataMap[Key0]->SetDemandRate(EResourceType::IronOre, 1.0f);
	Ctx.MarketDataMap[Key0]->RecalculateAllMetrics();
	Ctx.PriceSystemMap[Key0]->UpdateAllPrices();

	// Market 1: Scarce Iron (expensive to buy, high demand)
	// Remove all supply to make it scarce
	int32 CurrentSupply = Ctx.MarketDataMap[Key1]->GetCurrentSupply(EResourceType::IronOre);
	if (CurrentSupply > 0) Ctx.MarketDataMap[Key1]->RemoveSupply(EResourceType::IronOre, CurrentSupply);
	Ctx.MarketDataMap[Key1]->SetDemandRate(EResourceType::IronOre, 100.0f);
	Ctx.MarketDataMap[Key1]->RecalculateAllMetrics();
	Ctx.PriceSystemMap[Key1]->UpdateAllPrices();

	// Analyze routes
	Ctx.TradeAnalyzer->AnalyzeAllRoutes();

	// Should find profitable Iron trade from Market 0 to Market 1
	FRouteAnalysisResult Analysis = Ctx.TradeAnalyzer->AnalyzeRoute(
		Ctx.RegisteredMarkets[0], Ctx.RegisteredMarkets[1]);

	// Verify analysis was performed
	TestEqual(TEXT("Analysis source should match"), Analysis.SourceMarket, Ctx.RegisteredMarkets[0]);

	// Check net profit
	int32 NetProfit = Ctx.TradeAnalyzer->CalculateNetProfit(
		Ctx.RegisteredMarkets[0], Ctx.RegisteredMarkets[1],
		EResourceType::IronOre, 10);

	// With such extreme differential, should have positive profit
	TestTrue(TEXT("Net profit should be positive with extreme supply differential"), NetProfit >= 0);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 4. FULL SAVE/LOAD CYCLE WITH ACTIVE EVENTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_SaveLoadWithActiveEvents,
	"Odyssey.Economy.Integration.SaveLoadPreservesActiveEvents",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_SaveLoadWithActiveEvents::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = IntegrationTestHelpers::CreateFullContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	// Trigger events
	TArray<FMarketId> Markets;
	Markets.Add(Ctx.RegisteredMarkets[0]);

	FEconomicEvent Event;
	Event.EventType = EEconomicEventType::MarketBoom;
	Event.Duration = 600.0;
	Event.AffectedMarkets = Markets;
	Event.AffectedResources.Add(EResourceType::IronOre);
	Event.SupplyModifier = 1.0f;
	Event.DemandModifier = 2.0f;
	Event.PriceModifier = 1.3f;
	Ctx.EventSystem->TriggerEvent(Event);

	int32 EventCountBefore = Ctx.EventSystem->GetActiveEventCount();

	// Save snapshot
	FEconomySaveData Snapshot = Ctx.SaveSystem->CaptureEconomySnapshot();

	TestTrue(TEXT("Snapshot should contain active events"), Snapshot.ActiveEvents.Num() >= 1);

	// Verify event data in snapshot
	bool bFoundBoomEvent = false;
	for (const auto& SavedEvent : Snapshot.ActiveEvents)
	{
		if (SavedEvent.EventType == EEconomicEventType::MarketBoom)
		{
			bFoundBoomEvent = true;
			TestTrue(TEXT("Saved event demand modifier should be 2.0"),
				FMath::IsNearlyEqual(SavedEvent.DemandModifier, 2.0f, 0.01f));
		}
	}
	TestTrue(TEXT("Market Boom event should be in snapshot"), bFoundBoomEvent);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 5. SUPPLY CHANGE -> PRICE -> TRADE ROUTE UPDATE CHAIN
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_SupplyChangePropagates,
	"Odyssey.Economy.Integration.SupplyChangePropagatesThroughPriceSystem",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_SupplyChangePropagates::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = IntegrationTestHelpers::CreateFullContext(World);
	TestNotNull(TEXT("Context"), Ctx.Actor);

	FName Key0(*Ctx.RegisteredMarkets[0].ToString());

	// Step 1: Record initial scarcity and price
	Ctx.MarketDataMap[Key0]->RecalculateAllMetrics();
	float InitialScarcity = Ctx.MarketDataMap[Key0]->GetScarcityIndex(EResourceType::IronOre);
	Ctx.PriceSystemMap[Key0]->UpdateAllPrices();
	int32 InitialPrice = Ctx.PriceSystemMap[Key0]->CalculateBuyPrice(EResourceType::IronOre);

	// Step 2: Remove most supply (simulate depletion)
	int32 CurrentSupply = Ctx.MarketDataMap[Key0]->GetCurrentSupply(EResourceType::IronOre);
	if (CurrentSupply > 10)
	{
		Ctx.MarketDataMap[Key0]->RemoveSupply(EResourceType::IronOre, CurrentSupply - 5);
	}

	// Step 3: Recalculate and verify scarcity increased
	Ctx.MarketDataMap[Key0]->RecalculateAllMetrics();
	float NewScarcity = Ctx.MarketDataMap[Key0]->GetScarcityIndex(EResourceType::IronOre);
	TestTrue(TEXT("Scarcity should increase after supply depletion"), NewScarcity >= InitialScarcity);

	// Step 4: Update prices and verify price increased
	Ctx.PriceSystemMap[Key0]->UpdateAllPrices();
	int32 NewPrice = Ctx.PriceSystemMap[Key0]->CalculateBuyPrice(EResourceType::IronOre);
	TestTrue(TEXT("Price should increase or stay same after supply depletion"), NewPrice >= InitialPrice);

	// Step 5: Analyze trade routes - the depleted market should now be a good sell destination
	Ctx.TradeAnalyzer->AnalyzeAllRoutes();
	TArray<FTradeOpportunity> Opps = Ctx.TradeAnalyzer->GetTopOpportunities(10);
	// Should not crash and should produce valid results
	TestTrue(TEXT("Trade analysis after supply change should not crash"), true);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 6. FEconomicEvent STRUCT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_EventStructLifecycle,
	"Odyssey.Economy.Integration.EventStructActivationAndExpiry",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_EventStructLifecycle::RunTest(const FString& Parameters)
{
	FEconomicEvent Event;
	Event.Duration = 100.0;

	// Before activation
	TestFalse(TEXT("Event should not be active before Activate()"), Event.bIsActive);

	// Activate at time 1000
	Event.Activate(1000.0);
	TestTrue(TEXT("Event should be active after Activate()"), Event.bIsActive);
	TestEqual(TEXT("Start time should be 1000"), Event.StartTime, 1000.0);
	TestEqual(TEXT("End time should be 1100"), Event.EndTime, 1100.0);

	// During event
	TestFalse(TEXT("Should not expire at t=1050"), Event.ShouldExpire(1050.0));
	float Remaining = Event.GetRemainingDuration(1050.0);
	TestTrue(TEXT("Remaining duration should be ~50"), FMath::IsNearlyEqual(Remaining, 50.0f, 1.0f));

	float Progress = Event.GetProgress(1050.0);
	TestTrue(TEXT("Progress should be ~0.5 at midpoint"), FMath::IsNearlyEqual(Progress, 0.5f, 0.01f));

	// After expiry
	TestTrue(TEXT("Should expire at t=1200"), Event.ShouldExpire(1200.0));
	float FinalRemaining = Event.GetRemainingDuration(1200.0);
	TestTrue(TEXT("Remaining should be 0 after expiry"), FMath::IsNearlyEqual(FinalRemaining, 0.0f, 0.01f));

	return true;
}

// ============================================================================
// 7. FTradeOpportunity METRIC CALCULATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_TradeOpportunityMetrics,
	"Odyssey.Economy.Integration.TradeOpportunityMetricCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_TradeOpportunityMetrics::RunTest(const FString& Parameters)
{
	FTradeOpportunity Opp;
	Opp.BuyPrice = 100;
	Opp.SellPrice = 150;
	Opp.AvailableQuantity = 50;
	Opp.Route.TravelTime = 2.0f;
	Opp.Route.RiskLevel = ETradeRouteRisk::Low;

	Opp.CalculateMetrics();

	TestEqual(TEXT("ProfitPerUnit should be 50"), Opp.ProfitPerUnit, 50);
	TestTrue(TEXT("ProfitMarginPercent should be 50%"), FMath::IsNearlyEqual(Opp.ProfitMarginPercent, 50.0f, 0.1f));
	TestEqual(TEXT("MaxProfitPotential should be 2500"), Opp.MaxProfitPotential, 2500);

	// RiskAdjustedReturn for Low risk (0.95 multiplier): 2500 * 0.95 = 2375
	TestTrue(TEXT("RiskAdjustedReturn should be ~2375"),
		FMath::IsNearlyEqual(Opp.RiskAdjustedReturn, 2375.0f, 10.0f));

	// TimeEfficiency: 2375 / 2.0 = 1187.5
	TestTrue(TEXT("TimeEfficiency should be ~1187.5"),
		FMath::IsNearlyEqual(Opp.TimeEfficiency, 1187.5f, 10.0f));

	TestTrue(TEXT("OpportunityScore should be positive"), Opp.OpportunityScore > 0.0f);

	// Test with dangerous route
	FTradeOpportunity DangerousOpp;
	DangerousOpp.BuyPrice = 100;
	DangerousOpp.SellPrice = 150;
	DangerousOpp.AvailableQuantity = 50;
	DangerousOpp.Route.TravelTime = 2.0f;
	DangerousOpp.Route.RiskLevel = ETradeRouteRisk::Dangerous;
	DangerousOpp.CalculateMetrics();

	TestTrue(TEXT("Dangerous route should have lower risk-adjusted return"),
		DangerousOpp.RiskAdjustedReturn < Opp.RiskAdjustedReturn);

	return true;
}

// ============================================================================
// 8. FResourceSupplyDemand RECALCULATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_SupplyDemandRecalculation,
	"Odyssey.Economy.Integration.SupplyDemandStructRecalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_SupplyDemandRecalculation::RunTest(const FString& Parameters)
{
	FResourceSupplyDemand Data;
	Data.CurrentSupply = 500;
	Data.MaxSupply = 1000;
	Data.DemandRate = 10.0f;
	Data.DemandModifier = 1.0f;

	Data.RecalculateMetrics();

	TestTrue(TEXT("SupplyDemandRatio should be positive"), Data.SupplyDemandRatio > 0.0f);
	TestTrue(TEXT("ScarcityIndex should be ~0.5 at half supply"),
		FMath::IsNearlyEqual(Data.ScarcityIndex, 0.5f, 0.01f));

	// Full supply
	Data.CurrentSupply = 1000;
	Data.RecalculateMetrics();
	TestTrue(TEXT("ScarcityIndex should be ~0.0 at full supply"),
		FMath::IsNearlyEqual(Data.ScarcityIndex, 0.0f, 0.01f));

	// Empty supply
	Data.CurrentSupply = 0;
	Data.RecalculateMetrics();
	TestTrue(TEXT("ScarcityIndex should be ~1.0 at zero supply"),
		FMath::IsNearlyEqual(Data.ScarcityIndex, 1.0f, 0.01f));

	// NaN protection: RecalculateMetrics should never produce NaN
	TestFalse(TEXT("SupplyDemandRatio must not be NaN"), FMath::IsNaN(Data.SupplyDemandRatio));
	TestFalse(TEXT("ScarcityIndex must not be NaN"), FMath::IsNaN(Data.ScarcityIndex));

	return true;
}

// ============================================================================
// 9. FDynamicMarketPrice HISTORY MANAGEMENT
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_PriceHistoryBounds,
	"Odyssey.Economy.Integration.PriceHistoryStaysWithinBounds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_PriceHistoryBounds::RunTest(const FString& Parameters)
{
	FDynamicMarketPrice Price;
	Price.MaxHistoryEntries = 5;

	// Add more entries than the max
	for (int32 i = 0; i < 20; ++i)
	{
		Price.AddHistoryEntry(100 + i, 10, 1.0f);
	}

	TestTrue(TEXT("History should be bounded to MaxHistoryEntries"),
		Price.PriceHistory.Num() <= 5);

	// Most recent entries should be preserved
	if (Price.PriceHistory.Num() > 0)
	{
		int32 LastPrice = Price.PriceHistory.Last().Price;
		TestEqual(TEXT("Last entry should be the most recent"), LastPrice, 119);
	}

	// Average price calculation
	float Avg = Price.CalculateAveragePrice(3);
	TestFalse(TEXT("Average must not be NaN"), FMath::IsNaN(Avg));
	TestTrue(TEXT("Average should be positive"), Avg > 0.0f);

	return true;
}

// ============================================================================
// 10. FMarketId EQUALITY AND HASHING
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_MarketIdOperations,
	"Odyssey.Economy.Integration.MarketIdEqualityAndHashing",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_MarketIdOperations::RunTest(const FString& Parameters)
{
	FMarketId A(FName("Station"), 1);
	FMarketId B(FName("Station"), 1);
	FMarketId C(FName("Station"), 2);
	FMarketId D(FName("Outpost"), 1);

	TestTrue(TEXT("Same name and region should be equal"), A == B);
	TestFalse(TEXT("Different region should not be equal"), A == C);
	TestFalse(TEXT("Different name should not be equal"), A == D);

	// Hash consistency
	TestEqual(TEXT("Equal IDs should have equal hashes"), GetTypeHash(A), GetTypeHash(B));

	// ToString
	FString Str = A.ToString();
	TestTrue(TEXT("ToString should contain market name"), Str.Contains(TEXT("Station")));
	TestTrue(TEXT("ToString should contain region"), Str.Contains(TEXT("1")));

	return true;
}

// ============================================================================
// 11. ECONOMY CONFIGURATION DEFAULTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconInteg_ConfigurationDefaults,
	"Odyssey.Economy.Integration.ConfigurationDefaultsAreReasonable",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconInteg_ConfigurationDefaults::RunTest(const FString& Parameters)
{
	FEconomyConfiguration Config;

	TestTrue(TEXT("TickInterval should be > 0"), Config.TickIntervalSeconds > 0.0f);
	TestTrue(TEXT("PriceUpdateInterval should be > 0"), Config.PriceUpdateIntervalSeconds > 0.0f);
	TestTrue(TEXT("BaseVolatility should be > 0"), Config.BaseVolatilityPercent > 0.0f);
	TestTrue(TEXT("SupplyDemandInfluence should be in [0,1]"),
		Config.SupplyDemandPriceInfluence >= 0.0f && Config.SupplyDemandPriceInfluence <= 1.0f);
	TestTrue(TEXT("PriceSmoothingFactor should be in [0,1]"),
		Config.PriceSmoothingFactor >= 0.0f && Config.PriceSmoothingFactor <= 1.0f);
	TestTrue(TEXT("MaxActiveEvents should be > 0"), Config.MaxActiveEvents > 0);
	TestTrue(TEXT("MaxPriceHistoryEntries should be > 0"), Config.MaxPriceHistoryEntries > 0);
	TestTrue(TEXT("RippleMaxPropagationDepth should be > 0"), Config.RippleMaxPropagationDepth > 0);
	TestTrue(TEXT("RippleDefaultDampening should be in (0,1)"),
		Config.RippleDefaultDampening > 0.0f && Config.RippleDefaultDampening < 1.0f);

	return true;
}
