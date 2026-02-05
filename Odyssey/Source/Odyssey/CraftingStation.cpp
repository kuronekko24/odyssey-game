#include "CraftingStation.h"
#include "OdysseyCharacter.h"
#include "OdysseyCraftingComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"

ACraftingStation::ACraftingStation()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create station mesh
	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	StationMesh->SetupAttachment(RootComponent);
	StationMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StationMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Create interaction sphere
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(300.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Tag as interactable
	Tags.Add(TEXT("Interactable"));
	Tags.Add(TEXT("CraftingStation"));

	// Default station settings
	StationType = ECraftingStationType::Basic;
	CraftingSpeedBonus = 0.2f; // 20% faster crafting
	CraftingSuccessBonus = 0.1f; // 10% better success rate
	AdditionalCraftingSlots = 2;

	// Visual effects
	bShowParticleEffects = true;
	OperatingEffectIntensity = 1.0f;
	OperatingTimer = 0.0f;

	// Available recipe categories based on station type
	AvailableRecipeCategories.Add(TEXT("Basic Refining"));

	CurrentUser = nullptr;

	// Bind overlap events
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACraftingStation::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ACraftingStation::OnInteractionSphereEndOverlap);
}

void ACraftingStation::BeginPlay()
{
	Super::BeginPlay();

	// Configure station based on type
	switch (StationType)
	{
		case ECraftingStationType::Basic:
			CraftingSpeedBonus = 0.2f;
			CraftingSuccessBonus = 0.1f;
			AdditionalCraftingSlots = 2;
			AvailableRecipeCategories = {TEXT("Basic Refining")};
			break;

		case ECraftingStationType::Advanced:
			CraftingSpeedBonus = 0.4f;
			CraftingSuccessBonus = 0.2f;
			AdditionalCraftingSlots = 4;
			AvailableRecipeCategories = {TEXT("Basic Refining"), TEXT("Advanced Materials")};
			break;

		case ECraftingStationType::Industrial:
			CraftingSpeedBonus = 0.6f;
			CraftingSuccessBonus = 0.3f;
			AdditionalCraftingSlots = 6;
			AvailableRecipeCategories = {TEXT("Basic Refining"), TEXT("Advanced Materials"), TEXT("Batch Processing")};
			break;
	}

	UE_LOG(LogTemp, Warning, TEXT("Crafting station initialized: Type %d, Speed Bonus: %f, Success Bonus: %f"),
		(int32)StationType, CraftingSpeedBonus, CraftingSuccessBonus);
}

void ACraftingStation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update visual effects
	if (IsStationInUse() || bShowParticleEffects)
	{
		UpdateVisualEffects(DeltaTime);
	}
}

void ACraftingStation::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AOdysseyCharacter* Player = Cast<AOdysseyCharacter>(OtherActor);
	if (Player && CanPlayerUseStation(Player))
	{
		// Player can potentially use this station
		UE_LOG(LogTemp, Verbose, TEXT("Player entered crafting station interaction range"));
	}
}

void ACraftingStation::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AOdysseyCharacter* Player = Cast<AOdysseyCharacter>(OtherActor);
	if (Player == CurrentUser)
	{
		// Player left the interaction range while using station
		StopUsingStation();
	}
}

bool ACraftingStation::CanPlayerUseStation(AOdysseyCharacter* Player) const
{
	if (!Player)
		return false;

	// Check if player has crafting component
	UOdysseyCraftingComponent* CraftingComp = Player->GetCraftingComponent();
	if (!CraftingComp)
		return false;

	// Check if station is already in use
	if (IsStationInUse() && CurrentUser != Player)
		return false;

	return true;
}

bool ACraftingStation::StartUsingStation(AOdysseyCharacter* Player)
{
	if (!CanPlayerUseStation(Player))
		return false;

	CurrentUser = Player;

	// Apply station bonuses to player's crafting component
	UOdysseyCraftingComponent* CraftingComp = Player->GetCraftingComponent();
	if (CraftingComp)
	{
		ApplyStationBonuses(CraftingComp);
	}

	OnPlayerStartedUsing(Player);

	UE_LOG(LogTemp, Warning, TEXT("Player %s started using crafting station"), *Player->GetName());

	return true;
}

void ACraftingStation::StopUsingStation()
{
	if (!CurrentUser)
		return;

	AOdysseyCharacter* Player = CurrentUser;

	// Remove station bonuses from player's crafting component
	UOdysseyCraftingComponent* CraftingComp = Player->GetCraftingComponent();
	if (CraftingComp)
	{
		RemoveStationBonuses(CraftingComp);
	}

	CurrentUser = nullptr;

	OnPlayerStoppedUsing(Player);

	UE_LOG(LogTemp, Warning, TEXT("Player %s stopped using crafting station"), *Player->GetName());
}

void ACraftingStation::ApplyStationBonuses(UOdysseyCraftingComponent* CraftingComponent)
{
	if (!CraftingComponent)
		return;

	// Store original values
	BaseCraftingSpeedMultiplier = CraftingComponent->CraftingSpeedMultiplier;
	BaseCraftingSuccessBonus = CraftingComponent->CraftingSuccessBonus;
	BaseCraftingSlots = CraftingComponent->MaxCraftingSlots;

	// Apply bonuses
	CraftingComponent->CraftingSpeedMultiplier += CraftingSpeedBonus;
	CraftingComponent->CraftingSuccessBonus += CraftingSuccessBonus;
	CraftingComponent->MaxCraftingSlots += AdditionalCraftingSlots;

	UE_LOG(LogTemp, Verbose, TEXT("Applied station bonuses: Speed +%f, Success +%f, Slots +%d"),
		CraftingSpeedBonus, CraftingSuccessBonus, AdditionalCraftingSlots);
}

void ACraftingStation::RemoveStationBonuses(UOdysseyCraftingComponent* CraftingComponent)
{
	if (!CraftingComponent)
		return;

	// Restore original values
	CraftingComponent->CraftingSpeedMultiplier = BaseCraftingSpeedMultiplier;
	CraftingComponent->CraftingSuccessBonus = BaseCraftingSuccessBonus;
	CraftingComponent->MaxCraftingSlots = BaseCraftingSlots;

	UE_LOG(LogTemp, Verbose, TEXT("Removed station bonuses, restored original values"));
}

TArray<FName> ACraftingStation::GetAvailableRecipesForStation(UOdysseyCraftingComponent* CraftingComponent) const
{
	TArray<FName> FilteredRecipes;

	if (!CraftingComponent)
		return FilteredRecipes;

	// Get all available recipes from crafting component
	TArray<FName> AllRecipes = CraftingComponent->GetAvailableRecipes();

	for (const FName& RecipeID : AllRecipes)
	{
		if (CanCraftRecipeAtStation(RecipeID))
		{
			FilteredRecipes.Add(RecipeID);
		}
	}

	return FilteredRecipes;
}

bool ACraftingStation::CanCraftRecipeAtStation(FName RecipeID) const
{
	// For now, basic stations can craft all recipes
	// In the future, this could check recipe categories against AvailableRecipeCategories
	return true;
}

void ACraftingStation::UpdateVisualEffects(float DeltaTime)
{
	OperatingTimer += DeltaTime;

	// Create pulsing effect when station is in use
	if (IsStationInUse())
	{
		float PulseValue = FMath::Sin(OperatingTimer * 2.0f) * 0.5f + 0.5f;
		// This pulse value could be used to drive material parameters or particle effects
		// Implementation would be done in Blueprint or with particle systems
	}
}