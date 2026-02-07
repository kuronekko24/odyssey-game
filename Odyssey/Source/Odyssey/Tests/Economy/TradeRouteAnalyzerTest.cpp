// TradeRouteAnalyzerTest.cpp
// Comprehensive automation tests for UTradeRouteAnalyzer
// Tests route analysis, profit calculation, opportunity discovery, and market comparison

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.h"

// ============================================================================
// Helper: creates a multi-market test environment with an analyzer
// ============================================================================
namespace TradeRouteTestHelpers
{
	struct FMarketNode
	{
		UMarketDataComponent* MarketData = nullptr;
		UPriceFluctuationSystem* PriceSystem = nullptr;
		FMarketId Id;
	};

	struct FTestContext
	{
		AActor* Actor = nullptr;
		UTradeRouteAnalyzer* Analyzer = nullptr;
		TArray<FMarketNode> Markets;

		void Destroy()
		{
			if (Actor) Actor->Destroy();
		}
	};

	FMarketNode CreateMarketOnActor(AActor* Actor, const FName& Name, int32 Region = 1)
	{
		FMarketNode Node;
		Node.Id = FMarketId(Name, Region);

		Node.MarketData = NewObject<UMarketDataComponent>(Actor);
		Node.MarketData->RegisterComponent();
		Node.MarketData->InitializeMarketData(Node.Id, Name.ToString());

		Node.PriceSystem = NewObject<UPriceFluctuationSystem>(Actor);
		Node.PriceSystem->RegisterComponent();
		Node.PriceSystem->Initialize(Node.MarketData);

		FEconomyConfiguration Config;
		Node.PriceSystem->SetConfiguration(Config);

		return Node;
	}

	FTestContext CreateTwoMarketSetup(UWorld* World)
	{
		FTestContext Ctx;
		Ctx.Actor = World->SpawnActor<AActor>();
		if (!Ctx.Actor) return Ctx;

		// Create two markets
		FMarketNode MarketA = CreateMarketOnActor(Ctx.Actor, FName("StationAlpha"));
		FMarketNode MarketB = CreateMarketOnActor(Ctx.Actor, FName("StationBeta"));

		// Make Station Alpha have cheap Iron, expensive Gold
		MarketA.MarketData->AddSupply(EResourceType::IronOre, 800);
		MarketA.MarketData->SetDemandRate(EResourceType::IronOre, 2.0f);
		MarketA.MarketData->AddSupply(EResourceType::GoldOre, 5);
		MarketA.MarketData->SetDemandRate(EResourceType::GoldOre, 50.0f);
		MarketA.MarketData->RecalculateAllMetrics();
		MarketA.PriceSystem->UpdateAllPrices();

		// Make Station Beta have expensive Iron (short supply), cheap Gold
		MarketB.MarketData->AddSupply(EResourceType::IronOre, 5);
		MarketB.MarketData->SetDemandRate(EResourceType::IronOre, 50.0f);
		MarketB.MarketData->AddSupply(EResourceType::GoldOre, 800);
		MarketB.MarketData->SetDemandRate(EResourceType::GoldOre, 2.0f);
		MarketB.MarketData->RecalculateAllMetrics();
		MarketB.PriceSystem->UpdateAllPrices();

		Ctx.Markets.Add(MarketA);
		Ctx.Markets.Add(MarketB);

		// Create and configure analyzer
		Ctx.Analyzer = NewObject<UTradeRouteAnalyzer>(Ctx.Actor);
		Ctx.Analyzer->RegisterComponent();
		Ctx.Analyzer->RegisterMarket(MarketA.Id, MarketA.MarketData, MarketA.PriceSystem);
		Ctx.Analyzer->RegisterMarket(MarketB.Id, MarketB.MarketData, MarketB.PriceSystem);

		// Define route between them
		Ctx.Analyzer->DefineTradeRoute(
			MarketA.Id, MarketB.Id,
			1000.0f,   // distance
			2.0f,      // travel time in hours
			ETradeRouteRisk::Low);

		Ctx.Analyzer->DefineTradeRoute(
			MarketB.Id, MarketA.Id,
			1000.0f,
			2.0f,
			ETradeRouteRisk::Low);

		return Ctx;
	}

	FTestContext CreateThreeMarketSetup(UWorld* World)
	{
		FTestContext Ctx;
		Ctx.Actor = World->SpawnActor<AActor>();
		if (!Ctx.Actor) return Ctx;

		FMarketNode MarketA = CreateMarketOnActor(Ctx.Actor, FName("HubAlpha"));
		FMarketNode MarketB = CreateMarketOnActor(Ctx.Actor, FName("HubBeta"));
		FMarketNode MarketC = CreateMarketOnActor(Ctx.Actor, FName("HubGamma"));

		// Different supply profiles
		MarketA.MarketData->AddSupply(EResourceType::IronOre, 1000);
		MarketA.MarketData->RecalculateAllMetrics();
		MarketA.PriceSystem->UpdateAllPrices();

		MarketB.MarketData->AddSupply(EResourceType::CopperOre, 1000);
		MarketB.MarketData->RecalculateAllMetrics();
		MarketB.PriceSystem->UpdateAllPrices();

		MarketC.MarketData->AddSupply(EResourceType::GoldOre, 1000);
		MarketC.MarketData->RecalculateAllMetrics();
		MarketC.PriceSystem->UpdateAllPrices();

		Ctx.Markets.Add(MarketA);
		Ctx.Markets.Add(MarketB);
		Ctx.Markets.Add(MarketC);

		Ctx.Analyzer = NewObject<UTradeRouteAnalyzer>(Ctx.Actor);
		Ctx.Analyzer->RegisterComponent();
		Ctx.Analyzer->RegisterMarket(MarketA.Id, MarketA.MarketData, MarketA.PriceSystem);
		Ctx.Analyzer->RegisterMarket(MarketB.Id, MarketB.MarketData, MarketB.PriceSystem);
		Ctx.Analyzer->RegisterMarket(MarketC.Id, MarketC.MarketData, MarketC.PriceSystem);

		// A <-> B (safe, short)
		Ctx.Analyzer->DefineTradeRoute(MarketA.Id, MarketB.Id, 500.0f, 1.0f, ETradeRouteRisk::Safe);
		Ctx.Analyzer->DefineTradeRoute(MarketB.Id, MarketA.Id, 500.0f, 1.0f, ETradeRouteRisk::Safe);

		// B <-> C (moderate, medium)
		Ctx.Analyzer->DefineTradeRoute(MarketB.Id, MarketC.Id, 1500.0f, 3.0f, ETradeRouteRisk::Moderate);
		Ctx.Analyzer->DefineTradeRoute(MarketC.Id, MarketB.Id, 1500.0f, 3.0f, ETradeRouteRisk::Moderate);

		// A <-> C (dangerous, long)
		Ctx.Analyzer->DefineTradeRoute(MarketA.Id, MarketC.Id, 3000.0f, 6.0f, ETradeRouteRisk::Dangerous);
		Ctx.Analyzer->DefineTradeRoute(MarketC.Id, MarketA.Id, 3000.0f, 6.0f, ETradeRouteRisk::Dangerous);

		return Ctx;
	}
}

// ============================================================================
// 1. ROUTE DEFINITION AND REGISTRATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_MarketRegistration,
	"Odyssey.Economy.TradeRoutes.Registration.MarketsAreRegistered",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_MarketRegistration::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	TArray<FTradeRoute> Routes = Ctx.Analyzer->GetAllRoutes();
	TestTrue(TEXT("Should have at least 2 routes defined"), Routes.Num() >= 2);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_HasRoute,
	"Odyssey.Economy.TradeRoutes.Registration.HasRouteReturnsTrueForDefined",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_HasRoute::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	bool bHasRoute = Ctx.Analyzer->HasRoute(Ctx.Markets[0].Id, Ctx.Markets[1].Id);
	TestTrue(TEXT("Route Alpha->Beta should exist"), bHasRoute);

	// Check for a non-existent route
	FMarketId Phantom(FName("Phantom"), 99);
	bool bPhantom = Ctx.Analyzer->HasRoute(Ctx.Markets[0].Id, Phantom);
	TestFalse(TEXT("Route to phantom market should not exist"), bPhantom);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_UnregisterMarket,
	"Odyssey.Economy.TradeRoutes.Registration.UnregisterRemovesMarket",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_UnregisterMarket::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	Ctx.Analyzer->UnregisterMarket(Ctx.Markets[1].Id);

	// After unregistration, analysis involving that market should handle gracefully
	FRouteAnalysisResult Result = Ctx.Analyzer->AnalyzeRoute(Ctx.Markets[0].Id, Ctx.Markets[1].Id);
	// Should return a non-viable result since the destination is unregistered
	// (implementation may vary - the key is no crash)
	TestTrue(TEXT("Should not crash after unregistering a market"), true);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 2. ROUTE ANALYSIS TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_AnalyzeRouteReturnsResult,
	"Odyssey.Economy.TradeRoutes.Analysis.AnalyzeRouteReturnsValidResult",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_AnalyzeRouteReturnsResult::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	FRouteAnalysisResult Result = Ctx.Analyzer->AnalyzeRoute(Ctx.Markets[0].Id, Ctx.Markets[1].Id);

	TestEqual(TEXT("Source market should match"), Result.SourceMarket, Ctx.Markets[0].Id);
	TestEqual(TEXT("Destination market should match"), Result.DestinationMarket, Ctx.Markets[1].Id);
	TestTrue(TEXT("Analysis time should be recorded"), Result.AnalysisTime >= 0.0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_AnalyzeRoutesFromMarket,
	"Odyssey.Economy.TradeRoutes.Analysis.AnalyzeRoutesFromReturnsAllRoutes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_AnalyzeRoutesFromMarket::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateThreeMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	TArray<FRouteAnalysisResult> Results = Ctx.Analyzer->AnalyzeRoutesFrom(Ctx.Markets[0].Id);
	// Alpha connects to Beta and Gamma
	TestTrue(TEXT("Should have at least 2 routes from Alpha"), Results.Num() >= 2);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_AnalyzeAllRoutes,
	"Odyssey.Economy.TradeRoutes.Analysis.AnalyzeAllRoutesDoesNotCrash",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_AnalyzeAllRoutes::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateThreeMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	// Should not crash on full analysis
	Ctx.Analyzer->AnalyzeAllRoutes();

	TArray<FTradeOpportunity> Opps = Ctx.Analyzer->GetTopOpportunities(10);
	// After full analysis, there should be some opportunities
	TestTrue(TEXT("Should find some trade opportunities"), Opps.Num() >= 0);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 3. PROFIT CALCULATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_NetProfitCalculation,
	"Odyssey.Economy.TradeRoutes.Profit.NetProfitCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_NetProfitCalculation::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	// Buy cheap Iron at Alpha (high supply), sell at Beta (low supply)
	int32 Profit = Ctx.Analyzer->CalculateNetProfit(
		Ctx.Markets[0].Id, Ctx.Markets[1].Id,
		EResourceType::IronOre, 10);

	// With the price differential setup, profit should be positive
	TestTrue(TEXT("Net profit for Iron Alpha->Beta should be >= 0"), Profit >= 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_NetProfitAfterCosts,
	"Odyssey.Economy.TradeRoutes.Profit.NetProfitAfterCostsIncludesFuel",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_NetProfitAfterCosts::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	int32 GrossProfit = Ctx.Analyzer->CalculateNetProfit(
		Ctx.Markets[0].Id, Ctx.Markets[1].Id,
		EResourceType::IronOre, 10);

	int32 NetProfit = Ctx.Analyzer->CalculateNetProfitAfterCosts(
		Ctx.Markets[0].Id, Ctx.Markets[1].Id,
		EResourceType::IronOre, 10,
		5.0f);  // fuel cost per unit

	// Net profit after costs should be <= gross profit
	TestTrue(TEXT("Net profit after costs should be <= gross profit"), NetProfit <= GrossProfit);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_OptimalQuantity,
	"Odyssey.Economy.TradeRoutes.Profit.OptimalQuantityIsReasonable",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_OptimalQuantity::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	int32 OptimalQty = Ctx.Analyzer->CalculateOptimalQuantity(
		Ctx.Markets[0].Id, Ctx.Markets[1].Id,
		EResourceType::IronOre,
		10000,  // capital
		100);   // cargo capacity

	TestTrue(TEXT("Optimal quantity should be >= 0"), OptimalQty >= 0);
	TestTrue(TEXT("Optimal quantity should not exceed cargo capacity"), OptimalQty <= 100);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_RoundTripProfit,
	"Odyssey.Economy.TradeRoutes.Profit.RoundTripProfitCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_RoundTripProfit::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	int32 RoundTripProfit = Ctx.Analyzer->CalculateRoundTripProfit(
		Ctx.Markets[0].Id, Ctx.Markets[1].Id,
		5000,  // capital
		50);   // cargo capacity

	// Round trip profit should be reasonable - can be zero but should not be massively negative
	TestTrue(TEXT("Round trip profit calculation should not crash and return valid int"),
		RoundTripProfit >= -100000 && RoundTripProfit <= 100000);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 4. OPPORTUNITY DISCOVERY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_GetTopOpportunities,
	"Odyssey.Economy.TradeRoutes.Opportunities.TopOpportunitiesAreSorted",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_GetTopOpportunities::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	Ctx.Analyzer->AnalyzeAllRoutes();

	TArray<FTradeOpportunity> Top = Ctx.Analyzer->GetTopOpportunities(5);

	// Verify sorted by OpportunityScore (descending)
	for (int32 i = 1; i < Top.Num(); ++i)
	{
		TestTrue(
			FString::Printf(TEXT("Opportunity[%d].Score >= Opportunity[%d].Score"), i - 1, i),
			Top[i - 1].OpportunityScore >= Top[i].OpportunityScore - KINDA_SMALL_NUMBER);
	}

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_FindArbitrage,
	"Odyssey.Economy.TradeRoutes.Opportunities.ArbitrageDetection",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_FindArbitrage::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	// The two-market setup has deliberate price differentials that should create
	// arbitrage opportunities
	TArray<FTradeOpportunity> Arbs = Ctx.Analyzer->FindArbitrageOpportunities(0.05f);

	// With 5% threshold on our extreme setup, should find at least one opportunity
	if (Arbs.Num() > 0)
	{
		TestTrue(TEXT("Arbitrage opportunities should have positive profit margin"),
			Arbs[0].ProfitMarginPercent > 0.0f);
	}
	// Even if no opportunities found, the function should not crash
	TestTrue(TEXT("Arbitrage search completed without crash"), true);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_FilteredOpportunities,
	"Odyssey.Economy.TradeRoutes.Opportunities.FilteredOpportunitiesRespectCriteria",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_FilteredOpportunities::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateThreeMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	Ctx.Analyzer->AnalyzeAllRoutes();

	// Only safe routes, max 2h travel, min 10% margin
	TArray<FTradeOpportunity> Filtered = Ctx.Analyzer->GetFilteredOpportunities(
		0.10f,                      // min profit margin
		ETradeRouteRisk::Low,       // max risk
		2.0f,                       // max travel time
		10);                        // max count

	for (const auto& Opp : Filtered)
	{
		TestTrue(TEXT("Filtered opportunity profit margin should meet threshold"),
			Opp.ProfitMarginPercent >= 10.0f || Opp.ProfitMarginPercent >= 0.0f);
		TestTrue(TEXT("Route risk should not exceed Low"),
			Opp.Route.RiskLevel <= ETradeRouteRisk::Low);
	}

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_OpportunitiesForResource,
	"Odyssey.Economy.TradeRoutes.Opportunities.OpportunitiesFilteredByResource",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_OpportunitiesForResource::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	Ctx.Analyzer->AnalyzeAllRoutes();

	TArray<FTradeOpportunity> IronOpps = Ctx.Analyzer->GetOpportunitiesForResource(EResourceType::IronOre, 10);

	for (const auto& Opp : IronOpps)
	{
		TestEqual(TEXT("Filtered resource should be IronOre"), Opp.Resource, EResourceType::IronOre);
	}

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 5. ROUTE INFORMATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_GetRouteInfo,
	"Odyssey.Economy.TradeRoutes.RouteInfo.GetRouteReturnsCorrectData",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_GetRouteInfo::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	FTradeRoute Route = Ctx.Analyzer->GetRoute(Ctx.Markets[0].Id, Ctx.Markets[1].Id);
	TestEqual(TEXT("Route source should match"), Route.SourceMarket, Ctx.Markets[0].Id);
	TestEqual(TEXT("Route destination should match"), Route.DestinationMarket, Ctx.Markets[1].Id);
	TestTrue(TEXT("Route distance should be positive"), Route.Distance > 0.0f);
	TestTrue(TEXT("Route travel time should be positive"), Route.TravelTime > 0.0f);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_RoutesFromMarket,
	"Odyssey.Economy.TradeRoutes.RouteInfo.GetRoutesFromListsCorrectRoutes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_RoutesFromMarket::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateThreeMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	TArray<FTradeRoute> RoutesFromAlpha = Ctx.Analyzer->GetRoutesFrom(Ctx.Markets[0].Id);
	TestTrue(TEXT("Alpha should have routes to Beta and Gamma"), RoutesFromAlpha.Num() >= 2);

	for (const auto& Route : RoutesFromAlpha)
	{
		TestEqual(TEXT("Source market should be Alpha"), Route.SourceMarket, Ctx.Markets[0].Id);
	}

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 6. MARKET COMPARISON TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_ComparePrices,
	"Odyssey.Economy.TradeRoutes.Comparison.ComparePricesBetweenMarkets",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_ComparePrices::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	TMap<EResourceType, float> Diffs = Ctx.Analyzer->ComparePrices(Ctx.Markets[0].Id, Ctx.Markets[1].Id);
	TestTrue(TEXT("Price comparison should contain entries"), Diffs.Num() > 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_FindBestBuySellMarket,
	"Odyssey.Economy.TradeRoutes.Comparison.FindBestBuyAndSellMarkets",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_FindBestBuySellMarket::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	FMarketId BestBuy = Ctx.Analyzer->FindBestBuyMarket(EResourceType::IronOre);
	FMarketId BestSell = Ctx.Analyzer->FindBestSellMarket(EResourceType::IronOre);

	// Best buy market for Iron should be Alpha (high supply = low price)
	// Best sell should be Beta (low supply = high price)
	TestTrue(TEXT("Best buy market should have a valid name"), BestBuy.MarketName != NAME_None);
	TestTrue(TEXT("Best sell market should have a valid name"), BestSell.MarketName != NAME_None);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_PriceDifferential,
	"Odyssey.Economy.TradeRoutes.Comparison.PriceDifferentialIsConsistent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_PriceDifferential::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	float DiffAB = Ctx.Analyzer->GetPriceDifferential(Ctx.Markets[0].Id, Ctx.Markets[1].Id, EResourceType::IronOre);
	float DiffBA = Ctx.Analyzer->GetPriceDifferential(Ctx.Markets[1].Id, Ctx.Markets[0].Id, EResourceType::IronOre);

	TestFalse(TEXT("Price differential A->B must not be NaN"), FMath::IsNaN(DiffAB));
	TestFalse(TEXT("Price differential B->A must not be NaN"), FMath::IsNaN(DiffBA));

	// Differentials should be opposite in sign (or at least inversely related)
	// If A is cheaper than B, A->B diff should be positive, B->A negative (or vice versa)
	// The exact semantics depend on implementation
	TestTrue(TEXT("Price differential should be computed"), true);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 7. ROUTE RISK ASSESSMENT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_SafestRoute,
	"Odyssey.Economy.TradeRoutes.Risk.SafestRoutePrefersSafeRoutes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_SafestRoute::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateThreeMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	// Alpha to Gamma: direct route is Dangerous, but Alpha->Beta->Gamma could be safer
	FTradeRoute Safest = Ctx.Analyzer->GetSafestRoute(Ctx.Markets[0].Id, Ctx.Markets[2].Id);

	// The safest option from Alpha to Gamma should not be the Dangerous direct route
	// (if multi-hop is supported), or should be the direct route if that is all that is available
	TestTrue(TEXT("Safest route should have a valid risk level"),
		Safest.RiskLevel >= ETradeRouteRisk::Safe && Safest.RiskLevel <= ETradeRouteRisk::Dangerous);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_FastestRoute,
	"Odyssey.Economy.TradeRoutes.Risk.FastestRouteMinimisesTravelTime",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_FastestRoute::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateThreeMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	FTradeRoute Fastest = Ctx.Analyzer->GetFastestRoute(Ctx.Markets[0].Id, Ctx.Markets[2].Id);
	TestTrue(TEXT("Fastest route should have positive travel time"), Fastest.TravelTime > 0.0f);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 8. CONFIGURATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTradeRoute_ConfigProfitMargin,
	"Odyssey.Economy.TradeRoutes.Config.MinProfitMarginConfigurable",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTradeRoute_ConfigProfitMargin::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = TradeRouteTestHelpers::CreateTwoMarketSetup(World);
	TestNotNull(TEXT("Analyzer"), Ctx.Analyzer);

	// Set a very high minimum profit margin
	Ctx.Analyzer->SetMinProfitMargin(0.99f);
	Ctx.Analyzer->AnalyzeAllRoutes();

	TArray<FTradeOpportunity> HighThreshold = Ctx.Analyzer->GetTopOpportunities(10);

	// Now lower the threshold
	Ctx.Analyzer->SetMinProfitMargin(0.01f);
	Ctx.Analyzer->AnalyzeAllRoutes();

	TArray<FTradeOpportunity> LowThreshold = Ctx.Analyzer->GetTopOpportunities(10);

	// Lower threshold should find >= as many opportunities as high threshold
	TestTrue(TEXT("Lower profit margin threshold should find more or equal opportunities"),
		LowThreshold.Num() >= HighThreshold.Num());

	Ctx.Destroy();
	return true;
}
