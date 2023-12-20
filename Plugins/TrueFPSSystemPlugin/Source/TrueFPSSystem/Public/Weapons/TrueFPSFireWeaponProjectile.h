// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/TrueFPSFireWeaponBase.h"
#include "Settings/TrueFPSFireWeaponProjectileSettings.h"
#include "TrueFPSFireWeaponProjectile.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class TRUEFPSSYSTEM_API ATrueFPSFireWeaponProjectile : public ATrueFPSFireWeaponBase
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Fire Weapon Instant")
	TObjectPtr<UTrueFPSFireWeaponProjectileSettings> FireProjectileSettings;

public:

	ATrueFPSFireWeaponProjectile(const FObjectInitializer& ObjectInitializer);

	/** apply config on projectile */
	void ApplyWeaponConfig(UTrueFPSFireWeaponProjectileSettings* Data);

protected:

	virtual EAmmoType GetAmmoType() const override
	{
		return EAmmoType::ERocket;
	}

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() override;

	/** spawn projectile on server */
	UFUNCTION(reliable, server)
	void ServerFireProjectile(FVector Origin, FVector_NetQuantizeNormal ShootDir);
	
};
