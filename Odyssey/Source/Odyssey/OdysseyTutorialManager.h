#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "OdysseyTutorialManager.generated.h"

UENUM(BlueprintType)
enum class ETutorialStep : uint8
{
	Welcome = 0,
	Movement = 1,
	Mining = 2,
	Inventory = 3,
	Crafting = 4,
	Trading = 5,
	Upgrades = 6,
	Completion = 7
};

UENUM(BlueprintType)
enum class ETutorialTriggerType : uint8
{
	Automatic,
	InteractionBased,
	TimeBased,
	PerformanceBased
};

USTRUCT(BlueprintType, meta = (DataTable))
struct ODYSSEY_API FTutorialStepData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	ETutorialStep StepType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	FString StepTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	FString StepDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	FString DetailedInstructions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	ETutorialTriggerType TriggerType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	float AutoTriggerDelay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	TArray<FString> RequiredActions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	TArray<FString> CompletionConditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	bool bShowUIHighlight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	FString UIElementToHighlight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Step")
	bool bPauseGameplay;

	FTutorialStepData()
	{
		StepType = ETutorialStep::Welcome;
		StepTitle = TEXT("Tutorial Step");
		StepDescription = TEXT("Learn the basics");
		DetailedInstructions = TEXT("Follow the instructions");
		TriggerType = ETutorialTriggerType::Automatic;
		AutoTriggerDelay = 0.0f;
		bShowUIHighlight = false;
		UIElementToHighlight = TEXT("");
		bPauseGameplay = false;
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FTutorialProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial Progress")
	ETutorialStep CurrentStep;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial Progress")
	float StepProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial Progress")
	bool bStepCompleted;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial Progress")
	float TotalTutorialProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial Progress")
	int32 CompletedSteps;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial Progress")
	int32 TotalSteps;

	FTutorialProgress()
	{
		CurrentStep = ETutorialStep::Welcome;
		StepProgress = 0.0f;
		bStepCompleted = false;
		TotalTutorialProgress = 0.0f;
		CompletedSteps = 0;
		TotalSteps = 8;
	}
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FTutorialObjective
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Objective")
	FString ObjectiveText;

	UPROPERTY(BlueprintReadWrite, Category = "Objective")
	bool bIsCompleted;

	UPROPERTY(BlueprintReadWrite, Category = "Objective")
	bool bIsOptional;

	FTutorialObjective()
	{
		ObjectiveText = TEXT("");
		bIsCompleted = false;
		bIsOptional = false;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyTutorialManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyTutorialManager();

protected:
	// Tutorial configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Settings")
	UDataTable* TutorialStepsDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Settings")
	bool bEnableTutorial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Settings")
	bool bCanSkipTutorial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Settings")
	bool bShowTutorialHints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial Settings")
	float HintDisplayDuration;

	// Current tutorial state
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial State")
	FTutorialProgress TutorialProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial State")
	TArray<FTutorialObjective> CurrentObjectives;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial State")
	bool bTutorialActive;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial State")
	bool bTutorialCompleted;

	// Timing and triggers
	UPROPERTY(BlueprintReadOnly, Category = "Tutorial Timing")
	float StepStartTime;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial Timing")
	float TotalTutorialTime;

	// Demo integration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Settings")
	float DemoTimeLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo Settings")
	bool bShowTimeRemaining;

	UPROPERTY(BlueprintReadOnly, Category = "Demo Settings")
	float DemoTimeRemaining;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Tutorial control
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StartTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SkipTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void PauseTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ResumeTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void RestartTutorial();

	// Step management
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void AdvanceToNextStep();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void GoToStep(ETutorialStep TargetStep);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CompleteCurrentStep();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	bool IsStepCompleted(ETutorialStep Step) const;

	// Objective management
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void AddObjective(const FString& ObjectiveText, bool bIsOptional = false);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void CompleteObjective(const FString& ObjectiveText);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearObjectives();

	// Progress tracking
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	FTutorialProgress GetTutorialProgress() const { return TutorialProgress; }

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	float GetStepProgress() const;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	bool IsTutorialActive() const { return bTutorialActive; }

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	bool IsTutorialCompleted() const { return bTutorialCompleted; }

	// Tutorial data access
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	FTutorialStepData GetCurrentStepData() const;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	FTutorialStepData GetStepData(ETutorialStep Step) const;

	// Demo timing
	UFUNCTION(BlueprintCallable, Category = "Demo")
	float GetDemoTimeRemaining() const { return DemoTimeRemaining; }

	UFUNCTION(BlueprintCallable, Category = "Demo")
	float GetDemoProgress() const;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	bool IsDemoTimeExpired() const;

	// Action tracking for tutorial triggers
	UFUNCTION(BlueprintCallable, Category = "Tutorial Actions")
	void OnPlayerMoved();

	UFUNCTION(BlueprintCallable, Category = "Tutorial Actions")
	void OnResourceMined(int32 ResourceType, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Tutorial Actions")
	void OnItemCrafted(const FString& ItemName);

	UFUNCTION(BlueprintCallable, Category = "Tutorial Actions")
	void OnResourceSold(int32 ResourceType, int32 Amount, int32 OMENEarned);

	UFUNCTION(BlueprintCallable, Category = "Tutorial Actions")
	void OnUpgradePurchased(const FString& UpgradeName);

	UFUNCTION(BlueprintCallable, Category = "Tutorial Actions")
	void OnInventoryOpened();

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial Events")
	void OnTutorialStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial Events")
	void OnTutorialCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial Events")
	void OnTutorialSkipped();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial Events")
	void OnStepStarted(ETutorialStep Step, const FTutorialStepData& StepData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial Events")
	void OnStepCompleted(ETutorialStep Step);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial Events")
	void OnObjectiveAdded(const FString& ObjectiveText);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial Events")
	void OnObjectiveCompleted(const FString& ObjectiveText);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial Events")
	void OnDemoTimeWarning(float TimeRemaining);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial Events")
	void OnDemoCompleted();

private:
	void InitializeTutorial();
	void UpdateStepProgress(float DeltaTime);
	void CheckStepCompletion();
	void TriggerNextStep();
	bool CheckCompletionConditions(const FTutorialStepData& StepData) const;
	void UpdateDemoTimer(float DeltaTime);

	// Action tracking variables
	bool bPlayerHasMoved;
	bool bPlayerHasMinedResource;
	bool bPlayerHasCraftedItem;
	bool bPlayerHasSoldResource;
	bool bPlayerHasPurchasedUpgrade;
	bool bPlayerHasOpenedInventory;

	float StepTimer;
	TArray<ETutorialStep> CompletedSteps;
};