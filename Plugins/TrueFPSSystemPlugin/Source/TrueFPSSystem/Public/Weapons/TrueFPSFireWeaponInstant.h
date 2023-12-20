// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Settings/TrueFPSFireWeaponInstantSettings.h"
#include "Weapons/TrueFPSFireWeaponBase.h"
#include "TrueFPSFireWeaponInstant.generated.h"

USTRUCT(BlueprintType)
struct FInstantHitInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector Origin;

	UPROPERTY()
	float ReticleSpread;

	UPROPERTY()
	int32 RandomSeed;

	FInstantHitInfo()
		: Origin(0)
		, ReticleSpread(0)
		, RandomSeed(0)
	{
	}
};

// A weapon where the damage impact occurs instantly upon firing
UCLASS(Abstract)
class TRUEFPSSYSTEM_API ATrueFPSFireWeaponInstant : public ATrueFPSFireWeaponBase
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Fire Weapon Instant")
	TObjectPtr<UTrueFPSFireWeaponInstantSettings> FireInstantSettings;

	// Replicated State
	
	/** instant hit notify for replication */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Fire Weapon Instant", Transient, ReplicatedUsing=OnRep_HitNotify)
	FInstantHitInfo HitNotify;

public:

	ATrueFPSFireWeaponInstant(const FObjectInitializer& ObjectInitializer);

	/** get current spread */
	float GetCurrentSpread() const;

protected:

	virtual EAmmoType GetAmmoType() const override
	{
		return EAmmoType::EBullet;
	}

	/** current spread from continuous firing */
	float CurrentFiringSpread;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** server notified of hit from client to verify */
	UFUNCTION(reliable, server)
	void ServerNotifyHit(const FHitResult& Impact, FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);

	/** server notified of miss to show trail FX */
	UFUNCTION(unreliable, server)
	void ServerNotifyMiss(FVector_NetQuantizeNormal ShootDir, int32 RandomSeed, float ReticleSpread);

	/** process the instant hit and notify the server if necessary */
	void ProcessInstantHit(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread);

	/** continue processing the instant hit, as if it has been confirmed by the server */
	void ProcessInstantHit_Confirmed(const FHitResult& Impact, const FVector& Origin, const FVector& ShootDir, int32 RandomSeed, float ReticleSpread);

	/** check if weapon should deal damage to actor */
	bool ShouldDealDamage(AActor* TestActor) const;

	/** handle damage */
	void DealDamage(const FHitResult& Impact, const FVector& ShootDir);

	UFUNCTION(BlueprintImplementableEvent)
	void OnDealDamage(const FHitResult& Impact, const FVector& ShootDir);

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() override;

	/** [local + server] update spread on firing */
	virtual void OnBurstFinished() override;


	//////////////////////////////////////////////////////////////////////////
	// Effects replication
	
	UFUNCTION()
	void OnRep_HitNotify();

	/** called in network play to do the cosmetic fx  */
	void SimulateInstantHit(const FVector& Origin, int32 RandomSeed, float ReticleSpread);

	/** spawn effects for impact */
	void SpawnImpactEffects(const FHitResult& Impact);

	/** spawn trail effect */
	void SpawnTrailEffect(const FVector& EndPoint);
	
};
