// PlanetaryEconomyComponent.cpp
// Implementation of planetary economic specialization, production, consumption, and markets
// Part of the Odyssey Procedural Planet & Resource Generation System

#include "OdysseyPlanetaryEconomyComponent.h"

// ============================================================================
// CONSTRUCTOR & LIFECYCLE
// ============================================================================

UOdysseyPlanetaryEconomyComponent::UOdysseyPlanetaryEconomyComponent()
	: PlanetID(0)
	, PlanetName(FText::FromString(TEXT("Unknown")))
	, PrimarySpecialization(EEconomicSpecialization::None)
	, SecondarySpecialization(EEconomicSpecialization::None)
	, WealthLevel(50)
	, DevelopmentLevel(50)
	, Population(1000)
	, MarketUpdateInterval(60.0f)
	, PriceVolatility(0.1f)
	, DemandMultiplier(1.0f)
	, MarketUpdateTimer(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f; // Tick once per second for economy updates
}

void UOdysseyPlanetaryEconomyComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeTradeGoods();
}

void UOdysseyPlanetaryEconomyComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateProduction(DeltaTime);
	UpdateConsumption(DeltaTime);

	MarketUpdateTimer += DeltaTime;
	if (MarketUpdateTimer >= MarketUpdateInterval)
	{
		MarketUpdateTimer = 0.0f;
		UpdateMarketPrices();
	}
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void UOdysseyPlanetaryEconomyComponent::InitializeFromPlanetData(
	const FGeneratedPlanetData& PlanetData,
	int32 Seed)
{
	PlanetID = PlanetData.PlanetID;
	PlanetName = PlanetData.PlanetName;

	DetermineSpecialization(PlanetData, Seed);
	SetupProduction(Seed);
	SetupConsumption(Seed);
	InitializeMarketPrices();

	// Set economic metrics based on planet data
	WealthLevel = FMath::Clamp(PlanetData.EconomicRating, 0, 100);
	DangerModifiesWealth(PlanetData.DangerRating, Seed);

	// Population scales with planet size and habitability
	float SizeFactor = 1.0f;
	switch (PlanetData.PlanetSize)
	{
	case EPlanetSize::Tiny:   SizeFactor = 0.2f; break;
	case EPlanetSize::Small:  SizeFactor = 0.5f; break;
	case EPlanetSize::Medium: SizeFactor = 1.0f; break;
	case EPlanetSize::Large:  SizeFactor = 2.0f; break;
	case EPlanetSize::Huge:   SizeFactor = 3.5f; break;
	case EPlanetSize::Giant:  SizeFactor = 5.0f; break;
	}

	float HabitabilityFactor = 1.0f;
	switch (PlanetData.PlanetType)
	{
	case EPlanetType::Terrestrial: HabitabilityFactor = 1.0f; break;
	case EPlanetType::Oceanic:     HabitabilityFactor = 0.7f; break;
	case EPlanetType::Jungle:      HabitabilityFactor = 0.8f; break;
	case EPlanetType::Desert:      HabitabilityFactor = 0.4f; break;
	case EPlanetType::Arctic:      HabitabilityFactor = 0.3f; break;
	case EPlanetType::Volcanic:    HabitabilityFactor = 0.15f; break;
	case EPlanetType::Barren:      HabitabilityFactor = 0.05f; break;
	case EPlanetType::Exotic:      HabitabilityFactor = 0.1f; break;
	case EPlanetType::Artificial:  HabitabilityFactor = 0.6f; break;
	}

	int32 BasePopulation = SeededRandomRange(Seed + 500, 500, 5000);
	Population = static_cast<int32>(static_cast<float>(BasePopulation) * SizeFactor * HabitabilityFactor);
	DevelopmentLevel = FMath::Clamp(static_cast<int32>(static_cast<float>(WealthLevel) * HabitabilityFactor), 10, 100);
}

void UOdysseyPlanetaryEconomyComponent::InitializeTradeGoods()
{
	TradeGoodDefinitions.Empty();

	// Raw materials
	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("RawOre"));
		Good.DisplayName = FText::FromString(TEXT("Raw Ore"));
		Good.Description = FText::FromString(TEXT("Unprocessed mineral ore ready for smelting."));
		Good.BaseValue = 8;
		Good.VolumePerUnit = 2.0f;
		Good.SourceResource = EResourceType::Silicate;
		Good.ProducingSpecialization = EEconomicSpecialization::Mining;
		Good.ConsumingSpecializations = { EEconomicSpecialization::Manufacturing, EEconomicSpecialization::Technology };
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}

	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("OrganicCompounds"));
		Good.DisplayName = FText::FromString(TEXT("Organic Compounds"));
		Good.Description = FText::FromString(TEXT("Carbon-based compounds for industrial and biological use."));
		Good.BaseValue = 12;
		Good.VolumePerUnit = 1.5f;
		Good.SourceResource = EResourceType::Carbon;
		Good.ProducingSpecialization = EEconomicSpecialization::Agriculture;
		Good.ConsumingSpecializations = { EEconomicSpecialization::Manufacturing, EEconomicSpecialization::Research };
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}

	// Processed goods
	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("RefinedAlloys"));
		Good.DisplayName = FText::FromString(TEXT("Refined Alloys"));
		Good.Description = FText::FromString(TEXT("High-purity metal alloys for construction and engineering."));
		Good.BaseValue = 25;
		Good.VolumePerUnit = 3.0f;
		Good.SourceResource = EResourceType::RefinedSilicate;
		Good.ProducingSpecialization = EEconomicSpecialization::Manufacturing;
		Good.ConsumingSpecializations = { EEconomicSpecialization::Technology, EEconomicSpecialization::Military };
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}

	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("AdvancedPolymers"));
		Good.DisplayName = FText::FromString(TEXT("Advanced Polymers"));
		Good.Description = FText::FromString(TEXT("Engineered carbon polymers with exceptional properties."));
		Good.BaseValue = 30;
		Good.VolumePerUnit = 1.0f;
		Good.SourceResource = EResourceType::RefinedCarbon;
		Good.ProducingSpecialization = EEconomicSpecialization::Manufacturing;
		Good.ConsumingSpecializations = { EEconomicSpecialization::Technology, EEconomicSpecialization::Research };
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}

	// High-tech goods
	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("NanoComponents"));
		Good.DisplayName = FText::FromString(TEXT("Nano-Components"));
		Good.Description = FText::FromString(TEXT("Microscale engineering components for advanced devices."));
		Good.BaseValue = 75;
		Good.VolumePerUnit = 0.5f;
		Good.SourceResource = EResourceType::CompositeMaterial;
		Good.ProducingSpecialization = EEconomicSpecialization::Technology;
		Good.ConsumingSpecializations = { EEconomicSpecialization::Research, EEconomicSpecialization::Military };
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}

	// Service goods
	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("FoodRations"));
		Good.DisplayName = FText::FromString(TEXT("Food Rations"));
		Good.Description = FText::FromString(TEXT("Nutrient-rich food supplies for colony sustenance."));
		Good.BaseValue = 15;
		Good.VolumePerUnit = 1.0f;
		Good.SourceResource = EResourceType::Carbon;
		Good.ProducingSpecialization = EEconomicSpecialization::Agriculture;
		Good.ConsumingSpecializations = {
			EEconomicSpecialization::Mining, EEconomicSpecialization::Military,
			EEconomicSpecialization::Research, EEconomicSpecialization::Trade
		};
		Good.Perishability = 1;
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}

	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("EnergyCells"));
		Good.DisplayName = FText::FromString(TEXT("Energy Cells"));
		Good.Description = FText::FromString(TEXT("Portable energy storage units."));
		Good.BaseValue = 20;
		Good.VolumePerUnit = 2.0f;
		Good.SourceResource = EResourceType::None;
		Good.ProducingSpecialization = EEconomicSpecialization::Energy;
		Good.ConsumingSpecializations = {
			EEconomicSpecialization::Mining, EEconomicSpecialization::Manufacturing,
			EEconomicSpecialization::Technology, EEconomicSpecialization::Military
		};
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}

	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("ResearchData"));
		Good.DisplayName = FText::FromString(TEXT("Research Data"));
		Good.Description = FText::FromString(TEXT("Compiled scientific findings and experimental results."));
		Good.BaseValue = 50;
		Good.VolumePerUnit = 0.1f;
		Good.SourceResource = EResourceType::None;
		Good.ProducingSpecialization = EEconomicSpecialization::Research;
		Good.ConsumingSpecializations = { EEconomicSpecialization::Technology, EEconomicSpecialization::Manufacturing };
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}

	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("LuxuryGoods"));
		Good.DisplayName = FText::FromString(TEXT("Luxury Goods"));
		Good.Description = FText::FromString(TEXT("High-end consumer products and artisan crafts."));
		Good.BaseValue = 60;
		Good.VolumePerUnit = 1.0f;
		Good.SourceResource = EResourceType::None;
		Good.ProducingSpecialization = EEconomicSpecialization::Tourism;
		Good.ConsumingSpecializations = { EEconomicSpecialization::Trade, EEconomicSpecialization::Tourism };
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}

	{
		FTradeGood Good;
		Good.GoodID = FName(TEXT("Munitions"));
		Good.DisplayName = FText::FromString(TEXT("Munitions"));
		Good.Description = FText::FromString(TEXT("Weapons and defensive equipment."));
		Good.BaseValue = 40;
		Good.VolumePerUnit = 3.0f;
		Good.SourceResource = EResourceType::None;
		Good.ProducingSpecialization = EEconomicSpecialization::Military;
		Good.ConsumingSpecializations = { EEconomicSpecialization::Mining, EEconomicSpecialization::Trade };
		Good.LegalityStatus = 1; // Restricted
		TradeGoodDefinitions.Add(Good.GoodID, Good);
	}
}

// ============================================================================
// SPECIALIZATION
// ============================================================================

void UOdysseyPlanetaryEconomyComponent::SetSpecializations(
	EEconomicSpecialization Primary,
	EEconomicSpecialization Secondary)
{
	PrimarySpecialization = Primary;
	SecondarySpecialization = Secondary;
}

bool UOdysseyPlanetaryEconomyComponent::HasSpecialization(EEconomicSpecialization Specialization) const
{
	return PrimarySpecialization == Specialization || SecondarySpecialization == Specialization;
}

void UOdysseyPlanetaryEconomyComponent::DetermineSpecialization(
	const FGeneratedPlanetData& PlanetData,
	int32 Seed)
{
	// Determine primary specialization based on planet type and resources
	switch (PlanetData.PlanetType)
	{
	case EPlanetType::Terrestrial:
		PrimarySpecialization = EEconomicSpecialization::Agriculture;
		break;
	case EPlanetType::Oceanic:
		PrimarySpecialization = EEconomicSpecialization::Research;
		break;
	case EPlanetType::Desert:
		PrimarySpecialization = EEconomicSpecialization::Mining;
		break;
	case EPlanetType::Arctic:
		PrimarySpecialization = EEconomicSpecialization::Research;
		break;
	case EPlanetType::Volcanic:
		PrimarySpecialization = EEconomicSpecialization::Energy;
		break;
	case EPlanetType::Jungle:
		PrimarySpecialization = EEconomicSpecialization::Agriculture;
		break;
	case EPlanetType::Barren:
		PrimarySpecialization = EEconomicSpecialization::Mining;
		break;
	case EPlanetType::Exotic:
		PrimarySpecialization = EEconomicSpecialization::Tourism;
		break;
	case EPlanetType::Artificial:
		PrimarySpecialization = EEconomicSpecialization::Technology;
		break;
	}

	// Override based on resource richness
	int32 HighValueResourceCount = 0;
	for (const FResourceDepositLocation& Deposit : PlanetData.ResourceDeposits)
	{
		if (Deposit.ResourceType == EResourceType::CompositeMaterial ||
			Deposit.ResourceType == EResourceType::RefinedSilicate ||
			Deposit.ResourceType == EResourceType::RefinedCarbon)
		{
			HighValueResourceCount++;
		}
	}

	if (HighValueResourceCount > PlanetData.ResourceDeposits.Num() / 3)
	{
		PrimarySpecialization = EEconomicSpecialization::Technology;
	}
	else if (PlanetData.ResourceDeposits.Num() > 20)
	{
		PrimarySpecialization = EEconomicSpecialization::Mining;
	}

	// Determine secondary specialization
	TArray<EEconomicSpecialization> Candidates = {
		EEconomicSpecialization::Mining,
		EEconomicSpecialization::Agriculture,
		EEconomicSpecialization::Manufacturing,
		EEconomicSpecialization::Technology,
		EEconomicSpecialization::Trade,
		EEconomicSpecialization::Research,
		EEconomicSpecialization::Military,
		EEconomicSpecialization::Tourism,
		EEconomicSpecialization::Energy
	};

	// Remove primary from candidates
	Candidates.RemoveAll([this](EEconomicSpecialization S) { return S == PrimarySpecialization; });

	if (Candidates.Num() > 0)
	{
		int32 Index = SeededRandomRange(Seed + 200, 0, Candidates.Num() - 1);
		SecondarySpecialization = Candidates[Index];
	}
}

// ============================================================================
// PRODUCTION & CONSUMPTION SETUP
// ============================================================================

void UOdysseyPlanetaryEconomyComponent::SetupProduction(int32 Seed)
{
	Productions.Empty();

	// Set up production based on primary specialization
	for (const auto& GoodPair : TradeGoodDefinitions)
	{
		const FTradeGood& Good = GoodPair.Value;

		if (Good.ProducingSpecialization == PrimarySpecialization)
		{
			FPlanetaryProduction Prod;
			Prod.GoodID = Good.GoodID;
			Prod.ProductionRate = SeededRandomRange(Seed + Good.BaseValue, 5, 25);
			Prod.Efficiency = 0.8f + SeededRandom(Seed + Good.BaseValue + 100) * 0.4f;
			Prod.MaxStorage = Prod.ProductionRate * 50;
			Prod.CurrentStock = Prod.ProductionRate * SeededRandomRange(Seed + Good.BaseValue + 200, 5, 20);
			Prod.bIsActive = true;
			Productions.Add(Prod);
		}
		else if (Good.ProducingSpecialization == SecondarySpecialization)
		{
			// Secondary production at reduced rate
			FPlanetaryProduction Prod;
			Prod.GoodID = Good.GoodID;
			Prod.ProductionRate = SeededRandomRange(Seed + Good.BaseValue + 300, 2, 10);
			Prod.Efficiency = 0.5f + SeededRandom(Seed + Good.BaseValue + 400) * 0.3f;
			Prod.MaxStorage = Prod.ProductionRate * 30;
			Prod.CurrentStock = Prod.ProductionRate * SeededRandomRange(Seed + Good.BaseValue + 500, 2, 10);
			Prod.bIsActive = true;
			Productions.Add(Prod);
		}
	}
}

void UOdysseyPlanetaryEconomyComponent::SetupConsumption(int32 Seed)
{
	Consumptions.Empty();

	for (const auto& GoodPair : TradeGoodDefinitions)
	{
		const FTradeGood& Good = GoodPair.Value;

		// Consume goods that our specializations need
		bool bNeeded = false;
		for (EEconomicSpecialization ConsumerSpec : Good.ConsumingSpecializations)
		{
			if (ConsumerSpec == PrimarySpecialization || ConsumerSpec == SecondarySpecialization)
			{
				bNeeded = true;
				break;
			}
		}

		// Do not consume what we primarily produce (self-sufficient)
		if (Good.ProducingSpecialization == PrimarySpecialization)
		{
			bNeeded = false;
		}

		if (bNeeded)
		{
			FPlanetaryConsumption Cons;
			Cons.GoodID = Good.GoodID;
			Cons.ConsumptionRate = SeededRandomRange(Seed + Good.BaseValue + 600, 3, 15);
			Cons.CurrentDemand = Cons.ConsumptionRate * SeededRandomRange(Seed + Good.BaseValue + 700, 5, 15);
			Cons.CurrentStock = SeededRandomRange(Seed + Good.BaseValue + 800, 0, Cons.CurrentDemand);
			Cons.Urgency = (Cons.CurrentStock < Cons.CurrentDemand / 3) ? 2 : (Cons.CurrentStock < Cons.CurrentDemand) ? 1 : 0;
			Consumptions.Add(Cons);
		}
	}

	// Everyone needs food
	bool bHasFood = false;
	for (const FPlanetaryConsumption& C : Consumptions)
	{
		if (C.GoodID == FName(TEXT("FoodRations")))
		{
			bHasFood = true;
			break;
		}
	}

	if (!bHasFood && PrimarySpecialization != EEconomicSpecialization::Agriculture)
	{
		FPlanetaryConsumption FoodCons;
		FoodCons.GoodID = FName(TEXT("FoodRations"));
		FoodCons.ConsumptionRate = FMath::Max(5, Population / 200);
		FoodCons.CurrentDemand = FoodCons.ConsumptionRate * 10;
		FoodCons.CurrentStock = SeededRandomRange(Seed + 900, 0, FoodCons.CurrentDemand);
		FoodCons.Urgency = (FoodCons.CurrentStock < FoodCons.CurrentDemand / 3) ? 2 : 0;
		Consumptions.Add(FoodCons);
	}
}

void UOdysseyPlanetaryEconomyComponent::InitializeMarketPrices()
{
	MarketPrices.Empty();

	for (const auto& GoodPair : TradeGoodDefinitions)
	{
		const FTradeGood& Good = GoodPair.Value;
		FPlanetaryMarketPrice Price;
		Price.GoodID = Good.GoodID;

		int32 BasePrice = Good.BaseValue;

		// Producers sell cheaper
		if (IsProducing(Good.GoodID))
		{
			int32 Stock = GetProductionStock(Good.GoodID);
			float SupplyFactor = 1.0f - FMath::Clamp(static_cast<float>(Stock) / 500.0f, 0.0f, 0.3f);
			BasePrice = static_cast<int32>(static_cast<float>(BasePrice) * SupplyFactor);
		}

		// Consumers pay more
		if (IsConsuming(Good.GoodID))
		{
			int32 Demand = GetConsumptionDemand(Good.GoodID);
			float DemandFactor = 1.0f + FMath::Clamp(static_cast<float>(Demand) / 200.0f, 0.0f, 0.5f);
			BasePrice = static_cast<int32>(static_cast<float>(BasePrice) * DemandFactor);
		}

		BasePrice = FMath::Max(1, BasePrice);

		Price.BuyPrice = static_cast<int32>(static_cast<float>(BasePrice) * 1.1f);
		Price.SellPrice = static_cast<int32>(static_cast<float>(BasePrice) * 0.9f);
		Price.AvailableQuantity = IsProducing(Good.GoodID) ? GetProductionStock(Good.GoodID) : 0;
		Price.DemandQuantity = IsConsuming(Good.GoodID) ? GetConsumptionDemand(Good.GoodID) : 0;
		Price.PriceTrend = 0;
		Price.LastUpdateTime = 0.0f;

		MarketPrices.Add(Price);
	}
}

// ============================================================================
// PRODUCTION & CONSUMPTION ACCESS
// ============================================================================

bool UOdysseyPlanetaryEconomyComponent::IsProducing(FName GoodID) const
{
	for (const FPlanetaryProduction& P : Productions)
	{
		if (P.GoodID == GoodID) return true;
	}
	return false;
}

int32 UOdysseyPlanetaryEconomyComponent::GetProductionStock(FName GoodID) const
{
	for (const FPlanetaryProduction& P : Productions)
	{
		if (P.GoodID == GoodID) return P.CurrentStock;
	}
	return 0;
}

void UOdysseyPlanetaryEconomyComponent::AddProduction(const FPlanetaryProduction& Production)
{
	// Replace if exists
	for (int32 i = 0; i < Productions.Num(); ++i)
	{
		if (Productions[i].GoodID == Production.GoodID)
		{
			Productions[i] = Production;
			return;
		}
	}
	Productions.Add(Production);
}

void UOdysseyPlanetaryEconomyComponent::RemoveProduction(FName GoodID)
{
	Productions.RemoveAll([GoodID](const FPlanetaryProduction& P) { return P.GoodID == GoodID; });
}

bool UOdysseyPlanetaryEconomyComponent::IsConsuming(FName GoodID) const
{
	for (const FPlanetaryConsumption& C : Consumptions)
	{
		if (C.GoodID == GoodID) return true;
	}
	return false;
}

int32 UOdysseyPlanetaryEconomyComponent::GetConsumptionDemand(FName GoodID) const
{
	for (const FPlanetaryConsumption& C : Consumptions)
	{
		if (C.GoodID == GoodID) return C.CurrentDemand;
	}
	return 0;
}

void UOdysseyPlanetaryEconomyComponent::AddConsumption(const FPlanetaryConsumption& Consumption)
{
	for (int32 i = 0; i < Consumptions.Num(); ++i)
	{
		if (Consumptions[i].GoodID == Consumption.GoodID)
		{
			Consumptions[i] = Consumption;
			return;
		}
	}
	Consumptions.Add(Consumption);
}

// ============================================================================
// MARKET OPERATIONS
// ============================================================================

FPlanetaryMarketPrice UOdysseyPlanetaryEconomyComponent::GetMarketPrice(FName GoodID) const
{
	for (const FPlanetaryMarketPrice& P : MarketPrices)
	{
		if (P.GoodID == GoodID) return P;
	}
	return FPlanetaryMarketPrice();
}

int32 UOdysseyPlanetaryEconomyComponent::GetBuyPrice(FName GoodID) const
{
	for (const FPlanetaryMarketPrice& P : MarketPrices)
	{
		if (P.GoodID == GoodID) return P.BuyPrice;
	}
	return 0;
}

int32 UOdysseyPlanetaryEconomyComponent::GetSellPrice(FName GoodID) const
{
	for (const FPlanetaryMarketPrice& P : MarketPrices)
	{
		if (P.GoodID == GoodID) return P.SellPrice;
	}
	return 0;
}

bool UOdysseyPlanetaryEconomyComponent::CanBuyGood(FName GoodID, int32 Quantity) const
{
	for (const FPlanetaryMarketPrice& P : MarketPrices)
	{
		if (P.GoodID == GoodID)
		{
			return P.AvailableQuantity >= Quantity;
		}
	}
	return false;
}

bool UOdysseyPlanetaryEconomyComponent::CanSellGood(FName GoodID, int32 Quantity) const
{
	for (const FPlanetaryMarketPrice& P : MarketPrices)
	{
		if (P.GoodID == GoodID)
		{
			return P.DemandQuantity >= Quantity;
		}
	}
	return false;
}

bool UOdysseyPlanetaryEconomyComponent::ExecuteBuy(FName GoodID, int32 Quantity, int32& OutTotalCost)
{
	for (FPlanetaryMarketPrice& P : MarketPrices)
	{
		if (P.GoodID == GoodID)
		{
			if (P.AvailableQuantity < Quantity)
			{
				return false;
			}

			OutTotalCost = P.BuyPrice * Quantity;
			P.AvailableQuantity -= Quantity;

			// Reduce production stock
			for (FPlanetaryProduction& Prod : Productions)
			{
				if (Prod.GoodID == GoodID)
				{
					Prod.CurrentStock = FMath::Max(0, Prod.CurrentStock - Quantity);
					break;
				}
			}

			OnTradeExecuted(GoodID, Quantity, true);
			return true;
		}
	}

	return false;
}

bool UOdysseyPlanetaryEconomyComponent::ExecuteSell(FName GoodID, int32 Quantity, int32& OutTotalRevenue)
{
	for (FPlanetaryMarketPrice& P : MarketPrices)
	{
		if (P.GoodID == GoodID)
		{
			OutTotalRevenue = P.SellPrice * Quantity;
			P.DemandQuantity = FMath::Max(0, P.DemandQuantity - Quantity);

			// Add to consumption stock
			for (FPlanetaryConsumption& Cons : Consumptions)
			{
				if (Cons.GoodID == GoodID)
				{
					Cons.CurrentStock += Quantity;
					Cons.Urgency = (Cons.CurrentStock < Cons.CurrentDemand / 3) ? 2
						: (Cons.CurrentStock < Cons.CurrentDemand) ? 1 : 0;
					break;
				}
			}

			OnTradeExecuted(GoodID, Quantity, false);
			return true;
		}
	}

	return false;
}

void UOdysseyPlanetaryEconomyComponent::UpdateMarketPrices()
{
	for (FPlanetaryMarketPrice& Price : MarketPrices)
	{
		int32 OldBuyPrice = Price.BuyPrice;
		int32 NewPrice = CalculateDynamicPrice(Price.GoodID, true);

		Price.BuyPrice = NewPrice;
		Price.SellPrice = static_cast<int32>(static_cast<float>(NewPrice) * 0.82f); // ~18% spread

		// Determine trend
		if (NewPrice > OldBuyPrice * 1.05f)
		{
			Price.PriceTrend = 1; // Rising
		}
		else if (NewPrice < OldBuyPrice * 0.95f)
		{
			Price.PriceTrend = -1; // Falling
		}
		else
		{
			Price.PriceTrend = 0; // Stable
		}

		// Update available quantity from production
		Price.AvailableQuantity = GetProductionStock(Price.GoodID);
		Price.DemandQuantity = GetConsumptionDemand(Price.GoodID);
		Price.LastUpdateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	}

	OnMarketPricesUpdated();
}

int32 UOdysseyPlanetaryEconomyComponent::CalculateDynamicPrice(FName GoodID, bool bForBuying) const
{
	const FTradeGood* GoodDef = TradeGoodDefinitions.Find(GoodID);
	if (!GoodDef)
	{
		return 10;
	}

	float BasePrice = static_cast<float>(GoodDef->BaseValue);

	// Supply factor
	float SupplyFactor = 1.0f;
	for (const FPlanetaryProduction& Prod : Productions)
	{
		if (Prod.GoodID == GoodID)
		{
			float StockRatio = static_cast<float>(Prod.CurrentStock) / FMath::Max(1.0f, static_cast<float>(Prod.MaxStorage));
			SupplyFactor = 1.0f - (StockRatio * 0.3f); // More stock = lower price
			break;
		}
	}

	// Demand factor
	float DemandFactor = 1.0f;
	for (const FPlanetaryConsumption& Cons : Consumptions)
	{
		if (Cons.GoodID == GoodID)
		{
			float DemandRatio = static_cast<float>(Cons.CurrentDemand) / FMath::Max(1.0f, static_cast<float>(Cons.CurrentStock + 1));
			DemandFactor = 1.0f + FMath::Clamp(DemandRatio * 0.2f, 0.0f, 0.8f);

			// Urgency bonus
			DemandFactor += Cons.Urgency * 0.15f;
			break;
		}
	}

	// Population factor (more people = more demand)
	float PopFactor = 1.0f + FMath::Clamp(static_cast<float>(Population) / 10000.0f, 0.0f, 0.3f);

	// Wealth factor (wealthy planets can afford higher prices)
	float WealthFactor = 0.8f + (static_cast<float>(WealthLevel) / 100.0f) * 0.4f;

	float FinalPrice = BasePrice * SupplyFactor * DemandFactor * PopFactor * WealthFactor * DemandMultiplier;

	// Add spread
	if (bForBuying)
	{
		FinalPrice *= 1.05f;
	}
	else
	{
		FinalPrice *= 0.87f;
	}

	return FMath::Max(1, static_cast<int32>(FinalPrice));
}

// ============================================================================
// TRADE GOOD INFO
// ============================================================================

FTradeGood UOdysseyPlanetaryEconomyComponent::GetTradeGoodInfo(FName GoodID) const
{
	if (const FTradeGood* Good = TradeGoodDefinitions.Find(GoodID))
	{
		return *Good;
	}
	return FTradeGood();
}

TArray<FName> UOdysseyPlanetaryEconomyComponent::GetAllTradeGoodIDs() const
{
	TArray<FName> IDs;
	TradeGoodDefinitions.GetKeys(IDs);
	return IDs;
}

TArray<FName> UOdysseyPlanetaryEconomyComponent::GetProducedGoods() const
{
	TArray<FName> Produced;
	for (const FPlanetaryProduction& P : Productions)
	{
		Produced.Add(P.GoodID);
	}
	return Produced;
}

TArray<FName> UOdysseyPlanetaryEconomyComponent::GetConsumedGoods() const
{
	TArray<FName> Consumed;
	for (const FPlanetaryConsumption& C : Consumptions)
	{
		Consumed.Add(C.GoodID);
	}
	return Consumed;
}

// ============================================================================
// ECONOMIC RELATIONSHIPS
// ============================================================================

FEconomicRelationship UOdysseyPlanetaryEconomyComponent::GetRelationship(int32 OtherPlanetID) const
{
	for (const FEconomicRelationship& Rel : EconomicRelationships)
	{
		if (Rel.PartnerPlanetID == OtherPlanetID)
		{
			return Rel;
		}
	}
	return FEconomicRelationship();
}

void UOdysseyPlanetaryEconomyComponent::UpdateRelationship(const FEconomicRelationship& Relationship)
{
	for (int32 i = 0; i < EconomicRelationships.Num(); ++i)
	{
		if (EconomicRelationships[i].PartnerPlanetID == Relationship.PartnerPlanetID)
		{
			EconomicRelationships[i] = Relationship;
			return;
		}
	}
	EconomicRelationships.Add(Relationship);
}

float UOdysseyPlanetaryEconomyComponent::GetTariffRate(int32 OtherPlanetID) const
{
	for (const FEconomicRelationship& Rel : EconomicRelationships)
	{
		if (Rel.PartnerPlanetID == OtherPlanetID)
		{
			return Rel.bHasTradeAgreement ? Rel.TariffRate * 0.5f : Rel.TariffRate;
		}
	}
	return 0.1f; // Default tariff for unknown partners
}

TArray<int32> UOdysseyPlanetaryEconomyComponent::GetTradingPartners() const
{
	TArray<int32> Partners;
	for (const FEconomicRelationship& Rel : EconomicRelationships)
	{
		Partners.Add(Rel.PartnerPlanetID);
	}
	return Partners;
}

// ============================================================================
// ECONOMIC ANALYSIS
// ============================================================================

TArray<FName> UOdysseyPlanetaryEconomyComponent::GetMostProfitableExports() const
{
	TArray<TPair<FName, int32>> ExportValues;

	for (const FPlanetaryProduction& Prod : Productions)
	{
		if (const FTradeGood* Good = TradeGoodDefinitions.Find(Prod.GoodID))
		{
			int32 Value = Good->BaseValue * Prod.CurrentStock;
			ExportValues.Add(TPair<FName, int32>(Prod.GoodID, Value));
		}
	}

	ExportValues.Sort([](const TPair<FName, int32>& A, const TPair<FName, int32>& B)
	{
		return A.Value > B.Value;
	});

	TArray<FName> Result;
	for (const auto& Pair : ExportValues)
	{
		Result.Add(Pair.Key);
	}
	return Result;
}

TArray<FName> UOdysseyPlanetaryEconomyComponent::GetMostNeededImports() const
{
	TArray<TPair<FName, int32>> ImportUrgency;

	for (const FPlanetaryConsumption& Cons : Consumptions)
	{
		int32 Deficit = FMath::Max(0, Cons.CurrentDemand - Cons.CurrentStock);
		int32 UrgencyScore = Deficit * (Cons.Urgency + 1);
		ImportUrgency.Add(TPair<FName, int32>(Cons.GoodID, UrgencyScore));
	}

	ImportUrgency.Sort([](const TPair<FName, int32>& A, const TPair<FName, int32>& B)
	{
		return A.Value > B.Value;
	});

	TArray<FName> Result;
	for (const auto& Pair : ImportUrgency)
	{
		Result.Add(Pair.Key);
	}
	return Result;
}

int32 UOdysseyPlanetaryEconomyComponent::CalculatePotentialProfit(
	FName GoodID, int32 Quantity, int32 DestinationPlanetID) const
{
	int32 BuyPrice = GetBuyPrice(GoodID);
	if (BuyPrice <= 0) return 0;

	// Estimate sell price at destination (simplified)
	int32 BaseSellPrice = BuyPrice;
	if (const FTradeGood* Good = TradeGoodDefinitions.Find(GoodID))
	{
		BaseSellPrice = Good->BaseValue;
	}

	// Apply tariff
	float Tariff = GetTariffRate(DestinationPlanetID);
	int32 EstimatedRevenue = static_cast<int32>(static_cast<float>(BaseSellPrice * Quantity) * (1.0f - Tariff));
	int32 Cost = BuyPrice * Quantity;

	return EstimatedRevenue - Cost;
}

int32 UOdysseyPlanetaryEconomyComponent::GetTotalGDP() const
{
	int32 GDP = 0;

	for (const FPlanetaryProduction& Prod : Productions)
	{
		if (const FTradeGood* Good = TradeGoodDefinitions.Find(Prod.GoodID))
		{
			GDP += Good->BaseValue * Prod.ProductionRate;
		}
	}

	return GDP * 365; // Annualized
}

// ============================================================================
// SIMULATION UPDATES
// ============================================================================

void UOdysseyPlanetaryEconomyComponent::UpdateProduction(float DeltaTime)
{
	float DaysPerSecond = 1.0f / 86400.0f; // Game time scale (can be adjusted)
	float ProductionDelta = DeltaTime * DaysPerSecond;

	for (FPlanetaryProduction& Prod : Productions)
	{
		if (!Prod.bIsActive) continue;

		int32 Produced = static_cast<int32>(
			static_cast<float>(Prod.ProductionRate) * ProductionDelta * Prod.Efficiency
		);

		if (Produced > 0)
		{
			Prod.CurrentStock = FMath::Min(Prod.CurrentStock + Produced, Prod.MaxStorage);
		}
	}
}

void UOdysseyPlanetaryEconomyComponent::UpdateConsumption(float DeltaTime)
{
	float DaysPerSecond = 1.0f / 86400.0f;
	float ConsumptionDelta = DeltaTime * DaysPerSecond;

	for (FPlanetaryConsumption& Cons : Consumptions)
	{
		int32 Consumed = static_cast<int32>(
			static_cast<float>(Cons.ConsumptionRate) * ConsumptionDelta
		);

		if (Consumed > 0)
		{
			Cons.CurrentStock = FMath::Max(0, Cons.CurrentStock - Consumed);

			// Update urgency
			if (Cons.CurrentStock <= 0)
			{
				Cons.Urgency = 2;
				OnSupplyShortage(Cons.GoodID);
			}
			else if (Cons.CurrentStock < Cons.CurrentDemand / 3)
			{
				Cons.Urgency = 2;
			}
			else if (Cons.CurrentStock < Cons.CurrentDemand)
			{
				Cons.Urgency = 1;
			}
			else
			{
				Cons.Urgency = 0;
			}
		}

		// Regenerate demand over time
		Cons.CurrentDemand = FMath::Max(
			Cons.CurrentDemand,
			static_cast<int32>(static_cast<float>(Cons.ConsumptionRate) * 10.0f * DemandMultiplier)
		);
	}
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

void UOdysseyPlanetaryEconomyComponent::DangerModifiesWealth(int32 DangerRating, int32 Seed)
{
	// Dangerous planets have more volatile wealth
	if (DangerRating > 70)
	{
		WealthLevel = FMath::Clamp(WealthLevel - SeededRandomRange(Seed + 100, 5, 20), 10, 100);
		PriceVolatility += 0.1f;
	}
	else if (DangerRating < 30)
	{
		WealthLevel = FMath::Clamp(WealthLevel + SeededRandomRange(Seed + 100, 0, 10), 10, 100);
	}
}

uint32 UOdysseyPlanetaryEconomyComponent::HashSeed(int32 Seed)
{
	uint32 Hash = static_cast<uint32>(Seed);
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = ((Hash >> 16) ^ Hash) * 0x45d9f3b;
	Hash = (Hash >> 16) ^ Hash;
	return Hash;
}

float UOdysseyPlanetaryEconomyComponent::SeededRandom(int32 Seed)
{
	uint32 Hash = HashSeed(Seed);
	return static_cast<float>(Hash & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
}

int32 UOdysseyPlanetaryEconomyComponent::SeededRandomRange(int32 Seed, int32 Min, int32 Max)
{
	if (Min >= Max) return Min;
	float Random = SeededRandom(Seed);
	return Min + static_cast<int32>(Random * static_cast<float>(Max - Min + 1));
}

