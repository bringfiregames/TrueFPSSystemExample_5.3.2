// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TrueFPSWeaponBase.h"
#include "Settings/TrueFPSFireWeaponSettings.h"
#include "State/TrueFPSFireWeaponState.h"
#include "TrueFPSFireWeaponBase.generated.h"

UCLASS(Abstract)
class TRUEFPSSYSTEM_API ATrueFPSFireWeaponBase : public ATrueFPSWeaponBase
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Fire Weapon")
	TObjectPtr<UTrueFPSFireWeaponSettings> FireSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Fire Weapon", Transient)
	FTrueFPSFireWeaponState FireState;

	// Replicated State
	
	/** is reload animation playing? */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Fire Weapon", Transient, ReplicatedUsing=OnRep_Reload)
	bool bPendingReload{false};
	
	/** current total ammo */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Fire Weapon", Transient, Replicated)
	int32 CurrentAmmo{0};

	/** current ammo - inside clip */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Fire Weapon", Transient, Replicated)
	int32 CurrentAmmoInClip{0};

public:

	ATrueFPSFireWeaponBase(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	
	/** perform initial setup */
	virtual void PostInitializeComponents() override;

	//////////////////////////////////////////////////////////////////////////
	// Reading Data

	UTrueFPSFireWeaponSettings* GetFireSettings() const;

	const FTrueFPSFireWeaponState& GetFireState() const;
	
	/** get current ammo amount (total) */
	int32 GetCurrentAmmo() const;

	/** get current ammo amount (clip) */
	int32 GetCurrentAmmoInClip() const;

	/** get clip size */
	int32 GetAmmoPerClip() const;

	/** get max ammo amount */
	int32 GetMaxAmmo() const;
	
	/** check if weapon has infinite ammo (include owner's cheats) */
	bool HasInfiniteAmmo() const;

	/** check if weapon has infinite clip (include owner's cheats) */
	bool HasInfiniteClip() const;

	//////////////////////////////////////////////////////////////////////////
	// Ammo
	
	/** [server] add ammo */
	void GiveAmmo(int AddAmount);

	/** consume a bullet */
	void UseAmmo();

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	virtual void OnUnEquip() override;
	
	virtual void OnEquipFinished() override;

	//////////////////////////////////////////////////////////////////////////
	// Input
	
	/** [all] start weapon reload */
	virtual void StartReload(bool bFromReplication = false);

	/** [local + server] interrupt weapon reload */
	virtual void StopReload();

	/** [server] performs actual reload */
	virtual void ReloadWeapon();

	/** trigger reload from server */
	UFUNCTION(reliable, client)
	void ClientStartReload();

	//////////////////////////////////////////////////////////////////////////
	// Control

	virtual bool CanFire() const override;
	
	/** check if weapon can be reloaded */
	virtual bool CanReload() const;
	
protected:

	//////////////////////////////////////////////////////////////////////////
	// Input - Server Side
	
	UFUNCTION(reliable, server)
	void ServerStartReload();

	UFUNCTION(reliable, server)
	void ServerStopReload();

	//////////////////////////////////////////////////////////////////////////
	// Replication & Effects

	virtual void SimulateWeaponFire() override;

	virtual void StopSimulatingWeaponFire() override;
	
	UFUNCTION()
	void OnRep_Reload();

	//////////////////////////////////////////////////////////////////////////
	// Weapon Usage

	virtual void ServerHandleFiring_Implementation() override;

	virtual void HandleFiring() override;

	virtual void DetermineWeaponState() override;

	virtual FVector GetAdjustedAim() const override;

	/** get the muzzle location of the weapon */
	FVector GetMuzzleLocation() const;

	/** get direction of weapon's muzzle */
	FVector GetMuzzleDirection() const;

	/** find hit */
	FHitResult WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const;

	virtual FVector GetCameraDamageStartLocation(const FVector& AimDir) const override;
	
};

inline UTrueFPSFireWeaponSettings* ATrueFPSFireWeaponBase::GetFireSettings() const
{
	return FireSettings;
}

inline const FTrueFPSFireWeaponState& ATrueFPSFireWeaponBase::GetFireState() const
{
	return FireState;
}
