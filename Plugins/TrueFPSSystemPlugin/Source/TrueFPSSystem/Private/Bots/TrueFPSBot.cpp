// Fill out your copyright notice in the Description page of Project Settings.

#include "Bots/TrueFPSBot.h"

#include "Bots/TrueFPSAIController.h"

ATrueFPSBot::ATrueFPSBot(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AIControllerClass = ATrueFPSAIController::StaticClass();
	
	// UpdatePawnMeshes(); @todo: Implement Pawn Mesh Colors

	bUseControllerRotationYaw = true;
}

void ATrueFPSBot::FaceRotation(FRotator NewRotation, float DeltaTime)
{
	FRotator CurrentRotation = FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 8.0f);

	Super::FaceRotation(CurrentRotation, DeltaTime);
}
