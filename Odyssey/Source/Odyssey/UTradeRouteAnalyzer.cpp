// UTradeRouteAnalyzer.cpp
// Implementation of trade route analysis and opportunity detection

#include "UTradeRouteAnalyzer.h"
#include "Engine/World.h"

UTradeRouteAnalyzer::UTradeRouteAnalyzer()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f;

	MinProfitMarginThreshold = 0.10f; // 10% minimum profit
	AnalysisIntervalSeconds = 10.0f;
	MaxTrackedOpportunities = 50;
	OpportunityExpirationSeconds = 60.0f;
	TimeSinceLastAnalysis = 0.0f;
}

void UTradeRouteAnalyzer::BeginPlay()
{
	Super::BeginPlay();
}

void UTradeRouteAnalyzer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastAnalysis += DeltaTime;

	if (TimeSinceLastAnalysis >= AnalysisIntervalSeconds)
	{
		UpdateOpportunities();
		TimeSinceLastAnalysis = 0.0f;
	}

	// Prune expired opportunities periodically
	PruneExpiredOpportunities();
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void UTradeRouteAnalyzer::RegisterMarket(const FMarketId& MarketId, UMarketDataComponent* MarketData, UPriceFluctuationSystem* PriceSystem)
{
	FString Key = GetMarketKey(MarketId);
	FName KeyName = FName(*Key);

	if (MarketData)
	{
		MarketDataComponents.Add(KeyName, MarketData);
		
		// Store market location
		FMarketData Data = MarketData->GetMarketData();
		MarketLocations.Add(KeyName, Data.WorldLocation);
	}

	if (PriceSystem)
	{
		PriceSystems.Add(KeyName, PriceSystem);
	}
}

void UTradeRouteAnalyzer::UnregisterMarket(const FMarketId& MarketId)
{
	FString Key = GetMarketKey(MarketId);
	FName KeyName = FName(*Key);

	MarketDataComponents.Remove(KeyName);
	PriceSystems.Remove(KeyName);
	MarketLocations.Remove(KeyName);

	// Remove routes involving this market
	TradeRoutes.RemoveAll([&MarketId](const FTradeRoute& Route) {
		return Route.SourceMarket == MarketId || Route.DestinationMarket == MarketId;
	});
}

void UTradeRouteAnalyzer::DefineTradeRoute(const FMarketId& Source, const FMarketId& Destination, float Distance, float TravelTime, ETradeRouteRisk Risk)
{
	// Check if route already exists
	for (FTradeRoute& Route : TradeRoutes)
	{
		if (Route.SourceMarket == Source && Route.DestinationMarket == Destination)
		{
			// Update existing route
			Route.Distance = Distance;
			Route.TravelTime = TravelTime;
			Route.RiskLevel = Risk;
			return;
		}
	}

	// Add new route
	FTradeRoute NewRoute;
	NewRoute.SourceMarket = Source;
	NewRoute.DestinationMarket = Destination;
	NewRoute.Distance = Distance;
	NewRoute.TravelTime = TravelTime;
	NewRoute.RiskLevel = Risk;
	NewRoute.bIsActive = true;

	TradeRoutes.Add(NewRoute);
}

void UTradeRouteAnalyzer::GenerateAllRoutes(float BaseSpeedUnitsPerHour)
{
	TArray<FName> MarketKeys;
	MarketLocations.GetKeys(MarketKeys);

	for (int32 i = 0; i < MarketKeys.Num(); ++i)
	{
		for (int32 j = i + 1; j < MarketKeys.Num(); ++j)
		{
			FVector* LocA = MarketLocations.Find(MarketKeys[i]);
			FVector* LocB = MarketLocations.Find(MarketKeys[j]);

			if (LocA && LocB)
			{
				float Distance = FVector::Distance(*LocA, *LocB);
				float TravelTime = Distance / BaseSpeedUnitsPerHour;

				// Assign risk based on distance
				ETradeRouteRisk Risk = ETradeRouteRisk::Safe;
				if (Distance > 10000.0f) Risk = ETradeRouteRisk::Dangerous;
				else if (Distance > 5000.0f) Risk = ETradeRouteRisk::High;
				else if (Distance > 2000.0f) Risk = ETradeRouteRisk::Moderate;
				else if (Distance > 500.0f) Risk = ETradeRouteRisk::Low;

				// Create routes in both directions
				FMarketId MarketA, MarketB;
				MarketA.MarketName = MarketKeys[i];
				MarketB.MarketName = MarketKeys[j];

				DefineTradeRoute(MarketA, MarketB, Distance, TravelTime, Risk);
				DefineTradeRoute(MarketB, MarketA, Distance, TravelTime, Risk);
			}
		}
	}
}

// ============================================================================
// ROUTE ANALYSIS
// ============================================================================

FRouteAnalysisResult UTradeRouteAnalyzer::AnalyzeRoute(const FMarketId& Source, const FMarketId& Destination) const
{
	FRouteAnalysisResult Result;
	Result.SourceMarket = Source;
	Result.DestinationMarket = Destination;
	Result.AnalysisTime = FPlatformTime::Seconds();

	// Find the route
	const FTradeRoute* Route = nullptr;
	for (const FTradeRoute& R : TradeRoutes)
	{
		if (R.SourceMarket == Source && R.DestinationMarket == Destination)
		{
			Route = &R;
			break;
		}
	}

	if (!Route)
	{
		Result.bIsViable = false;
		return Result;
	}

	Result.RouteName = FString::Printf(TEXT("%s to %s"), 
		*Source.MarketName.ToString(), 
		*Destination.MarketName.ToString());

	// Analyze each resource
	TArray<EResourceType> Resources = {
		EResourceType::Silicate,
		EResourceType::Carbon,
		EResourceType::RefinedSilicate,
		EResourceType::RefinedCarbon,
		EResourceType::CompositeMaterial
	};

	float BestScore = 0.0f;
	for (EResourceType Resource : Resources)
	{
		int32 BuyPrice = GetBuyPriceAt(Source, Resource);
		int32 SellPrice = GetSellPriceAt(Destination, Resource);
		int32 Supply = GetSupplyAt(Source, Resource);

		if (BuyPrice > 0 && SellPrice > BuyPrice)
		{
			FTradeOpportunity Opp = CalculateOpportunity(*Route, Resource, BuyPrice, SellPrice, Supply);
			Result.Opportunities.Add(Resource, Opp);

			if (Opp.OpportunityScore > BestScore)
			{
				BestScore = Opp.OpportunityScore;
				Result.BestResource = Resource;
				Result.MaxPotentialProfit = Opp.MaxProfitPotential;
			}
		}
	}

	Result.bIsViable = Result.Opportunities.Num() > 0;
	Result.OverallRouteScore = BestScore;

	return Result;
}

TArray<FRouteAnalysisResult> UTradeRouteAnalyzer::AnalyzeRoutesFrom(const FMarketId& Source) const
{
	TArray<FRouteAnalysisResult> Results;

	for (const FTradeRoute& Route : TradeRoutes)
	{
		if (Route.SourceMarket == Source)
		{
			FRouteAnalysisResult Result = AnalyzeRoute(Source, Route.DestinationMarket);
			if (Result.bIsViable)
			{
				Results.Add(Result);
			}
		}
	}

	// Sort by score
	Results.Sort([](const FRouteAnalysisResult& A, const FRouteAnalysisResult& B) {
		return A.OverallRouteScore > B.OverallRouteScore;
	});

	return Results;
}

TArray<FRouteAnalysisResult> UTradeRouteAnalyzer::AnalyzeRoutesTo(const FMarketId& Destination) const
{
	TArray<FRouteAnalysisResult> Results;

	for (const FTradeRoute& Route : TradeRoutes)
	{
		if (Route.DestinationMarket == Destination)
		{
			FRouteAnalysisResult Result = AnalyzeRoute(Route.SourceMarket, Destination);
			if (Result.bIsViable)
			{
				Results.Add(Result);
			}
		}
	}

	Results.Sort([](const FRouteAnalysisResult& A, const FRouteAnalysisResult& B) {
		return A.OverallRouteScore > B.OverallRouteScore;
	});

	return Results;
}

void UTradeRouteAnalyzer::AnalyzeAllRoutes()
{
	CurrentOpportunities.Empty();

	for (const FTradeRoute& Route : TradeRoutes)
	{
		if (!Route.bIsActive) continue;

		TArray<EResourceType> Resources = {
			EResourceType::Silicate,
			EResourceType::Carbon,
			EResourceType::RefinedSilicate,
			EResourceType::RefinedCarbon,
			EResourceType::CompositeMaterial
		};

		for (EResourceType Resource : Resources)
		{
			int32 BuyPrice = GetBuyPriceAt(Route.SourceMarket, Resource);
			int32 SellPrice = GetSellPriceAt(Route.DestinationMarket, Resource);
			int32 Supply = GetSupplyAt(Route.SourceMarket, Resource);

			if (BuyPrice > 0 && SellPrice > BuyPrice)
			{
				float Margin = static_cast<float>(SellPrice - BuyPrice) / static_cast<float>(BuyPrice);
				
				if (Margin >= MinProfitMarginThreshold)
				{
					FTradeOpportunity Opp = CalculateOpportunity(Route, Resource, BuyPrice, SellPrice, Supply);
					CurrentOpportunities.Add(Opp);
				}
			}
		}
	}

	SortOpportunities();

	// Trim to max
	while (CurrentOpportunities.Num() > MaxTrackedOpportunities)
	{
		CurrentOpportunities.RemoveAt(CurrentOpportunities.Num() - 1);
	}
}

FTradeOpportunity UTradeRouteAnalyzer::GetOpportunity(const FMarketId& Source, const FMarketId& Destination, EResourceType Resource) const
{
	for (const FTradeOpportunity& Opp : CurrentOpportunities)
	{
		if (Opp.Route.SourceMarket == Source && 
		    Opp.Route.DestinationMarket == Destination && 
		    Opp.Resource == Resource)
		{
			return Opp;
		}
	}
	return FTradeOpportunity();
}

// ============================================================================
// OPPORTUNITY DISCOVERY
// ============================================================================

TArray<FTradeOpportunity> UTradeRouteAnalyzer::GetTopOpportunities(int32 MaxCount) const
{
	TArray<FTradeOpportunity> Result;
	int32 Count = FMath::Min(MaxCount, CurrentOpportunities.Num());
	
	for (int32 i = 0; i < Count; ++i)
	{
		Result.Add(CurrentOpportunities[i]);
	}
	
	return Result;
}

TArray<FTradeOpportunity> UTradeRouteAnalyzer::GetOpportunitiesFrom(const FMarketId& Source, int32 MaxCount) const
{
	TArray<FTradeOpportunity> Result;
	
	for (const FTradeOpportunity& Opp : CurrentOpportunities)
	{
		if (Opp.Route.SourceMarket == Source)
		{
			Result.Add(Opp);
			if (Result.Num() >= MaxCount) break;
		}
	}
	
	return Result;
}

TArray<FTradeOpportunity> UTradeRouteAnalyzer::GetOpportunitiesForResource(EResourceType Resource, int32 MaxCount) const
{
	TArray<FTradeOpportunity> Result;
	
	for (const FTradeOpportunity& Opp : CurrentOpportunities)
	{
		if (Opp.Resource == Resource)
		{
			Result.Add(Opp);
			if (Result.Num() >= MaxCount) break;
		}
	}
	
	return Result;
}

TArray<FTradeOpportunity> UTradeRouteAnalyzer::FindArbitrageOpportunities(float MinProfitMargin) const
{
	TArray<FTradeOpportunity> Result;
	
	for (const FTradeOpportunity& Opp : CurrentOpportunities)
	{
		if (Opp.ProfitMarginPercent >= MinProfitMargin * 100.0f)
		{
			Result.Add(Opp);
		}
	}
	
	// Sort by profit margin
	Result.Sort([](const FTradeOpportunity& A, const FTradeOpportunity& B) {
		return A.ProfitMarginPercent > B.ProfitMarginPercent;
	});
	
	return Result;
}

TArray<FTradeOpportunity> UTradeRouteAnalyzer::GetFilteredOpportunities(
	float MinProfitMargin,
	ETradeRouteRisk MaxRisk,
	float MaxTravelTime,
	int32 MaxCount) const
{
	TArray<FTradeOpportunity> Result;
	
	for (const FTradeOpportunity& Opp : CurrentOpportunities)
	{
		if (Opp.ProfitMarginPercent >= MinProfitMargin * 100.0f &&
		    static_cast<uint8>(Opp.Route.RiskLevel) <= static_cast<uint8>(MaxRisk) &&
		    Opp.Route.TravelTime <= MaxTravelTime)
		{
			Result.Add(Opp);
			if (Result.Num() >= MaxCount) break;
		}
	}
	
	return Result;
}

// ============================================================================
// PROFIT CALCULATION
// ============================================================================

int32 UTradeRouteAnalyzer::CalculateNetProfit(
	const FMarketId& Source,
	const FMarketId& Destination,
	EResourceType Resource,
	int32 Quantity) const
{
	int32 BuyPrice = GetBuyPriceAt(Source, Resource);
	int32 SellPrice = GetSellPriceAt(Destination, Resource);
	
	int32 TotalCost = BuyPrice * Quantity;
	int32 TotalRevenue = SellPrice * Quantity;
	
	return TotalRevenue - TotalCost;
}

int32 UTradeRouteAnalyzer::CalculateNetProfitAfterCosts(
	const FMarketId& Source,
	const FMarketId& Destination,
	EResourceType Resource,
	int32 Quantity,
	float FuelCostPerUnit) const
{
	int32 GrossProfit = CalculateNetProfit(Source, Destination, Resource, Quantity);
	
	// Get route for fuel calculation
	FTradeRoute Route = GetRoute(Source, Destination);
	int32 FuelCost = FMath::RoundToInt(Route.Distance * FuelCostPerUnit);
	
	// Estimate taxes (5% of transaction value)
	int32 TaxCost = FMath::RoundToInt(GetBuyPriceAt(Source, Resource) * Quantity * 0.05f);
	
	return GrossProfit - FuelCost - TaxCost;
}

int32 UTradeRouteAnalyzer::CalculateOptimalQuantity(
	const FMarketId& Source,
	const FMarketId& Destination,
	EResourceType Resource,
	int32 AvailableCapital,
	int32 CargoCapacity) const
{
	int32 BuyPrice = GetBuyPriceAt(Source, Resource);
	if (BuyPrice <= 0) return 0;
	
	int32 Supply = GetSupplyAt(Source, Resource);
	
	// Calculate max affordable
	int32 MaxAffordable = AvailableCapital / BuyPrice;
	
	// Take minimum of all constraints
	return FMath::Min3(MaxAffordable, CargoCapacity, Supply);
}

int32 UTradeRouteAnalyzer::CalculateRoundTripProfit(
	const FMarketId& MarketA,
	const FMarketId& MarketB,
	int32 Capital,
	int32 CargoCapacity) const
{
	int32 TotalProfit = 0;
	int32 RemainingCapital = Capital;

	// Find best resource A -> B
	EResourceType BestAtoB = EResourceType::None;
	float BestMarginAtoB = 0.0f;
	
	TArray<EResourceType> Resources = {
		EResourceType::Silicate,
		EResourceType::Carbon,
		EResourceType::RefinedSilicate,
		EResourceType::RefinedCarbon,
		EResourceType::CompositeMaterial
	};

	for (EResourceType Resource : Resources)
	{
		float Margin = GetPriceDifferential(MarketA, MarketB, Resource);
		if (Margin > BestMarginAtoB)
		{
			BestMarginAtoB = Margin;
			BestAtoB = Resource;
		}
	}

	// Calculate first leg profit
	if (BestAtoB != EResourceType::None && BestMarginAtoB > 0)
	{
		int32 Quantity = CalculateOptimalQuantity(MarketA, MarketB, BestAtoB, RemainingCapital, CargoCapacity);
		int32 Profit = CalculateNetProfit(MarketA, MarketB, BestAtoB, Quantity);
		TotalProfit += Profit;
		RemainingCapital += Profit;
	}

	// Find best resource B -> A
	EResourceType BestBtoA = EResourceType::None;
	float BestMarginBtoA = 0.0f;

	for (EResourceType Resource : Resources)
	{
		float Margin = GetPriceDifferential(MarketB, MarketA, Resource);
		if (Margin > BestMarginBtoA)
		{
			BestMarginBtoA = Margin;
			BestBtoA = Resource;
		}
	}

	// Calculate return leg profit
	if (BestBtoA != EResourceType::None && BestMarginBtoA > 0)
	{
		int32 Quantity = CalculateOptimalQuantity(MarketB, MarketA, BestBtoA, RemainingCapital, CargoCapacity);
		int32 Profit = CalculateNetProfit(MarketB, MarketA, BestBtoA, Quantity);
		TotalProfit += Profit;
	}

	return TotalProfit;
}

// ============================================================================
// ROUTE INFORMATION
// ============================================================================

TArray<FTradeRoute> UTradeRouteAnalyzer::GetAllRoutes() const
{
	return TradeRoutes;
}

TArray<FTradeRoute> UTradeRouteAnalyzer::GetRoutesFrom(const FMarketId& Source) const
{
	TArray<FTradeRoute> Result;
	
	for (const FTradeRoute& Route : TradeRoutes)
	{
		if (Route.SourceMarket == Source)
		{
			Result.Add(Route);
		}
	}
	
	return Result;
}

FTradeRoute UTradeRouteAnalyzer::GetRoute(const FMarketId& Source, const FMarketId& Destination) const
{
	for (const FTradeRoute& Route : TradeRoutes)
	{
		if (Route.SourceMarket == Source && Route.DestinationMarket == Destination)
		{
			return Route;
		}
	}
	return FTradeRoute();
}

bool UTradeRouteAnalyzer::HasRoute(const FMarketId& Source, const FMarketId& Destination) const
{
	for (const FTradeRoute& Route : TradeRoutes)
	{
		if (Route.SourceMarket == Source && Route.DestinationMarket == Destination)
		{
			return true;
		}
	}
	return false;
}

FTradeRoute UTradeRouteAnalyzer::GetSafestRoute(const FMarketId& Source, const FMarketId& Destination) const
{
	FTradeRoute Safest;
	Safest.RiskLevel = ETradeRouteRisk::Dangerous;
	
	for (const FTradeRoute& Route : TradeRoutes)
	{
		if (Route.SourceMarket == Source && Route.DestinationMarket == Destination)
		{
			if (static_cast<uint8>(Route.RiskLevel) < static_cast<uint8>(Safest.RiskLevel))
			{
				Safest = Route;
			}
		}
	}
	
	return Safest;
}

FTradeRoute UTradeRouteAnalyzer::GetFastestRoute(const FMarketId& Source, const FMarketId& Destination) const
{
	FTradeRoute Fastest;
	Fastest.TravelTime = FLT_MAX;
	
	for (const FTradeRoute& Route : TradeRoutes)
	{
		if (Route.SourceMarket == Source && Route.DestinationMarket == Destination)
		{
			if (Route.TravelTime < Fastest.TravelTime)
			{
				Fastest = Route;
			}
		}
	}
	
	return Fastest;
}

// ============================================================================
// MARKET COMPARISON
// ============================================================================

TMap<EResourceType, float> UTradeRouteAnalyzer::ComparePrices(const FMarketId& MarketA, const FMarketId& MarketB) const
{
	TMap<EResourceType, float> Result;
	
	TArray<EResourceType> Resources = {
		EResourceType::Silicate,
		EResourceType::Carbon,
		EResourceType::RefinedSilicate,
		EResourceType::RefinedCarbon,
		EResourceType::CompositeMaterial
	};

	for (EResourceType Resource : Resources)
	{
		float Differential = GetPriceDifferential(MarketA, MarketB, Resource);
		Result.Add(Resource, Differential);
	}
	
	return Result;
}

FMarketId UTradeRouteAnalyzer::FindBestBuyMarket(EResourceType Resource) const
{
	FMarketId BestMarket;
	int32 LowestPrice = INT32_MAX;
	
	for (const auto& Pair : PriceSystems)
	{
		if (Pair.Value)
		{
			int32 Price = Pair.Value->CalculateBuyPrice(Resource);
			if (Price < LowestPrice && Price > 0)
			{
				LowestPrice = Price;
				BestMarket.MarketName = Pair.Key;
			}
		}
	}
	
	return BestMarket;
}

FMarketId UTradeRouteAnalyzer::FindBestSellMarket(EResourceType Resource) const
{
	FMarketId BestMarket;
	int32 HighestPrice = 0;
	
	for (const auto& Pair : PriceSystems)
	{
		if (Pair.Value)
		{
			int32 Price = Pair.Value->CalculateSellPrice(Resource);
			if (Price > HighestPrice)
			{
				HighestPrice = Price;
				BestMarket.MarketName = Pair.Key;
			}
		}
	}
	
	return BestMarket;
}

float UTradeRouteAnalyzer::GetPriceDifferential(const FMarketId& Source, const FMarketId& Destination, EResourceType Resource) const
{
	int32 BuyPrice = GetBuyPriceAt(Source, Resource);
	int32 SellPrice = GetSellPriceAt(Destination, Resource);
	
	if (BuyPrice <= 0) return 0.0f;
	
	return static_cast<float>(SellPrice - BuyPrice) / static_cast<float>(BuyPrice);
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

void UTradeRouteAnalyzer::UpdateOpportunities()
{
	TArray<FTradeOpportunity> OldOpportunities = CurrentOpportunities;
	AnalyzeAllRoutes();
	
	// Notify for new high-value opportunities
	for (const FTradeOpportunity& NewOpp : CurrentOpportunities)
	{
		if (NewOpp.OpportunityScore > 0.7f) // High-value threshold
		{
			bool bIsNew = true;
			for (const FTradeOpportunity& OldOpp : OldOpportunities)
			{
				if (OldOpp.Route.SourceMarket == NewOpp.Route.SourceMarket &&
				    OldOpp.Route.DestinationMarket == NewOpp.Route.DestinationMarket &&
				    OldOpp.Resource == NewOpp.Resource)
				{
					bIsNew = false;
					break;
				}
			}
			
			if (bIsNew)
			{
				OnOpportunityFound.Broadcast(NewOpp);
			}
		}
	}
}

FTradeOpportunity UTradeRouteAnalyzer::CalculateOpportunity(
	const FTradeRoute& Route,
	EResourceType Resource,
	int32 SourceBuyPrice,
	int32 DestSellPrice,
	int32 AvailableSupply) const
{
	FTradeOpportunity Opp;
	Opp.Route = Route;
	Opp.Resource = Resource;
	Opp.BuyPrice = SourceBuyPrice;
	Opp.SellPrice = DestSellPrice;
	Opp.AvailableQuantity = AvailableSupply;
	Opp.ExpirationTime = FPlatformTime::Seconds() + OpportunityExpirationSeconds;
	
	Opp.CalculateMetrics();
	
	return Opp;
}

void UTradeRouteAnalyzer::SortOpportunities()
{
	CurrentOpportunities.Sort([](const FTradeOpportunity& A, const FTradeOpportunity& B) {
		return A.OpportunityScore > B.OpportunityScore;
	});
}

void UTradeRouteAnalyzer::PruneExpiredOpportunities()
{
	double CurrentTime = FPlatformTime::Seconds();
	
	CurrentOpportunities.RemoveAll([CurrentTime](const FTradeOpportunity& Opp) {
		return CurrentTime >= Opp.ExpirationTime;
	});
}

FString UTradeRouteAnalyzer::GetMarketKey(const FMarketId& MarketId) const
{
	return MarketId.ToString();
}

int32 UTradeRouteAnalyzer::GetBuyPriceAt(const FMarketId& Market, EResourceType Resource) const
{
	FName Key = FName(*GetMarketKey(Market));
	UPriceFluctuationSystem* const* PriceSystem = PriceSystems.Find(Key);
	
	if (PriceSystem && *PriceSystem)
	{
		return (*PriceSystem)->CalculateBuyPrice(Resource);
	}
	
	return 0;
}

int32 UTradeRouteAnalyzer::GetSellPriceAt(const FMarketId& Market, EResourceType Resource) const
{
	FName Key = FName(*GetMarketKey(Market));
	UPriceFluctuationSystem* const* PriceSystem = PriceSystems.Find(Key);
	
	if (PriceSystem && *PriceSystem)
	{
		return (*PriceSystem)->CalculateSellPrice(Resource);
	}
	
	return 0;
}

int32 UTradeRouteAnalyzer::GetSupplyAt(const FMarketId& Market, EResourceType Resource) const
{
	FName Key = FName(*GetMarketKey(Market));
	UMarketDataComponent* const* MarketData = MarketDataComponents.Find(Key);
	
	if (MarketData && *MarketData)
	{
		return (*MarketData)->GetCurrentSupply(Resource);
	}
	
	return 0;
}
