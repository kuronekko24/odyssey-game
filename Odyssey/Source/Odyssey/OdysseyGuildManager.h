// OdysseyGuildManager.h
// Guild and organization management system for Odyssey
// Handles guild creation, membership, roles, permissions, and coordination

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyGuildManager.generated.h"

// Forward declarations
class UOdysseyGuildManager;
class UReputationSystem;

/**
 * Guild permission flags - defines what actions members can perform
 */
UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EGuildPermission : uint32
{
	None                    = 0         UMETA(Hidden),
	ViewMembers             = 1 << 0    UMETA(DisplayName = "View Members"),
	InviteMembers           = 1 << 1    UMETA(DisplayName = "Invite Members"),
	KickMembers             = 1 << 2    UMETA(DisplayName = "Kick Members"),
	PromoteMembers          = 1 << 3    UMETA(DisplayName = "Promote Members"),
	DemoteMembers           = 1 << 4    UMETA(DisplayName = "Demote Members"),
	EditRoles               = 1 << 5    UMETA(DisplayName = "Edit Roles"),
	AccessGuildBank         = 1 << 6    UMETA(DisplayName = "Access Guild Bank"),
	DepositToBank           = 1 << 7    UMETA(DisplayName = "Deposit to Bank"),
	WithdrawFromBank        = 1 << 8    UMETA(DisplayName = "Withdraw from Bank"),
	ManageProjects          = 1 << 9    UMETA(DisplayName = "Manage Projects"),
	ContributeToProjects    = 1 << 10   UMETA(DisplayName = "Contribute to Projects"),
	UseGuildFacilities      = 1 << 11   UMETA(DisplayName = "Use Guild Facilities"),
	ManageFacilities        = 1 << 12   UMETA(DisplayName = "Manage Facilities"),
	SendGuildAnnouncements  = 1 << 13   UMETA(DisplayName = "Send Announcements"),
	ModifyGuildSettings     = 1 << 14   UMETA(DisplayName = "Modify Guild Settings"),
	DeclareWar              = 1 << 15   UMETA(DisplayName = "Declare War"),
	NegotiateAlliances      = 1 << 16   UMETA(DisplayName = "Negotiate Alliances"),
	ManageTaxes             = 1 << 17   UMETA(DisplayName = "Manage Taxes"),
	ViewAuditLog            = 1 << 18   UMETA(DisplayName = "View Audit Log"),
	DisbandGuild            = 1 << 19   UMETA(DisplayName = "Disband Guild"),
	All                     = 0xFFFFFFFF UMETA(Hidden)
};
ENUM_CLASS_FLAGS(EGuildPermission);

/**
 * Guild relationship types
 */
UENUM(BlueprintType)
enum class EGuildRelationship : uint8
{
	Neutral,
	Friendly,
	Allied,
	Hostile,
	AtWar
};

/**
 * Guild membership status
 */
UENUM(BlueprintType)
enum class EGuildMemberStatus : uint8
{
	Active,
	Inactive,
	Away,
	Banned
};

/**
 * Guild role definition
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildRole
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Role")
	FName RoleID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Role")
	FString RoleName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Role")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Role")
	int32 RankPriority; // Higher = more authority

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Role", Meta = (Bitmask, BitmaskEnum = "/Script/Odyssey.EGuildPermission"))
	int32 Permissions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Role")
	FLinearColor RoleColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Role")
	int32 MaxWithdrawalPerDay; // OMEN withdrawal limit

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Role")
	bool bIsDefault;

	FGuildRole()
		: RoleID(NAME_None)
		, RoleName(TEXT("Member"))
		, Description(TEXT("Standard guild member"))
		, RankPriority(0)
		, Permissions(0)
		, RoleColor(FLinearColor::White)
		, MaxWithdrawalPerDay(0)
		, bIsDefault(false)
	{
	}

	bool HasPermission(EGuildPermission Permission) const
	{
		return (Permissions & static_cast<int32>(Permission)) \!= 0;
	}

	void AddPermission(EGuildPermission Permission)
	{
		Permissions |= static_cast<int32>(Permission);
	}

	void RemovePermission(EGuildPermission Permission)
	{
		Permissions &= ~static_cast<int32>(Permission);
	}
};

/**
 * Guild member data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildMember
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	FString PlayerID; // Unique player identifier

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	FName RoleID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	EGuildMemberStatus Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	FDateTime JoinDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	FDateTime LastActiveDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	int64 TotalContribution; // Total OMEN contributed

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	int32 ContributionPoints; // Activity-based points

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	int32 WithdrawnToday; // Tracks daily withdrawal

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Member")
	FString Note; // Officer note about member

	FGuildMember()
		: PlayerID(TEXT(""))
		, PlayerName(TEXT("Unknown"))
		, RoleID(NAME_None)
		, Status(EGuildMemberStatus::Active)
		, TotalContribution(0)
		, ContributionPoints(0)
		, WithdrawnToday(0)
		, Note(TEXT(""))
	{
		JoinDate = FDateTime::Now();
		LastActiveDate = FDateTime::Now();
	}
};

/**
 * Guild invitation data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildInvitation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Invitation")
	FGuid InvitationID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Invitation")
	FGuid GuildID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Invitation")
	FString InvitedPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Invitation")
	FString InviterPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Invitation")
	FDateTime InvitationDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Invitation")
	FDateTime ExpirationDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Invitation")
	FString Message;

	FGuildInvitation()
		: InvitedPlayerID(TEXT(""))
		, InviterPlayerID(TEXT(""))
		, Message(TEXT(""))
	{
		InvitationID = FGuid::NewGuid();
		InvitationDate = FDateTime::Now();
		ExpirationDate = FDateTime::Now() + FTimespan::FromDays(7);
	}

	bool IsExpired() const
	{
		return FDateTime::Now() > ExpirationDate;
	}
};

/**
 * Guild announcement/message
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildAnnouncement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Announcement")
	FGuid AnnouncementID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Announcement")
	FString AuthorPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Announcement")
	FString AuthorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Announcement")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Announcement")
	FString Content;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Announcement")
	FDateTime PostDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Announcement")
	bool bIsPinned;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Announcement")
	int32 Priority;

	FGuildAnnouncement()
		: AuthorPlayerID(TEXT(""))
		, AuthorName(TEXT(""))
		, Title(TEXT(""))
		, Content(TEXT(""))
		, bIsPinned(false)
		, Priority(0)
	{
		AnnouncementID = FGuid::NewGuid();
		PostDate = FDateTime::Now();
	}
};

/**
 * Guild audit log entry
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildAuditLogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Audit")
	FDateTime Timestamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Audit")
	FString ActorPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Audit")
	FString ActorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Audit")
	FString Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Audit")
	FString Details;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Audit")
	FString TargetPlayerID;

	FGuildAuditLogEntry()
		: ActorPlayerID(TEXT(""))
		, ActorName(TEXT(""))
		, Action(TEXT(""))
		, Details(TEXT(""))
		, TargetPlayerID(TEXT(""))
	{
		Timestamp = FDateTime::Now();
	}
};

/**
 * Guild relationship with another guild
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildDiplomacy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Diplomacy")
	FGuid OtherGuildID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Diplomacy")
	FString OtherGuildName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Diplomacy")
	EGuildRelationship Relationship;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Diplomacy")
	FDateTime RelationshipStartDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Diplomacy")
	int32 StandingPoints; // -100 to 100

	FGuildDiplomacy()
		: OtherGuildName(TEXT(""))
		, Relationship(EGuildRelationship::Neutral)
		, StandingPoints(0)
	{
		RelationshipStartDate = FDateTime::Now();
	}
};

/**
 * Complete guild data structure
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	FGuid GuildID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	FString GuildName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	FString GuildTag; // Short abbreviation [TAG]

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	FString MotD; // Message of the Day

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	FString FounderPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	FDateTime FoundedDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	int32 Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	int64 Experience;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	int32 MaxMembers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	bool bIsRecruiting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	int32 MinLevelToJoin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	float TaxRate; // 0.0 to 1.0 - portion of member earnings to guild

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	TArray<FGuildRole> Roles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	TArray<FGuildMember> Members;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	TArray<FGuildInvitation> PendingInvitations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	TArray<FGuildAnnouncement> Announcements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	TArray<FGuildAuditLogEntry> AuditLog;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	TArray<FGuildDiplomacy> Diplomacy;

	// Guild bank resources
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild")
	TMap<EResourceType, int64> BankResources;

	FGuildData()
		: GuildName(TEXT("New Guild"))
		, GuildTag(TEXT("NEW"))
		, Description(TEXT(""))
		, MotD(TEXT(""))
		, FounderPlayerID(TEXT(""))
		, Level(1)
		, Experience(0)
		, MaxMembers(50)
		, bIsRecruiting(true)
		, MinLevelToJoin(1)
		, TaxRate(0.05f)
	{
		GuildID = FGuid::NewGuid();
		FoundedDate = FDateTime::Now();
		InitializeDefaultRoles();
	}

	void InitializeDefaultRoles()
	{
		// Guild Master role
		FGuildRole GuildMaster;
		GuildMaster.RoleID = FName(TEXT("GuildMaster"));
		GuildMaster.RoleName = TEXT("Guild Master");
		GuildMaster.Description = TEXT("Leader of the guild with full permissions");
		GuildMaster.RankPriority = 100;
		GuildMaster.Permissions = static_cast<int32>(EGuildPermission::All);
		GuildMaster.RoleColor = FLinearColor(1.0f, 0.84f, 0.0f); // Gold
		GuildMaster.MaxWithdrawalPerDay = -1; // Unlimited
		Roles.Add(GuildMaster);

		// Officer role
		FGuildRole Officer;
		Officer.RoleID = FName(TEXT("Officer"));
		Officer.RoleName = TEXT("Officer");
		Officer.Description = TEXT("Senior member with management permissions");
		Officer.RankPriority = 50;
		Officer.Permissions = static_cast<int32>(EGuildPermission::ViewMembers) |
			static_cast<int32>(EGuildPermission::InviteMembers) |
			static_cast<int32>(EGuildPermission::KickMembers) |
			static_cast<int32>(EGuildPermission::AccessGuildBank) |
			static_cast<int32>(EGuildPermission::DepositToBank) |
			static_cast<int32>(EGuildPermission::WithdrawFromBank) |
			static_cast<int32>(EGuildPermission::ManageProjects) |
			static_cast<int32>(EGuildPermission::ContributeToProjects) |
			static_cast<int32>(EGuildPermission::UseGuildFacilities) |
			static_cast<int32>(EGuildPermission::SendGuildAnnouncements) |
			static_cast<int32>(EGuildPermission::ViewAuditLog);
		Officer.RoleColor = FLinearColor(0.0f, 0.5f, 1.0f); // Blue
		Officer.MaxWithdrawalPerDay = 10000;
		Roles.Add(Officer);

		// Member role (default)
		FGuildRole Member;
		Member.RoleID = FName(TEXT("Member"));
		Member.RoleName = TEXT("Member");
		Member.Description = TEXT("Standard guild member");
		Member.RankPriority = 10;
		Member.Permissions = static_cast<int32>(EGuildPermission::ViewMembers) |
			static_cast<int32>(EGuildPermission::AccessGuildBank) |
			static_cast<int32>(EGuildPermission::DepositToBank) |
			static_cast<int32>(EGuildPermission::ContributeToProjects) |
			static_cast<int32>(EGuildPermission::UseGuildFacilities);
		Member.RoleColor = FLinearColor::Green;
		Member.MaxWithdrawalPerDay = 1000;
		Member.bIsDefault = true;
		Roles.Add(Member);

		// Recruit role
		FGuildRole Recruit;
		Recruit.RoleID = FName(TEXT("Recruit"));
		Recruit.RoleName = TEXT("Recruit");
		Recruit.Description = TEXT("New member on probation");
		Recruit.RankPriority = 1;
		Recruit.Permissions = static_cast<int32>(EGuildPermission::ViewMembers) |
			static_cast<int32>(EGuildPermission::DepositToBank) |
			static_cast<int32>(EGuildPermission::ContributeToProjects);
		Recruit.RoleColor = FLinearColor::Gray;
		Recruit.MaxWithdrawalPerDay = 0;
		Roles.Add(Recruit);
	}

	const FGuildRole* GetRole(FName RoleID) const
	{
		return Roles.FindByPredicate([RoleID](const FGuildRole& Role) {
			return Role.RoleID == RoleID;
		});
	}

	const FGuildRole* GetDefaultRole() const
	{
		return Roles.FindByPredicate([](const FGuildRole& Role) {
			return Role.bIsDefault;
		});
	}

	FGuildMember* GetMember(const FString& PlayerID)
	{
		return Members.FindByPredicate([&PlayerID](const FGuildMember& Member) {
			return Member.PlayerID == PlayerID;
		});
	}

	const FGuildMember* GetMember(const FString& PlayerID) const
	{
		return Members.FindByPredicate([&PlayerID](const FGuildMember& Member) {
			return Member.PlayerID == PlayerID;
		});
	}

	bool IsMember(const FString& PlayerID) const
	{
		return GetMember(PlayerID) \!= nullptr;
	}

	int32 GetMemberCount() const
	{
		return Members.Num();
	}

	int32 GetOnlineMemberCount() const
	{
		int32 Count = 0;
		for (const FGuildMember& Member : Members)
		{
			if (Member.Status == EGuildMemberStatus::Active)
			{
				Count++;
			}
		}
		return Count;
	}
};

// Delegates for guild events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGuildCreated, const FGuid&, GuildID, const FString&, GuildName, const FString&, FounderID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGuildDisbanded, const FGuid&, GuildID, const FString&, GuildName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMemberJoined, const FGuid&, GuildID, const FString&, PlayerID, const FString&, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMemberLeft, const FGuid&, GuildID, const FString&, PlayerID, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMemberPromoted, const FGuid&, GuildID, const FString&, PlayerID, FName, OldRole, FName, NewRole);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGuildBankTransaction, const FGuid&, GuildID, const FString&, PlayerID, EResourceType, ResourceType, int64, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGuildLevelUp, const FGuid&, GuildID, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnGuildRelationshipChanged, const FGuid&, GuildID, const FGuid&, OtherGuildID, EGuildRelationship, OldRelationship, EGuildRelationship, NewRelationship);

/**
 * UOdysseyGuildManager
 * 
 * Central manager for all guild operations in Odyssey.
 * Handles guild lifecycle, membership, permissions, and coordination.
 * Designed for multiplayer with proper synchronization.
 */
UCLASS(Blueprintable, BlueprintType)
class ODYSSEY_API UOdysseyGuildManager : public UObject
{
	GENERATED_BODY()

public:
	UOdysseyGuildManager();

	// Initialize the guild manager
	UFUNCTION(BlueprintCallable, Category = "Guild")
	void Initialize();

	// ==================== Guild Lifecycle ====================

	/** Create a new guild */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	FGuid CreateGuild(const FString& FounderPlayerID, const FString& FounderName,
		const FString& GuildName, const FString& GuildTag, const FString& Description);

	/** Disband a guild (founder/GM only) */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool DisbandGuild(const FGuid& GuildID, const FString& RequestingPlayerID);

	/** Get guild data by ID */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool GetGuildData(const FGuid& GuildID, FGuildData& OutGuildData) const;

	/** Get guild by name (for searching) */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	FGuid FindGuildByName(const FString& GuildName) const;

	/** Get player's current guild */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	FGuid GetPlayerGuild(const FString& PlayerID) const;

	/** Check if player is in any guild */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool IsPlayerInGuild(const FString& PlayerID) const;

	// ==================== Membership Management ====================

	/** Invite a player to the guild */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool InvitePlayer(const FGuid& GuildID, const FString& InviterPlayerID,
		const FString& InvitedPlayerID, const FString& Message);

	/** Accept a guild invitation */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool AcceptInvitation(const FGuid& InvitationID, const FString& PlayerID, const FString& PlayerName);

	/** Decline a guild invitation */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool DeclineInvitation(const FGuid& InvitationID, const FString& PlayerID);

	/** Get pending invitations for a player */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	TArray<FGuildInvitation> GetPlayerInvitations(const FString& PlayerID) const;

	/** Leave the guild voluntarily */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool LeaveGuild(const FString& PlayerID);

	/** Kick a member from the guild */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool KickMember(const FGuid& GuildID, const FString& KickerPlayerID,
		const FString& TargetPlayerID, const FString& Reason);

	/** Update member role */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool SetMemberRole(const FGuid& GuildID, const FString& PromoterPlayerID,
		const FString& TargetPlayerID, FName NewRoleID);

	/** Update member status (online/offline/away) */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	void UpdateMemberStatus(const FString& PlayerID, EGuildMemberStatus NewStatus);

	// ==================== Permission System ====================

	/** Check if player has a specific permission */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool HasPermission(const FGuid& GuildID, const FString& PlayerID, EGuildPermission Permission) const;

	/** Check if player can perform action on target */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool CanActOnMember(const FGuid& GuildID, const FString& ActorPlayerID,
		const FString& TargetPlayerID) const;

	// ==================== Role Management ====================

	/** Create a new custom role */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool CreateRole(const FGuid& GuildID, const FString& CreatorPlayerID,
		const FGuildRole& NewRole);

	/** Modify an existing role */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool ModifyRole(const FGuid& GuildID, const FString& ModifierPlayerID,
		FName RoleID, const FGuildRole& UpdatedRole);

	/** Delete a custom role */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool DeleteRole(const FGuid& GuildID, const FString& DeleterPlayerID, FName RoleID);

	/** Get all roles in a guild */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	TArray<FGuildRole> GetGuildRoles(const FGuid& GuildID) const;

	// ==================== Guild Bank ====================

	/** Deposit resources to guild bank */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool DepositToBank(const FGuid& GuildID, const FString& PlayerID,
		EResourceType ResourceType, int64 Amount);

	/** Withdraw resources from guild bank */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool WithdrawFromBank(const FGuid& GuildID, const FString& PlayerID,
		EResourceType ResourceType, int64 Amount);

	/** Get guild bank balance for a resource */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	int64 GetBankBalance(const FGuid& GuildID, EResourceType ResourceType) const;

	/** Get all guild bank resources */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	TMap<EResourceType, int64> GetAllBankResources(const FGuid& GuildID) const;

	/** Check daily withdrawal limit for member */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	int32 GetRemainingWithdrawal(const FGuid& GuildID, const FString& PlayerID) const;

	// ==================== Communication ====================

	/** Post an announcement */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool PostAnnouncement(const FGuid& GuildID, const FString& AuthorPlayerID,
		const FString& Title, const FString& Content, bool bPinned);

	/** Delete an announcement */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool DeleteAnnouncement(const FGuid& GuildID, const FString& DeleterPlayerID,
		const FGuid& AnnouncementID);

	/** Get guild announcements */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	TArray<FGuildAnnouncement> GetAnnouncements(const FGuid& GuildID, int32 MaxCount = 10) const;

	/** Update Message of the Day */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool SetMotD(const FGuid& GuildID, const FString& PlayerID, const FString& NewMotD);

	// ==================== Guild Settings ====================

	/** Update guild settings */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool UpdateGuildSettings(const FGuid& GuildID, const FString& PlayerID,
		const FString& NewDescription, float NewTaxRate, bool bNewRecruiting, int32 NewMinLevel);

	/** Set guild tax rate */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool SetTaxRate(const FGuid& GuildID, const FString& PlayerID, float NewTaxRate);

	/** Add experience to guild (from member activities) */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	void AddGuildExperience(const FGuid& GuildID, int64 Experience);

	/** Get experience required for next level */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	int64 GetExperienceForLevel(int32 Level) const;

	// ==================== Diplomacy ====================

	/** Propose alliance with another guild */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool ProposeAlliance(const FGuid& ProposingGuildID, const FString& ProposerPlayerID,
		const FGuid& TargetGuildID);

	/** Accept alliance proposal */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool AcceptAlliance(const FGuid& AcceptingGuildID, const FString& AccepterPlayerID,
		const FGuid& ProposingGuildID);

	/** Declare war on another guild */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool DeclareWar(const FGuid& DeclaringGuildID, const FString& DeclarerPlayerID,
		const FGuid& TargetGuildID);

	/** End war/alliance with another guild */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	bool EndDiplomaticRelation(const FGuid& GuildID, const FString& PlayerID,
		const FGuid& OtherGuildID);

	/** Get relationship between two guilds */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	EGuildRelationship GetGuildRelationship(const FGuid& GuildID1, const FGuid& GuildID2) const;

	// ==================== Audit Log ====================

	/** Get audit log entries */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	TArray<FGuildAuditLogEntry> GetAuditLog(const FGuid& GuildID, const FString& PlayerID,
		int32 MaxEntries = 50) const;

	// ==================== Search & Discovery ====================

	/** Search for guilds */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	TArray<FGuildData> SearchGuilds(const FString& SearchQuery, bool bRecruitingOnly = false,
		int32 MaxResults = 20) const;

	/** Get top guilds by level */
	UFUNCTION(BlueprintCallable, Category = "Guild")
	TArray<FGuildData> GetTopGuilds(int32 Count = 10) const;

	// ==================== Events ====================

	UPROPERTY(BlueprintAssignable, Category = "Guild Events")
	FOnGuildCreated OnGuildCreated;

	UPROPERTY(BlueprintAssignable, Category = "Guild Events")
	FOnGuildDisbanded OnGuildDisbanded;

	UPROPERTY(BlueprintAssignable, Category = "Guild Events")
	FOnMemberJoined OnMemberJoined;

	UPROPERTY(BlueprintAssignable, Category = "Guild Events")
	FOnMemberLeft OnMemberLeft;

	UPROPERTY(BlueprintAssignable, Category = "Guild Events")
	FOnMemberPromoted OnMemberPromoted;

	UPROPERTY(BlueprintAssignable, Category = "Guild Events")
	FOnGuildBankTransaction OnGuildBankTransaction;

	UPROPERTY(BlueprintAssignable, Category = "Guild Events")
	FOnGuildLevelUp OnGuildLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Guild Events")
	FOnGuildRelationshipChanged OnGuildRelationshipChanged;

protected:
	// All guilds indexed by ID
	UPROPERTY()
	TMap<FGuid, FGuildData> Guilds;

	// Player to guild mapping for quick lookup
	UPROPERTY()
	TMap<FString, FGuid> PlayerGuildMap;

	// Thread safety
	mutable FCriticalSection GuildLock;

	// Internal helper functions
	void AddAuditLogEntry(FGuildData& Guild, const FString& ActorPlayerID,
		const FString& ActorName, const FString& Action, const FString& Details,
		const FString& TargetPlayerID = TEXT(""));

	void CheckGuildLevelUp(FGuildData& Guild);

	void SetGuildRelationship(FGuildData& Guild, const FGuid& OtherGuildID,
		const FString& OtherGuildName, EGuildRelationship NewRelationship);

	void ResetDailyWithdrawals();

	FTimerHandle DailyResetTimerHandle;
};
