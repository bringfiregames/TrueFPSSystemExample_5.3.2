#pragma once

#include "TrueFPSAnimInstanceState.generated.h"

class UCurveVector;

USTRUCT(BlueprintType)
struct TRUEFPSSYSTEM_API FTrueFPSAnimInstanceState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation")
	TWeakObjectPtr<UAnimSequence> WeaponAnimPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation")
	TWeakObjectPtr<UCurveVector> IdleWeaponSwayCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation")
	TWeakObjectPtr<UCurveVector> MovementWeaponSwayLocationCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation")
	TWeakObjectPtr<UCurveVector> MovementWeaponSwayRotationCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation")
	bool bFirstPerson{true};

	/**
	/* Stationary
	/*/
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Stationary")
	float RootYawOffset{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Stationary")
	bool bIsTurningInPlace{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Stationary")
	float StationaryYawThreshold{45.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Stationary")
	float StationaryVelocityThreshold{10.f};

	// The stationary rotation speed and direction (negative == left positive == right) ranging from -8 to 8
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Stationary")
	float StationaryYawInterpSpeed{0.f};

	// The current yaw rotation speed of this frame. Multiply play rate based on this value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Stationary")
	float StationaryYawSpeedNormal{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Stationary")
	float StationaryYawAmount{0.f};

	/**
	/* IK
	/*/

	// The current weapon sights transform relative to the Dominant-Hand. Used for aiming calculations.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|IK")
	FTransform SightsRelativeTransform{FTransform::Identity};

	// The current weapon origin transform relative to the Dominant-Hand.
	// Used for determining the pivot-point / orientation of this weapon.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|IK")
	FTransform OriginRelativeTransform{FTransform::Identity};

	// Applies an offset to the weapon transform
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|IK")
	FTransform OffsetTransform{FTransform::Identity};

	// Applies an offset to the weapon that doesn't affect aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|IK")
	FTransform PlacementTransform{FTransform::Identity};

	// A value between 0 and 1 that determines the amount we are aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|IK", Meta = (ClampMin = 0, ClampMax = 1))
	float AimingValue{0.f};

	// The multiplier for certain offsets. Larger scale means more exaggerated movements, thus a heavier appearance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|IK", Meta = (ClampMin = 1))
	float OffsetWeightScale{1.f};

	// The current weapon's custom placement transform
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|IK")
	FTransform CurrentWeaponCustomOffsetTransform{FTransform::Identity};

	// The current weapon's off hand additive transform
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|IK")
	FTransform CurrentWeaponOffHandAdditiveTransform{FTransform::Identity};

	/**
	/* Basic Locomotion
	/*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Locomotion")
	float MovementDirection{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Locomotion")
	float MovementSpeed{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Locomotion")
	bool bIsFalling{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Locomotion")
	float MovementWeaponSwayProgress{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Locomotion", Meta = (ClampMin = 0, ClampMax = 1))
	float MovementAnimationsAvoidance{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Locomotion")
	bool bIsRunning{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Locomotion")
	float CrouchValue{0.f};

	/**
	/* Accumulative Offsets
	/*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Accumulative Offsets")
	FRotator AccumulativeRotation{FRotator::ZeroRotator};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Accumulative Offsets")
	FRotator AccumulativeRotationInterp{FRotator::ZeroRotator};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Accumulative Offsets")
	FVector VelocityTarget{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Accumulative Offsets")
	FVector VelocityInterp{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Accumulative Offsets")
	float MovementSpeedInterp{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Accumulative Offsets")
	float MovementWeaponSwayProgressTime{0.f};

	/**
	/* Others
	/*/

	// This value should be pre-calculated
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Others")
	FRotator CameraRotation{FRotator(0.f, 90.f, 0.f)};

	// Relative to the root
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Others")
	FRotator AimRotation{FRotator(0.f, 90.f, 0.f)};

	// Camera's transform relative to the head
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Others")
	FVector CameraRelativeLocation{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Others")
	FVector LastVelocity{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Others")
	FRotator LastCameraRotation{FRotator::ZeroRotator};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS|Animation|Others")
	FRotator AimingHeadRotationOffset{FRotator::ZeroRotator};

};
