#include "OdysseyCameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AOdysseyCameraActor::AOdysseyCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set default isometric values
	CameraHeight = 2000.0f;
	CameraDistance = 2000.0f;
	CameraPitch = -45.0f;  // 45 degrees down
	CameraYaw = -45.0f;    // 45 degrees offset for isometric view

	// Follow settings
	FollowOffset = FVector(0, 0, 0);
	FollowSpeed = 5.0f;
	bSmoothFollowing = true;
	FollowTarget = nullptr;

	// Set up the camera component
	if (GetCameraComponent())
	{
		GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
		GetCameraComponent()->SetOrthoWidth(2000.0f); // Adjust based on desired view size
	}
}

void AOdysseyCameraActor::BeginPlay()
{
	Super::BeginPlay();

	SetupIsometricView();

	// Find the player character to follow
	if (!FollowTarget)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (PlayerPawn)
		{
			SetFollowTarget(PlayerPawn);
		}
	}
}

void AOdysseyCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FollowTarget)
	{
		UpdateCameraPosition(DeltaTime);
	}
}

void AOdysseyCameraActor::SetupIsometricView()
{
	// Set the camera rotation for isometric view
	FRotator IsometricRotation = FRotator(CameraPitch, CameraYaw, 0.0f);
	SetActorRotation(IsometricRotation);

	// Set orthographic projection if not already set
	if (GetCameraComponent())
	{
		GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Orthographic);
		GetCameraComponent()->SetOrthoWidth(2000.0f);
	}

	UE_LOG(LogTemp, Warning, TEXT("Isometric camera setup complete: Pitch=%f, Yaw=%f"), CameraPitch, CameraYaw);
}

void AOdysseyCameraActor::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;

	if (FollowTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Camera now following: %s"), *FollowTarget->GetName());
	}
}

void AOdysseyCameraActor::UpdateCameraPosition(float DeltaTime)
{
	if (!FollowTarget)
		return;

	// Calculate desired position based on target and offset
	FVector TargetLocation = FollowTarget->GetActorLocation();

	// Apply isometric offset
	FVector DesiredLocation = TargetLocation + FollowOffset;

	// Adjust for isometric view distance and height
	FVector CameraOffset = FVector(-CameraDistance * 0.707f, -CameraDistance * 0.707f, CameraHeight);
	DesiredLocation += CameraOffset;

	if (bSmoothFollowing)
	{
		// Smooth interpolation to desired position
		FVector CurrentLocation = GetActorLocation();
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, DesiredLocation, DeltaTime, FollowSpeed);
		SetActorLocation(NewLocation);
	}
	else
	{
		// Direct positioning
		SetActorLocation(DesiredLocation);
	}
}

FVector AOdysseyCameraActor::WorldToIsometric(FVector WorldLocation)
{
	// Convert world coordinates to isometric screen space
	// This is a simplified transformation for 2.5D isometric view
	float X = (WorldLocation.X - WorldLocation.Y) * 0.707f;  // cos(45°)
	float Y = (WorldLocation.X + WorldLocation.Y) * 0.354f;  // sin(45°) * 0.5 for depth
	float Z = WorldLocation.Z;

	return FVector(X, Y, Z);
}

FVector AOdysseyCameraActor::IsometricToWorld(FVector IsometricLocation)
{
	// Convert isometric screen coordinates back to world space
	float WorldX = (IsometricLocation.X * 0.707f) + (IsometricLocation.Y * 1.414f);
	float WorldY = (IsometricLocation.Y * 1.414f) - (IsometricLocation.X * 0.707f);
	float WorldZ = IsometricLocation.Z;

	return FVector(WorldX, WorldY, WorldZ);
}