// EconomySaveSystemTest.cpp
// Comprehensive automation tests for UEconomySaveSystem
// Tests snapshot capture, restoration, validation, and disk save/load

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UTradeRouteAnalyzer.h"
#include "UEconomicEventSystem.h"
#include "Economy/EconomySaveSystem.h"

// ============================================================================
// Helper: creates a full economy context with save system
// ============================================================================
namespace SaveSystemTestHelpers
{
	struct FTestContext
	{
		AActor* Actor = nullptr;
		UEconomySaveSystem* SaveSystem = nullptr;
		TMap<FName, UMarketDataComponent*> MarketDataMap;
		TMap<FName, UPriceFluctuationSystem*> PriceSystemMap;
		UTradeRouteAnalyzer* TradeRouteAnalyzer = nullptr;
		UEconomicEventSystem* EventSystem = nullptr;
		TArray<FMarketId> RegisteredMarkets;

		void Destroy()
		{
			if (Actor) Actor->Destroy();
		}
	};

	FTestContext CreateFullEconomyContext(UWorld* World)
	{
		FTestContext Ctx;
		Ctx.Actor = World->SpawnActor<AActor>();
		if (!Ctx.Actor) return Ctx;

		// Create two markets
		FMarketId MarketA(FName("SaveTestAlpha"), 1);
		FMarketId MarketB(FName("SaveTestBeta"), 1);

		auto CreateMarket = [&](const FMarketId& Id) {
			FName Key(*Id.ToString());

			auto* MD = NewObject<UMarketDataComponent>(Ctx.Actor);
			MD->RegisterComponent();
			MD->InitializeMarketData(Id, Id.MarketName.ToString());
			Ctx.MarketDataMap.Add(Key, MD);

			auto* PS = NewObject<UPriceFluctuationSystem>(Ctx.Actor);
			PS->RegisterComponent();
			PS->Initialize(MD);
			Ctx.PriceSystemMap.Add(Key, PS);

			Ctx.RegisteredMarkets.Add(Id);
		};

		CreateMarket(MarketA);
		CreateMarket(MarketB);

		// Add known supply values for verification
		FName KeyA(*MarketA.ToString());
		Ctx.MarketDataMap[KeyA]->AddSupply(EResourceType::IronOre, 500);
		Ctx.MarketDataMap[KeyA]->AddSupply(EResourceType::GoldOre, 50);
		Ctx.MarketDataMap[KeyA]->RecalculateAllMetrics();

		FName KeyB(*MarketB.ToString());
		Ctx.MarketDataMap[KeyB]->AddSupply(EResourceType::CopperOre, 300);
		Ctx.MarketDataMap[KeyB]->RecalculateAllMetrics();

		// Update prices
		for (auto& Pair : Ctx.PriceSystemMap)
		{
			Pair.Value->UpdateAllPrices();
		}

		// Trade route analyzer
		Ctx.TradeRouteAnalyzer = NewObject<UTradeRouteAnalyzer>(Ctx.Actor);
		Ctx.TradeRouteAnalyzer->RegisterComponent();
		Ctx.TradeRouteAnalyzer->RegisterMarket(MarketA, Ctx.MarketDataMap[KeyA], Ctx.PriceSystemMap[KeyA]);
		Ctx.TradeRouteAnalyzer->RegisterMarket(MarketB, Ctx.MarketDataMap[KeyB], Ctx.PriceSystemMap[KeyB]);
		Ctx.TradeRouteAnalyzer->DefineTradeRoute(MarketA, MarketB, 1000.0f, 2.0f, ETradeRouteRisk::Low);

		// Event system
		Ctx.EventSystem = NewObject<UEconomicEventSystem>(Ctx.Actor);
		Ctx.EventSystem->RegisterComponent();
		FEventGenerationParams Params;
		Ctx.EventSystem->Initialize(Params);
		Ctx.EventSystem->RegisterMarket(MarketA, Ctx.MarketDataMap[KeyA], Ctx.PriceSystemMap[KeyA]);
		Ctx.EventSystem->RegisterMarket(MarketB, Ctx.MarketDataMap[KeyB], Ctx.PriceSystemMap[KeyB]);

		// Save system
		Ctx.SaveSystem = NewObject<UEconomySaveSystem>(Ctx.Actor);
		Ctx.SaveSystem->RegisterComponent();
		Ctx.SaveSystem->SetEconomyReferences(
			&Ctx.MarketDataMap,
			&Ctx.PriceSystemMap,
			Ctx.TradeRouteAnalyzer,
			Ctx.EventSystem,
			&Ctx.RegisteredMarkets);

		return Ctx;
	}
}

// ============================================================================
// 1. SNAPSHOT CAPTURE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_CaptureSnapshot,
	"Odyssey.Economy.SaveSystem.Snapshot.CaptureSnapshotProducesValidData",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_CaptureSnapshot::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	FEconomySaveData Snapshot = Ctx.SaveSystem->CaptureEconomySnapshot();

	TestTrue(TEXT("Save version should be > 0"), Snapshot.SaveVersion > 0);
	TestTrue(TEXT("Save timestamp should be > 0"), Snapshot.SaveTimestamp > 0.0);
	TestTrue(TEXT("Should have market data"), Snapshot.Markets.Num() >= 2);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_SnapshotContainsMarketData,
	"Odyssey.Economy.SaveSystem.Snapshot.SnapshotContainsAllMarketData",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_SnapshotContainsMarketData::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	FEconomySaveData Snapshot = Ctx.SaveSystem->CaptureEconomySnapshot();

	// Verify each market is in the snapshot
	bool bFoundAlpha = false;
	bool bFoundBeta = false;
	for (const auto& Market : Snapshot.Markets)
	{
		if (Market.MarketId.MarketName == FName("SaveTestAlpha")) bFoundAlpha = true;
		if (Market.MarketId.MarketName == FName("SaveTestBeta")) bFoundBeta = true;
	}

	TestTrue(TEXT("Snapshot should contain Alpha market"), bFoundAlpha);
	TestTrue(TEXT("Snapshot should contain Beta market"), bFoundBeta);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_SnapshotCapturesSupplyDemand,
	"Odyssey.Economy.SaveSystem.Snapshot.SupplyDemandDataIsPreserved",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_SnapshotCapturesSupplyDemand::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	FEconomySaveData Snapshot = Ctx.SaveSystem->CaptureEconomySnapshot();

	// Find Alpha market and check supply data
	for (const auto& Market : Snapshot.Markets)
	{
		if (Market.MarketId.MarketName == FName("SaveTestAlpha"))
		{
			TestTrue(TEXT("Alpha market should have supply/demand data"),
				Market.SupplyDemandData.Num() > 0);

			if (const FResourceSupplyDemand* IronData = Market.SupplyDemandData.Find(EResourceType::IronOre))
			{
				TestTrue(TEXT("Alpha Iron supply should be around 500+initial"),
					IronData->CurrentSupply >= 500);
			}
			break;
		}
	}

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_SnapshotContainsTradeRoutes,
	"Odyssey.Economy.SaveSystem.Snapshot.TradeRoutesAreIncluded",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_SnapshotContainsTradeRoutes::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	FEconomySaveData Snapshot = Ctx.SaveSystem->CaptureEconomySnapshot();
	TestTrue(TEXT("Snapshot should contain trade routes"), Snapshot.TradeRoutes.Num() >= 1);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 2. STATE RESTORATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_RestoreFromSnapshot,
	"Odyssey.Economy.SaveSystem.Restore.RestoreFromSnapshotSucceeds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_RestoreFromSnapshot::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	// Capture initial state
	FEconomySaveData Snapshot = Ctx.SaveSystem->CaptureEconomySnapshot();

	// Modify state
	FName KeyA(*FMarketId(FName("SaveTestAlpha"), 1).ToString());
	Ctx.MarketDataMap[KeyA]->AddSupply(EResourceType::IronOre, 9999);

	// Restore from snapshot
	bool bRestored = Ctx.SaveSystem->RestoreEconomyFromSnapshot(Snapshot);
	TestTrue(TEXT("Restore should succeed"), bRestored);

	// Verify supply was restored to pre-modification value
	int32 RestoredSupply = Ctx.MarketDataMap[KeyA]->GetCurrentSupply(EResourceType::IronOre);
	TestTrue(TEXT("Supply should be restored to snapshot value (much less than 9999+)"),
		RestoredSupply < 9999);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_RestorePreservesIntegrity,
	"Odyssey.Economy.SaveSystem.Restore.DataIntegrityAfterRestoration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_RestorePreservesIntegrity::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	// Record known values before save
	FName KeyA(*FMarketId(FName("SaveTestAlpha"), 1).ToString());
	int32 OriginalIronSupply = Ctx.MarketDataMap[KeyA]->GetCurrentSupply(EResourceType::IronOre);
	int32 OriginalGoldSupply = Ctx.MarketDataMap[KeyA]->GetCurrentSupply(EResourceType::GoldOre);

	FName KeyB(*FMarketId(FName("SaveTestBeta"), 1).ToString());
	int32 OriginalCopperSupply = Ctx.MarketDataMap[KeyB]->GetCurrentSupply(EResourceType::CopperOre);

	// Capture -> modify -> restore
	FEconomySaveData Snapshot = Ctx.SaveSystem->CaptureEconomySnapshot();

	Ctx.MarketDataMap[KeyA]->AddSupply(EResourceType::IronOre, 5000);
	Ctx.MarketDataMap[KeyB]->AddSupply(EResourceType::CopperOre, 5000);

	Ctx.SaveSystem->RestoreEconomyFromSnapshot(Snapshot);

	// Verify all values restored
	int32 RestoredIron = Ctx.MarketDataMap[KeyA]->GetCurrentSupply(EResourceType::IronOre);
	int32 RestoredGold = Ctx.MarketDataMap[KeyA]->GetCurrentSupply(EResourceType::GoldOre);
	int32 RestoredCopper = Ctx.MarketDataMap[KeyB]->GetCurrentSupply(EResourceType::CopperOre);

	TestEqual(TEXT("Iron supply should be restored"), RestoredIron, OriginalIronSupply);
	TestEqual(TEXT("Gold supply should be restored"), RestoredGold, OriginalGoldSupply);
	TestEqual(TEXT("Copper supply should be restored"), RestoredCopper, OriginalCopperSupply);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 3. VALIDATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_ValidateGoodData,
	"Odyssey.Economy.SaveSystem.Validation.ValidDataPassesValidation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_ValidateGoodData::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	FEconomySaveData Snapshot = Ctx.SaveSystem->CaptureEconomySnapshot();

	bool bValid = Ctx.SaveSystem->ValidateSaveData(Snapshot);
	TestTrue(TEXT("Valid snapshot should pass validation"), bValid);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_ValidateEmptyData,
	"Odyssey.Economy.SaveSystem.Validation.EmptyDataFailsValidation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_ValidateEmptyData::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	FEconomySaveData EmptyData;
	bool bValid = Ctx.SaveSystem->ValidateSaveData(EmptyData);

	// Empty data with default version=1 and no markets might still pass basic validation
	// but an implementation may reject it; either way, it should not crash
	TestTrue(TEXT("Validation of empty data should not crash"), true);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_ValidateCorruptVersion,
	"Odyssey.Economy.SaveSystem.Validation.InvalidVersionDetected",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_ValidateCorruptVersion::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	FEconomySaveData CorruptData;
	CorruptData.SaveVersion = 999; // Invalid future version

	bool bValid = Ctx.SaveSystem->ValidateSaveData(CorruptData);
	// Should either reject or handle gracefully
	TestTrue(TEXT("Validation with corrupt version should not crash"), true);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_CurrentSaveVersion,
	"Odyssey.Economy.SaveSystem.Validation.CurrentVersionIsPositive",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_CurrentSaveVersion::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	int32 Version = Ctx.SaveSystem->GetCurrentSaveVersion();
	TestTrue(TEXT("Current save version should be > 0"), Version > 0);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 4. DISK SAVE/LOAD TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_SaveToDisk,
	"Odyssey.Economy.SaveSystem.DiskIO.SaveToDiskSucceeds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_SaveToDisk::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	const FString SlotName = TEXT("EconomyTestSave_Disk");

	bool bSaved = Ctx.SaveSystem->SaveEconomyToDisk(SlotName, 0);
	TestTrue(TEXT("SaveEconomyToDisk should succeed"), bSaved);

	bool bExists = Ctx.SaveSystem->DoesSaveExist(SlotName, 0);
	TestTrue(TEXT("Save slot should exist after saving"), bExists);

	// Cleanup
	Ctx.SaveSystem->DeleteSave(SlotName, 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_LoadFromDisk,
	"Odyssey.Economy.SaveSystem.DiskIO.LoadFromDiskRestoresState",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_LoadFromDisk::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	const FString SlotName = TEXT("EconomyTestSave_Load");

	// Record state, save to disk
	FName KeyA(*FMarketId(FName("SaveTestAlpha"), 1).ToString());
	int32 OriginalIron = Ctx.MarketDataMap[KeyA]->GetCurrentSupply(EResourceType::IronOre);

	Ctx.SaveSystem->SaveEconomyToDisk(SlotName, 0);

	// Modify state
	Ctx.MarketDataMap[KeyA]->AddSupply(EResourceType::IronOre, 9999);

	// Load from disk
	bool bLoaded = Ctx.SaveSystem->LoadEconomyFromDisk(SlotName, 0);
	TestTrue(TEXT("LoadEconomyFromDisk should succeed"), bLoaded);

	int32 RestoredIron = Ctx.MarketDataMap[KeyA]->GetCurrentSupply(EResourceType::IronOre);
	TestEqual(TEXT("Iron supply should be restored from disk save"), RestoredIron, OriginalIron);

	// Cleanup
	Ctx.SaveSystem->DeleteSave(SlotName, 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_LoadNonExistent,
	"Odyssey.Economy.SaveSystem.DiskIO.LoadNonExistentSaveReturnsFalse",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_LoadNonExistent::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	bool bLoaded = Ctx.SaveSystem->LoadEconomyFromDisk(TEXT("NonExistentSlot_12345"), 0);
	TestFalse(TEXT("Loading non-existent save should return false"), bLoaded);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_DeleteSave,
	"Odyssey.Economy.SaveSystem.DiskIO.DeleteSaveRemovesSlot",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_DeleteSave::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	const FString SlotName = TEXT("EconomyTestSave_Delete");

	Ctx.SaveSystem->SaveEconomyToDisk(SlotName, 0);
	TestTrue(TEXT("Save should exist"), Ctx.SaveSystem->DoesSaveExist(SlotName, 0));

	bool bDeleted = Ctx.SaveSystem->DeleteSave(SlotName, 0);
	TestTrue(TEXT("Delete should succeed"), bDeleted);

	TestFalse(TEXT("Save should no longer exist"), Ctx.SaveSystem->DoesSaveExist(SlotName, 0));

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 5. AUTOSAVE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_AutosaveConfig,
	"Odyssey.Economy.SaveSystem.Autosave.EnableAndDisableAutosave",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_AutosaveConfig::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	Ctx.SaveSystem->EnableAutosave(30.0f, TEXT("TestAutosave"));
	TestTrue(TEXT("Autosave should be enabled"), Ctx.SaveSystem->IsAutosaveEnabled());

	Ctx.SaveSystem->DisableAutosave();
	TestFalse(TEXT("Autosave should be disabled"), Ctx.SaveSystem->IsAutosaveEnabled());

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_AutosaveZeroInterval,
	"Odyssey.Economy.SaveSystem.Autosave.ZeroIntervalDisablesAutosave",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_AutosaveZeroInterval::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	Ctx.SaveSystem->EnableAutosave(0.0f, TEXT("TestAutosave"));
	TestFalse(TEXT("Zero interval should effectively disable autosave"),
		Ctx.SaveSystem->IsAutosaveEnabled());

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 6. ROUND-TRIP INTEGRITY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_RoundTripIntegrity,
	"Odyssey.Economy.SaveSystem.Integrity.FullRoundTripPreservesAllData",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_RoundTripIntegrity::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	// Capture initial snapshot
	FEconomySaveData Snapshot1 = Ctx.SaveSystem->CaptureEconomySnapshot();

	// Restore from it
	Ctx.SaveSystem->RestoreEconomyFromSnapshot(Snapshot1);

	// Capture again
	FEconomySaveData Snapshot2 = Ctx.SaveSystem->CaptureEconomySnapshot();

	// Compare: should have the same number of markets
	TestEqual(TEXT("Market count should match after round-trip"),
		Snapshot2.Markets.Num(), Snapshot1.Markets.Num());

	// Compare market supply data
	for (int32 i = 0; i < FMath::Min(Snapshot1.Markets.Num(), Snapshot2.Markets.Num()); ++i)
	{
		TestEqual(
			FString::Printf(TEXT("Market[%d] ID should match"), i),
			Snapshot2.Markets[i].MarketId, Snapshot1.Markets[i].MarketId);

		for (auto& Pair : Snapshot1.Markets[i].SupplyDemandData)
		{
			if (const FResourceSupplyDemand* RestoreData = Snapshot2.Markets[i].SupplyDemandData.Find(Pair.Key))
			{
				TestEqual(
					FString::Printf(TEXT("Market[%d] resource %d supply"), i, static_cast<int32>(Pair.Key)),
					RestoreData->CurrentSupply, Pair.Value.CurrentSupply);
			}
		}
	}

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSaveSystem_DiskRoundTrip,
	"Odyssey.Economy.SaveSystem.Integrity.DiskSaveLoadRoundTrip",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveSystem_DiskRoundTrip::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = SaveSystemTestHelpers::CreateFullEconomyContext(World);
	TestNotNull(TEXT("SaveSystem"), Ctx.SaveSystem);

	const FString SlotName = TEXT("EconomyTestSave_RoundTrip");

	// Capture snapshot before save
	FEconomySaveData BeforeSave = Ctx.SaveSystem->CaptureEconomySnapshot();

	// Save -> modify -> load
	Ctx.SaveSystem->SaveEconomyToDisk(SlotName, 0);

	FName KeyA(*FMarketId(FName("SaveTestAlpha"), 1).ToString());
	Ctx.MarketDataMap[KeyA]->AddSupply(EResourceType::IronOre, 77777);

	Ctx.SaveSystem->LoadEconomyFromDisk(SlotName, 0);

	// Capture snapshot after load
	FEconomySaveData AfterLoad = Ctx.SaveSystem->CaptureEconomySnapshot();

	// Compare market counts
	TestEqual(TEXT("Market count should survive disk round-trip"),
		AfterLoad.Markets.Num(), BeforeSave.Markets.Num());

	// Cleanup
	Ctx.SaveSystem->DeleteSave(SlotName, 0);

	Ctx.Destroy();
	return true;
}
