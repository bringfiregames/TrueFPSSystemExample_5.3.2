// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TrueFPSCharacterMovement.h"

#include "Character/TrueFPSCharacter.h"

UTrueFPSCharacterMovement::UTrueFPSCharacterMovement(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

float UTrueFPSCharacterMovement::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	if (const ATrueFPSCharacter* TrueFPSCharacter = Cast<ATrueFPSCharacter>(PawnOwner))
	{
		if (TrueFPSCharacter->IsAiming())
		{
			MaxSpeed *= TrueFPSCharacter->GetAimingSpeedModifier();
		}
		
		if (TrueFPSCharacter->IsRunning())
		{
			MaxSpeed *= TrueFPSCharacter->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}
