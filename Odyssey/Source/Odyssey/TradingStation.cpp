#include "TradingStation.h"
#include "OdysseyCharacter.h"
#include "OdysseyTradingComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"

ATradingStation::ATradingStation()
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
	InteractionSphere->SetSphereRadius(400.0f); // Larger than crafting station
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Tag as interactable
	Tags.Add(TEXT("Interactable"));
	Tags.Add(TEXT("TradingStation"));

	// Default station settings
	StationType = ETradingStationType::Basic;
	PriceBonusMultiplier = 1.0f; // No bonus by default
	bOffersUpgrades = true;

	// Visual settings
	bShowHolographicPrices = true;
	HologramUpdateInterval = 5.0f;
	HologramTimer = 0.0f;

	// Supported resources (all by default)
	SupportedResourceCategories.Add(TEXT("Raw Materials"));
	SupportedResourceCategories.Add(TEXT("Refined Materials"));
	SupportedResourceCategories.Add(TEXT("Advanced Materials"));

	CurrentUser = nullptr;

	// Bind overlap events
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATradingStation::OnInteractionSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ATradingStation::OnInteractionSphereEndOverlap);
}

void ATradingStation::BeginPlay()
{
	Super::BeginPlay();

	// Configure station based on type
	switch (StationType)
	{
		case ETradingStationType::Basic:
			PriceBonusMultiplier = 1.0f; // Standard prices
			bOffersUpgrades = true;
			break;

		case ETradingStationType::Advanced:
			PriceBonusMultiplier = 1.1f; // 10% better prices
			bOffersUpgrades = true;
			break;

		case ETradingStationType::Premium:
			PriceBonusMultiplier = 1.2f; // 20% better prices
			bOffersUpgrades = true;
			break;
	}

	UE_LOG(LogTemp, Warning, TEXT("Trading station initialized: Type %d, Price Bonus: %f"),
		(int32)StationType, PriceBonusMultiplier);
}

void ATradingStation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update holographic display
	if (bShowHolographicPrices)
	{
		UpdateHolographicDisplay(DeltaTime);
	}
}

void ATradingStation::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AOdysseyCharacter* Player = Cast<AOdysseyCharacter>(OtherActor);
	if (Player && CanPlayerUseStation(Player))
	{
		// Player can potentially use this trading station
		UE_LOG(LogTemp, Verbose, TEXT("Player entered trading station interaction range"));
	}
}

void ATradingStation::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AOdysseyCharacter* Player = Cast<AOdysseyCharacter>(OtherActor);
	if (Player == CurrentUser)
	{
		// Player left the interaction range while trading
		StopUsingStation();
	}
}

bool ATradingStation::CanPlayerUseStation(AOdysseyCharacter* Player) const
{
	if (!Player)
		return false;

	// Check if player has trading component (through character)
	// For now, all characters can trade

	// Check if station is already in use
	if (IsStationInUse() && CurrentUser != Player)
		return false;

	return true;
}

bool ATradingStation::StartUsingStation(AOdysseyCharacter* Player)
{
	if (!CanPlayerUseStation(Player))
		return false;

	CurrentUser = Player;

	OnPlayerStartedTrading(Player);

	UE_LOG(LogTemp, Warning, TEXT("Player %s started using trading station"), *Player->GetName());

	return true;
}

void ATradingStation::StopUsingStation()
{
	if (!CurrentUser)
		return;

	AOdysseyCharacter* Player = CurrentUser;
	CurrentUser = nullptr;

	OnPlayerStoppedTrading(Player);

	UE_LOG(LogTemp, Warning, TEXT("Player %s stopped using trading station"), *Player->GetName());
}

bool ATradingStation::SellResourceAtStation(EResourceType ResourceType, int32 Quantity)
{
	if (!CurrentUser)
		return false;

	// Get player's trading component (we'll add it to character)
	// For now, use the character's built-in trading functionality
	UOdysseyInventoryComponent* Inventory = CurrentUser->GetInventoryComponent();
	if (!Inventory)
		return false;

	// Calculate price with station bonus
	int32 BasePrice = 0;
	switch (ResourceType)
	{
		case EResourceType::Silicate:
			BasePrice = 2;
			break;
		case EResourceType::Carbon:
			BasePrice = 3;
			break;
		case EResourceType::RefinedSilicate:
			BasePrice = 10;
			break;
		case EResourceType::RefinedCarbon:
			BasePrice = 15;
			break;
		case EResourceType::CompositeMaterial:
			BasePrice = 35;
			break;
		default:
			BasePrice = 1;
			break;
	}

	int32 TotalPrice = FMath::RoundToInt(BasePrice * Quantity * PriceBonusMultiplier);

	// Check if player has the resource
	if (!Inventory->HasResource(ResourceType, Quantity))
		return false;

	// Perform the trade
	if (Inventory->RemoveResource(ResourceType, Quantity))
	{
		Inventory->AddResource(EResourceType::OMEN, TotalPrice);

		OnResourceTraded(ResourceType, Quantity, true);

		UE_LOG(LogTemp, Warning, TEXT("Player sold %d %s for %d OMEN at station"),
			Quantity, *Inventory->GetResourceName(ResourceType), TotalPrice);

		return true;
	}

	return false;
}

bool ATradingStation::BuyResourceAtStation(EResourceType ResourceType, int32 Quantity)
{
	// For demo, players can only sell resources, not buy them
	// This could be implemented later for more complex trading
	return false;
}

int32 ATradingStation::GetStationSellPrice(EResourceType ResourceType, int32 Quantity) const
{
	int32 BasePrice = 0;
	switch (ResourceType)
	{
		case EResourceType::Silicate:
			BasePrice = 2;
			break;
		case EResourceType::Carbon:
			BasePrice = 3;
			break;
		case EResourceType::RefinedSilicate:
			BasePrice = 10;
			break;
		case EResourceType::RefinedCarbon:
			BasePrice = 15;
			break;
		case EResourceType::CompositeMaterial:
			BasePrice = 35;
			break;
		default:
			BasePrice = 1;
			break;
	}

	return FMath::RoundToInt(BasePrice * Quantity * PriceBonusMultiplier);
}

int32 ATradingStation::GetStationBuyPrice(EResourceType ResourceType, int32 Quantity) const
{
	// Not implemented for demo
	return 0;
}

TArray<FName> ATradingStation::GetAvailableUpgrades() const
{
	TArray<FName> AvailableUpgrades;

	if (bOffersUpgrades)
	{
		// Return basic upgrades for demo
		AvailableUpgrades.Add(FName("MiningPowerUpgrade"));
		AvailableUpgrades.Add(FName("MiningSpeedUpgrade"));
		AvailableUpgrades.Add(FName("InventoryUpgrade"));
		AvailableUpgrades.Add(FName("CraftingSpeedUpgrade"));
	}

	return AvailableUpgrades;
}

bool ATradingStation::PurchaseUpgradeAtStation(FName UpgradeID)
{
	if (!CurrentUser || !bOffersUpgrades)
		return false;

	UOdysseyInventoryComponent* Inventory = CurrentUser->GetInventoryComponent();
	if (!Inventory)
		return false;

	// Define upgrade costs and effects (would normally come from data table)
	int32 Cost = 0;
	float MiningPowerIncrease = 0.0f;
	float MiningSpeedIncrease = 0.0f;
	int32 InventoryIncrease = 0;

	if (UpgradeID == FName("MiningPowerUpgrade"))
	{
		Cost = 50;
		MiningPowerIncrease = 0.5f;
	}
	else if (UpgradeID == FName("MiningSpeedUpgrade"))
	{
		Cost = 75;
		MiningSpeedIncrease = 0.3f;
	}
	else if (UpgradeID == FName("InventoryUpgrade"))
	{
		Cost = 100;
		InventoryIncrease = 5;
	}
	else if (UpgradeID == FName("CraftingSpeedUpgrade"))
	{
		Cost = 80;
		// Crafting speed handled separately
	}
	else
	{
		return false; // Unknown upgrade
	}

	// Check if player has enough OMEN
	if (!Inventory->HasResource(EResourceType::OMEN, Cost))
		return false;

	// Purchase the upgrade
	if (Inventory->RemoveResource(EResourceType::OMEN, Cost))
	{
		// Apply upgrade effects
		if (MiningPowerIncrease > 0.0f)
		{
			CurrentUser->UpgradeMiningPower(MiningPowerIncrease);
		}
		if (MiningSpeedIncrease > 0.0f)
		{
			CurrentUser->UpgradeMiningSpeed(MiningSpeedIncrease);
		}
		if (InventoryIncrease > 0)
		{
			CurrentUser->UpgradeInventoryCapacity(InventoryIncrease);
		}

		OnUpgradePurchased(UpgradeID);

		UE_LOG(LogTemp, Warning, TEXT("Player purchased upgrade %s for %d OMEN"),
			*UpgradeID.ToString(), Cost);

		return true;
	}

	return false;
}

void ATradingStation::UpdateHolographicDisplay(float DeltaTime)
{
	HologramTimer += DeltaTime;

	if (HologramTimer >= HologramUpdateInterval)
	{
		// Update holographic price display
		OnPricesUpdated();
		HologramTimer = 0.0f;
	}
}

float ATradingStation::CalculateStationPriceModifier() const
{
	return PriceBonusMultiplier;
}