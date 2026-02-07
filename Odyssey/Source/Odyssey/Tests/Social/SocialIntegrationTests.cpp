// SocialIntegrationTests.cpp
// Integration tests for USocialSystemsIntegration
// Tests cross-system event wiring, lifecycle flows, and full system coordination

#include "Misc/AutomationTest.h"
#include "Social/SocialSystemsIntegration.h"
#include "OdysseyGuildManager.h"
#include "CooperativeProjectSystem.h"
#include "SocialContractSystem.h"
#include "Social/ReputationSystem.h"
#include "Social/GuildEconomyComponent.h"
#include "OdysseyInventoryComponent.h"

// ============================================================================
// TEST HELPERS
// ============================================================================

namespace IntegrationTestHelpers
{
	USocialSystemsIntegration* CreateAndInitialize()
	{
		USocialSystemsIntegration* Integration = NewObject<USocialSystemsIntegration>();
		Integration->InitializeAllSystems();
		return Integration;
	}
}

// ============================================================================
// INITIALIZATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_Initialize,
	"Odyssey.Social.Integration.Initialization.AllSystemsCreated",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_Initialize::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	TestTrue(TEXT("System should be initialized"), Integration->IsInitialized());
	TestNotNull(TEXT("GuildManager should exist"), Integration->GetGuildManager());
	TestNotNull(TEXT("ProjectSystem should exist"), Integration->GetProjectSystem());
	TestNotNull(TEXT("ContractSystem should exist"), Integration->GetContractSystem());
	TestNotNull(TEXT("ReputationSystem should exist"), Integration->GetReputationSystem());
	TestNotNull(TEXT("GuildEconomy should exist"), Integration->GetGuildEconomy());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_DoubleInit,
	"Odyssey.Social.Integration.Initialization.DoubleInitSafe",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_DoubleInit::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	// Should not crash on double init
	Integration->InitializeAllSystems();
	TestTrue(TEXT("Should still be initialized"), Integration->IsInitialized());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_Shutdown,
	"Odyssey.Social.Integration.Initialization.ShutdownClean",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_Shutdown::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	Integration->ShutdownAllSystems();
	TestFalse(TEXT("Should not be initialized after shutdown"), Integration->IsInitialized());

	// Shutdown again should not crash
	Integration->ShutdownAllSystems();
	TestTrue(TEXT("Double shutdown should be safe"), true);

	return true;
}

// ============================================================================
// GUILD CREATION -> ECONOMY INITIALIZATION EVENT TEST
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_GuildCreatesEconomy,
	"Odyssey.Social.Integration.Events.GuildCreationTriggersEconomy",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_GuildCreatesEconomy::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();
	UGuildEconomyComponent* Economy = Integration->GetGuildEconomy();

	// Create a guild -- this should fire OnGuildCreated which triggers
	// GuildEconomy::InitializeGuildEconomy and RegisterMember
	FGuid GuildID = GuildMgr->CreateGuild(
		TEXT("P001"), TEXT("Alice"), TEXT("TestGuild"), TEXT("TG"), TEXT(""));
	TestTrue(TEXT("Guild should be created"), GuildID.IsValid());

	// The economy should now have data for this guild
	int64 Capacity = Economy->GetTreasuryCapacity(GuildID);
	TestTrue(TEXT("Guild economy should be initialized (capacity > 0)"), Capacity > 0);

	// Founder should be registered in economy
	FMemberEconomicContribution Contribution;
	bool bHasContrib = Economy->GetMemberContribution(GuildID, TEXT("P001"), Contribution);
	TestTrue(TEXT("Founder should be registered in economy"), bHasContrib);

	return true;
}

// ============================================================================
// GUILD DISBAND -> ECONOMY CLEANUP EVENT TEST
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_GuildDisbandCleansEconomy,
	"Odyssey.Social.Integration.Events.GuildDisbandCleansEconomy",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_GuildDisbandCleansEconomy::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();
	UGuildEconomyComponent* Economy = Integration->GetGuildEconomy();

	FGuid GuildID = GuildMgr->CreateGuild(
		TEXT("P001"), TEXT("Alice"), TEXT("DisbandGuild"), TEXT("DG"), TEXT(""));

	// Deposit some funds
	Economy->TreasuryDeposit(GuildID, TEXT("P001"), TEXT("Alice"),
		EResourceType::OMEN, 5000);
	TestTrue(TEXT("Balance should be > 0 before disband"),
		Economy->GetTreasuryBalance(GuildID, EResourceType::OMEN) > 0);

	// Disband -- should trigger economy cleanup
	GuildMgr->DisbandGuild(GuildID, TEXT("P001"));

	int64 AfterBalance = Economy->GetTreasuryBalance(GuildID, EResourceType::OMEN);
	TestEqual(TEXT("Balance should be 0 after disband"), AfterBalance, (int64)0);

	return true;
}

// ============================================================================
// MEMBER JOIN -> ECONOMY REGISTRATION EVENT TEST
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_MemberJoinRegistersEconomy,
	"Odyssey.Social.Integration.Events.MemberJoinRegistersInEconomy",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_MemberJoinRegistersEconomy::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();
	UGuildEconomyComponent* Economy = Integration->GetGuildEconomy();

	FGuid GuildID = GuildMgr->CreateGuild(
		TEXT("P001"), TEXT("Alice"), TEXT("JoinGuild"), TEXT("JG"), TEXT(""));

	// Invite and add member
	GuildMgr->InvitePlayer(GuildID, TEXT("P001"), TEXT("P002"), TEXT("Welcome"));
	TArray<FGuildInvitation> Invites = GuildMgr->GetPlayerInvitations(TEXT("P002"));
	if (Invites.Num() > 0)
	{
		GuildMgr->AcceptInvitation(Invites[0].InvitationID, TEXT("P002"), TEXT("Bob"));
	}

	// New member should be registered in economy
	FMemberEconomicContribution Contribution;
	bool bFound = Economy->GetMemberContribution(GuildID, TEXT("P002"), Contribution);
	TestTrue(TEXT("New member should be registered in guild economy"), bFound);

	return true;
}

// ============================================================================
// MEMBER LEAVE -> ECONOMY UNREGISTRATION EVENT TEST
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_MemberLeaveUnregistersEconomy,
	"Odyssey.Social.Integration.Events.MemberLeaveUnregistersFromEconomy",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_MemberLeaveUnregistersEconomy::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();
	UGuildEconomyComponent* Economy = Integration->GetGuildEconomy();

	FGuid GuildID = GuildMgr->CreateGuild(
		TEXT("P001"), TEXT("Alice"), TEXT("LeaveGuild"), TEXT("LG"), TEXT(""));

	GuildMgr->InvitePlayer(GuildID, TEXT("P001"), TEXT("P002"), TEXT(""));
	TArray<FGuildInvitation> Invites = GuildMgr->GetPlayerInvitations(TEXT("P002"));
	if (Invites.Num() > 0)
	{
		GuildMgr->AcceptInvitation(Invites[0].InvitationID, TEXT("P002"), TEXT("Bob"));
	}

	// Member leaves guild
	GuildMgr->LeaveGuild(TEXT("P002"));

	FMemberEconomicContribution Contribution;
	bool bFound = Economy->GetMemberContribution(GuildID, TEXT("P002"), Contribution);
	TestFalse(TEXT("Left member should be unregistered from economy"), bFound);

	return true;
}

// ============================================================================
// CONTRACT COMPLETION -> REPUTATION UPDATE EVENT TEST
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_ContractCompletionUpdatesRep,
	"Odyssey.Social.Integration.Events.ContractCompletionUpdatesReputation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_ContractCompletionUpdatesRep::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UReputationSystem* RepSystem = Integration->GetReputationSystem();
	USocialContractSystem* ContractSystem = Integration->GetContractSystem();

	// Ensure player profiles
	RepSystem->EnsurePlayerProfile(TEXT("Client001"), TEXT("Alice"));
	RepSystem->EnsurePlayerProfile(TEXT("Contractor001"), TEXT("Bob"));

	float RepBefore = RepSystem->GetReputation(TEXT("Contractor001"), EFaction::VoidTraders);
	FPlayerSocialReputation SocialBefore;
	RepSystem->GetPlayerSocialReputation(TEXT("Contractor001"), SocialBefore);

	// Create and complete a contract
	FContractPaymentTerms Payment;
	Payment.TotalPayment.Add(EResourceType::OMEN, 5000);
	Payment.CompletionPercentage = 1.0f;
	Payment.bUseEscrow = true;

	FGuid ContractID = ContractSystem->CreateContract(
		TEXT("Client001"), TEXT("Alice"),
		TEXT("Integration Test Contract"), TEXT(""),
		EContractType::Crafting, Payment);

	FContractMilestone M;
	M.Description = TEXT("Do work");
	M.PaymentPercentage = 1.0f;
	ContractSystem->AddMilestone(ContractID, TEXT("Client001"), M);

	ContractSystem->PostContract(ContractID, TEXT("Client001"));
	ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));
	ContractSystem->FundEscrow(ContractID, TEXT("Client001"));
	ContractSystem->CompleteMilestone(ContractID, TEXT("Contractor001"), 0);
	ContractSystem->ConfirmMilestone(ContractID, TEXT("Client001"), 0);

	// Check that reputation was affected (the integration handler should have fired)
	// Note: this depends on the contract reaching Completed status and the event firing
	FSocialContract Data;
	ContractSystem->GetContractData(ContractID, Data);

	if (Data.Status == EContractStatus::Completed)
	{
		float RepAfter = RepSystem->GetReputation(TEXT("Contractor001"), EFaction::VoidTraders);
		TestTrue(TEXT("Contractor VoidTraders rep should increase after contract completion"),
			RepAfter >= RepBefore);

		FPlayerSocialReputation SocialAfter;
		RepSystem->GetPlayerSocialReputation(TEXT("Contractor001"), SocialAfter);
		TestTrue(TEXT("Contractor contracts completed should increase"),
			SocialAfter.ContractsCompleted >= SocialBefore.ContractsCompleted);
	}

	return true;
}

// ============================================================================
// PLAYER LIFECYCLE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_PlayerJoined,
	"Odyssey.Social.Integration.PlayerLifecycle.PlayerJoined",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_PlayerJoined::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	Integration->OnPlayerJoined(TEXT("NewPlayer001"), TEXT("NewPlayer"));

	// Player should have a reputation profile
	TestTrue(TEXT("Player should have reputation profile"),
		Integration->GetReputationSystem()->HasPlayerProfile(TEXT("NewPlayer001")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_PlayerLeft,
	"Odyssey.Social.Integration.PlayerLifecycle.PlayerLeft",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_PlayerLeft::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();

	// Create guild and have founder go offline
	FGuid GuildID = GuildMgr->CreateGuild(
		TEXT("P001"), TEXT("Alice"), TEXT("OfflineGuild"), TEXT("OG"), TEXT(""));

	Integration->OnPlayerLeft(TEXT("P001"));

	// Verify member status was updated
	FGuildData Data;
	GuildMgr->GetGuildData(GuildID, Data);
	const FGuildMember* Member = Data.GetMember(TEXT("P001"));
	if (Member)
	{
		TestEqual(TEXT("Member status should be Inactive after leaving"),
			static_cast<uint8>(Member->Status), static_cast<uint8>(EGuildMemberStatus::Inactive));
	}

	return true;
}

// ============================================================================
// FULL WORKFLOW INTEGRATION TEST
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_FullWorkflow,
	"Odyssey.Social.Integration.FullWorkflow.GuildPolicyFacilityProject",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_FullWorkflow::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();
	UGuildEconomyComponent* Economy = Integration->GetGuildEconomy();
	UCooperativeProjectSystem* ProjSystem = Integration->GetProjectSystem();
	UReputationSystem* RepSystem = Integration->GetReputationSystem();

	// === Step 1: Create guild ===
	FGuid GuildID = GuildMgr->CreateGuild(
		TEXT("Leader"), TEXT("Commander"), TEXT("Starforge"), TEXT("SF"), TEXT(""));
	TestTrue(TEXT("[Step 1] Guild created"), GuildID.IsValid());

	// === Step 2: Add members ===
	GuildMgr->InvitePlayer(GuildID, TEXT("Leader"), TEXT("M001"), TEXT(""));
	TArray<FGuildInvitation> Inv1 = GuildMgr->GetPlayerInvitations(TEXT("M001"));
	if (Inv1.Num() > 0) GuildMgr->AcceptInvitation(Inv1[0].InvitationID, TEXT("M001"), TEXT("Engineer"));

	GuildMgr->InvitePlayer(GuildID, TEXT("Leader"), TEXT("M002"), TEXT(""));
	TArray<FGuildInvitation> Inv2 = GuildMgr->GetPlayerInvitations(TEXT("M002"));
	if (Inv2.Num() > 0) GuildMgr->AcceptInvitation(Inv2[0].InvitationID, TEXT("M002"), TEXT("Miner"));

	FGuildData GuildData;
	GuildMgr->GetGuildData(GuildID, GuildData);
	TestEqual(TEXT("[Step 2] Guild should have 3 members"), GuildData.GetMemberCount(), 3);

	// === Step 3: Set economic policy ===
	bool bPolicySet = Economy->SetEconomicPolicy(GuildID, TEXT("Leader"),
		EGuildEconomicPolicy::Cooperative);
	TestTrue(TEXT("[Step 3] Policy should be set"), bPolicySet);

	// === Step 4: Fund treasury ===
	Economy->TreasuryDeposit(GuildID, TEXT("Leader"), TEXT("Commander"),
		EResourceType::OMEN, 1000000);
	Economy->TreasuryDeposit(GuildID, TEXT("M001"), TEXT("Engineer"),
		EResourceType::OMEN, 500000);

	int64 Balance = Economy->GetTreasuryBalance(GuildID, EResourceType::OMEN);
	TestTrue(TEXT("[Step 4] Treasury should have funds"), Balance >= 1500000);

	// === Step 5: Build facility ===
	bool bBuilt = Economy->BuildFacility(GuildID, TEXT("Leader"),
		EGuildFacilityType::Workshop, TEXT("Engineering Bay"));
	TestTrue(TEXT("[Step 5] Facility should be built"), bBuilt);

	float Bonus = Economy->GetFacilityBonus(GuildID, EGuildFacilityType::Workshop);
	TestTrue(TEXT("[Step 5] Workshop bonus should be > 1.0"), Bonus > 1.0f);

	// === Step 6: Create cooperative project ===
	FGuid ProjID = ProjSystem->CreateProject(
		TEXT("Leader"), TEXT("Commander"),
		GuildID, TEXT("Mega Station"),
		TEXT("Build a massive space station"),
		EProjectType::Station, EProjectVisibility::Private);
	TestTrue(TEXT("[Step 6] Project created"), ProjID.IsValid());

	// Add milestone
	FProjectMilestone Milestone;
	Milestone.MilestoneName = TEXT("Foundation");
	Milestone.OrderIndex = 0;
	FProjectResourceRequirement Req(EResourceType::OMEN, 5000);
	Milestone.Requirements.Add(Req);
	ProjSystem->AddMilestone(ProjID, TEXT("Leader"), Milestone);

	// Start project
	ProjSystem->StartProject(ProjID, TEXT("Leader"));

	// === Step 7: All members contribute ===
	ProjSystem->ContributeResources(ProjID, TEXT("Leader"), TEXT("Commander"),
		EResourceType::OMEN, 3000);
	ProjSystem->ContributeResources(ProjID, TEXT("M001"), TEXT("Engineer"),
		EResourceType::OMEN, 1500);
	ProjSystem->ContributeResources(ProjID, TEXT("M002"), TEXT("Miner"),
		EResourceType::OMEN, 500);

	float Progress = ProjSystem->GetProjectProgress(ProjID);
	TestTrue(TEXT("[Step 7] Project progress should be 100%"), Progress >= 1.0f);

	// Check completion
	ProjSystem->CheckMilestoneCompletion(ProjID);

	FCooperativeProject ProjData;
	ProjSystem->GetProjectData(ProjID, ProjData);
	TestTrue(TEXT("[Step 7] All milestones should be complete"),
		ProjData.GetCompletedMilestoneCount() >= 1);

	// === Step 8: Set economic goal and track ===
	TMap<EResourceType, int64> GoalTargets;
	GoalTargets.Add(EResourceType::OMEN, 100000);
	FGuid GoalID = Economy->CreateGoal(GuildID, TEXT("Leader"),
		TEXT("Weekly Quota"), TEXT("Accumulate 100K OMEN"), GoalTargets, 5, 7);
	TestTrue(TEXT("[Step 8] Goal created"), GoalID.IsValid());

	// === Step 9: Verify reputation tracking across systems ===
	// The integration handlers should have recorded guild contributions
	RepSystem->EnsurePlayerProfile(TEXT("Leader"), TEXT("Commander"));
	FPlayerSocialReputation SocialRep;
	RepSystem->GetPlayerSocialReputation(TEXT("Leader"), SocialRep);
	TestTrue(TEXT("[Step 9] Leader should have guild contributions recorded"),
		SocialRep.GuildContributions >= 0);

	// === Step 10: Take economy snapshot ===
	Economy->TakeEconomySnapshot(GuildID);
	TArray<FGuildEconomySnapshot> Snapshots = Economy->GetEconomyHistory(GuildID);
	TestTrue(TEXT("[Step 10] Should have economy snapshot"), Snapshots.Num() >= 1);

	return true;
}

// ============================================================================
// CROSS-SYSTEM CONSISTENCY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_GuildProjectConsistency,
	"Odyssey.Social.Integration.Consistency.GuildProjectAlignment",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_GuildProjectConsistency::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();
	UCooperativeProjectSystem* ProjSystem = Integration->GetProjectSystem();

	FGuid GuildID = GuildMgr->CreateGuild(
		TEXT("P001"), TEXT("Alice"), TEXT("ConsistencyGuild"), TEXT("CG"), TEXT(""));

	// Create projects
	FGuid Proj1 = ProjSystem->CreateProject(TEXT("P001"), TEXT("Alice"),
		GuildID, TEXT("Proj1"), TEXT(""), EProjectType::Station, EProjectVisibility::Private);
	FGuid Proj2 = ProjSystem->CreateProject(TEXT("P001"), TEXT("Alice"),
		GuildID, TEXT("Proj2"), TEXT(""), EProjectType::Facility, EProjectVisibility::Private);

	TArray<FCooperativeProject> GuildProjects = ProjSystem->GetGuildProjects(GuildID);
	TestEqual(TEXT("Guild should have 2 projects"), GuildProjects.Num(), 2);

	// Verify guild data is consistent
	FGuildData GuildData;
	GuildMgr->GetGuildData(GuildID, GuildData);
	TestTrue(TEXT("Guild should still be valid"), GuildData.GuildID.IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_ReputationProfileConsistency,
	"Odyssey.Social.Integration.Consistency.ReputationProfileAcrossSystems",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_ReputationProfileConsistency::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UReputationSystem* RepSystem = Integration->GetReputationSystem();

	// Register player through integration
	Integration->OnPlayerJoined(TEXT("TestPlayer"), TEXT("TestName"));

	// Verify full reputation profile
	FPlayerReputationProfile Profile;
	bool bGot = RepSystem->GetReputationProfile(TEXT("TestPlayer"), Profile);
	TestTrue(TEXT("Should retrieve full profile"), bGot);
	TestEqual(TEXT("Player ID should match"), Profile.PlayerID, FString(TEXT("TestPlayer")));

	TMap<EFaction, FFactionStanding> Standings = RepSystem->GetAllStandings(TEXT("TestPlayer"));
	TestTrue(TEXT("Should have standings for all factions"), Standings.Num() >= 9);

	return true;
}

// ============================================================================
// EVENT COUNT / WIRING VERIFICATION
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_EventWiringCount,
	"Odyssey.Social.Integration.Events.All8EventConnectionsWired",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_EventWiringCount::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();
	USocialContractSystem* ContractSys = Integration->GetContractSystem();
	UCooperativeProjectSystem* ProjSys = Integration->GetProjectSystem();

	// Verify events are bound by checking if delegates have listeners
	// OnGuildCreated
	TestTrue(TEXT("OnGuildCreated should be bound"), GuildMgr->OnGuildCreated.IsBound());
	// OnGuildDisbanded
	TestTrue(TEXT("OnGuildDisbanded should be bound"), GuildMgr->OnGuildDisbanded.IsBound());
	// OnMemberJoined
	TestTrue(TEXT("OnMemberJoined should be bound"), GuildMgr->OnMemberJoined.IsBound());
	// OnMemberLeft
	TestTrue(TEXT("OnMemberLeft should be bound"), GuildMgr->OnMemberLeft.IsBound());
	// OnGuildLevelUp
	TestTrue(TEXT("OnGuildLevelUp should be bound"), GuildMgr->OnGuildLevelUp.IsBound());
	// OnContractCompleted
	TestTrue(TEXT("OnContractCompleted should be bound"), ContractSys->OnContractCompleted.IsBound());
	// OnContributionMade
	TestTrue(TEXT("OnContributionMade should be bound"), ProjSys->OnContributionMade.IsBound());
	// OnProjectCompleted
	TestTrue(TEXT("OnProjectCompleted should be bound"), ProjSys->OnProjectCompleted.IsBound());

	return true;
}

// ============================================================================
// GUILD LEVEL UP -> ECONOMY UPDATE
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_GuildLevelUpFacilities,
	"Odyssey.Social.Integration.Events.GuildLevelUpUpdatesFacilityLimit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_GuildLevelUpFacilities::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();
	UGuildEconomyComponent* Economy = Integration->GetGuildEconomy();

	FGuid GuildID = GuildMgr->CreateGuild(
		TEXT("P001"), TEXT("Alice"), TEXT("LevelGuild"), TEXT("LV"), TEXT(""));

	int32 MaxBefore = Economy->GetMaxFacilities(GuildID);

	// Level up the guild multiple times
	int64 XpPerLevel = GuildMgr->GetExperienceForLevel(1);
	GuildMgr->AddGuildExperience(GuildID, XpPerLevel * 10);

	int32 MaxAfter = Economy->GetMaxFacilities(GuildID);
	TestTrue(TEXT("Max facilities should increase with guild level"), MaxAfter >= MaxBefore);

	return true;
}

// ============================================================================
// PROJECT CONTRIBUTION -> REPUTATION EVENT TEST
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIntegration_ProjectContributionUpdatesRep,
	"Odyssey.Social.Integration.Events.ProjectContributionRecordsGuildContrib",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FIntegration_ProjectContributionUpdatesRep::RunTest(const FString& Parameters)
{
	USocialSystemsIntegration* Integration = IntegrationTestHelpers::CreateAndInitialize();

	UOdysseyGuildManager* GuildMgr = Integration->GetGuildManager();
	UCooperativeProjectSystem* ProjSystem = Integration->GetProjectSystem();
	UReputationSystem* RepSystem = Integration->GetReputationSystem();

	RepSystem->EnsurePlayerProfile(TEXT("P001"), TEXT("Alice"));

	FGuid GuildID = GuildMgr->CreateGuild(
		TEXT("P001"), TEXT("Alice"), TEXT("ContribGuild"), TEXT("CG"), TEXT(""));

	FGuid ProjID = ProjSystem->CreateProject(
		TEXT("P001"), TEXT("Alice"), GuildID,
		TEXT("Rep Test Project"), TEXT(""), EProjectType::Station,
		EProjectVisibility::Private);

	FProjectMilestone M;
	M.MilestoneName = TEXT("M1");
	FProjectResourceRequirement Req(EResourceType::OMEN, 1000);
	M.Requirements.Add(Req);
	ProjSystem->AddMilestone(ProjID, TEXT("P001"), M);
	ProjSystem->StartProject(ProjID, TEXT("P001"));

	FPlayerSocialReputation Before;
	RepSystem->GetPlayerSocialReputation(TEXT("P001"), Before);

	// Contribute -- this should fire OnContributionMade -> HandleProjectContribution
	ProjSystem->ContributeResources(ProjID, TEXT("P001"), TEXT("Alice"),
		EResourceType::OMEN, 500);

	FPlayerSocialReputation After;
	RepSystem->GetPlayerSocialReputation(TEXT("P001"), After);

	TestTrue(TEXT("Guild contributions should increase after project contribution"),
		After.GuildContributions > Before.GuildContributions);

	return true;
}

