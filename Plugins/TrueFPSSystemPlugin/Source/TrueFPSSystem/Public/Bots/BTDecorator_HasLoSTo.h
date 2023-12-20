// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIModule/Classes/BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasLoSTo.generated.h"

/**
 * 
 */
UCLASS()
class TRUEFPSSYSTEM_API UBTDecorator_HasLoSTo : public UBTDecorator
{
	GENERATED_BODY()
	
public:

UBTDecorator_HasLoSTo(const FObjectInitializer& ObjectInitializer);

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

protected:
	
	UPROPERTY(EditAnywhere, Category = Condition)
	struct FBlackboardKeySelector EnemyKey;

private:

	bool LOSTrace(AActor* InActor, AActor* InEnemyActor, const FVector& EndLocation) const;	
	
};
