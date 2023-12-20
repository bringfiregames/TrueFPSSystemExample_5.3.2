// Fill out your copyright notice in the Description page of Project Settings.

#include "Bots/BTDecorator_HasLoSTo.h"

#include "TrueFPSSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Bots/TrueFPSAIController.h"
#include "Bots/TrueFPSBot.h"
#include "Online/TrueFPSPlayerState.h"

UBTDecorator_HasLoSTo::UBTDecorator_HasLoSTo(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "Has LoS To";
	// accept only actors and vectors	
	EnemyKey.AddObjectFilter(this, *NodeName, AActor::StaticClass());
	EnemyKey.AddVectorFilter(this, *NodeName);
}

/*
bool UBTDecorator_HasLoSTo::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp == nullptr)
	{
		return false;
	}

	FVector PointA = FVector::ZeroVector;
	FVector PointB = FVector::ZeroVector;
	const bool bHasPointA = BlackboardComp->GetLocationFromEntry(BlackboardKeyA.GetSelectedKeyID(), PointA);
	const bool bHasPointB = BlackboardComp->GetLocationFromEntry(BlackboardKeyB.GetSelectedKeyID(), PointB);

	bool bHasPath = false;

	const UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(OwnerComp->GetWorld());
	if (NavSys && bHasPointA && bHasPointB)
	{
		const AAIController* AIOwner = Cast<AAIController>(OwnerComp->GetOwner());
		const ANavigationData* NavData = AIOwner && AIOwner->NavComponent ? AIOwner->NavComponent->GetNavData() : nullptr;
		TSharedPtr<const FNavigationQueryFilter> QueryFilter = UNavigationQueryFilter::GetQueryFilter(NavData, FilterClass);

		if (PathQueryType == EPathExistanceQueryType::NavmeshRaycast2D)
		{
#if WITH_RECAST
			const ARecastNavMesh* RecastNavMesh = Cast<const ARecastNavMesh>(NavData);
			bHasPath = RecastNavMesh && RecastNavMesh->IsSegmentOnNavmesh(PointA, PointB, QueryFilter);
#endif
		}
		else
		{
			EPathFindingMode::Type TestMode = (PathQueryType == EPathExistanceQueryType::HierarchicalQuery) ? EPathFindingMode::Hierarchical : EPathFindingMode::Regular;
			bHasPath = NavSys->TestPathSync(FPathFindingQuery(AIOwner, NavData, PointA, PointB, QueryFilter), TestMode);
		}
	}

	return bHasPath;
}*/

bool UBTDecorator_HasLoSTo::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	AAIController* MyController = OwnerComp.GetAIOwner();
	bool HasLOS = false;

	if (MyController && MyBlackboard)
	{
		auto MyID = MyBlackboard->GetKeyID(EnemyKey.SelectedKeyName);
		auto TargetKeyType = MyBlackboard->GetKeyType(MyID);
		
		FVector TargetLocation;
		bool bGotTarget = false;
		AActor* EnemyActor = nullptr;
		if (TargetKeyType == UBlackboardKeyType_Object::StaticClass())
		{
			UObject* KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Object>(MyID);
			EnemyActor = Cast<AActor>(KeyValue);
			if (EnemyActor)
			{
				TargetLocation = EnemyActor->GetActorLocation();
				bGotTarget = true;
			}
		}
		else if (TargetKeyType == UBlackboardKeyType_Vector::StaticClass())
		{
			TargetLocation = MyBlackboard->GetValue<UBlackboardKeyType_Vector>(MyID);
			bGotTarget = true;
		}

		if (bGotTarget== true )
		{
			if (LOSTrace(OwnerComp.GetOwner(), EnemyActor, TargetLocation) == true)
			{
				HasLOS = true;
			}
		}			
	}

	return HasLOS;
}

bool UBTDecorator_HasLoSTo::LOSTrace(AActor* InActor, AActor* InEnemyActor, const FVector& EndLocation) const
{
	ATrueFPSAIController* MyController = Cast<ATrueFPSAIController>(InActor);
	ATrueFPSBot* MyBot = MyController ? Cast<ATrueFPSBot>(MyController->GetPawn()) : nullptr; 

	bool bHasLOS = false;
	{
		if (MyBot != nullptr)
		{
			// Perform trace to retrieve hit info
			FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(AILosTrace), true, InActor);
			
			TraceParams.bReturnPhysicalMaterial = true;
			TraceParams.AddIgnoredActor(MyBot);
			const FVector StartLocation = MyBot->GetActorLocation();
			FHitResult Hit(ForceInit);
			GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, COLLISION_WEAPON, TraceParams);
			if (Hit.bBlockingHit == true)
			{
				// We hit something. If we have an actor supplied, just check if the hit actor is an enemy. If it is consider that 'has LOS'
				AActor* HitActor = Hit.GetActor();
				if (Hit.GetActor() != nullptr)
				{
					// If the hit is our target actor consider it LOS
					if (HitActor == InActor)
					{
						bHasLOS = true;
					}
					else
					{
						// Check the team of us against the team of the actor we hit if we are able. If they dont match good to go.
						ACharacter* HitChar = Cast<ACharacter>(HitActor);
						if ( (HitChar != nullptr)
							&& (MyController->PlayerState != nullptr) && (HitChar->GetPlayerState() != nullptr))
						{
							ATrueFPSPlayerState* HitPlayerState = Cast<ATrueFPSPlayerState>(HitChar->GetPlayerState());
							ATrueFPSPlayerState* MyPlayerState = Cast<ATrueFPSPlayerState>(MyController->PlayerState);
							if ((HitPlayerState != nullptr) && (MyPlayerState != nullptr))
							{
								if (HitPlayerState->GetTeamNum() != MyPlayerState->GetTeamNum())
								{
									bHasLOS = true;
								}
							}
						}
					}
				}
				else //we didnt hit an actor
				{
					if (InEnemyActor == nullptr)
					{
						// We were not given an actor - so check of the distance between what we hit and the target. If what we hit is further away than the target we should be able to hit our target.
						FVector HitDelta = Hit.ImpactPoint - StartLocation;
						FVector TargetDelta = EndLocation - StartLocation;
						if (TargetDelta.SizeSquared() < HitDelta.SizeSquared())
						{
							bHasLOS = true;
						}
					}
				}
			}
		}
	}

	return bHasLOS;
}

// 
// FString UBTDecorator_HasLoSTo::GetStaticDescription() const
// {
// 	FString KeyDesc("invalid");
// 	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
// 		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
// 	{
// 		KeyDesc = BlackboardKey.SelectedKeyName.ToString();
// 	}
// 
// 	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *KeyDesc);
// }
// 
// void UBTDecorator_HasLoSTo::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
// {
// 	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);
// 
// 	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
// 
// 	if (BlackboardComp)
// 	{
// 		FString KeyValue = BlackboardComp->DescribeKeyValue(BlackboardKey.GetSelectedKeyID(), EBlackboardDescription::OnlyValue);
// 		Values.Add(FString::Printf(TEXT("LOS target: %s"), *KeyValue));
// 	}
// }
