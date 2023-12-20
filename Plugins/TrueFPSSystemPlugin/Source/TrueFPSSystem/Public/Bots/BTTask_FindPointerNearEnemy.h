// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIModule/Classes/BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindPointerNearEnemy.generated.h"

/**
 * 
 */
UCLASS()
class TRUEFPSSYSTEM_API UBTTask_FindPointerNearEnemy : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};
