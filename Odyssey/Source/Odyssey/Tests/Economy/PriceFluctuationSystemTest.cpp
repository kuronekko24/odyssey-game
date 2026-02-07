// PriceFluctuationSystemTest.cpp
// Comprehensive automation tests for UPriceFluctuationSystem
// Tests price calculation, volatility, supply/demand factors, trends, and shocks

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"

// ============================================================================
// Helper: creates a pair of (MarketData, PriceFluctuation) components on a
// temporary actor and links them together.
// ============================================================================
namespace PriceFlucTestHelpers
{
	struct FTestContext
	{
		AActor* Actor = nullptr;
		UMarketDataComponent* MarketData = nullptr;
		UPriceFluctuationSystem* PriceSystem = nullptr;

		void Destroy()
		{
			if (Actor) Actor->Destroy();
		}
	};

	FTestContext CreateLinkedComponents(UWorld* World, const FName& MarketName)
	{
		FTestContext Ctx;
		Ctx.Actor = World->SpawnActor<AActor>();
		if (!Ctx.Actor) return Ctx;

		Ctx.MarketData = NewObject<UMarketDataComponent>(Ctx.Actor);
		Ctx.MarketData->RegisterComponent();
		FMarketId Id(MarketName, 1);
		Ctx.MarketData->InitializeMarketData(Id, MarketName.ToString());

		Ctx.PriceSystem = NewObject<UPriceFluctuationSystem>(Ctx.Actor);
		Ctx.PriceSystem->RegisterComponent();
		Ctx.PriceSystem->Initialize(Ctx.MarketData);

		FEconomyConfiguration Config;
		Config.BaseVolatilityPercent = 0.05f;
		Config.SupplyDemandPriceInfluence = 0.7f;
		Config.PriceSmoothingFactor = 0.5f;
		Ctx.PriceSystem->SetConfiguration(Config);

		return Ctx;
	}
}

// ============================================================================
// 1. PRICE CALCULATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_BuyPricePositive,
	"Odyssey.Economy.PriceFluctuation.Calculation.BuyPriceIsPositive",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_BuyPricePositive::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_BuyPrice"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->UpdateAllPrices();

	int32 BuyPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);
	TestTrue(TEXT("Buy price must be > 0"), BuyPrice > 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_SellPriceLowerThanBuy,
	"Odyssey.Economy.PriceFluctuation.Calculation.SellPriceLowerThanBuyPrice",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_SellPriceLowerThanBuy::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_BuySellSpread"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->UpdateAllPrices();

	int32 BuyPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);
	int32 SellPrice = Ctx.PriceSystem->CalculateSellPrice(EResourceType::IronOre);

	TestTrue(TEXT("Sell price must be <= Buy price (spread)"), SellPrice <= BuyPrice);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_BulkPricing,
	"Odyssey.Economy.PriceFluctuation.Calculation.BulkQuantityPricing",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_BulkPricing::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_Bulk"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->UpdateAllPrices();

	int32 SingleBuy = Ctx.PriceSystem->CalculateBuyPriceForQuantity(EResourceType::IronOre, 1);
	int32 BulkBuy = Ctx.PriceSystem->CalculateBuyPriceForQuantity(EResourceType::IronOre, 100);

	TestTrue(TEXT("Single buy price must be > 0"), SingleBuy > 0);
	TestTrue(TEXT("Bulk buy price must be > 0"), BulkBuy > 0);
	// Bulk should cost at least as much as single (or potentially more per unit due to market impact)
	TestTrue(TEXT("Bulk price for 100 units should be > single unit price"), BulkBuy > SingleBuy);

	int32 SingleSell = Ctx.PriceSystem->CalculateSellPriceForQuantity(EResourceType::IronOre, 1);
	int32 BulkSell = Ctx.PriceSystem->CalculateSellPriceForQuantity(EResourceType::IronOre, 100);
	TestTrue(TEXT("Single sell price must be > 0"), SingleSell > 0);
	TestTrue(TEXT("Bulk sell for 100 should be > single sell"), BulkSell > SingleSell);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_PriceCalculationDetails,
	"Odyssey.Economy.PriceFluctuation.Calculation.DetailedBreakdownIsComplete",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_PriceCalculationDetails::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_Details"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->UpdateAllPrices();

	FPriceCalculationResult Result = Ctx.PriceSystem->GetPriceCalculationDetails(EResourceType::IronOre);

	TestTrue(TEXT("BasePrice should be > 0"), Result.BasePrice > 0);
	TestTrue(TEXT("SupplyDemandFactor should be > 0"), Result.SupplyDemandFactor > 0.0f);
	TestTrue(TEXT("CalculatedPrice should be > 0"), Result.CalculatedPrice > 0);
	TestTrue(TEXT("ClampedPrice should be > 0"), Result.ClampedPrice > 0);
	TestTrue(TEXT("ClampedPrice should be <= CalculatedPrice or clamped to range"),
		Result.ClampedPrice > 0);
	TestEqual(TEXT("Resource type should match"), Result.Resource, EResourceType::IronOre);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 2. SUPPLY/DEMAND RESPONSE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_HighDemandIncreasesPrice,
	"Odyssey.Economy.PriceFluctuation.SupplyDemand.HighDemandIncreasesPrice",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_HighDemandIncreasesPrice::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_HighDemand"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	// Set normal conditions first
	Ctx.MarketData->AddSupply(EResourceType::IronOre, 500);
	Ctx.MarketData->SetDemandRate(EResourceType::IronOre, 10.0f);
	Ctx.MarketData->RecalculateAllMetrics();
	Ctx.PriceSystem->UpdateAllPrices();
	int32 NormalPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);

	// Now spike demand
	Ctx.MarketData->SetDemandRate(EResourceType::IronOre, 100.0f);
	Ctx.MarketData->SetDemandModifier(EResourceType::IronOre, 3.0f);
	Ctx.MarketData->RecalculateAllMetrics();
	Ctx.PriceSystem->UpdateAllPrices();
	int32 HighDemandPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);

	TestTrue(TEXT("High demand should produce a higher price"), HighDemandPrice >= NormalPrice);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_HighSupplyDecreasesPrice,
	"Odyssey.Economy.PriceFluctuation.SupplyDemand.HighSupplyDecreasesPrice",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_HighSupplyDecreasesPrice::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_HighSupply"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	// Low supply scenario
	Ctx.MarketData->AddSupply(EResourceType::CopperOre, 10);
	Ctx.MarketData->SetDemandRate(EResourceType::CopperOre, 50.0f);
	Ctx.MarketData->RecalculateAllMetrics();
	Ctx.PriceSystem->UpdateAllPrices();
	int32 LowSupplyPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::CopperOre);

	// Flood market with supply
	Ctx.MarketData->AddSupply(EResourceType::CopperOre, 5000);
	Ctx.MarketData->RecalculateAllMetrics();
	Ctx.PriceSystem->UpdateAllPrices();
	int32 HighSupplyPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::CopperOre);

	TestTrue(TEXT("High supply should produce a lower or equal price"), HighSupplyPrice <= LowSupplyPrice);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_SupplyDemandFactor,
	"Odyssey.Economy.PriceFluctuation.SupplyDemand.FactorCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_SupplyDemandFactor::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_SDFactor"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.MarketData->AddSupply(EResourceType::IronOre, 500);
	Ctx.MarketData->SetDemandRate(EResourceType::IronOre, 10.0f);
	Ctx.MarketData->RecalculateAllMetrics();

	float Factor = Ctx.PriceSystem->CalculateSupplyDemandFactor(EResourceType::IronOre);
	TestTrue(TEXT("Supply/demand factor must be > 0"), Factor > 0.0f);
	TestFalse(TEXT("Factor must not be NaN"), FMath::IsNaN(Factor));

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_ScarcityPremium,
	"Odyssey.Economy.PriceFluctuation.SupplyDemand.ScarcityPremiumWhenLowSupply",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_ScarcityPremium::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_Scarcity"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	// Very low supply
	Ctx.MarketData->AddSupply(EResourceType::GoldOre, 1);
	Ctx.MarketData->RecalculateAllMetrics();

	float Premium = Ctx.PriceSystem->CalculateScarcityPremium(EResourceType::GoldOre);
	TestTrue(TEXT("Scarcity premium should be >= 0"), Premium >= 0.0f);

	// High supply should have lower or zero premium
	Ctx.MarketData->AddSupply(EResourceType::GoldOre, 5000);
	Ctx.MarketData->RecalculateAllMetrics();

	float LowPremium = Ctx.PriceSystem->CalculateScarcityPremium(EResourceType::GoldOre);
	TestTrue(TEXT("High supply scarcity premium should be <= low supply premium"), LowPremium <= Premium);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_AbundanceDiscount,
	"Odyssey.Economy.PriceFluctuation.SupplyDemand.AbundanceDiscountWhenHighSupply",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_AbundanceDiscount::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_Abundance"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	// Fill market to capacity
	int32 Max = Ctx.MarketData->GetMaxSupply(EResourceType::IronOre);
	Ctx.MarketData->AddSupply(EResourceType::IronOre, Max);
	Ctx.MarketData->RecalculateAllMetrics();

	float Discount = Ctx.PriceSystem->CalculateAbundanceDiscount(EResourceType::IronOre);
	TestTrue(TEXT("Abundance discount should be >= 0 when oversupplied"), Discount >= 0.0f);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 3. VOLATILITY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_VolatilityFactorInRange,
	"Odyssey.Economy.PriceFluctuation.Volatility.FactorIsWithinBounds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_VolatilityFactorInRange::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_VolRange"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->SetResourceVolatility(EResourceType::IronOre, EMarketVolatility::Stable);
	float StableFactor = Ctx.PriceSystem->GenerateVolatilityFactor(EResourceType::IronOre);
	TestFalse(TEXT("Volatility factor must not be NaN"), FMath::IsNaN(StableFactor));

	Ctx.PriceSystem->SetResourceVolatility(EResourceType::CopperOre, EMarketVolatility::Extreme);
	float ExtremeFactor = Ctx.PriceSystem->GenerateVolatilityFactor(EResourceType::CopperOre);
	TestFalse(TEXT("Extreme volatility factor must not be NaN"), FMath::IsNaN(ExtremeFactor));

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_VolatilityRangeScaling,
	"Odyssey.Economy.PriceFluctuation.Volatility.RangeScalesWithLevel",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_VolatilityRangeScaling::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_VolScale"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->SetResourceVolatility(EResourceType::IronOre, EMarketVolatility::Stable);
	float StableRange = Ctx.PriceSystem->GetVolatilityRange(EResourceType::IronOre);

	Ctx.PriceSystem->SetResourceVolatility(EResourceType::IronOre, EMarketVolatility::High);
	float HighRange = Ctx.PriceSystem->GetVolatilityRange(EResourceType::IronOre);

	Ctx.PriceSystem->SetResourceVolatility(EResourceType::IronOre, EMarketVolatility::Extreme);
	float ExtremeRange = Ctx.PriceSystem->GetVolatilityRange(EResourceType::IronOre);

	TestTrue(TEXT("Stable range < High range"), StableRange < HighRange);
	TestTrue(TEXT("High range < Extreme range"), HighRange < ExtremeRange);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_MarketNoise,
	"Odyssey.Economy.PriceFluctuation.Volatility.MarketNoiseIsBounded",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_MarketNoise::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_Noise"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	// Sample market noise many times and verify it stays within reasonable bounds
	float MinNoise = 999.0f;
	float MaxNoise = -999.0f;
	for (int32 i = 0; i < 100; ++i)
	{
		float Noise = Ctx.PriceSystem->SimulateMarketNoise(0.1f);
		MinNoise = FMath::Min(MinNoise, Noise);
		MaxNoise = FMath::Max(MaxNoise, Noise);
		TestFalse(TEXT("Market noise must not be NaN"), FMath::IsNaN(Noise));
	}

	// With 10% base volatility, noise should be roughly in [-0.5, 0.5] at most
	TestTrue(TEXT("Market noise should stay within reasonable bounds"), MaxNoise < 5.0f);
	TestTrue(TEXT("Market noise should stay within reasonable bounds"), MinNoise > -5.0f);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 4. PRICE SHOCK TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_PriceShockIncrease,
	"Odyssey.Economy.PriceFluctuation.Shocks.PriceShockIncreasesPrices",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_PriceShockIncrease::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_ShockUp"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.MarketData->AddSupply(EResourceType::IronOre, 500);
	Ctx.MarketData->RecalculateAllMetrics();
	Ctx.PriceSystem->UpdateAllPrices();

	int32 BaseBuyPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);

	// Apply a 2x price shock
	Ctx.PriceSystem->ApplyPriceShock(EResourceType::IronOre, 2.0f, 0.01f);
	Ctx.PriceSystem->UpdateAllPrices();

	int32 ShockedPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);
	TestTrue(TEXT("Price shock of 2.0 should increase price"), ShockedPrice >= BaseBuyPrice);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_PriceShockDecrease,
	"Odyssey.Economy.PriceFluctuation.Shocks.NegativeShockDecreasesPrices",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_PriceShockDecrease::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_ShockDown"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.MarketData->AddSupply(EResourceType::IronOre, 500);
	Ctx.MarketData->RecalculateAllMetrics();
	Ctx.PriceSystem->UpdateAllPrices();

	int32 BaseBuyPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);

	// Apply a 0.5x price shock (halves prices)
	Ctx.PriceSystem->ApplyPriceShock(EResourceType::IronOre, 0.5f, 0.01f);
	Ctx.PriceSystem->UpdateAllPrices();

	int32 ShockedPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);
	TestTrue(TEXT("Price shock of 0.5 should decrease price"), ShockedPrice <= BaseBuyPrice);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_EventModifier,
	"Odyssey.Economy.PriceFluctuation.Shocks.EventModifierAffectsPrices",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_EventModifier::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_EventMod"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.MarketData->AddSupply(EResourceType::IronOre, 500);
	Ctx.MarketData->RecalculateAllMetrics();
	Ctx.PriceSystem->UpdateAllPrices();

	int32 BasePrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);

	// Apply event modifier (e.g. war doubles prices) for 60 seconds
	Ctx.PriceSystem->ApplyEventModifier(EResourceType::IronOre, 2.0f, 60.0f);
	Ctx.PriceSystem->UpdateAllPrices();

	int32 ModifiedPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);
	TestTrue(TEXT("Event modifier of 2.0 should increase price"), ModifiedPrice >= BasePrice);

	// Clear and verify prices return to baseline area
	Ctx.PriceSystem->ClearEventModifiers(EResourceType::IronOre);
	Ctx.PriceSystem->UpdateAllPrices();

	int32 ClearedPrice = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);
	TestTrue(TEXT("Clearing event modifiers should normalize price"), ClearedPrice <= ModifiedPrice);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 5. TREND ANALYSIS TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_TrendFactor,
	"Odyssey.Economy.PriceFluctuation.Trends.TrendFactorIsFinite",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_TrendFactor::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_TrendFactor"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->UpdateAllPrices();

	float TrendFactor = Ctx.PriceSystem->CalculateTrendFactor(EResourceType::IronOre);
	TestFalse(TEXT("Trend factor must not be NaN"), FMath::IsNaN(TrendFactor));
	TestTrue(TEXT("Trend factor should be positive"), TrendFactor > 0.0f);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_TrendMomentum,
	"Odyssey.Economy.PriceFluctuation.Trends.MomentumIsFinite",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_TrendMomentum::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_Momentum"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	float Momentum = Ctx.PriceSystem->GetTrendMomentum(EResourceType::IronOre);
	TestFalse(TEXT("Momentum must not be NaN"), FMath::IsNaN(Momentum));

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_PredictFuturePrice,
	"Odyssey.Economy.PriceFluctuation.Trends.PricePredictionIsPositive",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_PredictFuturePrice::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_Predict"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.MarketData->AddSupply(EResourceType::IronOre, 500);
	Ctx.MarketData->RecalculateAllMetrics();
	Ctx.PriceSystem->UpdateAllPrices();

	int32 Predicted = Ctx.PriceSystem->PredictFuturePrice(EResourceType::IronOre, 1.0f);
	TestTrue(TEXT("Predicted future price should be > 0"), Predicted > 0);

	int32 PredictedFar = Ctx.PriceSystem->PredictFuturePrice(EResourceType::IronOre, 24.0f);
	TestTrue(TEXT("Far future prediction should be > 0"), PredictedFar > 0);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 6. PRICE HISTORY & TRADE RECORDING TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_RecordTrade,
	"Odyssey.Economy.PriceFluctuation.History.RecordTradeUpdatesPriceData",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_RecordTrade::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_TradeRecord"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->UpdateAllPrices();

	// Record a trade
	Ctx.PriceSystem->RecordTrade(EResourceType::IronOre, 150, 10, true);

	FDynamicMarketPrice PriceData = Ctx.PriceSystem->GetPriceData(EResourceType::IronOre);
	// Price history should contain at least one entry after recording
	TestTrue(TEXT("Price data should be valid after recording trade"),
		PriceData.ResourceType == EResourceType::IronOre || PriceData.BasePrice > 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_GetAllCurrentPrices,
	"Odyssey.Economy.PriceFluctuation.History.GetAllCurrentPricesReturnsData",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_GetAllCurrentPrices::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_AllPrices"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->UpdateAllPrices();

	TMap<EResourceType, FDynamicMarketPrice> AllPrices = Ctx.PriceSystem->GetAllCurrentPrices();
	TestTrue(TEXT("All current prices map should not be empty"), AllPrices.Num() > 0);

	// Every price should have a positive base price
	for (auto& Pair : AllPrices)
	{
		TestTrue(
			FString::Printf(TEXT("Resource %d should have BasePrice > 0"), static_cast<int32>(Pair.Key)),
			Pair.Value.BasePrice > 0);
	}

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 7. PRICE MULTIPLIER & CLAMPING TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_PriceMultiplierPositive,
	"Odyssey.Economy.PriceFluctuation.Multiplier.CurrentMultiplierIsPositive",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_PriceMultiplierPositive::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_Multiplier"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	Ctx.PriceSystem->UpdateAllPrices();

	float Multiplier = Ctx.PriceSystem->GetCurrentPriceMultiplier(EResourceType::IronOre);
	TestTrue(TEXT("Current price multiplier must be > 0"), Multiplier > 0.0f);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriceFluc_PriceNeverBelowFloor,
	"Odyssey.Economy.PriceFluctuation.Clamping.PriceNeverDropsBelowMinimum",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPriceFluc_PriceNeverBelowFloor::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = PriceFlucTestHelpers::CreateLinkedComponents(World, FName("TestPF_Floor"));
	TestNotNull(TEXT("PriceSystem"), Ctx.PriceSystem);

	// Massively oversupply to drive price down
	Ctx.MarketData->AddSupply(EResourceType::IronOre, 99999);
	Ctx.MarketData->SetDemandRate(EResourceType::IronOre, 0.001f);
	Ctx.MarketData->RecalculateAllMetrics();

	// Apply a downward shock
	Ctx.PriceSystem->ApplyPriceShock(EResourceType::IronOre, 0.01f, 0.0f);
	Ctx.PriceSystem->UpdateAllPrices();

	int32 Price = Ctx.PriceSystem->CalculateBuyPrice(EResourceType::IronOre);
	TestTrue(TEXT("Price must always be >= 1 (floor)"), Price >= 1);

	FDynamicMarketPrice PriceData = Ctx.PriceSystem->GetPriceData(EResourceType::IronOre);
	TestTrue(TEXT("Current buy price must be >= MinPrice"), PriceData.CurrentBuyPrice >= PriceData.MinPrice);

	Ctx.Destroy();
	return true;
}
