#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "OdysseyTradingComponent.h"
#include "TradingStation.generated.h"

UENUM(BlueprintType)
enum class ETradingStationType : uint8
{
	Basic,
	Advanced,
	Premium
};

UCLASS()
class ODYSSEY_API ATradingStation : public AActor
{
	GENERATED_BODY()

public:
	ATradingStation();

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StationMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractionSphere;

	// Station properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trading Station")
	ETradingStationType StationType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trading Station")
	float PriceBonusMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trading Station")
	TArray<FString> SupportedResourceCategories;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trading Station")
	bool bOffersUpgrades;

	// Visual settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	bool bShowHolographicPrices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	float HologramUpdateInterval;

	// Current user
	UPROPERTY(BlueprintReadOnly, Category = "Trading Station")
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
	UFUNCTION(BlueprintCallable, Category = "Trading Station")
	bool CanPlayerUseStation(AOdysseyCharacter* Player) const;

	UFUNCTION(BlueprintCallable, Category = "Trading Station")
	bool StartUsingStation(AOdysseyCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Trading Station")
	void StopUsingStation();

	UFUNCTION(BlueprintCallable, Category = "Trading Station")
	bool IsStationInUse() const { return CurrentUser != nullptr; }

	// Trading operations
	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool SellResourceAtStation(EResourceType ResourceType, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Trading")
	bool BuyResourceAtStation(EResourceType ResourceType, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Trading")
	int32 GetStationSellPrice(EResourceType ResourceType, int32 Quantity) const;

	UFUNCTION(BlueprintCallable, Category = "Trading")
	int32 GetStationBuyPrice(EResourceType ResourceType, int32 Quantity) const;

	// Upgrade operations
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	TArray<FName> GetAvailableUpgrades() const;

	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	bool PurchaseUpgradeAtStation(FName UpgradeID);

	// Station info
	UFUNCTION(BlueprintCallable, Category = "Trading Station")
	ETradingStationType GetStationType() const { return StationType; }

	UFUNCTION(BlueprintCallable, Category = "Trading Station")
	float GetPriceBonusMultiplier() const { return PriceBonusMultiplier; }

	UFUNCTION(BlueprintCallable, Category = "Trading Station")
	bool OffersUpgrades() const { return bOffersUpgrades; }

	// Events
	UFUNCTION(BlueprintImplementableEvent, Category = "Trading Station Events")
	void OnPlayerStartedTrading(AOdysseyCharacter* Player);

	UFUNCTION(BlueprintImplementableEvent, Category = "Trading Station Events")
	void OnPlayerStoppedTrading(AOdysseyCharacter* Player);

	UFUNCTION(BlueprintImplementableEvent, Category = "Trading Station Events")
	void OnResourceTraded(EResourceType ResourceType, int32 Quantity, bool bWasSelling);

	UFUNCTION(BlueprintImplementableEvent, Category = "Trading Station Events")
	void OnUpgradePurchased(FName UpgradeID);

	UFUNCTION(BlueprintImplementableEvent, Category = "Trading Station Events")
	void OnPricesUpdated();

protected:
	void UpdateHolographicDisplay(float DeltaTime);
	float CalculateStationPriceModifier() const;

private:
	float HologramTimer;
};