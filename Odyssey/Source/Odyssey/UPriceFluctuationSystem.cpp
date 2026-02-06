// UPriceFluctuationSystem.cpp
// Implementation of dynamic pricing engine

#include "UPriceFluctuationSystem.h"
#include "Engine/World.h"

UPriceFluctuationSystem::UPriceFluctuationSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f; // Update twice per second

	MarketDataComponent = nullptr;
	TimeSinceLastUpdate = 0.0f;
	TimeSinceLastVolatilityUpdate = 0.0f;

	// Initialize random stream with seed based on time
	PriceRandomStream.Initialize(FMath::Rand());
}

void UPriceFluctuationSystem::BeginPlay()
{
	Super::BeginPlay();

	InitializeDefaultPrices();
}

void UPriceFluctuationSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastUpdate += DeltaTime;
	TimeSinceLastVolatilityUpdate += DeltaTime;

	// Update event modifiers
	UpdateEventModifiers(DeltaTime);

	// Decay price shocks
	DecayPriceShocks(DeltaTime);

	// Periodic price updates
	if (TimeSinceLastUpdate >= EconomyConfig.PriceUpdateIntervalSeconds)
	{
		UpdateAllPrices();
		TimeSinceLastUpdate = 0.0f;
	}
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void UPriceFluctuationSystem::Initialize(UMarketDataComponent* InMarketData)
{
	MarketDataComponent = InMarketData;

	if (MarketDataComponent)
	{
		// Initialize prices from market data
		TMap<EResourceType, FResourceSupplyDemand> SupplyDemandData = MarketDataComponent->GetAllSupplyDemandData();
		for (const auto& Pair : SupplyDemandData)
		{
			UpdateResourcePrice(Pair.Key);
		}
	}
}

void UPriceFluctuationSystem::SetConfiguration(const FEconomyConfiguration& Config)
{
	EconomyConfig = Config;
}

// ============================================================================
// PRICE CALCULATION
// ============================================================================

int32 UPriceFluctuationSystem::CalculateBuyPrice(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	if (PriceData)
	{
		return PriceData->CurrentBuyPrice;
	}
	return 10; // Default fallback
}

int32 UPriceFluctuationSystem::CalculateSellPrice(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	if (PriceData)
	{
		return PriceData->CurrentSellPrice;
	}
	return 8; // Default fallback
}

int32 UPriceFluctuationSystem::CalculateBuyPriceForQuantity(EResourceType Resource, int32 Quantity) const
{
	int32 UnitPrice = CalculateBuyPrice(Resource);

	// Apply volume scaling - larger orders cost slightly more per unit (market impact)
	float VolumeMultiplier = 1.0f;
	if (Quantity > 100)
	{
		VolumeMultiplier = 1.0f + (static_cast<float>(Quantity - 100) / 1000.0f) * 0.1f;
	}

	return FMath::RoundToInt(UnitPrice * Quantity * VolumeMultiplier);
}

int32 UPriceFluctuationSystem::CalculateSellPriceForQuantity(EResourceType Resource, int32 Quantity) const
{
	int32 UnitPrice = CalculateSellPrice(Resource);

	// Apply volume scaling - larger sells get slightly less per unit (market impact)
	float VolumeMultiplier = 1.0f;
	if (Quantity > 100)
	{
		VolumeMultiplier = 1.0f - (static_cast<float>(Quantity - 100) / 1000.0f) * 0.1f;
		VolumeMultiplier = FMath::Max(0.8f, VolumeMultiplier);
	}

	return FMath::RoundToInt(UnitPrice * Quantity * VolumeMultiplier);
}

FPriceCalculationResult UPriceFluctuationSystem::GetPriceCalculationDetails(EResourceType Resource) const
{
	FPriceCalculationResult Result;
	Result.Resource = Resource;

	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	if (!PriceData)
	{
		return Result;
	}

	Result.BasePrice = PriceData->BasePrice;
	Result.SupplyDemandFactor = CalculateSupplyDemandFactor(Resource);
	Result.VolatilityFactor = PriceData->CurrentVolatilityFactor;
	Result.TrendFactor = CalculateTrendFactor(Resource);
	
	// Get event modifier
	const TArray<TPair<float, double>>* Modifiers = ActiveEventModifiers.Find(Resource);
	Result.EventModifier = 1.0f;
	if (Modifiers)
	{
		for (const auto& Mod : *Modifiers)
		{
			Result.EventModifier *= Mod.Key;
		}
	}

	// Get specialization modifier
	Result.SpecializationModifier = 1.0f;
	if (MarketDataComponent && MarketDataComponent->IsSpecializedIn(Resource))
	{
		Result.SpecializationModifier = MarketDataComponent->GetSpecializationBonus();
	}

	Result.FinalMultiplier = Result.SupplyDemandFactor * 
	                         (1.0f + Result.VolatilityFactor) * 
	                         Result.TrendFactor * 
	                         Result.EventModifier;

	Result.CalculatedPrice = FMath::RoundToInt(Result.BasePrice * Result.FinalMultiplier);
	Result.ClampedPrice = FMath::Clamp(Result.CalculatedPrice, PriceData->MinPrice, PriceData->MaxPrice);

	return Result;
}

float UPriceFluctuationSystem::GetCurrentPriceMultiplier(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	return PriceData ? PriceData->PriceMultiplier : 1.0f;
}

// ============================================================================
// MARKET DYNAMICS
// ============================================================================

void UPriceFluctuationSystem::UpdateAllPrices()
{
	// Limit updates per tick for mobile performance
	int32 UpdateCount = 0;
	
	for (auto& Pair : CurrentPrices)
	{
		if (UpdateCount >= EconomyConfig.MaxMarketsToUpdatePerTick)
		{
			break;
		}

		UpdateResourcePrice(Pair.Key);
		UpdateCount++;
	}
}

void UPriceFluctuationSystem::UpdateResourcePrice(EResourceType Resource)
{
	FDynamicMarketPrice& PriceData = GetOrCreatePrice(Resource);

	// Calculate new price multiplier
	float TargetMultiplier = CalculateBaseMultiplier(Resource);

	// Apply price smoothing to prevent jarring changes
	float SmoothedMultiplier = ApplyPriceSmoothing(TargetMultiplier, PriceData.PriceMultiplier);

	// Add volatility noise
	float VolatilityNoise = GenerateVolatilityFactor(Resource);
	PriceData.CurrentVolatilityFactor = VolatilityNoise;

	// Calculate final multiplier
	float FinalMultiplier = SmoothedMultiplier * (1.0f + VolatilityNoise);

	// Apply event modifiers
	const TArray<TPair<float, double>>* Modifiers = ActiveEventModifiers.Find(Resource);
	if (Modifiers)
	{
		for (const auto& Mod : *Modifiers)
		{
			FinalMultiplier *= Mod.Key;
		}
	}

	// Apply price shocks
	const TPair<float, float>* Shock = ActivePriceShocks.Find(Resource);
	if (Shock)
	{
		FinalMultiplier *= Shock->Key;
	}

	// Update stored multiplier
	PriceData.PriceMultiplier = FinalMultiplier;

	// Calculate actual prices
	int32 BasePrice = PriceData.BasePrice;
	int32 NewPrice = FMath::RoundToInt(BasePrice * FinalMultiplier);
	NewPrice = FMath::Clamp(NewPrice, PriceData.MinPrice, PriceData.MaxPrice);

	// Update buy/sell prices with spread
	int32 OldBuyPrice = PriceData.CurrentBuyPrice;
	PriceData.CurrentBuyPrice = FMath::RoundToInt(NewPrice * (1.0f + PriceData.BuySpreadPercent));
	PriceData.CurrentSellPrice = FMath::RoundToInt(NewPrice * (1.0f - PriceData.SellSpreadPercent));

	// Ensure sell price doesn't exceed buy price
	PriceData.CurrentSellPrice = FMath::Min(PriceData.CurrentSellPrice, PriceData.CurrentBuyPrice - 1);
	PriceData.CurrentSellPrice = FMath::Max(PriceData.CurrentSellPrice, PriceData.MinPrice);

	// Update trend
	if (MarketDataComponent)
	{
		PriceData.CurrentTrend = MarketDataComponent->GetPriceTrend(Resource);
	}

	// Broadcast change if significant
	float PriceChangePercent = FMath::Abs(static_cast<float>(PriceData.CurrentBuyPrice - OldBuyPrice) / 
	                                       static_cast<float>(FMath::Max(1, OldBuyPrice)));
	if (PriceChangePercent >= EconomyConfig.MinPriceChangePercent)
	{
		OnPriceChanged.Broadcast(Resource, PriceData);

		// Record in history
		if (MarketDataComponent)
		{
			MarketDataComponent->RecordPricePoint(Resource, NewPrice, 0);
		}
	}
}

void UPriceFluctuationSystem::ApplyPriceShock(EResourceType Resource, float ShockMultiplier, float DecayRate)
{
	ActivePriceShocks.Add(Resource, TPair<float, float>(ShockMultiplier, DecayRate));
	
	// Immediately update the price
	UpdateResourcePrice(Resource);
}

void UPriceFluctuationSystem::SetResourceVolatility(EResourceType Resource, EMarketVolatility Volatility)
{
	FDynamicMarketPrice& PriceData = GetOrCreatePrice(Resource);
	PriceData.Volatility = Volatility;
}

void UPriceFluctuationSystem::ApplyEventModifier(EResourceType Resource, float Modifier, float Duration)
{
	double ExpirationTime = FPlatformTime::Seconds() + Duration;

	if (!ActiveEventModifiers.Contains(Resource))
	{
		ActiveEventModifiers.Add(Resource, TArray<TPair<float, double>>());
	}

	ActiveEventModifiers[Resource].Add(TPair<float, double>(Modifier, ExpirationTime));
}

void UPriceFluctuationSystem::ClearEventModifiers(EResourceType Resource)
{
	ActiveEventModifiers.Remove(Resource);
}

// ============================================================================
// SUPPLY/DEMAND FACTORS
// ============================================================================

float UPriceFluctuationSystem::CalculateSupplyDemandFactor(EResourceType Resource) const
{
	if (!MarketDataComponent)
	{
		return 1.0f;
	}

	float SDRatio = MarketDataComponent->GetSupplyDemandRatio(Resource);
	
	// Inverse relationship: high supply ratio = lower prices
	// SDRatio > 1 means oversupply, SDRatio < 1 means undersupply
	
	// Use logarithmic scaling for smoother transitions
	float Factor = 1.0f;
	if (SDRatio > 0.0f)
	{
		// Log scale with influence setting
		Factor = 1.0f / FMath::Pow(SDRatio, EconomyConfig.SupplyDemandPriceInfluence);
	}

	// Clamp to reasonable bounds
	return FMath::Clamp(Factor, 0.25f, 4.0f);
}

float UPriceFluctuationSystem::CalculateScarcityPremium(EResourceType Resource) const
{
	if (!MarketDataComponent)
	{
		return 0.0f;
	}

	float Scarcity = MarketDataComponent->GetScarcityIndex(Resource);
	
	// Exponential premium for very scarce resources
	if (Scarcity > 0.7f)
	{
		return FMath::Pow(Scarcity - 0.7f, 2.0f) * 5.0f;
	}
	else if (Scarcity > 0.5f)
	{
		return (Scarcity - 0.5f) * 0.5f;
	}

	return 0.0f;
}

float UPriceFluctuationSystem::CalculateAbundanceDiscount(EResourceType Resource) const
{
	if (!MarketDataComponent)
	{
		return 0.0f;
	}

	float Scarcity = MarketDataComponent->GetScarcityIndex(Resource);
	
	// Discount for abundant resources
	if (Scarcity < 0.3f)
	{
		return (0.3f - Scarcity) * 0.5f;
	}

	return 0.0f;
}

// ============================================================================
// VOLATILITY SIMULATION
// ============================================================================

float UPriceFluctuationSystem::GenerateVolatilityFactor(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	if (!PriceData)
	{
		return 0.0f;
	}

	float VolatilityRange = GetVolatilityMultiplierRange(PriceData->Volatility);
	
	// Generate random factor within volatility range
	float RandomValue = PriceRandomStream.FRandRange(-1.0f, 1.0f);
	
	// Add market noise
	float Noise = SimulateMarketNoise(VolatilityRange);
	
	return (RandomValue * VolatilityRange) + Noise;
}

float UPriceFluctuationSystem::GetVolatilityRange(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	if (PriceData)
	{
		return GetVolatilityMultiplierRange(PriceData->Volatility);
	}
	return EconomyConfig.BaseVolatilityPercent;
}

float UPriceFluctuationSystem::SimulateMarketNoise(float BaseVolatility) const
{
	// Simulate micro-fluctuations using multiple random sources
	float Noise = 0.0f;
	
	// Fast noise component
	Noise += PriceRandomStream.FRandRange(-1.0f, 1.0f) * BaseVolatility * 0.3f;
	
	// Slow drift component
	float Time = FPlatformTime::Seconds();
	Noise += FMath::Sin(Time * 0.1f) * BaseVolatility * 0.2f;
	
	return Noise;
}

// ============================================================================
// TREND ANALYSIS
// ============================================================================

float UPriceFluctuationSystem::CalculateTrendFactor(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	if (!PriceData)
	{
		return 1.0f;
	}

	// Apply trend-based momentum to prices
	switch (PriceData->CurrentTrend)
	{
		case EMarketTrend::StrongBull:
			return 1.0f + (PriceData->TrendStrength * 0.15f);
		case EMarketTrend::Bull:
			return 1.0f + (PriceData->TrendStrength * 0.05f);
		case EMarketTrend::StrongBear:
			return 1.0f - (PriceData->TrendStrength * 0.15f);
		case EMarketTrend::Bear:
			return 1.0f - (PriceData->TrendStrength * 0.05f);
		default:
			return 1.0f;
	}
}

float UPriceFluctuationSystem::GetTrendMomentum(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	return PriceData ? PriceData->TrendMomentum : 0.0f;
}

int32 UPriceFluctuationSystem::PredictFuturePrice(EResourceType Resource, float HoursAhead) const
{
	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	if (!PriceData)
	{
		return 0;
	}

	// Simple linear extrapolation based on trend
	float TrendFactor = CalculateTrendFactor(Resource);
	float MomentumPerHour = PriceData->TrendMomentum;

	float PredictedMultiplier = TrendFactor + (MomentumPerHour * HoursAhead);
	int32 PredictedPrice = FMath::RoundToInt(PriceData->CurrentBuyPrice * PredictedMultiplier);

	return FMath::Clamp(PredictedPrice, PriceData->MinPrice, PriceData->MaxPrice);
}

// ============================================================================
// PRICE HISTORY
// ============================================================================

TMap<EResourceType, FDynamicMarketPrice> UPriceFluctuationSystem::GetAllCurrentPrices() const
{
	return CurrentPrices;
}

FDynamicMarketPrice UPriceFluctuationSystem::GetPriceData(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = CurrentPrices.Find(Resource);
	return PriceData ? *PriceData : FDynamicMarketPrice();
}

void UPriceFluctuationSystem::RecordTrade(EResourceType Resource, int32 Price, int32 Volume, bool bWasBuy)
{
	FDynamicMarketPrice& PriceData = GetOrCreatePrice(Resource);
	PriceData.AddHistoryEntry(Price, Volume, MarketDataComponent ? MarketDataComponent->GetSupplyDemandRatio(Resource) : 1.0f);

	// Trades affect supply/demand
	if (MarketDataComponent)
	{
		if (bWasBuy)
		{
			MarketDataComponent->RemoveSupply(Resource, Volume);
			MarketDataComponent->RegisterPlayerDemand(Resource, Volume);
		}
		else
		{
			MarketDataComponent->AddSupply(Resource, Volume);
		}
	}
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

float UPriceFluctuationSystem::CalculateBaseMultiplier(EResourceType Resource) const
{
	float SDFactor = CalculateSupplyDemandFactor(Resource);
	float ScarcityPremium = CalculateScarcityPremium(Resource);
	float AbundanceDiscount = CalculateAbundanceDiscount(Resource);

	return SDFactor * (1.0f + ScarcityPremium - AbundanceDiscount);
}

float UPriceFluctuationSystem::ApplyPriceSmoothing(float TargetMultiplier, float CurrentMultiplier) const
{
	// Exponential smoothing for gradual price changes
	return FMath::Lerp(CurrentMultiplier, TargetMultiplier, 1.0f - EconomyConfig.PriceSmoothingFactor);
}

float UPriceFluctuationSystem::GetVolatilityMultiplierRange(EMarketVolatility Volatility) const
{
	switch (Volatility)
	{
		case EMarketVolatility::Stable:
			return 0.05f;
		case EMarketVolatility::Low:
			return 0.10f;
		case EMarketVolatility::Moderate:
			return 0.20f;
		case EMarketVolatility::High:
			return 0.35f;
		case EMarketVolatility::Extreme:
			return 0.50f;
		default:
			return 0.10f;
	}
}

void UPriceFluctuationSystem::UpdateEventModifiers(float DeltaTime)
{
	double CurrentTime = FPlatformTime::Seconds();

	for (auto& Pair : ActiveEventModifiers)
	{
		TArray<TPair<float, double>>& Modifiers = Pair.Value;
		
		// Remove expired modifiers
		Modifiers.RemoveAll([CurrentTime](const TPair<float, double>& Mod) {
			return CurrentTime >= Mod.Value;
		});
	}

	// Clean up empty entries
	for (auto It = ActiveEventModifiers.CreateIterator(); It; ++It)
	{
		if (It->Value.Num() == 0)
		{
			It.RemoveCurrent();
		}
	}
}

void UPriceFluctuationSystem::DecayPriceShocks(float DeltaTime)
{
	TArray<EResourceType> ExpiredShocks;

	for (auto& Pair : ActivePriceShocks)
	{
		// Decay shock multiplier toward 1.0
		float& Multiplier = Pair.Value.Key;
		float DecayRate = Pair.Value.Value;

		if (Multiplier > 1.0f)
		{
			Multiplier = FMath::Max(1.0f, Multiplier - (DecayRate * DeltaTime));
		}
		else if (Multiplier < 1.0f)
		{
			Multiplier = FMath::Min(1.0f, Multiplier + (DecayRate * DeltaTime));
		}

		// Mark as expired when back to normal
		if (FMath::IsNearlyEqual(Multiplier, 1.0f, 0.01f))
		{
			ExpiredShocks.Add(Pair.Key);
		}
	}

	// Remove expired shocks
	for (EResourceType Resource : ExpiredShocks)
	{
		ActivePriceShocks.Remove(Resource);
	}
}

void UPriceFluctuationSystem::InitializeDefaultPrices()
{
	// Initialize prices for common resources
	TArray<TPair<EResourceType, int32>> DefaultPrices = {
		{EResourceType::Silicate, 5},
		{EResourceType::Carbon, 8},
		{EResourceType::RefinedSilicate, 25},
		{EResourceType::RefinedCarbon, 35},
		{EResourceType::CompositeMaterial, 100}
	};

	for (const auto& Pair : DefaultPrices)
	{
		FDynamicMarketPrice& Price = GetOrCreatePrice(Pair.Key);
		Price.BasePrice = Pair.Value;
		Price.MinPrice = FMath::Max(1, Pair.Value / 5);
		Price.MaxPrice = Pair.Value * 10;
		Price.CurrentBuyPrice = Pair.Value;
		Price.CurrentSellPrice = FMath::RoundToInt(Pair.Value * 0.8f);
		Price.PriceMultiplier = 1.0f;
		Price.Volatility = EMarketVolatility::Moderate;
		Price.BuySpreadPercent = 0.1f;
		Price.SellSpreadPercent = 0.1f;
	}
}

FDynamicMarketPrice& UPriceFluctuationSystem::GetOrCreatePrice(EResourceType Resource)
{
	if (!CurrentPrices.Contains(Resource))
	{
		FDynamicMarketPrice NewPrice;
		NewPrice.ResourceType = Resource;
		NewPrice.MaxHistoryEntries = EconomyConfig.MaxPriceHistoryEntries;
		CurrentPrices.Add(Resource, NewPrice);
	}
	return CurrentPrices[Resource];
}
