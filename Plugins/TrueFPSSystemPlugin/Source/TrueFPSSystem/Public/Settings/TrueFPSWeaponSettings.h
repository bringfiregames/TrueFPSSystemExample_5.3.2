// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/Canvas.h"
#include "Engine/DataAsset.h"
#include "TrueFPSTypes.h"
#include "TrueFPSWeaponSettings.generated.h"

class UCurveVector;
class UAnimSequence;

USTRUCT(BlueprintType)
struct FWeaponAnim
{
	GENERATED_USTRUCT_BODY()

	/** animation played on pawn (1st person view) */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	UAnimMontage* Pawn1P = nullptr;

	/** animation played on pawn (3rd person view) */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	UAnimMontage* Pawn3P = nullptr;

	/** animation played on weapon (1st person view) */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	UAnimMontage* Weapon1P = nullptr;

	/** animation played on weapon (3rd person view) */
	UPROPERTY(EditDefaultsOnly, Category="Animation")
	UAnimMontage* Weapon3P = nullptr;

	FWeaponAnim() {}
};

UCLASS(Blueprintable, BlueprintType)
class TRUEFPSSYSTEM_API UTrueFPSWeaponSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:

	// Stats

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Stats")
	float TimeBetweenShots{0.2f};

	/** Whether to allow automatic weapons to catch up with shorter refire cycles */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Stats")
	bool bAllowAutomaticWeaponCatchup{true};

	// Animations

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	TObjectPtr<UAnimSequence> AnimPose;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animation")
	FName AttachPoint{"hand_r"};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animation")
	FRotator AimingHeadRotationOffset{FRotator::ZeroRotator};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	float OffsetWeightScale{1.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	FTransform CustomOffsetTransform{FTransform::Identity};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	FTransform OriginRelativeTransform{FTransform::Identity};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	FTransform PlacementTransform{FTransform::Identity};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	float AimOffset{10.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	FName SightSocketName{"sights"};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	TObjectPtr<UCurveVector> IdleWeaponSwayCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	TObjectPtr<UCurveVector> MovementWeaponSwayLocationCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	TObjectPtr<UCurveVector> MovementWeaponSwayRotationCurve;

	/** equip animations */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	FWeaponAnim EquipAnim;

	/** fire animations */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	FWeaponAnim FireAnim;

	/** is fire animation looped? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	bool bLoopedFireAnim{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations|Wall Avoidance")
	FTransform WallOffsetTransform{{100.f, 0.f, 0.f}, {-10.f, 0.f, 0.f}};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations|Wall Avoidance")
	float WallOffsetTransformMaxDistance{80.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations|Wall Avoidance")
	float WallOffsetTransformMinDistance{30.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations|Recoil")
	FRecoilParams Recoil;

	// Effects

	/** camera shake on firing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects")
	TSubclassOf<UCameraShakeBase> FireCameraShake;

	/** force feedback effect to play when the weapon is fired */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects")
	TObjectPtr<UForceFeedbackEffect> FireForceFeedback;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Recoil")
	TObjectPtr<UCurveVector> CameraRecoilCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Recoil")
	float CameraRecoilMultiplier{1.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Recoil")
	float CameraRecoilRecoveryTime{0.2f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Recoil")
	float CameraRecoilRecoverySpeed{10.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Recoil")
	float CameraRecoilProgressRate{1.f};

	// Sounds
	
	/** name of bone/socket for attach sounds in weapon mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Sounds")
	FName SoundsAttachPoint;

	/** single fire sound (bLoopedFireSound not set) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Sounds")
	USoundBase* FireSound;

	/** looped fire sound (bLoopedFireSound set) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Sounds")
	USoundBase* FireLoopSound;

	/** finished burst sound (bLoopedFireSound set) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Sounds")
	USoundBase* FireFinishSound;

	/** out of ammo sound */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Sounds")
	USoundBase* OutOfAmmoSound;

	/** equip sound */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Sounds")
	USoundBase* EquipSound;

	/** is fire sound looped? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Sounds")
	bool bLoopedFireSound{false};

	// HUD
	
	/** icon displayed on the HUD when weapon is equipped as primary */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	FCanvasIcon PrimaryIcon;

	/** icon displayed on the HUD when weapon is secondary */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	FCanvasIcon SecondaryIcon;

	/** crosshair parts icons (left, top, right, bottom and center) */
	UPROPERTY(EditAnywhere, Category = "Settings|HUD")
	FCanvasIcon Crosshair[5];

	/** crosshair parts icons when targeting (left, top, right, bottom and center) */
	UPROPERTY(EditAnywhere, Category = "Settings|HUD")
	FCanvasIcon AimingCrosshair[5];

	/** only use red colored center part of aiming crosshair */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	bool UseLaserDot{false};

	/** false = default crosshair */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	bool UseCustomCrosshair{false};

	/** false = use custom one if set, otherwise default crosshair */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	bool UseCustomAimingCrosshair{false};

	/** true - crosshair will not be shown unless aiming with the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	bool bHideCrosshairWhileNotAiming{false};

	/** true - crosshair will not be shown unless aiming with the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	bool bHideCrosshairWhileAiming{true};
	
};
