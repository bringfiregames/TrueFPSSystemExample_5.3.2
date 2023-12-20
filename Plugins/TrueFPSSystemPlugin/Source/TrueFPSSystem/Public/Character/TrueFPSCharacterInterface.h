// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TrueFPSCharacterInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UTrueFPSCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TRUEFPSSYSTEM_API ITrueFPSCharacterInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
	
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character")
	bool TrueFPSInterface_IsAlive() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character")
	class USkeletalMeshComponent* TrueFPSInterface_GetSpecificPawnMesh(bool WantFirstPerson) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Animations")
	FRotator TrueFPSInterface_GetLastUpdatedRotationValues() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Animations")
	FTransform TrueFPSInterface_GetDomHandTransform() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Animations")
	bool TrueFPSInterface_IsRunning() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Animations")
	float TrueFPSInterface_GetCrouchValue() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Animations")
	float TrueFPSInterface_GetLeanValue() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Animations")
	FVector TrueFPSInterface_GetFirstPersonCameraTarget() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Animations")
	FRotator TrueFPSInterface_GetFirstPersonCameraTargetRotation() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Animations")
	float TrueFPSInterface_GetAimingInterpSpeed() const;

	//////////////////////////////////////////////////////////////////////////
	// Weapons

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Weapons")
	bool TrueFPSInterface_HasCurrentWeapon() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Weapons")
	class ATrueFPSWeaponBase* TrueFPSInterface_GetCurrentWeapon() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Weapons")
	bool TrueFPSInterface_IsAiming() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Weapons")
	bool TrueFPSInterface_CanFire() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="TrueFPS Character|Weapons")
	bool TrueFPSInterface_CanReload() const;
	
};
