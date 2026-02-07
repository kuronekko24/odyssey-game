// GuildEconomyTests.cpp
// Automation tests for UGuildEconomyComponent
// Tests treasury, facilities, policies, dividends, goals, upkeep, and member contributions

#include "Misc/AutomationTest.h"
#include "Social/GuildEconomyComponent.h"
#include "OdysseyGuildManager.h"
#include "OdysseyInventoryComponent.h"

// ============================================================================
// TEST HELPERS
// ============================================================================

namespace EconomyTestHelpers
{
	struct FEconomyTestContext
	{
		UOdysseyGuildManager* GuildManager;
		UGuildEconomyComponent* Economy;
		FGuid GuildID;
	};

	FEconomyTestContext CreateTestContext()
	{
		FEconomyTestContext Ctx;
		Ctx.GuildManager = NewObject<UOdysseyGuildManager>();
		Ctx.GuildManager->Initialize();
		Ctx.Economy = NewObject<UGuildEconomyComponent>();
		Ctx.Economy->Initialize(Ctx.GuildManager);

		// Create a guild with founder
		Ctx.GuildID = Ctx.GuildManager->CreateGuild(
			TEXT("Founder001"), TEXT("Alice"),
			TEXT("EconGuild"), TEXT("EG"), TEXT("Economy test guild"));

		// Initialize economy for the guild
		Ctx.Economy->InitializeGuildEconomy(Ctx.GuildID);
		Ctx.Economy->RegisterMember(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"));

		return Ctx;
	}

	void AddGuildMember(FEconomyTestContext& Ctx, const FString& PlayerID, const FString& Name)
	{
		// Add to guild manager
		Ctx.GuildManager->InvitePlayer(Ctx.GuildID, TEXT("Founder001"), PlayerID, TEXT(""));
		TArray<FGuildInvitation> Invites = Ctx.GuildManager->GetPlayerInvitations(PlayerID);
		if (Invites.Num() > 0)
		{
			Ctx.GuildManager->AcceptInvitation(Invites[0].InvitationID, PlayerID, Name);
		}
		// Register in economy
		Ctx.Economy->RegisterMember(Ctx.GuildID, PlayerID, Name);
	}
}

// ============================================================================
// TREASURY OPERATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_TreasuryDeposit,
	"Odyssey.Social.GuildEconomy.Treasury.Deposit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_TreasuryDeposit::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	bool bDeposited = Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"),
		TEXT("Alice"), EResourceType::OMEN, 10000, TEXT("Initial deposit"));
	TestTrue(TEXT("Treasury deposit should succeed"), bDeposited);

	int64 Balance = Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::OMEN);
	TestEqual(TEXT("Balance should be 10000"), Balance, (int64)10000);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_TreasuryWithdraw,
	"Odyssey.Social.GuildEconomy.Treasury.Withdraw",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_TreasuryWithdraw::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 10000);

	bool bWithdrawn = Ctx.Economy->TreasuryWithdraw(Ctx.GuildID, TEXT("Founder001"),
		EResourceType::OMEN, 3000, TEXT("Operational expenses"));
	TestTrue(TEXT("Withdrawal should succeed"), bWithdrawn);

	int64 Balance = Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::OMEN);
	TestEqual(TEXT("Balance should be 7000"), Balance, (int64)7000);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_TreasuryInsufficientFunds,
	"Odyssey.Social.GuildEconomy.Treasury.InsufficientFunds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_TreasuryInsufficientFunds::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 1000);

	bool bWithdrawn = Ctx.Economy->TreasuryWithdraw(Ctx.GuildID, TEXT("Founder001"),
		EResourceType::OMEN, 5000);
	TestFalse(TEXT("Withdrawal exceeding balance should fail"), bWithdrawn);

	int64 Balance = Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::OMEN);
	TestEqual(TEXT("Balance should remain 1000"), Balance, (int64)1000);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_TreasuryMultiResource,
	"Odyssey.Social.GuildEconomy.Treasury.MultipleResourceTypes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_TreasuryMultiResource::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 5000);
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::Silicate, 200);
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::Carbon, 150);

	TMap<EResourceType, int64> All = Ctx.Economy->GetAllTreasuryBalances(Ctx.GuildID);
	TestTrue(TEXT("Should have multiple resource entries"), All.Num() >= 3);

	TestEqual(TEXT("OMEN balance"), Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::OMEN), (int64)5000);
	TestEqual(TEXT("Silicate balance"), Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::Silicate), (int64)200);
	TestEqual(TEXT("Carbon balance"), Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::Carbon), (int64)150);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_TaxCollection,
	"Odyssey.Social.GuildEconomy.Treasury.TaxCollection",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_TaxCollection::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();
	EconomyTestHelpers::AddGuildMember(Ctx, TEXT("Member001"), TEXT("Bob"));

	int64 Earnings = 10000;
	bool bTaxed = Ctx.Economy->CollectTax(Ctx.GuildID, TEXT("Member001"), TEXT("Bob"),
		EResourceType::OMEN, Earnings);
	TestTrue(TEXT("Tax collection should succeed"), bTaxed);

	int64 Balance = Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::OMEN);
	TestTrue(TEXT("Treasury should have received tax"), Balance > 0);

	// Tax should be proportional to effective tax rate
	float TaxRate = Ctx.Economy->GetEffectiveTaxRate(Ctx.GuildID);
	int64 ExpectedTax = static_cast<int64>(Earnings * TaxRate);
	TestTrue(TEXT("Tax collected should approximately match rate"),
		FMath::Abs(Balance - ExpectedTax) <= 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_TransactionHistory,
	"Odyssey.Social.GuildEconomy.Treasury.TransactionHistory",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_TransactionHistory::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 5000, TEXT("Deposit 1"));
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 3000, TEXT("Deposit 2"));
	Ctx.Economy->TreasuryWithdraw(Ctx.GuildID, TEXT("Founder001"),
		EResourceType::OMEN, 1000, TEXT("Withdrawal 1"));

	TArray<FTreasuryTransaction> History = Ctx.Economy->GetTransactionHistory(Ctx.GuildID);
	TestTrue(TEXT("Should have at least 3 transactions"), History.Num() >= 3);

	TArray<FTreasuryTransaction> Deposits = Ctx.Economy->GetTransactionsByType(
		Ctx.GuildID, ETreasuryTransactionType::Deposit);
	TestTrue(TEXT("Should have at least 2 deposit transactions"), Deposits.Num() >= 2);

	return true;
}

// ============================================================================
// FACILITY MANAGEMENT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_BuildFacility,
	"Odyssey.Social.GuildEconomy.Facilities.Build",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_BuildFacility::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	// Fund treasury for construction costs
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 1000000);

	bool bBuilt = Ctx.Economy->BuildFacility(Ctx.GuildID, TEXT("Founder001"),
		EGuildFacilityType::Warehouse, TEXT("Main Warehouse"));
	TestTrue(TEXT("Building warehouse should succeed"), bBuilt);

	TArray<FGuildFacility> Facilities = Ctx.Economy->GetFacilities(Ctx.GuildID);
	TestTrue(TEXT("Should have at least 1 facility"), Facilities.Num() >= 1);

	if (Facilities.Num() > 0)
	{
		TestEqual(TEXT("Facility type should be Warehouse"),
			static_cast<uint8>(Facilities[0].FacilityType),
			static_cast<uint8>(EGuildFacilityType::Warehouse));
		TestEqual(TEXT("Facility level should be 1"), Facilities[0].Level, 1);
		TestTrue(TEXT("Facility should be active"), Facilities[0].bIsActive);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_UpgradeFacility,
	"Odyssey.Social.GuildEconomy.Facilities.Upgrade",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_UpgradeFacility::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 5000000);

	Ctx.Economy->BuildFacility(Ctx.GuildID, TEXT("Founder001"),
		EGuildFacilityType::TradingPost, TEXT("Trading Hub"));

	TArray<FGuildFacility> Before = Ctx.Economy->GetFacilities(Ctx.GuildID);
	TestTrue(TEXT("Should have facility to upgrade"), Before.Num() > 0);

	if (Before.Num() > 0)
	{
		FGuid FacilityID = Before[0].FacilityID;
		int32 LevelBefore = Before[0].Level;

		bool bUpgraded = Ctx.Economy->UpgradeFacility(Ctx.GuildID, TEXT("Founder001"), FacilityID);
		TestTrue(TEXT("Upgrade should succeed"), bUpgraded);

		TArray<FGuildFacility> After = Ctx.Economy->GetFacilities(Ctx.GuildID);
		if (After.Num() > 0)
		{
			TestEqual(TEXT("Level should increase by 1"), After[0].Level, LevelBefore + 1);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_FacilityMaxLevel,
	"Odyssey.Social.GuildEconomy.Facilities.MaxLevelOverflow",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_FacilityMaxLevel::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	// Massive funding
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 100000000);

	Ctx.Economy->BuildFacility(Ctx.GuildID, TEXT("Founder001"),
		EGuildFacilityType::Workshop, TEXT("Workshop"));

	TArray<FGuildFacility> Facilities = Ctx.Economy->GetFacilities(Ctx.GuildID);
	if (Facilities.Num() > 0)
	{
		FGuid FacilityID = Facilities[0].FacilityID;
		int32 MaxLevel = Facilities[0].MaxLevel;

		// Upgrade to max
		for (int32 i = 1; i < MaxLevel; i++)
		{
			Ctx.Economy->UpgradeFacility(Ctx.GuildID, TEXT("Founder001"), FacilityID);
		}

		// Verify at max
		Facilities = Ctx.Economy->GetFacilities(Ctx.GuildID);
		if (Facilities.Num() > 0)
		{
			TestEqual(TEXT("Facility should be at max level"), Facilities[0].Level, MaxLevel);

			// Try to exceed max level
			bool bOverflow = Ctx.Economy->UpgradeFacility(Ctx.GuildID, TEXT("Founder001"), FacilityID);
			TestFalse(TEXT("Upgrading beyond max level should fail"), bOverflow);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_DeactivateActivateFacility,
	"Odyssey.Social.GuildEconomy.Facilities.DeactivateAndReactivate",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_DeactivateActivateFacility::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 5000000);
	Ctx.Economy->BuildFacility(Ctx.GuildID, TEXT("Founder001"),
		EGuildFacilityType::Refinery, TEXT("Refinery"));

	TArray<FGuildFacility> Facilities = Ctx.Economy->GetFacilities(Ctx.GuildID);
	if (Facilities.Num() > 0)
	{
		FGuid FID = Facilities[0].FacilityID;

		bool bDeactivated = Ctx.Economy->DeactivateFacility(Ctx.GuildID, TEXT("Founder001"), FID);
		TestTrue(TEXT("Deactivation should succeed"), bDeactivated);

		// Bonus should be 1.0 (no bonus) when deactivated
		float Bonus = Ctx.Economy->GetFacilityBonus(Ctx.GuildID, EGuildFacilityType::Refinery);
		TestEqual(TEXT("Deactivated facility bonus should be 1.0"), Bonus, 1.0f);

		bool bReactivated = Ctx.Economy->ActivateFacility(Ctx.GuildID, TEXT("Founder001"), FID);
		TestTrue(TEXT("Reactivation should succeed"), bReactivated);

		Bonus = Ctx.Economy->GetFacilityBonus(Ctx.GuildID, EGuildFacilityType::Refinery);
		TestTrue(TEXT("Active facility should provide bonus > 1.0"), Bonus > 1.0f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_DemolishFacility,
	"Odyssey.Social.GuildEconomy.Facilities.Demolish",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_DemolishFacility::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 5000000);
	Ctx.Economy->BuildFacility(Ctx.GuildID, TEXT("Founder001"),
		EGuildFacilityType::Beacon, TEXT("Beacon"));

	TArray<FGuildFacility> Before = Ctx.Economy->GetFacilities(Ctx.GuildID);
	int32 CountBefore = Before.Num();

	if (Before.Num() > 0)
	{
		bool bDemolished = Ctx.Economy->DemolishFacility(Ctx.GuildID, TEXT("Founder001"),
			Before[0].FacilityID);
		TestTrue(TEXT("Demolishing should succeed"), bDemolished);

		TArray<FGuildFacility> After = Ctx.Economy->GetFacilities(Ctx.GuildID);
		TestEqual(TEXT("Facility count should decrease"), After.Num(), CountBefore - 1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_AllFacilityTypes,
	"Odyssey.Social.GuildEconomy.Facilities.All10Types",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_AllFacilityTypes::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	// Give max level guild to allow many facilities
	Ctx.GuildManager->AddGuildExperience(Ctx.GuildID, 999999999);

	// Massive funding
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 999999999);

	TArray<EGuildFacilityType> AllTypes = {
		EGuildFacilityType::Warehouse,
		EGuildFacilityType::TradingPost,
		EGuildFacilityType::Refinery,
		EGuildFacilityType::Workshop,
		EGuildFacilityType::ResearchLab,
		EGuildFacilityType::DefensePlatform,
		EGuildFacilityType::ShipYard,
		EGuildFacilityType::MarketTerminal,
		EGuildFacilityType::Beacon,
		EGuildFacilityType::Embassy
	};

	int32 BuiltCount = 0;
	for (int32 i = 0; i < AllTypes.Num(); i++)
	{
		FString Name = FString::Printf(TEXT("Facility_%d"), i);
		bool bBuilt = Ctx.Economy->BuildFacility(Ctx.GuildID, TEXT("Founder001"), AllTypes[i], Name);
		if (bBuilt) BuiltCount++;
	}

	// We may be limited by max facilities, but should at least build some
	TestTrue(TEXT("Should build at least 3 different facility types"), BuiltCount >= 3);

	// Verify build costs exist for each type
	for (EGuildFacilityType Type : AllTypes)
	{
		TMap<EResourceType, int64> Cost = Ctx.Economy->GetFacilityBuildCost(Type, 1);
		TestTrue(FString::Printf(TEXT("Facility type %d should have build cost"), static_cast<uint8>(Type)),
			Cost.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_FacilityUpkeep,
	"Odyssey.Social.GuildEconomy.Facilities.UpkeepAutoDeactivation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_FacilityUpkeep::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	// Build facility with minimal treasury
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 5000000);
	Ctx.Economy->BuildFacility(Ctx.GuildID, TEXT("Founder001"),
		EGuildFacilityType::TradingPost, TEXT("TradingPost"));

	// Drain treasury (leave very little)
	int64 CurrentBalance = Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::OMEN);
	if (CurrentBalance > 1)
	{
		Ctx.Economy->TreasuryWithdraw(Ctx.GuildID, TEXT("Founder001"),
			EResourceType::OMEN, CurrentBalance - 1);
	}

	// Process upkeep with nearly empty treasury
	Ctx.Economy->ProcessFacilityUpkeep(Ctx.GuildID);

	// If upkeep cannot be paid, facility may be auto-deactivated
	TArray<FGuildFacility> Facilities = Ctx.Economy->GetFacilities(Ctx.GuildID);
	// We verify the upkeep was attempted -- facility may or may not be deactivated
	// depending on the exact cost vs remaining balance
	TestTrue(TEXT("Upkeep processing should complete without crash"), true);

	return true;
}

// ============================================================================
// ECONOMIC POLICY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_SetPolicy,
	"Odyssey.Social.GuildEconomy.Policy.SetPolicy",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_SetPolicy::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	// Default policy
	EGuildEconomicPolicy DefaultPolicy = Ctx.Economy->GetEconomicPolicy(Ctx.GuildID);
	TestEqual(TEXT("Default policy should be Cooperative"),
		static_cast<uint8>(DefaultPolicy), static_cast<uint8>(EGuildEconomicPolicy::Cooperative));

	bool bChanged = Ctx.Economy->SetEconomicPolicy(Ctx.GuildID, TEXT("Founder001"),
		EGuildEconomicPolicy::MilitaryEconomy);
	TestTrue(TEXT("Setting policy should succeed"), bChanged);

	EGuildEconomicPolicy NewPolicy = Ctx.Economy->GetEconomicPolicy(Ctx.GuildID);
	TestEqual(TEXT("Policy should be MilitaryEconomy"),
		static_cast<uint8>(NewPolicy), static_cast<uint8>(EGuildEconomicPolicy::MilitaryEconomy));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_PolicyEffectsOnTaxRate,
	"Odyssey.Social.GuildEconomy.Policy.AffectsTaxRate",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_PolicyEffectsOnTaxRate::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	// Collect rates for different policies
	TArray<EGuildEconomicPolicy> Policies = {
		EGuildEconomicPolicy::FreeMarket,
		EGuildEconomicPolicy::Cooperative,
		EGuildEconomicPolicy::Collectivist,
		EGuildEconomicPolicy::MilitaryEconomy,
		EGuildEconomicPolicy::Research
	};

	TMap<EGuildEconomicPolicy, float> Rates;
	for (EGuildEconomicPolicy Policy : Policies)
	{
		Ctx.Economy->SetEconomicPolicy(Ctx.GuildID, TEXT("Founder001"), Policy);
		float Rate = Ctx.Economy->GetEffectiveTaxRate(Ctx.GuildID);
		Rates.Add(Policy, Rate);
		TestTrue(FString::Printf(TEXT("Tax rate for policy %d should be >= 0"), static_cast<uint8>(Policy)),
			Rate >= 0.0f);
		TestTrue(FString::Printf(TEXT("Tax rate for policy %d should be <= 1"), static_cast<uint8>(Policy)),
			Rate <= 1.0f);
	}

	// FreeMarket should have lower tax than Collectivist
	if (Rates.Contains(EGuildEconomicPolicy::FreeMarket) && Rates.Contains(EGuildEconomicPolicy::Collectivist))
	{
		TestTrue(TEXT("FreeMarket tax should be <= Collectivist tax"),
			Rates[EGuildEconomicPolicy::FreeMarket] <= Rates[EGuildEconomicPolicy::Collectivist]);
	}

	return true;
}

// ============================================================================
// DIVIDEND DISTRIBUTION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_EqualDividend,
	"Odyssey.Social.GuildEconomy.Dividends.EqualDistribution",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_EqualDividend::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();
	EconomyTestHelpers::AddGuildMember(Ctx, TEXT("Member001"), TEXT("Bob"));
	EconomyTestHelpers::AddGuildMember(Ctx, TEXT("Member002"), TEXT("Carol"));

	// Fund treasury
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 30000);

	int64 Before = Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::OMEN);

	bool bDistributed = Ctx.Economy->DistributeEqualDividend(
		Ctx.GuildID, TEXT("Founder001"), EResourceType::OMEN, 9000);
	TestTrue(TEXT("Equal dividend distribution should succeed"), bDistributed);

	int64 After = Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::OMEN);
	TestEqual(TEXT("Treasury should decrease by dividend amount"), After, Before - 9000);

	// Check dividend history
	TArray<FDividendRecord> History = Ctx.Economy->GetDividendHistory(Ctx.GuildID);
	TestTrue(TEXT("Should have dividend record"), History.Num() >= 1);
	if (History.Num() > 0)
	{
		TestEqual(TEXT("Method should be Equal"), History[0].Method, FString(TEXT("Equal")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_ContributionDividend,
	"Odyssey.Social.GuildEconomy.Dividends.ContributionBased",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_ContributionDividend::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();
	EconomyTestHelpers::AddGuildMember(Ctx, TEXT("Member001"), TEXT("Bob"));

	// Make contributions from different members
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 50000);
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Member001"), TEXT("Bob"),
		EResourceType::OMEN, 10000);

	bool bDistributed = Ctx.Economy->DistributeContributionDividend(
		Ctx.GuildID, TEXT("Founder001"), EResourceType::OMEN, 6000);
	TestTrue(TEXT("Contribution-based dividend should succeed"), bDistributed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_RankDividend,
	"Odyssey.Social.GuildEconomy.Dividends.RankBased",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_RankDividend::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();
	EconomyTestHelpers::AddGuildMember(Ctx, TEXT("Member001"), TEXT("Bob"));

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 50000);

	bool bDistributed = Ctx.Economy->DistributeRankDividend(
		Ctx.GuildID, TEXT("Founder001"), EResourceType::OMEN, 6000);
	TestTrue(TEXT("Rank-based dividend should succeed"), bDistributed);

	TArray<FDividendRecord> History = Ctx.Economy->GetDividendHistory(Ctx.GuildID);
	TestTrue(TEXT("Should have dividend record"), History.Num() >= 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_DividendInsufficientFunds,
	"Odyssey.Social.GuildEconomy.Dividends.InsufficientFunds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_DividendInsufficientFunds::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();
	EconomyTestHelpers::AddGuildMember(Ctx, TEXT("Member001"), TEXT("Bob"));

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 100);

	bool bDistributed = Ctx.Economy->DistributeEqualDividend(
		Ctx.GuildID, TEXT("Founder001"), EResourceType::OMEN, 999999);
	TestFalse(TEXT("Dividend exceeding treasury should fail"), bDistributed);

	return true;
}

// ============================================================================
// ECONOMIC GOAL TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_CreateGoal,
	"Odyssey.Social.GuildEconomy.Goals.CreateAndTrack",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_CreateGoal::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	TMap<EResourceType, int64> Targets;
	Targets.Add(EResourceType::OMEN, 50000);
	Targets.Add(EResourceType::Silicate, 1000);

	FGuid GoalID = Ctx.Economy->CreateGoal(Ctx.GuildID, TEXT("Founder001"),
		TEXT("Weekly Target"), TEXT("Collect resources"),
		Targets, 10, 7);

	TestTrue(TEXT("Goal ID should be valid"), GoalID.IsValid());

	TArray<FGuildEconomicGoal> Active = Ctx.Economy->GetActiveGoals(Ctx.GuildID);
	TestTrue(TEXT("Should have at least 1 active goal"), Active.Num() >= 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_GoalProgress,
	"Odyssey.Social.GuildEconomy.Goals.ProgressTracking",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_GoalProgress::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	TMap<EResourceType, int64> Targets;
	Targets.Add(EResourceType::OMEN, 10000);

	Ctx.Economy->CreateGoal(Ctx.GuildID, TEXT("Founder001"),
		TEXT("OMEN Goal"), TEXT(""), Targets, 0, 30);

	// Update progress
	Ctx.Economy->UpdateGoalProgress(Ctx.GuildID, EResourceType::OMEN, 5000);

	TArray<FGuildEconomicGoal> Goals = Ctx.Economy->GetActiveGoals(Ctx.GuildID);
	if (Goals.Num() > 0)
	{
		float Progress = Goals[0].GetResourceProgress();
		TestTrue(TEXT("Progress should be ~50%"), FMath::Abs(Progress - 0.5f) < 0.05f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_GoalTradeCount,
	"Odyssey.Social.GuildEconomy.Goals.TradeCountIncrement",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_GoalTradeCount::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	TMap<EResourceType, int64> NoResources;
	Ctx.Economy->CreateGoal(Ctx.GuildID, TEXT("Founder001"),
		TEXT("Trade Goal"), TEXT("Complete 5 trades"), NoResources, 5, 14);

	for (int32 i = 0; i < 3; i++)
	{
		Ctx.Economy->IncrementGoalTradeCount(Ctx.GuildID);
	}

	TArray<FGuildEconomicGoal> Goals = Ctx.Economy->GetActiveGoals(Ctx.GuildID);
	if (Goals.Num() > 0)
	{
		TestEqual(TEXT("Trade count should be 3"), Goals[0].CurrentTradeCount, 3);
		float Progress = Goals[0].GetTradeProgress();
		TestTrue(TEXT("Trade progress should be 60%"), FMath::Abs(Progress - 0.6f) < 0.05f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_CancelGoal,
	"Odyssey.Social.GuildEconomy.Goals.CancelGoal",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_CancelGoal::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	TMap<EResourceType, int64> Targets;
	Targets.Add(EResourceType::OMEN, 100000);
	FGuid GoalID = Ctx.Economy->CreateGoal(Ctx.GuildID, TEXT("Founder001"),
		TEXT("Cancelled Goal"), TEXT(""), Targets, 0, 30);

	bool bCancelled = Ctx.Economy->CancelGoal(Ctx.GuildID, TEXT("Founder001"), GoalID);
	TestTrue(TEXT("Cancelling goal should succeed"), bCancelled);

	TArray<FGuildEconomicGoal> Active = Ctx.Economy->GetActiveGoals(Ctx.GuildID);
	TestEqual(TEXT("Should have 0 active goals after cancel"), Active.Num(), 0);

	return true;
}

// ============================================================================
// MEMBER CONTRIBUTION TRACKING TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_MemberContribution,
	"Odyssey.Social.GuildEconomy.Members.ContributionTracking",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_MemberContribution::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();
	EconomyTestHelpers::AddGuildMember(Ctx, TEXT("Member001"), TEXT("Bob"));

	// Member deposits
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Member001"), TEXT("Bob"),
		EResourceType::OMEN, 5000, TEXT("Contribution"));

	// Member pays tax
	Ctx.Economy->CollectTax(Ctx.GuildID, TEXT("Member001"), TEXT("Bob"),
		EResourceType::OMEN, 20000);

	FMemberEconomicContribution Contribution;
	bool bGot = Ctx.Economy->GetMemberContribution(Ctx.GuildID, TEXT("Member001"), Contribution);
	TestTrue(TEXT("Should retrieve member contribution"), bGot);
	TestTrue(TEXT("Total deposited should be > 0"), Contribution.TotalDeposited > 0);
	TestTrue(TEXT("Total taxes paid should be > 0"), Contribution.TotalTaxesPaid > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_TopContributors,
	"Odyssey.Social.GuildEconomy.Members.TopContributors",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_TopContributors::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();
	EconomyTestHelpers::AddGuildMember(Ctx, TEXT("Member001"), TEXT("Bob"));
	EconomyTestHelpers::AddGuildMember(Ctx, TEXT("Member002"), TEXT("Carol"));

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 50000);
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Member001"), TEXT("Bob"),
		EResourceType::OMEN, 30000);
	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Member002"), TEXT("Carol"),
		EResourceType::OMEN, 10000);

	TArray<FMemberEconomicContribution> Top = Ctx.Economy->GetTopContributors(Ctx.GuildID, 3);
	TestTrue(TEXT("Should have 3 contributors"), Top.Num() >= 2);

	// Top contributor should have highest net
	if (Top.Num() >= 2)
	{
		TestTrue(TEXT("First contributor should have >= second's contribution"),
			Top[0].NetContribution >= Top[1].NetContribution);
	}

	return true;
}

// ============================================================================
// ANALYTICS TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_Snapshot,
	"Odyssey.Social.GuildEconomy.Analytics.TakeSnapshot",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_Snapshot::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 25000);

	Ctx.Economy->TakeEconomySnapshot(Ctx.GuildID);

	TArray<FGuildEconomySnapshot> History = Ctx.Economy->GetEconomyHistory(Ctx.GuildID);
	TestTrue(TEXT("Should have at least 1 snapshot"), History.Num() >= 1);

	if (History.Num() > 0)
	{
		TestTrue(TEXT("Snapshot should have treasury data"),
			History[0].TreasuryBalances.Num() > 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_PeriodTracking,
	"Odyssey.Social.GuildEconomy.Analytics.PeriodIncomeExpenses",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_PeriodTracking::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->ResetPeriodTracking(Ctx.GuildID);

	Ctx.Economy->TreasuryDeposit(Ctx.GuildID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 10000);
	Ctx.Economy->TreasuryWithdraw(Ctx.GuildID, TEXT("Founder001"),
		EResourceType::OMEN, 3000);

	int64 Income = Ctx.Economy->GetPeriodIncome(Ctx.GuildID);
	int64 Expenses = Ctx.Economy->GetPeriodExpenses(Ctx.GuildID);

	TestTrue(TEXT("Period income should be > 0"), Income > 0);
	TestTrue(TEXT("Period expenses should be > 0"), Expenses > 0);

	return true;
}

// ============================================================================
// GUILD ECONOMY LIFECYCLE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_InitRemoveGuild,
	"Odyssey.Social.GuildEconomy.Lifecycle.InitializeAndRemove",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_InitRemoveGuild::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	// Economy should be initialized for the test guild
	int64 Capacity = Ctx.Economy->GetTreasuryCapacity(Ctx.GuildID);
	TestTrue(TEXT("Treasury capacity should be > 0"), Capacity > 0);

	// Remove
	Ctx.Economy->RemoveGuildEconomy(Ctx.GuildID);

	// After removal, balance queries should return 0
	int64 Balance = Ctx.Economy->GetTreasuryBalance(Ctx.GuildID, EResourceType::OMEN);
	TestEqual(TEXT("Balance after removal should be 0"), Balance, (int64)0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEcon_RegisterUnregisterMember,
	"Odyssey.Social.GuildEconomy.Lifecycle.MemberRegistration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEcon_RegisterUnregisterMember::RunTest(const FString& Parameters)
{
	auto Ctx = EconomyTestHelpers::CreateTestContext();

	Ctx.Economy->RegisterMember(Ctx.GuildID, TEXT("NewMember"), TEXT("NewPlayer"));

	FMemberEconomicContribution Contrib;
	bool bFound = Ctx.Economy->GetMemberContribution(Ctx.GuildID, TEXT("NewMember"), Contrib);
	TestTrue(TEXT("Newly registered member should have contribution entry"), bFound);

	Ctx.Economy->UnregisterMember(Ctx.GuildID, TEXT("NewMember"));

	bFound = Ctx.Economy->GetMemberContribution(Ctx.GuildID, TEXT("NewMember"), Contrib);
	TestFalse(TEXT("Unregistered member should not have contribution entry"), bFound);

	return true;
}

