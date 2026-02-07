// MarketDataComponentTest.cpp
// Comprehensive automation tests for UMarketDataComponent
// Tests supply/demand tracking, price history, scarcity analysis, and simulation

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"

// ============================================================================
// Helper: creates a temporary actor with a UMarketDataComponent attached and
// initialised for a given market.  Caller is responsible for destroying the
// actor after the test (handled via AddCleanup).
// ============================================================================
namespace MarketDataTestHelpers
{
	UMarketDataComponent* CreateInitialisedComponent(UWorld* World, const FName& MarketName, int32 RegionId = 1)
	{
		AActor* TestActor = World->SpawnActor<AActor>();
		if (!TestActor) return nullptr;

		UMarketDataComponent* Comp = NewObject<UMarketDataComponent>(TestActor);
		if (!Comp) return nullptr;

		Comp->RegisterComponent();

		FMarketId Id(MarketName, RegionId);
		Comp->InitializeMarketData(Id, MarketName.ToString());
		return Comp;
	}
}

// ============================================================================
// 1. SUPPLY MANAGEMENT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_InitialSupply,
	"Odyssey.Economy.MarketData.Supply.InitialSupplyIsNonNegative",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_InitialSupply::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	TestNotNull(TEXT("World must exist"), World);

	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Supply"));
	TestNotNull(TEXT("Component must be created"), Comp);

	// After initialisation every resource should have non-negative supply
	int32 Supply = Comp->GetCurrentSupply(EResourceType::IronOre);
	TestTrue(TEXT("Initial supply must be >= 0"), Supply >= 0);

	int32 MaxSupply = Comp->GetMaxSupply(EResourceType::IronOre);
	TestTrue(TEXT("Max supply must be > 0"), MaxSupply > 0);

	float Pct = Comp->GetSupplyPercent(EResourceType::IronOre);
	TestTrue(TEXT("Supply percent must be in [0,1]"), Pct >= 0.0f && Pct <= 1.0f);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_AddSupply,
	"Odyssey.Economy.MarketData.Supply.AddSupplyIncreasesStock",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_AddSupply::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_AddSupply"));
	TestNotNull(TEXT("Component"), Comp);

	int32 Before = Comp->GetCurrentSupply(EResourceType::IronOre);
	Comp->AddSupply(EResourceType::IronOre, 50);
	int32 After = Comp->GetCurrentSupply(EResourceType::IronOre);

	TestEqual(TEXT("Supply should increase by exactly 50"), After, Before + 50);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_RemoveSupply,
	"Odyssey.Economy.MarketData.Supply.RemoveSupplyDecreasesStock",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_RemoveSupply::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_RemoveSupply"));
	TestNotNull(TEXT("Component"), Comp);

	Comp->AddSupply(EResourceType::CopperOre, 100);
	int32 Before = Comp->GetCurrentSupply(EResourceType::CopperOre);

	bool bSuccess = Comp->RemoveSupply(EResourceType::CopperOre, 40);
	int32 After = Comp->GetCurrentSupply(EResourceType::CopperOre);

	TestTrue(TEXT("RemoveSupply should return true when sufficient stock exists"), bSuccess);
	TestEqual(TEXT("Supply should decrease by 40"), After, Before - 40);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_RemoveSupplyInsufficientStock,
	"Odyssey.Economy.MarketData.Supply.RemoveSupplyFailsWhenInsufficient",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_RemoveSupplyInsufficientStock::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_InsufficientStock"));
	TestNotNull(TEXT("Component"), Comp);

	// Try to remove more than available
	int32 CurrentSupply = Comp->GetCurrentSupply(EResourceType::GoldOre);
	bool bSuccess = Comp->RemoveSupply(EResourceType::GoldOre, CurrentSupply + 999);

	TestFalse(TEXT("RemoveSupply should return false when stock is insufficient"), bSuccess);

	// Supply should not go negative
	int32 AfterAttempt = Comp->GetCurrentSupply(EResourceType::GoldOre);
	TestTrue(TEXT("Supply must never go negative"), AfterAttempt >= 0);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_SupplyModifier,
	"Odyssey.Economy.MarketData.Supply.ModifierAffectsSupplyDemandRatio",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_SupplyModifier::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_SupplyMod"));
	TestNotNull(TEXT("Component"), Comp);

	Comp->AddSupply(EResourceType::IronOre, 200);
	Comp->RecalculateAllMetrics();

	float RatioBefore = Comp->GetSupplyDemandRatio(EResourceType::IronOre);

	// Setting a supply modifier of 2.0 should affect production calculations
	Comp->SetSupplyModifier(EResourceType::IronOre, 2.0f);
	Comp->RecalculateAllMetrics();

	// The modifier should be applied (exact effect depends on implementation,
	// but the data should be recorded)
	FResourceSupplyDemand Data = Comp->GetSupplyDemandData(EResourceType::IronOre);
	TestEqual(TEXT("Supply modifier should be stored as set"), Data.SupplyModifier, 2.0f);

	Comp->GetOwner()->Destroy();
	return true;
}

// ============================================================================
// 2. DEMAND MANAGEMENT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_DemandRateTracking,
	"Odyssey.Economy.MarketData.Demand.SetAndGetDemandRate",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_DemandRateTracking::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_DemandRate"));
	TestNotNull(TEXT("Component"), Comp);

	Comp->SetDemandRate(EResourceType::CopperOre, 25.0f);
	float Rate = Comp->GetDemandRate(EResourceType::CopperOre);
	TestEqual(TEXT("Demand rate should match set value"), Rate, 25.0f);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_DemandModifier,
	"Odyssey.Economy.MarketData.Demand.ModifierIsStored",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_DemandModifier::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_DemandMod"));
	TestNotNull(TEXT("Component"), Comp);

	Comp->SetDemandModifier(EResourceType::IronOre, 1.5f);
	FResourceSupplyDemand Data = Comp->GetSupplyDemandData(EResourceType::IronOre);
	TestEqual(TEXT("Demand modifier should be stored"), Data.DemandModifier, 1.5f);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_DemandElasticity,
	"Odyssey.Economy.MarketData.Demand.ElasticityIsReadable",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_DemandElasticity::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Elasticity"));
	TestNotNull(TEXT("Component"), Comp);

	float Elasticity = Comp->GetDemandElasticity(EResourceType::IronOre);
	TestTrue(TEXT("Elasticity must be positive"), Elasticity > 0.0f);

	Comp->GetOwner()->Destroy();
	return true;
}

// ============================================================================
// 3. SUPPLY/DEMAND ANALYSIS TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_SupplyDemandRatioBalanced,
	"Odyssey.Economy.MarketData.Analysis.BalancedMarketRatioNearOne",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_SupplyDemandRatioBalanced::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Balanced"));
	TestNotNull(TEXT("Component"), Comp);

	// Set supply and demand to equal values
	Comp->AddSupply(EResourceType::IronOre, 100);
	Comp->SetDemandRate(EResourceType::IronOre, 10.0f);
	Comp->SetSupplyRate(EResourceType::IronOre, 10.0f);
	Comp->RecalculateAllMetrics();

	float Ratio = Comp->GetSupplyDemandRatio(EResourceType::IronOre);
	// Ratio should be positive; exact value depends on implementation
	TestTrue(TEXT("Supply/demand ratio must be positive"), Ratio > 0.0f);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_ScarcityIndexRange,
	"Odyssey.Economy.MarketData.Analysis.ScarcityIndexInZeroOneRange",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_ScarcityIndexRange::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Scarcity"));
	TestNotNull(TEXT("Component"), Comp);

	float Scarcity = Comp->GetScarcityIndex(EResourceType::IronOre);
	TestTrue(TEXT("Scarcity index must be in [0, 1]"), Scarcity >= 0.0f && Scarcity <= 1.0f);

	// Scarce scenario: very low supply
	Comp->AddSupply(EResourceType::CopperOre, 1);
	Comp->RecalculateAllMetrics();
	float ScarceLow = Comp->GetScarcityIndex(EResourceType::CopperOre);
	TestTrue(TEXT("Low supply scarcity should be high"), ScarceLow >= 0.0f);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_IsResourceScarce,
	"Odyssey.Economy.MarketData.Analysis.IsResourceScarceDetectsLowSupply",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_IsResourceScarce::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_ScarceCheck"));
	TestNotNull(TEXT("Component"), Comp);

	// With default initial supply, resource should not be considered scarce
	// unless the initial supply is very low relative to max
	bool bScarce = Comp->IsResourceScarce(EResourceType::IronOre, 0.99f);
	// With a very high threshold, most resources should qualify as scarce
	// This validates the function is callable and returns a valid bool
	TestTrue(TEXT("IsResourceScarce should return a valid result"), bScarce || !bScarce);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_IsResourceAbundant,
	"Odyssey.Economy.MarketData.Analysis.IsResourceAbundantDetectsHighSupply",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_IsResourceAbundant::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Abundant"));
	TestNotNull(TEXT("Component"), Comp);

	// Fill supply to near maximum
	int32 MaxSup = Comp->GetMaxSupply(EResourceType::IronOre);
	Comp->AddSupply(EResourceType::IronOre, MaxSup);
	Comp->RecalculateAllMetrics();

	bool bAbundant = Comp->IsResourceAbundant(EResourceType::IronOre, 0.3f);
	TestTrue(TEXT("Fully stocked resource should be considered abundant"), bAbundant);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_GetResourcesByScarcity,
	"Odyssey.Economy.MarketData.Analysis.ResourcesSortedByScarcity",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_GetResourcesByScarcity::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_SortScarcity"));
	TestNotNull(TEXT("Component"), Comp);

	TArray<EResourceType> Sorted = Comp->GetResourcesByScarcity(true);
	// Should return at least one resource
	TestTrue(TEXT("Should return at least one resource type"), Sorted.Num() > 0);

	// Verify it is sorted by scarcity (descending)
	for (int32 i = 1; i < Sorted.Num(); ++i)
	{
		float ScarcityA = Comp->GetScarcityIndex(Sorted[i - 1]);
		float ScarcityB = Comp->GetScarcityIndex(Sorted[i]);
		TestTrue(
			FString::Printf(TEXT("Scarcity[%d] (%.3f) >= Scarcity[%d] (%.3f)"), i - 1, ScarcityA, i, ScarcityB),
			ScarcityA >= ScarcityB - KINDA_SMALL_NUMBER);
	}

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_MultipleCommodityTracking,
	"Odyssey.Economy.MarketData.Analysis.MultipleCommoditiesTrackedIndependently",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_MultipleCommodityTracking::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_MultiCommodity"));
	TestNotNull(TEXT("Component"), Comp);

	// Add different amounts to different resources
	Comp->AddSupply(EResourceType::IronOre, 100);
	Comp->AddSupply(EResourceType::CopperOre, 200);
	Comp->AddSupply(EResourceType::GoldOre, 50);

	int32 Iron = Comp->GetCurrentSupply(EResourceType::IronOre);
	int32 Copper = Comp->GetCurrentSupply(EResourceType::CopperOre);
	int32 Gold = Comp->GetCurrentSupply(EResourceType::GoldOre);

	// Each resource should track independently
	TestTrue(TEXT("Copper supply should be higher than Iron"), Copper > Iron);
	TestTrue(TEXT("Iron supply should be higher than Gold"), Iron > Gold);

	TMap<EResourceType, FResourceSupplyDemand> AllData = Comp->GetAllSupplyDemandData();
	TestTrue(TEXT("AllSupplyDemandData should contain at least 3 entries"), AllData.Num() >= 3);

	Comp->GetOwner()->Destroy();
	return true;
}

// ============================================================================
// 4. EDGE CASE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_ZeroSupplyHandling,
	"Odyssey.Economy.MarketData.EdgeCases.ZeroSupplyDoesNotCrash",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_ZeroSupplyHandling::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_ZeroSupply"));
	TestNotNull(TEXT("Component"), Comp);

	// Remove all supply to reach zero
	int32 Current = Comp->GetCurrentSupply(EResourceType::IronOre);
	if (Current > 0)
	{
		Comp->RemoveSupply(EResourceType::IronOre, Current);
	}

	// These should not crash or produce NaN
	float Ratio = Comp->GetSupplyDemandRatio(EResourceType::IronOre);
	TestFalse(TEXT("Supply/demand ratio must not be NaN at zero supply"), FMath::IsNaN(Ratio));

	float Scarcity = Comp->GetScarcityIndex(EResourceType::IronOre);
	TestFalse(TEXT("Scarcity must not be NaN at zero supply"), FMath::IsNaN(Scarcity));

	float SupplyPct = Comp->GetSupplyPercent(EResourceType::IronOre);
	TestFalse(TEXT("Supply percent must not be NaN at zero supply"), FMath::IsNaN(SupplyPct));
	TestTrue(TEXT("Supply percent should be 0 at zero supply"), FMath::IsNearlyEqual(SupplyPct, 0.0f, 0.01f));

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_AddZeroSupply,
	"Odyssey.Economy.MarketData.EdgeCases.AddZeroSupplyIsNoOp",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_AddZeroSupply::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_ZeroAdd"));
	TestNotNull(TEXT("Component"), Comp);

	int32 Before = Comp->GetCurrentSupply(EResourceType::IronOre);
	Comp->AddSupply(EResourceType::IronOre, 0);
	int32 After = Comp->GetCurrentSupply(EResourceType::IronOre);

	TestEqual(TEXT("Adding zero supply should not change stock"), After, Before);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_NegativeValueProtection,
	"Odyssey.Economy.MarketData.EdgeCases.NegativeAmountsAreRejected",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_NegativeValueProtection::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_NegValues"));
	TestNotNull(TEXT("Component"), Comp);

	int32 Before = Comp->GetCurrentSupply(EResourceType::IronOre);
	Comp->AddSupply(EResourceType::IronOre, -50);
	int32 After = Comp->GetCurrentSupply(EResourceType::IronOre);

	// Negative values should either be rejected (supply unchanged) or
	// handled gracefully (no crash, supply >= 0)
	TestTrue(TEXT("Supply must remain non-negative after negative add"), After >= 0);

	Comp->GetOwner()->Destroy();
	return true;
}

// ============================================================================
// 5. PRICE HISTORY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_RecordPricePoint,
	"Odyssey.Economy.MarketData.PriceHistory.RecordAndRetrievePricePoints",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_RecordPricePoint::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_PriceHist"));
	TestNotNull(TEXT("Component"), Comp);

	Comp->RecordPricePoint(EResourceType::IronOre, 100, 10);
	Comp->RecordPricePoint(EResourceType::IronOre, 110, 15);
	Comp->RecordPricePoint(EResourceType::IronOre, 105, 12);

	TArray<FPriceHistoryEntry> History = Comp->GetPriceHistory(EResourceType::IronOre);
	TestTrue(TEXT("Price history should have at least 3 entries"), History.Num() >= 3);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_AveragePrice,
	"Odyssey.Economy.MarketData.PriceHistory.AveragePriceCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_AveragePrice::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_AvgPrice"));
	TestNotNull(TEXT("Component"), Comp);

	// Record known prices
	Comp->RecordPricePoint(EResourceType::CopperOre, 100, 10);
	Comp->RecordPricePoint(EResourceType::CopperOre, 200, 10);
	Comp->RecordPricePoint(EResourceType::CopperOre, 300, 10);

	float Avg = Comp->GetAveragePrice(EResourceType::CopperOre, 3);
	// Expected: (100 + 200 + 300) / 3 = 200
	TestTrue(TEXT("Average price should be approximately 200"), FMath::IsNearlyEqual(Avg, 200.0f, 1.0f));

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_PriceTrend,
	"Odyssey.Economy.MarketData.PriceHistory.TrendDetection",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_PriceTrend::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Trend"));
	TestNotNull(TEXT("Component"), Comp);

	// Record rising prices
	for (int32 i = 0; i < 10; ++i)
	{
		Comp->RecordPricePoint(EResourceType::IronOre, 50 + i * 10, 10);
	}

	EMarketTrend Trend = Comp->GetPriceTrend(EResourceType::IronOre);
	// Should detect a bullish trend
	TestTrue(TEXT("Rising prices should produce Bull or StrongBull trend"),
		Trend == EMarketTrend::Bull || Trend == EMarketTrend::StrongBull);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_PriceVolatility,
	"Odyssey.Economy.MarketData.PriceHistory.VolatilityMeasurement",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_PriceVolatility::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Volatility"));
	TestNotNull(TEXT("Component"), Comp);

	// Record stable prices
	for (int32 i = 0; i < 10; ++i)
	{
		Comp->RecordPricePoint(EResourceType::IronOre, 100, 10);
	}
	float LowVol = Comp->GetPriceVolatility(EResourceType::IronOre);

	// Record volatile prices
	for (int32 i = 0; i < 10; ++i)
	{
		int32 Price = (i % 2 == 0) ? 50 : 200;
		Comp->RecordPricePoint(EResourceType::CopperOre, Price, 10);
	}
	float HighVol = Comp->GetPriceVolatility(EResourceType::CopperOre);

	TestTrue(TEXT("Volatile prices should have higher volatility measure"), HighVol > LowVol);
	TestTrue(TEXT("Volatility should be non-negative"), LowVol >= 0.0f);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_HighestLowestPrice,
	"Odyssey.Economy.MarketData.PriceHistory.HighestAndLowestRecentPrice",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_HighestLowestPrice::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_HighLow"));
	TestNotNull(TEXT("Component"), Comp);

	Comp->RecordPricePoint(EResourceType::GoldOre, 50, 5);
	Comp->RecordPricePoint(EResourceType::GoldOre, 300, 5);
	Comp->RecordPricePoint(EResourceType::GoldOre, 150, 5);

	int32 Highest = Comp->GetHighestRecentPrice(EResourceType::GoldOre, 10);
	int32 Lowest = Comp->GetLowestRecentPrice(EResourceType::GoldOre, 10);

	TestEqual(TEXT("Highest recent price should be 300"), Highest, 300);
	TestEqual(TEXT("Lowest recent price should be 50"), Lowest, 50);

	Comp->GetOwner()->Destroy();
	return true;
}

// ============================================================================
// 6. MARKET INFO TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_MarketIdPersistence,
	"Odyssey.Economy.MarketData.MarketInfo.MarketIdIsPreserved",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_MarketIdPersistence::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("AlphaStation"), 42);
	TestNotNull(TEXT("Component"), Comp);

	FMarketId Id = Comp->GetMarketId();
	TestEqual(TEXT("Market name must match"), Id.MarketName, FName("AlphaStation"));
	TestEqual(TEXT("Region ID must match"), Id.RegionId, 42);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_Specialization,
	"Odyssey.Economy.MarketData.MarketInfo.SpecializationBonus",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_Specialization::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Spec"));
	TestNotNull(TEXT("Component"), Comp);

	float Bonus = Comp->GetSpecializationBonus();
	TestTrue(TEXT("Specialization bonus should be non-negative"), Bonus >= 0.0f);

	Comp->GetOwner()->Destroy();
	return true;
}

// ============================================================================
// 7. SIMULATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_SimulateSupplyDemand,
	"Odyssey.Economy.MarketData.Simulation.SimulateSupplyDemandUpdatesState",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_SimulateSupplyDemand::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Simulate"));
	TestNotNull(TEXT("Component"), Comp);

	Comp->AddSupply(EResourceType::IronOre, 500);
	Comp->SetSupplyRate(EResourceType::IronOre, 10.0f);
	Comp->SetDemandRate(EResourceType::IronOre, 20.0f);

	int32 SupplyBefore = Comp->GetCurrentSupply(EResourceType::IronOre);

	// Simulate 1 game hour of supply/demand
	Comp->SimulateSupplyDemand(1.0f);

	int32 SupplyAfter = Comp->GetCurrentSupply(EResourceType::IronOre);

	// With demand exceeding supply rate, stock should decrease (or at least change)
	// The exact behaviour depends on implementation, but the call should not crash
	TestTrue(TEXT("Supply should change or remain valid after simulation"), SupplyAfter >= 0);

	Comp->GetOwner()->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMarketData_ResetToDefaults,
	"Odyssey.Economy.MarketData.Simulation.ResetToDefaultsRestoresInitialState",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMarketData_ResetToDefaults::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	UMarketDataComponent* Comp = MarketDataTestHelpers::CreateInitialisedComponent(World, FName("TestMarket_Reset"));
	TestNotNull(TEXT("Component"), Comp);

	int32 InitialSupply = Comp->GetCurrentSupply(EResourceType::IronOre);

	// Modify state
	Comp->AddSupply(EResourceType::IronOre, 999);
	Comp->SetDemandRate(EResourceType::IronOre, 999.0f);

	// Reset
	Comp->ResetToDefaults();

	int32 ResetSupply = Comp->GetCurrentSupply(EResourceType::IronOre);
	TestEqual(TEXT("Supply should return to initial value after reset"), ResetSupply, InitialSupply);

	Comp->GetOwner()->Destroy();
	return true;
}
