// EconomyPerformanceTest.cpp
// Performance and stress tests for the Dynamic Economy System
// Tests scalability, tick performance, memory usage, and concurrent operations

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.h"
#include "UEconomicEventSystem.h"
#include "Economy/EconomyRippleEffect.h"

// ============================================================================
// Helper: performance test context builder
// ============================================================================
namespace PerfTestHelpers
{
	struct FPerfContext
	{
		AActor* Actor = nullptr;
		TArray<UMarketDataComponent*> MarketDataComponents;
		TArray<UPriceFluctuationSystem*> PriceSystems;
		TArray<FMarketId> MarketIds;

		void Destroy()
		{
			if (Actor) Actor->Destroy();
		}
	};

	FPerfContext CreateManyMarkets(UWorld* World, int32 Count)
	{
		FPerfContext Ctx;
		Ctx.Actor = World->SpawnActor<AActor>();
		if (!Ctx.Actor) return Ctx;

		for (int32 i = 0; i < Count; ++i)
		{
			FMarketId Id(FName(*FString::Printf(TEXT("PerfMarket_%d"), i)), 1);
			Ctx.MarketIds.Add(Id);

			auto* MD = NewObject<UMarketDataComponent>(Ctx.Actor);
			MD->RegisterComponent();
			MD->InitializeMarketData(Id, FString::Printf(TEXT("Market %d"), i));
			MD->AddSupply(EResourceType::IronOre, FMath::RandRange(100, 1000));
			MD->AddSupply(EResourceType::CopperOre, FMath::RandRange(100, 1000));
			MD->AddSupply(EResourceType::GoldOre, FMath::RandRange(10, 200));
			MD->SetDemandRate(EResourceType::IronOre, FMath::RandRange(5.0f, 50.0f));
			MD->SetDemandRate(EResourceType::CopperOre, FMath::RandRange(5.0f, 50.0f));
			MD->SetDemandRate(EResourceType::GoldOre, FMath::RandRange(1.0f, 20.0f));
			MD->RecalculateAllMetrics();
			Ctx.MarketDataComponents.Add(MD);

			auto* PS = NewObject<UPriceFluctuationSystem>(Ctx.Actor);
			PS->RegisterComponent();
			PS->Initialize(MD);
			FEconomyConfiguration Config;
			PS->SetConfiguration(Config);
			PS->UpdateAllPrices();
			Ctx.PriceSystems.Add(PS);
		}

		return Ctx;
	}
}

// ============================================================================
// 1. MARKET DATA TICK PERFORMANCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconPerf_MarketDataTickWith50Markets,
	"Odyssey.Economy.Performance.MarketData.TickWith50Markets",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconPerf_MarketDataTickWith50Markets::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PerfTestHelpers::CreateManyMarkets(World, 50);
	TestNotNull(TEXT("Actor"), Ctx.Actor);

	// Warm up
	for (auto* MD : Ctx.MarketDataComponents)
	{
		MD->SimulateSupplyDemand(0.016f);
	}

	// Measure time for 100 simulation steps across all 50 markets
	double StartTime = FPlatformTime::Seconds();

	for (int32 Step = 0; Step < 100; ++Step)
	{
		for (auto* MD : Ctx.MarketDataComponents)
		{
			MD->SimulateSupplyDemand(0.016f); // ~60fps delta
		}
	}

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	// Performance budget: 100 steps across 50 markets should take < 500ms
	TestTrue(
		FString::Printf(TEXT("50 markets x 100 steps took %.2fms (budget: 500ms)"), ElapsedMs),
		ElapsedMs < 500.0);

	AddInfo(FString::Printf(TEXT("50 markets x 100 steps: %.2fms (%.4fms per market per step)"),
		ElapsedMs, ElapsedMs / (50.0 * 100.0)));

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 2. PRICE UPDATE PERFORMANCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconPerf_PriceUpdateWith50Markets,
	"Odyssey.Economy.Performance.Pricing.UpdateWith50Markets",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconPerf_PriceUpdateWith50Markets::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PerfTestHelpers::CreateManyMarkets(World, 50);
	TestNotNull(TEXT("Actor"), Ctx.Actor);

	// Measure price update performance
	double StartTime = FPlatformTime::Seconds();

	for (int32 Step = 0; Step < 100; ++Step)
	{
		for (auto* PS : Ctx.PriceSystems)
		{
			PS->UpdateAllPrices();
		}
	}

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	TestTrue(
		FString::Printf(TEXT("50 markets x 100 price updates took %.2fms (budget: 1000ms)"), ElapsedMs),
		ElapsedMs < 1000.0);

	AddInfo(FString::Printf(TEXT("50 markets x 100 price updates: %.2fms"), ElapsedMs));

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 3. PRICE CALCULATION PERFORMANCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconPerf_PriceCalculation1000Queries,
	"Odyssey.Economy.Performance.Pricing.Calculate1000PriceQueries",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconPerf_PriceCalculation1000Queries::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PerfTestHelpers::CreateManyMarkets(World, 10);
	TestNotNull(TEXT("Actor"), Ctx.Actor);

	TArray<EResourceType> Resources = {EResourceType::IronOre, EResourceType::CopperOre, EResourceType::GoldOre};

	double StartTime = FPlatformTime::Seconds();

	int32 TotalQueries = 0;
	for (int32 i = 0; i < 1000; ++i)
	{
		for (auto* PS : Ctx.PriceSystems)
		{
			for (EResourceType Res : Resources)
			{
				volatile int32 BuyPrice = PS->CalculateBuyPrice(Res);
				volatile int32 SellPrice = PS->CalculateSellPrice(Res);
				(void)BuyPrice;
				(void)SellPrice;
				TotalQueries += 2;
			}
		}
	}

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	TestTrue(
		FString::Printf(TEXT("%d price queries took %.2fms (budget: 500ms)"), TotalQueries, ElapsedMs),
		ElapsedMs < 500.0);

	AddInfo(FString::Printf(TEXT("%d price queries: %.2fms (%.4fus per query)"),
		TotalQueries, ElapsedMs, (ElapsedMs * 1000.0) / TotalQueries));

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 4. SUPPLY/DEMAND OPERATIONS PERFORMANCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconPerf_SupplyDemandOperations,
	"Odyssey.Economy.Performance.SupplyDemand.HighVolumeOperations",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconPerf_SupplyDemandOperations::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PerfTestHelpers::CreateManyMarkets(World, 20);
	TestNotNull(TEXT("Actor"), Ctx.Actor);

	double StartTime = FPlatformTime::Seconds();

	int32 Ops = 0;
	for (int32 Round = 0; Round < 100; ++Round)
	{
		for (auto* MD : Ctx.MarketDataComponents)
		{
			MD->AddSupply(EResourceType::IronOre, 10);
			MD->RemoveSupply(EResourceType::IronOre, 5);
			MD->SetDemandRate(EResourceType::IronOre, 15.0f + Round * 0.1f);
			MD->RecalculateAllMetrics();

			volatile float Ratio = MD->GetSupplyDemandRatio(EResourceType::IronOre);
			volatile float Scarcity = MD->GetScarcityIndex(EResourceType::IronOre);
			(void)Ratio;
			(void)Scarcity;
			Ops += 6;
		}
	}

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	TestTrue(
		FString::Printf(TEXT("%d supply/demand ops took %.2fms (budget: 500ms)"), Ops, ElapsedMs),
		ElapsedMs < 500.0);

	AddInfo(FString::Printf(TEXT("%d supply/demand operations: %.2fms"), Ops, ElapsedMs));

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 5. TRADE ROUTE ANALYSIS PERFORMANCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconPerf_TradeRouteAnalysis20Markets,
	"Odyssey.Economy.Performance.TradeRoutes.FullAnalysisWith20Markets",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconPerf_TradeRouteAnalysis20Markets::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PerfTestHelpers::CreateManyMarkets(World, 20);
	TestNotNull(TEXT("Actor"), Ctx.Actor);

	// Create and register trade route analyzer
	auto* Analyzer = NewObject<UTradeRouteAnalyzer>(Ctx.Actor);
	Analyzer->RegisterComponent();

	for (int32 i = 0; i < Ctx.MarketIds.Num(); ++i)
	{
		Analyzer->RegisterMarket(Ctx.MarketIds[i], Ctx.MarketDataComponents[i], Ctx.PriceSystems[i]);
	}

	// Create routes between all markets (fully connected graph)
	for (int32 i = 0; i < Ctx.MarketIds.Num(); ++i)
	{
		for (int32 j = i + 1; j < Ctx.MarketIds.Num(); ++j)
		{
			float Dist = FMath::RandRange(500.0f, 5000.0f);
			float Time = Dist / 500.0f;
			Analyzer->DefineTradeRoute(Ctx.MarketIds[i], Ctx.MarketIds[j], Dist, Time, ETradeRouteRisk::Moderate);
			Analyzer->DefineTradeRoute(Ctx.MarketIds[j], Ctx.MarketIds[i], Dist, Time, ETradeRouteRisk::Moderate);
		}
	}

	// Measure full analysis
	double StartTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < 10; ++i)
	{
		Analyzer->AnalyzeAllRoutes();
	}

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	int32 RouteCount = Analyzer->GetAllRoutes().Num();
	TestTrue(
		FString::Printf(TEXT("10 full analyses of %d routes took %.2fms (budget: 2000ms)"), RouteCount, ElapsedMs),
		ElapsedMs < 2000.0);

	AddInfo(FString::Printf(TEXT("10 full analyses (%d routes across 20 markets): %.2fms"),
		RouteCount, ElapsedMs));

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 6. EVENT SYSTEM PERFORMANCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconPerf_EventSystemWith100Events,
	"Odyssey.Economy.Performance.Events.ManageAndQuery100Events",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconPerf_EventSystemWith100Events::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	AActor* TestActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Actor"), TestActor);

	auto* EventSystem = NewObject<UEconomicEventSystem>(TestActor);
	EventSystem->RegisterComponent();

	FEventGenerationParams Params;
	Params.MaxActiveEvents = 200;
	Params.MinTimeBetweenEvents = 0.0f;
	EventSystem->Initialize(Params);

	// Create a market
	FMarketId MarketId(FName("PerfEventMarket"), 1);
	auto* MD = NewObject<UMarketDataComponent>(TestActor);
	MD->RegisterComponent();
	MD->InitializeMarketData(MarketId, TEXT("PerfEventMarket"));
	MD->AddSupply(EResourceType::IronOre, 500);
	MD->RecalculateAllMetrics();

	auto* PS = NewObject<UPriceFluctuationSystem>(TestActor);
	PS->RegisterComponent();
	PS->Initialize(MD);
	PS->UpdateAllPrices();

	EventSystem->RegisterMarket(MarketId, MD, PS);

	// Trigger 100 events
	TArray<FMarketId> Markets;
	Markets.Add(MarketId);

	double StartTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < 100; ++i)
	{
		FEconomicEvent Event;
		Event.EventType = EEconomicEventType::ResourceDiscovery;
		Event.Severity = EEconomicEventSeverity::Minor;
		Event.EventName = FString::Printf(TEXT("Perf Event %d"), i);
		Event.Duration = 600.0;
		Event.AffectedMarkets = Markets;
		Event.AffectedResources.Add(EResourceType::IronOre);
		Event.SupplyModifier = 1.1f;
		Event.DemandModifier = 1.0f;
		Event.PriceModifier = 1.0f;
		Event.bShowNotification = false;

		EventSystem->TriggerEvent(Event);
	}

	// Query all events multiple times
	for (int32 i = 0; i < 100; ++i)
	{
		volatile int32 Count = EventSystem->GetActiveEventCount();
		(void)Count;
		TArray<FEconomicEvent> Active = EventSystem->GetActiveEvents();
		TArray<FEconomicEvent> MarketEvents = EventSystem->GetEventsAffectingMarket(MarketId);
		float SupplyMod = EventSystem->GetTotalSupplyModifier(MarketId, EResourceType::IronOre);
		(void)SupplyMod;
	}

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	TestTrue(
		FString::Printf(TEXT("100 events + 100 query rounds took %.2fms (budget: 1000ms)"), ElapsedMs),
		ElapsedMs < 1000.0);

	AddInfo(FString::Printf(TEXT("100 events + 100 query rounds: %.2fms"), ElapsedMs));

	TestActor->Destroy();
	return true;
}

// ============================================================================
// 7. PRICE HISTORY MEMORY SCALING
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconPerf_PriceHistoryMemoryBounded,
	"Odyssey.Economy.Performance.Memory.PriceHistoryIsBounded",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconPerf_PriceHistoryMemoryBounded::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	AActor* TestActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Actor"), TestActor);

	auto* MD = NewObject<UMarketDataComponent>(TestActor);
	MD->RegisterComponent();
	FMarketId Id(FName("MemoryTest"), 1);
	MD->InitializeMarketData(Id, TEXT("MemoryTest"));

	// Add thousands of price points
	for (int32 i = 0; i < 5000; ++i)
	{
		MD->RecordPricePoint(EResourceType::IronOre, FMath::RandRange(50, 200), FMath::RandRange(1, 50));
	}

	TArray<FPriceHistoryEntry> History = MD->GetPriceHistory(EResourceType::IronOre);

	// History should be bounded (FDynamicMarketPrice::MaxHistoryEntries defaults to 100)
	TestTrue(
		FString::Printf(TEXT("Price history should be bounded (got %d entries)"), History.Num()),
		History.Num() <= 200); // Allow some tolerance above default max

	AddInfo(FString::Printf(TEXT("Price history entries after 5000 recordings: %d"), History.Num()));

	TestActor->Destroy();
	return true;
}

// ============================================================================
// 8. CONCURRENT SUPPLY/DEMAND OPERATIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconPerf_ManyResourceTypes,
	"Odyssey.Economy.Performance.Scaling.ManyResourceTypesPerMarket",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconPerf_ManyResourceTypes::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	AActor* TestActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Actor"), TestActor);

	auto* MD = NewObject<UMarketDataComponent>(TestActor);
	MD->RegisterComponent();
	FMarketId Id(FName("MultiResTest"), 1);
	MD->InitializeMarketData(Id, TEXT("MultiResTest"));

	// Configure all known resource types
	TArray<EResourceType> AllResources = {
		EResourceType::IronOre,
		EResourceType::CopperOre,
		EResourceType::GoldOre,
		EResourceType::SilverOre,
		EResourceType::Titanium,
		EResourceType::Crystal,
		EResourceType::Wood,
		EResourceType::Stone,
		EResourceType::Fiber,
		EResourceType::Water
	};

	for (EResourceType Res : AllResources)
	{
		MD->AddSupply(Res, FMath::RandRange(50, 500));
		MD->SetDemandRate(Res, FMath::RandRange(5.0f, 50.0f));
	}

	double StartTime = FPlatformTime::Seconds();

	for (int32 Step = 0; Step < 1000; ++Step)
	{
		MD->SimulateSupplyDemand(0.016f);
		MD->RecalculateAllMetrics();
	}

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	TestTrue(
		FString::Printf(TEXT("1000 simulation steps with %d resource types took %.2fms (budget: 500ms)"),
			AllResources.Num(), ElapsedMs),
		ElapsedMs < 500.0);

	AddInfo(FString::Printf(TEXT("1000 steps with %d resource types: %.2fms"),
		AllResources.Num(), ElapsedMs));

	TestActor->Destroy();
	return true;
}

// ============================================================================
// 9. SCARCITY ANALYSIS PERFORMANCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconPerf_ScarcitySorting,
	"Odyssey.Economy.Performance.Analysis.ScarcitySortingPerformance",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconPerf_ScarcitySorting::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PerfTestHelpers::CreateManyMarkets(World, 30);
	TestNotNull(TEXT("Actor"), Ctx.Actor);

	double StartTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < 1000; ++i)
	{
		for (auto* MD : Ctx.MarketDataComponents)
		{
			TArray<EResourceType> Sorted = MD->GetResourcesByScarcity(true);
			(void)Sorted;
		}
	}

	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	TestTrue(
		FString::Printf(TEXT("30000 scarcity sort operations took %.2fms (budget: 1000ms)"), ElapsedMs),
		ElapsedMs < 1000.0);

	AddInfo(FString::Printf(TEXT("30 markets x 1000 scarcity sorts: %.2fms"), ElapsedMs));

	Ctx.Destroy();
	return true;
}
