// SocialSystemsIntegration.cpp
// Implementation of the social systems orchestrator

#include "SocialSystemsIntegration.h"
#include "OdysseyGuildManager.h"
#include "CooperativeProjectSystem.h"
#include "SocialContractSystem.h"
#include "ReputationSystem.h"
#include "GuildEconomyComponent.h"

USocialSystemsIntegration::USocialSystemsIntegration()
	: GuildManager(nullptr)
	, ProjectSystem(nullptr)
	, ContractSystem(nullptr)
	, ReputationSystem(nullptr)
	, GuildEconomy(nullptr)
	, bIsInitialized(false)
{
}

void USocialSystemsIntegration::InitializeAllSystems()
{
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("SocialSystemsIntegration: Already initialized"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("SocialSystemsIntegration: Initializing all social systems..."));

	// 1. Create and initialize ReputationSystem (no dependencies)
	ReputationSystem = NewObject<UReputationSystem>(this, TEXT("ReputationSystem"));
	ReputationSystem->Initialize();
	UE_LOG(LogTemp, Log, TEXT("  [1/5] ReputationSystem initialized"));

	// 2. Create and initialize GuildManager (no dependencies)
	GuildManager = NewObject<UOdysseyGuildManager>(this, TEXT("GuildManager"));
	GuildManager->Initialize();
	UE_LOG(LogTemp, Log, TEXT("  [2/5] GuildManager initialized"));

	// 3. Create and initialize GuildEconomyComponent (depends on GuildManager)
	GuildEconomy = NewObject<UGuildEconomyComponent>(this, TEXT("GuildEconomy"));
	GuildEconomy->Initialize(GuildManager);
	UE_LOG(LogTemp, Log, TEXT("  [3/5] GuildEconomyComponent initialized"));

	// 4. Create and initialize CooperativeProjectSystem (depends on GuildManager)
	ProjectSystem = NewObject<UCooperativeProjectSystem>(this, TEXT("ProjectSystem"));
	ProjectSystem->Initialize(GuildManager);
	UE_LOG(LogTemp, Log, TEXT("  [4/5] CooperativeProjectSystem initialized"));

	// 5. Create and initialize SocialContractSystem (depends on GuildManager, ReputationSystem)
	ContractSystem = NewObject<USocialContractSystem>(this, TEXT("ContractSystem"));
	ContractSystem->Initialize(GuildManager, ReputationSystem);
	UE_LOG(LogTemp, Log, TEXT("  [5/5] SocialContractSystem initialized"));

	// Wire cross-system events

	// Guild lifecycle -> GuildEconomy
	GuildManager->OnGuildCreated.AddDynamic(this, &USocialSystemsIntegration::HandleGuildCreated);
	GuildManager->OnGuildDisbanded.AddDynamic(this, &USocialSystemsIntegration::HandleGuildDisbanded);
	GuildManager->OnMemberJoined.AddDynamic(this, &USocialSystemsIntegration::HandleMemberJoined);
	GuildManager->OnMemberLeft.AddDynamic(this, &USocialSystemsIntegration::HandleMemberLeft);
	GuildManager->OnGuildLevelUp.AddDynamic(this, &USocialSystemsIntegration::HandleGuildLevelUp);

	// Contract completion -> ReputationSystem
	ContractSystem->OnContractCompleted.AddDynamic(this, &USocialSystemsIntegration::HandleContractCompleted);

	// Project events -> ReputationSystem
	ProjectSystem->OnContributionMade.AddDynamic(this, &USocialSystemsIntegration::HandleProjectContribution);
	ProjectSystem->OnProjectCompleted.AddDynamic(this, &USocialSystemsIntegration::HandleProjectCompleted);

	bIsInitialized = true;
	UE_LOG(LogTemp, Log, TEXT("SocialSystemsIntegration: All systems initialized and events wired."));
}

void USocialSystemsIntegration::ShutdownAllSystems()
{
	if (!bIsInitialized) return;

	UE_LOG(LogTemp, Log, TEXT("SocialSystemsIntegration: Shutting down all social systems..."));

	// Unbind events
	if (GuildManager)
	{
		GuildManager->OnGuildCreated.RemoveAll(this);
		GuildManager->OnGuildDisbanded.RemoveAll(this);
		GuildManager->OnMemberJoined.RemoveAll(this);
		GuildManager->OnMemberLeft.RemoveAll(this);
		GuildManager->OnGuildLevelUp.RemoveAll(this);
	}

	if (ContractSystem)
	{
		ContractSystem->OnContractCompleted.RemoveAll(this);
	}

	if (ProjectSystem)
	{
		ProjectSystem->OnContributionMade.RemoveAll(this);
		ProjectSystem->OnProjectCompleted.RemoveAll(this);
	}

	bIsInitialized = false;

	UE_LOG(LogTemp, Log, TEXT("SocialSystemsIntegration: Shutdown complete."));
}

// ==================== Player Lifecycle ====================

void USocialSystemsIntegration::OnPlayerJoined(const FString& PlayerID, const FString& PlayerName)
{
	if (!bIsInitialized) return;

	// Ensure player has a reputation profile
	if (ReputationSystem)
	{
		ReputationSystem->EnsurePlayerProfile(PlayerID, PlayerName);
	}

	// Update guild member status if in a guild
	if (GuildManager)
	{
		GuildManager->UpdateMemberStatus(PlayerID, EGuildMemberStatus::Active);
	}

	UE_LOG(LogTemp, Log, TEXT("Social systems: Player %s (%s) joined"), *PlayerID, *PlayerName);
}

void USocialSystemsIntegration::OnPlayerLeft(const FString& PlayerID)
{
	if (!bIsInitialized) return;

	// Update guild member status
	if (GuildManager)
	{
		GuildManager->UpdateMemberStatus(PlayerID, EGuildMemberStatus::Inactive);
	}

	UE_LOG(LogTemp, Log, TEXT("Social systems: Player %s left"), *PlayerID);
}

// ==================== Event Handlers ====================

void USocialSystemsIntegration::HandleGuildCreated(const FGuid& GuildID, const FString& GuildName, const FString& FounderID)
{
	// Initialize guild economy
	if (GuildEconomy)
	{
		GuildEconomy->InitializeGuildEconomy(GuildID);
		GuildEconomy->RegisterMember(GuildID, FounderID, TEXT(""));
	}

	// Grant reputation for founding a guild
	if (ReputationSystem)
	{
		ReputationSystem->RecordGuildContribution(FounderID);
	}

	UE_LOG(LogTemp, Log, TEXT("Cross-system: Guild '%s' created -> Economy initialized"), *GuildName);
}

void USocialSystemsIntegration::HandleGuildDisbanded(const FGuid& GuildID, const FString& GuildName)
{
	// Clean up guild economy
	if (GuildEconomy)
	{
		GuildEconomy->RemoveGuildEconomy(GuildID);
	}

	UE_LOG(LogTemp, Log, TEXT("Cross-system: Guild '%s' disbanded -> Economy removed"), *GuildName);
}

void USocialSystemsIntegration::HandleMemberJoined(const FGuid& GuildID, const FString& PlayerID, const FString& PlayerName)
{
	// Register member in economy system
	if (GuildEconomy)
	{
		GuildEconomy->RegisterMember(GuildID, PlayerID, PlayerName);
	}

	UE_LOG(LogTemp, Log, TEXT("Cross-system: %s joined guild -> Registered in economy"), *PlayerName);
}

void USocialSystemsIntegration::HandleMemberLeft(const FGuid& GuildID, const FString& PlayerID, const FString& Reason)
{
	// Unregister member from economy system
	if (GuildEconomy)
	{
		GuildEconomy->UnregisterMember(GuildID, PlayerID);
	}

	UE_LOG(LogTemp, Log, TEXT("Cross-system: %s left guild (%s) -> Unregistered from economy"), *PlayerID, *Reason);
}

void USocialSystemsIntegration::HandleContractCompleted(const FGuid& ContractID, const FString& ClientID, const FString& ContractorID)
{
	// Update reputation for both parties
	if (ReputationSystem)
	{
		ReputationSystem->RecordContractOutcome(ContractorID, true, 4.0f); // Default good rating
		ReputationSystem->RecordTradeOutcome(ClientID);

		// Grant faction reputation for completing contracts
		ReputationSystem->ModifyReputation(ContractorID, EFaction::VoidTraders, 5.0f,
			EReputationChangeSource::ContractCompletion,
			FString::Printf(TEXT("Contract %s completed"), *ContractID.ToString()));
	}

	// Update guild economy trade count if contractor is in a guild
	if (GuildManager && GuildEconomy)
	{
		FGuid ContractorGuild = GuildManager->GetPlayerGuild(ContractorID);
		if (ContractorGuild.IsValid())
		{
			GuildEconomy->IncrementGoalTradeCount(ContractorGuild);
		}
	}
}

void USocialSystemsIntegration::HandleProjectContribution(const FGuid& ProjectID, const FString& PlayerID,
	EResourceType ResourceType, int64 Amount)
{
	// Record guild contribution for reputation
	if (ReputationSystem)
	{
		ReputationSystem->RecordGuildContribution(PlayerID);

		// Small faction reputation boost for cooperative work
		ReputationSystem->ModifyReputation(PlayerID, EFaction::MinerGuild, 1.0f,
			EReputationChangeSource::GuildActivity,
			TEXT("Cooperative project contribution"));
	}
}

void USocialSystemsIntegration::HandleProjectCompleted(const FGuid& ProjectID, const FString& ProjectName)
{
	UE_LOG(LogTemp, Log, TEXT("Cross-system: Project '%s' completed -> Rewards processing"), *ProjectName);

	// Additional cross-system rewards could be handled here
	// e.g., faction reputation for the entire guild
}

void USocialSystemsIntegration::HandleGuildLevelUp(const FGuid& GuildID, int32 NewLevel)
{
	UE_LOG(LogTemp, Log, TEXT("Cross-system: Guild leveled up to %d -> Max facilities: %d"),
		NewLevel, 3 + (NewLevel / 2));

	// Guild level ups can unlock new facility slots, project templates, etc.
	// The GuildEconomy already queries guild level dynamically
}
