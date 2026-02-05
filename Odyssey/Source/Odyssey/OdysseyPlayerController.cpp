#include "OdysseyPlayerController.h"
#include "OdysseyCharacter.h"
#include "OdysseyTouchInterface.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AOdysseyPlayerController::AOdysseyPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = true;
	bEnableTouchEvents = true;
	bEnableMouseOverEvents = true;

	// Create touch interface component
	TouchInterface = CreateDefaultSubobject<UOdysseyTouchInterface>(TEXT("TouchInterface"));
}

void AOdysseyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Get reference to controlled character
	OdysseyCharacter = Cast<AOdysseyCharacter>(GetPawn());

	// Setup Enhanced Input
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (OdysseyMappingContext)
		{
			Subsystem->AddMappingContext(OdysseyMappingContext, 0);
		}
	}

	// Configure touch controls for mobile
	if (TouchInterface && IsMobilePlatform())
	{
		TouchInterface->SetTouchControlsVisible(true);
	}
}

void AOdysseyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Bind movement action
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AOdysseyPlayerController::Move);
		}

		// Bind interact action
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AOdysseyPlayerController::Interact);
		}

		// Bind touch action
		if (TouchAction)
		{
			EnhancedInputComponent->BindAction(TouchAction, ETriggerEvent::Started, this, &AOdysseyPlayerController::Touch);
		}
	}
}

void AOdysseyPlayerController::Move(const FInputActionValue& Value)
{
	if (OdysseyCharacter)
	{
		const FVector2D MovementVector = Value.Get<FVector2D>();

		// Transform 2D input to 3D movement for isometric view
		// Convert to diagonal movement for isometric camera
		const FVector ForwardDirection = FVector(1.0f, 1.0f, 0.0f).GetSafeNormal();
		const FVector RightDirection = FVector(1.0f, -1.0f, 0.0f).GetSafeNormal();

		OdysseyCharacter->AddMovementInput(ForwardDirection, MovementVector.Y);
		OdysseyCharacter->AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AOdysseyPlayerController::Interact(const FInputActionValue& Value)
{
	if (OdysseyCharacter)
	{
		// Trigger interaction on the character
		OdysseyCharacter->TryInteract();
	}
}

void AOdysseyPlayerController::Touch(const FInputActionValue& Value)
{
	const FVector2D TouchLocation = Value.Get<FVector2D>();
	HandleTouchInput(TouchLocation);
}

void AOdysseyPlayerController::HandleTouchInput(FVector2D TouchLocation)
{
	// Convert screen touch to world location for movement or interaction
	FVector WorldLocation, WorldDirection;
	if (DeprojectScreenPositionToWorld(TouchLocation.X, TouchLocation.Y, WorldLocation, WorldDirection))
	{
		// Perform a line trace to find what was touched
		FHitResult HitResult;
		FVector Start = WorldLocation;
		FVector End = Start + (WorldDirection * 10000.0f); // Long trace distance

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility))
		{
			if (HitResult.GetActor())
			{
				// Check if the touched object is interactable
				if (HitResult.GetActor()->ActorHasTag(TEXT("Interactable")))
				{
					// Trigger interaction
					if (OdysseyCharacter)
					{
						OdysseyCharacter->TryInteract();
					}
				}
			}
		}
	}
}

FVector2D AOdysseyPlayerController::ConvertTouchToWorldDirection(FVector2D TouchLocation)
{
	FVector WorldLocation, WorldDirection;
	if (DeprojectScreenPositionToWorld(TouchLocation.X, TouchLocation.Y, WorldLocation, WorldDirection))
	{
		// Project onto the XY plane and normalize for isometric movement
		FVector2D Direction = FVector2D(WorldDirection.X, WorldDirection.Y).GetSafeNormal();
		return Direction;
	}
	return FVector2D::ZeroVector;
}

void AOdysseyPlayerController::OnTouchMovementInput(FVector2D MovementInput)
{
	if (OdysseyCharacter)
	{
		// Transform 2D touch input to 3D movement for isometric view
		const FVector ForwardDirection = FVector(1.0f, 1.0f, 0.0f).GetSafeNormal();
		const FVector RightDirection = FVector(1.0f, -1.0f, 0.0f).GetSafeNormal();

		OdysseyCharacter->AddMovementInput(ForwardDirection, MovementInput.Y);
		OdysseyCharacter->AddMovementInput(RightDirection, MovementInput.X);
	}
}

void AOdysseyPlayerController::OnTouchInteract()
{
	if (OdysseyCharacter)
	{
		OdysseyCharacter->TryInteract();
	}
}

bool AOdysseyPlayerController::IsMobilePlatform() const
{
	FString PlatformName = UGameplayStatics::GetPlatformName();
	return (PlatformName == TEXT("Android") || PlatformName == TEXT("IOS"));
}