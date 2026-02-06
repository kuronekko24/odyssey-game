// OdysseyCraftingSkillSystem.cpp
// Implementation of crafting skill progression and mastery system
// Drives economic demand by gating recipes behind skill trees and rewarding specialization

#include "OdysseyCraftingSkillSystem.h"
#include "OdysseyCraftingManager.h"

UOdysseyCraftingSkillSystem::UOdysseyCraftingSkillSystem()
{
	PrimaryComponentTick.bCanEverTick = false;

	ExperienceCurveMultiplier = 1.5f;
	SkillPointsPerLevel = 1;
	RespecCostMultiplier = 1.0f;
	TotalExperience = 0;

	CraftingManager = nullptr;
}

void UOdysseyCraftingSkillSystem::BeginPlay()
{
	Super::BeginPlay();

	InitializeDefaultSkills();
	InitializeDefaultMasteries();
	UpdateSkillPoints();
}

// ============================================================================
// Skill Management
// ============================================================================

FCraftingSkill UOdysseyCraftingSkillSystem::GetSkill(FName SkillID) const
{
	const FCraftingSkill* Skill = Skills.Find(SkillID);
	return Skill ? *Skill : FCraftingSkill();
}

TArray<FCraftingSkill> UOdysseyCraftingSkillSystem::GetAllSkills() const
{
	TArray<FCraftingSkill> AllSkills;
	for (const auto& Pair : Skills)
	{
		AllSkills.Add(Pair.Value);
	}
	return AllSkills;
}

TArray<FCraftingSkill> UOdysseyCraftingSkillSystem::GetSkillsByCategory(ECraftingSkillCategory Category) const
{
	TArray<FCraftingSkill> CategorySkills;
	for (const auto& Pair : Skills)
	{
		if (Pair.Value.Category == Category)
		{
			CategorySkills.Add(Pair.Value);
		}
	}
	return CategorySkills;
}

int32 UOdysseyCraftingSkillSystem::GetSkillLevel(FName SkillID) const
{
	const FCraftingSkill* Skill = Skills.Find(SkillID);
	if (Skill && UnlockedSkills.Contains(SkillID))
	{
		return Skill->CurrentLevel;
	}
	return 0;
}

bool UOdysseyCraftingSkillSystem::IsSkillUnlocked(FName SkillID) const
{
	return UnlockedSkills.Contains(SkillID);
}

bool UOdysseyCraftingSkillSystem::CanUnlockSkill(FName SkillID) const
{
	if (UnlockedSkills.Contains(SkillID))
	{
		return false; // Already unlocked
	}

	const FCraftingSkill* Skill = Skills.Find(SkillID);
	if (!Skill)
	{
		return false;
	}

	// Check prerequisites
	for (const auto& Prereq : Skill->RequiredSkillLevels)
	{
		if (!UnlockedSkills.Contains(Prereq.Key))
		{
			return false;
		}

		const FCraftingSkill* PrereqSkill = Skills.Find(Prereq.Key);
		if (!PrereqSkill || PrereqSkill->CurrentLevel < Prereq.Value)
		{
			return false;
		}
	}

	// Check if player has available skill points
	if (SkillPoints.AvailableSkillPoints <= 0)
	{
		return false;
	}

	return true;
}

bool UOdysseyCraftingSkillSystem::UnlockSkill(FName SkillID)
{
	if (!CanUnlockSkill(SkillID))
	{
		return false;
	}

	UnlockedSkills.Add(SkillID);

	// Spend a skill point to unlock
	SkillPoints.AvailableSkillPoints--;
	SkillPoints.SpentSkillPoints++;

	// Set initial level to 1
	FCraftingSkill* Skill = Skills.Find(SkillID);
	if (Skill && Skill->CurrentLevel == 0)
	{
		Skill->CurrentLevel = 1;
		Skill->CurrentExperience = 0;
		Skill->ExperienceToNextLevel = CalculateExperienceForLevel(2);
	}

	// Unlock any recipes associated with this skill
	if (CraftingManager && Skill)
	{
		for (const FName& RecipeID : Skill->UnlocksRecipes)
		{
			CraftingManager->UnlockRecipe(RecipeID, SkillID);
		}
	}

	OnSkillUnlocked.Broadcast(SkillID, NAME_None);

	// Check mastery unlocks
	CheckMasteryUnlocks();

	return true;
}

// ============================================================================
// Experience System
// ============================================================================

void UOdysseyCraftingSkillSystem::AddSkillExperience(FName SkillID, int32 Experience)
{
	if (Experience <= 0)
	{
		return;
	}

	FCraftingSkill* Skill = Skills.Find(SkillID);
	if (!Skill)
	{
		return;
	}

	// Must be unlocked to gain experience
	if (!UnlockedSkills.Contains(SkillID))
	{
		return;
	}

	// Cannot gain experience past max level
	if (Skill->CurrentLevel >= Skill->MaxLevel)
	{
		return;
	}

	Skill->CurrentExperience += Experience;
	TotalExperience += Experience;

	OnSkillExperienceGained.Broadcast(SkillID, Experience);

	// Check for level ups (can gain multiple levels at once)
	while (Skill->CurrentLevel < Skill->MaxLevel &&
		   Skill->CurrentExperience >= Skill->ExperienceToNextLevel)
	{
		LevelUpSkill(SkillID);
	}
}

void UOdysseyCraftingSkillSystem::AddCraftingExperience(FName RecipeID, int32 Quantity, EItemQuality Quality)
{
	if (!CraftingManager)
	{
		return;
	}

	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		return;
	}

	// Quality multiplier for experience
	float QualityMultiplier = 1.0f + (static_cast<float>(Quality) * 0.15f);

	// Distribute experience to relevant skills
	for (const auto& SkillExp : Recipe.SkillExperienceRewards)
	{
		int32 TotalXP = FMath::CeilToInt(SkillExp.Value * Quantity * QualityMultiplier);
		AddSkillExperience(SkillExp.Key, TotalXP);
	}

	// Also add a small amount to general crafting skill
	int32 GeneralXP = FMath::CeilToInt(Recipe.BaseExperienceReward * Quantity * QualityMultiplier * 0.5f);
	AddSkillExperience(FName(TEXT("GeneralCrafting")), GeneralXP);
}

FSkillProgressInfo UOdysseyCraftingSkillSystem::GetSkillProgress(FName SkillID) const
{
	FSkillProgressInfo Info;
	Info.SkillID = SkillID;

	const FCraftingSkill* Skill = Skills.Find(SkillID);
	if (Skill)
	{
		Info.Level = Skill->CurrentLevel;
		Info.TotalExperienceGained = Skill->CurrentExperience;

		if (Skill->ExperienceToNextLevel > 0)
		{
			Info.ProgressToNextLevel = static_cast<float>(Skill->CurrentExperience) / Skill->ExperienceToNextLevel;
		}
		else
		{
			Info.ProgressToNextLevel = 1.0f;
		}
	}

	return Info;
}

int32 UOdysseyCraftingSkillSystem::CalculateExperienceForLevel(int32 Level) const
{
	if (Level <= 1)
	{
		return 0;
	}

	// Exponential curve: BaseXP * Level^ExperienceCurveMultiplier
	float BaseXP = 100.0f;
	return FMath::CeilToInt(BaseXP * FMath::Pow(static_cast<float>(Level), ExperienceCurveMultiplier));
}

// ============================================================================
// Skill Points
// ============================================================================

FSkillPointAllocation UOdysseyCraftingSkillSystem::GetSkillPointAllocation() const
{
	return SkillPoints;
}

bool UOdysseyCraftingSkillSystem::SpendSkillPoint(FName SkillID)
{
	if (SkillPoints.AvailableSkillPoints <= 0)
	{
		return false;
	}

	FCraftingSkill* Skill = Skills.Find(SkillID);
	if (!Skill)
	{
		return false;
	}

	// If not unlocked, try to unlock
	if (!UnlockedSkills.Contains(SkillID))
	{
		return UnlockSkill(SkillID);
	}

	// If already at max level, cannot spend point
	if (Skill->CurrentLevel >= Skill->MaxLevel)
	{
		return false;
	}

	// Level up with skill point
	int32 OldLevel = Skill->CurrentLevel;
	Skill->CurrentLevel++;
	Skill->ExperienceToNextLevel = CalculateExperienceForLevel(Skill->CurrentLevel + 1);
	Skill->CurrentExperience = 0;

	SkillPoints.AvailableSkillPoints--;
	SkillPoints.SpentSkillPoints++;

	OnSkillLevelUp.Broadcast(SkillID, OldLevel, Skill->CurrentLevel);

	// Unlock recipes tied to this skill level
	if (CraftingManager)
	{
		for (const FName& RecipeID : Skill->UnlocksRecipes)
		{
			FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);
			const int32* ReqLevel = Recipe.RequiredSkillLevels.Find(SkillID);
			if (ReqLevel && Skill->CurrentLevel >= *ReqLevel)
			{
				CraftingManager->UnlockRecipe(RecipeID, SkillID);
			}
		}
	}

	CheckMasteryUnlocks();

	return true;
}

bool UOdysseyCraftingSkillSystem::ResetSkillPoints()
{
	// Reset all skill levels to 0
	for (auto& Pair : Skills)
	{
		Pair.Value.CurrentLevel = 0;
		Pair.Value.CurrentExperience = 0;
		Pair.Value.ExperienceToNextLevel = CalculateExperienceForLevel(1);
	}

	// Reset unlocked skills (keep only default unlocks)
	TSet<FName> DefaultSkills;
	for (const auto& Pair : Skills)
	{
		if (Pair.Value.RequiredSkillLevels.Num() == 0)
		{
			DefaultSkills.Add(Pair.Key);
		}
	}
	UnlockedSkills = DefaultSkills;

	// Refund all skill points
	SkillPoints.AvailableSkillPoints = SkillPoints.TotalSkillPoints;
	SkillPoints.SpentSkillPoints = 0;

	// Reset mastery unlocks
	UnlockedMasteries.Empty();

	return true;
}

void UOdysseyCraftingSkillSystem::AddSkillPoints(int32 Points)
{
	if (Points <= 0)
	{
		return;
	}

	SkillPoints.TotalSkillPoints += Points;
	SkillPoints.AvailableSkillPoints += Points;

	OnSkillPointsEarned.Broadcast(Points, SkillPoints.TotalSkillPoints);
}

// ============================================================================
// Crafting Bonuses
// ============================================================================

float UOdysseyCraftingSkillSystem::GetCraftingSpeedBonus() const
{
	float TotalBonus = 0.0f;

	for (const auto& Pair : Skills)
	{
		if (UnlockedSkills.Contains(Pair.Key))
		{
			TotalBonus += Pair.Value.CurrentLevel * Pair.Value.SpeedBonusPerLevel;
		}
	}

	// Apply mastery speed multipliers
	for (const auto& MasteryPair : Masteries)
	{
		if (UnlockedMasteries.Contains(MasteryPair.Key))
		{
			TotalBonus *= MasteryPair.Value.SpeedMultiplier;
		}
	}

	return FMath::Min(TotalBonus, 0.75f); // Cap at 75% speed bonus
}

float UOdysseyCraftingSkillSystem::GetCraftingQualityBonus() const
{
	float TotalBonus = 0.0f;

	for (const auto& Pair : Skills)
	{
		if (UnlockedSkills.Contains(Pair.Key))
		{
			TotalBonus += Pair.Value.CurrentLevel * Pair.Value.QualityBonusPerLevel;
		}
	}

	// Apply mastery quality multipliers
	for (const auto& MasteryPair : Masteries)
	{
		if (UnlockedMasteries.Contains(MasteryPair.Key))
		{
			TotalBonus *= MasteryPair.Value.QualityMultiplier;
		}
	}

	return FMath::Min(TotalBonus, 0.50f); // Cap at 50% quality bonus
}

float UOdysseyCraftingSkillSystem::GetCraftingSuccessBonus() const
{
	float TotalBonus = 0.0f;

	for (const auto& Pair : Skills)
	{
		if (UnlockedSkills.Contains(Pair.Key))
		{
			TotalBonus += Pair.Value.CurrentLevel * Pair.Value.SuccessBonusPerLevel;
		}
	}

	return FMath::Min(TotalBonus, 0.10f); // Cap at 10% success bonus (base is already 90%)
}

float UOdysseyCraftingSkillSystem::GetMaterialEfficiencyBonus() const
{
	float TotalBonus = 0.0f;

	for (const auto& Pair : Skills)
	{
		if (UnlockedSkills.Contains(Pair.Key))
		{
			TotalBonus += Pair.Value.CurrentLevel * Pair.Value.MaterialEfficiencyPerLevel;
		}
	}

	return FMath::Min(TotalBonus, 0.30f); // Cap at 30% material savings
}

float UOdysseyCraftingSkillSystem::GetCategoryBonus(ECraftingSkillCategory Category, FName BonusType) const
{
	float TotalBonus = 0.0f;

	for (const auto& Pair : Skills)
	{
		if (Pair.Value.Category != Category || !UnlockedSkills.Contains(Pair.Key))
		{
			continue;
		}

		if (BonusType == FName(TEXT("Speed")))
		{
			TotalBonus += Pair.Value.CurrentLevel * Pair.Value.SpeedBonusPerLevel;
		}
		else if (BonusType == FName(TEXT("Quality")))
		{
			TotalBonus += Pair.Value.CurrentLevel * Pair.Value.QualityBonusPerLevel;
		}
		else if (BonusType == FName(TEXT("Success")))
		{
			TotalBonus += Pair.Value.CurrentLevel * Pair.Value.SuccessBonusPerLevel;
		}
		else if (BonusType == FName(TEXT("Efficiency")))
		{
			TotalBonus += Pair.Value.CurrentLevel * Pair.Value.MaterialEfficiencyPerLevel;
		}
	}

	return TotalBonus;
}

float UOdysseyCraftingSkillSystem::GetRecipeSkillBonus(FName RecipeID, FName BonusType) const
{
	TArray<FName> RelevantSkills = GetRelevantSkillsForRecipe(RecipeID);
	float TotalBonus = 0.0f;

	for (const FName& SkillID : RelevantSkills)
	{
		const FCraftingSkill* Skill = Skills.Find(SkillID);
		if (!Skill || !UnlockedSkills.Contains(SkillID))
		{
			continue;
		}

		if (BonusType == FName(TEXT("Speed")))
		{
			TotalBonus += Skill->CurrentLevel * Skill->SpeedBonusPerLevel;
		}
		else if (BonusType == FName(TEXT("Quality")))
		{
			TotalBonus += Skill->CurrentLevel * Skill->QualityBonusPerLevel;
		}
		else if (BonusType == FName(TEXT("Success")))
		{
			TotalBonus += Skill->CurrentLevel * Skill->SuccessBonusPerLevel;
		}
		else if (BonusType == FName(TEXT("Efficiency")))
		{
			TotalBonus += Skill->CurrentLevel * Skill->MaterialEfficiencyPerLevel;
		}
	}

	return TotalBonus;
}

// ============================================================================
// Mastery System
// ============================================================================

TArray<FCraftingMasteryBonus> UOdysseyCraftingSkillSystem::GetAllMasteries() const
{
	TArray<FCraftingMasteryBonus> AllMasteries;
	for (const auto& Pair : Masteries)
	{
		FCraftingMasteryBonus Mastery = Pair.Value;
		Mastery.bIsUnlocked = UnlockedMasteries.Contains(Pair.Key);
		AllMasteries.Add(Mastery);
	}
	return AllMasteries;
}

FCraftingMasteryBonus UOdysseyCraftingSkillSystem::GetMastery(FName MasteryID) const
{
	const FCraftingMasteryBonus* Mastery = Masteries.Find(MasteryID);
	if (Mastery)
	{
		FCraftingMasteryBonus Result = *Mastery;
		Result.bIsUnlocked = UnlockedMasteries.Contains(MasteryID);
		return Result;
	}
	return FCraftingMasteryBonus();
}

bool UOdysseyCraftingSkillSystem::IsMasteryUnlocked(FName MasteryID) const
{
	return UnlockedMasteries.Contains(MasteryID);
}

float UOdysseyCraftingSkillSystem::GetMasteryProgress(FName MasteryID) const
{
	const FCraftingMasteryBonus* Mastery = Masteries.Find(MasteryID);
	if (!Mastery)
	{
		return 0.0f;
	}

	int32 CurrentLevels = GetTotalLevelsInCategory(Mastery->Category);
	return FMath::Min(static_cast<float>(CurrentLevels) / Mastery->RequiredTotalLevels, 1.0f);
}

int32 UOdysseyCraftingSkillSystem::GetTotalLevelsInCategory(ECraftingSkillCategory Category) const
{
	int32 TotalLevels = 0;

	for (const auto& Pair : Skills)
	{
		if (Pair.Value.Category == Category && UnlockedSkills.Contains(Pair.Key))
		{
			TotalLevels += Pair.Value.CurrentLevel;
		}
	}

	return TotalLevels;
}

TArray<FCraftingMasteryBonus> UOdysseyCraftingSkillSystem::GetActiveMasteryBonuses() const
{
	TArray<FCraftingMasteryBonus> ActiveMasteries;
	for (const FName& MasteryID : UnlockedMasteries)
	{
		const FCraftingMasteryBonus* Mastery = Masteries.Find(MasteryID);
		if (Mastery)
		{
			ActiveMasteries.Add(*Mastery);
		}
	}
	return ActiveMasteries;
}

// ============================================================================
// Skill Tree
// ============================================================================

TArray<FSkillTreeNode> UOdysseyCraftingSkillSystem::GetSkillTreeNodes(ECraftingSkillCategory Category) const
{
	TArray<FSkillTreeNode> TreeNodes;

	// Layout positions based on skill dependencies
	int32 Row = 0;
	int32 Col = 0;

	for (const auto& Pair : Skills)
	{
		if (Pair.Value.Category != Category)
		{
			continue;
		}

		FSkillTreeNode Node;
		Node.SkillID = Pair.Key;
		Node.bIsUnlocked = UnlockedSkills.Contains(Pair.Key);
		Node.bCanUnlock = CanUnlockSkill(Pair.Key);

		// Calculate position based on prerequisite depth
		int32 Depth = 0;
		for (const auto& Prereq : Pair.Value.RequiredSkillLevels)
		{
			Depth = FMath::Max(Depth, 1);
		}

		Node.Position = FVector2D(Depth * 200.0f, Col * 120.0f);
		Col++;

		// Add connections from prerequisites
		for (const auto& Prereq : Pair.Value.RequiredSkillLevels)
		{
			Node.ConnectedSkills.Add(Prereq.Key);
		}

		// Add connections to skills this unlocks
		for (const FName& UnlockSkillID : Pair.Value.UnlocksSkills)
		{
			Node.ConnectedSkills.AddUnique(UnlockSkillID);
		}

		TreeNodes.Add(Node);
	}

	return TreeNodes;
}

TArray<FName> UOdysseyCraftingSkillSystem::GetSkillPrerequisites(FName SkillID) const
{
	TArray<FName> Prerequisites;

	const FCraftingSkill* Skill = Skills.Find(SkillID);
	if (Skill)
	{
		for (const auto& Prereq : Skill->RequiredSkillLevels)
		{
			Prerequisites.Add(Prereq.Key);
		}
	}

	return Prerequisites;
}

TArray<FName> UOdysseyCraftingSkillSystem::GetUnlockedBySkill(FName SkillID) const
{
	TArray<FName> UnlockedBy;

	const FCraftingSkill* Skill = Skills.Find(SkillID);
	if (Skill)
	{
		UnlockedBy = Skill->UnlocksSkills;
	}

	return UnlockedBy;
}

// ============================================================================
// Statistics
// ============================================================================

int32 UOdysseyCraftingSkillSystem::GetTotalSkillLevels() const
{
	int32 Total = 0;
	for (const auto& Pair : Skills)
	{
		if (UnlockedSkills.Contains(Pair.Key))
		{
			Total += Pair.Value.CurrentLevel;
		}
	}
	return Total;
}

int32 UOdysseyCraftingSkillSystem::GetTotalExperienceEarned() const
{
	return TotalExperience;
}

int32 UOdysseyCraftingSkillSystem::GetHighestSkillLevel() const
{
	int32 Highest = 0;
	for (const auto& Pair : Skills)
	{
		if (UnlockedSkills.Contains(Pair.Key))
		{
			Highest = FMath::Max(Highest, Pair.Value.CurrentLevel);
		}
	}
	return Highest;
}

int32 UOdysseyCraftingSkillSystem::GetUnlockedSkillsCount() const
{
	return UnlockedSkills.Num();
}

// ============================================================================
// Integration
// ============================================================================

void UOdysseyCraftingSkillSystem::SetCraftingManager(UOdysseyCraftingManager* NewManager)
{
	CraftingManager = NewManager;
}

// ============================================================================
// Internal Methods
// ============================================================================

void UOdysseyCraftingSkillSystem::InitializeDefaultSkills()
{
	if (Skills.Num() > 0)
	{
		return; // Already initialized (possibly from data table)
	}

	// Load from data table if available
	if (SkillDataTable)
	{
		TArray<FName> RowNames = SkillDataTable->GetRowNames();
		for (const FName& RowName : RowNames)
		{
			FCraftingSkill* Skill = SkillDataTable->FindRow<FCraftingSkill>(RowName, TEXT("InitializeDefaultSkills"));
			if (Skill)
			{
				Skills.Add(Skill->SkillID, *Skill);
				// Auto-unlock skills with no prerequisites
				if (Skill->RequiredSkillLevels.Num() == 0)
				{
					UnlockedSkills.Add(Skill->SkillID);
				}
			}
		}
		return;
	}

	// ---- General Skills ----
	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("GeneralCrafting"));
		Skill.SkillName = TEXT("General Crafting");
		Skill.Description = TEXT("Foundational crafting knowledge. Improves all crafting activities.");
		Skill.Category = ECraftingSkillCategory::General;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.02f;
		Skill.QualityBonusPerLevel = 0.01f;
		Skill.SuccessBonusPerLevel = 0.01f;
		Skill.MaterialEfficiencyPerLevel = 0.005f;
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
		UnlockedSkills.Add(Skill.SkillID);
	}

	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("Precision"));
		Skill.SkillName = TEXT("Precision");
		Skill.Description = TEXT("Careful crafting technique. Increases critical craft chance and quality.");
		Skill.Category = ECraftingSkillCategory::General;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.0f;
		Skill.QualityBonusPerLevel = 0.025f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.0f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("GeneralCrafting")), 3);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("MaterialEfficiency"));
		Skill.SkillName = TEXT("Material Efficiency");
		Skill.Description = TEXT("Waste reduction techniques. Reduces material consumption per craft.");
		Skill.Category = ECraftingSkillCategory::General;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.0f;
		Skill.QualityBonusPerLevel = 0.0f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.02f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("GeneralCrafting")), 3);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("CraftingMastery"));
		Skill.SkillName = TEXT("Crafting Mastery");
		Skill.Description = TEXT("Advanced production knowledge. Bonus output chance and efficiency.");
		Skill.Category = ECraftingSkillCategory::General;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.015f;
		Skill.QualityBonusPerLevel = 0.015f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.01f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("Precision")), 3);
		Skill.RequiredSkillLevels.Add(FName(TEXT("MaterialEfficiency")), 3);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	// ---- Material Processing Skills ----
	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("Refining"));
		Skill.SkillName = TEXT("Refining");
		Skill.Description = TEXT("Raw material processing. Faster and more efficient ore refining.");
		Skill.Category = ECraftingSkillCategory::MaterialProcessing;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.03f;
		Skill.QualityBonusPerLevel = 0.01f;
		Skill.SuccessBonusPerLevel = 0.01f;
		Skill.MaterialEfficiencyPerLevel = 0.015f;
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
		UnlockedSkills.Add(Skill.SkillID);
	}

	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("Alloying"));
		Skill.SkillName = TEXT("Alloying");
		Skill.Description = TEXT("Metal combination mastery. Unlock and improve alloy recipes.");
		Skill.Category = ECraftingSkillCategory::MaterialProcessing;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.02f;
		Skill.QualityBonusPerLevel = 0.02f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.01f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("Refining")), 3);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("Composites"));
		Skill.SkillName = TEXT("Composite Materials");
		Skill.Description = TEXT("Advanced material science. Create high-performance composites.");
		Skill.Category = ECraftingSkillCategory::MaterialProcessing;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.015f;
		Skill.QualityBonusPerLevel = 0.025f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.01f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("Alloying")), 5);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	// ---- Weapon Crafting Skills ----
	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("SmallArms"));
		Skill.SkillName = TEXT("Small Arms Manufacturing");
		Skill.Description = TEXT("Personal weapon production. Craft pistols, rifles, and melee weapons.");
		Skill.Category = ECraftingSkillCategory::WeaponCrafting;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.02f;
		Skill.QualityBonusPerLevel = 0.02f;
		Skill.SuccessBonusPerLevel = 0.01f;
		Skill.MaterialEfficiencyPerLevel = 0.005f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("GeneralCrafting")), 2);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("HeavyWeaponry"));
		Skill.SkillName = TEXT("Heavy Weaponry");
		Skill.Description = TEXT("Ship-mounted weapon systems. Craft turrets, missiles, and beam weapons.");
		Skill.Category = ECraftingSkillCategory::WeaponCrafting;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.015f;
		Skill.QualityBonusPerLevel = 0.025f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.01f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("SmallArms")), 5);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("Ammunition"));
		Skill.SkillName = TEXT("Ammunition Production");
		Skill.Description = TEXT("Mass ammunition manufacturing. Critical for sustained combat supply.");
		Skill.Category = ECraftingSkillCategory::WeaponCrafting;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.04f;
		Skill.QualityBonusPerLevel = 0.01f;
		Skill.SuccessBonusPerLevel = 0.01f;
		Skill.MaterialEfficiencyPerLevel = 0.02f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("SmallArms")), 3);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	// ---- Ship Module Skills ----
	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("ShipSystems"));
		Skill.SkillName = TEXT("Ship Systems Engineering");
		Skill.Description = TEXT("Core ship module production. Engines, shields, and hull components.");
		Skill.Category = ECraftingSkillCategory::ShipModules;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.02f;
		Skill.QualityBonusPerLevel = 0.02f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.01f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("GeneralCrafting")), 3);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("AdvancedPropulsion"));
		Skill.SkillName = TEXT("Advanced Propulsion");
		Skill.Description = TEXT("Cutting-edge drive systems. Warp drives and thruster optimization.");
		Skill.Category = ECraftingSkillCategory::ShipModules;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.015f;
		Skill.QualityBonusPerLevel = 0.03f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.005f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("ShipSystems")), 5);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	// ---- Electronics Skills ----
	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("Electronics"));
		Skill.SkillName = TEXT("Electronics");
		Skill.Description = TEXT("Circuit and sensor manufacturing. Components for advanced systems.");
		Skill.Category = ECraftingSkillCategory::Electronics;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.02f;
		Skill.QualityBonusPerLevel = 0.02f;
		Skill.SuccessBonusPerLevel = 0.01f;
		Skill.MaterialEfficiencyPerLevel = 0.01f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("GeneralCrafting")), 2);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	// ---- Chemistry Skills ----
	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("Chemistry"));
		Skill.SkillName = TEXT("Chemistry");
		Skill.Description = TEXT("Chemical processing and synthesis. Fuels, medicines, and catalysts.");
		Skill.Category = ECraftingSkillCategory::Chemistry;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.025f;
		Skill.QualityBonusPerLevel = 0.015f;
		Skill.SuccessBonusPerLevel = 0.01f;
		Skill.MaterialEfficiencyPerLevel = 0.015f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("GeneralCrafting")), 2);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	// ---- Research Skills ----
	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("Research"));
		Skill.SkillName = TEXT("Research");
		Skill.Description = TEXT("Scientific research methods. Faster blueprint research and experimentation.");
		Skill.Category = ECraftingSkillCategory::Research;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.03f;
		Skill.QualityBonusPerLevel = 0.005f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.005f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("GeneralCrafting")), 2);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	// ---- Automation Skills ----
	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("AutomationBasics"));
		Skill.SkillName = TEXT("Automation Basics");
		Skill.Description = TEXT("Automated production fundamentals. Build and manage automation nodes.");
		Skill.Category = ECraftingSkillCategory::Automation;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.03f;
		Skill.QualityBonusPerLevel = 0.0f;
		Skill.SuccessBonusPerLevel = 0.01f;
		Skill.MaterialEfficiencyPerLevel = 0.015f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("GeneralCrafting")), 5);
		Skill.RequiredSkillLevels.Add(FName(TEXT("Electronics")), 3);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	{
		FCraftingSkill Skill;
		Skill.SkillID = FName(TEXT("AdvancedAutomation"));
		Skill.SkillName = TEXT("Advanced Automation");
		Skill.Description = TEXT("Complex production line management. Advanced nodes and optimization.");
		Skill.Category = ECraftingSkillCategory::Automation;
		Skill.MaxLevel = 10;
		Skill.SpeedBonusPerLevel = 0.025f;
		Skill.QualityBonusPerLevel = 0.01f;
		Skill.SuccessBonusPerLevel = 0.005f;
		Skill.MaterialEfficiencyPerLevel = 0.02f;
		Skill.RequiredSkillLevels.Add(FName(TEXT("AutomationBasics")), 5);
		Skill.ExperienceToNextLevel = CalculateExperienceForLevel(1);
		Skills.Add(Skill.SkillID, Skill);
	}

	// Wire up skill tree connections (UnlocksSkills)
	for (auto& SkillPair : Skills)
	{
		for (const auto& Prereq : SkillPair.Value.RequiredSkillLevels)
		{
			FCraftingSkill* PrereqSkill = Skills.Find(Prereq.Key);
			if (PrereqSkill)
			{
				PrereqSkill->UnlocksSkills.AddUnique(SkillPair.Key);
			}
		}
	}

	// Give starting skill points
	SkillPoints.TotalSkillPoints = 3;
	SkillPoints.AvailableSkillPoints = 3;
	SkillPoints.SpentSkillPoints = 0;
}

void UOdysseyCraftingSkillSystem::InitializeDefaultMasteries()
{
	if (Masteries.Num() > 0)
	{
		return;
	}

	// Material Processing Mastery
	{
		FCraftingMasteryBonus Mastery;
		Mastery.MasteryID = FName(TEXT("MaterialMaster"));
		Mastery.MasteryName = TEXT("Master Refiner");
		Mastery.Category = ECraftingSkillCategory::MaterialProcessing;
		Mastery.RequiredTotalLevels = 25;
		Mastery.SpeedMultiplier = 1.25f;
		Mastery.QualityMultiplier = 1.1f;
		Mastery.UniqueItemChance = 0.03f;
		Masteries.Add(Mastery.MasteryID, Mastery);
	}

	// Weapon Crafting Mastery
	{
		FCraftingMasteryBonus Mastery;
		Mastery.MasteryID = FName(TEXT("WeaponMaster"));
		Mastery.MasteryName = TEXT("Master Weaponsmith");
		Mastery.Category = ECraftingSkillCategory::WeaponCrafting;
		Mastery.RequiredTotalLevels = 25;
		Mastery.SpeedMultiplier = 1.15f;
		Mastery.QualityMultiplier = 1.25f;
		Mastery.UniqueItemChance = 0.05f;
		Masteries.Add(Mastery.MasteryID, Mastery);
	}

	// Ship Module Mastery
	{
		FCraftingMasteryBonus Mastery;
		Mastery.MasteryID = FName(TEXT("ShipEngineer"));
		Mastery.MasteryName = TEXT("Master Ship Engineer");
		Mastery.Category = ECraftingSkillCategory::ShipModules;
		Mastery.RequiredTotalLevels = 20;
		Mastery.SpeedMultiplier = 1.2f;
		Mastery.QualityMultiplier = 1.2f;
		Mastery.UniqueItemChance = 0.04f;
		Masteries.Add(Mastery.MasteryID, Mastery);
	}

	// Electronics Mastery
	{
		FCraftingMasteryBonus Mastery;
		Mastery.MasteryID = FName(TEXT("ElectronicsMaster"));
		Mastery.MasteryName = TEXT("Master Technician");
		Mastery.Category = ECraftingSkillCategory::Electronics;
		Mastery.RequiredTotalLevels = 10;
		Mastery.SpeedMultiplier = 1.2f;
		Mastery.QualityMultiplier = 1.15f;
		Mastery.UniqueItemChance = 0.03f;
		Masteries.Add(Mastery.MasteryID, Mastery);
	}

	// Automation Mastery
	{
		FCraftingMasteryBonus Mastery;
		Mastery.MasteryID = FName(TEXT("AutomationMaster"));
		Mastery.MasteryName = TEXT("Master Automator");
		Mastery.Category = ECraftingSkillCategory::Automation;
		Mastery.RequiredTotalLevels = 15;
		Mastery.SpeedMultiplier = 1.3f;
		Mastery.QualityMultiplier = 1.05f;
		Mastery.UniqueItemChance = 0.02f;
		Masteries.Add(Mastery.MasteryID, Mastery);
	}

	// General Mastery (Grand Master)
	{
		FCraftingMasteryBonus Mastery;
		Mastery.MasteryID = FName(TEXT("GrandMaster"));
		Mastery.MasteryName = TEXT("Grand Master Crafter");
		Mastery.Category = ECraftingSkillCategory::General;
		Mastery.RequiredTotalLevels = 40;
		Mastery.SpeedMultiplier = 1.15f;
		Mastery.QualityMultiplier = 1.15f;
		Mastery.UniqueItemChance = 0.08f;
		Masteries.Add(Mastery.MasteryID, Mastery);
	}
}

void UOdysseyCraftingSkillSystem::LevelUpSkill(FName SkillID)
{
	FCraftingSkill* Skill = Skills.Find(SkillID);
	if (!Skill || Skill->CurrentLevel >= Skill->MaxLevel)
	{
		return;
	}

	int32 OldLevel = Skill->CurrentLevel;

	// Subtract experience cost and level up
	Skill->CurrentExperience -= Skill->ExperienceToNextLevel;
	Skill->CurrentLevel++;
	Skill->ExperienceToNextLevel = CalculateExperienceForLevel(Skill->CurrentLevel + 1);

	OnSkillLevelUp.Broadcast(SkillID, OldLevel, Skill->CurrentLevel);

	// Award skill points at certain thresholds
	if (Skill->CurrentLevel % 5 == 0) // Every 5 levels
	{
		AddSkillPoints(SkillPointsPerLevel);
	}

	// Check if any recipes should be unlocked at this level
	if (CraftingManager)
	{
		for (const FName& RecipeID : Skill->UnlocksRecipes)
		{
			FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);
			const int32* ReqLevel = Recipe.RequiredSkillLevels.Find(SkillID);
			if (ReqLevel && Skill->CurrentLevel >= *ReqLevel)
			{
				CraftingManager->UnlockRecipe(RecipeID, SkillID);
			}
		}
	}

	// Check mastery unlocks
	CheckMasteryUnlocks();

	// Update skill points tracking
	UpdateSkillPoints();
}

void UOdysseyCraftingSkillSystem::CheckMasteryUnlocks()
{
	for (auto& MasteryPair : Masteries)
	{
		if (UnlockedMasteries.Contains(MasteryPair.Key))
		{
			continue; // Already unlocked
		}

		FCraftingMasteryBonus& Mastery = MasteryPair.Value;
		int32 TotalLevels = GetTotalLevelsInCategory(Mastery.Category);

		if (TotalLevels >= Mastery.RequiredTotalLevels)
		{
			Mastery.bIsUnlocked = true;
			UnlockedMasteries.Add(MasteryPair.Key);

			OnMasteryUnlocked.Broadcast(MasteryPair.Key, Mastery.Category);

			// Unlock exclusive mastery recipes
			if (CraftingManager)
			{
				for (const FName& RecipeID : Mastery.ExclusiveRecipes)
				{
					CraftingManager->UnlockRecipe(RecipeID, MasteryPair.Key);
				}
			}

			UE_LOG(LogTemp, Log, TEXT("CraftingSkill: Mastery unlocked: %s"), *Mastery.MasteryName);
		}
	}
}

void UOdysseyCraftingSkillSystem::UpdateSkillPoints()
{
	// Recalculate total skill points based on total levels
	int32 TotalLevels = GetTotalSkillLevels();
	int32 EarnedPoints = 3; // Starting points

	// Every 5 combined levels earns a skill point
	EarnedPoints += TotalLevels / 5;

	// Update if new points earned
	if (EarnedPoints > SkillPoints.TotalSkillPoints)
	{
		int32 NewPoints = EarnedPoints - SkillPoints.TotalSkillPoints;
		SkillPoints.TotalSkillPoints = EarnedPoints;
		SkillPoints.AvailableSkillPoints += NewPoints;
	}
}

TArray<FName> UOdysseyCraftingSkillSystem::GetRelevantSkillsForRecipe(FName RecipeID) const
{
	TArray<FName> RelevantSkills;

	if (!CraftingManager)
	{
		return RelevantSkills;
	}

	FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(RecipeID);
	if (Recipe.RecipeID.IsNone())
	{
		return RelevantSkills;
	}

	// Add explicitly required skills
	for (const auto& SkillReq : Recipe.RequiredSkillLevels)
	{
		RelevantSkills.AddUnique(SkillReq.Key);
	}

	// Add skills that give experience for this recipe
	for (const auto& SkillExp : Recipe.SkillExperienceRewards)
	{
		RelevantSkills.AddUnique(SkillExp.Key);
	}

	// Always include general crafting
	RelevantSkills.AddUnique(FName(TEXT("GeneralCrafting")));

	return RelevantSkills;
}
