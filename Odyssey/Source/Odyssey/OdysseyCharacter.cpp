#include "OdysseyCharacter.h"
#include "OdysseyInventoryComponent.h"
#include "OdysseyCraftingComponent.h"
#include "OdysseyTradingComponent.h"
#include "ResourceNode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"

AOdysseyCharacter::AOdysseyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set up collision capsule
	GetCapsuleComponent()->SetCapsuleSize(42.0f, 96.0f);

	// Create ship mesh component
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMesh->SetupAttachment(RootComponent);

	// Create interaction sphere
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(200.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Create inventory component
	InventoryComponent = CreateDefaultSubobject<UOdysseyInventoryComponent>(TEXT("InventoryComponent"));

	// Create crafting component
	CraftingComponent = CreateDefaultSubobject<UOdysseyCraftingComponent>(TEXT("CraftingComponent"));

	// Create trading component
	TradingComponent = CreateDefaultSubobject<UOdysseyTradingComponent>(TEXT("TradingComponent"));

	// Set default values
	MiningPower = 1.0f;
	MiningSpeed = 1.0f;
	InventoryCapacity = 10;
	IsometricMovementSpeed = 600.0f;
	bIsMining = false;
	MiningTimer = 0.0f;
	CurrentResourceNode = nullptr;

	// Configure character movement for isometric view
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0, 0, 1));
	GetCharacterMovement()->MaxWalkSpeed = IsometricMovementSpeed;

	// Bind overlap events
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AOdysseyCharacter::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AOdysseyCharacter::OnInteractionSphereEndOverlap);
}

void AOdysseyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (InventoryComponent)
	{
		InventoryComponent->SetMaxCapacity(InventoryCapacity);
	}

	// Link crafting component to inventory
	if (CraftingComponent && InventoryComponent)
	{
		CraftingComponent->SetInventoryComponent(InventoryComponent);
	}

	// Link trading component to inventory
	if (TradingComponent && InventoryComponent)
	{
		TradingComponent->SetInventoryComponent(InventoryComponent);
	}
}

void AOdysseyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle mining timing
	if (bIsMining && CurrentResourceNode)
	{
		MiningTimer += DeltaTime;
		float MiningInterval = 1.0f / MiningSpeed; // Time between mining operations

		if (MiningTimer >= MiningInterval)
		{
			// Attempt to mine resource
			if (CurrentResourceNode->CanBeMined())
			{
				int32 ResourceType = CurrentResourceNode->GetResourceType();
				int32 Amount = FMath::RoundToInt(MiningPower);

				if (CurrentResourceNode->MineResource(Amount) && InventoryComponent)
				{
					if (InventoryComponent->AddResource(ResourceType, Amount))
					{
						OnResourceGathered(ResourceType, Amount);
					}
				}
			}
			else
			{
				// Resource node is depleted, stop mining
				StopMining();
			}

			MiningTimer = 0.0f;
		}
	}
}

void AOdysseyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AOdysseyCharacter::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AResourceNode* ResourceNode = Cast<AResourceNode>(OtherActor))
	{
		CurrentResourceNode = ResourceNode;
	}
}

void AOdysseyCharacter::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AResourceNode* ResourceNode = Cast<AResourceNode>(OtherActor))
	{
		if (CurrentResourceNode == ResourceNode)
		{
			if (bIsMining)
			{
				StopMining();
			}
			CurrentResourceNode = nullptr;
		}
	}
}

void AOdysseyCharacter::TryInteract()
{
	if (CurrentResourceNode && CurrentResourceNode->CanBeMined())
	{
		if (!bIsMining)
		{
			StartMining(CurrentResourceNode);
		}
		else
		{
			StopMining();
		}
	}
}

void AOdysseyCharacter::StartMining(AResourceNode* ResourceNode)
{
	if (ResourceNode && !bIsMining)
	{
		bIsMining = true;
		MiningTimer = 0.0f;
		CurrentResourceNode = ResourceNode;
		OnMiningStarted();

		UE_LOG(LogTemp, Warning, TEXT("Started mining resource node"));
	}
}

void AOdysseyCharacter::StopMining()
{
	if (bIsMining)
	{
		bIsMining = false;
		MiningTimer = 0.0f;
		OnMiningStopped();

		UE_LOG(LogTemp, Warning, TEXT("Stopped mining"));
	}
}

void AOdysseyCharacter::UpgradeMiningPower(float Increase)
{
	MiningPower += Increase;
	UE_LOG(LogTemp, Warning, TEXT("Mining power upgraded to: %f"), MiningPower);
}

void AOdysseyCharacter::UpgradeMiningSpeed(float Increase)
{
	MiningSpeed += Increase;
	UE_LOG(LogTemp, Warning, TEXT("Mining speed upgraded to: %f"), MiningSpeed);
}

void AOdysseyCharacter::UpgradeInventoryCapacity(int32 Increase)
{
	InventoryCapacity += Increase;
	if (InventoryComponent)
	{
		InventoryComponent->SetMaxCapacity(InventoryCapacity);
	}
	UE_LOG(LogTemp, Warning, TEXT("Inventory capacity upgraded to: %d"), InventoryCapacity);
}