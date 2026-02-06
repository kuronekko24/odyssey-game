// OdysseyGuildManager.cpp
// Implementation of the guild management system

#include "OdysseyGuildManager.h"
#include "TimerManager.h"
#include "Engine/World.h"

UOdysseyGuildManager::UOdysseyGuildManager()
{
}

void UOdysseyGuildManager::Initialize()
{
	// Set up daily reset timer for withdrawal limits
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DailyResetTimerHandle,
			this,
			&UOdysseyGuildManager::ResetDailyWithdrawals,
			86400.0f, // 24 hours in seconds
			true
		);
	}
}

// ==================== Guild Lifecycle ====================

FGuid UOdysseyGuildManager::CreateGuild(const FString& FounderPlayerID, const FString& FounderName,
	const FString& GuildName, const FString& GuildTag, const FString& Description)
{
	FScopeLock Lock(&GuildLock);

	// Check if player is already in a guild
	if (PlayerGuildMap.Contains(FounderPlayerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateGuild: Player %s is already in a guild"), *FounderPlayerID);
		return FGuid();
	}

	// Check for duplicate guild name
	for (const auto& GuildPair : Guilds)
	{
		if (GuildPair.Value.GuildName.Equals(GuildName, ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTemp, Warning, TEXT("CreateGuild: Guild name '%s' already exists"), *GuildName);
			return FGuid();
		}
	}

	// Create the guild
	FGuildData NewGuild;
	NewGuild.GuildName = GuildName;
	NewGuild.GuildTag = GuildTag;
	NewGuild.Description = Description;
	NewGuild.FounderPlayerID = FounderPlayerID;
	NewGuild.FoundedDate = FDateTime::Now();

	// Add founder as Guild Master
	FGuildMember Founder;
	Founder.PlayerID = FounderPlayerID;
	Founder.PlayerName = FounderName;
	Founder.RoleID = FName(TEXT("GuildMaster"));
	Founder.Status = EGuildMemberStatus::Active;
	Founder.JoinDate = FDateTime::Now();
	Founder.LastActiveDate = FDateTime::Now();
	NewGuild.Members.Add(Founder);

	// Store the guild
	FGuid GuildID = NewGuild.GuildID;
	Guilds.Add(GuildID, NewGuild);
	PlayerGuildMap.Add(FounderPlayerID, GuildID);

	// Log the creation
	AddAuditLogEntry(Guilds[GuildID], FounderPlayerID, FounderName,
		TEXT("GuildCreated"), FString::Printf(TEXT("Created guild '%s'"), *GuildName), TEXT(""));

	// Broadcast event
	OnGuildCreated.Broadcast(GuildID, GuildName, FounderPlayerID);

	UE_LOG(LogTemp, Log, TEXT("Guild '%s' created by %s"), *GuildName, *FounderPlayerID);

	return GuildID;
}

bool UOdysseyGuildManager::DisbandGuild(const FGuid& GuildID, const FString& RequestingPlayerID)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	// Check if requester has permission (must be founder or have DisbandGuild permission)
	if (Guild->FounderPlayerID != RequestingPlayerID)
	{
		if (!HasPermission(GuildID, RequestingPlayerID, EGuildPermission::DisbandGuild))
		{
			UE_LOG(LogTemp, Warning, TEXT("DisbandGuild: Player %s lacks permission"), *RequestingPlayerID);
			return false;
		}
	}

	FString GuildName = Guild->GuildName;

	// Remove all members from player-guild mapping
	for (const FGuildMember& Member : Guild->Members)
	{
		PlayerGuildMap.Remove(Member.PlayerID);
	}

	// Remove the guild
	Guilds.Remove(GuildID);

	// Broadcast event
	OnGuildDisbanded.Broadcast(GuildID, GuildName);

	UE_LOG(LogTemp, Log, TEXT("Guild '%s' disbanded by %s"), *GuildName, *RequestingPlayerID);

	return true;
}

bool UOdysseyGuildManager::GetGuildData(const FGuid& GuildID, FGuildData& OutGuildData) const
{
	FScopeLock Lock(&GuildLock);

	const FGuildData* Guild = Guilds.Find(GuildID);
	if (Guild)
	{
		OutGuildData = *Guild;
		return true;
	}
	return false;
}

FGuid UOdysseyGuildManager::FindGuildByName(const FString& GuildName) const
{
	FScopeLock Lock(&GuildLock);

	for (const auto& GuildPair : Guilds)
	{
		if (GuildPair.Value.GuildName.Equals(GuildName, ESearchCase::IgnoreCase))
		{
			return GuildPair.Key;
		}
	}
	return FGuid();
}

FGuid UOdysseyGuildManager::GetPlayerGuild(const FString& PlayerID) const
{
	FScopeLock Lock(&GuildLock);

	const FGuid* GuildID = PlayerGuildMap.Find(PlayerID);
	return GuildID ? *GuildID : FGuid();
}

bool UOdysseyGuildManager::IsPlayerInGuild(const FString& PlayerID) const
{
	FScopeLock Lock(&GuildLock);
	return PlayerGuildMap.Contains(PlayerID);
}

// ==================== Membership Management ====================

bool UOdysseyGuildManager::InvitePlayer(const FGuid& GuildID, const FString& InviterPlayerID,
	const FString& InvitedPlayerID, const FString& Message)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	// Check if inviter has permission
	if (!HasPermission(GuildID, InviterPlayerID, EGuildPermission::InviteMembers))
	{
		UE_LOG(LogTemp, Warning, TEXT("InvitePlayer: Player %s lacks InviteMembers permission"), *InviterPlayerID);
		return false;
	}

	// Check if invited player is already in a guild
	if (PlayerGuildMap.Contains(InvitedPlayerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("InvitePlayer: Player %s is already in a guild"), *InvitedPlayerID);
		return false;
	}

	// Check if there's already a pending invitation
	for (const FGuildInvitation& Invite : Guild->PendingInvitations)
	{
		if (Invite.InvitedPlayerID == InvitedPlayerID && !Invite.IsExpired())
		{
			UE_LOG(LogTemp, Warning, TEXT("InvitePlayer: Player %s already has pending invitation"), *InvitedPlayerID);
			return false;
		}
	}

	// Check guild capacity
	if (Guild->Members.Num() >= Guild->MaxMembers)
	{
		UE_LOG(LogTemp, Warning, TEXT("InvitePlayer: Guild %s is at max capacity"), *Guild->GuildName);
		return false;
	}

	// Create invitation
	FGuildInvitation Invitation;
	Invitation.GuildID = GuildID;
	Invitation.InvitedPlayerID = InvitedPlayerID;
	Invitation.InviterPlayerID = InviterPlayerID;
	Invitation.Message = Message;

	Guild->PendingInvitations.Add(Invitation);

	// Get inviter name for audit log
	const FGuildMember* Inviter = Guild->GetMember(InviterPlayerID);
	FString InviterName = Inviter ? Inviter->PlayerName : TEXT("Unknown");

	AddAuditLogEntry(*Guild, InviterPlayerID, InviterName,
		TEXT("PlayerInvited"), FString::Printf(TEXT("Invited player %s"), *InvitedPlayerID), InvitedPlayerID);

	return true;
}

bool UOdysseyGuildManager::AcceptInvitation(const FGuid& InvitationID, const FString& PlayerID, const FString& PlayerName)
{
	FScopeLock Lock(&GuildLock);

	// Find the invitation
	for (auto& GuildPair : Guilds)
	{
		FGuildData& Guild = GuildPair.Value;
		for (int32 i = Guild.PendingInvitations.Num() - 1; i >= 0; --i)
		{
			FGuildInvitation& Invite = Guild.PendingInvitations[i];
			if (Invite.InvitationID == InvitationID && Invite.InvitedPlayerID == PlayerID)
			{
				if (Invite.IsExpired())
				{
					Guild.PendingInvitations.RemoveAt(i);
					return false;
				}

				// Check if player is already in a guild
				if (PlayerGuildMap.Contains(PlayerID))
				{
					return false;
				}

				// Check guild capacity
				if (Guild.Members.Num() >= Guild.MaxMembers)
				{
					return false;
				}

				// Add member with default role
				const FGuildRole* DefaultRole = Guild.GetDefaultRole();
				FGuildMember NewMember;
				NewMember.PlayerID = PlayerID;
				NewMember.PlayerName = PlayerName;
				NewMember.RoleID = DefaultRole ? DefaultRole->RoleID : FName(TEXT("Member"));
				NewMember.Status = EGuildMemberStatus::Active;

				Guild.Members.Add(NewMember);
				PlayerGuildMap.Add(PlayerID, Guild.GuildID);

				// Remove invitation
				Guild.PendingInvitations.RemoveAt(i);

				AddAuditLogEntry(Guild, PlayerID, PlayerName,
					TEXT("MemberJoined"), TEXT("Accepted invitation and joined the guild"), TEXT(""));

				OnMemberJoined.Broadcast(Guild.GuildID, PlayerID, PlayerName);

				return true;
			}
		}
	}

	return false;
}

bool UOdysseyGuildManager::DeclineInvitation(const FGuid& InvitationID, const FString& PlayerID)
{
	FScopeLock Lock(&GuildLock);

	for (auto& GuildPair : Guilds)
	{
		FGuildData& Guild = GuildPair.Value;
		for (int32 i = Guild.PendingInvitations.Num() - 1; i >= 0; --i)
		{
			if (Guild.PendingInvitations[i].InvitationID == InvitationID &&
				Guild.PendingInvitations[i].InvitedPlayerID == PlayerID)
			{
				Guild.PendingInvitations.RemoveAt(i);
				return true;
			}
		}
	}

	return false;
}

TArray<FGuildInvitation> UOdysseyGuildManager::GetPlayerInvitations(const FString& PlayerID) const
{
	FScopeLock Lock(&GuildLock);

	TArray<FGuildInvitation> Invitations;
	for (const auto& GuildPair : Guilds)
	{
		for (const FGuildInvitation& Invite : GuildPair.Value.PendingInvitations)
		{
			if (Invite.InvitedPlayerID == PlayerID && !Invite.IsExpired())
			{
				Invitations.Add(Invite);
			}
		}
	}
	return Invitations;
}

bool UOdysseyGuildManager::LeaveGuild(const FString& PlayerID)
{
	FScopeLock Lock(&GuildLock);

	FGuid* GuildIDPtr = PlayerGuildMap.Find(PlayerID);
	if (!GuildIDPtr)
	{
		return false;
	}

	FGuildData* Guild = Guilds.Find(*GuildIDPtr);
	if (!Guild)
	{
		PlayerGuildMap.Remove(PlayerID);
		return false;
	}

	// Find and get member info
	FString PlayerName;
	FName PlayerRole;
	for (int32 i = Guild->Members.Num() - 1; i >= 0; --i)
	{
		if (Guild->Members[i].PlayerID == PlayerID)
		{
			PlayerName = Guild->Members[i].PlayerName;
			PlayerRole = Guild->Members[i].RoleID;

			// Check if this is the guild master/founder
			if (PlayerID == Guild->FounderPlayerID && Guild->Members.Num() > 1)
			{
				// Must transfer ownership first
				UE_LOG(LogTemp, Warning, TEXT("LeaveGuild: Founder must transfer ownership before leaving"));
				return false;
			}

			Guild->Members.RemoveAt(i);
			break;
		}
	}

	PlayerGuildMap.Remove(PlayerID);

	// If last member left, disband guild
	if (Guild->Members.Num() == 0)
	{
		FString GuildName = Guild->GuildName;
		FGuid GuildID = Guild->GuildID;
		Guilds.Remove(GuildID);
		OnGuildDisbanded.Broadcast(GuildID, GuildName);
	}
	else
	{
		AddAuditLogEntry(*Guild, PlayerID, PlayerName,
			TEXT("MemberLeft"), TEXT("Left the guild voluntarily"), TEXT(""));
	}

	OnMemberLeft.Broadcast(*GuildIDPtr, PlayerID, TEXT("Left voluntarily"));

	return true;
}

bool UOdysseyGuildManager::KickMember(const FGuid& GuildID, const FString& KickerPlayerID,
	const FString& TargetPlayerID, const FString& Reason)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	// Cannot kick yourself
	if (KickerPlayerID == TargetPlayerID)
	{
		return false;
	}

	// Check permission
	if (!HasPermission(GuildID, KickerPlayerID, EGuildPermission::KickMembers))
	{
		return false;
	}

	// Check if kicker outranks target
	if (!CanActOnMember(GuildID, KickerPlayerID, TargetPlayerID))
	{
		return false;
	}

	// Cannot kick the founder
	if (TargetPlayerID == Guild->FounderPlayerID)
	{
		return false;
	}

	// Find and remove target
	FString TargetName;
	for (int32 i = Guild->Members.Num() - 1; i >= 0; --i)
	{
		if (Guild->Members[i].PlayerID == TargetPlayerID)
		{
			TargetName = Guild->Members[i].PlayerName;
			Guild->Members.RemoveAt(i);
			break;
		}
	}

	if (TargetName.IsEmpty())
	{
		return false;
	}

	PlayerGuildMap.Remove(TargetPlayerID);

	const FGuildMember* Kicker = Guild->GetMember(KickerPlayerID);
	FString KickerName = Kicker ? Kicker->PlayerName : TEXT("Unknown");

	AddAuditLogEntry(*Guild, KickerPlayerID, KickerName,
		TEXT("MemberKicked"), FString::Printf(TEXT("Kicked %s. Reason: %s"), *TargetName, *Reason),
		TargetPlayerID);

	OnMemberLeft.Broadcast(GuildID, TargetPlayerID, FString::Printf(TEXT("Kicked: %s"), *Reason));

	return true;
}

bool UOdysseyGuildManager::SetMemberRole(const FGuid& GuildID, const FString& PromoterPlayerID,
	const FString& TargetPlayerID, FName NewRoleID)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	// Check permissions
	bool bPromoting = false;
	const FGuildRole* NewRole = Guild->GetRole(NewRoleID);
	if (!NewRole)
	{
		return false;
	}

	FGuildMember* Target = Guild->GetMember(TargetPlayerID);
	if (!Target)
	{
		return false;
	}

	const FGuildRole* CurrentRole = Guild->GetRole(Target->RoleID);
	if (CurrentRole)
	{
		bPromoting = NewRole->RankPriority > CurrentRole->RankPriority;
	}

	EGuildPermission RequiredPerm = bPromoting ? EGuildPermission::PromoteMembers : EGuildPermission::DemoteMembers;
	if (!HasPermission(GuildID, PromoterPlayerID, RequiredPerm))
	{
		return false;
	}

	// Check if promoter outranks target
	if (!CanActOnMember(GuildID, PromoterPlayerID, TargetPlayerID))
	{
		return false;
	}

	// Cannot assign a role equal or higher than your own (unless founder)
	const FGuildMember* Promoter = Guild->GetMember(PromoterPlayerID);
	if (Promoter && PromoterPlayerID != Guild->FounderPlayerID)
	{
		const FGuildRole* PromoterRole = Guild->GetRole(Promoter->RoleID);
		if (PromoterRole && NewRole->RankPriority >= PromoterRole->RankPriority)
		{
			return false;
		}
	}

	FName OldRoleID = Target->RoleID;
	Target->RoleID = NewRoleID;

	FString PromoterName = Promoter ? Promoter->PlayerName : TEXT("Unknown");
	FString ActionType = bPromoting ? TEXT("Promoted") : TEXT("Demoted");

	AddAuditLogEntry(*Guild, PromoterPlayerID, PromoterName,
		ActionType, FString::Printf(TEXT("%s %s to %s"), *ActionType, *Target->PlayerName, *NewRole->RoleName),
		TargetPlayerID);

	OnMemberPromoted.Broadcast(GuildID, TargetPlayerID, OldRoleID, NewRoleID);

	return true;
}

void UOdysseyGuildManager::UpdateMemberStatus(const FString& PlayerID, EGuildMemberStatus NewStatus)
{
	FScopeLock Lock(&GuildLock);

	FGuid* GuildIDPtr = PlayerGuildMap.Find(PlayerID);
	if (!GuildIDPtr)
	{
		return;
	}

	FGuildData* Guild = Guilds.Find(*GuildIDPtr);
	if (!Guild)
	{
		return;
	}

	FGuildMember* Member = Guild->GetMember(PlayerID);
	if (Member)
	{
		Member->Status = NewStatus;
		Member->LastActiveDate = FDateTime::Now();
	}
}

// ==================== Permission System ====================

bool UOdysseyGuildManager::HasPermission(const FGuid& GuildID, const FString& PlayerID,
	EGuildPermission Permission) const
{
	// Note: Lock should already be held by caller if needed
	const FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	const FGuildMember* Member = Guild->GetMember(PlayerID);
	if (!Member)
	{
		return false;
	}

	const FGuildRole* Role = Guild->GetRole(Member->RoleID);
	if (!Role)
	{
		return false;
	}

	return Role->HasPermission(Permission);
}

bool UOdysseyGuildManager::CanActOnMember(const FGuid& GuildID, const FString& ActorPlayerID,
	const FString& TargetPlayerID) const
{
	// Note: Lock should already be held by caller if needed
	const FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	// Founder can act on anyone
	if (ActorPlayerID == Guild->FounderPlayerID)
	{
		return true;
	}

	const FGuildMember* Actor = Guild->GetMember(ActorPlayerID);
	const FGuildMember* Target = Guild->GetMember(TargetPlayerID);
	if (!Actor || !Target)
	{
		return false;
	}

	const FGuildRole* ActorRole = Guild->GetRole(Actor->RoleID);
	const FGuildRole* TargetRole = Guild->GetRole(Target->RoleID);
	if (!ActorRole || !TargetRole)
	{
		return false;
	}

	return ActorRole->RankPriority > TargetRole->RankPriority;
}

// ==================== Role Management ====================

bool UOdysseyGuildManager::CreateRole(const FGuid& GuildID, const FString& CreatorPlayerID,
	const FGuildRole& NewRole)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, CreatorPlayerID, EGuildPermission::EditRoles))
	{
		return false;
	}

	// Check for duplicate role ID
	if (Guild->GetRole(NewRole.RoleID))
	{
		return false;
	}

	Guild->Roles.Add(NewRole);

	const FGuildMember* Creator = Guild->GetMember(CreatorPlayerID);
	AddAuditLogEntry(*Guild, CreatorPlayerID, Creator ? Creator->PlayerName : TEXT("Unknown"),
		TEXT("RoleCreated"), FString::Printf(TEXT("Created role '%s'"), *NewRole.RoleName), TEXT(""));

	return true;
}

bool UOdysseyGuildManager::ModifyRole(const FGuid& GuildID, const FString& ModifierPlayerID,
	FName RoleID, const FGuildRole& UpdatedRole)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, ModifierPlayerID, EGuildPermission::EditRoles))
	{
		return false;
	}

	// Cannot modify GuildMaster role unless you are the founder
	if (RoleID == FName(TEXT("GuildMaster")) && ModifierPlayerID != Guild->FounderPlayerID)
	{
		return false;
	}

	for (FGuildRole& Role : Guild->Roles)
	{
		if (Role.RoleID == RoleID)
		{
			FString OldName = Role.RoleName;
			Role = UpdatedRole;
			Role.RoleID = RoleID; // Preserve original ID

			const FGuildMember* Modifier = Guild->GetMember(ModifierPlayerID);
			AddAuditLogEntry(*Guild, ModifierPlayerID, Modifier ? Modifier->PlayerName : TEXT("Unknown"),
				TEXT("RoleModified"), FString::Printf(TEXT("Modified role '%s'"), *OldName), TEXT(""));

			return true;
		}
	}

	return false;
}

bool UOdysseyGuildManager::DeleteRole(const FGuid& GuildID, const FString& DeleterPlayerID, FName RoleID)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, DeleterPlayerID, EGuildPermission::EditRoles))
	{
		return false;
	}

	// Cannot delete built-in roles
	if (RoleID == FName(TEXT("GuildMaster")) || RoleID == FName(TEXT("Member")))
	{
		return false;
	}

	// Find and remove the role
	for (int32 i = Guild->Roles.Num() - 1; i >= 0; --i)
	{
		if (Guild->Roles[i].RoleID == RoleID)
		{
			FString RoleName = Guild->Roles[i].RoleName;

			// Reassign members with this role to default role
			const FGuildRole* DefaultRole = Guild->GetDefaultRole();
			FName DefaultRoleID = DefaultRole ? DefaultRole->RoleID : FName(TEXT("Member"));

			for (FGuildMember& Member : Guild->Members)
			{
				if (Member.RoleID == RoleID)
				{
					Member.RoleID = DefaultRoleID;
				}
			}

			Guild->Roles.RemoveAt(i);

			const FGuildMember* Deleter = Guild->GetMember(DeleterPlayerID);
			AddAuditLogEntry(*Guild, DeleterPlayerID, Deleter ? Deleter->PlayerName : TEXT("Unknown"),
				TEXT("RoleDeleted"), FString::Printf(TEXT("Deleted role '%s'"), *RoleName), TEXT(""));

			return true;
		}
	}

	return false;
}

TArray<FGuildRole> UOdysseyGuildManager::GetGuildRoles(const FGuid& GuildID) const
{
	FScopeLock Lock(&GuildLock);

	const FGuildData* Guild = Guilds.Find(GuildID);
	if (Guild)
	{
		return Guild->Roles;
	}
	return TArray<FGuildRole>();
}

// ==================== Guild Bank ====================

bool UOdysseyGuildManager::DepositToBank(const FGuid& GuildID, const FString& PlayerID,
	EResourceType ResourceType, int64 Amount)
{
	FScopeLock Lock(&GuildLock);

	if (Amount <= 0)
	{
		return false;
	}

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, PlayerID, EGuildPermission::DepositToBank))
	{
		return false;
	}

	// Add to bank
	if (!Guild->BankResources.Contains(ResourceType))
	{
		Guild->BankResources.Add(ResourceType, 0);
	}
	Guild->BankResources[ResourceType] += Amount;

	// Update member contribution
	FGuildMember* Member = Guild->GetMember(PlayerID);
	if (Member)
	{
		if (ResourceType == EResourceType::OMEN)
		{
			Member->TotalContribution += Amount;
		}
		Member->ContributionPoints += FMath::Max(1, static_cast<int32>(Amount / 100));
	}

	// Add experience to guild
	AddGuildExperience(GuildID, Amount / 10);

	const FGuildMember* Depositor = Guild->GetMember(PlayerID);
	AddAuditLogEntry(*Guild, PlayerID, Depositor ? Depositor->PlayerName : TEXT("Unknown"),
		TEXT("BankDeposit"), FString::Printf(TEXT("Deposited %lld of resource %d"), Amount, static_cast<int32>(ResourceType)),
		TEXT(""));

	OnGuildBankTransaction.Broadcast(GuildID, PlayerID, ResourceType, Amount);

	return true;
}

bool UOdysseyGuildManager::WithdrawFromBank(const FGuid& GuildID, const FString& PlayerID,
	EResourceType ResourceType, int64 Amount)
{
	FScopeLock Lock(&GuildLock);

	if (Amount <= 0)
	{
		return false;
	}

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, PlayerID, EGuildPermission::WithdrawFromBank))
	{
		return false;
	}

	// Check if resource exists and has enough
	int64* CurrentAmount = Guild->BankResources.Find(ResourceType);
	if (!CurrentAmount || *CurrentAmount < Amount)
	{
		return false;
	}

	// Check daily withdrawal limit for OMEN
	if (ResourceType == EResourceType::OMEN)
	{
		FGuildMember* Member = Guild->GetMember(PlayerID);
		if (Member)
		{
			const FGuildRole* Role = Guild->GetRole(Member->RoleID);
			if (Role && Role->MaxWithdrawalPerDay >= 0) // -1 = unlimited
			{
				int32 Remaining = Role->MaxWithdrawalPerDay - Member->WithdrawnToday;
				if (Amount > Remaining)
				{
					UE_LOG(LogTemp, Warning, TEXT("WithdrawFromBank: Exceeds daily limit. Remaining: %d"), Remaining);
					return false;
				}
				Member->WithdrawnToday += static_cast<int32>(Amount);
			}
		}
	}

	// Withdraw
	*CurrentAmount -= Amount;

	const FGuildMember* Withdrawer = Guild->GetMember(PlayerID);
	AddAuditLogEntry(*Guild, PlayerID, Withdrawer ? Withdrawer->PlayerName : TEXT("Unknown"),
		TEXT("BankWithdrawal"), FString::Printf(TEXT("Withdrew %lld of resource %d"), Amount, static_cast<int32>(ResourceType)),
		TEXT(""));

	OnGuildBankTransaction.Broadcast(GuildID, PlayerID, ResourceType, -Amount);

	return true;
}

int64 UOdysseyGuildManager::GetBankBalance(const FGuid& GuildID, EResourceType ResourceType) const
{
	FScopeLock Lock(&GuildLock);

	const FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return 0;
	}

	const int64* Amount = Guild->BankResources.Find(ResourceType);
	return Amount ? *Amount : 0;
}

TMap<EResourceType, int64> UOdysseyGuildManager::GetAllBankResources(const FGuid& GuildID) const
{
	FScopeLock Lock(&GuildLock);

	const FGuildData* Guild = Guilds.Find(GuildID);
	if (Guild)
	{
		return Guild->BankResources;
	}
	return TMap<EResourceType, int64>();
}

int32 UOdysseyGuildManager::GetRemainingWithdrawal(const FGuid& GuildID, const FString& PlayerID) const
{
	FScopeLock Lock(&GuildLock);

	const FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return 0;
	}

	const FGuildMember* Member = Guild->GetMember(PlayerID);
	if (!Member)
	{
		return 0;
	}

	const FGuildRole* Role = Guild->GetRole(Member->RoleID);
	if (!Role)
	{
		return 0;
	}

	if (Role->MaxWithdrawalPerDay < 0)
	{
		return INT32_MAX; // Unlimited
	}

	return FMath::Max(0, Role->MaxWithdrawalPerDay - Member->WithdrawnToday);
}

// ==================== Communication ====================

bool UOdysseyGuildManager::PostAnnouncement(const FGuid& GuildID, const FString& AuthorPlayerID,
	const FString& Title, const FString& Content, bool bPinned)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, AuthorPlayerID, EGuildPermission::SendGuildAnnouncements))
	{
		return false;
	}

	const FGuildMember* Author = Guild->GetMember(AuthorPlayerID);

	FGuildAnnouncement Announcement;
	Announcement.AuthorPlayerID = AuthorPlayerID;
	Announcement.AuthorName = Author ? Author->PlayerName : TEXT("Unknown");
	Announcement.Title = Title;
	Announcement.Content = Content;
	Announcement.bIsPinned = bPinned;

	Guild->Announcements.Add(Announcement);

	// Keep only last 100 non-pinned announcements
	int32 NonPinnedCount = 0;
	for (int32 i = Guild->Announcements.Num() - 1; i >= 0; --i)
	{
		if (!Guild->Announcements[i].bIsPinned)
		{
			NonPinnedCount++;
			if (NonPinnedCount > 100)
			{
				Guild->Announcements.RemoveAt(i);
			}
		}
	}

	return true;
}

bool UOdysseyGuildManager::DeleteAnnouncement(const FGuid& GuildID, const FString& DeleterPlayerID,
	const FGuid& AnnouncementID)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, DeleterPlayerID, EGuildPermission::SendGuildAnnouncements))
	{
		return false;
	}

	for (int32 i = Guild->Announcements.Num() - 1; i >= 0; --i)
	{
		if (Guild->Announcements[i].AnnouncementID == AnnouncementID)
		{
			Guild->Announcements.RemoveAt(i);
			return true;
		}
	}

	return false;
}

TArray<FGuildAnnouncement> UOdysseyGuildManager::GetAnnouncements(const FGuid& GuildID, int32 MaxCount) const
{
	FScopeLock Lock(&GuildLock);

	const FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return TArray<FGuildAnnouncement>();
	}

	TArray<FGuildAnnouncement> Result;

	// First add pinned announcements
	for (const FGuildAnnouncement& Ann : Guild->Announcements)
	{
		if (Ann.bIsPinned)
		{
			Result.Add(Ann);
		}
	}

	// Then add recent non-pinned
	for (int32 i = Guild->Announcements.Num() - 1; i >= 0 && Result.Num() < MaxCount; --i)
	{
		if (!Guild->Announcements[i].bIsPinned)
		{
			Result.Add(Guild->Announcements[i]);
		}
	}

	return Result;
}

bool UOdysseyGuildManager::SetMotD(const FGuid& GuildID, const FString& PlayerID, const FString& NewMotD)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, PlayerID, EGuildPermission::SendGuildAnnouncements))
	{
		return false;
	}

	Guild->MotD = NewMotD;

	const FGuildMember* Member = Guild->GetMember(PlayerID);
	AddAuditLogEntry(*Guild, PlayerID, Member ? Member->PlayerName : TEXT("Unknown"),
		TEXT("MotDChanged"), TEXT("Updated Message of the Day"), TEXT(""));

	return true;
}

// ==================== Guild Settings ====================

bool UOdysseyGuildManager::UpdateGuildSettings(const FGuid& GuildID, const FString& PlayerID,
	const FString& NewDescription, float NewTaxRate, bool bNewRecruiting, int32 NewMinLevel)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, PlayerID, EGuildPermission::ModifyGuildSettings))
	{
		return false;
	}

	Guild->Description = NewDescription;
	Guild->TaxRate = FMath::Clamp(NewTaxRate, 0.0f, 0.5f); // Max 50% tax
	Guild->bIsRecruiting = bNewRecruiting;
	Guild->MinLevelToJoin = FMath::Max(1, NewMinLevel);

	const FGuildMember* Member = Guild->GetMember(PlayerID);
	AddAuditLogEntry(*Guild, PlayerID, Member ? Member->PlayerName : TEXT("Unknown"),
		TEXT("SettingsChanged"), TEXT("Updated guild settings"), TEXT(""));

	return true;
}

bool UOdysseyGuildManager::SetTaxRate(const FGuid& GuildID, const FString& PlayerID, float NewTaxRate)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return false;
	}

	if (!HasPermission(GuildID, PlayerID, EGuildPermission::ManageTaxes))
	{
		return false;
	}

	float OldRate = Guild->TaxRate;
	Guild->TaxRate = FMath::Clamp(NewTaxRate, 0.0f, 0.5f);

	const FGuildMember* Member = Guild->GetMember(PlayerID);
	AddAuditLogEntry(*Guild, PlayerID, Member ? Member->PlayerName : TEXT("Unknown"),
		TEXT("TaxRateChanged"), FString::Printf(TEXT("Changed tax rate from %.1f%% to %.1f%%"),
			OldRate * 100.0f, Guild->TaxRate * 100.0f), TEXT(""));

	return true;
}

void UOdysseyGuildManager::AddGuildExperience(const FGuid& GuildID, int64 Experience)
{
	// Note: May be called without lock, so acquire it
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild || Experience <= 0)
	{
		return;
	}

	Guild->Experience += Experience;
	CheckGuildLevelUp(*Guild);
}

int64 UOdysseyGuildManager::GetExperienceForLevel(int32 Level) const
{
	// Exponential curve: 1000 * Level^2
	return static_cast<int64>(1000.0 * FMath::Pow(static_cast<float>(Level), 2.0f));
}

void UOdysseyGuildManager::CheckGuildLevelUp(FGuildData& Guild)
{
	while (true)
	{
		int64 RequiredExp = GetExperienceForLevel(Guild.Level + 1);
		if (Guild.Experience >= RequiredExp)
		{
			Guild.Level++;
			Guild.MaxMembers = 50 + (Guild.Level - 1) * 10; // +10 slots per level

			AddAuditLogEntry(Guild, TEXT("System"), TEXT("System"),
				TEXT("GuildLevelUp"), FString::Printf(TEXT("Guild leveled up to %d"), Guild.Level), TEXT(""));

			OnGuildLevelUp.Broadcast(Guild.GuildID, Guild.Level);
		}
		else
		{
			break;
		}
	}
}

// ==================== Diplomacy ====================

bool UOdysseyGuildManager::ProposeAlliance(const FGuid& ProposingGuildID, const FString& ProposerPlayerID,
	const FGuid& TargetGuildID)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* ProposingGuild = Guilds.Find(ProposingGuildID);
	FGuildData* TargetGuild = Guilds.Find(TargetGuildID);

	if (!ProposingGuild || !TargetGuild)
	{
		return false;
	}

	if (!HasPermission(ProposingGuildID, ProposerPlayerID, EGuildPermission::NegotiateAlliances))
	{
		return false;
	}

	// Check current relationship
	EGuildRelationship CurrentRel = GetGuildRelationship(ProposingGuildID, TargetGuildID);
	if (CurrentRel == EGuildRelationship::Allied || CurrentRel == EGuildRelationship::AtWar)
	{
		return false;
	}

	// Set to Friendly as alliance proposal pending
	SetGuildRelationship(*ProposingGuild, TargetGuildID, TargetGuild->GuildName, EGuildRelationship::Friendly);

	const FGuildMember* Proposer = ProposingGuild->GetMember(ProposerPlayerID);
	AddAuditLogEntry(*ProposingGuild, ProposerPlayerID, Proposer ? Proposer->PlayerName : TEXT("Unknown"),
		TEXT("AllianceProposed"), FString::Printf(TEXT("Proposed alliance with %s"), *TargetGuild->GuildName),
		TEXT(""));

	return true;
}

bool UOdysseyGuildManager::AcceptAlliance(const FGuid& AcceptingGuildID, const FString& AccepterPlayerID,
	const FGuid& ProposingGuildID)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* AcceptingGuild = Guilds.Find(AcceptingGuildID);
	FGuildData* ProposingGuild = Guilds.Find(ProposingGuildID);

	if (!AcceptingGuild || !ProposingGuild)
	{
		return false;
	}

	if (!HasPermission(AcceptingGuildID, AccepterPlayerID, EGuildPermission::NegotiateAlliances))
	{
		return false;
	}

	// Check that there's a pending friendly relationship from proposing guild
	EGuildRelationship CurrentRel = GetGuildRelationship(ProposingGuildID, AcceptingGuildID);
	if (CurrentRel != EGuildRelationship::Friendly)
	{
		return false;
	}

	EGuildRelationship OldRel = GetGuildRelationship(AcceptingGuildID, ProposingGuildID);

	// Set both to Allied
	SetGuildRelationship(*AcceptingGuild, ProposingGuildID, ProposingGuild->GuildName, EGuildRelationship::Allied);
	SetGuildRelationship(*ProposingGuild, AcceptingGuildID, AcceptingGuild->GuildName, EGuildRelationship::Allied);

	const FGuildMember* Accepter = AcceptingGuild->GetMember(AccepterPlayerID);
	AddAuditLogEntry(*AcceptingGuild, AccepterPlayerID, Accepter ? Accepter->PlayerName : TEXT("Unknown"),
		TEXT("AllianceAccepted"), FString::Printf(TEXT("Formed alliance with %s"), *ProposingGuild->GuildName),
		TEXT(""));

	OnGuildRelationshipChanged.Broadcast(AcceptingGuildID, ProposingGuildID, OldRel, EGuildRelationship::Allied);

	return true;
}

bool UOdysseyGuildManager::DeclareWar(const FGuid& DeclaringGuildID, const FString& DeclarerPlayerID,
	const FGuid& TargetGuildID)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* DeclaringGuild = Guilds.Find(DeclaringGuildID);
	FGuildData* TargetGuild = Guilds.Find(TargetGuildID);

	if (!DeclaringGuild || !TargetGuild)
	{
		return false;
	}

	if (!HasPermission(DeclaringGuildID, DeclarerPlayerID, EGuildPermission::DeclareWar))
	{
		return false;
	}

	EGuildRelationship OldRel = GetGuildRelationship(DeclaringGuildID, TargetGuildID);

	// Set both to AtWar
	SetGuildRelationship(*DeclaringGuild, TargetGuildID, TargetGuild->GuildName, EGuildRelationship::AtWar);
	SetGuildRelationship(*TargetGuild, DeclaringGuildID, DeclaringGuild->GuildName, EGuildRelationship::AtWar);

	const FGuildMember* Declarer = DeclaringGuild->GetMember(DeclarerPlayerID);
	AddAuditLogEntry(*DeclaringGuild, DeclarerPlayerID, Declarer ? Declarer->PlayerName : TEXT("Unknown"),
		TEXT("WarDeclared"), FString::Printf(TEXT("Declared war on %s"), *TargetGuild->GuildName),
		TEXT(""));

	OnGuildRelationshipChanged.Broadcast(DeclaringGuildID, TargetGuildID, OldRel, EGuildRelationship::AtWar);

	return true;
}

bool UOdysseyGuildManager::EndDiplomaticRelation(const FGuid& GuildID, const FString& PlayerID,
	const FGuid& OtherGuildID)
{
	FScopeLock Lock(&GuildLock);

	FGuildData* Guild = Guilds.Find(GuildID);
	FGuildData* OtherGuild = Guilds.Find(OtherGuildID);

	if (!Guild || !OtherGuild)
	{
		return false;
	}

	if (!HasPermission(GuildID, PlayerID, EGuildPermission::NegotiateAlliances))
	{
		return false;
	}

	EGuildRelationship OldRel = GetGuildRelationship(GuildID, OtherGuildID);

	// Set both to Neutral
	SetGuildRelationship(*Guild, OtherGuildID, OtherGuild->GuildName, EGuildRelationship::Neutral);
	SetGuildRelationship(*OtherGuild, GuildID, Guild->GuildName, EGuildRelationship::Neutral);

	const FGuildMember* Member = Guild->GetMember(PlayerID);
	AddAuditLogEntry(*Guild, PlayerID, Member ? Member->PlayerName : TEXT("Unknown"),
		TEXT("DiplomacyEnded"), FString::Printf(TEXT("Ended diplomatic relation with %s"), *OtherGuild->GuildName),
		TEXT(""));

	OnGuildRelationshipChanged.Broadcast(GuildID, OtherGuildID, OldRel, EGuildRelationship::Neutral);

	return true;
}

EGuildRelationship UOdysseyGuildManager::GetGuildRelationship(const FGuid& GuildID1, const FGuid& GuildID2) const
{
	// Note: Lock should be held by caller if needed
	const FGuildData* Guild1 = Guilds.Find(GuildID1);
	if (!Guild1)
	{
		return EGuildRelationship::Neutral;
	}

	for (const FGuildDiplomacy& Diplomacy : Guild1->Diplomacy)
	{
		if (Diplomacy.OtherGuildID == GuildID2)
		{
			return Diplomacy.Relationship;
		}
	}

	return EGuildRelationship::Neutral;
}

void UOdysseyGuildManager::SetGuildRelationship(FGuildData& Guild, const FGuid& OtherGuildID,
	const FString& OtherGuildName, EGuildRelationship NewRelationship)
{
	for (FGuildDiplomacy& Diplomacy : Guild.Diplomacy)
	{
		if (Diplomacy.OtherGuildID == OtherGuildID)
		{
			Diplomacy.Relationship = NewRelationship;
			Diplomacy.RelationshipStartDate = FDateTime::Now();
			return;
		}
	}

	// Add new diplomacy entry
	FGuildDiplomacy NewDiplomacy;
	NewDiplomacy.OtherGuildID = OtherGuildID;
	NewDiplomacy.OtherGuildName = OtherGuildName;
	NewDiplomacy.Relationship = NewRelationship;
	Guild.Diplomacy.Add(NewDiplomacy);
}

// ==================== Audit Log ====================

TArray<FGuildAuditLogEntry> UOdysseyGuildManager::GetAuditLog(const FGuid& GuildID, const FString& PlayerID,
	int32 MaxEntries) const
{
	FScopeLock Lock(&GuildLock);

	const FGuildData* Guild = Guilds.Find(GuildID);
	if (!Guild)
	{
		return TArray<FGuildAuditLogEntry>();
	}

	if (!HasPermission(GuildID, PlayerID, EGuildPermission::ViewAuditLog))
	{
		return TArray<FGuildAuditLogEntry>();
	}

	TArray<FGuildAuditLogEntry> Result;
	int32 StartIndex = FMath::Max(0, Guild->AuditLog.Num() - MaxEntries);
	for (int32 i = Guild->AuditLog.Num() - 1; i >= StartIndex; --i)
	{
		Result.Add(Guild->AuditLog[i]);
	}

	return Result;
}

void UOdysseyGuildManager::AddAuditLogEntry(FGuildData& Guild, const FString& ActorPlayerID,
	const FString& ActorName, const FString& Action, const FString& Details,
	const FString& TargetPlayerID)
{
	FGuildAuditLogEntry Entry;
	Entry.ActorPlayerID = ActorPlayerID;
	Entry.ActorName = ActorName;
	Entry.Action = Action;
	Entry.Details = Details;
	Entry.TargetPlayerID = TargetPlayerID;

	Guild.AuditLog.Add(Entry);

	// Keep only last 1000 entries
	while (Guild.AuditLog.Num() > 1000)
	{
		Guild.AuditLog.RemoveAt(0);
	}
}

// ==================== Search & Discovery ====================

TArray<FGuildData> UOdysseyGuildManager::SearchGuilds(const FString& SearchQuery, bool bRecruitingOnly,
	int32 MaxResults) const
{
	FScopeLock Lock(&GuildLock);

	TArray<FGuildData> Results;

	for (const auto& GuildPair : Guilds)
	{
		const FGuildData& Guild = GuildPair.Value;

		if (bRecruitingOnly && !Guild.bIsRecruiting)
		{
			continue;
		}

		// Match name, tag, or description
		if (Guild.GuildName.Contains(SearchQuery, ESearchCase::IgnoreCase) ||
			Guild.GuildTag.Contains(SearchQuery, ESearchCase::IgnoreCase) ||
			Guild.Description.Contains(SearchQuery, ESearchCase::IgnoreCase))
		{
			Results.Add(Guild);
			if (Results.Num() >= MaxResults)
			{
				break;
			}
		}
	}

	return Results;
}

TArray<FGuildData> UOdysseyGuildManager::GetTopGuilds(int32 Count) const
{
	FScopeLock Lock(&GuildLock);

	TArray<FGuildData> AllGuilds;
	for (const auto& GuildPair : Guilds)
	{
		AllGuilds.Add(GuildPair.Value);
	}

	// Sort by level descending, then by experience
	AllGuilds.Sort([](const FGuildData& A, const FGuildData& B) {
		if (A.Level != B.Level)
		{
			return A.Level > B.Level;
		}
		return A.Experience > B.Experience;
	});

	// Return top N
	TArray<FGuildData> Results;
	for (int32 i = 0; i < FMath::Min(Count, AllGuilds.Num()); ++i)
	{
		Results.Add(AllGuilds[i]);
	}

	return Results;
}

void UOdysseyGuildManager::ResetDailyWithdrawals()
{
	FScopeLock Lock(&GuildLock);

	for (auto& GuildPair : Guilds)
	{
		for (FGuildMember& Member : GuildPair.Value.Members)
		{
			Member.WithdrawnToday = 0;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Guild daily withdrawal limits reset"));
}
