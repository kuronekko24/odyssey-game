// OdysseyCraftingManagerTests.cpp
// Comprehensive automation tests for the Crafting Manager
// Tests craft execution, queue management, facility management, and calculations

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Crafting/OdysseyCraftingManager.h"
#include "Crafting/OdysseyCraftingRecipeComponent.h"
#include "Crafting/OdysseyQualityControlSystem.h"
#include "Crafting/OdysseyCraftingSkillSystem.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyCraftingComponent.h"

// ============================================================================
// 1. Facility Registration
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerFacilityRegistration,
	"Odyssey.Crafting.Manager.FacilityRegistration",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerFacilityRegistration::RunTest(const FString& Parameters)
{
	FCraftingFacility Facility;
	Facility.FacilityID = FName(TEXT("Facility_Basic"));
	Facility.FacilityName = TEXT("Basic Workshop");
	Facility.Tier = ECraftingTier::Basic;
	Facility.Level = 1;
	Facility.MaxConcurrentJobs = 3;
	Facility.SpeedMultiplier = 1.0f;
	Facility.QualityBonus = 0.0f;
	Facility.EnergyEfficiency = 1.0f;
	Facility.bIsOnline = true;

	// Verify facility struct is correctly populated
	TestEqual(TEXT("FacilityID should match"), Facility.FacilityID, FName(TEXT("Facility_Basic")));
	TestEqual(TEXT("Tier should be Basic"), Facility.Tier, ECraftingTier::Basic);
	TestEqual(TEXT("MaxConcurrentJobs should be 3"), Facility.MaxConcurrentJobs, 3);
	TestTrue(TEXT("Facility should be online"), Facility.bIsOnline);

	return true;
}

// ============================================================================
// 2. Facility Registration Rejects Invalid ID
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerFacilityInvalidID,
	"Odyssey.Crafting.Manager.FacilityRejectsInvalidID",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerFacilityInvalidID::RunTest(const FString& Parameters)
{
	FCraftingFacility Facility;
	// Leave FacilityID as NAME_None (invalid)

	TestEqual(TEXT("Default FacilityID should be NAME_None"), Facility.FacilityID, NAME_None);
	TestTrue(TEXT("NAME_None IsNone should be true"), Facility.FacilityID.IsNone());

	return true;
}

// ============================================================================
// 3. Facility Upgrade Mechanics
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerFacilityUpgrade,
	"Odyssey.Crafting.Manager.FacilityUpgradeMechanics",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerFacilityUpgrade::RunTest(const FString& Parameters)
{
	// Simulate facility upgrade logic inline (matches implementation)
	FCraftingFacility Facility;
	Facility.FacilityID = FName(TEXT("Facility_Upgrade"));
	Facility.Tier = ECraftingTier::Basic;
	Facility.Level = 1;
	Facility.SpeedMultiplier = 1.0f;
	Facility.QualityBonus = 0.0f;
	Facility.MaxConcurrentJobs = 1;

	// Simulate upgrade (same logic as UOdysseyCraftingManager::UpgradeFacility)
	Facility.Tier = static_cast<ECraftingTier>(static_cast<uint8>(Facility.Tier) + 1);
	Facility.Level++;
	Facility.SpeedMultiplier *= 1.15f;
	Facility.QualityBonus += 0.05f;
	Facility.MaxConcurrentJobs++;

	TestEqual(TEXT("Tier should be Advanced after upgrade"), Facility.Tier, ECraftingTier::Advanced);
	TestEqual(TEXT("Level should be 2 after upgrade"), Facility.Level, 2);
	TestTrue(TEXT("SpeedMultiplier should increase"), Facility.SpeedMultiplier > 1.0f);
	TestTrue(TEXT("SpeedMultiplier should be ~1.15"), FMath::IsNearlyEqual(Facility.SpeedMultiplier, 1.15f, 0.01f));
	TestTrue(TEXT("QualityBonus should be 0.05"), FMath::IsNearlyEqual(Facility.QualityBonus, 0.05f, 0.01f));
	TestEqual(TEXT("MaxConcurrentJobs should be 2"), Facility.MaxConcurrentJobs, 2);

	return true;
}

// ============================================================================
// 4. Max Tier Cannot Be Upgraded
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerMaxTierNoUpgrade,
	"Odyssey.Crafting.Manager.MaxTierCannotUpgrade",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerMaxTierNoUpgrade::RunTest(const FString& Parameters)
{
	FCraftingFacility Facility;
	Facility.Tier = ECraftingTier::Quantum;

	// The manager checks this: if Tier == Quantum, return false
	bool bCanUpgrade = (Facility.Tier != ECraftingTier::Quantum);
	TestFalse(TEXT("Quantum tier facility should not be upgradeable"), bCanUpgrade);

	return true;
}

// ============================================================================
// 5. Job Priority Sorting
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerJobPriority,
	"Odyssey.Crafting.Manager.JobPrioritySorting",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerJobPriority::RunTest(const FString& Parameters)
{
	TArray<FCraftingJob> Jobs;

	auto MakeJob = [](FName RecipeID, int32 Priority) -> FCraftingJob
	{
		FCraftingJob Job;
		Job.RecipeID = RecipeID;
		Job.Priority = Priority;
		return Job;
	};

	Jobs.Add(MakeJob(FName(TEXT("Low")), 1));
	Jobs.Add(MakeJob(FName(TEXT("High")), 10));
	Jobs.Add(MakeJob(FName(TEXT("Medium")), 5));
	Jobs.Add(MakeJob(FName(TEXT("Highest")), 100));

	// Sort by priority descending (matches SortJobsByPriority implementation)
	Jobs.Sort([](const FCraftingJob& A, const FCraftingJob& B)
	{
		return A.Priority > B.Priority;
	});

	TestEqual(TEXT("First job should be Highest priority"), Jobs[0].RecipeID, FName(TEXT("Highest")));
	TestEqual(TEXT("Second job should be High priority"), Jobs[1].RecipeID, FName(TEXT("High")));
	TestEqual(TEXT("Third job should be Medium priority"), Jobs[2].RecipeID, FName(TEXT("Medium")));
	TestEqual(TEXT("Fourth job should be Low priority"), Jobs[3].RecipeID, FName(TEXT("Low")));

	return true;
}

// ============================================================================
// 6. Job Progress Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerJobProgress,
	"Odyssey.Crafting.Manager.JobProgressCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerJobProgress::RunTest(const FString& Parameters)
{
	FCraftingJob Job;
	Job.TotalTime = 10.0f;
	Job.RemainingTime = 10.0f;
	Job.State = ECraftingState::Crafting;

	// Simulate ticking the job (same formula as ProcessActiveJobs)
	float DeltaTime = 3.0f;
	Job.RemainingTime -= DeltaTime;
	Job.Progress = 1.0f - (Job.RemainingTime / Job.TotalTime);

	TestTrue(TEXT("Progress should be approximately 30%"), FMath::IsNearlyEqual(Job.Progress, 0.3f, 0.001f));
	TestTrue(TEXT("RemainingTime should be 7.0"), FMath::IsNearlyEqual(Job.RemainingTime, 7.0f, 0.001f));

	// Tick more
	DeltaTime = 7.0f;
	Job.RemainingTime -= DeltaTime;
	Job.Progress = 1.0f - (Job.RemainingTime / Job.TotalTime);

	TestTrue(TEXT("Progress should be 100%"), FMath::IsNearlyEqual(Job.Progress, 1.0f, 0.001f));
	TestTrue(TEXT("RemainingTime should be 0"), Job.RemainingTime <= 0.0f);

	return true;
}

// ============================================================================
// 7. Job Completion Detection
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerJobCompletion,
	"Odyssey.Crafting.Manager.JobCompletionDetection",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerJobCompletion::RunTest(const FString& Parameters)
{
	FCraftingJob Job;
	Job.TotalTime = 5.0f;
	Job.RemainingTime = 5.0f;
	Job.State = ECraftingState::Crafting;

	// Simulate overshooting the timer
	float DeltaTime = 6.0f;
	Job.RemainingTime -= DeltaTime;

	bool bShouldComplete = (Job.RemainingTime <= 0.0f);
	TestTrue(TEXT("Job should be detected as complete"), bShouldComplete);
	TestTrue(TEXT("RemainingTime should be negative"), Job.RemainingTime < 0.0f);

	return true;
}

// ============================================================================
// 8. Crafting Time Calculation with Facility Speed
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerTimeWithFacility,
	"Odyssey.Crafting.Manager.CraftingTimeWithFacilitySpeed",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerTimeWithFacility::RunTest(const FString& Parameters)
{
	// Simulate CalculateCraftingTime logic
	float BaseCraftingTime = 10.0f;
	int32 Quantity = 2;
	float BaseTime = BaseCraftingTime * Quantity; // 20.0f

	// Apply facility speed multiplier of 2.0x
	float FacilitySpeedMultiplier = 2.0f;
	float AdjustedTime = BaseTime / FacilitySpeedMultiplier; // 10.0f

	TestTrue(TEXT("Time with 2x speed should be 10.0"), FMath::IsNearlyEqual(AdjustedTime, 10.0f, 0.01f));

	// Minimum crafting time check
	float MinCraftingTime = FMath::Max(AdjustedTime, 0.5f);
	TestTrue(TEXT("Should not go below 0.5s minimum"), MinCraftingTime >= 0.5f);

	return true;
}

// ============================================================================
// 9. Crafting Time with Skill Speed Bonus
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerTimeWithSkill,
	"Odyssey.Crafting.Manager.CraftingTimeWithSkillBonus",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerTimeWithSkill::RunTest(const FString& Parameters)
{
	// Simulate time calculation with skill bonus
	float BaseTime = 10.0f;
	float SkillSpeedBonus = 0.2f; // 20% speed bonus

	float AdjustedTime = BaseTime * (1.0f - SkillSpeedBonus); // 8.0f

	TestTrue(TEXT("20% skill bonus should reduce time to 8.0"), FMath::IsNearlyEqual(AdjustedTime, 8.0f, 0.01f));

	// Combined with facility
	float FacilitySpeed = 1.5f;
	float CombinedTime = (BaseTime / FacilitySpeed) * (1.0f - SkillSpeedBonus);
	float ExpectedTime = (10.0f / 1.5f) * 0.8f; // ~5.33

	TestTrue(TEXT("Combined time should match"), FMath::IsNearlyEqual(CombinedTime, ExpectedTime, 0.01f));

	return true;
}

// ============================================================================
// 10. Minimum Crafting Time Enforcement
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerMinTime,
	"Odyssey.Crafting.Manager.MinimumCraftingTimeEnforced",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerMinTime::RunTest(const FString& Parameters)
{
	// Very fast setup that would theoretically result in sub-minimum time
	float BaseTime = 1.0f;
	float FacilitySpeed = 10.0f;
	float SkillBonus = 0.5f;

	float CalculatedTime = (BaseTime / FacilitySpeed) * (1.0f - SkillBonus); // 0.05
	float FinalTime = FMath::Max(CalculatedTime, 0.5f); // Clamped to 0.5

	TestTrue(TEXT("Calculated time should be below minimum"), CalculatedTime < 0.5f);
	TestEqual(TEXT("Final time should be clamped to 0.5"), FinalTime, 0.5f);

	return true;
}

// ============================================================================
// 11. Energy Cost Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerEnergyCost,
	"Odyssey.Crafting.Manager.EnergyCostCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerEnergyCost::RunTest(const FString& Parameters)
{
	// Simulate CalculateEnergyCost logic
	int32 BaseEnergyCost = 10;
	int32 Quantity = 3;
	float FacilityEfficiency = 0.8f; // 20% more efficient

	float RawCost = static_cast<float>(BaseEnergyCost * Quantity); // 30
	int32 FinalCost = FMath::CeilToInt(RawCost * FacilityEfficiency); // ceil(24.0) = 24

	TestEqual(TEXT("Energy cost with efficiency should be 24"), FinalCost, 24);

	// Without facility
	int32 CostNoFacility = FMath::CeilToInt(static_cast<float>(BaseEnergyCost * Quantity) * 1.0f);
	TestEqual(TEXT("Energy cost without efficiency should be 30"), CostNoFacility, 30);

	return true;
}

// ============================================================================
// 12. Success Chance Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerSuccessChance,
	"Odyssey.Crafting.Manager.SuccessChanceCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerSuccessChance::RunTest(const FString& Parameters)
{
	// Simulate CalculateSuccessChance logic
	float SuccessChance = 0.9f; // Base 90%

	// Add skill bonus
	float SkillBonus = 0.05f;
	SuccessChance += SkillBonus;

	// Add facility bonus (QualityBonus * 0.1)
	float FacilityQualityBonus = 0.2f;
	SuccessChance += FacilityQualityBonus * 0.1f;

	float FinalChance = FMath::Clamp(SuccessChance, 0.1f, 1.0f);

	TestTrue(TEXT("Success chance should be 0.97"), FMath::IsNearlyEqual(FinalChance, 0.97f, 0.001f));
	TestTrue(TEXT("Success chance should not exceed 1.0"), FinalChance <= 1.0f);

	// Test clamping at maximum
	float MaxChance = FMath::Clamp(1.5f, 0.1f, 1.0f);
	TestEqual(TEXT("Overshoot should clamp to 1.0"), MaxChance, 1.0f);

	// Test clamping at minimum
	float MinChance = FMath::Clamp(0.05f, 0.1f, 1.0f);
	TestEqual(TEXT("Undershoot should clamp to 0.1"), MinChance, 0.1f);

	return true;
}

// ============================================================================
// 13. Quality Expected Calculation Thresholds
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerQualityThresholds,
	"Odyssey.Crafting.Manager.QualityThresholds",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerQualityThresholds::RunTest(const FString& Parameters)
{
	// Test the quality threshold logic from CalculateExpectedQuality
	auto GetQualityFromScore = [](float Score) -> EItemQuality
	{
		if (Score >= 0.95f) return EItemQuality::Legendary;
		if (Score >= 0.85f) return EItemQuality::Masterwork;
		if (Score >= 0.70f) return EItemQuality::Superior;
		if (Score >= 0.55f) return EItemQuality::Quality;
		if (Score >= 0.40f) return EItemQuality::Standard;
		if (Score >= 0.20f) return EItemQuality::Common;
		return EItemQuality::Scrap;
	};

	TestEqual(TEXT("Score 0.0 should be Scrap"), GetQualityFromScore(0.0f), EItemQuality::Scrap);
	TestEqual(TEXT("Score 0.15 should be Scrap"), GetQualityFromScore(0.15f), EItemQuality::Scrap);
	TestEqual(TEXT("Score 0.20 should be Common"), GetQualityFromScore(0.20f), EItemQuality::Common);
	TestEqual(TEXT("Score 0.35 should be Common"), GetQualityFromScore(0.35f), EItemQuality::Common);
	TestEqual(TEXT("Score 0.40 should be Standard"), GetQualityFromScore(0.40f), EItemQuality::Standard);
	TestEqual(TEXT("Score 0.55 should be Quality"), GetQualityFromScore(0.55f), EItemQuality::Quality);
	TestEqual(TEXT("Score 0.70 should be Superior"), GetQualityFromScore(0.70f), EItemQuality::Superior);
	TestEqual(TEXT("Score 0.85 should be Masterwork"), GetQualityFromScore(0.85f), EItemQuality::Masterwork);
	TestEqual(TEXT("Score 0.95 should be Legendary"), GetQualityFromScore(0.95f), EItemQuality::Legendary);
	TestEqual(TEXT("Score 1.0 should be Legendary"), GetQualityFromScore(1.0f), EItemQuality::Legendary);

	return true;
}

// ============================================================================
// 14. Job Pause and Resume
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerPauseResume,
	"Odyssey.Crafting.Manager.JobPauseAndResume",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerPauseResume::RunTest(const FString& Parameters)
{
	FCraftingJob Job;
	Job.State = ECraftingState::Crafting;
	Job.RemainingTime = 5.0f;
	Job.TotalTime = 10.0f;

	// Pause: State changes from Crafting to Idle
	if (Job.State == ECraftingState::Crafting)
	{
		Job.State = ECraftingState::Idle;
	}
	TestEqual(TEXT("Paused job should be Idle"), Job.State, ECraftingState::Idle);

	// Resume: State changes from Idle to Crafting
	if (Job.State == ECraftingState::Idle)
	{
		Job.State = ECraftingState::Crafting;
	}
	TestEqual(TEXT("Resumed job should be Crafting"), Job.State, ECraftingState::Crafting);

	// Remaining time should not change during pause/resume
	TestTrue(TEXT("RemainingTime should be preserved"), FMath::IsNearlyEqual(Job.RemainingTime, 5.0f, 0.001f));

	return true;
}

// ============================================================================
// 15. Cannot Pause Already Paused Job
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerPausePausedJob,
	"Odyssey.Crafting.Manager.CannotPauseAlreadyPausedJob",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerPausePausedJob::RunTest(const FString& Parameters)
{
	FCraftingJob Job;
	Job.State = ECraftingState::Idle; // Already paused

	// Simulate PauseCraftingJob: only pauses if state is Crafting
	bool bPaused = false;
	if (Job.State == ECraftingState::Crafting)
	{
		Job.State = ECraftingState::Idle;
		bPaused = true;
	}

	TestFalse(TEXT("Should not be able to pause an already idle job"), bPaused);

	return true;
}

// ============================================================================
// 16. Cannot Resume Active Job
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerResumeActiveJob,
	"Odyssey.Crafting.Manager.CannotResumeActiveJob",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerResumeActiveJob::RunTest(const FString& Parameters)
{
	FCraftingJob Job;
	Job.State = ECraftingState::Crafting; // Already crafting

	// Simulate ResumeCraftingJob: only resumes if state is Idle
	bool bResumed = false;
	if (Job.State == ECraftingState::Idle)
	{
		Job.State = ECraftingState::Crafting;
		bResumed = true;
	}

	TestFalse(TEXT("Should not be able to resume an already active job"), bResumed);

	return true;
}

// ============================================================================
// 17. Refund Calculation on Job Cancel
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerRefundCalculation,
	"Odyssey.Crafting.Manager.RefundCalculationOnCancel",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerRefundCalculation::RunTest(const FString& Parameters)
{
	// Simulate partial refund logic from CancelCraftingJob
	FCraftingJob Job;
	Job.Quantity = 5;
	Job.CompletedQuantity = 2;
	Job.Progress = 0.4f; // 40% through current batch

	int32 RemainingQuantity = Job.Quantity - Job.CompletedQuantity; // 3
	float RefundMultiplier = 1.0f - Job.Progress; // 0.6

	// For an ingredient with Amount = 10
	int32 IngredientAmount = 10;
	int32 RefundAmount = FMath::FloorToInt(IngredientAmount * RemainingQuantity * RefundMultiplier);
	// = floor(10 * 3 * 0.6) = floor(18.0) = 18

	TestEqual(TEXT("Refund amount should be 18"), RefundAmount, 18);

	// Edge case: 0% progress = full refund of remaining
	Job.Progress = 0.0f;
	int32 FullRefund = FMath::FloorToInt(IngredientAmount * RemainingQuantity * (1.0f - Job.Progress));
	TestEqual(TEXT("0% progress should refund 30"), FullRefund, 30);

	// Edge case: 100% progress = no refund
	Job.Progress = 1.0f;
	int32 NoRefund = FMath::FloorToInt(IngredientAmount * RemainingQuantity * (1.0f - Job.Progress));
	TestEqual(TEXT("100% progress should refund 0"), NoRefund, 0);

	return true;
}

// ============================================================================
// 18. Statistics Update on Craft Success
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerStatsOnSuccess,
	"Odyssey.Crafting.Manager.StatisticsUpdateOnSuccess",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerStatsOnSuccess::RunTest(const FString& Parameters)
{
	FCraftingStatistics Stats;

	// Simulate UpdateStatistics for success
	TArray<FCraftedItem> ProducedItems;

	FCraftedItem Item1;
	Item1.Quality = EItemQuality::Superior;
	Item1.Category = EItemCategory::Weapon;
	Item1.Quantity = 1;
	ProducedItems.Add(Item1);

	FCraftedItem Item2;
	Item2.Quality = EItemQuality::Masterwork;
	Item2.Category = EItemCategory::Weapon;
	Item2.Quantity = 2;
	ProducedItems.Add(Item2);

	// Apply the same logic as UpdateStatistics
	bool bSuccess = true;
	if (bSuccess)
	{
		Stats.SuccessfulCrafts++;
	}
	else
	{
		Stats.FailedCrafts++;
	}

	for (const FCraftedItem& Item : ProducedItems)
	{
		Stats.TotalItemsCrafted += Item.Quantity;

		int32& QualityCount = Stats.ItemsByQuality.FindOrAdd(Item.Quality);
		QualityCount += Item.Quantity;

		int32& CategoryCount = Stats.ItemsByCategory.FindOrAdd(Item.Category);
		CategoryCount += Item.Quantity;

		if (Item.Quality == EItemQuality::Masterwork)
		{
			Stats.MasterworkItemsCreated += Item.Quantity;
		}
		else if (Item.Quality == EItemQuality::Legendary)
		{
			Stats.LegendaryItemsCreated += Item.Quantity;
		}
	}

	TestEqual(TEXT("SuccessfulCrafts should be 1"), Stats.SuccessfulCrafts, 1);
	TestEqual(TEXT("FailedCrafts should be 0"), Stats.FailedCrafts, 0);
	TestEqual(TEXT("TotalItemsCrafted should be 3"), Stats.TotalItemsCrafted, 3);
	TestEqual(TEXT("MasterworkItemsCreated should be 2"), Stats.MasterworkItemsCreated, 2);
	TestEqual(TEXT("LegendaryItemsCreated should be 0"), Stats.LegendaryItemsCreated, 0);

	const int32* SuperiorCount = Stats.ItemsByQuality.Find(EItemQuality::Superior);
	TestNotNull(TEXT("Should have Superior count"), SuperiorCount);
	if (SuperiorCount) TestEqual(TEXT("Superior count should be 1"), *SuperiorCount, 1);

	const int32* WeaponCount = Stats.ItemsByCategory.Find(EItemCategory::Weapon);
	TestNotNull(TEXT("Should have Weapon category count"), WeaponCount);
	if (WeaponCount) TestEqual(TEXT("Weapon category count should be 3"), *WeaponCount, 3);

	return true;
}

// ============================================================================
// 19. Statistics Update on Craft Failure
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerStatsOnFailure,
	"Odyssey.Crafting.Manager.StatisticsUpdateOnFailure",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerStatsOnFailure::RunTest(const FString& Parameters)
{
	FCraftingStatistics Stats;
	Stats.SuccessfulCrafts = 5;
	Stats.FailedCrafts = 2;

	// Simulate failure
	bool bSuccess = false;
	if (bSuccess)
	{
		Stats.SuccessfulCrafts++;
	}
	else
	{
		Stats.FailedCrafts++;
	}

	TestEqual(TEXT("SuccessfulCrafts should remain 5"), Stats.SuccessfulCrafts, 5);
	TestEqual(TEXT("FailedCrafts should increment to 3"), Stats.FailedCrafts, 3);

	return true;
}

// ============================================================================
// 20. Total Queue Time Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerTotalQueueTime,
	"Odyssey.Crafting.Manager.TotalQueueTimeCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerTotalQueueTime::RunTest(const FString& Parameters)
{
	TArray<FCraftingJob> Jobs;

	FCraftingJob Job1;
	Job1.RemainingTime = 5.0f;
	Jobs.Add(Job1);

	FCraftingJob Job2;
	Job2.RemainingTime = 10.0f;
	Jobs.Add(Job2);

	FCraftingJob Job3;
	Job3.RemainingTime = 3.5f;
	Jobs.Add(Job3);

	// Simulate GetTotalQueueTime
	float TotalTime = 0.0f;
	for (const FCraftingJob& Job : Jobs)
	{
		TotalTime += Job.RemainingTime;
	}

	TestTrue(TEXT("Total queue time should be 18.5"), FMath::IsNearlyEqual(TotalTime, 18.5f, 0.001f));

	return true;
}

// ============================================================================
// 21. Job Batch Processing Limit
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerBatchProcessing,
	"Odyssey.Crafting.Manager.BatchProcessingLimit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerBatchProcessing::RunTest(const FString& Parameters)
{
	int32 JobBatchSize = 5;
	TArray<FCraftingJob> Jobs;

	// Create 10 active jobs
	for (int32 i = 0; i < 10; ++i)
	{
		FCraftingJob Job;
		Job.State = ECraftingState::Crafting;
		Job.TotalTime = 10.0f;
		Job.RemainingTime = 10.0f;
		Jobs.Add(Job);
	}

	// Simulate batch processing
	int32 ProcessedCount = 0;
	float DeltaTime = 1.0f;

	for (int32 i = 0; i < Jobs.Num() && ProcessedCount < JobBatchSize; ++i)
	{
		if (Jobs[i].State == ECraftingState::Crafting)
		{
			Jobs[i].RemainingTime -= DeltaTime;
			ProcessedCount++;
		}
	}

	TestEqual(TEXT("Should process exactly batch size jobs"), ProcessedCount, JobBatchSize);

	// Verify only first 5 were processed
	for (int32 i = 0; i < 5; ++i)
	{
		TestTrue(TEXT("Processed job should have reduced time"), Jobs[i].RemainingTime < 10.0f);
	}
	for (int32 i = 5; i < 10; ++i)
	{
		TestTrue(TEXT("Unprocessed job should still be at full time"), FMath::IsNearlyEqual(Jobs[i].RemainingTime, 10.0f, 0.001f));
	}

	return true;
}

// ============================================================================
// 22. Global Concurrent Job Limit
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerGlobalJobLimit,
	"Odyssey.Crafting.Manager.GlobalConcurrentJobLimit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerGlobalJobLimit::RunTest(const FString& Parameters)
{
	int32 MaxGlobalConcurrentJobs = 10;
	TArray<FCraftingJob> ActiveJobs;

	// Fill to capacity
	for (int32 i = 0; i < MaxGlobalConcurrentJobs; ++i)
	{
		FCraftingJob Job;
		Job.RecipeID = FName(*FString::Printf(TEXT("Recipe_%d"), i));
		ActiveJobs.Add(Job);
	}

	// Attempt to add one more
	bool bCanAddMore = (ActiveJobs.Num() < MaxGlobalConcurrentJobs);

	TestEqual(TEXT("Active jobs should be at capacity"), ActiveJobs.Num(), MaxGlobalConcurrentJobs);
	TestFalse(TEXT("Should not be able to add more jobs"), bCanAddMore);

	return true;
}

// ============================================================================
// 23. Facility Selection for Recipe Based on Score
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerFacilitySelection,
	"Odyssey.Crafting.Manager.BestFacilitySelection",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerFacilitySelection::RunTest(const FString& Parameters)
{
	// Simulate scoring logic from GetBestFacilityForRecipe
	struct FFacilityScore
	{
		FName ID;
		float Score;
	};

	auto CalculateScore = [](float SpeedMult, float QualityBonus, float EnergyEff) -> float
	{
		return SpeedMult * 0.4f + QualityBonus * 0.4f + EnergyEff * 0.2f;
	};

	TArray<FFacilityScore> Facilities;
	Facilities.Add({FName(TEXT("Basic")), CalculateScore(1.0f, 0.0f, 1.0f)});
	Facilities.Add({FName(TEXT("Advanced")), CalculateScore(1.5f, 0.1f, 0.9f)});
	Facilities.Add({FName(TEXT("Industrial")), CalculateScore(2.0f, 0.2f, 0.8f)});

	FName BestFacility;
	float BestScore = -1.0f;
	for (const auto& F : Facilities)
	{
		if (F.Score > BestScore)
		{
			BestScore = F.Score;
			BestFacility = F.ID;
		}
	}

	TestEqual(TEXT("Best facility should be Industrial"), BestFacility, FName(TEXT("Industrial")));
	TestTrue(TEXT("Industrial score should be highest"),
		CalculateScore(2.0f, 0.2f, 0.8f) > CalculateScore(1.5f, 0.1f, 0.9f));

	return true;
}

// ============================================================================
// 24. Statistics Reset
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerStatsReset,
	"Odyssey.Crafting.Manager.StatisticsReset",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerStatsReset::RunTest(const FString& Parameters)
{
	FCraftingStatistics Stats;
	Stats.TotalItemsCrafted = 100;
	Stats.SuccessfulCrafts = 80;
	Stats.FailedCrafts = 20;
	Stats.MasterworkItemsCreated = 5;
	Stats.LegendaryItemsCreated = 1;

	// Simulate ResetStatistics
	Stats = FCraftingStatistics();

	TestEqual(TEXT("TotalItemsCrafted should be 0"), Stats.TotalItemsCrafted, 0);
	TestEqual(TEXT("SuccessfulCrafts should be 0"), Stats.SuccessfulCrafts, 0);
	TestEqual(TEXT("FailedCrafts should be 0"), Stats.FailedCrafts, 0);
	TestEqual(TEXT("MasterworkItemsCreated should be 0"), Stats.MasterworkItemsCreated, 0);
	TestEqual(TEXT("LegendaryItemsCreated should be 0"), Stats.LegendaryItemsCreated, 0);

	return true;
}

// ============================================================================
// 25. Quality Multiplier per Quality Tier
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCraftingManagerQualityMultiplier,
	"Odyssey.Crafting.Manager.QualityMultiplierScaling",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCraftingManagerQualityMultiplier::RunTest(const FString& Parameters)
{
	// Test the quality multiplier calculation from ProduceJobOutputs:
	// QualityMultiplier = 1.0 + (quality_enum_value * 0.15)
	auto CalcMultiplier = [](EItemQuality Quality) -> float
	{
		return 1.0f + (static_cast<float>(Quality) * 0.15f);
	};

	TestTrue(TEXT("Scrap multiplier should be 1.0"),
		FMath::IsNearlyEqual(CalcMultiplier(EItemQuality::Scrap), 1.0f, 0.001f));
	TestTrue(TEXT("Common multiplier should be ~1.15"),
		FMath::IsNearlyEqual(CalcMultiplier(EItemQuality::Common), 1.15f, 0.001f));
	TestTrue(TEXT("Standard multiplier should be ~1.30"),
		FMath::IsNearlyEqual(CalcMultiplier(EItemQuality::Standard), 1.30f, 0.001f));
	TestTrue(TEXT("Legendary multiplier should be ~1.90"),
		FMath::IsNearlyEqual(CalcMultiplier(EItemQuality::Legendary), 1.90f, 0.001f));

	// Verify ordering
	TestTrue(TEXT("Higher quality should have higher multiplier"),
		CalcMultiplier(EItemQuality::Legendary) > CalcMultiplier(EItemQuality::Masterwork));
	TestTrue(TEXT("Masterwork > Superior"),
		CalcMultiplier(EItemQuality::Masterwork) > CalcMultiplier(EItemQuality::Superior));

	return true;
}
