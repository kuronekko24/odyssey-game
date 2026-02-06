// EconomySaveSystem.h
// Serialization system for persistent economy state (Task #5)
//
// Provides save/load functionality for the entire economy simulation.
// The economy state is serialized to an FEconomySaveData struct which can
// be persisted to disk via UE5's SaveGame system or custom serialization.
//
// Design rationale:
// - Uses UPROPERTY(SaveGame) for UE5 native serialization support
// - Snapshot-based: captures full state at a point in time
// - Incremental-friendly: callers can serialize partial state for autosave
// - Decoupled: knows about data structures but not about the systems themselves

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SaveGame.h"
#include "OdysseyEconomyTypes.h"
#include "EconomySaveSystem.generated.h"

// Forward declarations
class UMarketDataComponent;
class UPriceFluctuationSystem;
class UTradeRouteAnalyzer;
class UEconomicEventSystem;

/**
 * USaveGame subclass that wraps the economy save data for UE5 serialization
 */
UCLASS()
class ODYSSEY_API UOdysseyEconomySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	FEconomySaveData EconomyData;

	UOdysseyEconomySaveGame()
	{
		EconomyData.SaveVersion = 1;
	}
};

/**
 * UEconomySaveSystem - Economy State Serialization Component
 *
 * Responsibilities:
 * - Capture full economy snapshot into serializable data
 * - Restore economy state from saved data
 * - Manage save slots and autosave timing
 * - Validate save data integrity on load
 * - Handle version migration for save format changes
 */
UCLASS(ClassGroup=(Economy), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UEconomySaveSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UEconomySaveSystem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// INITIALIZATION
	// ============================================================================

	/**
	 * Set references to all economy subsystems for snapshot capture
	 */
	void SetEconomyReferences(
		TMap<FName, UMarketDataComponent*>* InMarketDataMap,
		TMap<FName, UPriceFluctuationSystem*>* InPriceSystemMap,
		UTradeRouteAnalyzer* InTradeRouteAnalyzer,
		UEconomicEventSystem* InEventSystem,
		const TArray<FMarketId>* InRegisteredMarkets);

	// ============================================================================
	// SNAPSHOT
	// ============================================================================

	/**
	 * Capture the current economy state into a save data struct
	 * @return Complete economy snapshot
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save")
	FEconomySaveData CaptureEconomySnapshot() const;

	/**
	 * Restore economy state from save data
	 * This will overwrite all current market data, prices, events, etc.
	 * @param SaveData The snapshot to restore from
	 * @return True if restoration was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save")
	bool RestoreEconomyFromSnapshot(const FEconomySaveData& SaveData);

	// ============================================================================
	// DISK SAVE/LOAD (UE5 SaveGame system)
	// ============================================================================

	/**
	 * Save economy to disk using UE5 SaveGame
	 * @param SlotName Save slot identifier
	 * @param UserIndex Local player index (for multi-user support)
	 * @return True if save was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save")
	bool SaveEconomyToDisk(const FString& SlotName = TEXT("EconomySave"), int32 UserIndex = 0);

	/**
	 * Load economy from disk
	 * @param SlotName Save slot identifier
	 * @param UserIndex Local player index
	 * @return True if load was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save")
	bool LoadEconomyFromDisk(const FString& SlotName = TEXT("EconomySave"), int32 UserIndex = 0);

	/**
	 * Check if a save exists at the given slot
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save")
	bool DoesSaveExist(const FString& SlotName = TEXT("EconomySave"), int32 UserIndex = 0) const;

	/**
	 * Delete a save slot
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save")
	bool DeleteSave(const FString& SlotName = TEXT("EconomySave"), int32 UserIndex = 0);

	// ============================================================================
	// AUTOSAVE
	// ============================================================================

	/**
	 * Enable periodic autosave
	 * @param IntervalSeconds How often to autosave (0 = disable)
	 * @param SlotName Save slot for autosave
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save|Autosave")
	void EnableAutosave(float IntervalSeconds, const FString& SlotName = TEXT("EconomyAutosave"));

	/**
	 * Disable autosave
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save|Autosave")
	void DisableAutosave();

	/**
	 * Check if autosave is enabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save|Autosave")
	bool IsAutosaveEnabled() const { return bAutosaveEnabled && AutosaveIntervalSeconds > 0.0f; }

	// ============================================================================
	// VALIDATION
	// ============================================================================

	/**
	 * Validate save data integrity
	 * @param SaveData Data to validate
	 * @return True if data appears valid
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save|Validation")
	bool ValidateSaveData(const FEconomySaveData& SaveData) const;

	/**
	 * Get the current save format version
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy Save|Validation")
	int32 GetCurrentSaveVersion() const { return CurrentSaveVersion; }

	// ============================================================================
	// EVENTS
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Economy Save|Events")
	FOnEconomySaved OnEconomySaved;

	UPROPERTY(BlueprintAssignable, Category = "Economy Save|Events")
	FOnEconomyLoaded OnEconomyLoaded;

protected:
	// Current save format version (increment when format changes)
	static const int32 CurrentSaveVersion = 1;

	// External references (not owned)
	TMap<FName, UMarketDataComponent*>* MarketDataMap;
	TMap<FName, UPriceFluctuationSystem*>* PriceSystemMap;

	UPROPERTY()
	UTradeRouteAnalyzer* TradeRouteAnalyzer;

	UPROPERTY()
	UEconomicEventSystem* EventSystem;

	const TArray<FMarketId>* RegisteredMarkets;

	// Autosave state
	bool bAutosaveEnabled;
	float AutosaveIntervalSeconds;
	float TimeSinceLastAutosave;
	FString AutosaveSlotName;

private:
	/**
	 * Capture a single market's data into a save-friendly struct
	 */
	FMarketSaveData CaptureMarketSnapshot(const FMarketId& MarketId) const;

	/**
	 * Restore a single market from save data
	 */
	bool RestoreMarketFromSnapshot(const FMarketSaveData& MarketSave);

	/**
	 * Migrate save data from an older version to the current version
	 * @return True if migration was successful
	 */
	bool MigrateSaveData(FEconomySaveData& SaveData) const;

	FName GetMarketKey(const FMarketId& MarketId) const;
};
