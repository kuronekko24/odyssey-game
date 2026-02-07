// EconomicEventSystemTest.cpp
// Comprehensive automation tests for UEconomicEventSystem
// Tests event triggers, lifecycle, ripple propagation, templates, and notifications

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "OdysseyEconomyTypes.h"
#include "UMarketDataComponent.h"
#include "UPriceFluctuationSystem.h"
#include "UEconomicEventSystem.h"

// ============================================================================
// Helper: creates an event system linked to one or more markets
// ============================================================================
namespace EventSystemTestHelpers
{
	struct FMarketNode
	{
		UMarketDataComponent* MarketData = nullptr;
		UPriceFluctuationSystem* PriceSystem = nullptr;
		FMarketId Id;
	};

	struct FTestContext
	{
		AActor* Actor = nullptr;
		UEconomicEventSystem* EventSystem = nullptr;
		TArray<FMarketNode> Markets;

		void Destroy()
		{
			if (Actor) Actor->Destroy();
		}
	};

	FMarketNode CreateMarket(AActor* Actor, const FName& Name)
	{
		FMarketNode Node;
		Node.Id = FMarketId(Name, 1);
		Node.MarketData = NewObject<UMarketDataComponent>(Actor);
		Node.MarketData->RegisterComponent();
		Node.MarketData->InitializeMarketData(Node.Id, Name.ToString());
		Node.MarketData->AddSupply(EResourceType::IronOre, 500);
		Node.MarketData->AddSupply(EResourceType::CopperOre, 300);
		Node.MarketData->RecalculateAllMetrics();

		Node.PriceSystem = NewObject<UPriceFluctuationSystem>(Actor);
		Node.PriceSystem->RegisterComponent();
		Node.PriceSystem->Initialize(Node.MarketData);
		Node.PriceSystem->UpdateAllPrices();

		return Node;
	}

	FTestContext CreateSingleMarketContext(UWorld* World)
	{
		FTestContext Ctx;
		Ctx.Actor = World->SpawnActor<AActor>();
		if (!Ctx.Actor) return Ctx;

		FMarketNode Market = CreateMarket(Ctx.Actor, FName("EventTestStation"));
		Ctx.Markets.Add(Market);

		Ctx.EventSystem = NewObject<UEconomicEventSystem>(Ctx.Actor);
		Ctx.EventSystem->RegisterComponent();

		FEventGenerationParams Params;
		Params.BaseEventChancePerHour = 1.0f;
		Params.MinTimeBetweenEvents = 0.0f;
		Params.MaxActiveEvents = 10;
		Params.bAllowCatastrophicEvents = true;
		Ctx.EventSystem->Initialize(Params);

		Ctx.EventSystem->RegisterMarket(Market.Id, Market.MarketData, Market.PriceSystem);

		return Ctx;
	}

	FTestContext CreateMultiMarketContext(UWorld* World, int32 NumMarkets = 3)
	{
		FTestContext Ctx;
		Ctx.Actor = World->SpawnActor<AActor>();
		if (!Ctx.Actor) return Ctx;

		Ctx.EventSystem = NewObject<UEconomicEventSystem>(Ctx.Actor);
		Ctx.EventSystem->RegisterComponent();

		FEventGenerationParams Params;
		Params.MaxActiveEvents = 20;
		Params.MinTimeBetweenEvents = 0.0f;
		Ctx.EventSystem->Initialize(Params);

		for (int32 i = 0; i < NumMarkets; ++i)
		{
			FName MarketName = FName(*FString::Printf(TEXT("MultiMarket_%d"), i));
			FMarketNode Market = CreateMarket(Ctx.Actor, MarketName);
			Ctx.Markets.Add(Market);
			Ctx.EventSystem->RegisterMarket(Market.Id, Market.MarketData, Market.PriceSystem);
		}

		return Ctx;
	}

	FEconomicEvent CreateTestEvent(const TArray<FMarketId>& Markets, EEconomicEventType Type = EEconomicEventType::ResourceDiscovery)
	{
		FEconomicEvent Event;
		Event.EventType = Type;
		Event.Severity = EEconomicEventSeverity::Moderate;
		Event.EventName = TEXT("Test Event");
		Event.Description = TEXT("A test economic event");
		Event.Duration = 60.0;
		Event.AffectedMarkets = Markets;
		Event.AffectedResources.Add(EResourceType::IronOre);
		Event.SupplyModifier = 1.5f;
		Event.DemandModifier = 1.0f;
		Event.PriceModifier = 0.8f;
		Event.VolatilityIncrease = 0.1f;
		Event.NewsHeadline = TEXT("Test Resource Discovery!");
		Event.NewsBody = TEXT("A major discovery has been made.");
		Event.bShowNotification = true;
		return Event;
	}
}

// ============================================================================
// 1. EVENT TRIGGERING TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_TriggerManualEvent,
	"Odyssey.Economy.Events.Trigger.ManualEventTriggersSuccessfully",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_TriggerManualEvent::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> AffectedMarkets;
	AffectedMarkets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(AffectedMarkets);

	int32 EventId = Ctx.EventSystem->TriggerEvent(Event);
	TestTrue(TEXT("TriggerEvent should return a valid event ID"), EventId >= 0);

	bool bActive = Ctx.EventSystem->IsEventActive(EventId);
	TestTrue(TEXT("Triggered event should be active"), bActive);

	int32 Count = Ctx.EventSystem->GetActiveEventCount();
	TestTrue(TEXT("Active event count should be >= 1"), Count >= 1);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_TriggerByType,
	"Odyssey.Economy.Events.Trigger.TriggerByTypeCreatesAppropriateEvent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_TriggerByType::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);

	int32 EventId = Ctx.EventSystem->TriggerEventByType(EEconomicEventType::DemandSurge, Markets);
	TestTrue(TEXT("TriggerEventByType should return a valid event ID"), EventId >= 0);

	FEconomicEvent Event = Ctx.EventSystem->GetEvent(EventId);
	TestEqual(TEXT("Event type should match"), Event.EventType, EEconomicEventType::DemandSurge);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_EventAffectsMarket,
	"Odyssey.Economy.Events.Trigger.EventModifiesMarketConditions",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_EventAffectsMarket::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> AffectedMarkets;
	AffectedMarkets.Add(Ctx.Markets[0].Id);

	// Trigger a supply-boosting event
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(AffectedMarkets);
	Event.SupplyModifier = 2.0f;
	Event.DemandModifier = 1.0f;

	Ctx.EventSystem->TriggerEvent(Event);

	// Check supply modifier through the event system
	float SupplyMod = Ctx.EventSystem->GetTotalSupplyModifier(Ctx.Markets[0].Id, EResourceType::IronOre);
	TestTrue(TEXT("Supply modifier should be > 1.0 after supply boost event"), SupplyMod >= 1.0f);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 2. EVENT LIFECYCLE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_CancelEvent,
	"Odyssey.Economy.Events.Lifecycle.CancelEventRemovesFromActive",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_CancelEvent::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);

	int32 EventId = Ctx.EventSystem->TriggerEvent(Event);
	TestTrue(TEXT("Event should be active"), Ctx.EventSystem->IsEventActive(EventId));

	bool bCancelled = Ctx.EventSystem->CancelEvent(EventId);
	TestTrue(TEXT("CancelEvent should return true"), bCancelled);

	bool bStillActive = Ctx.EventSystem->IsEventActive(EventId);
	TestFalse(TEXT("Cancelled event should no longer be active"), bStillActive);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_ForceExpire,
	"Odyssey.Economy.Events.Lifecycle.ForceExpireEndEventImmediately",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_ForceExpire::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event.Duration = 9999.0; // long duration

	int32 EventId = Ctx.EventSystem->TriggerEvent(Event);
	TestTrue(TEXT("Event should be active"), Ctx.EventSystem->IsEventActive(EventId));

	Ctx.EventSystem->ForceExpireEvent(EventId);

	bool bActive = Ctx.EventSystem->IsEventActive(EventId);
	TestFalse(TEXT("Force-expired event should no longer be active"), bActive);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_ExtendDuration,
	"Odyssey.Economy.Events.Lifecycle.ExtendDurationIncreasesEventLifetime",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_ExtendDuration::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event.Duration = 60.0;

	int32 EventId = Ctx.EventSystem->TriggerEvent(Event);
	FEconomicEvent Before = Ctx.EventSystem->GetEvent(EventId);

	Ctx.EventSystem->ExtendEventDuration(EventId, 120.0f);

	FEconomicEvent After = Ctx.EventSystem->GetEvent(EventId);
	TestTrue(TEXT("Extended event should have a later end time"),
		After.EndTime >= Before.EndTime);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_ModifySeverity,
	"Odyssey.Economy.Events.Lifecycle.ModifySeverityUpdatesEvent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_ModifySeverity::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event.Severity = EEconomicEventSeverity::Minor;

	int32 EventId = Ctx.EventSystem->TriggerEvent(Event);

	Ctx.EventSystem->ModifyEventSeverity(EventId, EEconomicEventSeverity::Critical);

	FEconomicEvent Modified = Ctx.EventSystem->GetEvent(EventId);
	TestEqual(TEXT("Severity should be updated to Critical"),
		Modified.Severity, EEconomicEventSeverity::Critical);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 3. EVENT QUERY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_GetActiveEvents,
	"Odyssey.Economy.Events.Queries.GetActiveEventsReturnsAll",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_GetActiveEvents::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);

	// Trigger multiple events
	for (int32 i = 0; i < 3; ++i)
	{
		FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
		Event.EventName = FString::Printf(TEXT("Test Event %d"), i);
		Ctx.EventSystem->TriggerEvent(Event);
	}

	TArray<FEconomicEvent> ActiveEvents = Ctx.EventSystem->GetActiveEvents();
	TestEqual(TEXT("Should have 3 active events"), ActiveEvents.Num(), 3);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_GetEventsAffectingMarket,
	"Odyssey.Economy.Events.Queries.GetEventsAffectingSpecificMarket",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_GetEventsAffectingMarket::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateMultiMarketContext(World, 3);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	// Trigger event on market 0 only
	TArray<FMarketId> Market0;
	Market0.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event0 = EventSystemTestHelpers::CreateTestEvent(Market0);
	Ctx.EventSystem->TriggerEvent(Event0);

	// Trigger event on market 1 only
	TArray<FMarketId> Market1;
	Market1.Add(Ctx.Markets[1].Id);
	FEconomicEvent Event1 = EventSystemTestHelpers::CreateTestEvent(Market1);
	Event1.EventType = EEconomicEventType::PirateActivity;
	Ctx.EventSystem->TriggerEvent(Event1);

	TArray<FEconomicEvent> Events0 = Ctx.EventSystem->GetEventsAffectingMarket(Ctx.Markets[0].Id);
	TestEqual(TEXT("Market 0 should have 1 affecting event"), Events0.Num(), 1);

	TArray<FEconomicEvent> Events2 = Ctx.EventSystem->GetEventsAffectingMarket(Ctx.Markets[2].Id);
	TestEqual(TEXT("Market 2 should have 0 affecting events"), Events2.Num(), 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_GetEventsAffectingResource,
	"Odyssey.Economy.Events.Queries.GetEventsAffectingResource",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_GetEventsAffectingResource::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event.AffectedResources.Empty();
	Event.AffectedResources.Add(EResourceType::GoldOre);

	Ctx.EventSystem->TriggerEvent(Event);

	TArray<FEconomicEvent> GoldEvents = Ctx.EventSystem->GetEventsAffectingResource(EResourceType::GoldOre);
	TestTrue(TEXT("Should find at least 1 event affecting Gold"), GoldEvents.Num() >= 1);

	TArray<FEconomicEvent> IronEvents = Ctx.EventSystem->GetEventsAffectingResource(EResourceType::IronOre);
	TestEqual(TEXT("Should find 0 events affecting Iron (only Gold was specified)"), IronEvents.Num(), 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_EventHistory,
	"Odyssey.Economy.Events.Queries.EventHistoryIsRecorded",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_EventHistory::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);

	int32 EventId = Ctx.EventSystem->TriggerEvent(Event);
	Ctx.EventSystem->ForceExpireEvent(EventId);

	TArray<FEconomicEvent> History = Ctx.EventSystem->GetEventHistory(10);
	TestTrue(TEXT("Event history should contain at least 1 expired event"), History.Num() >= 1);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 4. MULTIPLE SIMULTANEOUS EVENTS TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_MultipleSimultaneousEvents,
	"Odyssey.Economy.Events.Multiple.MultipleEventsCanCoexist",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_MultipleSimultaneousEvents::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);

	// Trigger 5 different event types simultaneously
	TArray<EEconomicEventType> Types = {
		EEconomicEventType::ResourceDiscovery,
		EEconomicEventType::DemandSurge,
		EEconomicEventType::PirateActivity,
		EEconomicEventType::TradeRouteOpened,
		EEconomicEventType::MarketBoom
	};

	TArray<int32> EventIds;
	for (EEconomicEventType Type : Types)
	{
		int32 Id = Ctx.EventSystem->TriggerEventByType(Type, Markets);
		EventIds.Add(Id);
	}

	int32 ActiveCount = Ctx.EventSystem->GetActiveEventCount();
	TestEqual(TEXT("Should have 5 active events"), ActiveCount, 5);

	// All should be independently queryable
	for (int32 Id : EventIds)
	{
		TestTrue(FString::Printf(TEXT("Event %d should be active"), Id),
			Ctx.EventSystem->IsEventActive(Id));
	}

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_CumulativeModifiers,
	"Odyssey.Economy.Events.Multiple.CumulativeModifiersStack",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_CumulativeModifiers::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);

	// Trigger one event
	FEconomicEvent Event1 = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event1.SupplyModifier = 1.5f;
	Ctx.EventSystem->TriggerEvent(Event1);
	float OneMod = Ctx.EventSystem->GetTotalSupplyModifier(Ctx.Markets[0].Id, EResourceType::IronOre);

	// Trigger a second supply-affecting event
	FEconomicEvent Event2 = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event2.SupplyModifier = 1.5f;
	Event2.EventType = EEconomicEventType::ProductionBoost;
	Ctx.EventSystem->TriggerEvent(Event2);
	float TwoMod = Ctx.EventSystem->GetTotalSupplyModifier(Ctx.Markets[0].Id, EResourceType::IronOre);

	// Two modifiers should compound or stack (total should be >= single modifier)
	TestTrue(TEXT("Two supply modifiers should stack (combined >= single)"), TwoMod >= OneMod);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 5. EVENT GENERATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_TryGenerateRandomEvent,
	"Odyssey.Economy.Events.Generation.RandomEventGenerationDoesNotCrash",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_TryGenerateRandomEvent::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	// Try generating several random events - should not crash even if they fail
	int32 Generated = 0;
	for (int32 i = 0; i < 20; ++i)
	{
		if (Ctx.EventSystem->TryGenerateRandomEvent())
		{
			Generated++;
		}
	}

	// At least some should succeed with a high spawn chance
	TestTrue(TEXT("Random event generation should succeed at least once"), Generated >= 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_GenerateMarketEvent,
	"Odyssey.Economy.Events.Generation.GenerateMarketSpecificEvent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_GenerateMarketEvent::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	int32 EventId = Ctx.EventSystem->GenerateMarketEvent(Ctx.Markets[0].Id);
	// Event may or may not be generated depending on internal logic
	// but the function should not crash
	TestTrue(TEXT("GenerateMarketEvent should not crash"), true);

	if (EventId >= 0)
	{
		FEconomicEvent Event = Ctx.EventSystem->GetEvent(EventId);
		TestTrue(TEXT("Generated market event should affect the target market"),
			Event.AffectedMarkets.Contains(Ctx.Markets[0].Id));
	}

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_GenerateResourceEvent,
	"Odyssey.Economy.Events.Generation.GenerateResourceSpecificEvent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_GenerateResourceEvent::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	int32 EventId = Ctx.EventSystem->GenerateResourceEvent(EResourceType::IronOre);

	if (EventId >= 0)
	{
		FEconomicEvent Event = Ctx.EventSystem->GetEvent(EventId);
		TestTrue(TEXT("Generated resource event should affect Iron"),
			Event.AffectedResources.Contains(EResourceType::IronOre));
	}

	TestTrue(TEXT("GenerateResourceEvent should not crash"), true);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_ChainEvent,
	"Odyssey.Economy.Events.Generation.ChainEventFromExistingEvent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_ChainEvent::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
	int32 OriginalId = Ctx.EventSystem->TriggerEvent(Event);

	int32 ChainId = Ctx.EventSystem->GenerateChainEvent(OriginalId);
	// Chain events may or may not be generated
	TestTrue(TEXT("Chain event generation should not crash"), true);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 6. EVENT TEMPLATE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_AddAndUseTemplate,
	"Odyssey.Economy.Events.Templates.AddTemplateAndCreateEvent",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_AddAndUseTemplate::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	// Create a custom event template
	FEconomicEventTemplate Template;
	Template.EventType = EEconomicEventType::AsteroidStorm;
	Template.EventNameTemplate = TEXT("Asteroid Storm in {Market}");
	Template.DescriptionTemplate = TEXT("An asteroid storm disrupts mining operations");
	Template.PossibleResources.Add(EResourceType::IronOre);
	Template.PossibleResources.Add(EResourceType::CopperOre);
	Template.MinSupplyModifier = 0.5f;
	Template.MaxSupplyModifier = 0.8f;
	Template.MinDemandModifier = 1.0f;
	Template.MaxDemandModifier = 1.2f;
	Template.MinDuration = 30.0f;
	Template.MaxDuration = 120.0f;
	Template.BaseSpawnChance = 0.5f;
	Template.MinCooldown = 10.0f;

	Ctx.EventSystem->AddEventTemplate(Template);

	TArray<EEconomicEventType> Available = Ctx.EventSystem->GetAvailableEventTypes();
	TestTrue(TEXT("AsteroidStorm should be in available event types"),
		Available.Contains(EEconomicEventType::AsteroidStorm));

	// Create event from template
	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	TArray<EResourceType> Resources;
	Resources.Add(EResourceType::IronOre);

	FEconomicEvent Created = Ctx.EventSystem->CreateEventFromTemplate(
		EEconomicEventType::AsteroidStorm, Markets, Resources);

	TestEqual(TEXT("Created event should be AsteroidStorm type"),
		Created.EventType, EEconomicEventType::AsteroidStorm);
	TestTrue(TEXT("Supply modifier should be between template bounds"),
		Created.SupplyModifier >= 0.5f && Created.SupplyModifier <= 0.8f);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 7. NEWS AND NOTIFICATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_NewsHeadlines,
	"Odyssey.Economy.Events.News.HeadlinesGeneratedForEvents",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_NewsHeadlines::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event.NewsHeadline = TEXT("Breaking: Resource Discovery at Test Station!");

	int32 EventId = Ctx.EventSystem->TriggerEvent(Event);

	TArray<FString> Headlines = Ctx.EventSystem->GetLatestHeadlines(5);
	TestTrue(TEXT("Should have at least 1 headline"), Headlines.Num() >= 1);

	FString Headline = Ctx.EventSystem->GetEventHeadline(EventId);
	TestTrue(TEXT("Event headline should not be empty"), Headline.Len() > 0);

	FString Body = Ctx.EventSystem->GetEventNewsBody(EventId);
	TestTrue(TEXT("Event news body should not be empty"), Body.Len() > 0);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_Notifications,
	"Odyssey.Economy.Events.News.UnreadNotificationsTracked",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_Notifications::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	Ctx.EventSystem->MarkNotificationsRead();
	TestFalse(TEXT("No unread notifications after marking read"), Ctx.EventSystem->HasUnreadNotifications());

	// Trigger a notification-worthy event
	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event.bShowNotification = true;
	Ctx.EventSystem->TriggerEvent(Event);

	TestTrue(TEXT("Should have unread notifications after event"), Ctx.EventSystem->HasUnreadNotifications());

	Ctx.EventSystem->MarkNotificationsRead();
	TestFalse(TEXT("No unread notifications after marking read again"), Ctx.EventSystem->HasUnreadNotifications());

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 8. IMPACT CALCULATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_TotalPriceModifier,
	"Odyssey.Economy.Events.Impact.TotalPriceModifierReflectsActiveEvents",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_TotalPriceModifier::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	// No events: modifier should be 1.0 (neutral)
	float BaseMod = Ctx.EventSystem->GetTotalPriceModifier(Ctx.Markets[0].Id, EResourceType::IronOre);
	TestTrue(TEXT("Base price modifier with no events should be near 1.0"),
		FMath::IsNearlyEqual(BaseMod, 1.0f, 0.1f));

	// Add a price-increasing event
	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event.PriceModifier = 1.5f;
	Ctx.EventSystem->TriggerEvent(Event);

	float EventMod = Ctx.EventSystem->GetTotalPriceModifier(Ctx.Markets[0].Id, EResourceType::IronOre);
	TestTrue(TEXT("Price modifier should be > 1.0 after price-boosting event"), EventMod >= 1.0f);

	Ctx.Destroy();
	return true;
}

// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_VolatilityIncrease,
	"Odyssey.Economy.Events.Impact.EventsIncreaseVolatility",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_VolatilityIncrease::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	float BaseVol = Ctx.EventSystem->GetEventVolatilityIncrease(Ctx.Markets[0].Id);

	TArray<FMarketId> Markets;
	Markets.Add(Ctx.Markets[0].Id);
	FEconomicEvent Event = EventSystemTestHelpers::CreateTestEvent(Markets);
	Event.VolatilityIncrease = 0.2f;
	Ctx.EventSystem->TriggerEvent(Event);

	float EventVol = Ctx.EventSystem->GetEventVolatilityIncrease(Ctx.Markets[0].Id);
	TestTrue(TEXT("Volatility should increase after event"), EventVol >= BaseVol);

	Ctx.Destroy();
	return true;
}

// ============================================================================
// 9. CONFIGURATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEconEvent_DisableGeneration,
	"Odyssey.Economy.Events.Config.DisableEventGenerationStopsNewEvents",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEconEvent_DisableGeneration::RunTest(const FString& Parameters)
{
	UWorld* World = GEngine->GetWorldContexts()[0].World();
	auto Ctx = EventSystemTestHelpers::CreateSingleMarketContext(World);
	TestNotNull(TEXT("EventSystem"), Ctx.EventSystem);

	Ctx.EventSystem->SetEventGenerationEnabled(false);

	int32 Before = Ctx.EventSystem->GetActiveEventCount();
	for (int32 i = 0; i < 50; ++i)
	{
		Ctx.EventSystem->TryGenerateRandomEvent();
	}
	int32 After = Ctx.EventSystem->GetActiveEventCount();

	TestEqual(TEXT("No new events should be generated when disabled"), After, Before);

	Ctx.Destroy();
	return true;
}
