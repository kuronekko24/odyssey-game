// ReputationSystemTests.cpp
// Automation tests for UReputationSystem
// Tests faction reputation, tiers, ripple effects, decay, titles, trust scores, NPC behavior

#include "Misc/AutomationTest.h"
#include "Social/ReputationSystem.h"

// ============================================================================
// TEST HELPERS
// ============================================================================

namespace ReputationTestHelpers
{
	UReputationSystem* CreateTestReputationSystem()
	{
		UReputationSystem* System = NewObject<UReputationSystem>();
		System->Initialize();
		return System;
	}

	void SetupTestPlayer(UReputationSystem* System, const FString& PlayerID = TEXT("TestPlayer"),
		const FString& PlayerName = TEXT("Tester"))
	{
		System->EnsurePlayerProfile(PlayerID, PlayerName);
	}
}

// ============================================================================
// FACTION REPUTATION CORE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_ModifyGain,
	"Odyssey.Social.Reputation.Faction.GainReputation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_ModifyGain::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	float InitialRep = System->GetReputation(TEXT("TestPlayer"), EFaction::VoidTraders);

	float NewRep = System->ModifyReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 100.0f,
		EReputationChangeSource::QuestCompletion, TEXT("Delivered cargo"));

	TestTrue(TEXT("Reputation should increase"), NewRep > InitialRep);
	TestEqual(TEXT("GetReputation should match returned value"),
		System->GetReputation(TEXT("TestPlayer"), EFaction::VoidTraders), NewRep);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_ModifyLoss,
	"Odyssey.Social.Reputation.Faction.LoseReputation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_ModifyLoss::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	float InitialRep = System->GetReputation(TEXT("TestPlayer"), EFaction::IronVanguard);

	System->ModifyReputation(TEXT("TestPlayer"), EFaction::IronVanguard, -200.0f,
		EReputationChangeSource::CrimeCommitted, TEXT("Attacked faction ship"));

	float AfterRep = System->GetReputation(TEXT("TestPlayer"), EFaction::IronVanguard);
	TestTrue(TEXT("Reputation should decrease"), AfterRep < InitialRep);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_ClampMinMax,
	"Odyssey.Social.Reputation.Faction.ClampMinMax",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_ClampMinMax::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Push to extreme positive
	System->ModifyReputation(TEXT("TestPlayer"), EFaction::Concordat, 5000.0f,
		EReputationChangeSource::AdminAction, TEXT("Admin boost"));

	float MaxRep = System->GetReputation(TEXT("TestPlayer"), EFaction::Concordat);
	TestTrue(TEXT("Reputation should be clamped to max (1000)"), MaxRep <= 1000.0f);

	// Push to extreme negative
	System->ModifyReputation(TEXT("TestPlayer"), EFaction::Concordat, -10000.0f,
		EReputationChangeSource::AdminAction, TEXT("Admin penalty"));

	float MinRep = System->GetReputation(TEXT("TestPlayer"), EFaction::Concordat);
	TestTrue(TEXT("Reputation should be clamped to min (-1000)"), MinRep >= -1000.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_SetExact,
	"Odyssey.Social.Reputation.Faction.SetExactValue",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_SetExact::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	System->SetReputation(TEXT("TestPlayer"), EFaction::StellarAcademy, 500.0f);

	float Rep = System->GetReputation(TEXT("TestPlayer"), EFaction::StellarAcademy);
	TestEqual(TEXT("Reputation should be exactly 500"), Rep, 500.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_LockedReputation,
	"Odyssey.Social.Reputation.Faction.LockedPreventChanges",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_LockedReputation::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	System->SetReputation(TEXT("TestPlayer"), EFaction::FreeHaven, 200.0f);
	System->SetReputationLocked(TEXT("TestPlayer"), EFaction::FreeHaven, true);

	System->ModifyReputation(TEXT("TestPlayer"), EFaction::FreeHaven, 100.0f,
		EReputationChangeSource::QuestCompletion, TEXT(""));

	float Rep = System->GetReputation(TEXT("TestPlayer"), EFaction::FreeHaven);
	TestEqual(TEXT("Locked reputation should not change"), Rep, 200.0f);

	// Unlock and verify changes work again
	System->SetReputationLocked(TEXT("TestPlayer"), EFaction::FreeHaven, false);
	System->ModifyReputation(TEXT("TestPlayer"), EFaction::FreeHaven, 100.0f,
		EReputationChangeSource::QuestCompletion, TEXT(""));

	Rep = System->GetReputation(TEXT("TestPlayer"), EFaction::FreeHaven);
	TestEqual(TEXT("Unlocked reputation should accept changes"), Rep, 300.0f);

	return true;
}

// ============================================================================
// TIER TRANSITION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_TierFromNeutral,
	"Odyssey.Social.Reputation.Tiers.NeutralToHigher",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_TierFromNeutral::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Default should be Neutral
	EReputationTier Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::VoidTraders);
	TestEqual(TEXT("Default tier should be Neutral"),
		static_cast<uint8>(Tier), static_cast<uint8>(EReputationTier::Neutral));

	// Push to Amiable (50+)
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 50.0f);
	Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::VoidTraders);
	TestEqual(TEXT("50 rep should be Amiable"),
		static_cast<uint8>(Tier), static_cast<uint8>(EReputationTier::Amiable));

	// Push to Friendly (250+)
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 250.0f);
	Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::VoidTraders);
	TestEqual(TEXT("250 rep should be Friendly"),
		static_cast<uint8>(Tier), static_cast<uint8>(EReputationTier::Friendly));

	// Push to Honored (500+)
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 500.0f);
	Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::VoidTraders);
	TestEqual(TEXT("500 rep should be Honored"),
		static_cast<uint8>(Tier), static_cast<uint8>(EReputationTier::Honored));

	// Push to Exalted (750+)
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 750.0f);
	Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::VoidTraders);
	TestEqual(TEXT("750 rep should be Exalted"),
		static_cast<uint8>(Tier), static_cast<uint8>(EReputationTier::Exalted));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_TierNegative,
	"Odyssey.Social.Reputation.Tiers.NegativeTiers",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_TierNegative::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Wary (-249 to -50)
	System->SetReputation(TEXT("TestPlayer"), EFaction::IronVanguard, -100.0f);
	EReputationTier Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::IronVanguard);
	TestEqual(TEXT("-100 should be Wary"),
		static_cast<uint8>(Tier), static_cast<uint8>(EReputationTier::Wary));

	// Unfriendly (-499 to -250)
	System->SetReputation(TEXT("TestPlayer"), EFaction::IronVanguard, -300.0f);
	Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::IronVanguard);
	TestEqual(TEXT("-300 should be Unfriendly"),
		static_cast<uint8>(Tier), static_cast<uint8>(EReputationTier::Unfriendly));

	// Hostile (-749 to -500)
	System->SetReputation(TEXT("TestPlayer"), EFaction::IronVanguard, -600.0f);
	Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::IronVanguard);
	TestEqual(TEXT("-600 should be Hostile"),
		static_cast<uint8>(Tier), static_cast<uint8>(EReputationTier::Hostile));

	// Reviled (-1000 to -750)
	System->SetReputation(TEXT("TestPlayer"), EFaction::IronVanguard, -800.0f);
	Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::IronVanguard);
	TestEqual(TEXT("-800 should be Reviled"),
		static_cast<uint8>(Tier), static_cast<uint8>(EReputationTier::Reviled));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_AllNineTiers,
	"Odyssey.Social.Reputation.Tiers.FullSpectrumReviledToExalted",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_AllNineTiers::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	struct FTierTest { float Rep; EReputationTier Expected; FString Name; };
	TArray<FTierTest> Tests = {
		{ -1000.0f, EReputationTier::Reviled,     TEXT("Reviled at -1000") },
		{ -800.0f,  EReputationTier::Reviled,     TEXT("Reviled at -800") },
		{ -749.0f,  EReputationTier::Hostile,      TEXT("Hostile at -749") },
		{ -600.0f,  EReputationTier::Hostile,      TEXT("Hostile at -600") },
		{ -499.0f,  EReputationTier::Unfriendly,   TEXT("Unfriendly at -499") },
		{ -300.0f,  EReputationTier::Unfriendly,   TEXT("Unfriendly at -300") },
		{ -249.0f,  EReputationTier::Wary,         TEXT("Wary at -249") },
		{ -100.0f,  EReputationTier::Wary,         TEXT("Wary at -100") },
		{ -49.0f,   EReputationTier::Neutral,      TEXT("Neutral at -49") },
		{   0.0f,   EReputationTier::Neutral,      TEXT("Neutral at 0") },
		{  49.0f,   EReputationTier::Neutral,      TEXT("Neutral at 49") },
		{  50.0f,   EReputationTier::Amiable,      TEXT("Amiable at 50") },
		{ 200.0f,   EReputationTier::Amiable,      TEXT("Amiable at 200") },
		{ 250.0f,   EReputationTier::Friendly,     TEXT("Friendly at 250") },
		{ 400.0f,   EReputationTier::Friendly,     TEXT("Friendly at 400") },
		{ 500.0f,   EReputationTier::Honored,      TEXT("Honored at 500") },
		{ 700.0f,   EReputationTier::Honored,      TEXT("Honored at 700") },
		{ 750.0f,   EReputationTier::Exalted,      TEXT("Exalted at 750") },
		{ 1000.0f,  EReputationTier::Exalted,      TEXT("Exalted at 1000") },
	};

	for (const FTierTest& T : Tests)
	{
		System->SetReputation(TEXT("TestPlayer"), EFaction::Concordat, T.Rep);
		EReputationTier Tier = System->GetReputationTier(TEXT("TestPlayer"), EFaction::Concordat);
		TestEqual(T.Name, static_cast<uint8>(Tier), static_cast<uint8>(T.Expected));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_RapidTierChange,
	"Odyssey.Social.Reputation.Tiers.RapidTierChange",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_RapidTierChange::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Jump from Reviled to Exalted in one action
	System->SetReputation(TEXT("TestPlayer"), EFaction::Concordat, -1000.0f);
	EReputationTier Before = System->GetReputationTier(TEXT("TestPlayer"), EFaction::Concordat);
	TestEqual(TEXT("Should start at Reviled"), static_cast<uint8>(Before), static_cast<uint8>(EReputationTier::Reviled));

	// Massive reputation swing
	System->ModifyReputation(TEXT("TestPlayer"), EFaction::Concordat, 2000.0f,
		EReputationChangeSource::AdminAction, TEXT("Massive boost"));

	float Rep = System->GetReputation(TEXT("TestPlayer"), EFaction::Concordat);
	EReputationTier After = System->GetReputationTier(TEXT("TestPlayer"), EFaction::Concordat);
	TestTrue(TEXT("Reputation should be clamped to 1000"), Rep <= 1000.0f);
	TestEqual(TEXT("Should be Exalted after massive gain"), static_cast<uint8>(After), static_cast<uint8>(EReputationTier::Exalted));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_TierProgress,
	"Odyssey.Social.Reputation.Tiers.ProgressWithinTier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_TierProgress::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// At midpoint of Amiable (50-250), 150 should be ~50% progress
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 150.0f);
	float Progress = System->GetTierProgress(TEXT("TestPlayer"), EFaction::VoidTraders);
	TestTrue(TEXT("Mid-tier progress should be ~0.5"), FMath::Abs(Progress - 0.5f) < 0.05f);

	// At start of tier
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 50.0f);
	Progress = System->GetTierProgress(TEXT("TestPlayer"), EFaction::VoidTraders);
	TestTrue(TEXT("Start of tier progress should be near 0"), Progress < 0.05f);

	return true;
}

// ============================================================================
// CROSS-FACTION RIPPLE EFFECT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_RippleEffect,
	"Odyssey.Social.Reputation.Faction.RippleEffects",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_RippleEffect::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Record initial values for all factions
	TMap<EFaction, float> InitialReps;
	TArray<FFactionDefinition> Factions = System->GetAllFactions();
	for (const FFactionDefinition& Def : Factions)
	{
		InitialReps.Add(Def.FactionID, System->GetReputation(TEXT("TestPlayer"), Def.FactionID));
	}

	// Gain reputation with Concordat -- this should ripple to other factions
	System->ModifyReputation(TEXT("TestPlayer"), EFaction::Concordat, 100.0f,
		EReputationChangeSource::QuestCompletion, TEXT("Major quest"));

	// Check that at least some other factions were affected
	bool bAnyRipple = false;
	for (const FFactionDefinition& Def : Factions)
	{
		if (Def.FactionID == EFaction::Concordat) continue;

		float RippleMultiplier = System->GetFactionRippleMultiplier(EFaction::Concordat, Def.FactionID);
		float CurrentRep = System->GetReputation(TEXT("TestPlayer"), Def.FactionID);
		float InitialRep = InitialReps[Def.FactionID];

		if (FMath::Abs(RippleMultiplier) > 0.01f)
		{
			float Diff = CurrentRep - InitialRep;
			if (RippleMultiplier > 0)
			{
				TestTrue(FString::Printf(TEXT("Positive ripple to %s should increase rep"), *System->GetFactionName(Def.FactionID)),
					Diff > 0);
			}
			else
			{
				TestTrue(FString::Printf(TEXT("Negative ripple to %s should decrease rep"), *System->GetFactionName(Def.FactionID)),
					Diff < 0);
			}
			bAnyRipple = true;
		}
	}

	TestTrue(TEXT("At least one cross-faction ripple effect should occur"), bAnyRipple);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_FactionRelationships,
	"Odyssey.Social.Reputation.Faction.AllFactionRelationshipsExist",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_FactionRelationships::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();

	TArray<FFactionDefinition> Factions = System->GetAllFactions();
	TestTrue(TEXT("System should have multiple factions"), Factions.Num() >= 9);

	// Verify faction definitions have names
	for (const FFactionDefinition& Def : Factions)
	{
		FString Name = System->GetFactionName(Def.FactionID);
		TestFalse(FString::Printf(TEXT("Faction %d should have a name"), static_cast<uint8>(Def.FactionID)),
			Name.IsEmpty());
	}

	// Check known relationships
	bool bConcordatShadow = System->AreFactionEnemies(EFaction::Concordat, EFaction::ShadowSyndicate);
	TestTrue(TEXT("Concordat and ShadowSyndicate should be enemies"), bConcordatShadow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_RippleMultiplierSymmetry,
	"Odyssey.Social.Reputation.Faction.RippleMultipliers14Relationships",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_RippleMultiplierSymmetry::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();

	TArray<FFactionDefinition> Factions = System->GetAllFactions();

	// Count non-zero ripple relationships
	int32 RelationshipCount = 0;
	for (const FFactionDefinition& A : Factions)
	{
		for (const FFactionDefinition& B : Factions)
		{
			if (A.FactionID == B.FactionID) continue;

			float Ripple = System->GetFactionRippleMultiplier(A.FactionID, B.FactionID);
			if (FMath::Abs(Ripple) > 0.001f)
			{
				RelationshipCount++;
			}
		}
	}

	// There should be at least 14 faction relationships per the design
	TestTrue(FString::Printf(TEXT("Should have at least 14 faction relationships, found %d"), RelationshipCount),
		RelationshipCount >= 14);

	return true;
}

// ============================================================================
// TITLE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_TitleUnlock,
	"Odyssey.Social.Reputation.Titles.UnlockAtMilestones",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_TitleUnlock::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Start with no titles
	TArray<FName> InitialTitles = System->GetAvailableTitles(TEXT("TestPlayer"));

	// Gain Exalted with Concordat
	System->SetReputation(TEXT("TestPlayer"), EFaction::Concordat, 800.0f);
	// Force a check by modifying rep (the modify call should trigger title check)
	System->ModifyReputation(TEXT("TestPlayer"), EFaction::Concordat, 1.0f,
		EReputationChangeSource::Custom, TEXT(""));

	TArray<FName> AfterTitles = System->GetAvailableTitles(TEXT("TestPlayer"));
	TestTrue(TEXT("Should have unlocked at least one title after reaching Exalted"),
		AfterTitles.Num() > InitialTitles.Num());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_SetActiveTitle,
	"Odyssey.Social.Reputation.Titles.SetActiveTitle",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_SetActiveTitle::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Get Exalted to unlock titles
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 800.0f);
	System->ModifyReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 1.0f,
		EReputationChangeSource::Custom, TEXT(""));

	TArray<FName> Available = System->GetAvailableTitles(TEXT("TestPlayer"));
	if (Available.Num() > 0)
	{
		bool bSet = System->SetActiveTitle(TEXT("TestPlayer"), Available[0]);
		TestTrue(TEXT("Setting available title should succeed"), bSet);

		FName Active = System->GetActiveTitle(TEXT("TestPlayer"));
		TestEqual(TEXT("Active title should match set title"), Active, Available[0]);
	}

	// Try to set a title the player has not unlocked
	bool bInvalid = System->SetActiveTitle(TEXT("TestPlayer"), FName(TEXT("NonExistentTitle_XYZ")));
	TestFalse(TEXT("Setting unavailable title should fail"), bInvalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_MultiFactionTitles,
	"Odyssey.Social.Reputation.Titles.MultipleFactionTitleUnlocks",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_MultiFactionTitles::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Max out reputation with multiple factions
	TArray<EFaction> FactionsToMax = {
		EFaction::Concordat,
		EFaction::VoidTraders,
		EFaction::IronVanguard,
		EFaction::StellarAcademy
	};

	for (EFaction F : FactionsToMax)
	{
		System->SetReputation(TEXT("TestPlayer"), F, 1000.0f);
		System->ModifyReputation(TEXT("TestPlayer"), F, 1.0f, EReputationChangeSource::Custom, TEXT(""));
	}

	TArray<FName> Titles = System->GetAvailableTitles(TEXT("TestPlayer"));
	TestTrue(TEXT("Should have multiple titles from different factions"), Titles.Num() >= 4);

	return true;
}

// ============================================================================
// REPUTATION DECAY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_DecayTowardDefault,
	"Odyssey.Social.Reputation.Decay.DecaysTowardDefault",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_DecayTowardDefault::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Set high reputation
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 500.0f);
	float Before = System->GetReputation(TEXT("TestPlayer"), EFaction::VoidTraders);

	// Process decay for simulated time (large delta to see effect)
	System->ProcessReputationDecay(86400.0f); // 1 day

	float After = System->GetReputation(TEXT("TestPlayer"), EFaction::VoidTraders);
	TestTrue(TEXT("Positive reputation should decay toward default (decrease)"), After < Before);

	// Also test negative reputation decays upward
	System->SetReputation(TEXT("TestPlayer"), EFaction::IronVanguard, -500.0f);
	float NegBefore = System->GetReputation(TEXT("TestPlayer"), EFaction::IronVanguard);

	System->ProcessReputationDecay(86400.0f);

	float NegAfter = System->GetReputation(TEXT("TestPlayer"), EFaction::IronVanguard);
	TestTrue(TEXT("Negative reputation should decay toward default (increase)"), NegAfter > NegBefore);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_DecayMultiplier,
	"Odyssey.Social.Reputation.Decay.GlobalMultiplier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_DecayMultiplier::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	System->SetReputation(TEXT("TestPlayer"), EFaction::Concordat, 500.0f);

	// Process with normal multiplier
	System->SetDecayRateMultiplier(1.0f);
	System->ProcessReputationDecay(86400.0f);
	float AfterNormal = System->GetReputation(TEXT("TestPlayer"), EFaction::Concordat);

	// Reset and process with doubled multiplier
	System->SetReputation(TEXT("TestPlayer"), EFaction::Concordat, 500.0f);
	System->SetDecayRateMultiplier(2.0f);
	System->ProcessReputationDecay(86400.0f);
	float AfterDouble = System->GetReputation(TEXT("TestPlayer"), EFaction::Concordat);

	TestTrue(TEXT("Double decay rate should cause more decay"),
		(500.0f - AfterDouble) > (500.0f - AfterNormal));

	// Restore default
	System->SetDecayRateMultiplier(1.0f);

	return true;
}

// ============================================================================
// NPC BEHAVIOR MODIFIER TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_NPCAttackOnSight,
	"Odyssey.Social.Reputation.NPC.AttackOnSight",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_NPCAttackOnSight::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Neutral -- should NOT attack
	TestFalse(TEXT("NPC should not attack neutral player"),
		System->ShouldNPCAttackPlayer(TEXT("TestPlayer"), EFaction::IronVanguard));

	// Hostile -- should attack
	System->SetReputation(TEXT("TestPlayer"), EFaction::IronVanguard, -600.0f);
	TestTrue(TEXT("NPC should attack hostile player"),
		System->ShouldNPCAttackPlayer(TEXT("TestPlayer"), EFaction::IronVanguard));

	// Reviled -- definitely attack
	System->SetReputation(TEXT("TestPlayer"), EFaction::IronVanguard, -900.0f);
	TestTrue(TEXT("NPC should attack reviled player"),
		System->ShouldNPCAttackPlayer(TEXT("TestPlayer"), EFaction::IronVanguard));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_NPCRefuseService,
	"Odyssey.Social.Reputation.NPC.RefuseService",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_NPCRefuseService::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Friendly -- should not refuse
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 300.0f);
	TestFalse(TEXT("NPC should not refuse service to friendly player"),
		System->ShouldNPCRefuseService(TEXT("TestPlayer"), EFaction::VoidTraders));

	// Unfriendly -- should refuse
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, -400.0f);
	TestTrue(TEXT("NPC should refuse service to unfriendly player"),
		System->ShouldNPCRefuseService(TEXT("TestPlayer"), EFaction::VoidTraders));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_NPCDisposition,
	"Odyssey.Social.Reputation.NPC.DispositionModifier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_NPCDisposition::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Exalted should give positive modifier
	System->SetReputation(TEXT("TestPlayer"), EFaction::StellarAcademy, 800.0f);
	float ExaltedMod = System->GetNPCDispositionModifier(TEXT("TestPlayer"), EFaction::StellarAcademy);

	// Hostile should give negative modifier
	System->SetReputation(TEXT("TestPlayer"), EFaction::StellarAcademy, -600.0f);
	float HostileMod = System->GetNPCDispositionModifier(TEXT("TestPlayer"), EFaction::StellarAcademy);

	TestTrue(TEXT("Exalted disposition should be greater than hostile"),
		ExaltedMod > HostileMod);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_TradeModifier,
	"Odyssey.Social.Reputation.NPC.TradeModifier",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_TradeModifier::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	// Exalted should give discount (negative modifier)
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 800.0f);
	float DiscountMod = System->GetTradeModifier(TEXT("TestPlayer"), EFaction::VoidTraders);

	// Reviled should give markup (positive modifier)
	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, -900.0f);
	float MarkupMod = System->GetTradeModifier(TEXT("TestPlayer"), EFaction::VoidTraders);

	TestTrue(TEXT("Exalted trade modifier should be better than Reviled"), DiscountMod < MarkupMod);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_TierRequirement,
	"Odyssey.Social.Reputation.NPC.MeetsTierRequirement",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_TierRequirement::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	System->SetReputation(TEXT("TestPlayer"), EFaction::VoidTraders, 300.0f); // Friendly

	TestTrue(TEXT("Friendly player should meet Amiable requirement"),
		System->MeetsTierRequirement(TEXT("TestPlayer"), EFaction::VoidTraders, EReputationTier::Amiable));
	TestTrue(TEXT("Friendly player should meet Friendly requirement"),
		System->MeetsTierRequirement(TEXT("TestPlayer"), EFaction::VoidTraders, EReputationTier::Friendly));
	TestFalse(TEXT("Friendly player should NOT meet Honored requirement"),
		System->MeetsTierRequirement(TEXT("TestPlayer"), EFaction::VoidTraders, EReputationTier::Honored));

	return true;
}

// ============================================================================
// PLAYER-TO-PLAYER TRUST SCORE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_TrustScoreInitial,
	"Odyssey.Social.Reputation.Social.InitialTrustScore",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_TrustScoreInitial::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	float TrustScore = System->GetPlayerTrustScore(TEXT("TestPlayer"));
	TestTrue(TEXT("Initial trust score should be 50 (neutral)"),
		FMath::Abs(TrustScore - 50.0f) < 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_TrustScorePositiveFeedback,
	"Odyssey.Social.Reputation.Social.PositiveFeedbackIncreaseTrust",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_TrustScorePositiveFeedback::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System, TEXT("Target"), TEXT("TargetPlayer"));
	ReputationTestHelpers::SetupTestPlayer(System, TEXT("Reporter1"), TEXT("Reporter1"));
	ReputationTestHelpers::SetupTestPlayer(System, TEXT("Reporter2"), TEXT("Reporter2"));

	float Before = System->GetPlayerTrustScore(TEXT("Target"));

	System->SubmitPlayerFeedback(TEXT("Reporter1"), TEXT("Target"), EPlayerFeedbackType::Positive, TEXT("Good trade"));
	System->SubmitPlayerFeedback(TEXT("Reporter2"), TEXT("Target"), EPlayerFeedbackType::Positive, TEXT("Reliable"));

	float After = System->GetPlayerTrustScore(TEXT("Target"));
	TestTrue(TEXT("Trust score should increase with positive feedback"), After > Before);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_TrustScoreNegativeFeedback,
	"Odyssey.Social.Reputation.Social.NegativeFeedbackDecreaseTrust",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_TrustScoreNegativeFeedback::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System, TEXT("Target"), TEXT("TargetPlayer"));
	ReputationTestHelpers::SetupTestPlayer(System, TEXT("Reporter1"), TEXT("Reporter1"));

	float Before = System->GetPlayerTrustScore(TEXT("Target"));

	System->SubmitPlayerFeedback(TEXT("Reporter1"), TEXT("Target"), EPlayerFeedbackType::Negative, TEXT("Scammer"));

	float After = System->GetPlayerTrustScore(TEXT("Target"));
	TestTrue(TEXT("Trust score should decrease with negative feedback"), After < Before);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_ContractOutcome,
	"Odyssey.Social.Reputation.Social.ContractOutcomeAffectsTrust",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_ContractOutcome::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System, TEXT("Contractor"), TEXT("ContractorName"));

	float Before = System->GetPlayerTrustScore(TEXT("Contractor"));

	// Record several completed contracts
	for (int32 i = 0; i < 5; i++)
	{
		System->RecordContractOutcome(TEXT("Contractor"), true, 5.0f);
	}

	float AfterGood = System->GetPlayerTrustScore(TEXT("Contractor"));
	TestTrue(TEXT("Trust should increase with completed contracts"), AfterGood >= Before);

	// Record several failed contracts
	for (int32 i = 0; i < 10; i++)
	{
		System->RecordContractOutcome(TEXT("Contractor"), false, 1.0f);
	}

	float AfterBad = System->GetPlayerTrustScore(TEXT("Contractor"));
	TestTrue(TEXT("Trust should decrease with failed contracts"), AfterBad < AfterGood);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_SocialRepProfile,
	"Odyssey.Social.Reputation.Social.FullProfileRetrieval",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_SocialRepProfile::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System, TEXT("Target"), TEXT("TargetName"));
	ReputationTestHelpers::SetupTestPlayer(System, TEXT("Reporter"), TEXT("ReporterName"));

	System->RecordContractOutcome(TEXT("Target"), true, 4.5f);
	System->RecordTradeOutcome(TEXT("Target"));
	System->RecordGuildContribution(TEXT("Target"));
	System->SubmitPlayerFeedback(TEXT("Reporter"), TEXT("Target"), EPlayerFeedbackType::Positive, TEXT(""));

	FPlayerSocialReputation SocialRep;
	bool bGot = System->GetPlayerSocialReputation(TEXT("Target"), SocialRep);
	TestTrue(TEXT("Should retrieve social reputation"), bGot);
	TestEqual(TEXT("Contracts completed should be 1"), SocialRep.ContractsCompleted, 1);
	TestEqual(TEXT("Trades completed should be 1"), SocialRep.TradesCompleted, 1);
	TestEqual(TEXT("Guild contributions should be 1"), SocialRep.GuildContributions, 1);
	TestEqual(TEXT("Positive feedback count should be 1"), SocialRep.PositiveCount, 1);

	return true;
}

// ============================================================================
// REPUTATION HISTORY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_ChangeHistory,
	"Odyssey.Social.Reputation.History.RecordsChanges",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_ChangeHistory::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	System->ModifyReputation(TEXT("TestPlayer"), EFaction::Concordat, 50.0f,
		EReputationChangeSource::QuestCompletion, TEXT("Quest A"));
	System->ModifyReputation(TEXT("TestPlayer"), EFaction::VoidTraders, -30.0f,
		EReputationChangeSource::CombatKill, TEXT("Killed trader"));

	TArray<FReputationChangeRecord> History = System->GetReputationHistory(TEXT("TestPlayer"));
	TestTrue(TEXT("History should have at least 2 entries"), History.Num() >= 2);

	// Filtered history
	TArray<FReputationChangeRecord> ConcordatHistory = System->GetFactionHistory(TEXT("TestPlayer"), EFaction::Concordat);
	TestTrue(TEXT("Concordat history should have entries"), ConcordatHistory.Num() >= 1);

	return true;
}

// ============================================================================
// PROFILE MANAGEMENT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_ProfileManagement,
	"Odyssey.Social.Reputation.Profile.CreateAndRemove",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_ProfileManagement::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();

	TestFalse(TEXT("Player should not have profile initially"),
		System->HasPlayerProfile(TEXT("NewPlayer")));

	System->EnsurePlayerProfile(TEXT("NewPlayer"), TEXT("New"));
	TestTrue(TEXT("Player should have profile after creation"),
		System->HasPlayerProfile(TEXT("NewPlayer")));

	// EnsurePlayerProfile should be idempotent
	System->EnsurePlayerProfile(TEXT("NewPlayer"), TEXT("New"));
	TestTrue(TEXT("Profile should still exist after double-create"),
		System->HasPlayerProfile(TEXT("NewPlayer")));

	System->RemovePlayerProfile(TEXT("NewPlayer"));
	TestFalse(TEXT("Player should not have profile after removal"),
		System->HasPlayerProfile(TEXT("NewPlayer")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRep_AllStandings,
	"Odyssey.Social.Reputation.Profile.AllStandingsReturned",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FRep_AllStandings::RunTest(const FString& Parameters)
{
	UReputationSystem* System = ReputationTestHelpers::CreateTestReputationSystem();
	ReputationTestHelpers::SetupTestPlayer(System);

	TMap<EFaction, FFactionStanding> Standings = System->GetAllStandings(TEXT("TestPlayer"));
	TestTrue(TEXT("Should have standings for all factions"), Standings.Num() >= 9);

	return true;
}

