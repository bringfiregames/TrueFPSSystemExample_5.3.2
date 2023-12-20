// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TrueFPSPlayerCameraManager.h"

#include "Character/TrueFPSCharacterInterface.h"
#include "GameFramework/Character.h"

ATrueFPSPlayerCameraManager::ATrueFPSPlayerCameraManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NormalFOV = 90.0f;
	TargetingFOV = 60.0f;
	ViewPitchMin = -87.0f;
	ViewPitchMax = 87.0f;
	bAlwaysApplyModifiers = true;
}

void ATrueFPSPlayerCameraManager::UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime)
{
	// Partially taken from base class

	if (OutVT.Target)
	{
		FVector OutLocation;
		FRotator OutRotation;
		float OutFOV;

		if (OutVT.Target->IsA<ACharacter>())
		{
			if (CustomCameraBehavior(DeltaTime, OutLocation, OutRotation, OutFOV))
			{
				OutVT.POV.Location = OutLocation;
				OutVT.POV.Rotation = OutRotation;
				OutVT.POV.FOV = OutFOV;
			}
			else
			{
				OutVT.Target->CalcCamera(DeltaTime, OutVT.POV);
			}
		}
		else
		{
			OutVT.Target->CalcCamera(DeltaTime, OutVT.POV);
		}
	}
}

bool ATrueFPSPlayerCameraManager::CustomCameraBehavior(float DeltaTime, FVector& Location, FRotator& Rotation, float& FOV)
{
	const ACharacter* MyCharacter = PCOwner->GetPawn<ACharacter>();

	if (!MyCharacter) return false;
	if (!MyCharacter->Implements<UTrueFPSCharacterInterface>()) return false;

	const FVector& FPTarget = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetFirstPersonCameraTarget(MyCharacter);
	const FRotator& FPTargetRotation = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetFirstPersonCameraTargetRotation(MyCharacter);

	// FOV changing
	const float TargetFOV = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_IsAiming(MyCharacter) ? TargetingFOV : NormalFOV;

	FTransform TargetTransform(FPTargetRotation, FPTarget, FVector::OneVector);

	Location = TargetTransform.GetLocation();
	Rotation = TargetTransform.Rotator();
	FOV = FMath::FInterpTo(GetFOVAngle(), TargetFOV, DeltaTime, 10.f);

	return true;
}
