// Fill out your copyright notice in the Description page of Project Settings.

#include "Bots/TrueFPSAIController.h"

#include "EngineUtils.h"
#include "TrueFPSSystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Bots/TrueFPSBot.h"
#include "GameFramework/GameStateBase.h"
#include "Online/TrueFPSPlayerState.h"
#include "Weapons/TrueFPSFireWeaponBase.h"
#include "Weapons/TrueFPSWeaponBase.h"

ATrueFPSAIController::ATrueFPSAIController(const FObjectInitializer& ObjectInitializer)
{
	BlackboardComp = ObjectInitializer.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("BlackBoardComp"));
 	
	BrainComponent = BehaviorComp = ObjectInitializer.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("BehaviorComp"));	

	bWantsPlayerState = true;
}

void ATrueFPSAIController::GameHasEnded(AActor* EndGameFocus, bool bIsWinner)
{
	// Stop the behaviour tree/logic
	BehaviorComp->StopTree();

	// Stop any movement we already have
	StopMovement();

	// Cancel the repsawn timer
	GetWorldTimerManager().ClearTimer(TimerHandle_Respawn);

	// Clear any enemy
	SetEnemy(nullptr);

	// Finally stop firing
	ATrueFPSBot* MyBot = Cast<ATrueFPSBot>(GetPawn());
	ATrueFPSWeaponBase* MyWeapon = MyBot ? MyBot->GetWeapon() : nullptr;
	if (MyWeapon == nullptr)
	{
		return;
	}
	MyBot->StopWeaponFire();
}

void ATrueFPSAIController::BeginInactiveState()
{
	Super::BeginInactiveState();

	AGameStateBase const* const GameState = GetWorld()->GetGameState();

	const float MinRespawnDelay = GameState ? GameState->GetPlayerRespawnDelay(this) : 1.0f;

	GetWorldTimerManager().SetTimer(TimerHandle_Respawn, this, &ATrueFPSAIController::Respawn, MinRespawnDelay);
}

void ATrueFPSAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ATrueFPSBot* Bot = Cast<ATrueFPSBot>(InPawn);

	// start behavior
	if (Bot && Bot->BotBehavior)
	{
		if (Bot->BotBehavior->BlackboardAsset)
		{
			BlackboardComp->InitializeBlackboard(*Bot->BotBehavior->BlackboardAsset);
		}

		EnemyKeyID = BlackboardComp->GetKeyID("Enemy");
		NeedAmmoKeyID = BlackboardComp->GetKeyID("NeedAmmo");

		BehaviorComp->StartTree(*(Bot->BotBehavior));
	}
}

void ATrueFPSAIController::OnUnPossess()
{
	Super::OnUnPossess();

	BehaviorComp->StopTree();
}

void ATrueFPSAIController::Respawn()
{
	GetWorld()->GetAuthGameMode()->RestartPlayer(this);
}

void ATrueFPSAIController::CheckAmmo(const ATrueFPSWeaponBase* CurrentWeapon)
{
	if (CurrentWeapon && BlackboardComp)
	{
		if (const ATrueFPSFireWeaponBase* CurrentFireWeapon = Cast<ATrueFPSFireWeaponBase>(CurrentWeapon))
		{
			const int32 Ammo = CurrentFireWeapon->GetCurrentAmmo();
			const int32 MaxAmmo = CurrentFireWeapon->GetMaxAmmo();
			const float Ratio = (float) Ammo / (float) MaxAmmo;

			BlackboardComp->SetValue<UBlackboardKeyType_Bool>(NeedAmmoKeyID, (Ratio <= 0.1f));
		}
	}
}

void ATrueFPSAIController::SetEnemy(APawn* InPawn)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValue<UBlackboardKeyType_Object>(EnemyKeyID, InPawn);
		SetFocus(InPawn);
	}
}

ATrueFPSCharacter* ATrueFPSAIController::GetEnemy() const
{
	if (BlackboardComp)
	{
		return Cast<ATrueFPSCharacter>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(EnemyKeyID));
	}

	return nullptr;
}

void ATrueFPSAIController::ShootEnemy()
{
	ATrueFPSBot* MyBot = Cast<ATrueFPSBot>(GetPawn());
	ATrueFPSWeaponBase* MyWeapon = MyBot ? MyBot->GetWeapon() : nullptr;
	if (MyWeapon == nullptr)
	{
		return;
	}

	bool bCanShoot = false;
	ATrueFPSCharacter* Enemy = GetEnemy();
	if (ATrueFPSFireWeaponBase* MyFireWeaponBase = Cast<ATrueFPSFireWeaponBase>(MyWeapon))
	{
		if ( Enemy && ( Enemy->IsAlive() )&& (MyFireWeaponBase->GetCurrentAmmo() > 0) && ( MyFireWeaponBase->CanFire() == true ) )
		{
			if (LineOfSightTo(Enemy, MyBot->GetActorLocation()))
			{
				bCanShoot = true;
			}
		}
	}

	if (bCanShoot)
	{
		MyBot->StartWeaponFire();
	}
	else
	{
		MyBot->StopWeaponFire();
	}
}

void ATrueFPSAIController::FindClosestEnemy()
{
	APawn* MyBot = GetPawn();
	if (MyBot == nullptr)
	{
		return;
	}

	const FVector MyLoc = MyBot->GetActorLocation();
	float BestDistSq = MAX_FLT;
	ATrueFPSCharacter* BestPawn = nullptr;

	for (ATrueFPSCharacter* TestPawn : TActorRange<ATrueFPSCharacter>(GetWorld()))
	{
		if (TestPawn->IsAlive() && TestPawn->IsEnemyFor(this))
		{
			const float DistSq = (TestPawn->GetActorLocation() - MyLoc).SizeSquared();
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestPawn = TestPawn;
			}
		}
	}

	if (BestPawn)
	{
		SetEnemy(BestPawn);
	}
}

bool ATrueFPSAIController::FindClosestEnemyWithLOS(ATrueFPSCharacter* ExcludeEnemy)
{
	bool bGotEnemy = false;
	APawn* MyBot = GetPawn();
	if (MyBot != nullptr)
	{
		const FVector MyLoc = MyBot->GetActorLocation();
		float BestDistSq = MAX_FLT;
		ATrueFPSCharacter* BestPawn = nullptr;

		for (ATrueFPSCharacter* TestPawn : TActorRange<ATrueFPSCharacter>(GetWorld()))
		{
			if (TestPawn != ExcludeEnemy && TestPawn->IsAlive() && TestPawn->IsEnemyFor(this))
			{
				if (HasWeaponLOSToEnemy(TestPawn, true) == true)
				{
					const float DistSq = (TestPawn->GetActorLocation() - MyLoc).SizeSquared();
					if (DistSq < BestDistSq)
					{
						BestDistSq = DistSq;
						BestPawn = TestPawn;
					}
				}
			}
		}
		if (BestPawn)
		{
			SetEnemy(BestPawn);
			bGotEnemy = true;
		}
	}
	
	return bGotEnemy;
}

bool ATrueFPSAIController::HasWeaponLOSToEnemy(AActor* InEnemyActor, const bool bAnyEnemy) const
{
	ATrueFPSBot* MyBot = Cast<ATrueFPSBot>(GetPawn());

	bool bHasLOS = false;
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(AIWeaponLosTrace), true, GetPawn());

	TraceParams.bReturnPhysicalMaterial = true;	
	FVector StartLocation = MyBot->GetActorLocation();	
	StartLocation.Z += GetPawn()->BaseEyeHeight; //look from eyes
	
	FHitResult Hit(ForceInit);
	const FVector EndLocation = InEnemyActor->GetActorLocation();
	GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, COLLISION_WEAPON, TraceParams);
	if (Hit.bBlockingHit == true)
	{
		// Theres a blocking hit - check if its our enemy actor
		AActor* HitActor = Hit.GetActor();
		if (Hit.GetActor() != nullptr)
		{
			if (HitActor == InEnemyActor)
			{
				bHasLOS = true;
			}
			else if (bAnyEnemy == true)
			{
				// Its not our actor, maybe its still an enemy ?
				ACharacter* HitChar = Cast<ACharacter>(HitActor);
				if (HitChar != nullptr)
				{
					ATrueFPSPlayerState* HitPlayerState = Cast<ATrueFPSPlayerState>(HitChar->GetPlayerState());
					ATrueFPSPlayerState* MyPlayerState = Cast<ATrueFPSPlayerState>(PlayerState);
					
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
	}
	
	return bHasLOS;
}

void ATrueFPSAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// Look toward focus
	FVector FocalPoint = GetFocalPoint();
	if( !FocalPoint.IsZero() && GetPawn())
	{
		FVector Direction = FocalPoint - GetPawn()->GetActorLocation();
		FRotator NewControlRotation = Direction.Rotation();
		
		NewControlRotation.Yaw = FRotator::ClampAxis(NewControlRotation.Yaw);

		SetControlRotation(NewControlRotation);

		APawn* const P = GetPawn();
		if (P && bUpdatePawn)
		{
			P->FaceRotation(NewControlRotation, DeltaTime);
		}
	}
}
