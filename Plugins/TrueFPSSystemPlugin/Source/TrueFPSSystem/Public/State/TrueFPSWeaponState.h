#pragma once

#include "TrueFPSWeaponState.generated.h"

class ATrueFPSSightsAttachment;

namespace EWeaponState
{
	enum Type
	{
		Idle,
		Firing,
		Reloading,
		Equipping,
	};
}

USTRUCT(BlueprintType)
struct TRUEFPSSYSTEM_API FTrueFPSWeaponState
{
	GENERATED_BODY()

	/** current weapon state */
	EWeaponState::Type CurrentState{EWeaponState::Idle};

	/** is fire animation playing? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	bool bPlayingFireAnim{false};

	/** is weapon currently equipped? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	bool bIsEquipped{false};

	/** is weapon fire active? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	bool bWantsToFire{false};

	/** is equip animation playing? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	bool bPendingEquip{false};

	/** weapon is refiring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	bool bRefiring{false};

	/** time of last successful weapon fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float LastFireTime{0.f};

	/** last time when this weapon was switched to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float EquipStartedTime{0.f};

	/** how much time weapon needs to be equipped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float EquipDuration{0.f};

	/** [server] current sights being used (nullptr for default)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	TWeakObjectPtr<ATrueFPSSightsAttachment> CurrentSights;

	// Animations

	// The total offset from the accumulated recoil instances
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	FTransform RecoilOffset{FTransform::Identity};

	// The offset applied to the Off-Hand
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	FTransform OffHandAdditiveTransform{FTransform::Identity};

	// The sights transform relative to the weapon actor transform
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	FTransform SightsRelativeTransform{FTransform::Identity};

	// Custom weapon offset transform
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	FTransform OffsetTransform{FTransform::Identity};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	FRecoilInstance CurrentRecoilInstance;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float WallOffsetTransformAlpha{0.f};

	// Camera
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	bool bShouldUpdateCameraRecoil{false};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	bool bResetCameraRecoil{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	FVector2D TargetCameraRecoilAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	FVector2D TargetCameraRecoilRecoveryAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float CameraRecoilCurrentTime{0.f};

	// Effects

	/** firing audio (bLoopedFireSound set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	TWeakObjectPtr<UAudioComponent> FireAC;

	// Timer Handling

	/** Adjustment to handle frame rate affecting actual timer interval. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float TimerIntervalAdjustment{0.f};

	/** Handle for efficient management of OnEquipFinished timer */
	FTimerHandle TimerHandle_OnEquipFinished;

	/** Handle for efficient management of HandleFiring timer */
	FTimerHandle TimerHandle_HandleFiring;
	
	FTimerHandle TimerHandle_CameraRecoilRecovery;

};
