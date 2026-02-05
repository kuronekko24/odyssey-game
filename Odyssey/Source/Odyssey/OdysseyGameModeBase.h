#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OdysseyGameModeBase.generated.h"

class AOdysseyCameraPawn;
class AOdysseyCharacter;
class UOdysseyTutorialManager;

UCLASS()
class ODYSSEY_API AOdysseyGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOdysseyGameModeBase();

	// Tutorial state management
	UPROPERTY(BlueprintReadWrite, Category = "Tutorial")
	bool bTutorialCompleted;

	UPROPERTY(BlueprintReadWrite, Category = "Tutorial")
	int32 TutorialStep;

	// Game session timer for 10-minute demo
	UPROPERTY(BlueprintReadWrite, Category = "Demo")
	float DemoTimeRemaining;

	UPROPERTY(BlueprintReadWrite, Category = "Demo")
	float MaxDemoTime;

	// Tutorial management
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UOdysseyTutorialManager* TutorialManager;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void AdvanceTutorialStep();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void StartDemo();

	UFUNCTION(BlueprintImplementableEvent, Category = "Demo")
	void OnDemoTimeUpdate(float TimeRemaining);

	UFUNCTION(BlueprintImplementableEvent, Category = "Demo")
	void OnDemoComplete();

	// Tutorial functions
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	UOdysseyTutorialManager* GetTutorialManager() const { return TutorialManager; }

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StartTutorial();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SkipTutorial();
};