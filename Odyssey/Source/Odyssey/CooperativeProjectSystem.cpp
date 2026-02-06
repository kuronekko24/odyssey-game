// CooperativeProjectSystem.cpp
// Implementation of cooperative project management

#include "CooperativeProjectSystem.h"
#include "OdysseyGuildManager.h"

UCooperativeProjectSystem::UCooperativeProjectSystem()
	: GuildManager(nullptr)
	, ProjectTemplateTable(nullptr)
{
	InitializeResourceValues();
}

void UCooperativeProjectSystem::Initialize(UOdysseyGuildManager* InGuildManager)
{
	GuildManager = InGuildManager;
}

void UCooperativeProjectSystem::InitializeResourceValues()
{
	// Base values for resource contribution normalization (in OMEN equivalent)
	ResourceBaseValues.Add(EResourceType::Silicate, 1);
	ResourceBaseValues.Add(EResourceType::Carbon, 1);
	ResourceBaseValues.Add(EResourceType::RefinedSilicate, 5);
	ResourceBaseValues.Add(EResourceType::RefinedCarbon, 5);
	ResourceBaseValues.Add(EResourceType::CompositeMaterial, 25);
	ResourceBaseValues.Add(EResourceType::OMEN, 1);
}

// ==================== Project Lifecycle ====================

FGuid UCooperativeProjectSystem::CreateProject(const FString& CreatorPlayerID, const FString& CreatorName,
	const FGuid& GuildID, const FString& ProjectName, const FString& Description,
	EProjectType Type, EProjectVisibility Visibility)
{
	FScopeLock Lock(&ProjectLock);

	// Validate guild exists if specified
	if (GuildID.IsValid() && GuildManager)
	{
		FGuildData GuildData;
		if (!GuildManager->GetGuildData(GuildID, GuildData))
		{
			UE_LOG(LogTemp, Warning, TEXT("CreateProject: Invalid guild ID"));
			return FGuid();
		}

		// Check if creator is in the guild
		if (!GuildData.IsMember(CreatorPlayerID))
		{
			UE_LOG(LogTemp, Warning, TEXT("CreateProject: Creator is not a guild member"));
			return FGuid();
		}
	}

	FCooperativeProject NewProject;
	NewProject.ProjectName = ProjectName;
	NewProject.Description = Description;
	NewProject.ProjectType = Type;
	NewProject.Visibility = Visibility;
	NewProject.OwnerGuildID = GuildID;
	NewProject.CreatorPlayerID = CreatorPlayerID;
	NewProject.CreatorName = CreatorName;
	NewProject.State = EProjectState::Planning;

	FGuid ProjectID = NewProject.ProjectID;
	Projects.Add(ProjectID, NewProject);

	OnProjectCreated.Broadcast(ProjectID, ProjectName);

	UE_LOG(LogTemp, Log, TEXT("Created project '%s' by %s"), *ProjectName, *CreatorPlayerID);

	return ProjectID;
}

FGuid UCooperativeProjectSystem::CreateProjectFromTemplate(const FString& CreatorPlayerID, const FString& CreatorName,
	const FGuid& GuildID, FName TemplateID, const FString& ProjectName)
{
	FProjectTemplate Template;
	if (!GetTemplate(TemplateID, Template))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateProjectFromTemplate: Template %s not found"), *TemplateID.ToString());
		return FGuid();
	}

	// Check guild level requirement
	if (GuildID.IsValid() && GuildManager)
	{
		FGuildData GuildData;
		if (GuildManager->GetGuildData(GuildID, GuildData))
		{
			if (GuildData.Level < Template.MinGuildLevel)
			{
				UE_LOG(LogTemp, Warning, TEXT("CreateProjectFromTemplate: Guild level %d below required %d"),
					GuildData.Level, Template.MinGuildLevel);
				return FGuid();
			}
		}
	}

	FGuid ProjectID = CreateProject(CreatorPlayerID, CreatorName, GuildID, ProjectName,
		Template.Description, Template.ProjectType, EProjectVisibility::Private);

	if (!ProjectID.IsValid())
	{
		return FGuid();
	}

	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (Project)
	{
		// Copy milestones from template
		Project->Milestones = Template.Milestones;
		for (int32 i = 0; i < Project->Milestones.Num(); ++i)
		{
			Project->Milestones[i].MilestoneID = FGuid::NewGuid();
			Project->Milestones[i].OrderIndex = i;
		}

		Project->MinContributors = Template.MinContributors;
		Project->RewardPool = Template.BaseRewards;
		Project->ExperiencePool = Template.BaseExperience;
		Project->ResultAssetID = Template.ResultAssetClass;
	}

	return ProjectID;
}

bool UCooperativeProjectSystem::StartProject(const FGuid& ProjectID, const FString& PlayerID)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	if (Project->State != EProjectState::Planning)
	{
		return false;
	}

	// Must have at least one milestone
	if (Project->Milestones.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartProject: Project has no milestones"));
		return false;
	}

	Project->StartedAt = FDateTime::Now();
	ChangeProjectState(*Project, EProjectState::ResourceGathering);

	return true;
}

bool UCooperativeProjectSystem::PauseProject(const FGuid& ProjectID, const FString& PlayerID)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	if (Project->State != EProjectState::ResourceGathering && Project->State != EProjectState::Construction)
	{
		return false;
	}

	ChangeProjectState(*Project, EProjectState::OnHold);

	return true;
}

bool UCooperativeProjectSystem::ResumeProject(const FGuid& ProjectID, const FString& PlayerID)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	if (Project->State != EProjectState::OnHold)
	{
		return false;
	}

	// Resume to appropriate state based on milestone completion
	float Progress = Project->GetOverallProgress();
	if (Progress >= 1.0f)
	{
		ChangeProjectState(*Project, EProjectState::Testing);
	}
	else if (Project->CurrentMilestoneIndex > 0)
	{
		ChangeProjectState(*Project, EProjectState::Construction);
	}
	else
	{
		ChangeProjectState(*Project, EProjectState::ResourceGathering);
	}

	return true;
}

bool UCooperativeProjectSystem::CancelProject(const FGuid& ProjectID, const FString& PlayerID, bool bRefundContributions)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	if (Project->State == EProjectState::Completed)
	{
		return false;
	}

	if (bRefundContributions)
	{
		// TODO: Implement refund logic - would need inventory system integration
		UE_LOG(LogTemp, Log, TEXT("CancelProject: Refunds requested but not implemented"));
	}

	ChangeProjectState(*Project, EProjectState::Failed);

	return true;
}

bool UCooperativeProjectSystem::GetProjectData(const FGuid& ProjectID, FCooperativeProject& OutProject) const
{
	FScopeLock Lock(&ProjectLock);

	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (Project)
	{
		OutProject = *Project;
		return true;
	}
	return false;
}

TArray<FCooperativeProject> UCooperativeProjectSystem::GetGuildProjects(const FGuid& GuildID) const
{
	FScopeLock Lock(&ProjectLock);

	TArray<FCooperativeProject> Result;
	for (const auto& Pair : Projects)
	{
		if (Pair.Value.OwnerGuildID == GuildID)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FCooperativeProject> UCooperativeProjectSystem::GetPlayerProjects(const FString& PlayerID) const
{
	FScopeLock Lock(&ProjectLock);

	TArray<FCooperativeProject> Result;
	for (const auto& Pair : Projects)
	{
		if (Pair.Value.CreatorPlayerID == PlayerID || Pair.Value.Contributors.Contains(PlayerID))
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FCooperativeProject> UCooperativeProjectSystem::SearchPublicProjects(const FString& SearchQuery,
	EProjectType TypeFilter) const
{
	FScopeLock Lock(&ProjectLock);

	TArray<FCooperativeProject> Result;
	for (const auto& Pair : Projects)
	{
		const FCooperativeProject& Project = Pair.Value;

		if (Project.Visibility != EProjectVisibility::Public)
		{
			continue;
		}

		if (TypeFilter != EProjectType::Custom && Project.ProjectType != TypeFilter)
		{
			continue;
		}

		if (!SearchQuery.IsEmpty())
		{
			if (!Project.ProjectName.Contains(SearchQuery, ESearchCase::IgnoreCase) &&
				!Project.Description.Contains(SearchQuery, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		Result.Add(Project);
	}
	return Result;
}

// ==================== Milestone Management ====================

bool UCooperativeProjectSystem::AddMilestone(const FGuid& ProjectID, const FString& PlayerID,
	const FProjectMilestone& Milestone)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	// Can only add milestones during planning or if not past the new milestone index
	if (Project->State != EProjectState::Planning)
	{
		if (Milestone.OrderIndex <= Project->CurrentMilestoneIndex)
		{
			return false;
		}
	}

	FProjectMilestone NewMilestone = Milestone;
	NewMilestone.MilestoneID = FGuid::NewGuid();
	NewMilestone.bIsComplete = false;

	// Insert at correct position
	if (NewMilestone.OrderIndex >= Project->Milestones.Num())
	{
		NewMilestone.OrderIndex = Project->Milestones.Num();
		Project->Milestones.Add(NewMilestone);
	}
	else
	{
		Project->Milestones.Insert(NewMilestone, NewMilestone.OrderIndex);
		// Update order indices
		for (int32 i = NewMilestone.OrderIndex + 1; i < Project->Milestones.Num(); ++i)
		{
			Project->Milestones[i].OrderIndex = i;
		}
	}

	return true;
}

bool UCooperativeProjectSystem::ModifyMilestone(const FGuid& ProjectID, const FString& PlayerID,
	const FGuid& MilestoneID, const FProjectMilestone& UpdatedMilestone)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	for (FProjectMilestone& Milestone : Project->Milestones)
	{
		if (Milestone.MilestoneID == MilestoneID)
		{
			// Cannot modify completed milestones
			if (Milestone.bIsComplete)
			{
				return false;
			}

			// Preserve contribution data
			TArray<FProjectResourceRequirement> OldRequirements = Milestone.Requirements;

			Milestone.MilestoneName = UpdatedMilestone.MilestoneName;
			Milestone.Description = UpdatedMilestone.Description;
			Milestone.EstimatedHours = UpdatedMilestone.EstimatedHours;
			Milestone.ExperienceReward = UpdatedMilestone.ExperienceReward;
			Milestone.ResourceRewards = UpdatedMilestone.ResourceRewards;

			// Merge requirements, preserving contributions
			for (FProjectResourceRequirement& NewReq : Milestone.Requirements)
			{
				for (const FProjectResourceRequirement& OldReq : OldRequirements)
				{
					if (OldReq.ResourceType == NewReq.ResourceType)
					{
						NewReq.ContributedAmount = OldReq.ContributedAmount;
						break;
					}
				}
			}

			return true;
		}
	}

	return false;
}

bool UCooperativeProjectSystem::RemoveMilestone(const FGuid& ProjectID, const FString& PlayerID,
	const FGuid& MilestoneID)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	// Can only remove milestones during planning
	if (Project->State != EProjectState::Planning)
	{
		return false;
	}

	for (int32 i = 0; i < Project->Milestones.Num(); ++i)
	{
		if (Project->Milestones[i].MilestoneID == MilestoneID)
		{
			Project->Milestones.RemoveAt(i);

			// Update order indices
			for (int32 j = i; j < Project->Milestones.Num(); ++j)
			{
				Project->Milestones[j].OrderIndex = j;
			}

			return true;
		}
	}

	return false;
}

bool UCooperativeProjectSystem::AddResourceRequirement(const FGuid& ProjectID, const FString& PlayerID,
	const FGuid& MilestoneID, const FProjectResourceRequirement& Requirement)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	for (FProjectMilestone& Milestone : Project->Milestones)
	{
		if (Milestone.MilestoneID == MilestoneID)
		{
			if (Milestone.bIsComplete)
			{
				return false;
			}

			// Check for duplicate resource type
			for (const FProjectResourceRequirement& Existing : Milestone.Requirements)
			{
				if (Existing.ResourceType == Requirement.ResourceType)
				{
					return false;
				}
			}

			Milestone.Requirements.Add(Requirement);
			return true;
		}
	}

	return false;
}

// ==================== Contributions ====================

bool UCooperativeProjectSystem::ContributeResources(const FGuid& ProjectID, const FString& PlayerID,
	const FString& PlayerName, EResourceType ResourceType, int64 Amount)
{
	FScopeLock Lock(&ProjectLock);

	if (Amount <= 0)
	{
		return false;
	}

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	// Check if contributions are allowed
	if (Project->State != EProjectState::ResourceGathering && Project->State != EProjectState::Construction)
	{
		return false;
	}

	// Check access
	if (!CanContribute(ProjectID, PlayerID))
	{
		return false;
	}

	// Check contributor limit
	if (!Project->Contributors.Contains(PlayerID) &&
		Project->Contributors.Num() >= Project->MaxContributors)
	{
		return false;
	}

	// Find current milestone and matching requirement
	FProjectMilestone* CurrentMilestone = Project->GetCurrentMilestone();
	if (!CurrentMilestone)
	{
		return false;
	}

	FProjectResourceRequirement* MatchingReq = nullptr;
	for (FProjectResourceRequirement& Req : CurrentMilestone->Requirements)
	{
		if (Req.ResourceType == ResourceType && !Req.IsComplete())
		{
			MatchingReq = &Req;
			break;
		}
	}

	if (!MatchingReq)
	{
		// Resource not needed for current milestone
		return false;
	}

	// Calculate actual contribution (cap at requirement)
	int64 ActualContribution = FMath::Min(Amount, MatchingReq->GetRemainingAmount());
	MatchingReq->ContributedAmount += ActualContribution;

	// Record contribution
	FProjectContribution Contribution;
	Contribution.ContributorPlayerID = PlayerID;
	Contribution.ContributorName = PlayerName;
	Contribution.ResourceType = ResourceType;
	Contribution.Amount = ActualContribution;
	Contribution.MilestoneID = CurrentMilestone->MilestoneID;
	Project->ContributionHistory.Add(Contribution);

	// Update contributor summary
	FProjectContributorSummary* Summary = Project->Contributors.Find(PlayerID);
	if (!Summary)
	{
		FProjectContributorSummary NewSummary;
		NewSummary.PlayerID = PlayerID;
		NewSummary.PlayerName = PlayerName;
		Project->Contributors.Add(PlayerID, NewSummary);
		Summary = Project->Contributors.Find(PlayerID);
	}

	if (Summary)
	{
		if (!Summary->ResourceContributions.Contains(ResourceType))
		{
			Summary->ResourceContributions.Add(ResourceType, 0);
		}
		Summary->ResourceContributions[ResourceType] += ActualContribution;
		Summary->TotalValueContributed += GetResourceValue(ResourceType, ActualContribution);
		Summary->ContributionCount++;
		Summary->LastContribution = FDateTime::Now();
	}

	// Update all contributor percentages
	UpdateContributorSummaries(*Project);

	// Broadcast event
	OnContributionMade.Broadcast(ProjectID, PlayerID, ResourceType, ActualContribution);

	// Check milestone completion
	CheckMilestoneCompletion(ProjectID);

	return true;
}

bool UCooperativeProjectSystem::GetContributorSummary(const FGuid& ProjectID, const FString& PlayerID,
	FProjectContributorSummary& OutSummary) const
{
	FScopeLock Lock(&ProjectLock);

	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	const FProjectContributorSummary* Summary = Project->Contributors.Find(PlayerID);
	if (Summary)
	{
		OutSummary = *Summary;
		return true;
	}
	return false;
}

TArray<FProjectContributorSummary> UCooperativeProjectSystem::GetAllContributors(const FGuid& ProjectID) const
{
	FScopeLock Lock(&ProjectLock);

	TArray<FProjectContributorSummary> Result;
	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (Project)
	{
		for (const auto& Pair : Project->Contributors)
		{
			Result.Add(Pair.Value);
		}

		// Sort by contribution percentage descending
		Result.Sort([](const FProjectContributorSummary& A, const FProjectContributorSummary& B) {
			return A.ContributionPercentage > B.ContributionPercentage;
		});
	}
	return Result;
}

TArray<FProjectContribution> UCooperativeProjectSystem::GetContributionHistory(const FGuid& ProjectID,
	int32 MaxEntries) const
{
	FScopeLock Lock(&ProjectLock);

	TArray<FProjectContribution> Result;
	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (Project)
	{
		int32 StartIndex = FMath::Max(0, Project->ContributionHistory.Num() - MaxEntries);
		for (int32 i = Project->ContributionHistory.Num() - 1; i >= StartIndex; --i)
		{
			Result.Add(Project->ContributionHistory[i]);
		}
	}
	return Result;
}

bool UCooperativeProjectSystem::CanContribute(const FGuid& ProjectID, const FString& PlayerID) const
{
	// Note: Lock should be held by caller
	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	// Public projects allow anyone
	if (Project->bAllowPublicContributions || Project->Visibility == EProjectVisibility::Public)
	{
		return true;
	}

	// Check guild membership
	if (Project->OwnerGuildID.IsValid() && GuildManager)
	{
		FGuid PlayerGuild = GuildManager->GetPlayerGuild(PlayerID);
		if (PlayerGuild == Project->OwnerGuildID)
		{
			return true;
		}

		// Check for allied guilds
		if (Project->Visibility == EProjectVisibility::Allied)
		{
			EGuildRelationship Relationship = GuildManager->GetGuildRelationship(
				Project->OwnerGuildID, PlayerGuild);
			if (Relationship == EGuildRelationship::Allied)
			{
				return true;
			}
		}
	}

	// Creator can always contribute
	if (Project->CreatorPlayerID == PlayerID)
	{
		return true;
	}

	return false;
}

// ==================== Progress & Completion ====================

float UCooperativeProjectSystem::GetProjectProgress(const FGuid& ProjectID) const
{
	FScopeLock Lock(&ProjectLock);

	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (Project)
	{
		return Project->GetOverallProgress();
	}
	return 0.0f;
}

float UCooperativeProjectSystem::GetCurrentMilestoneProgress(const FGuid& ProjectID) const
{
	FScopeLock Lock(&ProjectLock);

	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (Project)
	{
		const FProjectMilestone* Milestone = Project->GetCurrentMilestone();
		if (Milestone)
		{
			return Milestone->GetOverallCompletion();
		}
	}
	return 0.0f;
}

bool UCooperativeProjectSystem::CheckMilestoneCompletion(const FGuid& ProjectID)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	FProjectMilestone* CurrentMilestone = Project->GetCurrentMilestone();
	if (!CurrentMilestone || CurrentMilestone->bIsComplete)
	{
		return false;
	}

	// Check if all required resources are contributed
	bool bAllComplete = true;
	for (const FProjectResourceRequirement& Req : CurrentMilestone->Requirements)
	{
		if (!Req.bIsOptional && !Req.IsComplete())
		{
			bAllComplete = false;
			break;
		}
	}

	if (bAllComplete)
	{
		CurrentMilestone->bIsComplete = true;
		CurrentMilestone->CompletedAt = FDateTime::Now();

		OnMilestoneCompleted.Broadcast(ProjectID, Project->CurrentMilestoneIndex, CurrentMilestone->MilestoneName);

		// Move to next milestone
		Project->CurrentMilestoneIndex++;

		// Check if project is complete
		if (Project->CurrentMilestoneIndex >= Project->Milestones.Num())
		{
			CheckAndCompleteProject(*Project);
		}
		else
		{
			// Move to construction phase after first milestone
			if (Project->State == EProjectState::ResourceGathering)
			{
				ChangeProjectState(*Project, EProjectState::Construction);
			}
		}

		return true;
	}

	return false;
}

bool UCooperativeProjectSystem::ForceCompleteProject(const FGuid& ProjectID)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	// Mark all milestones complete
	for (FProjectMilestone& Milestone : Project->Milestones)
	{
		Milestone.bIsComplete = true;
		Milestone.CompletedAt = FDateTime::Now();
	}

	Project->CurrentMilestoneIndex = Project->Milestones.Num();
	CheckAndCompleteProject(*Project);

	return true;
}

void UCooperativeProjectSystem::CheckAndCompleteProject(FCooperativeProject& Project)
{
	if (Project.State == EProjectState::Completed)
	{
		return;
	}

	// Verify all milestones are complete
	for (const FProjectMilestone& Milestone : Project.Milestones)
	{
		if (!Milestone.bIsComplete)
		{
			return;
		}
	}

	// Check minimum contributors
	if (Project.Contributors.Num() < Project.MinContributors)
	{
		UE_LOG(LogTemp, Warning, TEXT("Project %s needs %d contributors but only has %d"),
			*Project.ProjectName, Project.MinContributors, Project.Contributors.Num());
		return;
	}

	Project.CompletedAt = FDateTime::Now();
	ChangeProjectState(Project, EProjectState::Completed);

	OnProjectCompleted.Broadcast(Project.ProjectID, Project.ProjectName);

	UE_LOG(LogTemp, Log, TEXT("Project '%s' completed!"), *Project.ProjectName);
}

// ==================== Rewards ====================

bool UCooperativeProjectSystem::SetRewardConfig(const FGuid& ProjectID, const FString& PlayerID,
	const FProjectRewardConfig& Config)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	Project->RewardConfig = Config;
	return true;
}

bool UCooperativeProjectSystem::AddToRewardPool(const FGuid& ProjectID, const FString& PlayerID,
	EResourceType ResourceType, int64 Amount)
{
	FScopeLock Lock(&ProjectLock);

	if (Amount <= 0)
	{
		return false;
	}

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	if (!IsProjectCreatorOrManager(*Project, PlayerID))
	{
		return false;
	}

	if (!Project->RewardPool.Contains(ResourceType))
	{
		Project->RewardPool.Add(ResourceType, 0);
	}
	Project->RewardPool[ResourceType] += Amount;

	return true;
}

TMap<EResourceType, int64> UCooperativeProjectSystem::CalculateContributorRewards(const FGuid& ProjectID,
	const FString& PlayerID) const
{
	FScopeLock Lock(&ProjectLock);

	TMap<EResourceType, int64> Rewards;

	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return Rewards;
	}

	const FProjectContributorSummary* Summary = Project->Contributors.Find(PlayerID);
	if (!Summary)
	{
		return Rewards;
	}

	const FProjectRewardConfig& Config = Project->RewardConfig;

	for (const auto& PoolPair : Project->RewardPool)
	{
		int64 BaseReward = 0;

		if (Config.bDistributeByContribution)
		{
			// Proportional to contribution
			BaseReward = static_cast<int64>(PoolPair.Value * (Summary->ContributionPercentage / 100.0f));
		}
		else
		{
			// Equal share
			int32 ContributorCount = FMath::Max(1, Project->Contributors.Num());
			BaseReward = PoolPair.Value / ContributorCount;
		}

		// Apply tier multiplier
		float Multiplier = Config.GetTierMultiplier(Summary->Tier);
		int64 FinalReward = static_cast<int64>(BaseReward * Multiplier);

		// Add participation bonus
		FinalReward += Config.ParticipationBonus;

		Rewards.Add(PoolPair.Key, FinalReward);
	}

	return Rewards;
}

bool UCooperativeProjectSystem::DistributeRewards(const FGuid& ProjectID)
{
	FScopeLock Lock(&ProjectLock);

	FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project || Project->State != EProjectState::Completed)
	{
		return false;
	}

	// TODO: Actually distribute rewards to players via inventory system
	// For now, just log the distributions
	for (const auto& ContribPair : Project->Contributors)
	{
		TMap<EResourceType, int64> Rewards = CalculateContributorRewards(ProjectID, ContribPair.Key);

		UE_LOG(LogTemp, Log, TEXT("Rewards for %s in project %s:"),
			*ContribPair.Value.PlayerName, *Project->ProjectName);

		for (const auto& RewardPair : Rewards)
		{
			UE_LOG(LogTemp, Log, TEXT("  - Resource %d: %lld"),
				static_cast<int32>(RewardPair.Key), RewardPair.Value);
		}
	}

	return true;
}

// ==================== Access Control ====================

bool UCooperativeProjectSystem::HasProjectAccess(const FGuid& ProjectID, const FString& PlayerID) const
{
	FScopeLock Lock(&ProjectLock);

	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	// Creator always has access
	if (Project->CreatorPlayerID == PlayerID)
	{
		return true;
	}

	// Public projects are viewable by all
	if (Project->Visibility == EProjectVisibility::Public)
	{
		return true;
	}

	// Check guild membership
	if (Project->OwnerGuildID.IsValid() && GuildManager)
	{
		FGuid PlayerGuild = GuildManager->GetPlayerGuild(PlayerID);
		if (PlayerGuild == Project->OwnerGuildID)
		{
			return true;
		}

		if (Project->Visibility == EProjectVisibility::Allied)
		{
			EGuildRelationship Relationship = GuildManager->GetGuildRelationship(
				Project->OwnerGuildID, PlayerGuild);
			if (Relationship == EGuildRelationship::Allied)
			{
				return true;
			}
		}
	}

	return false;
}

bool UCooperativeProjectSystem::CanManageProject(const FGuid& ProjectID, const FString& PlayerID) const
{
	FScopeLock Lock(&ProjectLock);

	const FCooperativeProject* Project = Projects.Find(ProjectID);
	if (!Project)
	{
		return false;
	}

	return IsProjectCreatorOrManager(*Project, PlayerID);
}

// ==================== Templates ====================

TArray<FProjectTemplate> UCooperativeProjectSystem::GetAvailableTemplates(const FGuid& GuildID) const
{
	TArray<FProjectTemplate> Result;

	if (!ProjectTemplateTable)
	{
		return Result;
	}

	int32 GuildLevel = 1;
	if (GuildID.IsValid() && GuildManager)
	{
		FGuildData GuildData;
		if (GuildManager->GetGuildData(GuildID, GuildData))
		{
			GuildLevel = GuildData.Level;
		}
	}

	TArray<FProjectTemplate*> AllTemplates;
	ProjectTemplateTable->GetAllRows<FProjectTemplate>(TEXT("GetAvailableTemplates"), AllTemplates);

	for (FProjectTemplate* Template : AllTemplates)
	{
		if (Template && Template->MinGuildLevel <= GuildLevel)
		{
			Result.Add(*Template);
		}
	}

	return Result;
}

bool UCooperativeProjectSystem::GetTemplate(FName TemplateID, FProjectTemplate& OutTemplate) const
{
	if (!ProjectTemplateTable)
	{
		return false;
	}

	FProjectTemplate* Template = ProjectTemplateTable->FindRow<FProjectTemplate>(TemplateID, TEXT("GetTemplate"));
	if (Template)
	{
		OutTemplate = *Template;
		return true;
	}
	return false;
}

// ==================== Resource Value Calculation ====================

int64 UCooperativeProjectSystem::GetResourceValue(EResourceType ResourceType, int64 Amount) const
{
	const int64* BaseValue = ResourceBaseValues.Find(ResourceType);
	if (BaseValue)
	{
		return (*BaseValue) * Amount;
	}
	return Amount; // Default to 1:1 if not found
}

// ==================== Internal Helpers ====================

void UCooperativeProjectSystem::UpdateContributorSummaries(FCooperativeProject& Project)
{
	int64 TotalValue = Project.GetTotalValueContributed();
	if (TotalValue <= 0)
	{
		return;
	}

	for (auto& Pair : Project.Contributors)
	{
		FProjectContributorSummary& Summary = Pair.Value;
		Summary.ContributionPercentage = (static_cast<float>(Summary.TotalValueContributed) /
			static_cast<float>(TotalValue)) * 100.0f;
		Summary.UpdateTier();
	}
}

void UCooperativeProjectSystem::ChangeProjectState(FCooperativeProject& Project, EProjectState NewState)
{
	EProjectState OldState = Project.State;
	Project.State = NewState;
	OnProjectStateChanged.Broadcast(Project.ProjectID, OldState, NewState);
}

bool UCooperativeProjectSystem::IsProjectCreatorOrManager(const FCooperativeProject& Project,
	const FString& PlayerID) const
{
	// Creator has full access
	if (Project.CreatorPlayerID == PlayerID)
	{
		return true;
	}

	// Check if player has management rights based on contribution tier
	const FProjectContributorSummary* Summary = Project.Contributors.Find(PlayerID);
	if (Summary && Summary->Tier >= Project.RewardConfig.MinTierForManagement)
	{
		return true;
	}

	// Check guild permissions
	if (Project.OwnerGuildID.IsValid() && GuildManager)
	{
		return GuildManager->HasPermission(Project.OwnerGuildID, PlayerID, EGuildPermission::ManageProjects);
	}

	return false;
}
