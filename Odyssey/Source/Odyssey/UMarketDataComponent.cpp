// UMarketDataComponent.cpp
// Implementation of supply/demand tracking system

#include "UMarketDataComponent.h"
#include "Engine/World.h"

UMarketDataComponent::UMarketDataComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f; // Tick every second for efficiency

	LocationType = EMarketLocationType::Station;
	SpecializationBonus = 1.15f;
	MaxPriceHistoryEntries = 100;
	SimulationTimeScale = 1.0f;
	AccumulatedSimTime = 0.0f;
	LastUpdateTime = 0.0f;
}

void UMarketDataComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeDefaultSupplyDemand();
	UpdateCachedMarketData();
}

void UMarketDataComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Accumulate simulation time (convert real time to game hours)
	float GameHours = (DeltaTime * SimulationTimeScale) / 3600.0f;
	AccumulatedSimTime += GameHours;

	// Simulate every game minute (1/60th of an hour)
	const float SimulationThreshold = 1.0f / 60.0f;
	if (AccumulatedSimTime >= SimulationThreshold)
	{
		SimulateSupplyDemand(AccumulatedSimTime);
		AccumulatedSimTime = 0.0f;
	}
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void UMarketDataComponent::InitializeMarketData(const FMarketId& InMarketId, const FString& InDisplayName)
{
	MarketId = InMarketId;
	DisplayName = InDisplayName;
	InitializeDefaultSupplyDemand();
	UpdateCachedMarketData();
}

void UMarketDataComponent::InitializeFromDataTable(UDataTable* MarketDataTable)
{
	if (!MarketDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMarketDataComponent: No data table provided"));
		return;
	}

	// Load from data table - implementation depends on table structure
	InitializeDefaultSupplyDemand();
	UpdateCachedMarketData();
}

void UMarketDataComponent::InitializeDefaultSupplyDemand()
{
	// Initialize common resources with default values
	TArray<EResourceType> CommonResources = {
		EResourceType::Silicate,
		EResourceType::Carbon,
		EResourceType::RefinedSilicate,
		EResourceType::RefinedCarbon,
		EResourceType::CompositeMaterial
	};

	for (EResourceType Resource : CommonResources)
	{
		FResourceSupplyDemand& SD = GetOrCreateSupplyDemand(Resource);
		SD.ResourceType = Resource;

		// Set defaults based on resource tier
		switch (Resource)
		{
			case EResourceType::Silicate:
			case EResourceType::Carbon:
				SD.CurrentSupply = 500;
				SD.MaxSupply = 2000;
				SD.SupplyRate = 20.0f;
				SD.BaseDemand = 15;
				SD.DemandRate = 15.0f;
				SD.DemandElasticity = 1.2f;
				break;

			case EResourceType::RefinedSilicate:
			case EResourceType::RefinedCarbon:
				SD.CurrentSupply = 200;
				SD.MaxSupply = 1000;
				SD.SupplyRate = 8.0f;
				SD.BaseDemand = 10;
				SD.DemandRate = 10.0f;
				SD.DemandElasticity = 1.0f;
				break;

			case EResourceType::CompositeMaterial:
				SD.CurrentSupply = 50;
				SD.MaxSupply = 500;
				SD.SupplyRate = 3.0f;
				SD.BaseDemand = 5;
				SD.DemandRate = 5.0f;
				SD.DemandElasticity = 0.8f;
				break;

			default:
				SD.CurrentSupply = 100;
				SD.MaxSupply = 1000;
				SD.SupplyRate = 10.0f;
				SD.BaseDemand = 10;
				SD.DemandRate = 10.0f;
				SD.DemandElasticity = 1.0f;
				break;
		}

		SD.SupplyModifier = 1.0f;
		SD.DemandModifier = 1.0f;
		SD.RecalculateMetrics();

		// Initialize price data
		FDynamicMarketPrice& PriceData = GetOrCreatePriceData(Resource);
		PriceData.ResourceType = Resource;

		switch (Resource)
		{
			case EResourceType::Silicate:
				PriceData.BasePrice = 5;
				PriceData.MinPrice = 2;
				PriceData.MaxPrice = 20;
				break;
			case EResourceType::Carbon:
				PriceData.BasePrice = 8;
				PriceData.MinPrice = 3;
				PriceData.MaxPrice = 30;
				break;
			case EResourceType::RefinedSilicate:
				PriceData.BasePrice = 25;
				PriceData.MinPrice = 10;
				PriceData.MaxPrice = 80;
				break;
			case EResourceType::RefinedCarbon:
				PriceData.BasePrice = 35;
				PriceData.MinPrice = 15;
				PriceData.MaxPrice = 100;
				break;
			case EResourceType::CompositeMaterial:
				PriceData.BasePrice = 100;
				PriceData.MinPrice = 40;
				PriceData.MaxPrice = 300;
				break;
			default:
				PriceData.BasePrice = 10;
				PriceData.MinPrice = 5;
				PriceData.MaxPrice = 50;
				break;
		}

		PriceData.CurrentBuyPrice = PriceData.BasePrice;
		PriceData.CurrentSellPrice = FMath::RoundToInt(PriceData.BasePrice * 0.8f);
		PriceData.Volatility = EMarketVolatility::Moderate;
	}
}

// ============================================================================
// SUPPLY MANAGEMENT
// ============================================================================

int32 UMarketDataComponent::GetCurrentSupply(EResourceType Resource) const
{
	const FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	return SD ? SD->CurrentSupply : 0;
}

int32 UMarketDataComponent::GetMaxSupply(EResourceType Resource) const
{
	const FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	return SD ? SD->MaxSupply : 0;
}

float UMarketDataComponent::GetSupplyPercent(EResourceType Resource) const
{
	const FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	if (SD && SD->MaxSupply > 0)
	{
		return static_cast<float>(SD->CurrentSupply) / static_cast<float>(SD->MaxSupply);
	}
	return 0.0f;
}

void UMarketDataComponent::AddSupply(EResourceType Resource, int32 Amount)
{
	FResourceSupplyDemand& SD = GetOrCreateSupplyDemand(Resource);
	SD.CurrentSupply = FMath::Min(SD.CurrentSupply + Amount, SD.MaxSupply);
	SD.RecalculateMetrics();
	
	OnSupplyDemandChanged.Broadcast(Resource, SD);
}

bool UMarketDataComponent::RemoveSupply(EResourceType Resource, int32 Amount)
{
	FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	if (SD && SD->CurrentSupply >= Amount)
	{
		SD->CurrentSupply -= Amount;
		SD->RecalculateMetrics();
		OnSupplyDemandChanged.Broadcast(Resource, *SD);
		return true;
	}
	return false;
}

void UMarketDataComponent::SetSupplyRate(EResourceType Resource, float Rate)
{
	FResourceSupplyDemand& SD = GetOrCreateSupplyDemand(Resource);
	SD.SupplyRate = FMath::Max(0.0f, Rate);
}

void UMarketDataComponent::SetSupplyModifier(EResourceType Resource, float Modifier)
{
	FResourceSupplyDemand& SD = GetOrCreateSupplyDemand(Resource);
	SD.SupplyModifier = FMath::Max(0.0f, Modifier);
	SD.RecalculateMetrics();
}

// ============================================================================
// DEMAND MANAGEMENT
// ============================================================================

int32 UMarketDataComponent::GetBaseDemand(EResourceType Resource) const
{
	const FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	return SD ? SD->BaseDemand : 0;
}

float UMarketDataComponent::GetDemandRate(EResourceType Resource) const
{
	const FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	return SD ? SD->DemandRate : 0.0f;
}

float UMarketDataComponent::GetDemandElasticity(EResourceType Resource) const
{
	const FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	return SD ? SD->DemandElasticity : 1.0f;
}

void UMarketDataComponent::SetDemandRate(EResourceType Resource, float Rate)
{
	FResourceSupplyDemand& SD = GetOrCreateSupplyDemand(Resource);
	SD.DemandRate = FMath::Max(0.0f, Rate);
	SD.RecalculateMetrics();
}

void UMarketDataComponent::SetDemandModifier(EResourceType Resource, float Modifier)
{
	FResourceSupplyDemand& SD = GetOrCreateSupplyDemand(Resource);
	SD.DemandModifier = FMath::Max(0.0f, Modifier);
	SD.RecalculateMetrics();
}

void UMarketDataComponent::RegisterPlayerDemand(EResourceType Resource, int32 Quantity)
{
	FResourceSupplyDemand& SD = GetOrCreateSupplyDemand(Resource);
	// Player activity increases demand rate temporarily
	SD.DemandRate += static_cast<float>(Quantity) * 0.1f;
	SD.RecalculateMetrics();
}

// ============================================================================
// SUPPLY/DEMAND ANALYSIS
// ============================================================================

float UMarketDataComponent::GetSupplyDemandRatio(EResourceType Resource) const
{
	const FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	return SD ? SD->SupplyDemandRatio : 1.0f;
}

float UMarketDataComponent::GetScarcityIndex(EResourceType Resource) const
{
	const FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	return SD ? SD->ScarcityIndex : 0.0f;
}

FResourceSupplyDemand UMarketDataComponent::GetSupplyDemandData(EResourceType Resource) const
{
	const FResourceSupplyDemand* SD = SupplyDemandMap.Find(Resource);
	return SD ? *SD : FResourceSupplyDemand();
}

TMap<EResourceType, FResourceSupplyDemand> UMarketDataComponent::GetAllSupplyDemandData() const
{
	return SupplyDemandMap;
}

bool UMarketDataComponent::IsResourceScarce(EResourceType Resource, float ScarcityThreshold) const
{
	return GetScarcityIndex(Resource) >= ScarcityThreshold;
}

bool UMarketDataComponent::IsResourceAbundant(EResourceType Resource, float AbundanceThreshold) const
{
	return GetScarcityIndex(Resource) <= AbundanceThreshold;
}

TArray<EResourceType> UMarketDataComponent::GetResourcesByScarcity(bool bMostScarceFirst) const
{
	TArray<TPair<EResourceType, float>> ResourceScarcity;
	
	for (const auto& Pair : SupplyDemandMap)
	{
		ResourceScarcity.Add(TPair<EResourceType, float>(Pair.Key, Pair.Value.ScarcityIndex));
	}

	ResourceScarcity.Sort([bMostScarceFirst](const TPair<EResourceType, float>& A, const TPair<EResourceType, float>& B) {
		return bMostScarceFirst ? A.Value > B.Value : A.Value < B.Value;
	});

	TArray<EResourceType> Result;
	for (const auto& Pair : ResourceScarcity)
	{
		Result.Add(Pair.Key);
	}
	
	return Result;
}

// ============================================================================
// PRICE HISTORY
// ============================================================================

void UMarketDataComponent::RecordPricePoint(EResourceType Resource, int32 Price, int32 Volume)
{
	FDynamicMarketPrice& PriceData = GetOrCreatePriceData(Resource);
	
	float SDRatio = GetSupplyDemandRatio(Resource);
	PriceData.AddHistoryEntry(Price, Volume, SDRatio);
	
	// Update trend based on new history
	PriceData.CurrentTrend = CalculateTrend(PriceData.PriceHistory);
}

TArray<FPriceHistoryEntry> UMarketDataComponent::GetPriceHistory(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = PriceDataMap.Find(Resource);
	return PriceData ? PriceData->PriceHistory : TArray<FPriceHistoryEntry>();
}

float UMarketDataComponent::GetAveragePrice(EResourceType Resource, int32 NumEntries) const
{
	const FDynamicMarketPrice* PriceData = PriceDataMap.Find(Resource);
	return PriceData ? PriceData->CalculateAveragePrice(NumEntries) : 0.0f;
}

EMarketTrend UMarketDataComponent::GetPriceTrend(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = PriceDataMap.Find(Resource);
	return PriceData ? PriceData->CurrentTrend : EMarketTrend::Neutral;
}

float UMarketDataComponent::GetPriceVolatility(EResourceType Resource) const
{
	const FDynamicMarketPrice* PriceData = PriceDataMap.Find(Resource);
	if (PriceData)
	{
		return CalculateVolatilityFromHistory(PriceData->PriceHistory);
	}
	return 0.0f;
}

int32 UMarketDataComponent::GetHighestRecentPrice(EResourceType Resource, int32 NumEntries) const
{
	const FDynamicMarketPrice* PriceData = PriceDataMap.Find(Resource);
	if (!PriceData || PriceData->PriceHistory.Num() == 0)
	{
		return 0;
	}

	int32 Count = FMath::Min(NumEntries, PriceData->PriceHistory.Num());
	int32 HighestPrice = 0;
	
	for (int32 i = PriceData->PriceHistory.Num() - Count; i < PriceData->PriceHistory.Num(); ++i)
	{
		HighestPrice = FMath::Max(HighestPrice, PriceData->PriceHistory[i].Price);
	}
	
	return HighestPrice;
}

int32 UMarketDataComponent::GetLowestRecentPrice(EResourceType Resource, int32 NumEntries) const
{
	const FDynamicMarketPrice* PriceData = PriceDataMap.Find(Resource);
	if (!PriceData || PriceData->PriceHistory.Num() == 0)
	{
		return 0;
	}

	int32 Count = FMath::Min(NumEntries, PriceData->PriceHistory.Num());
	int32 LowestPrice = INT32_MAX;
	
	for (int32 i = PriceData->PriceHistory.Num() - Count; i < PriceData->PriceHistory.Num(); ++i)
	{
		LowestPrice = FMath::Min(LowestPrice, PriceData->PriceHistory[i].Price);
	}
	
	return LowestPrice == INT32_MAX ? 0 : LowestPrice;
}

// ============================================================================
// MARKET INFO
// ============================================================================

bool UMarketDataComponent::IsSpecializedIn(EResourceType Resource) const
{
	return SpecializedResources.Contains(Resource);
}

// ============================================================================
// SIMULATION
// ============================================================================

void UMarketDataComponent::SimulateSupplyDemand(float DeltaGameHours)
{
	for (auto& Pair : SupplyDemandMap)
	{
		FResourceSupplyDemand& SD = Pair.Value;

		// Production: Add supply based on rate and modifier
		float ProductionAmount = SD.SupplyRate * SD.SupplyModifier * DeltaGameHours;
		SD.CurrentSupply = FMath::Min(SD.CurrentSupply + FMath::RoundToInt(ProductionAmount), SD.MaxSupply);

		// Consumption: Remove supply based on demand
		float ConsumptionAmount = SD.DemandRate * SD.DemandModifier * DeltaGameHours;
		SD.CurrentSupply = FMath::Max(0, SD.CurrentSupply - FMath::RoundToInt(ConsumptionAmount));

		// Natural demand decay (demand slowly returns to base)
		float DemandDecay = 0.01f * DeltaGameHours;
		SD.DemandRate = FMath::Lerp(SD.DemandRate, static_cast<float>(SD.BaseDemand), DemandDecay);

		// Recalculate metrics
		SD.RecalculateMetrics();
	}

	UpdateCachedMarketData();
}

void UMarketDataComponent::RecalculateAllMetrics()
{
	for (auto& Pair : SupplyDemandMap)
	{
		Pair.Value.RecalculateMetrics();
	}
	UpdateCachedMarketData();
}

void UMarketDataComponent::ResetToDefaults()
{
	SupplyDemandMap.Empty();
	PriceDataMap.Empty();
	InitializeDefaultSupplyDemand();
	UpdateCachedMarketData();
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

void UMarketDataComponent::UpdateCachedMarketData()
{
	CachedMarketData.MarketId = MarketId;
	CachedMarketData.DisplayName = DisplayName;
	CachedMarketData.LocationType = LocationType;
	CachedMarketData.SupplyDemandData = SupplyDemandMap;
	CachedMarketData.ResourcePrices = PriceDataMap;
	CachedMarketData.SpecializedResources = SpecializedResources;
	CachedMarketData.SpecializationBonus = SpecializationBonus;
	CachedMarketData.LastUpdateTime = FPlatformTime::Seconds();
}

FResourceSupplyDemand& UMarketDataComponent::GetOrCreateSupplyDemand(EResourceType Resource)
{
	if (!SupplyDemandMap.Contains(Resource))
	{
		FResourceSupplyDemand NewSD;
		NewSD.ResourceType = Resource;
		SupplyDemandMap.Add(Resource, NewSD);
	}
	return SupplyDemandMap[Resource];
}

FDynamicMarketPrice& UMarketDataComponent::GetOrCreatePriceData(EResourceType Resource)
{
	if (!PriceDataMap.Contains(Resource))
	{
		FDynamicMarketPrice NewPrice;
		NewPrice.ResourceType = Resource;
		NewPrice.MaxHistoryEntries = MaxPriceHistoryEntries;
		PriceDataMap.Add(Resource, NewPrice);
	}
	return PriceDataMap[Resource];
}

EMarketTrend UMarketDataComponent::CalculateTrend(const TArray<FPriceHistoryEntry>& History) const
{
	if (History.Num() < 3)
	{
		return EMarketTrend::Neutral;
	}

	// Calculate trend using simple moving average comparison
	int32 RecentCount = FMath::Min(5, History.Num());
	int32 OlderCount = FMath::Min(10, History.Num());
	
	float RecentAvg = 0.0f;
	float OlderAvg = 0.0f;
	
	for (int32 i = History.Num() - RecentCount; i < History.Num(); ++i)
	{
		RecentAvg += History[i].Price;
	}
	RecentAvg /= RecentCount;
	
	for (int32 i = FMath::Max(0, History.Num() - OlderCount); i < History.Num() - RecentCount; ++i)
	{
		OlderAvg += History[i].Price;
	}
	if (History.Num() > RecentCount)
	{
		OlderAvg /= FMath::Min(OlderCount - RecentCount, History.Num() - RecentCount);
	}
	else
	{
		return EMarketTrend::Neutral;
	}

	float ChangePercent = OlderAvg > 0 ? (RecentAvg - OlderAvg) / OlderAvg : 0.0f;

	if (ChangePercent > 0.15f) return EMarketTrend::StrongBull;
	if (ChangePercent > 0.05f) return EMarketTrend::Bull;
	if (ChangePercent < -0.15f) return EMarketTrend::StrongBear;
	if (ChangePercent < -0.05f) return EMarketTrend::Bear;
	
	return EMarketTrend::Neutral;
}

float UMarketDataComponent::CalculateVolatilityFromHistory(const TArray<FPriceHistoryEntry>& History) const
{
	if (History.Num() < 2)
	{
		return 0.0f;
	}

	// Calculate standard deviation of price changes
	TArray<float> PriceChanges;
	for (int32 i = 1; i < History.Num(); ++i)
	{
		if (History[i - 1].Price > 0)
		{
			float Change = static_cast<float>(History[i].Price - History[i - 1].Price) / History[i - 1].Price;
			PriceChanges.Add(FMath::Abs(Change));
		}
	}

	if (PriceChanges.Num() == 0)
	{
		return 0.0f;
	}

	float Mean = 0.0f;
	for (float Change : PriceChanges)
	{
		Mean += Change;
	}
	Mean /= PriceChanges.Num();

	float Variance = 0.0f;
	for (float Change : PriceChanges)
	{
		Variance += FMath::Square(Change - Mean);
	}
	Variance /= PriceChanges.Num();

	return FMath::Sqrt(Variance);
}
