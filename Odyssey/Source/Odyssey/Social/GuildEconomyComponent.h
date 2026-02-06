// GuildEconomyComponent.h
// Guild-level economic activities, shared resources, and collective economic goals
// Integrates with market systems, cooperative projects, and reputation tracking

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OdysseyInventoryComponent.h"
#include "GuildEconomyComponent.generated.h"

// Forward declarations
class UOdysseyGuildManager;
class UCooperativeProjectSystem;
class UReputationSystem;

// ============================================================================
// ENUMERATIONS
// ============================================================================

/**
 * Types of guild economic policies
 */
UENUM(BlueprintType)
enum class EGuildEconomicPolicy : uint8
{
	FreeMarket,         // Members trade freely, low tax
	Cooperative,        // Shared resources, moderate tax
	Collectivist,       // All earnings taxed heavily, distributed equally
	MilitaryEconomy,    // Resources prioritized for defense/combat
	Research,           // Resources directed to research projects
	Custom              // Player-defined
};

/**
 * Guild facility types that provide economic bonuses
 */
UENUM(BlueprintType)
enum class EGuildFacilityType : uint8
{
	Warehouse,          // Increases bank storage capacity
	TradingPost,        // Provides trade discounts
	Refinery,           // Refining bonus for raw materials
	Workshop,           // Crafting speed/quality bonus
	ResearchLab,        // Unlocks research projects
	DefensePlatform,    // Territory defense bonus
	ShipYard,           // Ship construction/repair
	MarketTerminal,     // Direct market access with reduced fees
	Beacon,             // Increased visibility / recruitment
	Embassy             // Diplomacy bonus with factions
};

/**
 * Guild treasury transaction type
 */
UENUM(BlueprintType)
enum class ETreasuryTransactionType : uint8
{
	Deposit,
	Withdrawal,
	TaxCollection,
	ProjectFunding,
	FacilityPurchase,
	FacilityUpkeep,
	DividendPayout,
	WarFund,
	TradeProfit,
	AllianceDues,
	Bounty,
	Refund,
	SystemFee
};

/**
 * Guild economic goal tracking
 */
UENUM(BlueprintType)
enum class EGuildGoalStatus : uint8
{
	Active,
	Completed,
	Failed,
	Expired,
	Paused
};

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Treasury transaction record
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FTreasuryTransaction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasury")
	FGuid TransactionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasury")
	ETreasuryTransactionType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasury")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasury")
	int64 Amount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasury")
	int64 BalanceAfter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasury")
	FString InitiatorPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasury")
	FString InitiatorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasury")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treasury")
	FDateTime Timestamp;

	FTreasuryTransaction()
		: Type(ETreasuryTransactionType::Deposit)
		, ResourceType(EResourceType::OMEN)
		, Amount(0)
		, BalanceAfter(0)
		, InitiatorPlayerID(TEXT(""))
		, InitiatorName(TEXT(""))
		, Description(TEXT(""))
	{
		TransactionID = FGuid::NewGuid();
		Timestamp = FDateTime::Now();
	}
};

/**
 * Guild facility data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildFacility
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	FGuid FacilityID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	EGuildFacilityType FacilityType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	FString FacilityName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	int32 Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	int32 MaxLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	bool bIsActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	FDateTime BuiltAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	FDateTime LastUpkeepPaid;

	// Cost to build/upgrade (per level)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	TMap<EResourceType, int64> UpgradeCost;

	// Recurring upkeep cost (per day)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	TMap<EResourceType, int64> DailyUpkeep;

	// Bonus provided at current level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	float BonusMultiplier;

	// Capacity added (for storage facilities)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Facility")
	int64 CapacityBonus;

	FGuildFacility()
		: FacilityType(EGuildFacilityType::Warehouse)
		, FacilityName(TEXT("Facility"))
		, Level(1)
		, MaxLevel(5)
		, bIsActive(true)
		, BonusMultiplier(1.1f)
		, CapacityBonus(0)
	{
		FacilityID = FGuid::NewGuid();
		BuiltAt = FDateTime::Now();
		LastUpkeepPaid = FDateTime::Now();
	}

	float GetEffectiveBonus() const
	{
		if (!bIsActive) return 1.0f;
		// Bonus scales with level
		return 1.0f + ((BonusMultiplier - 1.0f) * static_cast<float>(Level));
	}
};

/**
 * Guild economic goal (collective target)
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildEconomicGoal
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	FGuid GoalID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	FString GoalName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	EGuildGoalStatus Status;

	// Target: accumulate resources
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	TMap<EResourceType, int64> TargetResources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	TMap<EResourceType, int64> CurrentProgress;

	// Target: reach treasury balance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	int64 TargetTreasuryBalance;

	// Target: complete N trades / contracts
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	int32 TargetTradeCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	int32 CurrentTradeCount;

	// Timing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	FDateTime CreatedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	FDateTime Deadline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	FDateTime CompletedAt;

	// Rewards for reaching the goal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	int64 GuildExperienceReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	TMap<EResourceType, int64> MemberRewards; // Per-member distribution

	// Who set this goal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Goal")
	FString CreatorPlayerID;

	FGuildEconomicGoal()
		: GoalName(TEXT("New Goal"))
		, Description(TEXT(""))
		, Status(EGuildGoalStatus::Active)
		, TargetTreasuryBalance(0)
		, TargetTradeCount(0)
		, CurrentTradeCount(0)
		, GuildExperienceReward(500)
		, CreatorPlayerID(TEXT(""))
	{
		GoalID = FGuid::NewGuid();
		CreatedAt = FDateTime::Now();
		Deadline = FDateTime::Now() + FTimespan::FromDays(30);
	}

	float GetResourceProgress() const
	{
		if (TargetResources.Num() == 0) return 1.0f;

		float TotalProgress = 0.0f;
		for (const auto& Pair : TargetResources)
		{
			const int64* Current = CurrentProgress.Find(Pair.Key);
			int64 CurrentAmount = Current ? *Current : 0;
			float Ratio = Pair.Value > 0 ?
				FMath::Clamp(static_cast<float>(CurrentAmount) / static_cast<float>(Pair.Value), 0.0f, 1.0f) : 1.0f;
			TotalProgress += Ratio;
		}
		return TotalProgress / static_cast<float>(TargetResources.Num());
	}

	float GetTradeProgress() const
	{
		if (TargetTradeCount <= 0) return 1.0f;
		return FMath::Clamp(static_cast<float>(CurrentTradeCount) / static_cast<float>(TargetTradeCount), 0.0f, 1.0f);
	}

	bool IsExpired() const
	{
		return FDateTime::Now() > Deadline && Status == EGuildGoalStatus::Active;
	}
};

/**
 * Dividend distribution record
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FDividendRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dividend")
	FGuid DividendID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dividend")
	FDateTime DistributionDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dividend")
	TMap<EResourceType, int64> TotalDistributed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dividend")
	int32 RecipientCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dividend")
	FString AuthorizerPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dividend")
	FString Method; // "Equal", "Contribution-based", "Rank-based"

	FDividendRecord()
		: RecipientCount(0)
		, AuthorizerPlayerID(TEXT(""))
		, Method(TEXT("Equal"))
	{
		DividendID = FGuid::NewGuid();
		DistributionDate = FDateTime::Now();
	}
};

/**
 * Member economic contribution summary (within guild context)
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FMemberEconomicContribution
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	FString PlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	FString PlayerName;

	// Taxes collected from this member
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	int64 TotalTaxesPaid;

	// Direct deposits to treasury
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	int64 TotalDeposited;

	// Withdrawals from treasury
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	int64 TotalWithdrawn;

	// Project resource contributions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	int64 TotalProjectContributions;

	// Trades completed on guild's behalf
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	int32 TradesCompleted;

	// Net economic value contributed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	int64 NetContribution;

	// Dividends received
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	int64 TotalDividendsReceived;

	// Period tracking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	int64 CurrentWeekContribution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Member Contribution")
	FDateTime LastContribution;

	FMemberEconomicContribution()
		: PlayerID(TEXT(""))
		, PlayerName(TEXT(""))
		, TotalTaxesPaid(0)
		, TotalDeposited(0)
		, TotalWithdrawn(0)
		, TotalProjectContributions(0)
		, TradesCompleted(0)
		, NetContribution(0)
		, TotalDividendsReceived(0)
		, CurrentWeekContribution(0)
	{
		LastContribution = FDateTime::Now();
	}

	void RecalculateNet()
	{
		NetContribution = TotalTaxesPaid + TotalDeposited + TotalProjectContributions
			- TotalWithdrawn - TotalDividendsReceived;
	}
};

/**
 * Guild economy snapshot for history/analytics
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildEconomySnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy Snapshot")
	FDateTime Timestamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy Snapshot")
	TMap<EResourceType, int64> TreasuryBalances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy Snapshot")
	int64 TotalIncome;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy Snapshot")
	int64 TotalExpenses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy Snapshot")
	int32 ActiveMemberCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy Snapshot")
	int32 ActiveProjectCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy Snapshot")
	int32 ActiveFacilityCount;

	FGuildEconomySnapshot()
		: TotalIncome(0)
		, TotalExpenses(0)
		, ActiveMemberCount(0)
		, ActiveProjectCount(0)
		, ActiveFacilityCount(0)
	{
		Timestamp = FDateTime::Now();
	}
};

/**
 * Complete guild economy data for a single guild
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FGuildEconomyData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	FGuid GuildID;

	// Economic policy
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	EGuildEconomicPolicy Policy;

	// Facilities
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	TArray<FGuildFacility> Facilities;

	// Goals
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	TArray<FGuildEconomicGoal> Goals;

	// Member economic data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	TMap<FString, FMemberEconomicContribution> MemberContributions;

	// Transaction history
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	TArray<FTreasuryTransaction> TransactionHistory;

	// Dividend history
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	TArray<FDividendRecord> DividendHistory;

	// Economy snapshots (daily)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	TArray<FGuildEconomySnapshot> EconomyHistory;

	// Current period income/expense tracking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	int64 PeriodIncome;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	int64 PeriodExpenses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	FDateTime PeriodStart;

	// Treasury storage capacity (affected by facilities)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	int64 BaseTreasuryCapacity;

	// Maximum number of facilities
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guild Economy")
	int32 MaxFacilities;

	FGuildEconomyData()
		: Policy(EGuildEconomicPolicy::Cooperative)
		, PeriodIncome(0)
		, PeriodExpenses(0)
		, BaseTreasuryCapacity(100000)
		, MaxFacilities(3)
	{
		PeriodStart = FDateTime::Now();
	}

	int64 GetTreasuryCapacity() const
	{
		int64 Capacity = BaseTreasuryCapacity;
		for (const FGuildFacility& Facility : Facilities)
		{
			if (Facility.bIsActive && Facility.FacilityType == EGuildFacilityType::Warehouse)
			{
				Capacity += Facility.CapacityBonus;
			}
		}
		return Capacity;
	}

	float GetFacilityBonus(EGuildFacilityType Type) const
	{
		float TotalBonus = 1.0f;
		for (const FGuildFacility& Facility : Facilities)
		{
			if (Facility.bIsActive && Facility.FacilityType == Type)
			{
				TotalBonus *= Facility.GetEffectiveBonus();
			}
		}
		return TotalBonus;
	}

	int32 GetActiveFacilityCount() const
	{
		int32 Count = 0;
		for (const FGuildFacility& F : Facilities) { if (F.bIsActive) Count++; }
		return Count;
	}
};

// ============================================================================
// DELEGATES
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnTreasuryChanged,
	const FGuid&, GuildID,
	EResourceType, ResourceType,
	int64, Amount,
	ETreasuryTransactionType, TransactionType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnFacilityBuilt,
	const FGuid&, GuildID,
	EGuildFacilityType, FacilityType,
	int32, Level);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnFacilityUpgraded,
	const FGuid&, GuildID,
	const FGuid&, FacilityID,
	int32, NewLevel);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnGoalCompleted,
	const FGuid&, GuildID,
	const FString&, GoalName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDividendDistributed,
	const FGuid&, GuildID,
	int32, RecipientCount,
	int64, TotalAmount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnPolicyChanged,
	const FGuid&, GuildID,
	EGuildEconomicPolicy, NewPolicy);

// ============================================================================
// MAIN SYSTEM CLASS
// ============================================================================

/**
 * UGuildEconomyComponent
 *
 * Manages guild-level economic activities including:
 * - Treasury management with full transaction logging
 * - Tax collection from member activities
 * - Guild facilities that provide economic bonuses
 * - Collective economic goals and milestone tracking
 * - Dividend distribution to members
 * - Integration with broader market systems
 * - Economic analytics and historical snapshots
 */
UCLASS(Blueprintable, BlueprintType)
class ODYSSEY_API UGuildEconomyComponent : public UObject
{
	GENERATED_BODY()

public:
	UGuildEconomyComponent();

	/** Initialize the guild economy system */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy")
	void Initialize(UOdysseyGuildManager* InGuildManager);

	// ==================== Treasury Operations ====================

	/** Collect tax from a member's earnings */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Treasury")
	bool CollectTax(const FGuid& GuildID, const FString& MemberPlayerID,
		const FString& MemberName, EResourceType ResourceType, int64 EarningAmount);

	/** Direct deposit to treasury */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Treasury")
	bool TreasuryDeposit(const FGuid& GuildID, const FString& PlayerID,
		const FString& PlayerName, EResourceType ResourceType, int64 Amount,
		const FString& Description = TEXT(""));

	/** Withdraw from treasury (permission checked) */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Treasury")
	bool TreasuryWithdraw(const FGuid& GuildID, const FString& PlayerID,
		EResourceType ResourceType, int64 Amount, const FString& Description = TEXT(""));

	/** Fund a cooperative project from treasury */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Treasury")
	bool FundProject(const FGuid& GuildID, const FString& PlayerID,
		const FGuid& ProjectID, EResourceType ResourceType, int64 Amount);

	/** Get treasury balance for a resource */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Treasury")
	int64 GetTreasuryBalance(const FGuid& GuildID, EResourceType ResourceType) const;

	/** Get all treasury balances */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Treasury")
	TMap<EResourceType, int64> GetAllTreasuryBalances(const FGuid& GuildID) const;

	/** Get treasury capacity */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Treasury")
	int64 GetTreasuryCapacity(const FGuid& GuildID) const;

	/** Get transaction history */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Treasury")
	TArray<FTreasuryTransaction> GetTransactionHistory(const FGuid& GuildID,
		int32 MaxEntries = 50) const;

	/** Get transaction history filtered by type */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Treasury")
	TArray<FTreasuryTransaction> GetTransactionsByType(const FGuid& GuildID,
		ETreasuryTransactionType Type, int32 MaxEntries = 20) const;

	// ==================== Facility Management ====================

	/** Build a new guild facility */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	bool BuildFacility(const FGuid& GuildID, const FString& PlayerID,
		EGuildFacilityType FacilityType, const FString& FacilityName);

	/** Upgrade an existing facility */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	bool UpgradeFacility(const FGuid& GuildID, const FString& PlayerID, const FGuid& FacilityID);

	/** Deactivate a facility (stops upkeep but also bonus) */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	bool DeactivateFacility(const FGuid& GuildID, const FString& PlayerID, const FGuid& FacilityID);

	/** Reactivate a facility */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	bool ActivateFacility(const FGuid& GuildID, const FString& PlayerID, const FGuid& FacilityID);

	/** Demolish a facility */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	bool DemolishFacility(const FGuid& GuildID, const FString& PlayerID, const FGuid& FacilityID);

	/** Get all facilities for a guild */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	TArray<FGuildFacility> GetFacilities(const FGuid& GuildID) const;

	/** Get facility bonus for a specific type */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	float GetFacilityBonus(const FGuid& GuildID, EGuildFacilityType Type) const;

	/** Get cost to build a facility type */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	TMap<EResourceType, int64> GetFacilityBuildCost(EGuildFacilityType Type, int32 GuildLevel) const;

	/** Get cost to upgrade a facility */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	TMap<EResourceType, int64> GetFacilityUpgradeCost(const FGuid& GuildID, const FGuid& FacilityID) const;

	/** Process daily facility upkeep */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	void ProcessFacilityUpkeep(const FGuid& GuildID);

	/** Get maximum facilities allowed for guild level */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Facilities")
	int32 GetMaxFacilities(const FGuid& GuildID) const;

	// ==================== Economic Goals ====================

	/** Create a new economic goal */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Goals")
	FGuid CreateGoal(const FGuid& GuildID, const FString& CreatorPlayerID,
		const FString& GoalName, const FString& Description,
		const TMap<EResourceType, int64>& TargetResources,
		int32 TargetTrades, int32 DaysToComplete);

	/** Update goal progress (called when relevant actions occur) */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Goals")
	void UpdateGoalProgress(const FGuid& GuildID, EResourceType ResourceType, int64 Amount);

	/** Update goal trade count */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Goals")
	void IncrementGoalTradeCount(const FGuid& GuildID);

	/** Cancel a goal */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Goals")
	bool CancelGoal(const FGuid& GuildID, const FString& PlayerID, const FGuid& GoalID);

	/** Get active goals */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Goals")
	TArray<FGuildEconomicGoal> GetActiveGoals(const FGuid& GuildID) const;

	/** Get all goals including completed */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Goals")
	TArray<FGuildEconomicGoal> GetAllGoals(const FGuid& GuildID) const;

	/** Check and complete goals if targets met */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Goals")
	void CheckGoalCompletion(const FGuid& GuildID);

	/** Check for expired goals */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Goals")
	void ProcessExpiredGoals(const FGuid& GuildID);

	// ==================== Dividend Distribution ====================

	/** Distribute treasury resources to members equally */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Dividends")
	bool DistributeEqualDividend(const FGuid& GuildID, const FString& AuthorizerPlayerID,
		EResourceType ResourceType, int64 TotalAmount);

	/** Distribute treasury resources based on contribution */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Dividends")
	bool DistributeContributionDividend(const FGuid& GuildID, const FString& AuthorizerPlayerID,
		EResourceType ResourceType, int64 TotalAmount);

	/** Distribute treasury resources based on rank */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Dividends")
	bool DistributeRankDividend(const FGuid& GuildID, const FString& AuthorizerPlayerID,
		EResourceType ResourceType, int64 TotalAmount);

	/** Get dividend history */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Dividends")
	TArray<FDividendRecord> GetDividendHistory(const FGuid& GuildID, int32 MaxEntries = 20) const;

	// ==================== Economic Policy ====================

	/** Set guild economic policy */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Policy")
	bool SetEconomicPolicy(const FGuid& GuildID, const FString& PlayerID, EGuildEconomicPolicy Policy);

	/** Get current economic policy */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Policy")
	EGuildEconomicPolicy GetEconomicPolicy(const FGuid& GuildID) const;

	/** Get effective tax rate (base + policy modifier) */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Policy")
	float GetEffectiveTaxRate(const FGuid& GuildID) const;

	// ==================== Member Economic Data ====================

	/** Get member's economic contribution data */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Members")
	bool GetMemberContribution(const FGuid& GuildID, const FString& PlayerID,
		FMemberEconomicContribution& OutContribution) const;

	/** Get all member contributions sorted by net */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Members")
	TArray<FMemberEconomicContribution> GetMemberContributions(const FGuid& GuildID) const;

	/** Get top contributors */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Members")
	TArray<FMemberEconomicContribution> GetTopContributors(const FGuid& GuildID, int32 Count = 10) const;

	// ==================== Analytics ====================

	/** Take an economy snapshot */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Analytics")
	void TakeEconomySnapshot(const FGuid& GuildID);

	/** Get economy snapshots for trend analysis */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Analytics")
	TArray<FGuildEconomySnapshot> GetEconomyHistory(const FGuid& GuildID, int32 MaxEntries = 30) const;

	/** Get current period income */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Analytics")
	int64 GetPeriodIncome(const FGuid& GuildID) const;

	/** Get current period expenses */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Analytics")
	int64 GetPeriodExpenses(const FGuid& GuildID) const;

	/** Reset period tracking */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy|Analytics")
	void ResetPeriodTracking(const FGuid& GuildID);

	// ==================== Guild Economy Lifecycle ====================

	/** Initialize economy data for a new guild */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy")
	void InitializeGuildEconomy(const FGuid& GuildID);

	/** Remove economy data for a disbanded guild */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy")
	void RemoveGuildEconomy(const FGuid& GuildID);

	/** Register a new member in the economy system */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy")
	void RegisterMember(const FGuid& GuildID, const FString& PlayerID, const FString& PlayerName);

	/** Unregister a member from the economy system */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy")
	void UnregisterMember(const FGuid& GuildID, const FString& PlayerID);

	/** Weekly maintenance: reset weekly counters, process upkeep */
	UFUNCTION(BlueprintCallable, Category = "Guild Economy")
	void ProcessWeeklyMaintenance();

	// ==================== Events ====================

	UPROPERTY(BlueprintAssignable, Category = "Guild Economy Events")
	FOnTreasuryChanged OnTreasuryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Guild Economy Events")
	FOnFacilityBuilt OnFacilityBuilt;

	UPROPERTY(BlueprintAssignable, Category = "Guild Economy Events")
	FOnFacilityUpgraded OnFacilityUpgraded;

	UPROPERTY(BlueprintAssignable, Category = "Guild Economy Events")
	FOnGoalCompleted OnGoalCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Guild Economy Events")
	FOnDividendDistributed OnDividendDistributed;

	UPROPERTY(BlueprintAssignable, Category = "Guild Economy Events")
	FOnPolicyChanged OnPolicyChanged;

protected:
	// Reference to guild manager
	UPROPERTY()
	UOdysseyGuildManager* GuildManager;

	// Economy data per guild
	UPROPERTY()
	TMap<FGuid, FGuildEconomyData> GuildEconomies;

	// Configuration
	UPROPERTY()
	int32 MaxTransactionHistory;

	UPROPERTY()
	int32 MaxEconomySnapshots;

	// Thread safety
	mutable FCriticalSection EconomyLock;

	// Timer handles
	FTimerHandle WeeklyMaintenanceTimer;
	FTimerHandle DailySnapshotTimer;

	// Internal helpers
	FGuildEconomyData* GetEconomyData(const FGuid& GuildID);
	const FGuildEconomyData* GetEconomyData(const FGuid& GuildID) const;
	void RecordTransaction(FGuildEconomyData& EconData, ETreasuryTransactionType Type,
		EResourceType ResourceType, int64 Amount, int64 BalanceAfter,
		const FString& PlayerID, const FString& PlayerName, const FString& Description);
	void UpdateMemberContribution(FGuildEconomyData& EconData, const FString& PlayerID,
		const FString& PlayerName, ETreasuryTransactionType Type, int64 Amount);
	FGuildFacility CreateDefaultFacility(EGuildFacilityType Type, const FString& Name, int32 GuildLevel);
	bool CanAfford(const FGuid& GuildID, const TMap<EResourceType, int64>& Cost) const;
	bool DeductCost(FGuildEconomyData& EconData, const TMap<EResourceType, int64>& Cost,
		const FString& PlayerID, const FString& PlayerName, const FString& Description);
	void TrimTransactionHistory(FGuildEconomyData& EconData);
};
