// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/TrueFPSAnimInstanceBase.h"

#include "KismetAnimationLibrary.h"
#include "Character/TrueFPSCharacterInterface.h"
#include "Weapons/TrueFPSWeaponBase.h"
#include "Settings/TrueFPSAnimInstanceSettings.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UTrueFPSAnimInstanceBase::UTrueFPSAnimInstanceBase()
{
}

void UTrueFPSAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ACharacter>(GetOwningActor());
}

void UTrueFPSAnimInstanceBase::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	check(IsValid(Settings));
	check(IsValid(Character));
	check(Character->Implements<UTrueFPSCharacterInterface>());
}

void UTrueFPSAnimInstanceBase::NativeUpdateAnimation(const float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!IsValid(Settings) || !IsValid(Character)) return;

	State.bFirstPerson = Character->IsLocallyControlled() && ITrueFPSCharacterInterface::Execute_TrueFPSInterface_IsAlive(Character);

	RefreshRotations(DeltaTime);
	RefreshLocomotionState(DeltaTime);
	RefreshAiming(DeltaTime);
	RefreshRelativeTransforms(DeltaTime);
	RefreshAccumulativeOffsets(DeltaTime);
	RefreshPlacementTransform(DeltaTime);
	RefreshTurnInPlaceState(DeltaTime);

	PostRefresh();
}

void UTrueFPSAnimInstanceBase::RefreshRotations(const float DeltaTime)
{
	check(IsInGameThread());

	if (const APlayerController* PC = Character->GetController<APlayerController>())
	{
		const FRotator CurrentControllerInputValues = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetLastUpdatedRotationValues(Character);

		const FRotator AddRot(
			CurrentControllerInputValues.Pitch * PC->InputPitchScale_DEPRECATED,
			CurrentControllerInputValues.Yaw * PC->InputYawScale_DEPRECATED,
			CurrentControllerInputValues.Roll * PC->InputRollScale_DEPRECATED);

		const FRotator PreCalculatedCameraRot = Character->GetBaseAimRotation() + AddRot;

		constexpr float ClampAngle = 89.f;
		State.CameraRotation = FRotator(FMath::ClampAngle(PreCalculatedCameraRot.Pitch, -ClampAngle, ClampAngle), PreCalculatedCameraRot.Yaw, PreCalculatedCameraRot.Roll);
	}
	else
	{
		constexpr float ClampAngle = 89.f;
		FRotator ClampedBaseAimRotation = Character->GetBaseAimRotation();
		ClampedBaseAimRotation.Pitch = FMath::ClampAngle(ClampedBaseAimRotation.Pitch, -ClampAngle, ClampAngle);

		State.CameraRotation = UKismetMathLibrary::RInterpTo(State.CameraRotation, ClampedBaseAimRotation, DeltaTime, Settings->NonLocalCameraRotationInterpSpeed);
	}
	
	State.AimRotation = State.CameraRotation - Character->GetActorRotation();

	// Leaning
	State.AimRotation.Roll = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetLeanValue(Character);
}

void UTrueFPSAnimInstanceBase::RefreshLocomotionState(const float DeltaTime)
{
	check(IsInGameThread());
	
	State.MovementDirection = UKismetAnimationLibrary::CalculateDirection(Character->GetCharacterMovement()->Velocity, FRotator(0.f, Character->GetBaseAimRotation().Yaw, 0.f));
	State.MovementSpeed = Character->GetCharacterMovement()->Velocity.Size();

	State.bIsFalling = Character->GetCharacterMovement()->IsFalling();
	State.bIsRunning = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_IsRunning(Character);
	State.CrouchValue = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetCrouchValue(Character);
}

void UTrueFPSAnimInstanceBase::RefreshRelativeTransforms(float DeltaTime)
{
	check(IsInGameThread());
	
	if (ATrueFPSWeaponBase* CurrentWeapon = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetCurrentWeapon(Character))
	{
		const FTransform DomHandTransform = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetDomHandTransform(Character);

		const FTransform OrientationWorldTransform = CurrentWeapon->GetOrientationWorldTransform(State.AimingValue);
		
		State.OriginRelativeTransform = OrientationWorldTransform.GetRelativeTransform(DomHandTransform);
		if (State.AimingValue > 0.f) State.SightsRelativeTransform = CurrentWeapon->GetSightsWorldTransform().GetRelativeTransform(DomHandTransform);

		State.CurrentWeaponOffHandAdditiveTransform = CurrentWeapon->GetState().OffHandAdditiveTransform;
	}
}

void UTrueFPSAnimInstanceBase::RefreshAiming(float DeltaTime)
{
	check(IsInGameThread());

	const auto bIsAiming = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_IsAiming(Character);
	State.AimingValue = UKismetMathLibrary::FInterpTo(State.AimingValue, bIsAiming ? 1.f : 0.f, DeltaTime, ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetAimingInterpSpeed(Character));
}

void UTrueFPSAnimInstanceBase::RefreshAccumulativeOffsets(float DeltaTime)
{
	check(IsInGameThread());

	constexpr float ANGLE_CLAMP = 4.5f * 60.f;
	const float AngleClampDelta = ANGLE_CLAMP * DeltaTime;
	
	const FRotator AddRotation = (State.CameraRotation - State.LastCameraRotation);
	FRotator AddRotationClamped = FRotator(FMath::ClampAngle(AddRotation.Pitch, -AngleClampDelta, AngleClampDelta) * 1.5f,
		FMath::ClampAngle(AddRotation.Yaw, -AngleClampDelta, AngleClampDelta),
		FMath::ClampAngle(AddRotation.Roll, -AngleClampDelta, AngleClampDelta));
	AddRotationClamped.Roll += AddRotationClamped.Yaw * 0.7f;

	// Multiplied by DeltaTime to remain frame-independent
	AddRotationClamped *= DeltaTime * 50.f;
	
	State.AccumulativeRotation += AddRotationClamped;
	State.AccumulativeRotation = UKismetMathLibrary::RInterpTo(State.AccumulativeRotation, FRotator::ZeroRotator, DeltaTime, Settings->AccumulativeRotationReturnInterpSpeed);
	State.AccumulativeRotationInterp = UKismetMathLibrary::RInterpTo(State.AccumulativeRotationInterp, State.AccumulativeRotation, DeltaTime, Settings->AccumulativeRotationInterpSpeed);

	const auto bApplyWeaponSwayCurve = !Character->GetCharacterMovement()->IsFalling() && Character->GetCharacterMovement()->Velocity.Size() > Settings->MinMoveSpeedToApplyMovementSway;
	State.MovementSpeedInterp = UKismetMathLibrary::FInterpTo(State.MovementSpeedInterp, bApplyWeaponSwayCurve ? Character->GetCharacterMovement()->Velocity.Size() : 0.f, DeltaTime, 3.f);

	const FVector Velocity = Character->GetCharacterMovement()->Velocity;
	const FVector Difference = (Velocity - State.LastVelocity) * DeltaTime;
	
	State.VelocityTarget = UKismetMathLibrary::VInterpTo(State.VelocityTarget, Character->GetCharacterMovement()->Velocity, DeltaTime, Settings->VelocityInterpSpeed);
	if (Difference.Z > 1.5f) // Jumping / landing impulse
		State.VelocityTarget.Z += Difference.Z * 400.f;

	// 25% when fully aiming
	State.VelocityInterp = UKismetMathLibrary::VInterpTo(State.VelocityInterp, State.VelocityTarget * (1.f - State.AimingValue * 0.75f), DeltaTime, Settings->VelocitySwaySpeed);

	State.MovementWeaponSwayProgressTime += DeltaTime * (State.MovementSpeedInterp / Settings->MaxMoveSpeed);

	if (ATrueFPSWeaponBase* CurrentWeapon = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetCurrentWeapon(Character))
	{
		const float TargetMovementWeaponSwayAvoidance = FMath::Max<float>(ITrueFPSCharacterInterface::Execute_TrueFPSInterface_IsAiming(Character) ? 1.f : 0.f, CurrentWeapon->GetState().CurrentState == EWeaponState::Firing ? 1.f : 0.f);
		State.MovementAnimationsAvoidance = UKismetMathLibrary::FInterpTo(State.MovementAnimationsAvoidance, TargetMovementWeaponSwayAvoidance, DeltaTime, ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetAimingInterpSpeed(Character));
		
		FVector TargetOffsetLocation{FVector::ZeroVector};
		FRotator TargetOffsetRotation{FRotator::ZeroRotator};

		// Get inverse to apply the opposite of the rotational influence to the weapon sway
		const FRotator AccumulativeRotationInterpInverse = State.AccumulativeRotationInterp.GetInverse();

		// Apply location offset from accumulative rotation inverse
		TargetOffsetLocation += FVector(0.f, AccumulativeRotationInterpInverse.Yaw, AccumulativeRotationInterpInverse.Pitch) / 6.f;

		// Apply location offset from interp orientation velocity
		const FVector OrientationVelocityInterp = (FRotator(0.f, State.CameraRotation.Yaw, 0.f) - State.VelocityInterp.Rotation()).Vector() * State.VelocityInterp.Size() * FVector(1.f, -1.f, 1.f);
		const FVector MovementOffset = (OrientationVelocityInterp / Settings->MaxMoveSpeed) * FMath::Max<float>(1.f - State.AimingValue, 0.6f);
		const FRotator MovementRotationOffset = FRotator(-MovementOffset.X, MovementOffset.Z, MovementOffset.Y) * Settings->VelocitySwayWeight;
		TargetOffsetLocation += MovementOffset;
		TargetOffsetRotation += MovementRotationOffset;
        
		// Add accumulative rotation
		TargetOffsetRotation += AccumulativeRotationInterpInverse;

		// Add movement offset to rotation
		TargetOffsetRotation += FRotator(MovementOffset.Z * 5.f, MovementOffset.Y, MovementOffset.Y * 2.f);

		// Apply weight scale of weapon to offsets before weapon sway curves
		TargetOffsetLocation *= State.OffsetWeightScale;
		TargetOffsetRotation.Pitch *= State.OffsetWeightScale;
		TargetOffsetRotation.Yaw *= State.OffsetWeightScale;
		TargetOffsetRotation.Roll *= State.OffsetWeightScale;

		// Apply idle vector curve anim to offset location
		if (State.IdleWeaponSwayCurve.IsValid())
		{
			const FVector SwayOffset = State.IdleWeaponSwayCurve->GetVectorValue(GetWorld()->GetTimeSeconds()) * 8.f * FMath::Max<float>(1.f - State.AimingValue, 0.1f);
			TargetOffsetLocation += SwayOffset;
			TargetOffsetRotation += FRotator(SwayOffset.Z * 0.3f, SwayOffset.Y * 0.5f, SwayOffset.Y * 1.2f);
		}
		
		// Apply movement offset to offset location
		if (State.MovementWeaponSwayLocationCurve.IsValid() && State.MovementWeaponSwayRotationCurve.IsValid() && State.bFirstPerson)
		{
			const FVector SwayLocationOffset = State.MovementWeaponSwayLocationCurve->GetVectorValue(State.MovementWeaponSwayProgressTime) * (State.MovementSpeedInterp / Settings->MaxMoveSpeed) * 5.f * FMath::Max<float>(1.f - State.MovementAnimationsAvoidance, 0.05f);
			const FVector SwayRotationOffset = State.MovementWeaponSwayRotationCurve->GetVectorValue(State.MovementWeaponSwayProgressTime) * (State.MovementSpeedInterp / Settings->MaxMoveSpeed) * 5.f * FMath::Max<float>(1.f - State.MovementAnimationsAvoidance, 0.05f);
			TargetOffsetLocation += SwayLocationOffset * 0.7f;
			TargetOffsetRotation += FRotator(SwayRotationOffset.Z, SwayRotationOffset.Y, SwayRotationOffset.Y);
		}

		State.OffsetTransform = CurrentWeapon->GetOffsetTransform() * FTransform(TargetOffsetRotation, TargetOffsetLocation);
	}
	else
	{
		State.OffsetTransform = FTransform::Identity;
	}
}

void UTrueFPSAnimInstanceBase::RefreshPlacementTransform(float DeltaTime)
{
	FVector TargetPlacementLocation{FVector::ZeroVector};
	FRotator TargetPlacementRotation{FRotator::ZeroRotator};

	TargetPlacementRotation.Pitch = State.CameraRotation.Pitch * Settings->WeaponPitchTiltMultiplier;
	TargetPlacementLocation.Z = -State.CameraRotation.Pitch * (Settings->WeaponPitchTiltMultiplier / 3.f);
	
	State.PlacementTransform = State.CurrentWeaponCustomOffsetTransform * FTransform(TargetPlacementRotation, TargetPlacementLocation);
}

void UTrueFPSAnimInstanceBase::RefreshTurnInPlaceState(const float DeltaTime)
{
	if (!State.bIsTurningInPlace && abs(State.RootYawOffset) < 2.f && Character->GetCharacterMovement()->Velocity.Size() < State.StationaryVelocityThreshold)
	{
		State.bIsTurningInPlace = true;
	}

	if (State.bIsTurningInPlace)
	{
		State.RootYawOffset += FRotator::NormalizeAxis(State.LastCameraRotation.Yaw - State.CameraRotation.Yaw);

		// If exceeded rotation or velocity thresholds, set turn in place to false and set rot speed to desired speed
		if (Character->GetCharacterMovement()->Velocity.Size() >= State.StationaryVelocityThreshold)
		{
			State.bIsTurningInPlace = false;
			State.StationaryYawInterpSpeed = 8.f;
		}
		else if (abs(State.RootYawOffset) >= State.StationaryYawThreshold)
		{
			State.bIsTurningInPlace = false;
			State.StationaryYawInterpSpeed = 5.f;
		}

		// If no longer turning in place, set the rotation amount for turning animation usage
		if (!State.bIsTurningInPlace)
		{
			State.StationaryYawAmount = -State.RootYawOffset;
		}
	}

	if (!State.bIsTurningInPlace && State.RootYawOffset)
	{
		const float YawDifference = FRotator::NormalizeAxis(State.LastCameraRotation.Yaw - State.CameraRotation.Yaw);
		State.RootYawOffset += YawDifference;
		
		if (-YawDifference > 0.f == State.StationaryYawAmount > 0.f)
		{
			State.StationaryYawAmount += -YawDifference;
		}

		State.StationaryYawSpeedNormal = FMath::Clamp<float>(State.StationaryYawAmount / 180.f, 1.5f, 3.f);
            
		// Never allow the yaw offset to exceed the yaw threshold
		State.RootYawOffset = FMath::ClampAngle(State.RootYawOffset, -State.StationaryYawThreshold, State.StationaryYawThreshold);
            
		// Never allow yaw offset to exceed yaw threshold
		State.RootYawOffset = UKismetMathLibrary::FInterpTo(State.RootYawOffset, 0.f, DeltaTime, State.StationaryYawInterpSpeed);

		// Once matched rotation, clear vars
		if (abs(State.RootYawOffset) < 2.f)
		{
			State.StationaryYawInterpSpeed = 0.f;
			State.StationaryYawAmount = 0.f;
			State.RootYawOffset = 0.f;
		}
	}
}

void UTrueFPSAnimInstanceBase::PostRefresh()
{
	State.LastCameraRotation = State.CameraRotation;
	State.LastVelocity = Character->GetCharacterMovement()->Velocity;
}

void UTrueFPSAnimInstanceBase::OnChangeWeapon(const ATrueFPSWeaponBase* NewWeapon)
{
	if (NewWeapon)
	{
		State.WeaponAnimPose = NewWeapon->GetSettings()->AnimPose;
		State.IdleWeaponSwayCurve = NewWeapon->GetSettings()->IdleWeaponSwayCurve;
		State.MovementWeaponSwayLocationCurve = NewWeapon->GetSettings()->MovementWeaponSwayLocationCurve;
		State.MovementWeaponSwayRotationCurve = NewWeapon->GetSettings()->MovementWeaponSwayRotationCurve;
		State.OffsetWeightScale = NewWeapon->GetSettings()->OffsetWeightScale;
		State.CurrentWeaponCustomOffsetTransform = NewWeapon->GetSettings()->CustomOffsetTransform;
		State.AimingHeadRotationOffset = NewWeapon->GetSettings()->AimingHeadRotationOffset;

		const FTransform DomHandTransform = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetDomHandTransform(Character);
		State.SightsRelativeTransform = NewWeapon->GetSightsWorldTransform().GetRelativeTransform(DomHandTransform);
	}
	else
	{
		State.WeaponAnimPose = nullptr;
		State.IdleWeaponSwayCurve = nullptr;
		State.MovementWeaponSwayLocationCurve = nullptr;
		State.MovementWeaponSwayRotationCurve = nullptr;
		State.OffsetWeightScale = 0.f;
		State.CurrentWeaponCustomOffsetTransform = FTransform::Identity;
		State.AimingHeadRotationOffset = FRotator::ZeroRotator;
		State.SightsRelativeTransform = FTransform::Identity;
	}
}
