#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "OdysseyCameraActor.generated.h"

UCLASS()
class ODYSSEY_API AOdysseyCameraActor : public ACameraActor
{
	GENERATED_BODY()

public:
	AOdysseyCameraActor();

protected:
	// Camera settings for isometric view
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Isometric Camera")
	float CameraHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Isometric Camera")
	float CameraDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Isometric Camera")
	float CameraPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Isometric Camera")
	float CameraYaw;

	// Camera following
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Following")
	AActor* FollowTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Following")
	FVector FollowOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Following")
	float FollowSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Following")
	bool bSmoothFollowing;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Isometric Camera")
	void SetupIsometricView();

	UFUNCTION(BlueprintCallable, Category = "Camera Following")
	void SetFollowTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Camera Following")
	void UpdateCameraPosition(float DeltaTime);

	// Utility functions for isometric projection
	UFUNCTION(BlueprintCallable, Category = "Isometric Camera")
	FVector WorldToIsometric(FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Isometric Camera")
	FVector IsometricToWorld(FVector IsometricLocation);
};