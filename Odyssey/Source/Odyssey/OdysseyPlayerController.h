#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "OdysseyPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class AOdysseyCharacter;
class UOdysseyTouchInterface;

UCLASS()
class ODYSSEY_API AOdysseyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AOdysseyPlayerController();

protected:
	// Input Mapping Context for mobile controls
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* OdysseyMappingContext;

	// Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* TouchAction;

	// Touch interface component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UOdysseyTouchInterface* TouchInterface;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// Input handling functions
	void Move(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void Touch(const FInputActionValue& Value);

	// Mobile-specific input handling
	UFUNCTION(BlueprintCallable, Category = "Input")
	void HandleTouchInput(FVector2D TouchLocation);

	UFUNCTION(BlueprintCallable, Category = "Input")
	FVector2D ConvertTouchToWorldDirection(FVector2D TouchLocation);

	// Touch events from interface
	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	void OnTouchMovementInput(FVector2D MovementInput);

	UFUNCTION(BlueprintCallable, Category = "Touch Input")
	void OnTouchInteract();

	// Mobile platform detection
	UFUNCTION(BlueprintCallable, Category = "Platform")
	bool IsMobilePlatform() const;

private:
	AOdysseyCharacter* OdysseyCharacter;
};