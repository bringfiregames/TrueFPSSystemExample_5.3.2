// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/TrueFPSCharacter.h"
#include "TrueFPSBot.generated.h"

UCLASS()
class TRUEFPSSYSTEM_API ATrueFPSBot : public ATrueFPSCharacter
{
	GENERATED_BODY()

public:
	
	ATrueFPSBot(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, Category=Behavior)
	class UBehaviorTree* BotBehavior;

	virtual void FaceRotation(FRotator NewRotation, float DeltaTime = 0.f) override;
	
};
