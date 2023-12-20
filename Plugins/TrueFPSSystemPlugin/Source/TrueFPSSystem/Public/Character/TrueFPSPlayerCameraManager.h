// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "TrueFPSPlayerCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class TRUEFPSSYSTEM_API ATrueFPSPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:

	ATrueFPSPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

	/** normal FOV */
	float NormalFOV;

	/** targeting FOV */
	float TargetingFOV;

protected:
	
	virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;

	bool CustomCameraBehavior(float DeltaTime, FVector& Location, FRotator& Rotation, float& FOV);
};
