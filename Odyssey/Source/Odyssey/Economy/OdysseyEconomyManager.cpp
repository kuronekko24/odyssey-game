// OdysseyEconomyManager.cpp
// Master controller implementation for the Dynamic Economy Simulation System (Task #5)
//
// This is the central orchestrator that:
//   - Creates and manages all economy subsystems
//   - Coordinates market registration and routing
//   - Processes trade transactions with proper supply/demand/price effects
//   - Integrates combat and crafting events into economic simulation
//   - Provides unified API for all economy queries
//   - Drives the ripple effect and save systems
//
// Mobile optimization strategy:
//   - Staggered updates: only N markets updated per tick
//   - Lazy analysis: trade routes re-analyzed on a configurable interval
//   - Event-driven: heavy computation triggered by events, not polling

#include "UOdysseyEconomyManager.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.h"
#include "UEconomicEventSystem.h"
#include "OdysseyCharacter.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyEventBus.h"
#include "Economy/EconomyRippleEffect.h"
#include "Economy/EconomySaveSystem.h"
#include "Engine/World.h"

// Singleton
UOdysseyEconomyManager* UOdysseyEconomyManager::GlobalInstance = nullptr;

UOdysseyEconomyManager::UOdysseyEconomyManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f;

	TradeRouteAnalyzer = nullptr;
	EconomicEventSystem = nullptr;
	EventBus = nullptr;
	bIsInitialized = false;
	bSimulationPaused = false;
	TimeScale = 1.0f;

	// Set singleton
	if (!GlobalInstance)
	{
		GlobalInstance = this;
	}
}

UOdysseyEconomyManager* UOdysseyEconomyManager::Get()
{
	return GlobalInstance;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void UOdysseyEconomyManager::BeginPlay()
{
	Super::BeginPlay();

	// Create subsystems with default config if not already initialized
	if (!bIsInitialized)
	{
		InitializeEconomy(EconomyConfig);
	}
}

void UOdysseyEconomyManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unsubscribe from event bus
	if (EventBus)
	{
		for (FOdysseyEventHandle& Handle : EventSubscriptions)
		{
			EventBus->Unsubscribe(Handle);
		}
		EventSubscriptions.Empty();
	}

	if (GlobalInstance == this)
	{
		GlobalInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UOdysseyEconomyManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsInitialized || bSimulationPaused)
	{
		return;
	}

	// The subsystem components tick themselves. The manager's tick is for:
	// 1. Updating global statistics
	// 2. Decaying crafting demand multipliers
	// 3. Coordinating cross-system updates

	float ScaledDelta = DeltaTime * TimeScale;

	// Decay crafting demand multipliers toward 1.0
	for (auto& Pair : CraftingDemandMultipliers)
	{
		Pair.Value = FMath::Lerp(Pair.Value, 1.0f, 0.01f * ScaledDelta);
		if (FMath::IsNearlyEqual(Pair.Value, 1.0f, 0.01f))
		{
			Pair.Value = 1.0f;
		}
	}

	// Periodic statistics update
	UpdateStatistics();
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void UOdysseyEconomyManager::InitializeEconomy(const FEconomyConfiguration& Config)
{
	EconomyConfig = Config;
	CreateSubsystems();
	bIsInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("OdysseyEconomyManager: Economy system initialized"));
}

void UOdysseyEconomyManager::CreateSubsystems()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("OdysseyEconomyManager: No owning actor for subsystem creation"));
		return;
	}

	// Create Trade Route Analyzer
	if (!TradeRouteAnalyzer)
	{
		TradeRouteAnalyzer = NewObject<UTradeRouteAnalyzer>(Owner);
		TradeRouteAnalyzer->RegisterComponent();
		TradeRouteAnalyzer->SetAnalysisInterval(EconomyConfig.TradeRouteAnalysisIntervalSeconds);
		TradeRouteAnalyzer->SetMaxOpportunities(EconomyConfig.MaxTradeOpportunities);
		TradeRouteAnalyzer->SetMinProfitMargin(EconomyConfig.MinProfitMarginForOpportunity);

		// Forward opportunity events
		TradeRouteAnalyzer->OnOpportunityFound.AddDynamic(this, &UOdysseyEconomyManager::HandleOpportunityFound);
	}

	// Create Economic Event System
	if (!EconomicEventSystem)
	{
		EconomicEventSystem = NewObject<UEconomicEventSystem>(Owner);
		EconomicEventSystem->RegisterComponent();

		FEventGenerationParams EventParams;
		EventParams.MaxActiveEvents = EconomyConfig.MaxActiveEvents;
		EventParams.bAllowCatastrophicEvents = EconomyConfig.bAllowCatastrophicEvents;
		EventParams.BaseEventChancePerHour = 0.2f * EconomyConfig.EventSpawnRateMultiplier;
		EconomicEventSystem->Initialize(EventParams);

		// Forward event notifications
		EconomicEventSystem->OnEventStarted.AddDynamic(this, &UOdysseyEconomyManager::HandleEconomicEventStarted);
		EconomicEventSystem->OnEventEnded.AddDynamic(this, &UOdysseyEconomyManager::HandleEconomicEventEnded);
	}

	// Create Ripple Effect System
	RippleEffectSystem = NewObject<UEconomyRippleEffect>(Owner);
	RippleEffectSystem->RegisterComponent();
	RippleEffectSystem->InitializeRippleSystem(EconomyConfig);
	RippleEffectSystem->SetMarketReferences(&MarketDataComponents, &PriceSystems, TradeRouteAnalyzer);

	// Create Save System
	SaveSystem = NewObject<UEconomySaveSystem>(Owner);
	SaveSystem->RegisterComponent();
	SaveSystem->SetEconomyReferences(
		&MarketDataComponents, &PriceSystems,
		TradeRouteAnalyzer, EconomicEventSystem, &RegisteredMarkets);
}

void UOdysseyEconomyManager::LoadEconomyData(
	UDataTable* MarketDataTable,
	UDataTable* ResourceDataTable,
	UDataTable* EventTemplateTable)
{
	if (EventTemplateTable && EconomicEventSystem)
	{
		EconomicEventSystem->LoadEventTemplates(EventTemplateTable);
	}

	UE_LOG(LogTemp, Log, TEXT("OdysseyEconomyManager: Economy data loaded"));
}

void UOdysseyEconomyManager::ConnectToEventBus(UOdysseyEventBus* InEventBus)
{
	EventBus = InEventBus;
	if (EventBus)
	{
		SetupEventListeners();
	}
}

void UOdysseyEconomyManager::SetupEventListeners()
{
	if (!EventBus) return;

	// Subscribe to combat events
	FOdysseyEventHandle CombatHandle = EventBus->Subscribe(
		EOdysseyEventType::DamageDealt,
		[this](const FOdysseyEventPayload& Payload)
		{
			if (const FCombatEventPayload* CombatPayload = static_cast<const FCombatEventPayload*>(&Payload))
			{
				HandleCombatEvent(*CombatPayload);
			}
		});
	EventSubscriptions.Add(CombatHandle);

	// Subscribe to interaction events (mining, crafting, trading)
	FOdysseyEventHandle InteractHandle = EventBus->Subscribe(
		EOdysseyEventType::InteractionCompleted,
		[this](const FOdysseyEventPayload& Payload)
		{
			if (const FInteractionEventPayload* InteractPayload = static_cast<const FInteractionEventPayload*>(&Payload))
			{
				HandleInteractionEvent(*InteractPayload);
			}
		});
	EventSubscriptions.Add(InteractHandle);

	UE_LOG(LogTemp, Log, TEXT("OdysseyEconomyManager: Connected to event bus"));
}

// ============================================================================
// MARKET MANAGEMENT
// ============================================================================

bool UOdysseyEconomyManager::CreateMarket(const FMarketId& MarketId, const FString& DisplayName, FVector WorldLocation, EMarketLocationType Type)
{
	FName Key = GetMarketKey(MarketId);

	if (MarketDataComponents.Contains(Key))
	{
		UE_LOG(LogTemp, Warning, TEXT("OdysseyEconomyManager: Market '%s' already exists"), *MarketId.ToString());
		return false;
	}

	AActor* Owner = GetOwner();
	if (!Owner) return false;

	// Create Market Data Component
	UMarketDataComponent* MarketData = NewObject<UMarketDataComponent>(Owner);
	MarketData->RegisterComponent();
	MarketData->InitializeMarketData(MarketId, DisplayName);
	MarketDataComponents.Add(Key, MarketData);

	// Create Price Fluctuation System for this market
	UPriceFluctuationSystem* PriceSystem = NewObject<UPriceFluctuationSystem>(Owner);
	PriceSystem->RegisterComponent();
	PriceSystem->SetConfiguration(EconomyConfig);
	PriceSystem->Initialize(MarketData);
	PriceSystems.Add(Key, PriceSystem);

	// Forward price change events
	PriceSystem->OnPriceChanged.AddDynamic(this, &UOdysseyEconomyManager::HandlePriceChanged);

	// Store location
	MarketLocations.Add(Key, WorldLocation);
	RegisteredMarkets.AddUnique(MarketId);

	// Register with subsystems
	if (TradeRouteAnalyzer)
	{
		TradeRouteAnalyzer->RegisterMarket(MarketId, MarketData, PriceSystem);
	}
	if (EconomicEventSystem)
	{
		EconomicEventSystem->RegisterMarket(MarketId, MarketData, PriceSystem);
	}

	// Update ripple system references
	if (RippleEffectSystem)
	{
		RippleEffectSystem->SetMarketReferences(&MarketDataComponents, &PriceSystems, TradeRouteAnalyzer);
	}

	UE_LOG(LogTemp, Log, TEXT("OdysseyEconomyManager: Created market '%s' at (%.0f, %.0f, %.0f)"),
		*DisplayName, WorldLocation.X, WorldLocation.Y, WorldLocation.Z);

	return true;
}

void UOdysseyEconomyManager::RemoveMarket(const FMarketId& MarketId)
{
	FName Key = GetMarketKey(MarketId);

	MarketDataComponents.Remove(Key);
	PriceSystems.Remove(Key);
	MarketLocations.Remove(Key);
	RegisteredMarkets.Remove(MarketId);

	if (TradeRouteAnalyzer)
	{
		TradeRouteAnalyzer->UnregisterMarket(MarketId);
	}
	if (EconomicEventSystem)
	{
		EconomicEventSystem->UnregisterMarket(MarketId);
	}
}

UMarketDataComponent* UOdysseyEconomyManager::GetMarketData(const FMarketId& MarketId) const
{
	FName Key = GetMarketKey(MarketId);
	UMarketDataComponent* const* Found = MarketDataComponents.Find(Key);
	return Found ? *Found : nullptr;
}

UPriceFluctuationSystem* UOdysseyEconomyManager::GetPriceSystem(const FMarketId& MarketId) const
{
	FName Key = GetMarketKey(MarketId);
	UPriceFluctuationSystem* const* Found = PriceSystems.Find(Key);
	return Found ? *Found : nullptr;
}

TArray<FMarketId> UOdysseyEconomyManager::GetAllMarkets() const
{
	return RegisteredMarkets;
}

FMarketId UOdysseyEconomyManager::GetNearestMarket(FVector Location) const
{
	FMarketId Nearest;
	float MinDist = FLT_MAX;

	for (const auto& Pair : MarketLocations)
	{
		float Dist = FVector::Distance(Location, Pair.Value);
		if (Dist < MinDist)
		{
			MinDist = Dist;
			// Reconstruct MarketId from key
			for (const FMarketId& Id : RegisteredMarkets)
			{
				if (GetMarketKey(Id) == Pair.Key)
				{
					Nearest = Id;
					break;
				}
			}
		}
	}

	return Nearest;
}

// ============================================================================
// TRADING API
// ============================================================================

bool UOdysseyEconomyManager::ExecuteBuy(
	const FMarketId& MarketId,
	EResourceType Resource,
	int32 Quantity,
	AOdysseyCharacter* Buyer)
{
	if (!CanBuy(MarketId, Resource, Quantity) || !Buyer)
	{
		return false;
	}

	UPriceFluctuationSystem* PriceSystem = GetPriceSystem(MarketId);
	UMarketDataComponent* MarketData = GetMarketData(MarketId);
	if (!PriceSystem || !MarketData) return false;

	int32 TotalCost = PriceSystem->CalculateBuyPriceForQuantity(Resource, Quantity);

	// Check buyer can afford
	UOdysseyInventoryComponent* Inventory = Buyer->GetInventoryComponent();
	if (!Inventory) return false;

	int32 PlayerOMEN = Inventory->GetResourceAmount(EResourceType::OMEN);
	if (PlayerOMEN < TotalCost) return false;

	// Execute transaction
	Inventory->RemoveResource(EResourceType::OMEN, TotalCost);
	Inventory->AddResource(Resource, Quantity);

	// Update market: remove supply, register demand
	MarketData->RemoveSupply(Resource, Quantity);
	MarketData->RegisterPlayerDemand(Resource, Quantity);

	// Record trade in price history
	PriceSystem->RecordTrade(Resource, PriceSystem->CalculateBuyPrice(Resource), Quantity, true);

	// Update statistics
	Statistics.TotalTradeVolume += Quantity;
	Statistics.TotalTransactionValue += TotalCost;

	// Create a small demand ripple from the purchase
	if (RippleEffectSystem && Quantity >= 50)
	{
		TArray<EResourceType> Resources;
		Resources.Add(Resource);
		float Intensity = FMath::Min(static_cast<float>(Quantity) / 500.0f, 0.3f);
		RippleEffectSystem->CreateDemandShockRipple(MarketId, Resources, Intensity);
	}

	OnTradeCompleted.Broadcast(MarketId, TotalCost);

	if (EconomyConfig.bEnableDetailedLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("EconomyManager: BUY %d x %s at %s for %d OMEN"),
			Quantity, *UEnum::GetValueAsString(Resource), *MarketId.ToString(), TotalCost);
	}

	return true;
}

bool UOdysseyEconomyManager::ExecuteSell(
	const FMarketId& MarketId,
	EResourceType Resource,
	int32 Quantity,
	AOdysseyCharacter* Seller)
{
	if (!CanSell(MarketId, Resource, Quantity) || !Seller)
	{
		return false;
	}

	UPriceFluctuationSystem* PriceSystem = GetPriceSystem(MarketId);
	UMarketDataComponent* MarketData = GetMarketData(MarketId);
	if (!PriceSystem || !MarketData) return false;

	// Check seller has the resource
	UOdysseyInventoryComponent* Inventory = Seller->GetInventoryComponent();
	if (!Inventory || !Inventory->HasResource(Resource, Quantity)) return false;

	int32 TotalRevenue = PriceSystem->CalculateSellPriceForQuantity(Resource, Quantity);

	// Apply tax
	int32 Tax = FMath::RoundToInt(TotalRevenue * MarketData->GetMarketData().TaxRate);
	int32 NetRevenue = TotalRevenue - Tax;

	// Execute transaction
	Inventory->RemoveResource(Resource, Quantity);
	Inventory->AddResource(EResourceType::OMEN, NetRevenue);

	// Update market: add supply
	MarketData->AddSupply(Resource, Quantity);

	// Record trade
	PriceSystem->RecordTrade(Resource, PriceSystem->CalculateSellPrice(Resource), Quantity, false);

	// Update statistics
	Statistics.TotalTradeVolume += Quantity;
	Statistics.TotalTransactionValue += TotalRevenue;

	// Large sells create supply ripple
	if (RippleEffectSystem && Quantity >= 50)
	{
		TArray<EResourceType> Resources;
		Resources.Add(Resource);
		float Intensity = FMath::Min(static_cast<float>(Quantity) / 500.0f, 0.3f);
		RippleEffectSystem->CreateSupplyShockRipple(MarketId, Resources, Intensity);
	}

	OnTradeCompleted.Broadcast(MarketId, TotalRevenue);

	return true;
}

int32 UOdysseyEconomyManager::GetBuyPrice(const FMarketId& MarketId, EResourceType Resource, int32 Quantity) const
{
	UPriceFluctuationSystem* PriceSystem = GetPriceSystem(MarketId);
	if (PriceSystem)
	{
		return Quantity > 1 ? PriceSystem->CalculateBuyPriceForQuantity(Resource, Quantity)
		                    : PriceSystem->CalculateBuyPrice(Resource);
	}
	return 0;
}

int32 UOdysseyEconomyManager::GetSellPrice(const FMarketId& MarketId, EResourceType Resource, int32 Quantity) const
{
	UPriceFluctuationSystem* PriceSystem = GetPriceSystem(MarketId);
	if (PriceSystem)
	{
		return Quantity > 1 ? PriceSystem->CalculateSellPriceForQuantity(Resource, Quantity)
		                    : PriceSystem->CalculateSellPrice(Resource);
	}
	return 0;
}

bool UOdysseyEconomyManager::CanBuy(const FMarketId& MarketId, EResourceType Resource, int32 Quantity) const
{
	UMarketDataComponent* MarketData = GetMarketData(MarketId);
	return MarketData && MarketData->GetCurrentSupply(Resource) >= Quantity;
}

bool UOdysseyEconomyManager::CanSell(const FMarketId& MarketId, EResourceType Resource, int32 Quantity) const
{
	// Markets can always accept sells (up to max supply)
	UMarketDataComponent* MarketData = GetMarketData(MarketId);
	if (!MarketData) return false;
	int32 RemainingCapacity = MarketData->GetMaxSupply(Resource) - MarketData->GetCurrentSupply(Resource);
	return RemainingCapacity >= Quantity;
}

// ============================================================================
// TRADE ROUTES & OPPORTUNITIES
// ============================================================================

TArray<FTradeOpportunity> UOdysseyEconomyManager::GetTopTradeOpportunities(int32 MaxCount) const
{
	if (TradeRouteAnalyzer)
	{
		return TradeRouteAnalyzer->GetTopOpportunities(MaxCount);
	}
	return TArray<FTradeOpportunity>();
}

TArray<FTradeOpportunity> UOdysseyEconomyManager::GetOpportunitiesFromLocation(FVector PlayerLocation, int32 MaxCount) const
{
	FMarketId NearestMarket = GetNearestMarket(PlayerLocation);
	if (NearestMarket.MarketName != NAME_None && TradeRouteAnalyzer)
	{
		return TradeRouteAnalyzer->GetOpportunitiesFrom(NearestMarket, MaxCount);
	}
	return TArray<FTradeOpportunity>();
}

FTradeRoute UOdysseyEconomyManager::FindBestRouteForResource(EResourceType Resource) const
{
	if (!TradeRouteAnalyzer) return FTradeRoute();

	FMarketId BestBuy = TradeRouteAnalyzer->FindBestBuyMarket(Resource);
	FMarketId BestSell = TradeRouteAnalyzer->FindBestSellMarket(Resource);

	if (BestBuy.MarketName != NAME_None && BestSell.MarketName != NAME_None)
	{
		return TradeRouteAnalyzer->GetRoute(BestBuy, BestSell);
	}
	return FTradeRoute();
}

// ============================================================================
// ECONOMIC EVENTS
// ============================================================================

int32 UOdysseyEconomyManager::TriggerEconomicEvent(EEconomicEventType EventType, const TArray<FMarketId>& Markets)
{
	if (!EconomicEventSystem) return -1;

	int32 EventId = EconomicEventSystem->TriggerEventByType(EventType, Markets);

	// Create ripple effect from the event
	if (EventId > 0 && RippleEffectSystem && Markets.Num() > 0)
	{
		FEconomicEvent Event = EconomicEventSystem->GetEvent(EventId);
		float Magnitude = FMath::Abs(Event.SupplyModifier - 1.0f) + FMath::Abs(Event.DemandModifier - 1.0f);
		Magnitude = FMath::Clamp(Magnitude, 0.1f, 1.0f);

		if (Event.SupplyModifier != 1.0f)
		{
			RippleEffectSystem->CreateSupplyShockRipple(
				Markets[0], Event.AffectedResources,
				Event.SupplyModifier > 1.0f ? Magnitude : -Magnitude,
				EventId);
		}
		if (Event.DemandModifier != 1.0f)
		{
			RippleEffectSystem->CreateDemandShockRipple(
				Markets[0], Event.AffectedResources,
				Event.DemandModifier > 1.0f ? Magnitude : -Magnitude,
				EventId);
		}
	}

	Statistics.TotalEventsGenerated++;
	return EventId;
}

TArray<FEconomicEvent> UOdysseyEconomyManager::GetActiveEvents() const
{
	if (EconomicEventSystem)
	{
		return EconomicEventSystem->GetActiveEvents();
	}
	return TArray<FEconomicEvent>();
}

TArray<FString> UOdysseyEconomyManager::GetEconomyNews(int32 MaxCount) const
{
	if (EconomicEventSystem)
	{
		return EconomicEventSystem->GetLatestHeadlines(MaxCount);
	}
	return TArray<FString>();
}

// ============================================================================
// COMBAT INTEGRATION
// ============================================================================

void UOdysseyEconomyManager::ReportCombatEvent(AActor* Attacker, AActor* Victim, float DamageDealt, bool bWasKill)
{
	if (!bIsInitialized) return;

	// Find nearest market to the combat
	FVector CombatLocation = Victim ? Victim->GetActorLocation() : FVector::ZeroVector;
	FMarketId NearestMarket = GetNearestMarket(CombatLocation);

	if (NearestMarket.MarketName == NAME_None) return;

	// Apply combat zone effects based on proximity and intensity
	float CombatIntensity = DamageDealt / 100.0f; // Normalize
	if (bWasKill) CombatIntensity *= 2.0f;

	ApplyCombatZoneEffects(NearestMarket, CombatIntensity);
}

int32 UOdysseyEconomyManager::CalculateCombatLootValue(AActor* DefeatedEnemy) const
{
	if (!DefeatedEnemy) return 0;

	// Base loot value scaled by combat impact settings
	float BaseValue = CombatImpact.BountyValue;

	// Modify based on resource prices (loot is more valuable in scarce markets)
	FVector EnemyLocation = DefeatedEnemy->GetActorLocation();
	FMarketId NearestMarket = GetNearestMarket(EnemyLocation);

	if (NearestMarket.MarketName != NAME_None)
	{
		UMarketDataComponent* MarketData = GetMarketData(NearestMarket);
		if (MarketData)
		{
			// Average scarcity across resources
			float AvgScarcity = 0.0f;
			int32 Count = 0;
			TArray<EResourceType> Resources = {
				EResourceType::Silicate, EResourceType::Carbon,
				EResourceType::RefinedSilicate, EResourceType::RefinedCarbon
			};
			for (EResourceType Res : Resources)
			{
				AvgScarcity += MarketData->GetScarcityIndex(Res);
				Count++;
			}
			if (Count > 0) AvgScarcity /= Count;

			// Higher scarcity = more valuable loot
			BaseValue *= (1.0f + AvgScarcity * 0.5f);
		}
	}

	return FMath::RoundToInt(BaseValue);
}

void UOdysseyEconomyManager::ApplyCombatZoneEffects(const FMarketId& NearestMarket, float CombatIntensity)
{
	CombatIntensity = FMath::Clamp(CombatIntensity, 0.0f, 1.0f);

	if (CombatIntensity < 0.1f) return; // Too minor to affect economy

	// Create combat zone ripple
	if (RippleEffectSystem)
	{
		RippleEffectSystem->CreateCombatZoneRipple(NearestMarket, CombatIntensity);
	}

	// High-intensity combat can trigger pirate activity events
	if (CombatIntensity > 0.5f && EconomicEventSystem)
	{
		float EventChance = CombatIntensity * CombatImpact.PirateActivityIncrease;
		if (FMath::FRand() < EventChance)
		{
			TArray<FMarketId> Markets;
			Markets.Add(NearestMarket);
			EconomicEventSystem->TriggerEventByType(EEconomicEventType::PirateActivity, Markets);
		}
	}
}

// ============================================================================
// CRAFTING INTEGRATION
// ============================================================================

void UOdysseyEconomyManager::ReportCraftingActivity(
	EResourceType ConsumedResource, int32 Quantity,
	EResourceType ProducedResource, int32 ProducedQuantity)
{
	if (!bIsInitialized) return;

	// Increase demand multiplier for consumed resource
	float& DemandMult = CraftingDemandMultipliers.FindOrAdd(ConsumedResource);
	DemandMult = FMath::Clamp(DemandMult + static_cast<float>(Quantity) * 0.01f * CraftingImpact.ResourceConsumptionMultiplier,
		1.0f, 3.0f);

	// Apply crafting demand to nearby markets (find the market with most supply of the ingredient)
	if (TradeRouteAnalyzer)
	{
		FMarketId BestSupplier = TradeRouteAnalyzer->FindBestBuyMarket(ConsumedResource);
		if (BestSupplier.MarketName != NAME_None)
		{
			UMarketDataComponent* MarketData = GetMarketData(BestSupplier);
			if (MarketData)
			{
				MarketData->RegisterPlayerDemand(ConsumedResource, Quantity);
			}

			// Create crafting demand ripple for larger crafting operations
			if (RippleEffectSystem && Quantity >= 20)
			{
				TArray<EResourceType> Ingredients;
				Ingredients.Add(ConsumedResource);
				float Intensity = FMath::Min(static_cast<float>(Quantity) / 200.0f, 0.5f);
				RippleEffectSystem->CreateCraftingDemandRipple(BestSupplier, Ingredients, Intensity);
			}
		}
	}
}

float UOdysseyEconomyManager::GetCraftedItemValueBonus(EResourceType CraftedResource) const
{
	return CraftingImpact.CraftedGoodsPriceBonus;
}

float UOdysseyEconomyManager::GetCraftingDemandMultiplier(EResourceType Resource) const
{
	const float* Mult = CraftingDemandMultipliers.Find(Resource);
	return Mult ? *Mult : 1.0f;
}

// ============================================================================
// STATISTICS & ANALYTICS
// ============================================================================

FEconomyStatistics UOdysseyEconomyManager::GetStatistics() const
{
	return Statistics;
}

EMarketTrend UOdysseyEconomyManager::GetGlobalPriceTrend(EResourceType Resource) const
{
	int32 BullCount = 0, BearCount = 0;

	for (const auto& Pair : PriceSystems)
	{
		if (Pair.Value)
		{
			FDynamicMarketPrice PriceData = Pair.Value->GetPriceData(Resource);
			switch (PriceData.CurrentTrend)
			{
				case EMarketTrend::StrongBull:
				case EMarketTrend::Bull:
					BullCount++;
					break;
				case EMarketTrend::StrongBear:
				case EMarketTrend::Bear:
					BearCount++;
					break;
				default:
					break;
			}
		}
	}

	if (BullCount > BearCount * 2) return EMarketTrend::StrongBull;
	if (BullCount > BearCount) return EMarketTrend::Bull;
	if (BearCount > BullCount * 2) return EMarketTrend::StrongBear;
	if (BearCount > BullCount) return EMarketTrend::Bear;
	return EMarketTrend::Neutral;
}

float UOdysseyEconomyManager::GetAverageMarketPrice(EResourceType Resource) const
{
	float Sum = 0.0f;
	int32 Count = 0;

	for (const auto& Pair : PriceSystems)
	{
		if (Pair.Value)
		{
			int32 Price = Pair.Value->CalculateBuyPrice(Resource);
			if (Price > 0)
			{
				Sum += Price;
				Count++;
			}
		}
	}

	return Count > 0 ? Sum / Count : 0.0f;
}

void UOdysseyEconomyManager::GetPriceRange(EResourceType Resource, int32& OutMinPrice, int32& OutMaxPrice) const
{
	OutMinPrice = INT32_MAX;
	OutMaxPrice = 0;

	for (const auto& Pair : PriceSystems)
	{
		if (Pair.Value)
		{
			int32 Price = Pair.Value->CalculateBuyPrice(Resource);
			if (Price > 0)
			{
				OutMinPrice = FMath::Min(OutMinPrice, Price);
				OutMaxPrice = FMath::Max(OutMaxPrice, Price);
			}
		}
	}

	if (OutMinPrice == INT32_MAX) OutMinPrice = 0;
}

// ============================================================================
// PLAYER ECONOMY
// ============================================================================

int32 UOdysseyEconomyManager::CalculatePlayerNetWorth(AOdysseyCharacter* Player) const
{
	if (!Player) return 0;

	UOdysseyInventoryComponent* Inventory = Player->GetInventoryComponent();
	if (!Inventory) return 0;

	int32 NetWorth = 0;

	// Count OMEN directly
	NetWorth += Inventory->GetResourceAmount(EResourceType::OMEN);

	// Value other resources at their average market sell price
	TArray<EResourceType> Resources = {
		EResourceType::Silicate, EResourceType::Carbon,
		EResourceType::RefinedSilicate, EResourceType::RefinedCarbon,
		EResourceType::CompositeMaterial
	};

	for (EResourceType Resource : Resources)
	{
		int32 Amount = Inventory->GetResourceAmount(Resource);
		if (Amount > 0)
		{
			float AvgPrice = GetAverageMarketPrice(Resource);
			NetWorth += FMath::RoundToInt(Amount * AvgPrice * 0.8f); // 80% of buy price as sell estimate
		}
	}

	return NetWorth;
}

FString UOdysseyEconomyManager::GetPlayerTradingSummary(AOdysseyCharacter* Player) const
{
	if (!Player) return TEXT("No player data");

	int32 NetWorth = CalculatePlayerNetWorth(Player);
	return FString::Printf(TEXT("Net Worth: %d OMEN | Markets: %d | Active Events: %d"),
		NetWorth, RegisteredMarkets.Num(),
		EconomicEventSystem ? EconomicEventSystem->GetActiveEventCount() : 0);
}

TArray<FTradeOpportunity> UOdysseyEconomyManager::GetRecommendedTrades(AOdysseyCharacter* Player, int32 MaxCount) const
{
	if (!Player || !TradeRouteAnalyzer)
	{
		return TArray<FTradeOpportunity>();
	}

	// Get player location and find nearby opportunities
	FVector PlayerLocation = Player->GetActorLocation();
	FMarketId NearestMarket = GetNearestMarket(PlayerLocation);

	if (NearestMarket.MarketName != NAME_None)
	{
		return TradeRouteAnalyzer->GetOpportunitiesFrom(NearestMarket, MaxCount);
	}

	return TradeRouteAnalyzer->GetTopOpportunities(MaxCount);
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void UOdysseyEconomyManager::UpdateConfiguration(const FEconomyConfiguration& NewConfig)
{
	EconomyConfig = NewConfig;

	if (TradeRouteAnalyzer)
	{
		TradeRouteAnalyzer->SetAnalysisInterval(NewConfig.TradeRouteAnalysisIntervalSeconds);
		TradeRouteAnalyzer->SetMaxOpportunities(NewConfig.MaxTradeOpportunities);
	}

	if (RippleEffectSystem)
	{
		RippleEffectSystem->SetConfiguration(NewConfig);
	}

	// Update all price systems
	for (auto& Pair : PriceSystems)
	{
		if (Pair.Value)
		{
			Pair.Value->SetConfiguration(NewConfig);
		}
	}
}

void UOdysseyEconomyManager::SetTimeScale(float Scale)
{
	TimeScale = FMath::Clamp(Scale, 0.0f, 10.0f);
}

void UOdysseyEconomyManager::SetSimulationPaused(bool bPaused)
{
	bSimulationPaused = bPaused;
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

void UOdysseyEconomyManager::HandleCombatEvent(const FCombatEventPayload& Payload)
{
	bool bWasKill = Payload.DamageAmount >= 100.0f; // Simplified kill detection
	ReportCombatEvent(
		Payload.Attacker.Get(),
		Payload.Target.Get(),
		Payload.DamageAmount,
		bWasKill);
}

void UOdysseyEconomyManager::HandleInteractionEvent(const FInteractionEventPayload& Payload)
{
	// Mining interactions add supply at nearest market
	if (Payload.InteractionType == FName("Mining"))
	{
		AActor* Miner = Payload.Source.Get();
		if (Miner)
		{
			FMarketId NearestMarket = GetNearestMarket(Miner->GetActorLocation());
			if (NearestMarket.MarketName != NAME_None)
			{
				UMarketDataComponent* MarketData = GetMarketData(NearestMarket);
				if (MarketData)
				{
					for (const auto& Pair : Payload.ResultItems)
					{
						// ResultItems maps FName -> int32, need to convert to resource type
						// Simple mapping for common resources
						EResourceType Resource = EResourceType::None;
						if (Pair.Key == FName("Silicate")) Resource = EResourceType::Silicate;
						else if (Pair.Key == FName("Carbon")) Resource = EResourceType::Carbon;

						if (Resource != EResourceType::None && Pair.Value > 0)
						{
							// Mining adds potential supply to the regional market
							MarketData->AddSupply(Resource, Pair.Value / 2); // Half goes to market
						}
					}
				}
			}
		}
	}
}

void UOdysseyEconomyManager::UpdateStatistics()
{
	Statistics.ActiveMarkets = RegisteredMarkets.Num();
	Statistics.ActiveTradeRoutes = TradeRouteAnalyzer ? TradeRouteAnalyzer->GetAllRoutes().Num() : 0;

	// Calculate average volatility
	float TotalVolatility = 0.0f;
	int32 VolatilityCount = 0;
	for (const auto& Pair : MarketDataComponents)
	{
		if (Pair.Value)
		{
			TArray<EResourceType> Resources = {
				EResourceType::Silicate, EResourceType::Carbon
			};
			for (EResourceType Res : Resources)
			{
				TotalVolatility += Pair.Value->GetPriceVolatility(Res);
				VolatilityCount++;
			}
		}
	}
	Statistics.AveragePriceVolatility = VolatilityCount > 0 ? TotalVolatility / VolatilityCount : 0.0f;
}

FName UOdysseyEconomyManager::GetMarketKey(const FMarketId& MarketId) const
{
	return FName(*MarketId.ToString());
}

void UOdysseyEconomyManager::BroadcastPriceChange(EResourceType Resource, const FDynamicMarketPrice& Price)
{
	OnPriceChanged.Broadcast(Resource, Price);
}

// ============================================================================
// INTERNAL EVENT HANDLERS (bound to subsystem delegates)
// ============================================================================

void UOdysseyEconomyManager::HandlePriceChanged(EResourceType Resource, const FDynamicMarketPrice& NewPrice)
{
	OnPriceChanged.Broadcast(Resource, NewPrice);
}

void UOdysseyEconomyManager::HandleEconomicEventStarted(const FEconomicEvent& Event)
{
	OnEventStarted.Broadcast(Event);
}

void UOdysseyEconomyManager::HandleEconomicEventEnded(const FEconomicEvent& Event)
{
	OnEventEnded.Broadcast(Event);
}

void UOdysseyEconomyManager::HandleOpportunityFound(const FTradeOpportunity& Opportunity)
{
	OnOpportunityFound.Broadcast(Opportunity);
}

// ============================================================================
// SAVE/LOAD CONVENIENCE API
// ============================================================================

bool UOdysseyEconomyManager::QuickSave()
{
	return SaveSystem ? SaveSystem->SaveEconomyToDisk() : false;
}

bool UOdysseyEconomyManager::QuickLoad()
{
	return SaveSystem ? SaveSystem->LoadEconomyFromDisk() : false;
}
