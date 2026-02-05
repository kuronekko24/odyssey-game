#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "OdysseyInventoryComponent.h"
#include "ResourceNode.generated.h"

UENUM(BlueprintType)
enum class EResourceNodeState : uint8
{
	Full,
	Depleting,
	Depleted,
	Regenerating
};

USTRUCT(BlueprintType)
struct ODYSSEY_API FResourceNodeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Node")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Node")
	int32 MaxResourceAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Node")
	int32 CurrentResourceAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Node")
	float MiningDifficulty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Node")
	float RegenerationRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Node")
	bool bCanRegenerate;

	FResourceNodeData()
	{
		ResourceType = EResourceType::Silicate;
		MaxResourceAmount = 50;
		CurrentResourceAmount = 50;
		MiningDifficulty = 1.0f;
		RegenerationRate = 0.1f;
		bCanRegenerate = true;
	}
};

UCLASS()
class ODYSSEY_API AResourceNode : public AActor
{
	GENERATED_BODY()

public:
	AResourceNode();

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ResourceMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractionCollision;

	// Resource data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Node")
	FResourceNodeData NodeData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Node")
	EResourceNodeState CurrentState;

	// Visual settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
	UMaterialInterface* FullStateMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
	UMaterialInterface* DepletingStateMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
	UMaterialInterface* DepletedStateMaterial;

	// Animation/Effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float PulseSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float PulseIntensity;

	// Regeneration
	UPROPERTY(BlueprintReadOnly, Category = "Resource Node")
	float RegenerationTimer;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Resource mining
	UFUNCTION(BlueprintCallable, Category = "Mining")
	bool CanBeMined() const;

	UFUNCTION(BlueprintCallable, Category = "Mining")
	bool MineResource(int32 AmountToMine);

	UFUNCTION(BlueprintCallable, Category = "Mining")
	int32 GetOptimalMiningAmount(float MinerPower) const;

	// Resource node info
	UFUNCTION(BlueprintCallable, Category = "Resource Node")
	EResourceType GetResourceType() const { return NodeData.ResourceType; }

	UFUNCTION(BlueprintCallable, Category = "Resource Node")
	int32 GetCurrentResourceAmount() const { return NodeData.CurrentResourceAmount; }

	UFUNCTION(BlueprintCallable, Category = "Resource Node")
	int32 GetMaxResourceAmount() const { return NodeData.MaxResourceAmount; }

	UFUNCTION(BlueprintCallable, Category = "Resource Node")
	float GetResourcePercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Resource Node")
	EResourceNodeState GetNodeState() const { return CurrentState; }

	// Node management
	UFUNCTION(BlueprintCallable, Category = "Resource Node")
	void SetResourceData(const FResourceNodeData& NewData);

	UFUNCTION(BlueprintCallable, Category = "Resource Node")
	void RegenerateResource(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Resource Node")
	void ResetToFull();

	// Visual updates
	UFUNCTION(BlueprintCallable, Category = "Visuals")
	void UpdateVisualState();

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "Resource Node Events")
	void OnResourceMined(int32 AmountMined);

	UFUNCTION(BlueprintImplementableEvent, Category = "Resource Node Events")
	void OnNodeDepleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Resource Node Events")
	void OnNodeRegenerated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Resource Node Events")
	void OnStateChanged(EResourceNodeState OldState, EResourceNodeState NewState);

protected:
	void UpdateNodeState();
	void HandleRegeneration(float DeltaTime);
	void UpdatePulseEffect(float DeltaTime);

private:
	float PulseTimer;
};