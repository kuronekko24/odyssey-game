#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "OdysseyCharacter.generated.h"

class UOdysseyInventoryComponent;
class UOdysseyCraftingComponent;
class UOdysseyTradingComponent;
class AResourceNode;

UCLASS()
class ODYSSEY_API AOdysseyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOdysseyCharacter();

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShipMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UOdysseyInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UOdysseyCraftingComponent* CraftingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UOdysseyTradingComponent* TradingComponent;

	// Character stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	float MiningPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	float MiningSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	int32 InventoryCapacity;

	// Current interaction target
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	AResourceNode* CurrentResourceNode;

	// Movement settings for isometric view
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float IsometricMovementSpeed;

protected:
	virtual void BeginPlay() override;

	// Interaction system
	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Interaction functions
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	UFUNCTION(BlueprintCallable, Category = "Mining")
	void StartMining(AResourceNode* ResourceNode);

	UFUNCTION(BlueprintCallable, Category = "Mining")
	void StopMining();

	UFUNCTION(BlueprintImplementableEvent, Category = "Mining")
	void OnMiningStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Mining")
	void OnMiningStopped();

	UFUNCTION(BlueprintImplementableEvent, Category = "Mining")
	void OnResourceGathered(int32 ResourceType, int32 Amount);

	// Getters
	UFUNCTION(BlueprintCallable, Category = "Character")
	UOdysseyInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintCallable, Category = "Character")
	UOdysseyCraftingComponent* GetCraftingComponent() const { return CraftingComponent; }

	UFUNCTION(BlueprintCallable, Category = "Character")
	UOdysseyTradingComponent* GetTradingComponent() const { return TradingComponent; }

	UFUNCTION(BlueprintCallable, Category = "Character")
	float GetMiningPower() const { return MiningPower; }

	UFUNCTION(BlueprintCallable, Category = "Character")
	float GetMiningSpeed() const { return MiningSpeed; }

	// Upgrade system
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	void UpgradeMiningPower(float Increase);

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	void UpgradeMiningSpeed(float Increase);

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	void UpgradeInventoryCapacity(int32 Increase);

private:
	bool bIsMining;
	float MiningTimer;
};