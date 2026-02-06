// SocialSystemsIntegration.h
// Integration layer connecting all Guild & Social Cooperation Systems
// Orchestrates initialization, cross-system events, and lifecycle management

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OdysseyInventoryComponent.h"
#include "SocialSystemsIntegration.generated.h"

// Forward declarations
class UOdysseyGuildManager;
class UCooperativeProjectSystem;
class USocialContractSystem;
class UReputationSystem;
class UGuildEconomyComponent;

/**
 * USocialSystemsIntegration
 *
 * Top-level orchestrator for all social/cooperation systems.
 * Handles initialization order, cross-system event wiring,
 * and provides a single entry point for the game mode.
 *
 * Initialization order:
 *   1. ReputationSystem (no dependencies)
 *   2. GuildManager (no dependencies)
 *   3. GuildEconomyComponent (depends on GuildManager)
 *   4. CooperativeProjectSystem (depends on GuildManager)
 *   5. SocialContractSystem (depends on GuildManager, ReputationSystem)
 *
 * Cross-system event wiring:
 *   - Guild creation -> GuildEconomy::InitializeGuildEconomy
 *   - Guild disband  -> GuildEconomy::RemoveGuildEconomy
 *   - Member join    -> GuildEconomy::RegisterMember
 *   - Member leave   -> GuildEconomy::UnregisterMember
 *   - Contract complete -> ReputationSystem::RecordContractOutcome
 *   - Project contribution -> ReputationSystem::RecordGuildContribution
 *   - Trade complete -> ReputationSystem::RecordTradeOutcome
 *   - Guild level up -> GuildEconomy max facilities update
 */
UCLASS(Blueprintable, BlueprintType)
class ODYSSEY_API USocialSystemsIntegration : public UObject
{
	GENERATED_BODY()

public:
	USocialSystemsIntegration();

	/**
	 * Initialize all social systems in correct order and wire events.
	 * Call this from your GameMode or GameInstance during startup.
	 */
	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	void InitializeAllSystems();

	/**
	 * Shutdown all social systems cleanly.
	 */
	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	void ShutdownAllSystems();

	// ==================== System Accessors ====================

	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	UOdysseyGuildManager* GetGuildManager() const { return GuildManager; }

	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	UCooperativeProjectSystem* GetProjectSystem() const { return ProjectSystem; }

	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	USocialContractSystem* GetContractSystem() const { return ContractSystem; }

	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	UReputationSystem* GetReputationSystem() const { return ReputationSystem; }

	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	UGuildEconomyComponent* GetGuildEconomy() const { return GuildEconomy; }

	// ==================== Player Lifecycle ====================

	/** Register a new player across all social systems */
	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	void OnPlayerJoined(const FString& PlayerID, const FString& PlayerName);

	/** Handle player going offline */
	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	void OnPlayerLeft(const FString& PlayerID);

	// ==================== Status ====================

	UFUNCTION(BlueprintCallable, Category = "Social Systems")
	bool IsInitialized() const { return bIsInitialized; }

protected:
	UPROPERTY()
	UOdysseyGuildManager* GuildManager;

	UPROPERTY()
	UCooperativeProjectSystem* ProjectSystem;

	UPROPERTY()
	USocialContractSystem* ContractSystem;

	UPROPERTY()
	UReputationSystem* ReputationSystem;

	UPROPERTY()
	UGuildEconomyComponent* GuildEconomy;

	bool bIsInitialized;

	// Event handlers for cross-system wiring
	UFUNCTION()
	void HandleGuildCreated(const FGuid& GuildID, const FString& GuildName, const FString& FounderID);

	UFUNCTION()
	void HandleGuildDisbanded(const FGuid& GuildID, const FString& GuildName);

	UFUNCTION()
	void HandleMemberJoined(const FGuid& GuildID, const FString& PlayerID, const FString& PlayerName);

	UFUNCTION()
	void HandleMemberLeft(const FGuid& GuildID, const FString& PlayerID, const FString& Reason);

	UFUNCTION()
	void HandleContractCompleted(const FGuid& ContractID, const FString& ClientID, const FString& ContractorID);

	UFUNCTION()
	void HandleProjectContribution(const FGuid& ProjectID, const FString& PlayerID, EResourceType ResourceType, int64 Amount);

	UFUNCTION()
	void HandleProjectCompleted(const FGuid& ProjectID, const FString& ProjectName);

	UFUNCTION()
	void HandleGuildLevelUp(const FGuid& GuildID, int32 NewLevel);
};
