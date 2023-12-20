// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TrueFPSWeaponBase.h"
#include "TrueFPSMeleeWeaponBase.generated.h"

USTRUCT()
struct FMeleeHitInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector Origin;

	FMeleeHitInfo()
		: Origin(0)
	{
	}
};

USTRUCT(DisplayName = "Melee Weapon")
struct FMeleeWeaponData
{
	GENERATED_USTRUCT_BODY()
	
	/** name of bone/socket for weapon tracing */
	UPROPERTY(EditDefaultsOnly)
	FName WeaponTracePoint;
	
	/** radius for weapon tracing */
	UPROPERTY(EditDefaultsOnly)
	float WeaponTraceRadius;

	/** damage amount */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	int32 HitDamage;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	TSubclassOf<UDamageType> DamageType;

	/** hit verification: scale for bounding box of hit actor */
	UPROPERTY(EditDefaultsOnly, Category=HitVerification)
	float ClientSideHitLeeway;

	/** defaults */
	FMeleeWeaponData()
	{
		WeaponTracePoint = FName("meleedamage");
		WeaponTraceRadius = 5.f;
		HitDamage = 10;
		DamageType = UDamageType::StaticClass();
		ClientSideHitLeeway = 200.0f;
	}
};

UCLASS(Abstract)
class TRUEFPSSYSTEM_API ATrueFPSMeleeWeaponBase : public ATrueFPSWeaponBase
{
	GENERATED_BODY()

public:

	ATrueFPSMeleeWeaponBase(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;

	//////////////////////////////////////////////////////////////////////////
	// Input

	/** used by TrueFPSMeleeWeaponTraceNotifyState for starting weapon trace */
	FORCEINLINE void StartWeaponTrace() { bWeaponTracing = true; }
	
	/** used by TrueFPSMeleeWeaponTraceNotifyState for stopping weapon trace */
	FORCEINLINE void StopWeaponTrace()
	{
		bWeaponTracing = false;
		AttackedActors.Empty();
	}

protected:

	virtual EAmmoType GetAmmoType() const override
	{
		return EAmmoType::ENone;
	}

	/** weapon config */
	UPROPERTY(EditDefaultsOnly, Category="Config")
	FMeleeWeaponData MeleeWeaponConfig;

	/** impact effects */
	UPROPERTY(EditDefaultsOnly, Category="Config|Effects")
	TSubclassOf<class ATrueFPSImpactEffect> ImpactTemplate;

	/** is weapon tracing happening? */
	UPROPERTY(Transient)
	uint32 bWeaponTracing : 1;
	
	/** instant hit notify for replication */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_HitNotify)
	FMeleeHitInfo HitNotify;
	
	/** instant hit notify for replication */
	UPROPERTY(Transient, Replicated)
	TArray<class AActor*> AttackedActors;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage
	
	/** find hit */
	FHitResult WeaponTrace(const FVector& TraceFrom) const;

	/** server notified of hit from client to verify */
	UFUNCTION(reliable, server)
	void ServerNotifyHit(const FHitResult& Impact);

	/** process the instant hit and notify the server if necessary */
	void ProcessInstantHit(const FHitResult& Impact);

	/** continue processing the instant hit, as if it has been confirmed by the server */
	void ProcessInstantHit_Confirmed(const FHitResult& Impact);

	/** check if weapon should deal damage to actor */
	bool ShouldDealDamage(AActor* TestActor) const;

	/** handle damage */
	void DealDamage(const FHitResult& Impact);

	UFUNCTION(BlueprintImplementableEvent)
	void OnDealDamage(const FHitResult& Impact);

	FVector GetWeaponTraceLocation() const;

	void HandleAttackTick();

	virtual FVector GetCameraDamageStartLocation(const FVector& AimDir) const override;


	//////////////////////////////////////////////////////////////////////////
	// Effects replication
	
	UFUNCTION()
	void OnRep_HitNotify();

	/** called in network play to do the cosmetic fx  */
	void SimulateInstantHit(const FVector& Origin);

	/** spawn effects for impact */
	void SpawnImpactEffects(const FHitResult& Impact);
	
};
