// EconomyRippleEffect.cpp
// Implementation of economic ripple propagation system

#include "EconomyRippleEffect.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.h"
#include "Engine/World.h"

UEconomyRippleEffect::UEconomyRippleEffect()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Ripple propagation happens at a moderate rate to spread effects gradually
	PrimaryComponentTick.TickInterval = 0.5f;

	MarketDataMap = nullptr;
	PriceSystemMap = nullptr;
	TradeRouteAnalyzer = nullptr;
	NextRippleId = 1;
}

void UEconomyRippleEffect::BeginPlay()
{
	Super::BeginPlay();
}

void UEconomyRippleEffect::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PropagateRipples(DeltaTime);
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void UEconomyRippleEffect::InitializeRippleSystem(const FEconomyConfiguration& InConfig)
{
	EconomyConfig = InConfig;
}

void UEconomyRippleEffect::SetMarketReferences(
	TMap<FName, UMarketDataComponent*>* InMarketDataMap,
	TMap<FName, UPriceFluctuationSystem*>* InPriceSystemMap,
	UTradeRouteAnalyzer* InTradeRouteAnalyzer)
{
	MarketDataMap = InMarketDataMap;
	PriceSystemMap = InPriceSystemMap;
	TradeRouteAnalyzer = InTradeRouteAnalyzer;
}

// ============================================================================
// RIPPLE CREATION HELPERS
// ============================================================================

FEconomicRipple UEconomyRippleEffect::CreateBaseRipple(
	ERippleType Type,
	const FMarketId& Origin,
	const TArray<EResourceType>& Resources,
	float Magnitude,
	int32 SourceEventId)
{
	FEconomicRipple Ripple;
	Ripple.RippleId = NextRippleId++;
	Ripple.RippleType = Type;
	Ripple.OriginMarket = Origin;
	Ripple.AffectedResources = Resources;
	Ripple.BaseMagnitude = FMath::Clamp(FMath::Abs(Magnitude), 0.0f, 2.0f);
	// Preserve sign for supply/demand direction via a convention:
	// Positive magnitude = increase (supply discovery, demand surge)
	// We store the absolute value in BaseMagnitude and use the sign of the original
	// to determine the direction when applying effects.
	if (Magnitude < 0.0f)
	{
		Ripple.BaseMagnitude = -Ripple.BaseMagnitude;
	}
	Ripple.DampeningFactor = EconomyConfig.RippleDefaultDampening;
	Ripple.MaxDepth = EconomyConfig.RippleMaxPropagationDepth;
	Ripple.PropagationSpeed = 2.0f; // 2 hops per second
	Ripple.CreationTime = FPlatformTime::Seconds();
	Ripple.SourceEventId = SourceEventId;
	Ripple.bIsActive = true;
	Ripple.CurrentDepth = 0;
	Ripple.AccumulatedTime = 0.0f;

	// The origin market is the first visited
	Ripple.VisitedMarkets.Add(Origin);

	// Seed the next wave with markets connected to the origin
	Ripple.NextWaveMarkets = GetConnectedMarkets(Origin, Ripple.VisitedMarkets);

	return Ripple;
}

// ============================================================================
// RIPPLE CREATION (PUBLIC API)
// ============================================================================

int32 UEconomyRippleEffect::CreateSupplyShockRipple(
	const FMarketId& OriginMarket,
	const TArray<EResourceType>& Resources,
	float Magnitude,
	int32 SourceEventId)
{
	if (ActiveRipples.Num() >= EconomyConfig.MaxActiveRipples)
	{
		return -1;
	}

	FEconomicRipple Ripple = CreateBaseRipple(ERippleType::SupplyShock, OriginMarket, Resources, Magnitude, SourceEventId);
	ActiveRipples.Add(Ripple);

	if (EconomyConfig.bEnableDetailedLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("EconomyRipple: Created SupplyShock ripple %d at %s (magnitude %.2f)"),
			Ripple.RippleId, *OriginMarket.ToString(), Magnitude);
	}

	return Ripple.RippleId;
}

int32 UEconomyRippleEffect::CreateDemandShockRipple(
	const FMarketId& OriginMarket,
	const TArray<EResourceType>& Resources,
	float Magnitude,
	int32 SourceEventId)
{
	if (ActiveRipples.Num() >= EconomyConfig.MaxActiveRipples)
	{
		return -1;
	}

	FEconomicRipple Ripple = CreateBaseRipple(ERippleType::DemandShock, OriginMarket, Resources, Magnitude, SourceEventId);
	ActiveRipples.Add(Ripple);
	return Ripple.RippleId;
}

int32 UEconomyRippleEffect::CreatePriceShockRipple(
	const FMarketId& OriginMarket,
	const TArray<EResourceType>& Resources,
	float Magnitude,
	int32 SourceEventId)
{
	if (ActiveRipples.Num() >= EconomyConfig.MaxActiveRipples)
	{
		return -1;
	}

	FEconomicRipple Ripple = CreateBaseRipple(ERippleType::PriceShock, OriginMarket, Resources, Magnitude, SourceEventId);
	ActiveRipples.Add(Ripple);
	return Ripple.RippleId;
}

int32 UEconomyRippleEffect::CreateTradeDisruptionRipple(
	const FMarketId& OriginMarket,
	float Magnitude,
	int32 SourceEventId)
{
	if (ActiveRipples.Num() >= EconomyConfig.MaxActiveRipples)
	{
		return -1;
	}

	// Trade disruptions affect all common resources
	TArray<EResourceType> AllResources = {
		EResourceType::Silicate,
		EResourceType::Carbon,
		EResourceType::RefinedSilicate,
		EResourceType::RefinedCarbon,
		EResourceType::CompositeMaterial
	};

	FEconomicRipple Ripple = CreateBaseRipple(ERippleType::TradeDisruption, OriginMarket, AllResources, Magnitude, SourceEventId);
	// Trade disruptions spread more slowly but deeper
	Ripple.PropagationSpeed = 1.0f;
	Ripple.MaxDepth = FMath::Min(EconomyConfig.RippleMaxPropagationDepth + 1, 6);
	ActiveRipples.Add(Ripple);
	return Ripple.RippleId;
}

int32 UEconomyRippleEffect::CreateCombatZoneRipple(
	const FMarketId& NearestMarket,
	float CombatIntensity,
	int32 SourceEventId)
{
	if (ActiveRipples.Num() >= EconomyConfig.MaxActiveRipples)
	{
		return -1;
	}

	TArray<EResourceType> AllResources = {
		EResourceType::Silicate,
		EResourceType::Carbon,
		EResourceType::RefinedSilicate,
		EResourceType::RefinedCarbon,
		EResourceType::CompositeMaterial
	};

	FEconomicRipple Ripple = CreateBaseRipple(ERippleType::CombatZone, NearestMarket, AllResources, CombatIntensity * 0.5f, SourceEventId);
	// Combat ripples are short range but intense
	Ripple.MaxDepth = FMath::Min(EconomyConfig.RippleMaxPropagationDepth, 3);
	Ripple.DampeningFactor = 0.5f; // Heavy dampening
	ActiveRipples.Add(Ripple);
	return Ripple.RippleId;
}

int32 UEconomyRippleEffect::CreateCraftingDemandRipple(
	const FMarketId& CraftingMarket,
	const TArray<EResourceType>& IngredientResources,
	float DemandIntensity,
	int32 SourceEventId)
{
	if (ActiveRipples.Num() >= EconomyConfig.MaxActiveRipples)
	{
		return -1;
	}

	FEconomicRipple Ripple = CreateBaseRipple(ERippleType::CraftingDemand, CraftingMarket, IngredientResources, DemandIntensity, SourceEventId);
	Ripple.MaxDepth = FMath::Min(EconomyConfig.RippleMaxPropagationDepth, 3);
	ActiveRipples.Add(Ripple);
	return Ripple.RippleId;
}

int32 UEconomyRippleEffect::CreateRipple(const FEconomicRipple& RippleTemplate)
{
	if (ActiveRipples.Num() >= EconomyConfig.MaxActiveRipples)
	{
		return -1;
	}

	FEconomicRipple Ripple = RippleTemplate;
	Ripple.RippleId = NextRippleId++;
	Ripple.CreationTime = FPlatformTime::Seconds();
	Ripple.bIsActive = true;

	if (Ripple.VisitedMarkets.Num() == 0)
	{
		Ripple.VisitedMarkets.Add(Ripple.OriginMarket);
	}
	if (Ripple.NextWaveMarkets.Num() == 0)
	{
		Ripple.NextWaveMarkets = GetConnectedMarkets(Ripple.OriginMarket, Ripple.VisitedMarkets);
	}

	ActiveRipples.Add(Ripple);
	return Ripple.RippleId;
}

// ============================================================================
// RIPPLE QUERIES
// ============================================================================

TArray<FEconomicRipple> UEconomyRippleEffect::GetActiveRipples() const
{
	return ActiveRipples;
}

FEconomicRipple UEconomyRippleEffect::GetRipple(int32 RippleId) const
{
	for (const FEconomicRipple& Ripple : ActiveRipples)
	{
		if (Ripple.RippleId == RippleId)
		{
			return Ripple;
		}
	}
	return FEconomicRipple();
}

bool UEconomyRippleEffect::CancelRipple(int32 RippleId)
{
	for (int32 i = 0; i < ActiveRipples.Num(); ++i)
	{
		if (ActiveRipples[i].RippleId == RippleId)
		{
			ActiveRipples.RemoveAt(i);
			return true;
		}
	}
	return false;
}

void UEconomyRippleEffect::CancelAllRipples()
{
	ActiveRipples.Empty();
}

void UEconomyRippleEffect::SetConfiguration(const FEconomyConfiguration& InConfig)
{
	EconomyConfig = InConfig;
}

// ============================================================================
// CORE PROPAGATION LOGIC
// ============================================================================

void UEconomyRippleEffect::PropagateRipples(float DeltaTime)
{
	TArray<int32> RipplesToRemove;

	for (int32 i = 0; i < ActiveRipples.Num(); ++i)
	{
		FEconomicRipple& Ripple = ActiveRipples[i];

		if (!Ripple.bIsActive)
		{
			RipplesToRemove.Add(i);
			continue;
		}

		// Check if ripple has dissipated
		if (Ripple.HasDissipated(EconomyConfig.RippleMinMagnitudeThreshold))
		{
			Ripple.bIsActive = false;
			RipplesToRemove.Add(i);
			continue;
		}

		// Check if there are markets left to propagate to
		if (Ripple.NextWaveMarkets.Num() == 0)
		{
			Ripple.bIsActive = false;
			RipplesToRemove.Add(i);
			continue;
		}

		// Accumulate time for wave propagation
		Ripple.AccumulatedTime += DeltaTime;
		float TimePerHop = Ripple.PropagationSpeed > 0.0f ? 1.0f / Ripple.PropagationSpeed : 1.0f;

		if (Ripple.AccumulatedTime < TimePerHop)
		{
			continue; // Not time for next wave yet
		}

		Ripple.AccumulatedTime -= TimePerHop;
		Ripple.CurrentDepth++;

		// Calculate effective magnitude at this depth
		float EffectiveMagnitude = Ripple.GetCurrentMagnitude();

		// Apply effect to all markets in the current wave
		TArray<FMarketId> NewNextWave;
		for (const FMarketId& MarketId : Ripple.NextWaveMarkets)
		{
			ApplyRippleToMarket(Ripple, MarketId, EffectiveMagnitude);
			Ripple.VisitedMarkets.Add(MarketId);

			// Discover next-hop markets from this node
			TArray<FMarketId> Connected = GetConnectedMarkets(MarketId, Ripple.VisitedMarkets);
			for (const FMarketId& NextMarket : Connected)
			{
				if (!NewNextWave.Contains(NextMarket))
				{
					NewNextWave.Add(NextMarket);
				}
			}
		}

		Ripple.NextWaveMarkets = NewNextWave;

		// Broadcast propagation event
		OnRipplePropagated.Broadcast(Ripple);

		if (EconomyConfig.bEnableDetailedLogging)
		{
			UE_LOG(LogTemp, Log, TEXT("EconomyRipple: Ripple %d propagated to depth %d (magnitude %.3f, %d next markets)"),
				Ripple.RippleId, Ripple.CurrentDepth, EffectiveMagnitude, Ripple.NextWaveMarkets.Num());
		}
	}

	// Remove dissipated ripples (reverse order)
	for (int32 i = RipplesToRemove.Num() - 1; i >= 0; --i)
	{
		if (RipplesToRemove[i] < ActiveRipples.Num())
		{
			ActiveRipples.RemoveAt(RipplesToRemove[i]);
		}
	}
}

void UEconomyRippleEffect::ApplyRippleToMarket(const FEconomicRipple& Ripple, const FMarketId& MarketId, float EffectiveMagnitude)
{
	FName Key = GetMarketKey(MarketId);

	UMarketDataComponent* MarketData = nullptr;
	UPriceFluctuationSystem* PriceSystem = nullptr;

	if (MarketDataMap)
	{
		UMarketDataComponent** Found = MarketDataMap->Find(Key);
		if (Found) MarketData = *Found;
	}
	if (PriceSystemMap)
	{
		UPriceFluctuationSystem** Found = PriceSystemMap->Find(Key);
		if (Found) PriceSystem = *Found;
	}

	if (!MarketData && !PriceSystem)
	{
		return; // No components to affect
	}

	// Apply effect based on ripple type
	switch (Ripple.RippleType)
	{
		case ERippleType::SupplyShock:
		{
			if (MarketData)
			{
				for (EResourceType Resource : Ripple.AffectedResources)
				{
					// A supply shock modifies the supply rate. Negative = supply decrease.
					float CurrentModifier = 1.0f; // Could query existing modifier
					float NewModifier = FMath::Clamp(CurrentModifier + EffectiveMagnitude, 0.1f, 3.0f);
					MarketData->SetSupplyModifier(Resource, NewModifier);
				}
			}
			break;
		}

		case ERippleType::DemandShock:
		{
			if (MarketData)
			{
				for (EResourceType Resource : Ripple.AffectedResources)
				{
					float CurrentModifier = 1.0f;
					float NewModifier = FMath::Clamp(CurrentModifier + EffectiveMagnitude, 0.1f, 3.0f);
					MarketData->SetDemandModifier(Resource, NewModifier);
				}
			}
			break;
		}

		case ERippleType::PriceShock:
		{
			if (PriceSystem)
			{
				for (EResourceType Resource : Ripple.AffectedResources)
				{
					// Apply as a temporary price shock that decays over time
					float ShockMultiplier = 1.0f + EffectiveMagnitude;
					ShockMultiplier = FMath::Clamp(ShockMultiplier, 0.25f, 4.0f);
					PriceSystem->ApplyPriceShock(Resource, ShockMultiplier, 0.1f);
				}
			}
			break;
		}

		case ERippleType::TradeDisruption:
		{
			// Trade disruption reduces supply and increases volatility
			if (MarketData)
			{
				float AbsMag = FMath::Abs(EffectiveMagnitude);
				for (EResourceType Resource : Ripple.AffectedResources)
				{
					float SupplyReduction = FMath::Clamp(1.0f - AbsMag * 0.5f, 0.3f, 1.0f);
					MarketData->SetSupplyModifier(Resource, SupplyReduction);
				}
			}
			if (PriceSystem)
			{
				for (EResourceType Resource : Ripple.AffectedResources)
				{
					PriceSystem->SetResourceVolatility(Resource, EMarketVolatility::High);
				}
			}
			break;
		}

		case ERippleType::CombatZone:
		{
			// Combat zones reduce supply and increase demand (for repair materials)
			float AbsMag = FMath::Abs(EffectiveMagnitude);
			if (MarketData)
			{
				for (EResourceType Resource : Ripple.AffectedResources)
				{
					MarketData->SetSupplyModifier(Resource, FMath::Clamp(1.0f - AbsMag * 0.3f, 0.4f, 1.0f));
					MarketData->SetDemandModifier(Resource, FMath::Clamp(1.0f + AbsMag * 0.4f, 1.0f, 2.0f));
				}
			}
			if (PriceSystem)
			{
				for (EResourceType Resource : Ripple.AffectedResources)
				{
					PriceSystem->SetResourceVolatility(Resource, EMarketVolatility::High);
				}
			}
			break;
		}

		case ERippleType::CraftingDemand:
		{
			// Crafting demand ripple increases demand for ingredients
			if (MarketData)
			{
				for (EResourceType Resource : Ripple.AffectedResources)
				{
					float DemandIncrease = FMath::Clamp(1.0f + FMath::Abs(EffectiveMagnitude) * 0.5f, 1.0f, 2.0f);
					MarketData->SetDemandModifier(Resource, DemandIncrease);
				}
			}
			break;
		}
	}
}

TArray<FMarketId> UEconomyRippleEffect::GetConnectedMarkets(const FMarketId& MarketId, const TArray<FMarketId>& ExcludeList) const
{
	TArray<FMarketId> Connected;

	if (!TradeRouteAnalyzer)
	{
		return Connected;
	}

	TArray<FTradeRoute> Routes = TradeRouteAnalyzer->GetRoutesFrom(MarketId);
	for (const FTradeRoute& Route : Routes)
	{
		if (Route.bIsActive && !ExcludeList.Contains(Route.DestinationMarket))
		{
			Connected.AddUnique(Route.DestinationMarket);
		}
	}

	return Connected;
}

FName UEconomyRippleEffect::GetMarketKey(const FMarketId& MarketId) const
{
	return FName(*MarketId.ToString());
}
