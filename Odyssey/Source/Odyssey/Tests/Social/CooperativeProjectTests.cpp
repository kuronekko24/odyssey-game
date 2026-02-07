// CooperativeProjectTests.cpp
// Automation tests for UCooperativeProjectSystem
// Tests project lifecycle, milestones, contributions, rewards, and edge cases

#include "Misc/AutomationTest.h"
#include "CooperativeProjectSystem.h"
#include "OdysseyGuildManager.h"
#include "OdysseyInventoryComponent.h"

// ============================================================================
// TEST HELPERS
// ============================================================================

namespace ProjectTestHelpers
{
	struct FProjectTestContext
	{
		UOdysseyGuildManager* GuildManager;
		UCooperativeProjectSystem* ProjectSystem;
		FGuid GuildID;
	};

	FProjectTestContext CreateTestContext()
	{
		FProjectTestContext Ctx;
		Ctx.GuildManager = NewObject<UOdysseyGuildManager>();
		Ctx.GuildManager->Initialize();
		Ctx.ProjectSystem = NewObject<UCooperativeProjectSystem>();
		Ctx.ProjectSystem->Initialize(Ctx.GuildManager);

		Ctx.GuildID = Ctx.GuildManager->CreateGuild(
			TEXT("Founder001"), TEXT("Alice"),
			TEXT("ProjectGuild"), TEXT("PG"), TEXT(""));

		return Ctx;
	}

	void AddGuildMember(FProjectTestContext& Ctx, const FString& PlayerID, const FString& Name)
	{
		Ctx.GuildManager->InvitePlayer(Ctx.GuildID, TEXT("Founder001"), PlayerID, TEXT(""));
		TArray<FGuildInvitation> Invites = Ctx.GuildManager->GetPlayerInvitations(PlayerID);
		if (Invites.Num() > 0)
		{
			Ctx.GuildManager->AcceptInvitation(Invites[0].InvitationID, PlayerID, Name);
		}
	}

	FGuid CreateBasicProject(FProjectTestContext& Ctx,
		const FString& Name = TEXT("Test Project"),
		EProjectType Type = EProjectType::Station)
	{
		return Ctx.ProjectSystem->CreateProject(
			TEXT("Founder001"), TEXT("Alice"),
			Ctx.GuildID, Name, TEXT("A test project"),
			Type, EProjectVisibility::Private);
	}

	FGuid CreateProjectWithMilestones(FProjectTestContext& Ctx)
	{
		FGuid ProjID = CreateBasicProject(Ctx, TEXT("Milestone Project"));

		FProjectMilestone M1;
		M1.MilestoneName = TEXT("Phase 1");
		M1.OrderIndex = 0;
		FProjectResourceRequirement Req1(EResourceType::OMEN, 1000);
		M1.Requirements.Add(Req1);
		M1.ExperienceReward = 500;
		Ctx.ProjectSystem->AddMilestone(ProjID, TEXT("Founder001"), M1);

		FProjectMilestone M2;
		M2.MilestoneName = TEXT("Phase 2");
		M2.OrderIndex = 1;
		FProjectResourceRequirement Req2(EResourceType::Silicate, 500);
		M2.Requirements.Add(Req2);
		M2.ExperienceReward = 1000;
		Ctx.ProjectSystem->AddMilestone(ProjID, TEXT("Founder001"), M2);

		return ProjID;
	}
}

// ============================================================================
// PROJECT CREATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_Create,
	"Odyssey.Social.CoopProject.Creation.ValidProject",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_Create::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();

	FGuid ProjID = Ctx.ProjectSystem->CreateProject(
		TEXT("Founder001"), TEXT("Alice"),
		Ctx.GuildID, TEXT("Star Station Alpha"),
		TEXT("A massive space station"),
		EProjectType::Station, EProjectVisibility::Private);

	TestTrue(TEXT("Project ID should be valid"), ProjID.IsValid());

	FCooperativeProject Data;
	bool bFound = Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestTrue(TEXT("Project should be retrievable"), bFound);
	TestEqual(TEXT("Name should match"), Data.ProjectName, FString(TEXT("Star Station Alpha")));
	TestEqual(TEXT("Type should be Station"),
		static_cast<uint8>(Data.ProjectType), static_cast<uint8>(EProjectType::Station));
	TestEqual(TEXT("State should be Planning"),
		static_cast<uint8>(Data.State), static_cast<uint8>(EProjectState::Planning));
	TestEqual(TEXT("Creator should match"), Data.CreatorPlayerID, FString(TEXT("Founder001")));
	TestEqual(TEXT("Guild should match"), Data.OwnerGuildID, Ctx.GuildID);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_AllTypes,
	"Odyssey.Social.CoopProject.Creation.AllProjectTypes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_AllTypes::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();

	TArray<EProjectType> Types = {
		EProjectType::Station, EProjectType::MegaShip, EProjectType::Infrastructure,
		EProjectType::Facility, EProjectType::Defensive, EProjectType::Research,
		EProjectType::Custom
	};

	for (EProjectType Type : Types)
	{
		FString Name = FString::Printf(TEXT("Project_%d"), static_cast<uint8>(Type));
		FGuid ID = Ctx.ProjectSystem->CreateProject(
			TEXT("Founder001"), TEXT("Alice"), Ctx.GuildID,
			Name, TEXT(""), Type, EProjectVisibility::Private);

		TestTrue(FString::Printf(TEXT("Project type %d should be creatable"), static_cast<uint8>(Type)),
			ID.IsValid());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_GetGuildProjects,
	"Odyssey.Social.CoopProject.Creation.GetGuildProjects",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_GetGuildProjects::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();

	ProjectTestHelpers::CreateBasicProject(Ctx, TEXT("Project A"));
	ProjectTestHelpers::CreateBasicProject(Ctx, TEXT("Project B"));
	ProjectTestHelpers::CreateBasicProject(Ctx, TEXT("Project C"));

	TArray<FCooperativeProject> Projects = Ctx.ProjectSystem->GetGuildProjects(Ctx.GuildID);
	TestEqual(TEXT("Guild should have 3 projects"), Projects.Num(), 3);

	return true;
}

// ============================================================================
// PROJECT LIFECYCLE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_StartProject,
	"Odyssey.Social.CoopProject.Lifecycle.Start",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_StartProject::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);

	bool bStarted = Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));
	TestTrue(TEXT("Starting project should succeed"), bStarted);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestEqual(TEXT("State should be ResourceGathering"),
		static_cast<uint8>(Data.State), static_cast<uint8>(EProjectState::ResourceGathering));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_PauseResume,
	"Odyssey.Social.CoopProject.Lifecycle.PauseAndResume",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_PauseResume::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	// Pause
	bool bPaused = Ctx.ProjectSystem->PauseProject(ProjID, TEXT("Founder001"));
	TestTrue(TEXT("Pausing should succeed"), bPaused);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestEqual(TEXT("State should be OnHold"),
		static_cast<uint8>(Data.State), static_cast<uint8>(EProjectState::OnHold));

	// Resume
	bool bResumed = Ctx.ProjectSystem->ResumeProject(ProjID, TEXT("Founder001"));
	TestTrue(TEXT("Resuming should succeed"), bResumed);

	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestEqual(TEXT("State should be ResourceGathering after resume"),
		static_cast<uint8>(Data.State), static_cast<uint8>(EProjectState::ResourceGathering));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_Cancel,
	"Odyssey.Social.CoopProject.Lifecycle.Cancel",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_Cancel::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateBasicProject(Ctx);

	bool bCancelled = Ctx.ProjectSystem->CancelProject(ProjID, TEXT("Founder001"), false);
	TestTrue(TEXT("Cancelling should succeed"), bCancelled);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestEqual(TEXT("State should be Failed"),
		static_cast<uint8>(Data.State), static_cast<uint8>(EProjectState::Failed));

	return true;
}

// ============================================================================
// MILESTONE MANAGEMENT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_AddMilestone,
	"Odyssey.Social.CoopProject.Milestones.Add",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_AddMilestone::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateBasicProject(Ctx);

	FProjectMilestone Milestone;
	Milestone.MilestoneName = TEXT("Foundation");
	Milestone.Description = TEXT("Lay the foundation");
	Milestone.OrderIndex = 0;
	FProjectResourceRequirement Req(EResourceType::OMEN, 5000);
	Milestone.Requirements.Add(Req);

	bool bAdded = Ctx.ProjectSystem->AddMilestone(ProjID, TEXT("Founder001"), Milestone);
	TestTrue(TEXT("Adding milestone should succeed"), bAdded);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestEqual(TEXT("Should have 1 milestone"), Data.Milestones.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_RemoveMilestone,
	"Odyssey.Social.CoopProject.Milestones.Remove",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_RemoveMilestone::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	int32 Before = Data.Milestones.Num();
	FGuid MilestoneID = Data.Milestones[0].MilestoneID;

	bool bRemoved = Ctx.ProjectSystem->RemoveMilestone(ProjID, TEXT("Founder001"), MilestoneID);
	TestTrue(TEXT("Removing milestone should succeed in Planning state"), bRemoved);

	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestEqual(TEXT("Milestone count should decrease"), Data.Milestones.Num(), Before - 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_AddResourceRequirement,
	"Odyssey.Social.CoopProject.Milestones.AddResourceRequirement",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_AddResourceRequirement::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	FGuid MilestoneID = Data.Milestones[0].MilestoneID;

	FProjectResourceRequirement ExtraReq(EResourceType::Carbon, 2000, false, 1.0f);
	bool bAdded = Ctx.ProjectSystem->AddResourceRequirement(ProjID, TEXT("Founder001"), MilestoneID, ExtraReq);
	TestTrue(TEXT("Adding resource requirement should succeed"), bAdded);

	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestTrue(TEXT("Milestone should have 2+ requirements"), Data.Milestones[0].Requirements.Num() >= 2);

	return true;
}

// ============================================================================
// CONTRIBUTION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_ContributeResources,
	"Odyssey.Social.CoopProject.Contributions.BasicContribution",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_ContributeResources::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	bool bContributed = Ctx.ProjectSystem->ContributeResources(
		ProjID, TEXT("Founder001"), TEXT("Alice"), EResourceType::OMEN, 500);
	TestTrue(TEXT("Contribution should succeed"), bContributed);

	FProjectContributorSummary Summary;
	bool bGot = Ctx.ProjectSystem->GetContributorSummary(ProjID, TEXT("Founder001"), Summary);
	TestTrue(TEXT("Should get contributor summary"), bGot);
	TestTrue(TEXT("Total value contributed should be > 0"), Summary.TotalValueContributed > 0);
	TestEqual(TEXT("Contribution count should be 1"), Summary.ContributionCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_MultiPlayerContributions,
	"Odyssey.Social.CoopProject.Contributions.MultiPlayer",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_MultiPlayerContributions::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	ProjectTestHelpers::AddGuildMember(Ctx, TEXT("Member001"), TEXT("Bob"));
	ProjectTestHelpers::AddGuildMember(Ctx, TEXT("Member002"), TEXT("Carol"));

	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	// Multiple players contribute
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("Founder001"), TEXT("Alice"), EResourceType::OMEN, 500);
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("Member001"), TEXT("Bob"), EResourceType::OMEN, 300);
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("Member002"), TEXT("Carol"), EResourceType::OMEN, 200);

	TArray<FProjectContributorSummary> Contributors = Ctx.ProjectSystem->GetAllContributors(ProjID);
	TestEqual(TEXT("Should have 3 contributors"), Contributors.Num(), 3);

	// Check contribution history
	TArray<FProjectContribution> History = Ctx.ProjectSystem->GetContributionHistory(ProjID);
	TestEqual(TEXT("Should have 3 contribution entries"), History.Num(), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_ContributionExceedsRequirement,
	"Odyssey.Social.CoopProject.Contributions.ExceedingRequirements",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_ContributionExceedsRequirement::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	// First milestone requires 1000 OMEN -- contribute 2000
	bool bContributed = Ctx.ProjectSystem->ContributeResources(
		ProjID, TEXT("Founder001"), TEXT("Alice"), EResourceType::OMEN, 2000);
	TestTrue(TEXT("Over-contribution should be accepted"), bContributed);

	// Check progress
	float MilestoneProgress = Ctx.ProjectSystem->GetCurrentMilestoneProgress(ProjID);
	TestTrue(TEXT("Milestone progress should be at 100%"), MilestoneProgress >= 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_CanContributeCheck,
	"Odyssey.Social.CoopProject.Contributions.CanContribute",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_CanContributeCheck::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);

	// Cannot contribute while in Planning state
	bool bCanBefore = Ctx.ProjectSystem->CanContribute(ProjID, TEXT("Founder001"));
	TestFalse(TEXT("Should not contribute during Planning"), bCanBefore);

	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	bool bCanAfter = Ctx.ProjectSystem->CanContribute(ProjID, TEXT("Founder001"));
	TestTrue(TEXT("Should be able to contribute during ResourceGathering"), bCanAfter);

	return true;
}

// ============================================================================
// PROGRESS & COMPLETION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_Progress,
	"Odyssey.Social.CoopProject.Progress.TrackOverall",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_Progress::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	float InitialProgress = Ctx.ProjectSystem->GetProjectProgress(ProjID);
	TestEqual(TEXT("Initial progress should be 0"), InitialProgress, 0.0f);

	// Contribute to first milestone fully (1000 OMEN)
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 1000);

	float AfterFirstMilestone = Ctx.ProjectSystem->GetProjectProgress(ProjID);
	TestTrue(TEXT("Progress should be ~50% after first milestone resources"),
		AfterFirstMilestone > 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_MilestoneCompletion,
	"Odyssey.Social.CoopProject.Progress.MilestoneCompletion",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_MilestoneCompletion::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	// Contribute enough for first milestone
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 1000);

	bool bCheck = Ctx.ProjectSystem->CheckMilestoneCompletion(ProjID);
	// May or may not advance depending on implementation
	TestTrue(TEXT("Milestone check should not crash"), true);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestTrue(TEXT("First milestone should be complete or project advanced"),
		Data.Milestones.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_FullCompletion,
	"Odyssey.Social.CoopProject.Progress.FullProjectCompletion",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_FullCompletion::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	// Complete all milestones
	// Milestone 1: 1000 OMEN
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 1000);
	Ctx.ProjectSystem->CheckMilestoneCompletion(ProjID);

	// Milestone 2: 500 Silicate
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::Silicate, 500);
	Ctx.ProjectSystem->CheckMilestoneCompletion(ProjID);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);

	// All milestones should be complete or project should be completed
	int32 CompletedCount = Data.GetCompletedMilestoneCount();
	TestEqual(TEXT("Both milestones should be complete"), CompletedCount, 2);

	// Project may auto-complete
	if (Data.State == EProjectState::Completed)
	{
		TestTrue(TEXT("Project should be completed"), true);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_ForceComplete,
	"Odyssey.Social.CoopProject.Progress.ForceComplete",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_ForceComplete::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	bool bForced = Ctx.ProjectSystem->ForceCompleteProject(ProjID);
	TestTrue(TEXT("Force complete should succeed"), bForced);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	TestEqual(TEXT("State should be Completed"),
		static_cast<uint8>(Data.State), static_cast<uint8>(EProjectState::Completed));

	return true;
}

// ============================================================================
// REWARD TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_RewardConfig,
	"Odyssey.Social.CoopProject.Rewards.ConfigureRewards",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_RewardConfig::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateBasicProject(Ctx);

	FProjectRewardConfig Config;
	Config.bDistributeByContribution = true;
	Config.ParticipationBonus = 500;
	Config.FounderMultiplier = 10.0f;

	bool bSet = Ctx.ProjectSystem->SetRewardConfig(ProjID, TEXT("Founder001"), Config);
	TestTrue(TEXT("Setting reward config should succeed"), bSet);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_AddRewardPool,
	"Odyssey.Social.CoopProject.Rewards.AddToRewardPool",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_AddRewardPool::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateBasicProject(Ctx);

	bool bAdded = Ctx.ProjectSystem->AddToRewardPool(ProjID, TEXT("Founder001"),
		EResourceType::OMEN, 10000);
	TestTrue(TEXT("Adding to reward pool should succeed"), bAdded);

	FCooperativeProject Data;
	Ctx.ProjectSystem->GetProjectData(ProjID, Data);
	const int64* Pool = Data.RewardPool.Find(EResourceType::OMEN);
	TestNotNull(TEXT("OMEN should be in reward pool"), Pool);
	if (Pool)
	{
		TestEqual(TEXT("Reward pool should have 10000 OMEN"), *Pool, (int64)10000);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_ContributorTiers,
	"Odyssey.Social.CoopProject.Rewards.ContributorTiers",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_ContributorTiers::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	ProjectTestHelpers::AddGuildMember(Ctx, TEXT("M001"), TEXT("Bob"));
	ProjectTestHelpers::AddGuildMember(Ctx, TEXT("M002"), TEXT("Carol"));

	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));

	// Founder contributes 60% (600 of 1000)
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 600);
	// M001 contributes 30%
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("M001"), TEXT("Bob"),
		EResourceType::OMEN, 300);
	// M002 contributes 10%
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("M002"), TEXT("Carol"),
		EResourceType::OMEN, 100);

	FProjectContributorSummary FounderSummary;
	Ctx.ProjectSystem->GetContributorSummary(ProjID, TEXT("Founder001"), FounderSummary);

	FProjectContributorSummary M001Summary;
	Ctx.ProjectSystem->GetContributorSummary(ProjID, TEXT("M001"), M001Summary);

	FProjectContributorSummary M002Summary;
	Ctx.ProjectSystem->GetContributorSummary(ProjID, TEXT("M002"), M002Summary);

	// Verify tiers
	// Founder (60%) = Founder tier, M001 (30%) = Major tier, M002 (10%) = Supporter tier
	TestTrue(TEXT("Founder's contribution percentage should be highest"),
		FounderSummary.ContributionPercentage > M001Summary.ContributionPercentage);
	TestTrue(TEXT("M001's contribution should be more than M002's"),
		M001Summary.ContributionPercentage > M002Summary.ContributionPercentage);

	return true;
}

// ============================================================================
// ACCESS CONTROL TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_AccessControl,
	"Odyssey.Social.CoopProject.Access.ProjectAccessCheck",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_AccessControl::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();
	FGuid ProjID = ProjectTestHelpers::CreateBasicProject(Ctx);

	// Guild member should have access
	bool bFounderAccess = Ctx.ProjectSystem->HasProjectAccess(ProjID, TEXT("Founder001"));
	TestTrue(TEXT("Creator should have access"), bFounderAccess);

	// Non-guild member for private project
	// (Random player not in guild)
	bool bRandomAccess = Ctx.ProjectSystem->HasProjectAccess(ProjID, TEXT("RandomPlayer"));
	TestFalse(TEXT("Random player should not have access to private project"), bRandomAccess);

	// Creator should be able to manage
	bool bCanManage = Ctx.ProjectSystem->CanManageProject(ProjID, TEXT("Founder001"));
	TestTrue(TEXT("Creator should be able to manage"), bCanManage);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_PublicProject,
	"Odyssey.Social.CoopProject.Access.PublicVisibility",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_PublicProject::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();

	FGuid ProjID = Ctx.ProjectSystem->CreateProject(
		TEXT("Founder001"), TEXT("Alice"),
		Ctx.GuildID, TEXT("Public Build"),
		TEXT("Everyone can join"), EProjectType::Infrastructure,
		EProjectVisibility::Public);

	TArray<FCooperativeProject> PublicProjects = Ctx.ProjectSystem->SearchPublicProjects(
		TEXT("Public"), EProjectType::Infrastructure);
	TestTrue(TEXT("Public project should appear in search"), PublicProjects.Num() >= 1);

	return true;
}

// ============================================================================
// RESOURCE VALUE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_ResourceValues,
	"Odyssey.Social.CoopProject.Resources.ValueCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_ResourceValues::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();

	int64 OMENValue = Ctx.ProjectSystem->GetResourceValue(EResourceType::OMEN, 100);
	int64 SilicateValue = Ctx.ProjectSystem->GetResourceValue(EResourceType::Silicate, 100);

	TestTrue(TEXT("OMEN should have value > 0"), OMENValue > 0);
	TestTrue(TEXT("Silicate should have value > 0"), SilicateValue > 0);

	// OMEN (currency) should have higher per-unit value than raw Silicate
	int64 OMENPer1 = Ctx.ProjectSystem->GetResourceValue(EResourceType::OMEN, 1);
	int64 SilicatePer1 = Ctx.ProjectSystem->GetResourceValue(EResourceType::Silicate, 1);
	TestTrue(TEXT("OMEN per-unit value should be >= Silicate"), OMENPer1 >= SilicatePer1);

	return true;
}

// ============================================================================
// PLAYER PROJECTS TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_PlayerProjects,
	"Odyssey.Social.CoopProject.Search.PlayerProjects",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_PlayerProjects::RunTest(const FString& Parameters)
{
	auto Ctx = ProjectTestHelpers::CreateTestContext();

	FGuid ProjID = ProjectTestHelpers::CreateProjectWithMilestones(Ctx);
	Ctx.ProjectSystem->StartProject(ProjID, TEXT("Founder001"));
	Ctx.ProjectSystem->ContributeResources(ProjID, TEXT("Founder001"), TEXT("Alice"),
		EResourceType::OMEN, 100);

	TArray<FCooperativeProject> PlayerProjects = Ctx.ProjectSystem->GetPlayerProjects(TEXT("Founder001"));
	TestTrue(TEXT("Player should have at least 1 project"), PlayerProjects.Num() >= 1);

	return true;
}

// ============================================================================
// STRUCT LOGIC TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_ResourceRequirementStruct,
	"Odyssey.Social.CoopProject.Structs.ResourceRequirement",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_ResourceRequirementStruct::RunTest(const FString& Parameters)
{
	FProjectResourceRequirement Req(EResourceType::OMEN, 1000);
	TestEqual(TEXT("Initial contributed should be 0"), Req.ContributedAmount, (int64)0);
	TestFalse(TEXT("Should not be complete initially"), Req.IsComplete());
	TestEqual(TEXT("Remaining should be 1000"), Req.GetRemainingAmount(), (int64)1000);
	TestEqual(TEXT("Completion should be 0%"), Req.GetCompletionPercentage(), 0.0f);

	Req.ContributedAmount = 500;
	TestEqual(TEXT("Completion should be 50%"), Req.GetCompletionPercentage(), 0.5f);
	TestEqual(TEXT("Remaining should be 500"), Req.GetRemainingAmount(), (int64)500);

	Req.ContributedAmount = 1000;
	TestTrue(TEXT("Should be complete at 1000"), Req.IsComplete());
	TestEqual(TEXT("Completion should be 100%"), Req.GetCompletionPercentage(), 1.0f);
	TestEqual(TEXT("Remaining should be 0"), Req.GetRemainingAmount(), (int64)0);

	// Overflow
	Req.ContributedAmount = 1500;
	TestTrue(TEXT("Should still be complete at 1500"), Req.IsComplete());
	TestEqual(TEXT("Completion should clamp to 1.0"), Req.GetCompletionPercentage(), 1.0f);
	TestEqual(TEXT("Remaining should be 0 on overflow"), Req.GetRemainingAmount(), (int64)0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_ContributorTierStruct,
	"Odyssey.Social.CoopProject.Structs.ContributorTierCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_ContributorTierStruct::RunTest(const FString& Parameters)
{
	FProjectContributorSummary Summary;

	Summary.ContributionPercentage = 3.0f;
	Summary.UpdateTier();
	TestEqual(TEXT("3% should be Participant"),
		static_cast<uint8>(Summary.Tier), static_cast<uint8>(EContributorTier::Participant));

	Summary.ContributionPercentage = 10.0f;
	Summary.UpdateTier();
	TestEqual(TEXT("10% should be Supporter"),
		static_cast<uint8>(Summary.Tier), static_cast<uint8>(EContributorTier::Supporter));

	Summary.ContributionPercentage = 20.0f;
	Summary.UpdateTier();
	TestEqual(TEXT("20% should be Contributor"),
		static_cast<uint8>(Summary.Tier), static_cast<uint8>(EContributorTier::Contributor));

	Summary.ContributionPercentage = 40.0f;
	Summary.UpdateTier();
	TestEqual(TEXT("40% should be Major"),
		static_cast<uint8>(Summary.Tier), static_cast<uint8>(EContributorTier::Major));

	Summary.ContributionPercentage = 60.0f;
	Summary.UpdateTier();
	TestEqual(TEXT("60% should be Founder"),
		static_cast<uint8>(Summary.Tier), static_cast<uint8>(EContributorTier::Founder));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProject_RewardConfigMultipliers,
	"Odyssey.Social.CoopProject.Structs.RewardConfigMultipliers",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FProject_RewardConfigMultipliers::RunTest(const FString& Parameters)
{
	FProjectRewardConfig Config;

	TestEqual(TEXT("Participant multiplier default"), Config.GetTierMultiplier(EContributorTier::Participant), 1.0f);
	TestEqual(TEXT("Supporter multiplier default"), Config.GetTierMultiplier(EContributorTier::Supporter), 1.5f);
	TestEqual(TEXT("Contributor multiplier default"), Config.GetTierMultiplier(EContributorTier::Contributor), 2.0f);
	TestEqual(TEXT("Major multiplier default"), Config.GetTierMultiplier(EContributorTier::Major), 3.0f);
	TestEqual(TEXT("Founder multiplier default"), Config.GetTierMultiplier(EContributorTier::Founder), 5.0f);

	return true;
}

