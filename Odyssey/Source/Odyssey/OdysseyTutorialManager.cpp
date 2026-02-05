#include "OdysseyTutorialManager.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

UOdysseyTutorialManager::UOdysseyTutorialManager()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Tutorial settings
	bEnableTutorial = true;
	bCanSkipTutorial = true;
	bShowTutorialHints = true;
	HintDisplayDuration = 3.0f;

	// Tutorial state
	bTutorialActive = false;
	bTutorialCompleted = false;
	StepStartTime = 0.0f;
	TotalTutorialTime = 0.0f;
	StepTimer = 0.0f;

	// Demo settings
	DemoTimeLimit = 600.0f; // 10 minutes
	bShowTimeRemaining = true;
	DemoTimeRemaining = DemoTimeLimit;

	// Initialize action tracking
	bPlayerHasMoved = false;
	bPlayerHasMinedResource = false;
	bPlayerHasCraftedItem = false;
	bPlayerHasSoldResource = false;
	bPlayerHasPurchasedUpgrade = false;
	bPlayerHasOpenedInventory = false;

	// Initialize tutorial progress
	TutorialProgress.CurrentStep = ETutorialStep::Welcome;
	TutorialProgress.StepProgress = 0.0f;
	TutorialProgress.bStepCompleted = false;
	TutorialProgress.TotalTutorialProgress = 0.0f;
	TutorialProgress.CompletedSteps = 0;
	TutorialProgress.TotalSteps = 8;
}

void UOdysseyTutorialManager::BeginPlay()
{
	Super::BeginPlay();

	InitializeTutorial();

	if (bEnableTutorial)
	{
		// Start tutorial after a brief delay
		FTimerHandle DelayTimer;
		GetWorld()->GetTimerManager().SetTimer(DelayTimer, this, &UOdysseyTutorialManager::StartTutorial, 1.0f, false);
	}

	UE_LOG(LogTemp, Warning, TEXT("Tutorial Manager initialized. Tutorial enabled: %s"),
		bEnableTutorial ? TEXT("true") : TEXT("false"));
}

void UOdysseyTutorialManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update demo timer
	UpdateDemoTimer(DeltaTime);

	// Update tutorial progress if active
	if (bTutorialActive && !bTutorialCompleted)
	{
		UpdateStepProgress(DeltaTime);
		CheckStepCompletion();
	}
}

void UOdysseyTutorialManager::StartTutorial()
{
	if (bTutorialCompleted)
		return;

	bTutorialActive = true;
	TutorialProgress.CurrentStep = ETutorialStep::Welcome;
	TutorialProgress.CompletedSteps = 0;
	TotalTutorialTime = 0.0f;
	StepStartTime = GetWorld()->GetTimeSeconds();

	// Clear action tracking
	bPlayerHasMoved = false;
	bPlayerHasMinedResource = false;
	bPlayerHasCraftedItem = false;
	bPlayerHasSoldResource = false;
	bPlayerHasPurchasedUpgrade = false;
	bPlayerHasOpenedInventory = false;

	// Set up first step objectives
	ClearObjectives();
	AddObjective(TEXT("Welcome to Odyssey! Let's learn the basics of space exploration."));

	OnTutorialStarted();
	OnStepStarted(TutorialProgress.CurrentStep, GetCurrentStepData());

	UE_LOG(LogTemp, Warning, TEXT("Tutorial started"));
}

void UOdysseyTutorialManager::SkipTutorial()
{
	if (!bCanSkipTutorial || !bTutorialActive)
		return;

	bTutorialActive = false;
	bTutorialCompleted = true;
	TutorialProgress.TotalTutorialProgress = 1.0f;
	TutorialProgress.CompletedSteps = TutorialProgress.TotalSteps;

	OnTutorialSkipped();

	UE_LOG(LogTemp, Warning, TEXT("Tutorial skipped"));
}

void UOdysseyTutorialManager::PauseTutorial()
{
	if (!bTutorialActive)
		return;

	// Pause tutorial timers and progression
	// This would be implemented with game state pausing
	UE_LOG(LogTemp, Warning, TEXT("Tutorial paused"));
}

void UOdysseyTutorialManager::ResumeTutorial()
{
	if (!bTutorialActive)
		return;

	// Resume tutorial progression
	UE_LOG(LogTemp, Warning, TEXT("Tutorial resumed"));
}

void UOdysseyTutorialManager::RestartTutorial()
{
	bTutorialActive = false;
	bTutorialCompleted = false;
	CompletedSteps.Empty();

	// Reset action tracking
	bPlayerHasMoved = false;
	bPlayerHasMinedResource = false;
	bPlayerHasCraftedItem = false;
	bPlayerHasSoldResource = false;
	bPlayerHasPurchasedUpgrade = false;
	bPlayerHasOpenedInventory = false;

	StartTutorial();

	UE_LOG(LogTemp, Warning, TEXT("Tutorial restarted"));
}

void UOdysseyTutorialManager::AdvanceToNextStep()
{
	if (!bTutorialActive || bTutorialCompleted)
		return;

	// Mark current step as completed
	CompleteCurrentStep();

	// Move to next step
	int32 CurrentStepInt = (int32)TutorialProgress.CurrentStep;
	int32 NextStepInt = CurrentStepInt + 1;

	if (NextStepInt >= TutorialProgress.TotalSteps)
	{
		// Tutorial completed
		bTutorialCompleted = true;
		bTutorialActive = false;
		TutorialProgress.TotalTutorialProgress = 1.0f;
		OnTutorialCompleted();

		UE_LOG(LogTemp, Warning, TEXT("Tutorial completed"));
		return;
	}

	// Set new step
	TutorialProgress.CurrentStep = (ETutorialStep)NextStepInt;
	TutorialProgress.StepProgress = 0.0f;
	TutorialProgress.bStepCompleted = false;
	StepStartTime = GetWorld()->GetTimeSeconds();
	StepTimer = 0.0f;

	// Update objectives for new step
	ClearObjectives();
	SetupStepObjectives(TutorialProgress.CurrentStep);

	OnStepStarted(TutorialProgress.CurrentStep, GetCurrentStepData());

	UE_LOG(LogTemp, Warning, TEXT("Advanced to tutorial step: %d"), NextStepInt);
}

void UOdysseyTutorialManager::GoToStep(ETutorialStep TargetStep)
{
	if (!bTutorialActive)
		return;

	TutorialProgress.CurrentStep = TargetStep;
	TutorialProgress.StepProgress = 0.0f;
	TutorialProgress.bStepCompleted = false;
	StepStartTime = GetWorld()->GetTimeSeconds();
	StepTimer = 0.0f;

	ClearObjectives();
	SetupStepObjectives(TargetStep);

	OnStepStarted(TargetStep, GetCurrentStepData());

	UE_LOG(LogTemp, Warning, TEXT("Jumped to tutorial step: %d"), (int32)TargetStep);
}

void UOdysseyTutorialManager::CompleteCurrentStep()
{
	if (!bTutorialActive || TutorialProgress.bStepCompleted)
		return;

	TutorialProgress.bStepCompleted = true;
	TutorialProgress.StepProgress = 1.0f;
	TutorialProgress.CompletedSteps++;
	CompletedSteps.AddUnique(TutorialProgress.CurrentStep);

	// Update total progress
	TutorialProgress.TotalTutorialProgress = (float)TutorialProgress.CompletedSteps / (float)TutorialProgress.TotalSteps;

	OnStepCompleted(TutorialProgress.CurrentStep);

	UE_LOG(LogTemp, Warning, TEXT("Completed tutorial step: %d"), (int32)TutorialProgress.CurrentStep);
}

bool UOdysseyTutorialManager::IsStepCompleted(ETutorialStep Step) const
{
	return CompletedSteps.Contains(Step);
}

void UOdysseyTutorialManager::AddObjective(const FString& ObjectiveText, bool bIsOptional)
{
	FTutorialObjective NewObjective;
	NewObjective.ObjectiveText = ObjectiveText;
	NewObjective.bIsCompleted = false;
	NewObjective.bIsOptional = bIsOptional;

	CurrentObjectives.Add(NewObjective);

	OnObjectiveAdded(ObjectiveText);

	UE_LOG(LogTemp, Verbose, TEXT("Added objective: %s"), *ObjectiveText);
}

void UOdysseyTutorialManager::CompleteObjective(const FString& ObjectiveText)
{
	for (FTutorialObjective& Objective : CurrentObjectives)
	{
		if (Objective.ObjectiveText == ObjectiveText && !Objective.bIsCompleted)
		{
			Objective.bIsCompleted = true;
			OnObjectiveCompleted(ObjectiveText);

			UE_LOG(LogTemp, Verbose, TEXT("Completed objective: %s"), *ObjectiveText);
			break;
		}
	}
}

void UOdysseyTutorialManager::ClearObjectives()
{
	CurrentObjectives.Empty();
}

float UOdysseyTutorialManager::GetStepProgress() const
{
	if (!bTutorialActive)
		return 0.0f;

	return TutorialProgress.StepProgress;
}

FTutorialStepData UOdysseyTutorialManager::GetCurrentStepData() const
{
	return GetStepData(TutorialProgress.CurrentStep);
}

FTutorialStepData UOdysseyTutorialManager::GetStepData(ETutorialStep Step) const
{
	if (TutorialStepsDataTable)
	{
		FString ContextString = TEXT("Tutorial Step Lookup");
		FName RowName = FName(*FString::FromInt((int32)Step));

		FTutorialStepData* StepData = TutorialStepsDataTable->FindRow<FTutorialStepData>(RowName, ContextString);
		if (StepData)
		{
			return *StepData;
		}
	}

	// Return default step data if not found in table
	FTutorialStepData DefaultStep;
	DefaultStep.StepType = Step;

	switch (Step)
	{
		case ETutorialStep::Welcome:
			DefaultStep.StepTitle = TEXT("Welcome to Odyssey");
			DefaultStep.StepDescription = TEXT("Learn the basics of space exploration");
			break;
		case ETutorialStep::Movement:
			DefaultStep.StepTitle = TEXT("Movement");
			DefaultStep.StepDescription = TEXT("Move around using touch controls");
			break;
		case ETutorialStep::Mining:
			DefaultStep.StepTitle = TEXT("Mining");
			DefaultStep.StepDescription = TEXT("Extract resources from nodes");
			break;
		case ETutorialStep::Inventory:
			DefaultStep.StepTitle = TEXT("Inventory");
			DefaultStep.StepDescription = TEXT("Manage your collected resources");
			break;
		case ETutorialStep::Crafting:
			DefaultStep.StepTitle = TEXT("Crafting");
			DefaultStep.StepDescription = TEXT("Refine materials into valuable goods");
			break;
		case ETutorialStep::Trading:
			DefaultStep.StepTitle = TEXT("Trading");
			DefaultStep.StepDescription = TEXT("Sell goods for OMEN currency");
			break;
		case ETutorialStep::Upgrades:
			DefaultStep.StepTitle = TEXT("Upgrades");
			DefaultStep.StepDescription = TEXT("Purchase improvements for your ship");
			break;
		case ETutorialStep::Completion:
			DefaultStep.StepTitle = TEXT("Mastery");
			DefaultStep.StepDescription = TEXT("You've mastered the basics!");
			break;
	}

	return DefaultStep;
}

float UOdysseyTutorialManager::GetDemoProgress() const
{
	return 1.0f - (DemoTimeRemaining / DemoTimeLimit);
}

bool UOdysseyTutorialManager::IsDemoTimeExpired() const
{
	return DemoTimeRemaining <= 0.0f;
}

// Action tracking functions
void UOdysseyTutorialManager::OnPlayerMoved()
{
	if (!bPlayerHasMoved)
	{
		bPlayerHasMoved = true;
		if (TutorialProgress.CurrentStep == ETutorialStep::Movement)
		{
			CompleteObjective(TEXT("Move using the virtual joystick"));
			AdvanceToNextStep();
		}
	}
}

void UOdysseyTutorialManager::OnResourceMined(int32 ResourceType, int32 Amount)
{
	if (!bPlayerHasMinedResource)
	{
		bPlayerHasMinedResource = true;
		if (TutorialProgress.CurrentStep == ETutorialStep::Mining)
		{
			CompleteObjective(TEXT("Mine your first resource"));
			AdvanceToNextStep();
		}
	}
}

void UOdysseyTutorialManager::OnItemCrafted(const FString& ItemName)
{
	if (!bPlayerHasCraftedItem)
	{
		bPlayerHasCraftedItem = true;
		if (TutorialProgress.CurrentStep == ETutorialStep::Crafting)
		{
			CompleteObjective(TEXT("Craft your first item"));
			AdvanceToNextStep();
		}
	}
}

void UOdysseyTutorialManager::OnResourceSold(int32 ResourceType, int32 Amount, int32 OMENEarned)
{
	if (!bPlayerHasSoldResource)
	{
		bPlayerHasSoldResource = true;
		if (TutorialProgress.CurrentStep == ETutorialStep::Trading)
		{
			CompleteObjective(TEXT("Sell resources for OMEN"));
			AdvanceToNextStep();
		}
	}
}

void UOdysseyTutorialManager::OnUpgradePurchased(const FString& UpgradeName)
{
	if (!bPlayerHasPurchasedUpgrade)
	{
		bPlayerHasPurchasedUpgrade = true;
		if (TutorialProgress.CurrentStep == ETutorialStep::Upgrades)
		{
			CompleteObjective(TEXT("Purchase your first upgrade"));
			AdvanceToNextStep();
		}
	}
}

void UOdysseyTutorialManager::OnInventoryOpened()
{
	if (!bPlayerHasOpenedInventory)
	{
		bPlayerHasOpenedInventory = true;
		if (TutorialProgress.CurrentStep == ETutorialStep::Inventory)
		{
			CompleteObjective(TEXT("Open your inventory"));
			AdvanceToNextStep();
		}
	}
}

void UOdysseyTutorialManager::InitializeTutorial()
{
	// Initialize tutorial steps and objectives
	DemoTimeRemaining = DemoTimeLimit;
}

void UOdysseyTutorialManager::UpdateStepProgress(float DeltaTime)
{
	StepTimer += DeltaTime;
	TotalTutorialTime += DeltaTime;

	// Calculate step progress based on time and completion
	FTutorialStepData CurrentStepData = GetCurrentStepData();

	if (CurrentStepData.TriggerType == ETutorialTriggerType::TimeBased)
	{
		float TimeProgress = StepTimer / FMath::Max(CurrentStepData.AutoTriggerDelay, 1.0f);
		TutorialProgress.StepProgress = FMath::Clamp(TimeProgress, 0.0f, 1.0f);
	}
	else
	{
		// Progress based on objectives completion
		int32 CompletedObjectives = 0;
		for (const FTutorialObjective& Objective : CurrentObjectives)
		{
			if (Objective.bIsCompleted)
				CompletedObjectives++;
		}

		if (CurrentObjectives.Num() > 0)
		{
			TutorialProgress.StepProgress = (float)CompletedObjectives / (float)CurrentObjectives.Num();
		}
	}
}

void UOdysseyTutorialManager::CheckStepCompletion()
{
	if (TutorialProgress.bStepCompleted)
		return;

	FTutorialStepData CurrentStepData = GetCurrentStepData();

	// Check if step should auto-complete
	if (CurrentStepData.TriggerType == ETutorialTriggerType::TimeBased)
	{
		if (StepTimer >= CurrentStepData.AutoTriggerDelay)
		{
			AdvanceToNextStep();
		}
	}
	else if (CurrentStepData.TriggerType == ETutorialTriggerType::PerformanceBased)
	{
		// Check performance-based completion conditions
		if (CheckCompletionConditions(CurrentStepData))
		{
			AdvanceToNextStep();
		}
	}
}

void UOdysseyTutorialManager::TriggerNextStep()
{
	AdvanceToNextStep();
}

bool UOdysseyTutorialManager::CheckCompletionConditions(const FTutorialStepData& StepData) const
{
	// Check if all required objectives are completed
	for (const FTutorialObjective& Objective : CurrentObjectives)
	{
		if (!Objective.bIsOptional && !Objective.bIsCompleted)
		{
			return false;
		}
	}

	return true;
}

void UOdysseyTutorialManager::UpdateDemoTimer(float DeltaTime)
{
	if (DemoTimeRemaining > 0.0f)
	{
		DemoTimeRemaining -= DeltaTime;

		// Trigger warnings at specific time thresholds
		if (DemoTimeRemaining <= 120.0f && DemoTimeRemaining > 119.0f) // 2 minutes left
		{
			OnDemoTimeWarning(DemoTimeRemaining);
		}
		else if (DemoTimeRemaining <= 30.0f && DemoTimeRemaining > 29.0f) // 30 seconds left
		{
			OnDemoTimeWarning(DemoTimeRemaining);
		}

		if (DemoTimeRemaining <= 0.0f)
		{
			DemoTimeRemaining = 0.0f;
			OnDemoCompleted();
		}
	}
}

void UOdysseyTutorialManager::SetupStepObjectives(ETutorialStep Step)
{
	switch (Step)
	{
		case ETutorialStep::Welcome:
			AddObjective(TEXT("Welcome to Odyssey! Let's learn the basics of space exploration."));
			break;
		case ETutorialStep::Movement:
			AddObjective(TEXT("Move using the virtual joystick"));
			break;
		case ETutorialStep::Mining:
			AddObjective(TEXT("Approach a resource node and tap to mine"));
			break;
		case ETutorialStep::Inventory:
			AddObjective(TEXT("Open your inventory to see collected resources"));
			break;
		case ETutorialStep::Crafting:
			AddObjective(TEXT("Use the crafting station to refine materials"));
			break;
		case ETutorialStep::Trading:
			AddObjective(TEXT("Sell your goods at the trading station"));
			break;
		case ETutorialStep::Upgrades:
			AddObjective(TEXT("Purchase an upgrade with your OMEN"));
			break;
		case ETutorialStep::Completion:
			AddObjective(TEXT("You've mastered the core Odyssey gameplay loop!"));
			break;
	}
}