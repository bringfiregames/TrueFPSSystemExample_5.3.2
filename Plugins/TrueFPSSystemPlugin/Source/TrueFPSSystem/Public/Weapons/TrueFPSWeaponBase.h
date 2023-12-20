// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrueFPSTypes.h"
#include "Settings/TrueFPSWeaponSettings.h"
#include "State/TrueFPSWeaponState.h"
#include "TrueFPSWeaponBase.generated.h"

class UAnimMontage;
class ACharacter;
class UAudioComponent;
class UParticleSystemComponent;
class UForceFeedbackEffect;
class USoundBase;
class UCameraShakeBase;

UCLASS(Abstract)
class TRUEFPSSYSTEM_API ATrueFPSWeaponBase : public AActor
{
	GENERATED_BODY()

protected:
	
	/** weapon mesh: 1st person view */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "TrueFPS Weapon")
	TObjectPtr<USkeletalMeshComponent> Mesh1P;

	/** weapon mesh: 3rd person view */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "TrueFPS Weapon")
	TObjectPtr<USkeletalMeshComponent> Mesh3P;
	
	/** pawn owner */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TrueFPS Weapon", Transient, ReplicatedUsing=OnRep_MyPawn)
	TObjectPtr<ACharacter> MyPawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Weapon")
	TObjectPtr<UTrueFPSWeaponSettings> Settings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Weapon", Transient)
	FTrueFPSWeaponState State;

	// Replicated State
	
	/** burst counter, used for replicating fire events to remote clients */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|MM Weapon", Transient, ReplicatedUsing = OnRep_BurstCounter)
	int32 BurstCounter{0};

	/** target sights relative transform */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|MM Weapon", Transient, Replicated)
	FTransform TargetSightsRelativeTransform{FTransform::Identity};
	
public:

	ATrueFPSWeaponBase(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	
	/** perform initial setup */
	virtual void PostInitializeComponents() override;

	virtual void Destroyed() override;

	//////////////////////////////////////////////////////////////////////////
	// Ammo
	
	enum class EAmmoType
	{
		ENone,
		EBullet,
		ERocket,
		EMax,
	};

	/** query ammo type */
	virtual EAmmoType GetAmmoType() const
	{
		return EAmmoType::ENone;
	}

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** weapon is being equipped by owner pawn */
	virtual void OnEquip(const ATrueFPSWeaponBase* LastWeapon);

	/** weapon is now equipped by owner pawn */
	virtual void OnEquipFinished();

	/** weapon is holstered by owner pawn */
	virtual void OnUnEquip();

	/** [server] weapon was added to pawn's inventory */
	virtual void OnEnterInventory(ACharacter* NewOwner);

	/** [server] weapon was removed from pawn's inventory */
	virtual void OnLeaveInventory();

	/** check if it's currently equipped */
	bool IsEquipped() const;

	/** check if mesh is already attached */
	bool IsAttachedToPawn() const;


	//////////////////////////////////////////////////////////////////////////
	// Input

	/** [local + server] start weapon fire */
	virtual void StartFire();

	/** [local + server] stop weapon fire */
	virtual void StopFire();

	/** [server] toggle between sights */
	virtual void ToggleSights();


	//////////////////////////////////////////////////////////////////////////
	// Control

	/** check if weapon can fire */
	virtual bool CanFire() const;

	/** check if weapon can be reloaded */
	virtual bool CanAim() const;


	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get current weapon state */
	FTrueFPSWeaponState& GetState();

	/** get weapon mesh (needs pawn owner to determine variant) */
	USkeletalMeshComponent* GetWeaponMesh() const;

	/** get pawn owner */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	class ACharacter* GetPawnOwner() const
	{
		return MyPawn;
	};

	/** get casted pawn owner */
	template<typename T>
	T* GetPawnOwner() const
	{
		return Cast<T>(MyPawn);
	}

	/** get if is first person using interface */
	bool IsFirstPerson() const;

	/** set the weapon's owning pawn */
	void SetOwningPawn(ACharacter* ACharacter);

	/** gets last time when this weapon was switched to */
	float GetEquipStartedTime() const;

	/** gets the duration of equipping weapon*/
	float GetEquipDuration() const;

	void GetAttachments(TArray<class ATrueFPSWeaponAttachmentBase*>& OutAttachments) const;

	template<typename T>
	void GetAttachments(TArray<T*>& OutAttachments) const;

	void GetAttachmentsOfClass(TArray<class ATrueFPSWeaponAttachmentBase*>& OutAttachments, const TSubclassOf<class ATrueFPSWeaponAttachmentBase>& Class) const;

	void GetAttachmentPoints(TArray<class UTrueFPSWeaponAttachmentPoint*>& OutAttachmentPoints) const;
	
	FORCEINLINE bool IsCloseToWall() const { return GetWallOffsetTransformAlpha() > 0.f; }

protected:

	//////////////////////////////////////////////////////////////////////////
	// Input - server side

	UFUNCTION(reliable, server)
	void ServerStartFire();

	UFUNCTION(reliable, server)
	void ServerStopFire();

	UFUNCTION(reliable, server)
	void ServerToggleSights();


	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	UFUNCTION()
	void OnRep_MyPawn();

	UFUNCTION()
	void OnRep_BurstCounter();

	/** Called in network play to do the cosmetic fx for firing */
	virtual void SimulateWeaponFire();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void StopSimulatingWeaponFire();

	
	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() PURE_VIRTUAL(ATrueFPSWeaponBase::FireWeapon,);

	/** [server] fire & update ammo */
	UFUNCTION(reliable, server)
	void ServerHandleFiring();
	virtual void ServerHandleFiring_Implementation();

	/** [local + server] handle weapon refire, compensating for slack time if the timer can't sample fast enough */
	void HandleReFiring();

	/** [local + server] handle weapon fire */
	virtual void HandleFiring();

	/** [local + server] firing started */
	virtual void OnBurstStarted();

	/** [local + server] firing finished */
	virtual void OnBurstFinished();

	/** update weapon state */
	void SetWeaponState(EWeaponState::Type NewState);

	/** determine current weapon state */
	virtual void DetermineWeaponState();

	/** adds new recoil instance */
	FORCEINLINE void AddRecoilInstance(const FRecoilParams& RecoilParams)
	{
		if (!RecoilParams.IsValid()) return;
		State.CurrentRecoilInstance = FRecoilInstance(RecoilParams);
	}

	/** handles recoil instances */
	void HandleRecoil(float DeltaSeconds);

	void ApplyCameraRecoil();
	void ResetCameraRecoil();
	void RefreshCameraRecoil(float DeltaTime);
	float ClampCameraRecoilRecovery(const float Value, const float Clamp) const;
	void AddCameraRecoilControlRotation(const FVector2D& Amount) const;


	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** attaches weapon mesh to pawn's mesh */
	void AttachMeshToPawn();

	/** detaches weapon mesh from pawn */
	void DetachMeshFromPawn() const;


	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	/** play weapon sounds */
	UAudioComponent* PlayWeaponSound(USoundBase* Sound);

	/** play weapon animations */
	float PlayWeaponAnimation(const FWeaponAnim& Animation) const;

	/** stop playing weapon animations */
	void StopWeaponAnimation(const FWeaponAnim& Animation) const;

	/** Get the aim of the weapon, allowing for adjustments to be made by the weapon */
	virtual FVector GetAdjustedAim() const;

	/** Get the aim of the camera */
	FVector GetCameraAim() const;

	/** get the originating location for camera damage */
	virtual FVector GetCameraDamageStartLocation(const FVector& AimDir) const;

	//////////////////////////////////////////////////////////////////////////
	// Weapon anim helpers

public:

	FTransform GetOffsetTransform() const;

	FTransform GetDefaultSightsRelativeTransform() const;

	FTransform GetSightsWorldTransform() const;

	FTransform GetOriginRelativeTransform() const;

	FTransform GetOriginWorldTransform() const;
	
	FTransform GetOrientationRelativeTransform(const float AimingValue = 0.f) const;
	
	FTransform GetOrientationWorldTransform(const float AimingValue = 0.f) const;

	UTrueFPSWeaponSettings* GetSettings() const { return Settings; }

	FORCEINLINE float GetWallOffsetTransformAlpha() const { return State.WallOffsetTransformAlpha; }

protected:
	
	float CalculateWallOffsetTransformAlpha() const;

	//////////////////////////////////////////////////////////////////////////
	// Debug

protected:

#if WITH_EDITORONLY_DATA
	class UStaticMeshComponent* OriginWidgetComp = nullptr;

	// Whether or not to show the visual-representation of the Origin Transform
	UPROPERTY(EditDefaultsOnly, Category = "Config|Animation|Origin Visualizer")
	bool bShowOriginWidget = false;

	// Setting this will override the default origin widget mesh
	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "bShowOriginWidget"), Category = "Config|Animation|Origin Visualizer")
	class UStaticMesh* OriginVisualizationMesh;
	
	virtual void RegisterAllComponents() override;
	
#endif// WITH_EDITORONLY_DATA

public:

#if WITH_EDITORONLY_DATA
	class UStaticMeshComponent* AimOffsetComp = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Animation|Aiming|Visualization")
	bool bShowAimOffset = false;
	
	UPROPERTY(EditDefaultsOnly, Meta = (DisplayName = "bShowAimOffset", EditCondition = bShowAimOffset), Category = "Config|Animation|Aiming|Visualization")
	class UStaticMesh* AimOffsetMesh;
#endif// WITH_EDITORONLY_DATA

};

inline FTrueFPSWeaponState& ATrueFPSWeaponBase::GetState()
{
	return State;
}
