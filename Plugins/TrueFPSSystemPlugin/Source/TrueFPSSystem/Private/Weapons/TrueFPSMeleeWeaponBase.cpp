// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/TrueFPSMeleeWeaponBase.h"

#include "TrueFPSSystem.h"
#include "Bots/TrueFPSAIController.h"
#include "Character/TrueFPSPlayerController.h"
#include "Effects/TrueFPSImpactEffect.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ATrueFPSMeleeWeaponBase::ATrueFPSMeleeWeaponBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bWeaponTracing = false;
}

void ATrueFPSMeleeWeaponBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	HandleAttackTick();
}

void ATrueFPSMeleeWeaponBase::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME_CONDITION( ThisClass, HitNotify, COND_SkipOwner );
	DOREPLIFETIME_CONDITION( ThisClass, AttackedActors, COND_OwnerOnly );
}

FHitResult ATrueFPSMeleeWeaponBase::WeaponTrace(const FVector& TraceFrom) const
{
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(WeaponTrace), true, GetInstigator());
	TraceParams.bReturnPhysicalMaterial = true;
	
	FHitResult Hit(ForceInit);
	GetWorld()->SweepSingleByChannel(Hit, TraceFrom, TraceFrom, FQuat::Identity,  COLLISION_WEAPON, FCollisionShape::MakeSphere(MeleeWeaponConfig.WeaponTraceRadius), TraceParams);

	return Hit;
}

void ATrueFPSMeleeWeaponBase::ServerNotifyHit_Implementation(const FHitResult& Impact)
{
	// if we have an instigator, calculate dot between the view and the shot
	if (GetInstigator() && ((Impact.GetActor() && !AttackedActors.Contains(Impact.GetActor())) || Impact.bBlockingHit))
	{
		if (State.CurrentState != EWeaponState::Idle)
		{
			if (Impact.GetActor() == nullptr)
			{
				if (Impact.bBlockingHit)
				{
					ProcessInstantHit_Confirmed(Impact);
				}
			}
			// assume it told the truth about static things because the don't move and the hit 
			// usually doesn't have significant gameplay implications
				
			// Get the component bounding box
			const FBox HitBox = Impact.GetActor()->GetComponentsBoundingBox();

			// calculate the box extent, and increase by a leeway
			FVector BoxExtent = 0.5 * (HitBox.Max - HitBox.Min);
			BoxExtent *= MeleeWeaponConfig.ClientSideHitLeeway;

			// avoid precision errors with really thin objects
			BoxExtent.X = FMath::Max(20.0f, BoxExtent.X);
			BoxExtent.Y = FMath::Max(20.0f, BoxExtent.Y);
			BoxExtent.Z = FMath::Max(20.0f, BoxExtent.Z);

			// Get the box center
			const FVector BoxCenter = (HitBox.Min + HitBox.Max) * 0.5;

			// if we are within client tolerance
			if (FMath::Abs(Impact.Location.Z - BoxCenter.Z) < BoxExtent.Z &&
				FMath::Abs(Impact.Location.X - BoxCenter.X) < BoxExtent.X &&
				FMath::Abs(Impact.Location.Y - BoxCenter.Y) < BoxExtent.Y)
			{
				ProcessInstantHit_Confirmed(Impact);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("%s Rejected client side hit of %s (outside bounding box tolerance)"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("%s Rejected client side hit of %s"), *GetNameSafe(this), *GetNameSafe(Impact.GetActor()));
		}
	}
}

void ATrueFPSMeleeWeaponBase::ProcessInstantHit(const FHitResult& Impact)
{
	if (MyPawn && MyPawn->IsLocallyControlled() && GetNetMode() == NM_Client)
	{
		// if we're a client and we've hit something that is being controlled by the server
		if (Impact.GetActor() && !AttackedActors.Contains(Impact.GetActor()) && Impact.GetActor()->GetRemoteRole() == ROLE_Authority)
		{
			// notify the server of the hit
			ServerNotifyHit(Impact);
		}
		else if (Impact.GetActor() == nullptr)
		{
			if (Impact.bBlockingHit)
			{
				// notify the server of the hit
				ServerNotifyHit(Impact);
			}
		}
	}

	// process a confirmed hit
	ProcessInstantHit_Confirmed(Impact);
}

void ATrueFPSMeleeWeaponBase::ProcessInstantHit_Confirmed(const FHitResult& Impact)
{
	// handle damage
	if (ShouldDealDamage(Impact.GetActor()))
	{
		DealDamage(Impact);
	}

	// play FX on remote clients
	if (GetLocalRole() == ROLE_Authority)
	{
		HitNotify.Origin = Impact.Location;
		AttackedActors.Add(Impact.GetActor());
	}

	// play FX locally
	if (GetNetMode() != NM_DedicatedServer)
	{
		SpawnImpactEffects(Impact);
	}
}

bool ATrueFPSMeleeWeaponBase::ShouldDealDamage(AActor* TestActor) const
{
	// if we're an actor on the server, or the actor's role is authoritative, we should register damage
	if (TestActor)
	{
		if (GetNetMode() != NM_Client ||
			TestActor->GetLocalRole() == ROLE_Authority ||
			TestActor->GetTearOff())
		{
			return true;
		}
	}

	return false;
}

void ATrueFPSMeleeWeaponBase::DealDamage(const FHitResult& Impact)
{
	FPointDamageEvent PointDmg;
	PointDmg.DamageTypeClass = MeleeWeaponConfig.DamageType;
	PointDmg.HitInfo = Impact;
	PointDmg.ShotDirection = FVector(ForceInit);
	PointDmg.Damage = MeleeWeaponConfig.HitDamage;

	Impact.GetActor()->TakeDamage(PointDmg.Damage, PointDmg, MyPawn->Controller, this);

	OnDealDamage(Impact);
}

FVector ATrueFPSMeleeWeaponBase::GetWeaponTraceLocation() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketLocation(MeleeWeaponConfig.WeaponTracePoint);
}

void ATrueFPSMeleeWeaponBase::HandleAttackTick()
{
	if (!bWeaponTracing) return;
	if (!GetPawnOwner()) return;
	if (!GetPawnOwner()->IsLocallyControlled()) return;

	const FVector Origin = GetWeaponTraceLocation();
	const FHitResult HitResult = WeaponTrace(Origin);

	if (HitResult.bBlockingHit)
	{
		ProcessInstantHit(HitResult);
	}
}

FVector ATrueFPSMeleeWeaponBase::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	ATrueFPSPlayerController* PC = MyPawn ? Cast<ATrueFPSPlayerController>(MyPawn->Controller) : NULL;
	ATrueFPSAIController* AIPC = MyPawn ? Cast<ATrueFPSAIController>(MyPawn->Controller) : NULL;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((GetInstigator()->GetActorLocation() - OutStartTrace) | AimDir);
	}
	else if (AIPC)
	{
		OutStartTrace = GetWeaponTraceLocation();
	}

	return OutStartTrace;
}

void ATrueFPSMeleeWeaponBase::OnRep_HitNotify()
{
	SimulateInstantHit(HitNotify.Origin);
}

void ATrueFPSMeleeWeaponBase::SimulateInstantHit(const FVector& Origin)
{
	const FVector StartTrace = GetWeaponTraceLocation();

	FHitResult Impact = WeaponTrace(StartTrace);
	if (Impact.bBlockingHit)
	{
		SpawnImpactEffects(Impact);
	}
}

void ATrueFPSMeleeWeaponBase::SpawnImpactEffects(const FHitResult& Impact)
{
	if (ImpactTemplate && Impact.bBlockingHit)
	{
		FHitResult UseImpact = Impact;
	
		// trace again to find component lost during replication
		if (!Impact.Component.IsValid())
		{
			const FVector StartTrace = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;
			FHitResult Hit = WeaponTrace(StartTrace);
			UseImpact = Hit;
		}
	
		FTransform const SpawnTransform(Impact.ImpactNormal.Rotation(), Impact.ImpactPoint);
		ATrueFPSImpactEffect* EffectActor = GetWorld()->SpawnActorDeferred<ATrueFPSImpactEffect>(ImpactTemplate, SpawnTransform);
		if (EffectActor)
		{
			EffectActor->SurfaceHit = UseImpact;
			UGameplayStatics::FinishSpawningActor(EffectActor, SpawnTransform);
		}
	}
}
