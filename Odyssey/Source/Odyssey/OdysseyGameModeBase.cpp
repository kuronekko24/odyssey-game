#include "OdysseyGameModeBase.h"
#include "OdysseyCharacter.h"
#include "OdysseyCameraPawn.h"
#include "OdysseyPlayerController.h"
#include "OdysseyTutorialManager.h"
#include "Kismet/GameplayStatics.h"

AOdysseyGameModeBase::AOdysseyGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set default pawn and controller classes
	DefaultPawnClass = AOdysseyCharacter::StaticClass();
	PlayerControllerClass = AOdysseyPlayerController::StaticClass();

	// Create tutorial manager component
	TutorialManager = CreateDefaultSubobject<UOdysseyTutorialManager>(TEXT("TutorialManager"));

	bTutorialCompleted = false;
	TutorialStep = 0;
	MaxDemoTime = 600.0f; // 10 minutes
	DemoTimeRemaining = MaxDemoTime;
}

void AOdysseyGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	StartDemo();
}

void AOdysseyGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DemoTimeRemaining > 0.0f)
	{
		DemoTimeRemaining -= DeltaTime;
		OnDemoTimeUpdate(DemoTimeRemaining);

		if (DemoTimeRemaining <= 0.0f)
		{
			OnDemoComplete();
		}
	}
}

void AOdysseyGameModeBase::AdvanceTutorialStep()
{
	TutorialStep++;
	if (TutorialStep >= 5) // Complete after 5 tutorial steps
	{
		bTutorialCompleted = true;
	}
}

void AOdysseyGameModeBase::StartDemo()
{
	DemoTimeRemaining = MaxDemoTime;
	TutorialStep = 0;
	bTutorialCompleted = false;

	// Start tutorial if available
	if (TutorialManager)
	{
		TutorialManager->StartTutorial();
	}
}

void AOdysseyGameModeBase::StartTutorial()
{
	if (TutorialManager)
	{
		TutorialManager->StartTutorial();
	}
}

void AOdysseyGameModeBase::SkipTutorial()
{
	if (TutorialManager)
	{
		TutorialManager->SkipTutorial();
	}
}