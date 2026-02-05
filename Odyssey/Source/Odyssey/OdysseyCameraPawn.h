#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "OdysseyCameraPawn.generated.h"

UCLASS()
class ODYSSEY_API AOdysseyCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AOdysseyCameraPawn();

protected:
	// Camera components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComponent;

	// Third-Person Isometric camera settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera")
	float CameraDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera")
	float CameraHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera")
	float IsometricPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera")
	float IsometricYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera")
	float OrthoWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Third Person Camera")
	bool bUseOrthographicProjection;

	// Chase camera behavior
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase Camera")
	AActor* FollowTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase Camera")
	FVector FollowOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase Camera")
	float FollowSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase Camera")
	float RotationSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase Camera")
	bool bEnableSmoothing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase Camera")
	bool bRotateWithTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chase Camera")
	bool bLookAheadOfTarget;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Camera setup functions
	UFUNCTION(BlueprintCallable, Category = "Third Person Camera")
	void InitializeThirdPersonIsometricCamera();

	UFUNCTION(BlueprintCallable, Category = "Chase Camera")
	void SetFollowTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Chase Camera")
	void UpdateChasePosition(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Chase Camera")
	void UpdateCameraRotation(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Third Person Camera")
	FVector CalculateChasePosition() const;

	UFUNCTION(BlueprintCallable, Category = "Third Person Camera")
	FRotator CalculateChaseRotation() const;

	// Utility functions
	UFUNCTION(BlueprintCallable, Category = "Isometric Camera")
	FVector2D WorldToScreenIsometric(FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Isometric Camera")
	FVector ScreenToWorldIsometric(FVector2D ScreenLocation, float WorldZ = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Isometric Camera")
	UCameraComponent* GetIsometricCamera() const { return CameraComponent; }
};