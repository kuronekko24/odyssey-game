#include "OdysseyCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

AOdysseyCameraPawn::AOdysseyCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root scene component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;

	// Create spring arm for camera positioning
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;

	// Create camera component
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArm);

	// Set default third-person isometric values
	CameraDistance = 800.0f;  // Closer for third-person view
	CameraHeight = 400.0f;    // Height above the ship
	IsometricPitch = -35.0f;  // Looking down at the ship
	IsometricYaw = 0.0f;      // Start aligned with ship forward
	OrthoWidth = 1920.0f;
	bUseOrthographicProjection = true;

	// Chase camera settings
	FollowOffset = FVector(-600.0f, 0.0f, 300.0f); // Behind and above the ship
	FollowSpeed = 8.0f;       // Faster following for chase cam
	RotationSpeed = 3.0f;     // How quickly camera rotates to follow ship
	bEnableSmoothing = true;
	bRotateWithTarget = true; // Camera rotates to match ship direction
	bLookAheadOfTarget = true; // Camera looks where ship is going
	FollowTarget = nullptr;
}

void AOdysseyCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	InitializeThirdPersonIsometricCamera();

	// Auto-find player to follow if not set
	if (!FollowTarget)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (PlayerPawn && PlayerPawn != this)
		{
			SetFollowTarget(PlayerPawn);
		}
	}
}

void AOdysseyCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FollowTarget)
	{
		UpdateChasePosition(DeltaTime);
		if (bRotateWithTarget)
		{
			UpdateCameraRotation(DeltaTime);
		}
	}
}

void AOdysseyCameraPawn::InitializeThirdPersonIsometricCamera()
{
	// Set up spring arm for third-person chase camera
	SpringArm->TargetArmLength = CameraDistance;
	SpringArm->SetRelativeRotation(FRotator(IsometricPitch, IsometricYaw, 0.0f));

	// Configure camera projection
	if (bUseOrthographicProjection)
	{
		CameraComponent->SetProjectionMode(ECameraProjectionMode::Orthographic);
		CameraComponent->SetOrthoWidth(OrthoWidth);
	}
	else
	{
		CameraComponent->SetProjectionMode(ECameraProjectionMode::Perspective);
		CameraComponent->SetFieldOfView(75.0f);
	}

	CameraComponent->SetAspectRatio(16.0f / 9.0f); // Standard mobile aspect ratio

	// Disable camera collision for smooth following
	SpringArm->bDoCollisionTest = false;
	SpringArm->bUsePawnControlRotation = false;

	UE_LOG(LogTemp, Warning, TEXT("Third-person isometric camera initialized: Distance=%f, Pitch=%f, Yaw=%f"),
		CameraDistance, IsometricPitch, IsometricYaw);
}

void AOdysseyCameraPawn::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;

	if (FollowTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Camera pawn now following: %s"), *FollowTarget->GetName());

		// Immediately snap to target position if not smoothing
		if (!bEnableSmoothing)
		{
			FVector TargetLocation = FollowTarget->GetActorLocation() + FollowOffset;
			SetActorLocation(TargetLocation);
		}
	}
}

void AOdysseyCameraPawn::UpdateChasePosition(float DeltaTime)
{
	if (!FollowTarget)
		return;

	FVector DesiredLocation = CalculateChasePosition();
	FVector CurrentLocation = GetActorLocation();

	if (bEnableSmoothing)
	{
		// Smooth interpolation for chase camera
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, DesiredLocation, DeltaTime, FollowSpeed);
		SetActorLocation(NewLocation);
	}
	else
	{
		// Direct positioning
		SetActorLocation(DesiredLocation);
	}
}

void AOdysseyCameraPawn::UpdateCameraRotation(float DeltaTime)
{
	if (!FollowTarget)
		return;

	FRotator DesiredRotation = CalculateChaseRotation();
	FRotator CurrentRotation = GetActorRotation();

	if (bEnableSmoothing)
	{
		// Smooth rotation interpolation
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, RotationSpeed);
		SetActorRotation(NewRotation);
	}
	else
	{
		// Direct rotation
		SetActorRotation(DesiredRotation);
	}
}

FVector AOdysseyCameraPawn::CalculateChasePosition() const
{
	if (!FollowTarget)
		return GetActorLocation();

	FVector TargetLocation = FollowTarget->GetActorLocation();
	FRotator TargetRotation = FollowTarget->GetActorRotation();

	// Calculate position behind and above the target
	FVector ForwardVector = TargetRotation.Vector();
	FVector RightVector = FRotationMatrix(TargetRotation).GetUnitAxis(EAxis::Y);
	FVector UpVector = FVector::UpVector;

	// Position camera behind the ship
	FVector ChasePosition = TargetLocation;
	ChasePosition -= ForwardVector * CameraDistance;  // Behind the ship
	ChasePosition += UpVector * CameraHeight;         // Above the ship
	ChasePosition += RightVector * FollowOffset.Y;    // Side offset if any

	return ChasePosition;
}

FRotator AOdysseyCameraPawn::CalculateChaseRotation() const
{
	if (!FollowTarget)
		return GetActorRotation();

	FVector TargetLocation = FollowTarget->GetActorLocation();
	FVector CameraLocation = GetActorLocation();

	FVector LookDirection;
	if (bLookAheadOfTarget)
	{
		// Look ahead of the ship in its movement direction
		FVector TargetForward = FollowTarget->GetActorRotation().Vector();
		FVector LookAheadPoint = TargetLocation + (TargetForward * 300.0f);
		LookDirection = (LookAheadPoint - CameraLocation).GetSafeNormal();
	}
	else
	{
		// Look directly at the ship
		LookDirection = (TargetLocation - CameraLocation).GetSafeNormal();
	}

	// Calculate rotation to look at target with isometric angle
	FRotator LookRotation = FRotationMatrix::MakeFromX(LookDirection).Rotator();

	// Apply isometric pitch offset
	LookRotation.Pitch = IsometricPitch;

	return LookRotation;
}

FVector2D AOdysseyCameraPawn::WorldToScreenIsometric(FVector WorldLocation)
{
	// Convert world coordinates to isometric 2D coordinates
	// Using standard isometric transformation matrix

	float IsoX = (WorldLocation.X - WorldLocation.Y);
	float IsoY = ((WorldLocation.X + WorldLocation.Y) * 0.5f) - WorldLocation.Z;

	return FVector2D(IsoX, IsoY);
}

FVector AOdysseyCameraPawn::ScreenToWorldIsometric(FVector2D ScreenLocation, float WorldZ)
{
	// Convert isometric 2D coordinates back to world coordinates
	// Inverse of the isometric transformation

	float WorldX = (ScreenLocation.X + (2.0f * ScreenLocation.Y) + (2.0f * WorldZ)) * 0.5f;
	float WorldY = ((2.0f * ScreenLocation.Y) + (2.0f * WorldZ) - ScreenLocation.X) * 0.5f;

	return FVector(WorldX, WorldY, WorldZ);
}