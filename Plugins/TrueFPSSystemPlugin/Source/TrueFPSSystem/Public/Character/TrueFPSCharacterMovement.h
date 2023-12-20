// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TrueFPSCharacterMovement.generated.h"

/**
 * 
 */
UCLASS()
class TRUEFPSSYSTEM_API UTrueFPSCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	UTrueFPSCharacterMovement(const FObjectInitializer& ObjectInitializer);
	
	virtual float GetMaxSpeed() const override;

};
