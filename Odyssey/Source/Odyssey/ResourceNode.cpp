#include "ResourceNode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"

AResourceNode::AResourceNode()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create mesh component
	ResourceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResourceMesh"));
	ResourceMesh->SetupAttachment(RootComponent);
	ResourceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ResourceMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Create interaction sphere
	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(RootComponent);
	InteractionCollision->SetSphereRadius(200.0f);
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Tag as interactable
	Tags.Add(TEXT("Interactable"));

	// Initialize resource data
	NodeData.ResourceType = EResourceType::Silicate;
	NodeData.MaxResourceAmount = 50;
	NodeData.CurrentResourceAmount = 50;
	NodeData.MiningDifficulty = 1.0f;
	NodeData.RegenerationRate = 0.1f; // Resources per second
	NodeData.bCanRegenerate = true;

	CurrentState = EResourceNodeState::Full;

	// Visual settings
	PulseSpeed = 2.0f;
	PulseIntensity = 0.3f;
	PulseTimer = 0.0f;
	RegenerationTimer = 0.0f;
}

void AResourceNode::BeginPlay()
{
	Super::BeginPlay();

	UpdateVisualState();

	UE_LOG(LogTemp, Warning, TEXT("Resource node spawned: %s with %d resources"),
		*UEnum::GetValueAsString(NodeData.ResourceType), NodeData.CurrentResourceAmount);
}

void AResourceNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle regeneration
	if (NodeData.bCanRegenerate)
	{
		HandleRegeneration(DeltaTime);
	}

	// Update visual effects
	UpdatePulseEffect(DeltaTime);

	// Update state if needed
	UpdateNodeState();
}

bool AResourceNode::CanBeMined() const
{
	return CurrentState != EResourceNodeState::Depleted && NodeData.CurrentResourceAmount > 0;
}

bool AResourceNode::MineResource(int32 AmountToMine)
{
	if (!CanBeMined() || AmountToMine <= 0)
		return false;

	// Calculate actual amount that can be mined based on difficulty
	float EffectiveAmount = AmountToMine / NodeData.MiningDifficulty;
	int32 ActualAmount = FMath::Min(FMath::RoundToInt(EffectiveAmount), NodeData.CurrentResourceAmount);

	if (ActualAmount > 0)
	{
		NodeData.CurrentResourceAmount -= ActualAmount;
		NodeData.CurrentResourceAmount = FMath::Max(0, NodeData.CurrentResourceAmount);

		OnResourceMined(ActualAmount);

		UE_LOG(LogTemp, Verbose, TEXT("Mined %d %s from resource node. Remaining: %d"),
			ActualAmount, *UEnum::GetValueAsString(NodeData.ResourceType), NodeData.CurrentResourceAmount);

		// Check if depleted
		if (NodeData.CurrentResourceAmount == 0)
		{
			OnNodeDepleted();
		}

		return true;
	}

	return false;
}

int32 AResourceNode::GetOptimalMiningAmount(float MinerPower) const
{
	if (!CanBeMined())
		return 0;

	// Calculate optimal mining amount based on miner power and node difficulty
	float OptimalAmount = MinerPower / NodeData.MiningDifficulty;
	return FMath::Min(FMath::RoundToInt(OptimalAmount), NodeData.CurrentResourceAmount);
}

float AResourceNode::GetResourcePercentage() const
{
	if (NodeData.MaxResourceAmount <= 0)
		return 0.0f;

	return (float)NodeData.CurrentResourceAmount / (float)NodeData.MaxResourceAmount;
}

void AResourceNode::SetResourceData(const FResourceNodeData& NewData)
{
	NodeData = NewData;
	UpdateVisualState();

	UE_LOG(LogTemp, Warning, TEXT("Resource node data updated: %s, Amount: %d/%d"),
		*UEnum::GetValueAsString(NodeData.ResourceType), NodeData.CurrentResourceAmount, NodeData.MaxResourceAmount);
}

void AResourceNode::RegenerateResource(int32 Amount)
{
	if (Amount <= 0)
		return;

	int32 OldAmount = NodeData.CurrentResourceAmount;
	NodeData.CurrentResourceAmount = FMath::Min(NodeData.CurrentResourceAmount + Amount, NodeData.MaxResourceAmount);

	if (NodeData.CurrentResourceAmount > OldAmount)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Resource node regenerated %d resources. Current: %d/%d"),
			NodeData.CurrentResourceAmount - OldAmount, NodeData.CurrentResourceAmount, NodeData.MaxResourceAmount);

		if (OldAmount == 0 && NodeData.CurrentResourceAmount > 0)
		{
			OnNodeRegenerated();
		}
	}
}

void AResourceNode::ResetToFull()
{
	NodeData.CurrentResourceAmount = NodeData.MaxResourceAmount;
	UpdateVisualState();

	UE_LOG(LogTemp, Warning, TEXT("Resource node reset to full capacity"));
}

void AResourceNode::UpdateVisualState()
{
	if (!ResourceMesh)
		return;

	UMaterialInterface* MaterialToUse = nullptr;

	switch (CurrentState)
	{
		case EResourceNodeState::Full:
			MaterialToUse = FullStateMaterial;
			break;
		case EResourceNodeState::Depleting:
			MaterialToUse = DepletingStateMaterial;
			break;
		case EResourceNodeState::Depleted:
			MaterialToUse = DepletedStateMaterial;
			break;
		case EResourceNodeState::Regenerating:
			MaterialToUse = FullStateMaterial; // Use full material with pulse
			break;
	}

	if (MaterialToUse)
	{
		ResourceMesh->SetMaterial(0, MaterialToUse);
	}

	// Update scale based on resource amount
	float ResourcePercent = GetResourcePercentage();
	float MinScale = 0.5f;
	float MaxScale = 1.0f;
	float CurrentScale = FMath::Lerp(MinScale, MaxScale, ResourcePercent);
	ResourceMesh->SetRelativeScale3D(FVector(CurrentScale));
}

void AResourceNode::UpdateNodeState()
{
	EResourceNodeState OldState = CurrentState;
	EResourceNodeState NewState = CurrentState;

	float ResourcePercent = GetResourcePercentage();

	if (ResourcePercent >= 0.8f)
	{
		NewState = EResourceNodeState::Full;
	}
	else if (ResourcePercent > 0.0f)
	{
		if (NodeData.bCanRegenerate && RegenerationTimer > 0)
		{
			NewState = EResourceNodeState::Regenerating;
		}
		else
		{
			NewState = EResourceNodeState::Depleting;
		}
	}
	else
	{
		if (NodeData.bCanRegenerate)
		{
			NewState = EResourceNodeState::Regenerating;
		}
		else
		{
			NewState = EResourceNodeState::Depleted;
		}
	}

	if (NewState != OldState)
	{
		CurrentState = NewState;
		OnStateChanged(OldState, NewState);
		UpdateVisualState();
	}
}

void AResourceNode::HandleRegeneration(float DeltaTime)
{
	if (NodeData.CurrentResourceAmount >= NodeData.MaxResourceAmount)
		return;

	RegenerationTimer += DeltaTime;

	// Regenerate resources over time
	if (RegenerationTimer >= 1.0f) // Every second
	{
		int32 RegenAmount = FMath::RoundToInt(NodeData.RegenerationRate);
		if (RegenAmount > 0)
		{
			RegenerateResource(RegenAmount);
			RegenerationTimer = 0.0f;
		}
	}
}

void AResourceNode::UpdatePulseEffect(float DeltaTime)
{
	if (CurrentState != EResourceNodeState::Regenerating)
		return;

	PulseTimer += DeltaTime * PulseSpeed;

	// Create pulsing effect for regenerating nodes
	float PulseValue = FMath::Sin(PulseTimer) * PulseIntensity + 1.0f;

	if (ResourceMesh)
	{
		// This could be used to modify material parameters or scale
		// For now, we'll just maintain the pulse timer for Blueprint implementation
	}
}