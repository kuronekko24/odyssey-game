// EconomySaveSystem.cpp
// Implementation of economy state serialization

#include "EconomySaveSystem.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.h"
#include "UEconomicEventSystem.h"
#include "Kismet/GameplayStatics.h"

UEconomySaveSystem::UEconomySaveSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 5.0f; // Check autosave infrequently

	MarketDataMap = nullptr;
	PriceSystemMap = nullptr;
	TradeRouteAnalyzer = nullptr;
	EventSystem = nullptr;
	RegisteredMarkets = nullptr;

	bAutosaveEnabled = false;
	AutosaveIntervalSeconds = 300.0f; // 5 minutes default
	TimeSinceLastAutosave = 0.0f;
	AutosaveSlotName = TEXT("EconomyAutosave");
}

void UEconomySaveSystem::BeginPlay()
{
	Super::BeginPlay();
}

void UEconomySaveSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Autosave logic
	if (bAutosaveEnabled && AutosaveIntervalSeconds > 0.0f)
	{
		TimeSinceLastAutosave += DeltaTime;
		if (TimeSinceLastAutosave >= AutosaveIntervalSeconds)
		{
			SaveEconomyToDisk(AutosaveSlotName, 0);
			TimeSinceLastAutosave = 0.0f;
		}
	}
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void UEconomySaveSystem::SetEconomyReferences(
	TMap<FName, UMarketDataComponent*>* InMarketDataMap,
	TMap<FName, UPriceFluctuationSystem*>* InPriceSystemMap,
	UTradeRouteAnalyzer* InTradeRouteAnalyzer,
	UEconomicEventSystem* InEventSystem,
	const TArray<FMarketId>* InRegisteredMarkets)
{
	MarketDataMap = InMarketDataMap;
	PriceSystemMap = InPriceSystemMap;
	TradeRouteAnalyzer = InTradeRouteAnalyzer;
	EventSystem = InEventSystem;
	RegisteredMarkets = InRegisteredMarkets;
}

// ============================================================================
// SNAPSHOT
// ============================================================================

FEconomySaveData UEconomySaveSystem::CaptureEconomySnapshot() const
{
	FEconomySaveData SaveData;
	SaveData.SaveVersion = CurrentSaveVersion;
	SaveData.SaveTimestamp = FPlatformTime::Seconds();

	// Capture all market states
	if (RegisteredMarkets)
	{
		for (const FMarketId& MarketId : *RegisteredMarkets)
		{
			FMarketSaveData MarketSave = CaptureMarketSnapshot(MarketId);
			SaveData.Markets.Add(MarketSave);
		}
	}

	// Capture trade routes
	if (TradeRouteAnalyzer)
	{
		SaveData.TradeRoutes = TradeRouteAnalyzer->GetAllRoutes();
	}

	// Capture active events and history
	if (EventSystem)
	{
		SaveData.ActiveEvents = EventSystem->GetActiveEvents();
		SaveData.EventHistory = EventSystem->GetEventHistory(50);
		SaveData.TotalEventsGenerated = SaveData.ActiveEvents.Num() + SaveData.EventHistory.Num();
	}

	// Calculate global trade volume
	SaveData.TotalGlobalTradeVolume = 0;
	for (const FMarketSaveData& Market : SaveData.Markets)
	{
		SaveData.TotalGlobalTradeVolume += Market.TotalTradeVolume;
	}

	return SaveData;
}

bool UEconomySaveSystem::RestoreEconomyFromSnapshot(const FEconomySaveData& SaveData)
{
	// Validate first
	if (!ValidateSaveData(SaveData))
	{
		UE_LOG(LogTemp, Error, TEXT("EconomySaveSystem: Save data validation failed"));
		return false;
	}

	// Handle version migration if needed
	FEconomySaveData MutableData = SaveData;
	if (MutableData.SaveVersion != CurrentSaveVersion)
	{
		if (!MigrateSaveData(MutableData))
		{
			UE_LOG(LogTemp, Error, TEXT("EconomySaveSystem: Save data migration failed from version %d to %d"),
				MutableData.SaveVersion, CurrentSaveVersion);
			return false;
		}
	}

	// Restore each market
	for (const FMarketSaveData& MarketSave : MutableData.Markets)
	{
		RestoreMarketFromSnapshot(MarketSave);
	}

	// Restore active events
	if (EventSystem)
	{
		// Cancel all current events first
		TArray<FEconomicEvent> CurrentEvents = EventSystem->GetActiveEvents();
		for (const FEconomicEvent& Event : CurrentEvents)
		{
			EventSystem->CancelEvent(Event.EventId);
		}

		// Re-trigger saved events
		for (const FEconomicEvent& Event : MutableData.ActiveEvents)
		{
			if (Event.bIsActive)
			{
				EventSystem->TriggerEvent(Event);
			}
		}
	}

	OnEconomyLoaded.Broadcast(MutableData);

	UE_LOG(LogTemp, Log, TEXT("EconomySaveSystem: Restored economy with %d markets, %d events"),
		MutableData.Markets.Num(), MutableData.ActiveEvents.Num());

	return true;
}

// ============================================================================
// DISK SAVE/LOAD
// ============================================================================

bool UEconomySaveSystem::SaveEconomyToDisk(const FString& SlotName, int32 UserIndex)
{
	UOdysseyEconomySaveGame* SaveGameInstance = Cast<UOdysseyEconomySaveGame>(
		UGameplayStatics::CreateSaveGameObject(UOdysseyEconomySaveGame::StaticClass()));

	if (!SaveGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("EconomySaveSystem: Failed to create SaveGame object"));
		return false;
	}

	SaveGameInstance->EconomyData = CaptureEconomySnapshot();

	bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName, UserIndex);

	if (bSuccess)
	{
		OnEconomySaved.Broadcast(SaveGameInstance->EconomyData);
		UE_LOG(LogTemp, Log, TEXT("EconomySaveSystem: Economy saved to slot '%s'"), *SlotName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EconomySaveSystem: Failed to save economy to slot '%s'"), *SlotName);
	}

	return bSuccess;
}

bool UEconomySaveSystem::LoadEconomyFromDisk(const FString& SlotName, int32 UserIndex)
{
	if (!DoesSaveExist(SlotName, UserIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("EconomySaveSystem: No save found at slot '%s'"), *SlotName);
		return false;
	}

	UOdysseyEconomySaveGame* LoadedGame = Cast<UOdysseyEconomySaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));

	if (!LoadedGame)
	{
		UE_LOG(LogTemp, Error, TEXT("EconomySaveSystem: Failed to load from slot '%s'"), *SlotName);
		return false;
	}

	bool bSuccess = RestoreEconomyFromSnapshot(LoadedGame->EconomyData);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("EconomySaveSystem: Economy loaded from slot '%s'"), *SlotName);
	}

	return bSuccess;
}

bool UEconomySaveSystem::DoesSaveExist(const FString& SlotName, int32 UserIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

bool UEconomySaveSystem::DeleteSave(const FString& SlotName, int32 UserIndex)
{
	return UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
}

// ============================================================================
// AUTOSAVE
// ============================================================================

void UEconomySaveSystem::EnableAutosave(float IntervalSeconds, const FString& SlotName)
{
	bAutosaveEnabled = true;
	AutosaveIntervalSeconds = FMath::Max(30.0f, IntervalSeconds); // Min 30 seconds
	AutosaveSlotName = SlotName;
	TimeSinceLastAutosave = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("EconomySaveSystem: Autosave enabled every %.0f seconds to slot '%s'"),
		AutosaveIntervalSeconds, *AutosaveSlotName);
}

void UEconomySaveSystem::DisableAutosave()
{
	bAutosaveEnabled = false;
	UE_LOG(LogTemp, Log, TEXT("EconomySaveSystem: Autosave disabled"));
}

// ============================================================================
// VALIDATION
// ============================================================================

bool UEconomySaveSystem::ValidateSaveData(const FEconomySaveData& SaveData) const
{
	// Version check
	if (SaveData.SaveVersion <= 0 || SaveData.SaveVersion > CurrentSaveVersion + 10)
	{
		UE_LOG(LogTemp, Warning, TEXT("EconomySaveSystem: Invalid save version %d"), SaveData.SaveVersion);
		return false;
	}

	// Timestamp sanity check
	if (SaveData.SaveTimestamp < 0.0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EconomySaveSystem: Invalid save timestamp"));
		return false;
	}

	// Validate market data
	for (const FMarketSaveData& Market : SaveData.Markets)
	{
		if (Market.MarketId.MarketName == NAME_None)
		{
			UE_LOG(LogTemp, Warning, TEXT("EconomySaveSystem: Market with invalid ID found"));
			return false;
		}

		// Check for reasonable supply values
		for (const auto& Pair : Market.SupplyDemandData)
		{
			if (Pair.Value.CurrentSupply < 0 || Pair.Value.MaxSupply < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("EconomySaveSystem: Negative supply values in market %s"),
					*Market.MarketId.ToString());
				return false;
			}
		}
	}

	return true;
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

FMarketSaveData UEconomySaveSystem::CaptureMarketSnapshot(const FMarketId& MarketId) const
{
	FMarketSaveData MarketSave;
	MarketSave.MarketId = MarketId;

	FName Key = GetMarketKey(MarketId);

	// Capture market data
	if (MarketDataMap)
	{
		UMarketDataComponent* const* MarketData = MarketDataMap->Find(Key);
		if (MarketData && *MarketData)
		{
			FMarketData FullData = (*MarketData)->GetMarketData();
			MarketSave.DisplayName = FullData.DisplayName;
			MarketSave.LocationType = FullData.LocationType;
			MarketSave.WorldLocation = FullData.WorldLocation;
			MarketSave.SupplyDemandData = FullData.SupplyDemandData;
			MarketSave.SpecializedResources = FullData.SpecializedResources;
			MarketSave.TaxRate = FullData.TaxRate;
			MarketSave.TotalTradeVolume = FullData.TotalTradeVolume;
		}
	}

	// Capture price data
	if (PriceSystemMap)
	{
		UPriceFluctuationSystem* const* PriceSystem = PriceSystemMap->Find(Key);
		if (PriceSystem && *PriceSystem)
		{
			MarketSave.ResourcePrices = (*PriceSystem)->GetAllCurrentPrices();
		}
	}

	return MarketSave;
}

bool UEconomySaveSystem::RestoreMarketFromSnapshot(const FMarketSaveData& MarketSave)
{
	FName Key = GetMarketKey(MarketSave.MarketId);

	// Restore market data component
	if (MarketDataMap)
	{
		UMarketDataComponent** MarketData = MarketDataMap->Find(Key);
		if (MarketData && *MarketData)
		{
			// Reset and re-initialize with saved data
			(*MarketData)->ResetToDefaults();
			(*MarketData)->InitializeMarketData(MarketSave.MarketId, MarketSave.DisplayName);

			// Restore supply/demand for each resource
			for (const auto& Pair : MarketSave.SupplyDemandData)
			{
				(*MarketData)->AddSupply(Pair.Key, Pair.Value.CurrentSupply);
				(*MarketData)->SetSupplyRate(Pair.Key, Pair.Value.SupplyRate);
				(*MarketData)->SetSupplyModifier(Pair.Key, Pair.Value.SupplyModifier);
				(*MarketData)->SetDemandRate(Pair.Key, Pair.Value.DemandRate);
				(*MarketData)->SetDemandModifier(Pair.Key, Pair.Value.DemandModifier);
			}

			(*MarketData)->RecalculateAllMetrics();
		}
	}

	// Restore price system
	if (PriceSystemMap)
	{
		UPriceFluctuationSystem** PriceSystem = PriceSystemMap->Find(Key);
		if (PriceSystem && *PriceSystem)
		{
			// Apply saved prices
			for (const auto& Pair : MarketSave.ResourcePrices)
			{
				(*PriceSystem)->ApplyPriceShock(
					Pair.Key,
					Pair.Value.PriceMultiplier,
					0.0f // No decay -- this is a restore, not a shock
				);
			}
		}
	}

	return true;
}

bool UEconomySaveSystem::MigrateSaveData(FEconomySaveData& SaveData) const
{
	// Currently at version 1, no migrations needed yet.
	// Future migrations would go here:
	//
	// if (SaveData.SaveVersion == 1)
	// {
	//     // Migrate v1 -> v2
	//     SaveData.SaveVersion = 2;
	// }
	//
	// if (SaveData.SaveVersion == 2)
	// {
	//     // Migrate v2 -> v3
	//     SaveData.SaveVersion = 3;
	// }

	if (SaveData.SaveVersion != CurrentSaveVersion)
	{
		UE_LOG(LogTemp, Warning, TEXT("EconomySaveSystem: Unknown save version %d, cannot migrate"), SaveData.SaveVersion);
		return false;
	}

	return true;
}

FName UEconomySaveSystem::GetMarketKey(const FMarketId& MarketId) const
{
	return FName(*MarketId.ToString());
}
