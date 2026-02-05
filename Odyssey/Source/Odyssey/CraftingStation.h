#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "OdysseyCraftingComponent.h"
#include "CraftingStation.generated.h"

UENUM(BlueprintType)
enum class ECraftingStationType : uint8
{
	Basic,
	Advanced,
	Industrial
};

UCLASS()
class ODYSSEY_API ACraftingStation : public AActor
{
	GENERATED_BODY()

public:
	ACraftingStation();

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StationMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractionSphere;

	// Station properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station")
	ECraftingStationType StationType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station")
	float CraftingSpeedBonus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station")
	float CraftingSuccessBonus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station")
	int32 AdditionalCraftingSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Station")
	TArray<FString> AvailableRecipeCategories;

	// Visual effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	bool bShowParticleEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float OperatingEffectIntensity;

	// Current user
	UPROPERTY(BlueprintReadOnly, Category = "Crafting Station")
	class AOdysseyCharacter* CurrentUser;

protected:
	virtual void BeginPlay() override;

	// Interaction events
	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	virtual void Tick(float DeltaTime) override;

	// Station interaction
	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	bool CanPlayerUseStation(AOdysseyCharacter* Player) const;

	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	bool StartUsingStation(AOdysseyCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	void StopUsingStation();

	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	bool IsStationInUse() const { return CurrentUser != nullptr; }

	// Station bonuses
	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	void ApplyStationBonuses(UOdysseyCraftingComponent* CraftingComponent);

	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	void RemoveStationBonuses(UOdysseyCraftingComponent* CraftingComponent);

	// Recipe filtering
	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	TArray<FName> GetAvailableRecipesForStation(UOdysseyCraftingComponent* CraftingComponent) const;

	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	bool CanCraftRecipeAtStation(FName RecipeID) const;

	// Station info
	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	ECraftingStationType GetStationType() const { return StationType; }

	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	float GetCraftingSpeedBonus() const { return CraftingSpeedBonus; }

	UFUNCTION(BlueprintCallable, Category = "Crafting Station")
	float GetCraftingSuccessBonus() const { return CraftingSuccessBonus; }

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting Station Events")
	void OnPlayerStartedUsing(AOdysseyCharacter* Player);

	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting Station Events")
	void OnPlayerStoppedUsing(AOdysseyCharacter* Player);

	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting Station Events")
	void OnCraftingStartedAtStation(FName RecipeID);

	UFUNCTION(BlueprintImplementableEvent, Category = "Crafting Station Events")
	void OnCraftingCompletedAtStation(FName RecipeID);

protected:
	void UpdateVisualEffects(float DeltaTime);

private:
	float OperatingTimer;
	float BaseCraftingSpeedMultiplier;
	float BaseCraftingSuccessBonus;
	int32 BaseCraftingSlots;
};