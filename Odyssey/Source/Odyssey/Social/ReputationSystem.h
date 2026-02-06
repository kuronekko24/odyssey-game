// ReputationSystem.h
// Faction and player reputation tracking for Odyssey
// Integrates with NPC behavior, trade pricing, guild diplomacy, and social contracts

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "ReputationSystem.generated.h"

// Forward declarations
class UOdysseyGuildManager;
class USocialContractSystem;

// ============================================================================
// ENUMERATIONS
// ============================================================================

/**
 * Known factions in the Odyssey universe
 */
UENUM(BlueprintType)
enum class EFaction : uint8
{
	None = 0                    UMETA(DisplayName = "None"),
	Concordat,                  // Central governing body
	VoidTraders,                // Merchant coalition
	IronVanguard,               // Military / mercenary faction
	StellarAcademy,             // Research / science faction
	FreeHaven,                  // Independent frontier settlers
	ShadowSyndicate,            // Underworld / black market
	AncientOrder,               // Mysterious precursor cult
	MinerGuild,                 // Industrial mining cooperative
	NomadFleet,                 // Spacefaring nomads
	Custom = 200                UMETA(DisplayName = "Custom Faction")
};

/**
 * Reputation tier thresholds dictating NPC behavior and access
 */
UENUM(BlueprintType)
enum class EReputationTier : uint8
{
	Reviled = 0,        // -1000 to -750  : KOS, no services
	Hostile,            // -749  to -500  : Attacked on sight near territory
	Unfriendly,         // -499  to -250  : Restricted services, higher prices
	Wary,               // -249  to  -50  : Limited interaction, slight penalties
	Neutral,            //  -49  to   49  : Default, normal interaction
	Amiable,            //   50  to  249  : Minor discounts, extra dialogue
	Friendly,           //  250  to  499  : Access to faction missions, discounts
	Honored,            //  500  to  749  : Access to faction vendors, significant perks
	Exalted             //  750  to 1000  : Maximum standing, unique rewards
};

/**
 * Type of reputation change source
 */
UENUM(BlueprintType)
enum class EReputationChangeSource : uint8
{
	QuestCompletion,
	QuestFailure,
	CombatKill,
	TradeTransaction,
	ContractCompletion,
	ContractFailure,
	GuildActivity,
	DiplomaticAction,
	CrimeCommitted,
	Donation,
	Discovery,
	PlayerReport,
	SystemDecay,
	AdminAction,
	Custom
};

/**
 * Type of player-to-player reputation feedback
 */
UENUM(BlueprintType)
enum class EPlayerFeedbackType : uint8
{
	Positive,
	Neutral,
	Negative
};

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Standing with a specific faction
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FFactionStanding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Standing")
	EFaction Faction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Standing")
	float CurrentReputation; // -1000 to 1000

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Standing")
	EReputationTier CurrentTier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Standing")
	float LifetimePositive; // Total positive reputation earned ever

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Standing")
	float LifetimeNegative; // Total negative reputation earned ever

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Standing")
	FDateTime LastChangeTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Standing")
	bool bIsLocked; // Reputation frozen by quest/event

	FFactionStanding()
		: Faction(EFaction::None)
		, CurrentReputation(0.0f)
		, CurrentTier(EReputationTier::Neutral)
		, LifetimePositive(0.0f)
		, LifetimeNegative(0.0f)
		, bIsLocked(false)
	{
		LastChangeTime = FDateTime::Now();
	}

	FFactionStanding(EFaction InFaction, float InReputation = 0.0f)
		: Faction(InFaction)
		, CurrentReputation(InReputation)
		, CurrentTier(EReputationTier::Neutral)
		, LifetimePositive(0.0f)
		, LifetimeNegative(0.0f)
		, bIsLocked(false)
	{
		LastChangeTime = FDateTime::Now();
		RecalculateTier();
	}

	void RecalculateTier()
	{
		if (CurrentReputation >= 750.0f)       CurrentTier = EReputationTier::Exalted;
		else if (CurrentReputation >= 500.0f)  CurrentTier = EReputationTier::Honored;
		else if (CurrentReputation >= 250.0f)  CurrentTier = EReputationTier::Friendly;
		else if (CurrentReputation >= 50.0f)   CurrentTier = EReputationTier::Amiable;
		else if (CurrentReputation >= -49.0f)  CurrentTier = EReputationTier::Neutral;
		else if (CurrentReputation >= -249.0f) CurrentTier = EReputationTier::Wary;
		else if (CurrentReputation >= -499.0f) CurrentTier = EReputationTier::Unfriendly;
		else if (CurrentReputation >= -749.0f) CurrentTier = EReputationTier::Hostile;
		else                                   CurrentTier = EReputationTier::Reviled;
	}

	float GetProgressToNextTier() const
	{
		float LowBound = 0.0f;
		float HighBound = 0.0f;

		switch (CurrentTier)
		{
		case EReputationTier::Reviled:     LowBound = -1000.0f; HighBound = -750.0f; break;
		case EReputationTier::Hostile:     LowBound = -750.0f;  HighBound = -500.0f; break;
		case EReputationTier::Unfriendly:  LowBound = -500.0f;  HighBound = -250.0f; break;
		case EReputationTier::Wary:        LowBound = -250.0f;  HighBound = -50.0f;  break;
		case EReputationTier::Neutral:     LowBound = -50.0f;   HighBound = 50.0f;   break;
		case EReputationTier::Amiable:     LowBound = 50.0f;    HighBound = 250.0f;  break;
		case EReputationTier::Friendly:    LowBound = 250.0f;   HighBound = 500.0f;  break;
		case EReputationTier::Honored:     LowBound = 500.0f;   HighBound = 750.0f;  break;
		case EReputationTier::Exalted:     LowBound = 750.0f;   HighBound = 1000.0f; break;
		}

		float Range = HighBound - LowBound;
		if (Range <= 0.0f) return 1.0f;
		return FMath::Clamp((CurrentReputation - LowBound) / Range, 0.0f, 1.0f);
	}
};

/**
 * Record of a single reputation change
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FReputationChangeRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	FGuid RecordID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	FString PlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	EFaction Faction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	float Amount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	float NewTotal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	EReputationTier OldTier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	EReputationTier NewTier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	EReputationChangeSource Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Change")
	FDateTime Timestamp;

	FReputationChangeRecord()
		: PlayerID(TEXT(""))
		, Faction(EFaction::None)
		, Amount(0.0f)
		, NewTotal(0.0f)
		, OldTier(EReputationTier::Neutral)
		, NewTier(EReputationTier::Neutral)
		, Source(EReputationChangeSource::Custom)
		, Description(TEXT(""))
	{
		RecordID = FGuid::NewGuid();
		Timestamp = FDateTime::Now();
	}
};

/**
 * Player-to-player trust/feedback record
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlayerFeedback
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Feedback")
	FGuid FeedbackID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Feedback")
	FString FromPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Feedback")
	FString ToPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Feedback")
	EPlayerFeedbackType FeedbackType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Feedback")
	FString Context; // e.g. "Contract #xyz", "Trade", "Combat"

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Feedback")
	FDateTime Timestamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Feedback")
	float Weight; // Weighted by reporter credibility

	FPlayerFeedback()
		: FromPlayerID(TEXT(""))
		, ToPlayerID(TEXT(""))
		, FeedbackType(EPlayerFeedbackType::Neutral)
		, Context(TEXT(""))
		, Weight(1.0f)
	{
		FeedbackID = FGuid::NewGuid();
		Timestamp = FDateTime::Now();
	}
};

/**
 * Aggregate player reputation data (social standing among other players)
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlayerSocialReputation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	FString PlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	float TrustScore; // 0.0 to 100.0

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	int32 PositiveCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	int32 NeutralCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	int32 NegativeCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	int32 ContractsCompleted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	int32 ContractsFailed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	int32 TradesCompleted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	float AverageContractRating; // 1.0 - 5.0

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	int32 GuildContributions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Social Reputation")
	FDateTime LastUpdated;

	FPlayerSocialReputation()
		: PlayerID(TEXT(""))
		, TrustScore(50.0f)
		, PositiveCount(0)
		, NeutralCount(0)
		, NegativeCount(0)
		, ContractsCompleted(0)
		, ContractsFailed(0)
		, TradesCompleted(0)
		, AverageContractRating(0.0f)
		, GuildContributions(0)
	{
		LastUpdated = FDateTime::Now();
	}

	float GetReliabilityScore() const
	{
		int32 TotalFeedback = PositiveCount + NeutralCount + NegativeCount;
		if (TotalFeedback == 0) return 50.0f;

		float PositiveWeight = static_cast<float>(PositiveCount) * 1.0f;
		float NeutralWeight = static_cast<float>(NeutralCount) * 0.5f;
		float NegativeWeight = static_cast<float>(NegativeCount) * 0.0f;

		return ((PositiveWeight + NeutralWeight + NegativeWeight) /
			static_cast<float>(TotalFeedback)) * 100.0f;
	}

	float GetCompletionRate() const
	{
		int32 Total = ContractsCompleted + ContractsFailed;
		if (Total == 0) return 1.0f;
		return static_cast<float>(ContractsCompleted) / static_cast<float>(Total);
	}
};

/**
 * Faction relationship definition (how factions feel about each other)
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FFactionRelationship
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Relationship")
	EFaction FactionA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Relationship")
	EFaction FactionB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Relationship")
	float BaseRelationship; // -100 to 100

	// Ripple effect: gaining rep with A changes rep with B by this multiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction Relationship")
	float RippleMultiplier;

	FFactionRelationship()
		: FactionA(EFaction::None)
		, FactionB(EFaction::None)
		, BaseRelationship(0.0f)
		, RippleMultiplier(0.0f)
	{
	}

	FFactionRelationship(EFaction A, EFaction B, float Relationship, float Ripple)
		: FactionA(A)
		, FactionB(B)
		, BaseRelationship(Relationship)
		, RippleMultiplier(Ripple)
	{
	}
};

/**
 * Faction definition with metadata
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FFactionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EFaction FactionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString FactionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	FLinearColor FactionColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	float DefaultReputation; // Starting reputation for new players

	// Trade discount per tier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	TMap<EReputationTier, float> TierTradeDiscounts;

	// Access requirements for faction services
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EReputationTier MinTierForMissions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EReputationTier MinTierForVendor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EReputationTier MinTierForHangar;

	// Reputation decay: how fast reputation returns to default
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	float DecayRatePerDay; // Points per real day toward default

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	bool bDecayEnabled;

	FFactionDefinition()
		: FactionID(EFaction::None)
		, FactionName(TEXT("Unknown"))
		, Description(TEXT(""))
		, FactionColor(FLinearColor::White)
		, DefaultReputation(0.0f)
		, MinTierForMissions(EReputationTier::Amiable)
		, MinTierForVendor(EReputationTier::Friendly)
		, MinTierForHangar(EReputationTier::Honored)
		, DecayRatePerDay(1.0f)
		, bDecayEnabled(true)
	{
	}
};

/**
 * Complete reputation profile for one player
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FPlayerReputationProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	FString PlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	FString PlayerName;

	// Faction standings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	TMap<EFaction, FFactionStanding> FactionStandings;

	// Social reputation among other players
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	FPlayerSocialReputation SocialReputation;

	// Recent change history (ring buffer, newest first)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	TArray<FReputationChangeRecord> RecentChanges;

	// Player feedback received
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	TArray<FPlayerFeedback> ReceivedFeedback;

	// Titles/achievements unlocked via reputation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	TArray<FName> UnlockedTitles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	FName ActiveTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	FDateTime ProfileCreated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation Profile")
	FDateTime LastDecayProcessed;

	FPlayerReputationProfile()
		: PlayerID(TEXT(""))
		, PlayerName(TEXT(""))
		, ActiveTitle(NAME_None)
	{
		ProfileCreated = FDateTime::Now();
		LastDecayProcessed = FDateTime::Now();
	}

	FFactionStanding* GetStanding(EFaction Faction)
	{
		return FactionStandings.Find(Faction);
	}

	const FFactionStanding* GetStanding(EFaction Faction) const
	{
		return FactionStandings.Find(Faction);
	}

	EReputationTier GetTier(EFaction Faction) const
	{
		const FFactionStanding* Standing = FactionStandings.Find(Faction);
		if (Standing) return Standing->CurrentTier;
		return EReputationTier::Neutral;
	}

	float GetReputation(EFaction Faction) const
	{
		const FFactionStanding* Standing = FactionStandings.Find(Faction);
		if (Standing) return Standing->CurrentReputation;
		return 0.0f;
	}
};

// ============================================================================
// DELEGATES
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FOnReputationChanged,
	const FString&, PlayerID,
	EFaction, Faction,
	float, OldReputation,
	float, NewReputation,
	EReputationChangeSource, Source);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnReputationTierChanged,
	const FString&, PlayerID,
	EFaction, Faction,
	EReputationTier, OldTier,
	EReputationTier, NewTier);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnPlayerTrustScoreChanged,
	const FString&, PlayerID,
	float, OldScore,
	float, NewScore);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnTitleUnlocked,
	const FString&, PlayerID,
	FName, TitleID,
	EFaction, Faction);

// ============================================================================
// MAIN SYSTEM CLASS
// ============================================================================

/**
 * UReputationSystem
 *
 * Central system for tracking and managing player reputation with factions
 * and between players. Integrates with NPC behavior, trade systems,
 * contracts, and guild diplomacy.
 *
 * Key features:
 * - Faction reputation with tier-based access and rewards
 * - Cross-faction ripple effects (helping one faction may hurt another)
 * - Player-to-player trust scores based on contract and trade history
 * - Reputation decay toward default over time
 * - Title/achievement unlocks at reputation milestones
 * - Trade price modifiers based on standing
 */
UCLASS(Blueprintable, BlueprintType)
class ODYSSEY_API UReputationSystem : public UObject
{
	GENERATED_BODY()

public:
	UReputationSystem();

	/** Initialize the reputation system with faction definitions */
	UFUNCTION(BlueprintCallable, Category = "Reputation")
	void Initialize();

	// ==================== Faction Reputation ====================

	/** Add or subtract reputation with a faction for a player */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	float ModifyReputation(const FString& PlayerID, EFaction Faction, float Amount,
		EReputationChangeSource Source, const FString& Description = TEXT(""));

	/** Set reputation to an exact value (admin/debug) */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	void SetReputation(const FString& PlayerID, EFaction Faction, float NewValue);

	/** Get current reputation value */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	float GetReputation(const FString& PlayerID, EFaction Faction) const;

	/** Get current reputation tier */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	EReputationTier GetReputationTier(const FString& PlayerID, EFaction Faction) const;

	/** Get all faction standings for a player */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	TMap<EFaction, FFactionStanding> GetAllStandings(const FString& PlayerID) const;

	/** Get the full reputation profile */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	bool GetReputationProfile(const FString& PlayerID, FPlayerReputationProfile& OutProfile) const;

	/** Get progress toward next tier (0.0 - 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	float GetTierProgress(const FString& PlayerID, EFaction Faction) const;

	/** Lock/unlock reputation changes for a faction (quest/event use) */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	void SetReputationLocked(const FString& PlayerID, EFaction Faction, bool bLocked);

	/** Check if player meets minimum tier for a faction service */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	bool MeetsTierRequirement(const FString& PlayerID, EFaction Faction, EReputationTier RequiredTier) const;

	// ==================== Trade Price Modifiers ====================

	/** Get trade price discount/penalty for a player with a faction (-1.0 to 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Trade")
	float GetTradeModifier(const FString& PlayerID, EFaction Faction) const;

	/** Get combined trade modifier considering guild reputation */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Trade")
	float GetCombinedTradeModifier(const FString& PlayerID, EFaction Faction) const;

	// ==================== Player Social Reputation ====================

	/** Submit feedback about another player */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Social")
	bool SubmitPlayerFeedback(const FString& FromPlayerID, const FString& ToPlayerID,
		EPlayerFeedbackType FeedbackType, const FString& Context);

	/** Record a completed contract for reputation tracking */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Social")
	void RecordContractOutcome(const FString& PlayerID, bool bCompleted, float Rating);

	/** Record a completed trade for reputation tracking */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Social")
	void RecordTradeOutcome(const FString& PlayerID);

	/** Record a guild contribution */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Social")
	void RecordGuildContribution(const FString& PlayerID);

	/** Get player trust score (0-100) */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Social")
	float GetPlayerTrustScore(const FString& PlayerID) const;

	/** Get full social reputation data */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Social")
	bool GetPlayerSocialReputation(const FString& PlayerID, FPlayerSocialReputation& OutRep) const;

	/** Get feedback history for a player */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Social")
	TArray<FPlayerFeedback> GetPlayerFeedbackHistory(const FString& PlayerID, int32 MaxCount = 20) const;

	// ==================== Cross-Faction Ripple Effects ====================

	/** Get the ripple multiplier between two factions */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	float GetFactionRippleMultiplier(EFaction SourceFaction, EFaction TargetFaction) const;

	/** Check if two factions are allies (positive base relationship) */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	bool AreFactionAllies(EFaction FactionA, EFaction FactionB) const;

	/** Check if two factions are enemies (negative base relationship) */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	bool AreFactionEnemies(EFaction FactionA, EFaction FactionB) const;

	// ==================== Titles ====================

	/** Get available titles for a player based on reputation */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Titles")
	TArray<FName> GetAvailableTitles(const FString& PlayerID) const;

	/** Set active title */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Titles")
	bool SetActiveTitle(const FString& PlayerID, FName TitleID);

	/** Get active title */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Titles")
	FName GetActiveTitle(const FString& PlayerID) const;

	// ==================== Reputation History ====================

	/** Get recent reputation changes */
	UFUNCTION(BlueprintCallable, Category = "Reputation|History")
	TArray<FReputationChangeRecord> GetReputationHistory(const FString& PlayerID,
		int32 MaxEntries = 50) const;

	/** Get reputation changes filtered by faction */
	UFUNCTION(BlueprintCallable, Category = "Reputation|History")
	TArray<FReputationChangeRecord> GetFactionHistory(const FString& PlayerID,
		EFaction Faction, int32 MaxEntries = 20) const;

	// ==================== Faction Data ====================

	/** Get faction definition */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	bool GetFactionDefinition(EFaction Faction, FFactionDefinition& OutDefinition) const;

	/** Get all registered factions */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	TArray<FFactionDefinition> GetAllFactions() const;

	/** Get faction name as string */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Faction")
	FString GetFactionName(EFaction Faction) const;

	// ==================== Reputation Decay ====================

	/** Process reputation decay for all players (call periodically) */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Decay")
	void ProcessReputationDecay(float DeltaTime);

	/** Set global decay rate multiplier */
	UFUNCTION(BlueprintCallable, Category = "Reputation|Decay")
	void SetDecayRateMultiplier(float Multiplier);

	// ==================== Player Profile Management ====================

	/** Create or get reputation profile for a player */
	UFUNCTION(BlueprintCallable, Category = "Reputation")
	void EnsurePlayerProfile(const FString& PlayerID, const FString& PlayerName);

	/** Remove a player's reputation profile */
	UFUNCTION(BlueprintCallable, Category = "Reputation")
	void RemovePlayerProfile(const FString& PlayerID);

	/** Check if a player has a reputation profile */
	UFUNCTION(BlueprintCallable, Category = "Reputation")
	bool HasPlayerProfile(const FString& PlayerID) const;

	// ==================== NPC Behavior Integration ====================

	/** Should NPC attack this player on sight? */
	UFUNCTION(BlueprintCallable, Category = "Reputation|NPC")
	bool ShouldNPCAttackPlayer(const FString& PlayerID, EFaction NPCFaction) const;

	/** Should NPC refuse service? */
	UFUNCTION(BlueprintCallable, Category = "Reputation|NPC")
	bool ShouldNPCRefuseService(const FString& PlayerID, EFaction NPCFaction) const;

	/** Get NPC dialogue disposition modifier */
	UFUNCTION(BlueprintCallable, Category = "Reputation|NPC")
	float GetNPCDispositionModifier(const FString& PlayerID, EFaction NPCFaction) const;

	// ==================== Events ====================

	UPROPERTY(BlueprintAssignable, Category = "Reputation Events")
	FOnReputationChanged OnReputationChanged;

	UPROPERTY(BlueprintAssignable, Category = "Reputation Events")
	FOnReputationTierChanged OnReputationTierChanged;

	UPROPERTY(BlueprintAssignable, Category = "Reputation Events")
	FOnPlayerTrustScoreChanged OnPlayerTrustScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "Reputation Events")
	FOnTitleUnlocked OnTitleUnlocked;

protected:
	// All player profiles
	UPROPERTY()
	TMap<FString, FPlayerReputationProfile> PlayerProfiles;

	// Faction definitions
	UPROPERTY()
	TMap<EFaction, FFactionDefinition> FactionDefinitions;

	// Faction-to-faction relationships (for ripple effects)
	UPROPERTY()
	TArray<FFactionRelationship> FactionRelationships;

	// Title definitions: TitleID -> {RequiredFaction, RequiredTier}
	UPROPERTY()
	TMap<FName, TPair<EFaction, EReputationTier>> TitleRequirements;

	// Configuration
	UPROPERTY()
	int32 MaxHistoryPerPlayer;

	UPROPERTY()
	int32 MaxFeedbackPerPlayer;

	UPROPERTY()
	float GlobalDecayMultiplier;

	UPROPERTY()
	float MinReputationValue;

	UPROPERTY()
	float MaxReputationValue;

	// Thread safety
	mutable FCriticalSection ReputationLock;

	// Timer for periodic decay
	FTimerHandle DecayTimerHandle;

	// Internal helpers
	void InitializeFactionDefinitions();
	void InitializeFactionRelationships();
	void InitializeTitleDefinitions();
	FPlayerReputationProfile& GetOrCreateProfile(const FString& PlayerID, const FString& PlayerName = TEXT(""));
	void ApplyRippleEffects(const FString& PlayerID, EFaction SourceFaction, float BaseAmount, EReputationChangeSource Source);
	void CheckTitleUnlocks(const FString& PlayerID, FPlayerReputationProfile& Profile);
	void RecalculateTrustScore(FPlayerSocialReputation& SocialRep);
	void TrimHistory(FPlayerReputationProfile& Profile);
};
