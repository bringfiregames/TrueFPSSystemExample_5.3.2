// Fill out your copyright notice in the Description page of Project Settings.

#include "Bots/BTTask_FindPickup.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Bots/TrueFPSAIController.h"
#include "Bots/TrueFPSBot.h"
#include "Online/TrueFPSGameMode.h"

EBTNodeResult::Type UBTTask_FindPickup::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ATrueFPSAIController* MyController = Cast<ATrueFPSAIController>(OwnerComp.GetAIOwner());
	ATrueFPSBot* MyBot = MyController ? Cast<ATrueFPSBot>(MyController->GetPawn()) : nullptr;
	if (MyBot == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	ATrueFPSGameMode* GameMode = MyBot->GetWorld()->GetAuthGameMode<ATrueFPSGameMode>();
	if (GameMode == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// @todo: Implement Ammo Pickups
	// const FVector MyLoc = MyBot->GetActorLocation();
	// AShooterPickup_Ammo* BestPickup = nullptr;
	// float BestDistSq = MAX_FLT;
	//
	// for (int32 i = 0; i < GameMode->LevelPickups.Num(); ++i)
	// {
	// 	AShooterPickup_Ammo* AmmoPickup = Cast<AShooterPickup_Ammo>(GameMode->LevelPickups[i]);
	// 	if (AmmoPickup && AmmoPickup->IsForWeapon(AShooterWeapon_Instant::StaticClass()) && AmmoPickup->CanBePickedUp(MyBot))
	// 	{
	// 		const float DistSq = (AmmoPickup->GetActorLocation() - MyLoc).SizeSquared();
	// 		if (BestDistSq == -1 || DistSq < BestDistSq)
	// 		{
	// 			BestDistSq = DistSq;
	// 			BestPickup = AmmoPickup;
	// 		}
	// 	}
	// }
	//
	// if (BestPickup)
	// {
	// 	OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), BestPickup->GetActorLocation());
	// 	return EBTNodeResult::Succeeded;
	// }

	return EBTNodeResult::Failed;
}
