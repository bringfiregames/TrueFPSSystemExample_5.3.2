// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "State/TrueFPSAnimInstanceState.h"
#include "TrueFPSAnimInstanceBase.generated.h"

class ATrueFPSWeaponBase;
class UTrueFPSAnimInstanceSettings;

UCLASS()
class TRUEFPSSYSTEM_API UTrueFPSAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TrueFPS Settings")
	TObjectPtr<UTrueFPSAnimInstanceSettings> Settings;

	// The Character that owns this Anim Instance
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TObjectPtr<ACharacter> Character;

	// The Mesh that owns this Anim Instance
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TObjectPtr<USkeletalMeshComponent> Mesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FTrueFPSAnimInstanceState State;

public:

	UTrueFPSAnimInstanceBase();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

private:

	void RefreshRotations(float DeltaTime);

	void RefreshLocomotionState(float DeltaTime);

	void RefreshRelativeTransforms(float DeltaTime);

	void RefreshAiming(float DeltaTime);

	void RefreshAccumulativeOffsets(float DeltaTime);

	void RefreshPlacementTransform(float DeltaTime);

	void RefreshTurnInPlaceState(float DeltaTime);

	void PostRefresh();

public:

	// Called when the Character's current weapon changes
	void OnChangeWeapon(const ATrueFPSWeaponBase* NewWeapon);
	
};
