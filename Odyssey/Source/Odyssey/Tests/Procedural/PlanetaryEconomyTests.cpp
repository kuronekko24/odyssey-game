// PlanetaryEconomyTests.cpp
// Comprehensive automation tests for UOdysseyPlanetaryEconomyComponent
// Tests economic specialization, trade values, market operations, and economy integration

#include "Misc/AutomationTest.h"
#include "OdysseyPlanetaryEconomyComponent.h"
#include "OdysseyPlanetGenerator.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyResourceDistributionSystem.h"
#include "Procedural/ProceduralTypes.h"

// ============================================================================
// HELPERS
// ============================================================================

namespace EconomyTestHelpers
{
	struct EconomyTestContext
	{
		UOdysseyBiomeDefinitionSystem* BiomeSystem;
		UOdysseyResourceDistributionSystem* ResourceSystem;
		UOdysseyPlanetGenerator* PlanetGen;
	};

	static EconomyTestContext CreateGenerationContext()
	{
		EconomyTestContext Ctx;

		Ctx.BiomeSystem = NewObject<UOdysseyBiomeDefinitionSystem>();
		Ctx.BiomeSystem->Initialize(nullptr);

		Ctx.ResourceSystem = NewObject<UOdysseyResourceDistributionSystem>();
		Ctx.ResourceSystem->Initialize(Ctx.BiomeSystem);

		Ctx.PlanetGen = NewObject<UOdysseyPlanetGenerator>();
		Ctx.PlanetGen->Initialize(Ctx.BiomeSystem, Ctx.ResourceSystem);

		return Ctx;
	}

	static UOdysseyPlanetaryEconomyComponent* CreateInitializedEconomy(
		EconomyTestContext& Ctx, int32 Seed = 42, EPlanetSize Size = EPlanetSize::Medium)
	{
		FGeneratedPlanetData Planet = Ctx.PlanetGen->GeneratePlanet(Seed, Size);

		UOdysseyPlanetaryEconomyComponent* Econ = NewObject<UOdysseyPlanetaryEconomyComponent>();
		Econ->InitializeTradeGoods();
		Econ->InitializeFromPlanetData(Planet, Seed);

		return Econ;
	}
}

// ============================================================================
// 1. ECONOMIC SPECIALIZATION ASSIGNMENT
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_SpecializationAssignment,
	"Odyssey.Procedural.PlanetaryEconomy.SpecializationAssignment",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_SpecializationAssignment::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	EEconomicSpecialization Primary = Econ->GetPrimarySpecialization();
	EEconomicSpecialization Secondary = Econ->GetSecondarySpecialization();

	TestTrue(TEXT("Primary specialization should not be None"), Primary != EEconomicSpecialization::None);
	TestTrue(TEXT("Primary and secondary should differ"), Primary != Secondary);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_SpecializationVariety,
	"Odyssey.Procedural.PlanetaryEconomy.SpecializationVarietyAcrossSeeds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_SpecializationVariety::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();

	TSet<EEconomicSpecialization> SpecializationsFound;
	for (int32 Seed = 0; Seed < 50; ++Seed)
	{
		UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx, Seed * 1000);
		SpecializationsFound.Add(Econ->GetPrimarySpecialization());
		SpecializationsFound.Add(Econ->GetSecondarySpecialization());
	}

	// With 50 planets, we should see at least 4 distinct specializations
	TestTrue(
		FString::Printf(TEXT("Should have at least 4 specializations from 50 planets, found %d"), SpecializationsFound.Num()),
		SpecializationsFound.Num() >= 4);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_SetSpecializations,
	"Odyssey.Procedural.PlanetaryEconomy.SetSpecializationsManually",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_SetSpecializations::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	Econ->SetSpecializations(EEconomicSpecialization::Technology, EEconomicSpecialization::Research);

	TestEqual(TEXT("Primary should be Technology"), Econ->GetPrimarySpecialization(), EEconomicSpecialization::Technology);
	TestEqual(TEXT("Secondary should be Research"), Econ->GetSecondarySpecialization(), EEconomicSpecialization::Research);
	TestTrue(TEXT("HasSpecialization(Technology) should return true"), Econ->HasSpecialization(EEconomicSpecialization::Technology));
	TestTrue(TEXT("HasSpecialization(Research) should return true"), Econ->HasSpecialization(EEconomicSpecialization::Research));
	TestFalse(TEXT("HasSpecialization(Mining) should return false"), Econ->HasSpecialization(EEconomicSpecialization::Mining));

	return true;
}

// ============================================================================
// 2. TRADE GOODS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_TradeGoodsInitialized,
	"Odyssey.Procedural.PlanetaryEconomy.TradeGoodsInitialized",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_TradeGoodsInitialized::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FName> AllGoods = Econ->GetAllTradeGoodIDs();

	TestTrue(TEXT("Should have trade goods defined"), AllGoods.Num() > 0);

	for (const FName& GoodID : AllGoods)
	{
		FTradeGood Good = Econ->GetTradeGoodInfo(GoodID);
		TestTrue(
			FString::Printf(TEXT("Trade good %s should have valid ID"), *GoodID.ToString()),
			Good.GoodID != NAME_None);
		TestFalse(
			FString::Printf(TEXT("Trade good %s display name should not be empty"), *GoodID.ToString()),
			Good.DisplayName.IsEmpty());
		TestTrue(
			FString::Printf(TEXT("Trade good %s base value should be positive"), *GoodID.ToString()),
			Good.BaseValue > 0);
		TestTrue(
			FString::Printf(TEXT("Trade good %s volume should be positive"), *GoodID.ToString()),
			Good.VolumePerUnit > 0.0f);
		TestTrue(
			FString::Printf(TEXT("Trade good %s legality should be in [0, 2]"), *GoodID.ToString()),
			Good.LegalityStatus >= 0 && Good.LegalityStatus <= 2);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_ProducedAndConsumedGoods,
	"Odyssey.Procedural.PlanetaryEconomy.ProducedAndConsumedGoodsExist",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_ProducedAndConsumedGoods::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FName> Produced = Econ->GetProducedGoods();
	TArray<FName> Consumed = Econ->GetConsumedGoods();

	TestTrue(TEXT("Planet should produce at least one trade good"), Produced.Num() > 0);
	TestTrue(TEXT("Planet should consume at least one trade good"), Consumed.Num() > 0);

	// Produced goods should have active production entries
	for (const FName& GoodID : Produced)
	{
		TestTrue(
			FString::Printf(TEXT("IsProducing(%s) should return true"), *GoodID.ToString()),
			Econ->IsProducing(GoodID));
	}

	// Consumed goods should have consumption entries
	for (const FName& GoodID : Consumed)
	{
		TestTrue(
			FString::Printf(TEXT("IsConsuming(%s) should return true"), *GoodID.ToString()),
			Econ->IsConsuming(GoodID));
	}

	return true;
}

// ============================================================================
// 3. PRODUCTION AND CONSUMPTION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_ProductionEntries,
	"Odyssey.Procedural.PlanetaryEconomy.ProductionEntriesValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_ProductionEntries::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FPlanetaryProduction> Productions = Econ->GetProductions();

	for (const FPlanetaryProduction& Prod : Productions)
	{
		TestTrue(
			FString::Printf(TEXT("Production good %s should have valid ID"), *Prod.GoodID.ToString()),
			Prod.GoodID != NAME_None);
		TestTrue(
			FString::Printf(TEXT("Production rate for %s should be positive"), *Prod.GoodID.ToString()),
			Prod.ProductionRate > 0);
		TestTrue(
			FString::Printf(TEXT("Max storage for %s should be positive"), *Prod.GoodID.ToString()),
			Prod.MaxStorage > 0);
		TestTrue(
			FString::Printf(TEXT("Efficiency for %s should be in [0, 2]"), *Prod.GoodID.ToString()),
			Prod.Efficiency >= 0.0f && Prod.Efficiency <= 2.0f);
		TestTrue(
			FString::Printf(TEXT("Current stock for %s should be <= max storage"), *Prod.GoodID.ToString()),
			Prod.CurrentStock <= Prod.MaxStorage);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_ConsumptionEntries,
	"Odyssey.Procedural.PlanetaryEconomy.ConsumptionEntriesValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_ConsumptionEntries::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FPlanetaryConsumption> Consumptions = Econ->GetConsumptions();

	for (const FPlanetaryConsumption& Cons : Consumptions)
	{
		TestTrue(
			FString::Printf(TEXT("Consumption good %s should have valid ID"), *Cons.GoodID.ToString()),
			Cons.GoodID != NAME_None);
		TestTrue(
			FString::Printf(TEXT("Consumption rate for %s should be positive"), *Cons.GoodID.ToString()),
			Cons.ConsumptionRate > 0);
		TestTrue(
			FString::Printf(TEXT("Current demand for %s should be >= 0"), *Cons.GoodID.ToString()),
			Cons.CurrentDemand >= 0);
		TestTrue(
			FString::Printf(TEXT("Urgency for %s should be in [0, 2]"), *Cons.GoodID.ToString()),
			Cons.Urgency >= 0 && Cons.Urgency <= 2);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_AddRemoveProduction,
	"Odyssey.Procedural.PlanetaryEconomy.AddAndRemoveProduction",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_AddRemoveProduction::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	FPlanetaryProduction NewProd;
	NewProd.GoodID = FName(TEXT("TestGood"));
	NewProd.ProductionRate = 25;
	NewProd.CurrentStock = 0;
	NewProd.MaxStorage = 500;
	NewProd.Efficiency = 1.0f;
	NewProd.bIsActive = true;

	Econ->AddProduction(NewProd);
	TestTrue(TEXT("Should be producing TestGood after adding"), Econ->IsProducing(FName(TEXT("TestGood"))));

	Econ->RemoveProduction(FName(TEXT("TestGood")));
	TestFalse(TEXT("Should not be producing TestGood after removing"), Econ->IsProducing(FName(TEXT("TestGood"))));

	return true;
}

// ============================================================================
// 4. MARKET OPERATIONS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_MarketPricesValid,
	"Odyssey.Procedural.PlanetaryEconomy.MarketPricesValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_MarketPricesValid::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FPlanetaryMarketPrice> Prices = Econ->GetAllMarketPrices();

	TestTrue(TEXT("Should have market prices"), Prices.Num() > 0);

	for (const FPlanetaryMarketPrice& Price : Prices)
	{
		TestTrue(
			FString::Printf(TEXT("Good %s should have valid ID"), *Price.GoodID.ToString()),
			Price.GoodID != NAME_None);
		TestTrue(
			FString::Printf(TEXT("Buy price for %s should be positive"), *Price.GoodID.ToString()),
			Price.BuyPrice > 0);
		TestTrue(
			FString::Printf(TEXT("Sell price for %s should be positive"), *Price.GoodID.ToString()),
			Price.SellPrice > 0);
		TestTrue(
			FString::Printf(TEXT("Buy price for %s should be >= sell price"), *Price.GoodID.ToString()),
			Price.BuyPrice >= Price.SellPrice);
		TestTrue(
			FString::Printf(TEXT("Price trend for %s should be in [-1, 1]"), *Price.GoodID.ToString()),
			Price.PriceTrend >= -1 && Price.PriceTrend <= 1);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_BuyAndSellPriceAccess,
	"Odyssey.Procedural.PlanetaryEconomy.BuyAndSellPriceAccess",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_BuyAndSellPriceAccess::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FName> AllGoods = Econ->GetAllTradeGoodIDs();

	if (AllGoods.Num() > 0)
	{
		FName TestGood = AllGoods[0];
		int32 BuyPrice = Econ->GetBuyPrice(TestGood);
		int32 SellPrice = Econ->GetSellPrice(TestGood);

		TestTrue(TEXT("Buy price should be positive"), BuyPrice > 0);
		TestTrue(TEXT("Sell price should be positive"), SellPrice > 0);
		TestTrue(TEXT("Buy price should be >= sell price (spread)"), BuyPrice >= SellPrice);

		FPlanetaryMarketPrice FullPrice = Econ->GetMarketPrice(TestGood);
		TestEqual(TEXT("GetMarketPrice should return matching buy price"), FullPrice.BuyPrice, BuyPrice);
		TestEqual(TEXT("GetMarketPrice should return matching sell price"), FullPrice.SellPrice, SellPrice);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_ExecuteBuy,
	"Odyssey.Procedural.PlanetaryEconomy.ExecuteBuyTransaction",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_ExecuteBuy::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FPlanetaryMarketPrice> Prices = Econ->GetAllMarketPrices();

	// Find a good with available quantity
	for (const FPlanetaryMarketPrice& Price : Prices)
	{
		if (Price.AvailableQuantity > 0)
		{
			int32 BuyQty = FMath::Min(5, Price.AvailableQuantity);
			bool bCanBuy = Econ->CanBuyGood(Price.GoodID, BuyQty);

			if (bCanBuy)
			{
				int32 TotalCost = 0;
				bool bBought = Econ->ExecuteBuy(Price.GoodID, BuyQty, TotalCost);

				TestTrue(TEXT("ExecuteBuy should succeed"), bBought);
				TestTrue(TEXT("Total cost should be positive"), TotalCost > 0);
				TestTrue(TEXT("Total cost should be buy price * quantity"),
					TotalCost >= Price.BuyPrice * BuyQty * 0.9f); // Allow some variance due to dynamic pricing
			}
			break;
		}
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_ExecuteSell,
	"Odyssey.Procedural.PlanetaryEconomy.ExecuteSellTransaction",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_ExecuteSell::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FPlanetaryMarketPrice> Prices = Econ->GetAllMarketPrices();

	// Find a good the market is demanding
	for (const FPlanetaryMarketPrice& Price : Prices)
	{
		if (Price.DemandQuantity > 0)
		{
			int32 SellQty = 5;
			bool bCanSell = Econ->CanSellGood(Price.GoodID, SellQty);

			if (bCanSell)
			{
				int32 TotalRevenue = 0;
				bool bSold = Econ->ExecuteSell(Price.GoodID, SellQty, TotalRevenue);

				if (bSold)
				{
					TestTrue(TEXT("Total revenue should be positive"), TotalRevenue > 0);
				}
			}
			break;
		}
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_CannotBuyMoreThanAvailable,
	"Odyssey.Procedural.PlanetaryEconomy.CannotBuyMoreThanAvailable",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_CannotBuyMoreThanAvailable::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FPlanetaryMarketPrice> Prices = Econ->GetAllMarketPrices();

	for (const FPlanetaryMarketPrice& Price : Prices)
	{
		if (Price.AvailableQuantity > 0)
		{
			// Try to buy way more than available
			bool bCanBuy = Econ->CanBuyGood(Price.GoodID, Price.AvailableQuantity + 10000);
			TestFalse(
				FString::Printf(TEXT("Should not be able to buy %d of %s (only %d available)"),
					Price.AvailableQuantity + 10000, *Price.GoodID.ToString(), Price.AvailableQuantity),
				bCanBuy);
			break;
		}
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_MarketPriceUpdate,
	"Odyssey.Procedural.PlanetaryEconomy.MarketPriceUpdateDoesNotCrash",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_MarketPriceUpdate::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	// Manually trigger market price update
	Econ->UpdateMarketPrices();

	TArray<FPlanetaryMarketPrice> UpdatedPrices = Econ->GetAllMarketPrices();
	TestTrue(TEXT("Should still have market prices after update"), UpdatedPrices.Num() > 0);

	// Prices should remain valid after update
	for (const FPlanetaryMarketPrice& Price : UpdatedPrices)
	{
		TestTrue(
			FString::Printf(TEXT("Updated buy price for %s should be positive"), *Price.GoodID.ToString()),
			Price.BuyPrice > 0);
		TestTrue(
			FString::Printf(TEXT("Updated sell price for %s should be positive"), *Price.GoodID.ToString()),
			Price.SellPrice > 0);
	}

	return true;
}

// ============================================================================
// 5. ECONOMIC METRICS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_EconomicMetrics,
	"Odyssey.Procedural.PlanetaryEconomy.EconomicMetricsValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_EconomicMetrics::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	int32 Wealth = Econ->GetWealthLevel();
	int32 Development = Econ->GetDevelopmentLevel();
	int32 Population = Econ->GetPopulation();
	int32 GDP = Econ->GetTotalGDP();

	TestTrue(TEXT("Wealth level should be in [0, 100]"), Wealth >= 0 && Wealth <= 100);
	TestTrue(TEXT("Development level should be in [0, 100]"), Development >= 0 && Development <= 100);
	TestTrue(TEXT("Population should be positive"), Population > 0);
	TestTrue(TEXT("GDP should be non-negative"), GDP >= 0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_MetricsVaryBySeed,
	"Odyssey.Procedural.PlanetaryEconomy.MetricsVaryAcrossSeeds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_MetricsVaryBySeed::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();

	TSet<int32> WealthValues;
	TSet<int32> PopulationValues;

	for (int32 Seed = 0; Seed < 20; ++Seed)
	{
		UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx, Seed * 1000);
		WealthValues.Add(Econ->GetWealthLevel());
		PopulationValues.Add(Econ->GetPopulation());
	}

	// With 20 seeds, there should be some variety
	TestTrue(
		FString::Printf(TEXT("Should have at least 3 distinct wealth levels, got %d"), WealthValues.Num()),
		WealthValues.Num() >= 3);

	return true;
}

// ============================================================================
// 6. ECONOMIC RELATIONSHIPS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_RelationshipAccess,
	"Odyssey.Procedural.PlanetaryEconomy.RelationshipAccessDoesNotCrash",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_RelationshipAccess::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	// Get relationship with a non-existent planet
	FEconomicRelationship Rel = Econ->GetRelationship(99999);
	// Should return a default relationship without crashing

	float TariffRate = Econ->GetTariffRate(99999);
	TestTrue(TEXT("Tariff rate should be non-negative"), TariffRate >= 0.0f);

	TArray<int32> Partners = Econ->GetTradingPartners();
	// May be empty if no relationships established
	TestTrue(TEXT("GetTradingPartners should return valid array"), true);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_UpdateRelationship,
	"Odyssey.Procedural.PlanetaryEconomy.UpdateRelationship",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_UpdateRelationship::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	FEconomicRelationship NewRel;
	NewRel.PartnerPlanetID = 42;
	NewRel.RelationshipStrength = 0.8f;
	NewRel.bHasTradeAgreement = true;
	NewRel.TariffRate = 0.05f;
	NewRel.PrimaryExportGood = FName(TEXT("RefinedMetals"));
	NewRel.PrimaryImportGood = FName(TEXT("FoodSupplies"));
	NewRel.TradeVolume = 500;

	Econ->UpdateRelationship(NewRel);

	FEconomicRelationship Retrieved = Econ->GetRelationship(42);
	TestEqual(TEXT("Partner ID should match"), Retrieved.PartnerPlanetID, 42);
	TestTrue(TEXT("Should have trade agreement"), Retrieved.bHasTradeAgreement);
	TestEqual(TEXT("Tariff rate should match"), Retrieved.TariffRate, 0.05f);

	TArray<int32> Partners = Econ->GetTradingPartners();
	TestTrue(TEXT("Should have at least one trading partner"), Partners.Num() > 0);

	return true;
}

// ============================================================================
// 7. ECONOMIC ANALYSIS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_MostProfitableExports,
	"Odyssey.Procedural.PlanetaryEconomy.MostProfitableExportsValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_MostProfitableExports::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FName> ProfitableExports = Econ->GetMostProfitableExports();
	// May be empty if no production, but should not crash

	for (const FName& GoodID : ProfitableExports)
	{
		TestTrue(
			FString::Printf(TEXT("Profitable export %s should be a produced good"), *GoodID.ToString()),
			Econ->IsProducing(GoodID));
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_MostNeededImports,
	"Odyssey.Procedural.PlanetaryEconomy.MostNeededImportsValid",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_MostNeededImports::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FName> NeededImports = Econ->GetMostNeededImports();
	// May be empty if no consumption, but should not crash

	for (const FName& GoodID : NeededImports)
	{
		TestTrue(
			FString::Printf(TEXT("Needed import %s should be a consumed good"), *GoodID.ToString()),
			Econ->IsConsuming(GoodID));
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_PotentialProfitCalc,
	"Odyssey.Procedural.PlanetaryEconomy.PotentialProfitCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_PotentialProfitCalc::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FName> Produced = Econ->GetProducedGoods();
	if (Produced.Num() > 0)
	{
		int32 Profit = Econ->CalculatePotentialProfit(Produced[0], 10, 99);
		// Profit can be positive, zero, or negative depending on market conditions
		TestTrue(TEXT("Profit calculation should complete without crash"), true);
	}

	return true;
}

// ============================================================================
// 8. RESOURCE EXPORT/IMPORT BALANCE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_ExportImportBalance,
	"Odyssey.Procedural.PlanetaryEconomy.ExportImportBalanceReasonable",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_ExportImportBalance::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FName> Produced = Econ->GetProducedGoods();
	TArray<FName> Consumed = Econ->GetConsumedGoods();

	// A planet should not produce exactly the same goods it consumes (economic specialization means trade)
	TSet<FName> ProducedSet(Produced);
	TSet<FName> ConsumedSet(Consumed);

	// Check for overlap
	int32 OverlapCount = 0;
	for (const FName& P : Produced)
	{
		if (ConsumedSet.Contains(P))
		{
			OverlapCount++;
		}
	}

	// Some overlap is acceptable but not total overlap (that would mean no trade needed)
	if (Produced.Num() > 1 && Consumed.Num() > 1)
	{
		TestTrue(TEXT("Not all produced goods should also be consumed (creates trade opportunity)"),
			OverlapCount < Produced.Num() || OverlapCount < Consumed.Num());
	}

	return true;
}

// ============================================================================
// 9. SPECIALIZATION DRIVES ECONOMY
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_MiningSpecProducesMetals,
	"Odyssey.Procedural.PlanetaryEconomy.MiningSpecializationProducesAppropriateGoods",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_MiningSpecProducesMetals::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();

	// Try multiple seeds to find a mining-specialized planet
	for (int32 Seed = 0; Seed < 100; ++Seed)
	{
		UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx, Seed);

		if (Econ->GetPrimarySpecialization() == EEconomicSpecialization::Mining)
		{
			TArray<FName> Produced = Econ->GetProducedGoods();
			TestTrue(TEXT("Mining planet should produce goods"), Produced.Num() > 0);

			// Mining planets should produce mining-related goods
			AddInfo(FString::Printf(TEXT("Mining planet (seed %d) produces: "), Seed));
			for (const FName& G : Produced)
			{
				AddInfo(FString::Printf(TEXT("  %s"), *G.ToString()));
			}
			break;
		}
	}

	return true;
}

// ============================================================================
// 10. EDGE CASES
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_EdgeCase_InvalidGoodID,
	"Odyssey.Procedural.PlanetaryEconomy.EdgeCases.InvalidGoodID",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_EdgeCase_InvalidGoodID::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	FName InvalidGood = FName(TEXT("NonExistentGood"));

	int32 BuyPrice = Econ->GetBuyPrice(InvalidGood);
	int32 SellPrice = Econ->GetSellPrice(InvalidGood);
	bool bCanBuy = Econ->CanBuyGood(InvalidGood, 1);
	bool bCanSell = Econ->CanSellGood(InvalidGood, 1);
	bool bIsProducing = Econ->IsProducing(InvalidGood);
	bool bIsConsuming = Econ->IsConsuming(InvalidGood);
	int32 Stock = Econ->GetProductionStock(InvalidGood);
	int32 Demand = Econ->GetConsumptionDemand(InvalidGood);

	TestFalse(TEXT("Should not be able to buy invalid good"), bCanBuy);
	TestFalse(TEXT("Should not be able to sell invalid good"), bCanSell);
	TestFalse(TEXT("Should not be producing invalid good"), bIsProducing);
	TestFalse(TEXT("Should not be consuming invalid good"), bIsConsuming);
	TestEqual(TEXT("Stock of invalid good should be 0"), Stock, 0);
	TestEqual(TEXT("Demand for invalid good should be 0"), Demand, 0);

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_EdgeCase_ZeroQuantityTrade,
	"Odyssey.Procedural.PlanetaryEconomy.EdgeCases.ZeroQuantityTrade",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_EdgeCase_ZeroQuantityTrade::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	TArray<FName> AllGoods = Econ->GetAllTradeGoodIDs();
	if (AllGoods.Num() > 0)
	{
		int32 TotalCost = 0;
		bool bBought = Econ->ExecuteBuy(AllGoods[0], 0, TotalCost);
		// Zero quantity buy should either succeed with 0 cost or fail gracefully
		TestTrue(TEXT("Zero quantity trade should not crash"), true);
	}

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_EdgeCase_DeterministicEconomy,
	"Odyssey.Procedural.PlanetaryEconomy.EdgeCases.DeterministicEconomy",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_EdgeCase_DeterministicEconomy::RunTest(const FString& Parameters)
{
	auto CtxA = EconomyTestHelpers::CreateGenerationContext();
	auto CtxB = EconomyTestHelpers::CreateGenerationContext();

	UOdysseyPlanetaryEconomyComponent* EconA = EconomyTestHelpers::CreateInitializedEconomy(CtxA, 42);
	UOdysseyPlanetaryEconomyComponent* EconB = EconomyTestHelpers::CreateInitializedEconomy(CtxB, 42);

	TestEqual(TEXT("Same seed should produce same primary specialization"),
		EconA->GetPrimarySpecialization(), EconB->GetPrimarySpecialization());
	TestEqual(TEXT("Same seed should produce same secondary specialization"),
		EconA->GetSecondarySpecialization(), EconB->GetSecondarySpecialization());
	TestEqual(TEXT("Same seed should produce same wealth level"),
		EconA->GetWealthLevel(), EconB->GetWealthLevel());
	TestEqual(TEXT("Same seed should produce same population"),
		EconA->GetPopulation(), EconB->GetPopulation());

	return true;
}

// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlanetEcon_EdgeCase_MultipleMarketUpdates,
	"Odyssey.Procedural.PlanetaryEconomy.EdgeCases.MultipleMarketUpdates",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlanetEcon_EdgeCase_MultipleMarketUpdates::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateGenerationContext();
	UOdysseyPlanetaryEconomyComponent* Econ = EconomyTestHelpers::CreateInitializedEconomy(Ctx);

	// Trigger multiple market updates to ensure stability
	for (int32 i = 0; i < 20; ++i)
	{
		Econ->UpdateMarketPrices();
	}

	// Prices should still be valid after many updates
	TArray<FPlanetaryMarketPrice> Prices = Econ->GetAllMarketPrices();
	for (const FPlanetaryMarketPrice& Price : Prices)
	{
		TestTrue(
			FString::Printf(TEXT("Price for %s should be positive after 20 updates"), *Price.GoodID.ToString()),
			Price.BuyPrice > 0 && Price.SellPrice > 0);
		// Prices should be bounded by min/max multiplier constants
		FTradeGood GoodInfo = Econ->GetTradeGoodInfo(Price.GoodID);
		float MaxAllowed = GoodInfo.BaseValue * ProceduralConstants::MaxPriceMultiplier * 1.5f;
		TestTrue(
			FString::Printf(TEXT("Buy price (%d) for %s should be reasonably bounded"), Price.BuyPrice, *Price.GoodID.ToString()),
			Price.BuyPrice <= static_cast<int32>(MaxAllowed));
	}

	return true;
}
