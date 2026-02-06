// CooperativeProjectSystem.h
// System for managing large-scale cooperative projects requiring multiple players
// Enables mega-builds like stations, capital ships, and shared infrastructure

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "OdysseyInventoryComponent.h"
#include "CooperativeProjectSystem.generated.h"

// Forward declarations
class UOdysseyGuildManager;

/**
 * Project phase/state
 */
UENUM(BlueprintType)
enum class EProjectState : uint8
{
	Planning,           // Initial planning phase
	ResourceGathering,  // Collecting required resources
	Construction,       // Active building phase
	Testing,            // Quality/functionality testing
	Completed,          // Successfully finished
	Failed,             // Project abandoned or failed
	OnHold              // Temporarily paused
};

/**
 * Project type categories
 */
UENUM(BlueprintType)
enum class EProjectType : uint8
{
	Station,            // Space station construction
	MegaShip,           // Capital/carrier class ship
	Infrastructure,     // Jump gates, communication arrays
	Facility,           // Crafting facilities, refineries
	Defensive,          // Defense platforms, shields
	Research,           // Research labs, observatories
	Custom              // Player-defined projects
};

/**
 * Project visibility/access
 */
UENUM(BlueprintType)
enum class EProjectVisibility : uint8
{
	Private,            // Guild members only
	Allied,             // Guild and allied guilds
	Public              // Anyone can view/contribute
};

/**
 * Contributor tier based on contribution percentage
 */
UENUM(BlueprintType)
enum class EContributorTier : uint8
{
	Participant,        // < 5% contribution
	Supporter,          // 5-15% contribution
	Contributor,        // 15-30% contribution
	Major,              // 30-50% contribution
	Founder             // > 50% contribution
};

/**
 * Resource requirement for a project
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProjectResourceRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Resource")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Resource")
	int64 RequiredAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Resource")
	int64 ContributedAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Resource")
	bool bIsOptional; // Speeds up completion if provided

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Resource")
	float BonusMultiplier; // Bonus if optional resource provided

	FProjectResourceRequirement()
		: ResourceType(EResourceType::None)
		, RequiredAmount(0)
		, ContributedAmount(0)
		, bIsOptional(false)
		, BonusMultiplier(1.0f)
	{
	}

	FProjectResourceRequirement(EResourceType Type, int64 Required, bool bOptional = false, float Bonus = 1.0f)
		: ResourceType(Type)
		, RequiredAmount(Required)
		, ContributedAmount(0)
		, bIsOptional(bOptional)
		, BonusMultiplier(Bonus)
	{
	}

	float GetCompletionPercentage() const
	{
		if (RequiredAmount <= 0) return 1.0f;
		return FMath::Clamp(static_cast<float>(ContributedAmount) / static_cast<float>(RequiredAmount), 0.0f, 1.0f);
	}

	bool IsComplete() const
	{
		return ContributedAmount >= RequiredAmount;
	}

	int64 GetRemainingAmount() const
	{
		return FMath::Max(0LL, RequiredAmount - ContributedAmount);
	}
};

/**
 * Project milestone/phase
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProjectMilestone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	FGuid MilestoneID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	FString MilestoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	int32 OrderIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	TArray<FProjectResourceRequirement> Requirements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	float EstimatedHours;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	bool bIsComplete;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	FDateTime CompletedAt;

	// Rewards for completing this milestone
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	int64 ExperienceReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Milestone")
	TMap<EResourceType, int64> ResourceRewards;

	FProjectMilestone()
		: MilestoneName(TEXT("Milestone"))
		, Description(TEXT(""))
		, OrderIndex(0)
		, EstimatedHours(1.0f)
		, bIsComplete(false)
		, ExperienceReward(100)
	{
		MilestoneID = FGuid::NewGuid();
	}

	float GetOverallCompletion() const
	{
		if (Requirements.Num() == 0) return bIsComplete ? 1.0f : 0.0f;

		float TotalRequired = 0.0f;
		float TotalContributed = 0.0f;

		for (const FProjectResourceRequirement& Req : Requirements)
		{
			if (!Req.bIsOptional)
			{
				TotalRequired += static_cast<float>(Req.RequiredAmount);
				TotalContributed += static_cast<float>(FMath::Min(Req.ContributedAmount, Req.RequiredAmount));
			}
		}

		if (TotalRequired <= 0.0f) return 1.0f;
		return TotalContributed / TotalRequired;
	}
};

/**
 * Individual contribution record
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProjectContribution
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Contribution")
	FGuid ContributionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Contribution")
	FString ContributorPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Contribution")
	FString ContributorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Contribution")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Contribution")
	int64 Amount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Contribution")
	FDateTime ContributionTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Contribution")
	FGuid MilestoneID; // Which milestone this was contributed to

	FProjectContribution()
		: ContributorPlayerID(TEXT(""))
		, ContributorName(TEXT(""))
		, ResourceType(EResourceType::None)
		, Amount(0)
	{
		ContributionID = FGuid::NewGuid();
		ContributionTime = FDateTime::Now();
	}
};

/**
 * Contributor summary for a project
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProjectContributorSummary
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contributor Summary")
	FString PlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contributor Summary")
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contributor Summary")
	TMap<EResourceType, int64> ResourceContributions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contributor Summary")
	int64 TotalValueContributed; // Normalized value in OMEN

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contributor Summary")
	float ContributionPercentage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contributor Summary")
	EContributorTier Tier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contributor Summary")
	int32 ContributionCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contributor Summary")
	FDateTime FirstContribution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contributor Summary")
	FDateTime LastContribution;

	FProjectContributorSummary()
		: PlayerID(TEXT(""))
		, PlayerName(TEXT(""))
		, TotalValueContributed(0)
		, ContributionPercentage(0.0f)
		, Tier(EContributorTier::Participant)
		, ContributionCount(0)
	{
		FirstContribution = FDateTime::Now();
		LastContribution = FDateTime::Now();
	}

	void UpdateTier()
	{
		if (ContributionPercentage >= 50.0f)
			Tier = EContributorTier::Founder;
		else if (ContributionPercentage >= 30.0f)
			Tier = EContributorTier::Major;
		else if (ContributionPercentage >= 15.0f)
			Tier = EContributorTier::Contributor;
		else if (ContributionPercentage >= 5.0f)
			Tier = EContributorTier::Supporter;
		else
			Tier = EContributorTier::Participant;
	}
};

/**
 * Project reward distribution configuration
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProjectRewardConfig
{
	GENERATED_BODY()

	// How to distribute rewards
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward Config")
	bool bDistributeByContribution; // If true, proportional to contribution

	// Flat rewards for participation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward Config")
	int64 ParticipationBonus;

	// Tier multipliers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward Config")
	float ParticipantMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward Config")
	float SupporterMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward Config")
	float ContributorMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward Config")
	float MajorMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward Config")
	float FounderMultiplier;

	// Access rights based on tier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward Config")
	EContributorTier MinTierForAccess;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward Config")
	EContributorTier MinTierForManagement;

	FProjectRewardConfig()
		: bDistributeByContribution(true)
		, ParticipationBonus(100)
		, ParticipantMultiplier(1.0f)
		, SupporterMultiplier(1.5f)
		, ContributorMultiplier(2.0f)
		, MajorMultiplier(3.0f)
		, FounderMultiplier(5.0f)
		, MinTierForAccess(EContributorTier::Participant)
		, MinTierForManagement(EContributorTier::Major)
	{
	}

	float GetTierMultiplier(EContributorTier Tier) const
	{
		switch (Tier)
		{
		case EContributorTier::Founder: return FounderMultiplier;
		case EContributorTier::Major: return MajorMultiplier;
		case EContributorTier::Contributor: return ContributorMultiplier;
		case EContributorTier::Supporter: return SupporterMultiplier;
		default: return ParticipantMultiplier;
		}
	}
};

/**
 * Complete cooperative project data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FCooperativeProject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FGuid ProjectID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FString ProjectName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	EProjectType ProjectType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	EProjectState State;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	EProjectVisibility Visibility;

	// Ownership
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FGuid OwnerGuildID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FString CreatorPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FString CreatorName;

	// Timing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FDateTime CreatedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FDateTime StartedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FDateTime CompletedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FDateTime Deadline; // Optional deadline

	// Progress tracking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	TArray<FProjectMilestone> Milestones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	int32 CurrentMilestoneIndex;

	// Contributions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	TArray<FProjectContribution> ContributionHistory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	TMap<FString, FProjectContributorSummary> Contributors;

	// Reward configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FProjectRewardConfig RewardConfig;

	// Final rewards pool
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	TMap<EResourceType, int64> RewardPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	int64 ExperiencePool;

	// Location/result
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FVector ResultLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	FString ResultAssetID; // ID of spawned asset on completion

	// Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	int32 MinContributors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	int32 MaxContributors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project")
	bool bAllowPublicContributions;

	FCooperativeProject()
		: ProjectName(TEXT("New Project"))
		, Description(TEXT(""))
		, ProjectType(EProjectType::Custom)
		, State(EProjectState::Planning)
		, Visibility(EProjectVisibility::Private)
		, CreatorPlayerID(TEXT(""))
		, CreatorName(TEXT(""))
		, CurrentMilestoneIndex(0)
		, ExperiencePool(0)
		, ResultLocation(FVector::ZeroVector)
		, ResultAssetID(TEXT(""))
		, MinContributors(1)
		, MaxContributors(100)
		, bAllowPublicContributions(false)
	{
		ProjectID = FGuid::NewGuid();
		CreatedAt = FDateTime::Now();
	}

	float GetOverallProgress() const
	{
		if (Milestones.Num() == 0) return 0.0f;

		float TotalProgress = 0.0f;
		for (const FProjectMilestone& Milestone : Milestones)
		{
			TotalProgress += Milestone.GetOverallCompletion();
		}
		return TotalProgress / static_cast<float>(Milestones.Num());
	}

	int32 GetCompletedMilestoneCount() const
	{
		int32 Count = 0;
		for (const FProjectMilestone& Milestone : Milestones)
		{
			if (Milestone.bIsComplete) Count++;
		}
		return Count;
	}

	FProjectMilestone* GetCurrentMilestone()
	{
		if (CurrentMilestoneIndex >= 0 && CurrentMilestoneIndex < Milestones.Num())
		{
			return &Milestones[CurrentMilestoneIndex];
		}
		return nullptr;
	}

	const FProjectMilestone* GetCurrentMilestone() const
	{
		if (CurrentMilestoneIndex >= 0 && CurrentMilestoneIndex < Milestones.Num())
		{
			return &Milestones[CurrentMilestoneIndex];
		}
		return nullptr;
	}

	int64 GetTotalValueContributed() const
	{
		int64 Total = 0;
		for (const auto& Pair : Contributors)
		{
			Total += Pair.Value.TotalValueContributed;
		}
		return Total;
	}
};

/**
 * Project template for common project types
 */
USTRUCT(BlueprintType, meta = (DataTable))
struct ODYSSEY_API FProjectTemplate : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Template")
	FString TemplateName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Template")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Template")
	EProjectType ProjectType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Template")
	TArray<FProjectMilestone> Milestones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Template")
	int32 MinGuildLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Template")
	int32 MinContributors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Template")
	FString ResultAssetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Template")
	TMap<EResourceType, int64> BaseRewards;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Project Template")
	int64 BaseExperience;

	FProjectTemplate()
		: TemplateName(TEXT("New Template"))
		, Description(TEXT(""))
		, ProjectType(EProjectType::Custom)
		, MinGuildLevel(1)
		, MinContributors(1)
		, ResultAssetClass(TEXT(""))
		, BaseExperience(1000)
	{
	}
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectCreated, const FGuid&, ProjectID, const FString&, ProjectName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectStateChanged, const FGuid&, ProjectID, EProjectState, OldState, EProjectState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnContributionMade, const FGuid&, ProjectID, const FString&, PlayerID, EResourceType, ResourceType, int64, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMilestoneCompleted, const FGuid&, ProjectID, int32, MilestoneIndex, const FString&, MilestoneName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectCompleted, const FGuid&, ProjectID, const FString&, ProjectName);

/**
 * UCooperativeProjectSystem
 * 
 * Manages large-scale cooperative projects that require multiple players.
 * Supports mega-builds, shared infrastructure, and collaborative construction.
 */
UCLASS(Blueprintable, BlueprintType)
class ODYSSEY_API UCooperativeProjectSystem : public UObject
{
	GENERATED_BODY()

public:
	UCooperativeProjectSystem();

	// Initialize the system
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	void Initialize(UOdysseyGuildManager* InGuildManager);

	// ==================== Project Lifecycle ====================

	/** Create a new project from scratch */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	FGuid CreateProject(const FString& CreatorPlayerID, const FString& CreatorName,
		const FGuid& GuildID, const FString& ProjectName, const FString& Description,
		EProjectType Type, EProjectVisibility Visibility);

	/** Create a project from a template */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	FGuid CreateProjectFromTemplate(const FString& CreatorPlayerID, const FString& CreatorName,
		const FGuid& GuildID, FName TemplateID, const FString& ProjectName);

	/** Start a project (move from Planning to ResourceGathering) */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool StartProject(const FGuid& ProjectID, const FString& PlayerID);

	/** Pause a project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool PauseProject(const FGuid& ProjectID, const FString& PlayerID);

	/** Resume a paused project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool ResumeProject(const FGuid& ProjectID, const FString& PlayerID);

	/** Cancel/abandon a project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool CancelProject(const FGuid& ProjectID, const FString& PlayerID, bool bRefundContributions);

	/** Get project data */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool GetProjectData(const FGuid& ProjectID, FCooperativeProject& OutProject) const;

	/** Get all projects for a guild */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	TArray<FCooperativeProject> GetGuildProjects(const FGuid& GuildID) const;

	/** Get all projects a player has contributed to */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	TArray<FCooperativeProject> GetPlayerProjects(const FString& PlayerID) const;

	/** Search for public projects */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	TArray<FCooperativeProject> SearchPublicProjects(const FString& SearchQuery, EProjectType TypeFilter) const;

	// ==================== Milestone Management ====================

	/** Add a milestone to a project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool AddMilestone(const FGuid& ProjectID, const FString& PlayerID, const FProjectMilestone& Milestone);

	/** Modify an existing milestone */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool ModifyMilestone(const FGuid& ProjectID, const FString& PlayerID,
		const FGuid& MilestoneID, const FProjectMilestone& UpdatedMilestone);

	/** Remove a milestone (only in Planning state) */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool RemoveMilestone(const FGuid& ProjectID, const FString& PlayerID, const FGuid& MilestoneID);

	/** Add resource requirement to a milestone */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool AddResourceRequirement(const FGuid& ProjectID, const FString& PlayerID,
		const FGuid& MilestoneID, const FProjectResourceRequirement& Requirement);

	// ==================== Contributions ====================

	/** Contribute resources to a project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool ContributeResources(const FGuid& ProjectID, const FString& PlayerID,
		const FString& PlayerName, EResourceType ResourceType, int64 Amount);

	/** Get a player's contribution summary for a project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool GetContributorSummary(const FGuid& ProjectID, const FString& PlayerID,
		FProjectContributorSummary& OutSummary) const;

	/** Get all contributors for a project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	TArray<FProjectContributorSummary> GetAllContributors(const FGuid& ProjectID) const;

	/** Get contribution history */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	TArray<FProjectContribution> GetContributionHistory(const FGuid& ProjectID, int32 MaxEntries = 50) const;

	/** Check if player can contribute to project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool CanContribute(const FGuid& ProjectID, const FString& PlayerID) const;

	// ==================== Progress & Completion ====================

	/** Get overall project progress (0.0 - 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	float GetProjectProgress(const FGuid& ProjectID) const;

	/** Get current milestone progress */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	float GetCurrentMilestoneProgress(const FGuid& ProjectID) const;

	/** Check and advance milestone if complete */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool CheckMilestoneCompletion(const FGuid& ProjectID);

	/** Manually complete a project (GM/debug) */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool ForceCompleteProject(const FGuid& ProjectID);

	// ==================== Rewards ====================

	/** Configure reward distribution */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool SetRewardConfig(const FGuid& ProjectID, const FString& PlayerID,
		const FProjectRewardConfig& Config);

	/** Add to reward pool */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool AddToRewardPool(const FGuid& ProjectID, const FString& PlayerID,
		EResourceType ResourceType, int64 Amount);

	/** Calculate rewards for a contributor */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	TMap<EResourceType, int64> CalculateContributorRewards(const FGuid& ProjectID,
		const FString& PlayerID) const;

	/** Distribute rewards on completion */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool DistributeRewards(const FGuid& ProjectID);

	// ==================== Access Control ====================

	/** Check if player has access to view project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool HasProjectAccess(const FGuid& ProjectID, const FString& PlayerID) const;

	/** Check if player can manage project */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool CanManageProject(const FGuid& ProjectID, const FString& PlayerID) const;

	// ==================== Templates ====================

	/** Get available project templates */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	TArray<FProjectTemplate> GetAvailableTemplates(const FGuid& GuildID) const;

	/** Get template by ID */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	bool GetTemplate(FName TemplateID, FProjectTemplate& OutTemplate) const;

	// ==================== Resource Value Calculation ====================

	/** Get the normalized value of a resource (in OMEN equivalent) */
	UFUNCTION(BlueprintCallable, Category = "Cooperative Project")
	int64 GetResourceValue(EResourceType ResourceType, int64 Amount) const;

	// ==================== Events ====================

	UPROPERTY(BlueprintAssignable, Category = "Cooperative Project Events")
	FOnProjectCreated OnProjectCreated;

	UPROPERTY(BlueprintAssignable, Category = "Cooperative Project Events")
	FOnProjectStateChanged OnProjectStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Cooperative Project Events")
	FOnContributionMade OnContributionMade;

	UPROPERTY(BlueprintAssignable, Category = "Cooperative Project Events")
	FOnMilestoneCompleted OnMilestoneCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Cooperative Project Events")
	FOnProjectCompleted OnProjectCompleted;

protected:
	// Reference to guild manager for permission checks
	UPROPERTY()
	UOdysseyGuildManager* GuildManager;

	// All projects indexed by ID
	UPROPERTY()
	TMap<FGuid, FCooperativeProject> Projects;

	// Project templates
	UPROPERTY(EditAnywhere, Category = "Templates")
	UDataTable* ProjectTemplateTable;

	// Resource value table (for contribution normalization)
	UPROPERTY()
	TMap<EResourceType, int64> ResourceBaseValues;

	// Thread safety
	mutable FCriticalSection ProjectLock;

	// Internal helpers
	void UpdateContributorSummaries(FCooperativeProject& Project);
	void CheckAndCompleteProject(FCooperativeProject& Project);
	void ChangeProjectState(FCooperativeProject& Project, EProjectState NewState);
	bool IsProjectCreatorOrManager(const FCooperativeProject& Project, const FString& PlayerID) const;
	void InitializeResourceValues();
};
