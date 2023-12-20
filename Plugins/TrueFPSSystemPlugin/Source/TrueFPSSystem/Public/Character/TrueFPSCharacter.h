// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TrueFPSTypes.h"
#include "InputActionValue.h"
#include "Character/TrueFPSCharacterInterface.h"
#include "State/TrueFPSCharacterState.h"
#include "Settings/TrueFPSCharacterSettings.h"
#include "TrueFPSCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;
class USoundCue;

UCLASS(Abstract, AutoExpandCategories = ("Settings|TrueFPS Character", "State|TrueFPS Character"))
class TRUEFPSSYSTEM_API ATrueFPSCharacter : public ACharacter, public ITrueFPSCharacterInterface
{
	GENERATED_BODY()

protected:

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "TrueFPS Character")
	TObjectPtr<USkeletalMeshComponent> ClientMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character")
	TObjectPtr<UTrueFPSCharacterSettings> Settings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> AimingAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> NextWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> PreviousWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LeanLeftAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LeanRightAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|TrueFPS Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ToggleSightsAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|TrueFPS Character", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookUpMouseSensitivity{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|TrueFPS Character", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookRightMouseSensitivity{1.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Character", Transient)
	FTrueFPSCharacterState State;

	// Replicated State

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Character", Transient, Replicated)
	TArray<ATrueFPSWeaponBase*> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Character", Transient, ReplicatedUsing = OnRep_CurrentWeapon)
	ATrueFPSWeaponBase* CurrentWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Character", Transient, ReplicatedUsing = OnRep_LastTakeHitInfo)
	FTakeHitInfo LastTakeHitInfo;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Character", Transient, Replicated)
	float LeanValueTarget{0.f};
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Character", Transient, Replicated)
	float CrouchValueTarget{0.f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Character", Transient, Replicated)
	float Health{50.f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Character", Transient, Replicated)
	bool bIsAiming{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|TrueFPS Character", Transient, Replicated)
	bool bWantsToRun{false};

public:
	
	ATrueFPSCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginDestroy() override;

	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;

	/** Update the character. (Running, health etc). */
	virtual void Tick(float DeltaTime) override;

	/** cleanup inventory */
	virtual void Destroyed() override;

	/** update mesh for first person view */
	virtual void PawnClientRestart() override;

	/** [server] perform PlayerState related setup */
	virtual void PossessedBy(class AController* C) override;

	/** [client] perform PlayerState related setup */
	virtual void OnRep_PlayerState() override;

	/** [server] called to determine if we should pause replication this actor to a specific player */
	virtual bool IsReplicationPausedForConnection(const FNetViewer& ConnectionOwnerNetViewer) override;

	/** [client] called when replication is paused for this actor */
	virtual void OnReplicationPausedChanged(bool bIsReplicationPaused) override;
	
	/**
	* Check if pawn is enemy if given controller.
	*
	* @param	TestPC	Controller to check against.
	*/
	bool IsEnemyFor(AController* TestPC) const;

	UFUNCTION(BlueprintPure)
	FORCEINLINE USkeletalMeshComponent* GetClientMesh() const { return ClientMesh; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool GetIsDying() const { return State.bIsDying; }
	
protected:
	
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	//////////////////////////////////////////////////////////////////////////
	// Locomotion

protected:
	
	void Move(const FInputActionValue& ActionValue);
	void Look(const FInputActionValue& ActionValue);

public:
	
	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerYawInput(float Val) override;
	virtual void AddControllerRollInput(float Val) override;

	//////////////////////////////////////////////////////////////////////////
	// Inventory

public:

	/**
	* [server] add weapon to inventory
	*
	* @param Weapon	Weapon to add.
	*/
	void AddWeapon(class ATrueFPSWeaponBase* Weapon);

	/**
	* [server] remove weapon from inventory
	*
	* @param Weapon	Weapon to remove.
	*/
	void RemoveWeapon(class ATrueFPSWeaponBase* Weapon);

	/**
	* Find in inventory
	*
	* @param WeaponClass	Class of weapon to find.
	*/
	class ATrueFPSWeaponBase* FindWeapon(TSubclassOf<class ATrueFPSWeaponBase> WeaponClass);

	/**
	* [server + local] equips weapon from inventory
	*
	* @param Weapon	Weapon to equip
	*/
	void EquipWeapon(class ATrueFPSWeaponBase* Weapon);

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] starts weapon fire */
	void StartWeaponFire();

	/** [local] stops weapon fire */
	void StopWeaponFire();

	/** check if pawn can fire weapon */
	bool CanFire() const;

	/** check if pawn can reload weapon */
	bool CanReload() const;

	/** [server + local] change aiming state */
	void SetAiming(bool bNewAiming);

	//////////////////////////////////////////////////////////////////////////
	// Movement

	/** [server + local] change running state */
	void SetRunning(bool bNewRunning, bool bToggle);

	//////////////////////////////////////////////////////////////////////////
	// Animations

	/** play anim montage */
	virtual float PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None) override;

	/** stop playing montage */
	virtual void StopAnimMontage(class UAnimMontage* AnimMontage) override;

	/** stop playing all montages */
	void StopAllAnimMontages() const;

	//////////////////////////////////////////////////////////////////////////
	// Input handlers

	/** player pressed start fire action */
	UFUNCTION(BlueprintCallable, Category = "TrueFPS|TrueFPS Character")
	void OnStartFire();

	/** player released start fire action */
	UFUNCTION(BlueprintCallable, Category = "TrueFPS|TrueFPS Character")
	void OnStopFire();

	/** player pressed aiming action */
	void OnStartAiming();

	/** player released aiming action */
	void OnStopAiming();

	/** player pressed next weapon action */
	void OnNextWeapon();

	/** player pressed prev weapon action */
	void OnPrevWeapon();

	/** player pressed reload action */
	void OnReload();

	/** player pressed jump action */
	void OnStartJump();

	/** player released jump action */
	void OnStopJump();

	/** player pressed run action */
	void OnStartRunning();

	/** player released run action */
	void OnStopRunning();

	/** player pressed left lean action */
	void OnLeanLeft();

	/** player pressed right lean action */
	void OnLeanRight();

	/** player released lean action */
	void OnStopLean();

	/** player pressed crouch action */
	void OnStartCrouching();

	/** player released crouch action */
	void OnStopCrouching();

	/** player toggled sights */
	void OnToggleSights();

	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get mesh component */
	UFUNCTION(BlueprintPure, Category = "TrueFPS|TrueFPS Character")
	USkeletalMeshComponent* GetPawnMesh() const;

	/** get currently equipped weapon */
	UFUNCTION(BlueprintCallable, Category = "TrueFPS|TrueFPS Character")
	class ATrueFPSWeaponBase* GetWeapon() const;

	/** get total number of inventory items */
	int32 GetInventoryCount() const;

	/**
	* get weapon from inventory at index. Index validity is not checked.
	*
	* @param Index Inventory index
	*/
	class ATrueFPSWeaponBase* GetInventoryWeapon(int32 Index) const;

	/** get weapon target modifier speed	*/
	UFUNCTION(BlueprintCallable, Category = "TrueFPS|TrueFPS Character")
	float GetAimingSpeedModifier() const;

	/** get aiming state */
	UFUNCTION(BlueprintCallable, Category = "TrueFPS|TrueFPS Character")
	bool IsAiming() const;

	/** get the modifier value for running speed */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	float GetRunningSpeedModifier() const;

	/** get running state */
	UFUNCTION(BlueprintCallable, Category = "TrueFPS|TrueFPS Character")
	bool IsRunning() const;

	/** get max health */
	int32 GetMaxHealth() const;

	/** check if pawn is still alive */
	bool IsAlive() const;

	/** returns percentage of health when low health effects should start */
	float GetLowHealthPercentage() const;

protected:

	void SetLeanValue(float NewLeanValue);

	UFUNCTION(Server, Reliable)
	void ServerSetLeanValue(float NewLeanValue);

	void ToggleCrouching(bool bNewCrouching);

	UFUNCTION(Server, Reliable)
	void ServerToggleCrouching(bool bNewCrouching);

	void RefreshLeanValue(float DeltaTime);

	void RefreshCrouchValue(float DeltaTime);

	void RefreshHealthRegen(float DeltaTime);

	void RefreshWallAvoidanceState(float DeltaTime);

protected:
	
	/** Responsible for cleaning up bodies on clients. */
	virtual void TornOff();

private:

	/** Whether or not the character is moving (based on movement input). */
	bool IsMoving() const;

	//////////////////////////////////////////////////////////////////////////
	// TrueFPSCharacterInterface overriding
	
public:

	virtual bool TrueFPSInterface_IsAlive_Implementation() const override;

	virtual USkeletalMeshComponent* TrueFPSInterface_GetSpecificPawnMesh_Implementation(bool WantFirstPerson) const override;

	virtual FRotator TrueFPSInterface_GetLastUpdatedRotationValues_Implementation() const override;

	virtual FTransform TrueFPSInterface_GetDomHandTransform_Implementation() const override;

	virtual bool TrueFPSInterface_IsRunning_Implementation() const override;

	virtual float TrueFPSInterface_GetCrouchValue_Implementation() const override;

	virtual float TrueFPSInterface_GetLeanValue_Implementation() const override;

	virtual FVector TrueFPSInterface_GetFirstPersonCameraTarget_Implementation() const override;

	virtual FRotator TrueFPSInterface_GetFirstPersonCameraTargetRotation_Implementation() const override;

	virtual bool TrueFPSInterface_HasCurrentWeapon_Implementation() const override;

	virtual class ATrueFPSWeaponBase* TrueFPSInterface_GetCurrentWeapon_Implementation() const override;

	virtual bool TrueFPSInterface_IsAiming_Implementation() const override;

	virtual bool TrueFPSInterface_CanFire_Implementation() const override;

	virtual bool TrueFPSInterface_CanReload_Implementation() const override;

	virtual float TrueFPSInterface_GetAimingInterpSpeed_Implementation() const override;

	//////////////////////////////////////////////////////////////////////////
	// Damage & death

	/** Take damage, handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	/** Pawn suicide */
	virtual void Suicide();

	/** Kill this pawn */
	virtual void KilledBy(class APawn* EventInstigator);

	/** Returns True if the pawn can die in the current state */
	virtual bool CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const;

	/**
	* Kills pawn.  Server/authority only.
	* @param KillingDamage - Damage amount of the killing blow
	* @param DamageEvent - Damage event of the killing blow
	* @param Killer - Who killed this pawn
	* @param DamageCauser - the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
	* @returns true if allowed
	*/
	virtual bool Die(float KillingDamage, struct FDamageEvent const& DamageEvent, class AController* Killer, class AActor* DamageCauser);

	// Die when we fall out of the world.
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/** Called on the actor right before replication occurs */
	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;
	
protected:
	
	/** notification when killed, for both the server and client. */
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser);

	/** play effects on hit */
	virtual void PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled = false);

	/** switch to ragdoll */
	void SetRagdollPhysics();

	/** sets up the replication for taking a hit */
	void ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser, bool bKilled);

	/** play hit or death on client */
	UFUNCTION()
	void OnRep_LastTakeHitInfo();

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** updates current weapon */
	void SetCurrentWeapon(class ATrueFPSWeaponBase* NewWeapon, class ATrueFPSWeaponBase* LastWeapon = nullptr);

	/** current weapon rep handler */
	UFUNCTION()
	void OnRep_CurrentWeapon(class ATrueFPSWeaponBase* LastWeapon);

	/** [server] spawns default inventory */
	void SpawnDefaultInventory();

	/** [server] remove all weapons from inventory and destroy them */
	void DestroyInventory();

	/** equip weapon */
	UFUNCTION(reliable, server)
	void ServerEquipWeapon(class ATrueFPSWeaponBase* NewWeapon);

	/** update aiming state */
	UFUNCTION(reliable, server)
	void ServerSetAiming(bool bNewAiming);

	/** update targeting state */
	UFUNCTION(reliable, server)
	void ServerSetRunning(bool bNewRunning, bool bToggle);

	/** Builds list of points to check for pausing replication for a connection*/
	void BuildPauseReplicationCheckPoints(TArray<FVector>& RelevancyCheckPoints) const;
	
};

inline void ATrueFPSCharacter::AddControllerPitchInput(float Val)
{
	Super::AddControllerPitchInput(Val);
	State.CurrentPitchControllerInput = Val;
}

inline void ATrueFPSCharacter::AddControllerYawInput(float Val)
{
	Super::AddControllerYawInput(Val);
	State.CurrentYawControllerInput = Val;
}

inline void ATrueFPSCharacter::AddControllerRollInput(float Val)
{
	Super::AddControllerRollInput(Val);
	State.CurrentRollControllerInput = Val;
}

inline bool ATrueFPSCharacter::TrueFPSInterface_IsAlive_Implementation() const
{
	return IsAlive();
}

inline USkeletalMeshComponent* ATrueFPSCharacter::TrueFPSInterface_GetSpecificPawnMesh_Implementation(bool WantFirstPerson) const
{
	return WantFirstPerson ? ClientMesh : GetMesh();
}

inline FRotator ATrueFPSCharacter::TrueFPSInterface_GetLastUpdatedRotationValues_Implementation() const
{
	return State.GetCurrentControllerInput();
}

inline FTransform ATrueFPSCharacter::TrueFPSInterface_GetDomHandTransform_Implementation() const
{
	return GetMesh()->GetSocketTransform(Settings->bRightHanded ? Settings->RightHandName : Settings->LeftHandName);
}

inline bool ATrueFPSCharacter::TrueFPSInterface_IsRunning_Implementation() const
{
	return IsRunning();
}

inline float ATrueFPSCharacter::TrueFPSInterface_GetCrouchValue_Implementation() const
{
	return State.CrouchValue;
}

inline float ATrueFPSCharacter::TrueFPSInterface_GetLeanValue_Implementation() const
{
	return State.LeanValue;
}

inline FVector ATrueFPSCharacter::TrueFPSInterface_GetFirstPersonCameraTarget_Implementation() const
{
	return GetMesh()->GetSocketLocation(FName("head"));
}

inline FRotator ATrueFPSCharacter::TrueFPSInterface_GetFirstPersonCameraTargetRotation_Implementation() const
{
	return GetControlRotation();
}

inline bool ATrueFPSCharacter::TrueFPSInterface_HasCurrentWeapon_Implementation() const
{
	return CurrentWeapon != nullptr;
}

inline ATrueFPSWeaponBase* ATrueFPSCharacter::TrueFPSInterface_GetCurrentWeapon_Implementation() const
{
	return CurrentWeapon;
}

inline bool ATrueFPSCharacter::TrueFPSInterface_IsAiming_Implementation() const
{
	return IsAiming();
}

inline bool ATrueFPSCharacter::TrueFPSInterface_CanFire_Implementation() const
{
	return CanFire();
}

inline bool ATrueFPSCharacter::TrueFPSInterface_CanReload_Implementation() const
{
	return CanFire();
}

inline float ATrueFPSCharacter::TrueFPSInterface_GetAimingInterpSpeed_Implementation() const
{
	if (!Settings) return 10.f;
	return Settings->AimingInterpSpeed;
}
