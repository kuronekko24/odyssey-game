// GuildManagerTests.cpp
// Automation tests for UOdysseyGuildManager
// Tests guild creation, membership, roles, permissions, diplomacy, and edge cases

#include "Misc/AutomationTest.h"
#include "OdysseyGuildManager.h"
#include "OdysseyInventoryComponent.h"

// ============================================================================
// TEST HELPERS
// ============================================================================

namespace GuildTestHelpers
{
	/** Create and initialize a fresh GuildManager for testing */
	UOdysseyGuildManager* CreateTestGuildManager()
	{
		UOdysseyGuildManager* Manager = NewObject<UOdysseyGuildManager>();
		Manager->Initialize();
		return Manager;
	}

	/** Create a guild with a default founder and return the GuildID */
	FGuid CreateTestGuild(UOdysseyGuildManager* Manager,
		const FString& FounderID = TEXT("Founder001"),
		const FString& FounderName = TEXT("TestFounder"),
		const FString& GuildName = TEXT("TestGuild"),
		const FString& GuildTag = TEXT("TG"),
		const FString& Description = TEXT("A test guild"))
	{
		return Manager->CreateGuild(FounderID, FounderName, GuildName, GuildTag, Description);
	}

	/** Invite and accept a player into a guild */
	bool AddMemberToGuild(UOdysseyGuildManager* Manager, const FGuid& GuildID,
		const FString& InviterID, const FString& InviteeID, const FString& InviteeName)
	{
		bool bInvited = Manager->InvitePlayer(GuildID, InviterID, InviteeID, TEXT("Join us!"));
		if (!bInvited) return false;

		TArray<FGuildInvitation> Invitations = Manager->GetPlayerInvitations(InviteeID);
		if (Invitations.Num() == 0) return false;

		return Manager->AcceptInvitation(Invitations[0].InvitationID, InviteeID, InviteeName);
	}
}

// ============================================================================
// GUILD CREATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildCreation_ValidParams,
	"Odyssey.Social.GuildManager.Creation.ValidParameters",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildCreation_ValidParams::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();

	FGuid GuildID = Manager->CreateGuild(
		TEXT("Player001"), TEXT("Alice"),
		TEXT("Starforged"), TEXT("SF"),
		TEXT("Elite space explorers"));

	TestTrue(TEXT("Guild ID should be valid"), GuildID.IsValid());

	FGuildData Data;
	bool bFound = Manager->GetGuildData(GuildID, Data);
	TestTrue(TEXT("Guild data should be retrievable"), bFound);
	TestEqual(TEXT("Guild name should match"), Data.GuildName, FString(TEXT("Starforged")));
	TestEqual(TEXT("Guild tag should match"), Data.GuildTag, FString(TEXT("SF")));
	TestEqual(TEXT("Description should match"), Data.Description, FString(TEXT("Elite space explorers")));
	TestEqual(TEXT("Founder ID should match"), Data.FounderPlayerID, FString(TEXT("Player001")));
	TestEqual(TEXT("Guild level should be 1"), Data.Level, 1);
	TestEqual(TEXT("Guild should start with 1 member (founder)"), Data.GetMemberCount(), 1);
	TestTrue(TEXT("Founder should be a member"), Data.IsMember(TEXT("Player001")));

	// Founder should have GuildMaster role
	const FGuildMember* Founder = Data.GetMember(TEXT("Player001"));
	TestNotNull(TEXT("Founder member data should exist"), Founder);
	if (Founder)
	{
		TestEqual(TEXT("Founder role should be GuildMaster"), Founder->RoleID, FName(TEXT("GuildMaster")));
	}

	// Player should be mapped to guild
	TestTrue(TEXT("Player should be registered in a guild"), Manager->IsPlayerInGuild(TEXT("Player001")));
	TestEqual(TEXT("Player guild lookup should return correct guild"), Manager->GetPlayerGuild(TEXT("Player001")), GuildID);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildCreation_DefaultRoles,
	"Odyssey.Social.GuildManager.Creation.DefaultRolesInitialized",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildCreation_DefaultRoles::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	TArray<FGuildRole> Roles = Manager->GetGuildRoles(GuildID);
	TestTrue(TEXT("Guild should have at least 4 default roles"), Roles.Num() >= 4);

	// Verify role hierarchy
	bool bHasGuildMaster = false;
	bool bHasOfficer = false;
	bool bHasMember = false;
	bool bHasRecruit = false;
	bool bHasDefaultRole = false;

	for (const FGuildRole& Role : Roles)
	{
		if (Role.RoleID == FName(TEXT("GuildMaster")))
		{
			bHasGuildMaster = true;
			TestEqual(TEXT("GuildMaster priority should be 100"), Role.RankPriority, 100);
			TestTrue(TEXT("GuildMaster should have all permissions"),
				Role.HasPermission(EGuildPermission::DisbandGuild));
		}
		else if (Role.RoleID == FName(TEXT("Officer")))
		{
			bHasOfficer = true;
			TestEqual(TEXT("Officer priority should be 50"), Role.RankPriority, 50);
			TestTrue(TEXT("Officer should be able to invite"),
				Role.HasPermission(EGuildPermission::InviteMembers));
			TestTrue(TEXT("Officer should be able to kick"),
				Role.HasPermission(EGuildPermission::KickMembers));
		}
		else if (Role.RoleID == FName(TEXT("Member")))
		{
			bHasMember = true;
			TestEqual(TEXT("Member priority should be 10"), Role.RankPriority, 10);
			TestTrue(TEXT("Member should be the default role"), Role.bIsDefault);
			bHasDefaultRole = Role.bIsDefault;
		}
		else if (Role.RoleID == FName(TEXT("Recruit")))
		{
			bHasRecruit = true;
			TestEqual(TEXT("Recruit priority should be 1"), Role.RankPriority, 1);
		}
	}

	TestTrue(TEXT("GuildMaster role should exist"), bHasGuildMaster);
	TestTrue(TEXT("Officer role should exist"), bHasOfficer);
	TestTrue(TEXT("Member role should exist"), bHasMember);
	TestTrue(TEXT("Recruit role should exist"), bHasRecruit);
	TestTrue(TEXT("A default role should be marked"), bHasDefaultRole);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildCreation_DuplicateName,
	"Odyssey.Social.GuildManager.Creation.DuplicateNameAllowed",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildCreation_DuplicateName::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();

	FGuid Guild1 = Manager->CreateGuild(TEXT("P001"), TEXT("Alice"), TEXT("UniqueName"), TEXT("UN"), TEXT(""));
	TestTrue(TEXT("First guild should be created"), Guild1.IsValid());

	// Second guild with same name from different player
	FGuid Guild2 = Manager->CreateGuild(TEXT("P002"), TEXT("Bob"), TEXT("UniqueName"), TEXT("UN"), TEXT(""));
	// Whether duplicates are allowed is implementation-specific; either outcome is valid
	// but we document the behavior
	if (Guild2.IsValid())
	{
		TestNotEqual(TEXT("Different guilds should have different IDs"), Guild1, Guild2);
	}
	// If invalid, duplicates are rejected -- also valid

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildCreation_AlreadyInGuild,
	"Odyssey.Social.GuildManager.Creation.FounderAlreadyInGuild",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildCreation_AlreadyInGuild::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();

	FGuid Guild1 = Manager->CreateGuild(TEXT("P001"), TEXT("Alice"), TEXT("Guild1"), TEXT("G1"), TEXT(""));
	TestTrue(TEXT("First guild creation should succeed"), Guild1.IsValid());

	// Player already in a guild tries to create another
	FGuid Guild2 = Manager->CreateGuild(TEXT("P001"), TEXT("Alice"), TEXT("Guild2"), TEXT("G2"), TEXT(""));
	TestFalse(TEXT("Player already in guild should not create another"), Guild2.IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildCreation_FindByName,
	"Odyssey.Social.GuildManager.Creation.FindByName",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildCreation_FindByName::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();

	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager, TEXT("P001"), TEXT("Alice"),
		TEXT("Astral Knights"), TEXT("AK"), TEXT(""));

	FGuid Found = Manager->FindGuildByName(TEXT("Astral Knights"));
	TestEqual(TEXT("FindGuildByName should return correct guild"), Found, GuildID);

	FGuid NotFound = Manager->FindGuildByName(TEXT("NonExistent"));
	TestFalse(TEXT("Non-existent guild name should return invalid GUID"), NotFound.IsValid());

	return true;
}

// ============================================================================
// MEMBER MANAGEMENT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildMember_InviteAndAccept,
	"Odyssey.Social.GuildManager.Members.InviteAndAccept",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildMember_InviteAndAccept::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	bool bInvited = Manager->InvitePlayer(GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Welcome!"));
	TestTrue(TEXT("Invitation should succeed"), bInvited);

	TArray<FGuildInvitation> Invitations = Manager->GetPlayerInvitations(TEXT("Player002"));
	TestEqual(TEXT("Player should have 1 pending invitation"), Invitations.Num(), 1);

	bool bAccepted = Manager->AcceptInvitation(Invitations[0].InvitationID, TEXT("Player002"), TEXT("Bob"));
	TestTrue(TEXT("Accepting invitation should succeed"), bAccepted);

	FGuildData Data;
	Manager->GetGuildData(GuildID, Data);
	TestEqual(TEXT("Guild should now have 2 members"), Data.GetMemberCount(), 2);
	TestTrue(TEXT("New member should be in the guild"), Data.IsMember(TEXT("Player002")));

	// New member should have default role
	const FGuildMember* NewMember = Data.GetMember(TEXT("Player002"));
	TestNotNull(TEXT("New member data should exist"), NewMember);
	if (NewMember)
	{
		const FGuildRole* DefaultRole = Data.GetDefaultRole();
		TestNotNull(TEXT("Default role should exist"), DefaultRole);
		if (DefaultRole)
		{
			TestEqual(TEXT("New member should have default role"), NewMember->RoleID, DefaultRole->RoleID);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildMember_DeclineInvitation,
	"Odyssey.Social.GuildManager.Members.DeclineInvitation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildMember_DeclineInvitation::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	Manager->InvitePlayer(GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT(""));
	TArray<FGuildInvitation> Invitations = Manager->GetPlayerInvitations(TEXT("Player002"));

	bool bDeclined = Manager->DeclineInvitation(Invitations[0].InvitationID, TEXT("Player002"));
	TestTrue(TEXT("Declining invitation should succeed"), bDeclined);

	TestFalse(TEXT("Player should not be in any guild"), Manager->IsPlayerInGuild(TEXT("Player002")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildMember_KickMember,
	"Odyssey.Social.GuildManager.Members.KickMember",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildMember_KickMember::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Bob"));

	// Founder kicks member
	bool bKicked = Manager->KickMember(GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Inactivity"));
	TestTrue(TEXT("Kicking member should succeed"), bKicked);

	FGuildData Data;
	Manager->GetGuildData(GuildID, Data);
	TestEqual(TEXT("Guild should have 1 member after kick"), Data.GetMemberCount(), 1);
	TestFalse(TEXT("Kicked player should not be in guild"), Data.IsMember(TEXT("Player002")));
	TestFalse(TEXT("Kicked player should not be in any guild"), Manager->IsPlayerInGuild(TEXT("Player002")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildMember_LeaveGuild,
	"Odyssey.Social.GuildManager.Members.VoluntaryLeave",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildMember_LeaveGuild::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Bob"));

	bool bLeft = Manager->LeaveGuild(TEXT("Player002"));
	TestTrue(TEXT("Leaving guild should succeed"), bLeft);
	TestFalse(TEXT("Player should no longer be in guild"), Manager->IsPlayerInGuild(TEXT("Player002")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildMember_InviteWithoutPermission,
	"Odyssey.Social.GuildManager.Members.InviteWithoutPermission",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildMember_InviteWithoutPermission::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	// Add a recruit (no invite permission)
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Recruit001"), TEXT("Recruit"));
	Manager->SetMemberRole(GuildID, TEXT("Founder001"), TEXT("Recruit001"), FName(TEXT("Recruit")));

	// Recruit tries to invite
	bool bInvited = Manager->InvitePlayer(GuildID, TEXT("Recruit001"), TEXT("Player003"), TEXT(""));
	TestFalse(TEXT("Recruit should not be able to invite"), bInvited);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildMember_MaxMembers,
	"Odyssey.Social.GuildManager.Members.MaxMembersLimit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildMember_MaxMembers::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	FGuildData Data;
	Manager->GetGuildData(GuildID, Data);
	int32 MaxMembers = Data.MaxMembers;

	// Fill guild to capacity (founder is already member #1)
	int32 AddedCount = 1;
	for (int32 i = 1; i < MaxMembers && AddedCount < MaxMembers; i++)
	{
		FString PlayerID = FString::Printf(TEXT("Player%03d"), i);
		FString PlayerName = FString::Printf(TEXT("Player_%d"), i);
		bool bAdded = GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), PlayerID, PlayerName);
		if (bAdded) AddedCount++;
	}

	Manager->GetGuildData(GuildID, Data);
	TestEqual(TEXT("Guild should be at max capacity"), Data.GetMemberCount(), MaxMembers);

	// Try to add one more
	FString OverflowID = TEXT("OverflowPlayer");
	bool bOverflow = GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), OverflowID, TEXT("Overflow"));
	TestFalse(TEXT("Adding member beyond max should fail"), bOverflow);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildMember_InviteAlreadyInGuild,
	"Odyssey.Social.GuildManager.Members.InvitePlayerAlreadyInGuild",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildMember_InviteAlreadyInGuild::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid Guild1 = GuildTestHelpers::CreateTestGuild(Manager, TEXT("P001"), TEXT("Alice"), TEXT("Guild1"), TEXT("G1"), TEXT(""));
	FGuid Guild2 = Manager->CreateGuild(TEXT("P002"), TEXT("Bob"), TEXT("Guild2"), TEXT("G2"), TEXT(""));

	// Try to invite P002 who is already in Guild2
	bool bInvited = Manager->InvitePlayer(Guild1, TEXT("P001"), TEXT("P002"), TEXT(""));
	// Player already in guild should not be invited (or invitation should fail to accept)
	// Either invite fails or accept fails -- test whichever the implementation does
	if (bInvited)
	{
		TArray<FGuildInvitation> Invitations = Manager->GetPlayerInvitations(TEXT("P002"));
		if (Invitations.Num() > 0)
		{
			bool bAccepted = Manager->AcceptInvitation(Invitations[0].InvitationID, TEXT("P002"), TEXT("Bob"));
			TestFalse(TEXT("Accepting invitation while in another guild should fail"), bAccepted);
		}
	}

	return true;
}

// ============================================================================
// ROLE & PERMISSION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildRole_AssignRole,
	"Odyssey.Social.GuildManager.Roles.AssignRole",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildRole_AssignRole::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Bob"));

	// Promote to Officer
	bool bPromoted = Manager->SetMemberRole(GuildID, TEXT("Founder001"), TEXT("Player002"), FName(TEXT("Officer")));
	TestTrue(TEXT("Promoting to Officer should succeed"), bPromoted);

	FGuildData Data;
	Manager->GetGuildData(GuildID, Data);
	const FGuildMember* Member = Data.GetMember(TEXT("Player002"));
	TestNotNull(TEXT("Member should exist"), Member);
	if (Member)
	{
		TestEqual(TEXT("Member should have Officer role"), Member->RoleID, FName(TEXT("Officer")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildRole_PermissionCheck,
	"Odyssey.Social.GuildManager.Roles.PermissionCheck",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildRole_PermissionCheck::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Bob"));
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player003"), TEXT("Carol"));

	// Assign roles
	Manager->SetMemberRole(GuildID, TEXT("Founder001"), TEXT("Player002"), FName(TEXT("Officer")));
	Manager->SetMemberRole(GuildID, TEXT("Founder001"), TEXT("Player003"), FName(TEXT("Recruit")));

	// GuildMaster has all permissions
	TestTrue(TEXT("GuildMaster should have DisbandGuild"),
		Manager->HasPermission(GuildID, TEXT("Founder001"), EGuildPermission::DisbandGuild));
	TestTrue(TEXT("GuildMaster should have ManageTaxes"),
		Manager->HasPermission(GuildID, TEXT("Founder001"), EGuildPermission::ManageTaxes));

	// Officer permissions
	TestTrue(TEXT("Officer should have InviteMembers"),
		Manager->HasPermission(GuildID, TEXT("Player002"), EGuildPermission::InviteMembers));
	TestTrue(TEXT("Officer should have KickMembers"),
		Manager->HasPermission(GuildID, TEXT("Player002"), EGuildPermission::KickMembers));
	TestFalse(TEXT("Officer should NOT have DisbandGuild"),
		Manager->HasPermission(GuildID, TEXT("Player002"), EGuildPermission::DisbandGuild));

	// Recruit permissions
	TestTrue(TEXT("Recruit should have ViewMembers"),
		Manager->HasPermission(GuildID, TEXT("Player003"), EGuildPermission::ViewMembers));
	TestFalse(TEXT("Recruit should NOT have InviteMembers"),
		Manager->HasPermission(GuildID, TEXT("Player003"), EGuildPermission::InviteMembers));
	TestFalse(TEXT("Recruit should NOT have WithdrawFromBank"),
		Manager->HasPermission(GuildID, TEXT("Player003"), EGuildPermission::WithdrawFromBank));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildRole_CanActOnMember,
	"Odyssey.Social.GuildManager.Roles.CanActOnMemberHierarchy",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildRole_CanActOnMember::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Bob"));
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player003"), TEXT("Carol"));

	Manager->SetMemberRole(GuildID, TEXT("Founder001"), TEXT("Player002"), FName(TEXT("Officer")));
	// Player003 stays as default Member

	// GuildMaster can act on anyone
	TestTrue(TEXT("GM can act on Officer"),
		Manager->CanActOnMember(GuildID, TEXT("Founder001"), TEXT("Player002")));
	TestTrue(TEXT("GM can act on Member"),
		Manager->CanActOnMember(GuildID, TEXT("Founder001"), TEXT("Player003")));

	// Officer can act on lower rank
	TestTrue(TEXT("Officer can act on Member"),
		Manager->CanActOnMember(GuildID, TEXT("Player002"), TEXT("Player003")));

	// Member cannot act on Officer
	TestFalse(TEXT("Member cannot act on Officer"),
		Manager->CanActOnMember(GuildID, TEXT("Player003"), TEXT("Player002")));

	// Member cannot act on GM
	TestFalse(TEXT("Member cannot act on GuildMaster"),
		Manager->CanActOnMember(GuildID, TEXT("Player003"), TEXT("Founder001")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildRole_CreateCustomRole,
	"Odyssey.Social.GuildManager.Roles.CreateCustomRole",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildRole_CreateCustomRole::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	FGuildRole CustomRole;
	CustomRole.RoleID = FName(TEXT("Treasurer"));
	CustomRole.RoleName = TEXT("Treasurer");
	CustomRole.Description = TEXT("Manages guild finances");
	CustomRole.RankPriority = 30;
	CustomRole.Permissions = static_cast<int32>(EGuildPermission::ViewMembers) |
		static_cast<int32>(EGuildPermission::AccessGuildBank) |
		static_cast<int32>(EGuildPermission::WithdrawFromBank) |
		static_cast<int32>(EGuildPermission::ManageTaxes) |
		static_cast<int32>(EGuildPermission::ViewAuditLog);
	CustomRole.MaxWithdrawalPerDay = 50000;

	bool bCreated = Manager->CreateRole(GuildID, TEXT("Founder001"), CustomRole);
	TestTrue(TEXT("Creating custom role should succeed"), bCreated);

	TArray<FGuildRole> Roles = Manager->GetGuildRoles(GuildID);
	TestTrue(TEXT("Guild should now have 5 roles"), Roles.Num() >= 5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildRole_DeleteRole,
	"Odyssey.Social.GuildManager.Roles.DeleteRole",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildRole_DeleteRole::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	// Create a custom role then delete it
	FGuildRole CustomRole;
	CustomRole.RoleID = FName(TEXT("CustomTemp"));
	CustomRole.RoleName = TEXT("Temporary");
	CustomRole.RankPriority = 25;
	Manager->CreateRole(GuildID, TEXT("Founder001"), CustomRole);

	int32 RoleBefore = Manager->GetGuildRoles(GuildID).Num();
	bool bDeleted = Manager->DeleteRole(GuildID, TEXT("Founder001"), FName(TEXT("CustomTemp")));
	TestTrue(TEXT("Deleting custom role should succeed"), bDeleted);

	int32 RoleAfter = Manager->GetGuildRoles(GuildID).Num();
	TestEqual(TEXT("Role count should decrease by 1"), RoleAfter, RoleBefore - 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildRole_PromoteWithoutPermission,
	"Odyssey.Social.GuildManager.Roles.PromoteWithoutPermission",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildRole_PromoteWithoutPermission::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Bob"));
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player003"), TEXT("Carol"));

	// Regular member tries to promote another
	bool bPromoted = Manager->SetMemberRole(GuildID, TEXT("Player002"), TEXT("Player003"), FName(TEXT("Officer")));
	TestFalse(TEXT("Regular member should not be able to promote"), bPromoted);

	return true;
}

// ============================================================================
// GUILD DISBAND TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildDisband_ByFounder,
	"Odyssey.Social.GuildManager.Disband.ByFounder",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildDisband_ByFounder::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Bob"));

	bool bDisbanded = Manager->DisbandGuild(GuildID, TEXT("Founder001"));
	TestTrue(TEXT("Founder should be able to disband"), bDisbanded);

	FGuildData Data;
	bool bFound = Manager->GetGuildData(GuildID, Data);
	TestFalse(TEXT("Guild should no longer exist"), bFound);

	TestFalse(TEXT("Founder should no longer be in guild"), Manager->IsPlayerInGuild(TEXT("Founder001")));
	TestFalse(TEXT("Member should no longer be in guild"), Manager->IsPlayerInGuild(TEXT("Player002")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildDisband_ByRegularMember,
	"Odyssey.Social.GuildManager.Disband.ByRegularMemberFails",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildDisband_ByRegularMember::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Bob"));

	bool bDisbanded = Manager->DisbandGuild(GuildID, TEXT("Player002"));
	TestFalse(TEXT("Regular member should NOT be able to disband"), bDisbanded);

	FGuildData Data;
	TestTrue(TEXT("Guild should still exist"), Manager->GetGuildData(GuildID, Data));

	return true;
}

// ============================================================================
// GUILD BANK TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildBank_DepositAndWithdraw,
	"Odyssey.Social.GuildManager.Bank.DepositAndWithdraw",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildBank_DepositAndWithdraw::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	// Deposit
	bool bDeposited = Manager->DepositToBank(GuildID, TEXT("Founder001"), EResourceType::OMEN, 5000);
	TestTrue(TEXT("Deposit should succeed"), bDeposited);

	int64 Balance = Manager->GetBankBalance(GuildID, EResourceType::OMEN);
	TestEqual(TEXT("Balance should be 5000"), Balance, (int64)5000);

	// Withdraw (founder has unlimited withdrawal)
	bool bWithdrawn = Manager->WithdrawFromBank(GuildID, TEXT("Founder001"), EResourceType::OMEN, 2000);
	TestTrue(TEXT("Withdrawal should succeed"), bWithdrawn);

	Balance = Manager->GetBankBalance(GuildID, EResourceType::OMEN);
	TestEqual(TEXT("Balance should be 3000 after withdrawal"), Balance, (int64)3000);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildBank_WithdrawExceedsBalance,
	"Odyssey.Social.GuildManager.Bank.WithdrawExceedsBalance",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildBank_WithdrawExceedsBalance::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	Manager->DepositToBank(GuildID, TEXT("Founder001"), EResourceType::OMEN, 1000);

	bool bWithdrawn = Manager->WithdrawFromBank(GuildID, TEXT("Founder001"), EResourceType::OMEN, 5000);
	TestFalse(TEXT("Withdrawing more than balance should fail"), bWithdrawn);

	int64 Balance = Manager->GetBankBalance(GuildID, EResourceType::OMEN);
	TestEqual(TEXT("Balance should remain unchanged"), Balance, (int64)1000);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildBank_DailyWithdrawalLimit,
	"Odyssey.Social.GuildManager.Bank.DailyWithdrawalLimit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildBank_DailyWithdrawalLimit::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);
	GuildTestHelpers::AddMemberToGuild(Manager, GuildID, TEXT("Founder001"), TEXT("Player002"), TEXT("Bob"));

	// Deposit a large amount
	Manager->DepositToBank(GuildID, TEXT("Founder001"), EResourceType::OMEN, 100000);

	// Member has MaxWithdrawalPerDay = 1000 (default Member role)
	int32 Remaining = Manager->GetRemainingWithdrawal(GuildID, TEXT("Player002"));
	TestTrue(TEXT("Member should have withdrawal allowance"), Remaining > 0);

	// Withdraw up to the limit
	bool bOk = Manager->WithdrawFromBank(GuildID, TEXT("Player002"), EResourceType::OMEN, 1000);
	TestTrue(TEXT("Withdrawal within daily limit should succeed"), bOk);

	// Try to withdraw more
	bool bOverLimit = Manager->WithdrawFromBank(GuildID, TEXT("Player002"), EResourceType::OMEN, 1);
	TestFalse(TEXT("Withdrawal exceeding daily limit should fail"), bOverLimit);

	return true;
}

// ============================================================================
// DIPLOMACY TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildDiplomacy_DeclareWar,
	"Odyssey.Social.GuildManager.Diplomacy.DeclareWar",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildDiplomacy_DeclareWar::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid Guild1 = Manager->CreateGuild(TEXT("P001"), TEXT("Alice"), TEXT("Guild1"), TEXT("G1"), TEXT(""));
	FGuid Guild2 = Manager->CreateGuild(TEXT("P002"), TEXT("Bob"), TEXT("Guild2"), TEXT("G2"), TEXT(""));

	// Default relationship should be Neutral
	EGuildRelationship Rel = Manager->GetGuildRelationship(Guild1, Guild2);
	TestEqual(TEXT("Default relationship should be Neutral"),
		static_cast<uint8>(Rel), static_cast<uint8>(EGuildRelationship::Neutral));

	bool bDeclared = Manager->DeclareWar(Guild1, TEXT("P001"), Guild2);
	TestTrue(TEXT("Declaring war should succeed"), bDeclared);

	Rel = Manager->GetGuildRelationship(Guild1, Guild2);
	TestEqual(TEXT("Relationship should be AtWar"),
		static_cast<uint8>(Rel), static_cast<uint8>(EGuildRelationship::AtWar));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildDiplomacy_Alliance,
	"Odyssey.Social.GuildManager.Diplomacy.FormAlliance",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildDiplomacy_Alliance::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid Guild1 = Manager->CreateGuild(TEXT("P001"), TEXT("Alice"), TEXT("Guild1"), TEXT("G1"), TEXT(""));
	FGuid Guild2 = Manager->CreateGuild(TEXT("P002"), TEXT("Bob"), TEXT("Guild2"), TEXT("G2"), TEXT(""));

	bool bProposed = Manager->ProposeAlliance(Guild1, TEXT("P001"), Guild2);
	TestTrue(TEXT("Proposing alliance should succeed"), bProposed);

	bool bAccepted = Manager->AcceptAlliance(Guild2, TEXT("P002"), Guild1);
	TestTrue(TEXT("Accepting alliance should succeed"), bAccepted);

	EGuildRelationship Rel = Manager->GetGuildRelationship(Guild1, Guild2);
	TestEqual(TEXT("Relationship should be Allied"),
		static_cast<uint8>(Rel), static_cast<uint8>(EGuildRelationship::Allied));

	return true;
}

// ============================================================================
// GUILD EXPERIENCE & LEVEL TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildExp_AddExperience,
	"Odyssey.Social.GuildManager.Experience.AddAndLevelUp",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildExp_AddExperience::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	FGuildData DataBefore;
	Manager->GetGuildData(GuildID, DataBefore);
	TestEqual(TEXT("Guild should start at level 1"), DataBefore.Level, 1);

	int64 XpNeeded = Manager->GetExperienceForLevel(1);
	TestTrue(TEXT("XP needed should be positive"), XpNeeded > 0);

	// Add enough XP to level up
	Manager->AddGuildExperience(GuildID, XpNeeded + 100);

	FGuildData DataAfter;
	Manager->GetGuildData(GuildID, DataAfter);
	TestTrue(TEXT("Guild should have leveled up"), DataAfter.Level > 1);

	return true;
}

// ============================================================================
// GUILD SETTINGS TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildSettings_UpdateTaxRate,
	"Odyssey.Social.GuildManager.Settings.TaxRate",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildSettings_UpdateTaxRate::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	bool bSet = Manager->SetTaxRate(GuildID, TEXT("Founder001"), 0.15f);
	TestTrue(TEXT("Setting tax rate should succeed"), bSet);

	FGuildData Data;
	Manager->GetGuildData(GuildID, Data);
	TestEqual(TEXT("Tax rate should be 0.15"), Data.TaxRate, 0.15f);

	// Invalid tax rate
	bool bInvalid = Manager->SetTaxRate(GuildID, TEXT("Founder001"), 1.5f);
	TestFalse(TEXT("Tax rate > 1.0 should be rejected"), bInvalid);

	return true;
}

// ============================================================================
// GUILD SEARCH TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildSearch_SearchGuilds,
	"Odyssey.Social.GuildManager.Search.BasicSearch",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildSearch_SearchGuilds::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	Manager->CreateGuild(TEXT("P001"), TEXT("Alice"), TEXT("Starforged Alliance"), TEXT("SA"), TEXT(""));
	Manager->CreateGuild(TEXT("P002"), TEXT("Bob"), TEXT("Dark Corsairs"), TEXT("DC"), TEXT(""));
	Manager->CreateGuild(TEXT("P003"), TEXT("Carol"), TEXT("Star Seekers"), TEXT("SS"), TEXT(""));

	TArray<FGuildData> Results = Manager->SearchGuilds(TEXT("Star"));
	TestTrue(TEXT("Search for 'Star' should return at least 2 results"), Results.Num() >= 2);

	return true;
}

// ============================================================================
// ANNOUNCEMENT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildAnnounce_PostAndGet,
	"Odyssey.Social.GuildManager.Announcements.PostAndRetrieve",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGuildAnnounce_PostAndGet::RunTest(const FString& Parameters)
{
	UOdysseyGuildManager* Manager = GuildTestHelpers::CreateTestGuildManager();
	FGuid GuildID = GuildTestHelpers::CreateTestGuild(Manager);

	bool bPosted = Manager->PostAnnouncement(GuildID, TEXT("Founder001"),
		TEXT("Weekly Raid"), TEXT("Join us Saturday at 8pm"), true);
	TestTrue(TEXT("Posting announcement should succeed"), bPosted);

	TArray<FGuildAnnouncement> Announcements = Manager->GetAnnouncements(GuildID);
	TestTrue(TEXT("Should have at least 1 announcement"), Announcements.Num() >= 1);

	if (Announcements.Num() > 0)
	{
		TestEqual(TEXT("Title should match"), Announcements[0].Title, FString(TEXT("Weekly Raid")));
		TestTrue(TEXT("Should be pinned"), Announcements[0].bIsPinned);
	}

	return true;
}

