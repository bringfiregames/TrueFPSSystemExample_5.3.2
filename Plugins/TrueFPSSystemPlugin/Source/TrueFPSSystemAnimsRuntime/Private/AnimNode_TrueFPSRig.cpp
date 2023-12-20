// Fill out your copyright notice in the Description page of Project Settings.



// Fill out your copyright notice in the Description page of Project Settings.

#include "AnimNode_TrueFPSRig.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimationCore/Public/TwoBoneIK.h"
#include "BoneControllers/AnimNode_TwoBoneIK.h"

FAnimNode_TrueFPSRig::FAnimNode_TrueFPSRig()
{
	RightHand = FBoneReference(FName("hand_r"));
	LeftHand = FBoneReference(FName("hand_l"));
	Head = FBoneReference(FName("head"));

	StableBone = FBoneReference(FName("pelvis"));

	constexpr float Weight = 1.f/5.f;
	SpineBoneParams = { FBoneParams(FBoneReference(FName("spine_01")), Weight), FBoneParams(FBoneReference(FName("spine_02")), Weight),
		FBoneParams(FBoneReference(FName("spine_03")), Weight), FBoneParams(FBoneReference(FName("neck_01")), Weight), FBoneParams(FBoneReference(FName("head")), Weight) };
}

void FAnimNode_TrueFPSRig::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	BasePose.Initialize(Context);
	ReferencePose.Initialize(Context);

	const FBoneContainer& RequiredBones = Context.AnimInstanceProxy->GetRequiredBones();
	RightHand.Initialize(RequiredBones);
	LeftHand.Initialize(RequiredBones);

	Head.Initialize(RequiredBones);
	StableBone.Initialize(RequiredBones);
	for(FBoneParams& Bone : SpineBoneParams)
		Bone.Bone.Initialize(RequiredBones);

	const FReferenceSkeleton& RefSkel = RequiredBones.GetReferenceSkeleton();
	
	if(!RightHand.IsValidToEvaluate() || !LeftHand.IsValidToEvaluate()) return;
	RightLowerArmIndex = RefSkel.GetParentIndex(RightHand.BoneIndex);
	if(RightLowerArmIndex != INDEX_NONE) RightUpperArmIndex = RefSkel.GetParentIndex(RightLowerArmIndex);

	LeftLowerArmIndex = RefSkel.GetParentIndex(LeftHand.BoneIndex);
	if(LeftLowerArmIndex != INDEX_NONE) LeftUpperArmIndex = RefSkel.GetParentIndex(LeftLowerArmIndex);

	if(RightUpperArmIndex != INDEX_NONE) CachedRightUpperArmParentBoneIndex = RefSkel.GetParentIndex(RightUpperArmIndex);
	if(LeftUpperArmIndex != INDEX_NONE) CachedLeftUpperArmParentBoneIndex = RefSkel.GetParentIndex(LeftUpperArmIndex);
}

void FAnimNode_TrueFPSRig::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	BasePose.CacheBones(Context);
	ReferencePose.CacheBones(Context);
}

void FAnimNode_TrueFPSRig::GatherDebugData(FNodeDebugData& DebugData)
{
	BasePose.GatherDebugData(DebugData);
	ReferencePose.GatherDebugData(DebugData);
}


void FAnimNode_TrueFPSRig::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	BasePose.Update(Context);
	ReferencePose.Update(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);
}


bool FAnimNode_TrueFPSRig::CanEvaluate() const
{
	if(!FAnimWeight::IsRelevant(Alpha))
		return false;
	
	if(!RightHand.IsValidToEvaluate() || !LeftHand.IsValidToEvaluate() || RightLowerArmIndex == INDEX_NONE || LeftLowerArmIndex == INDEX_NONE ||
		RightUpperArmIndex == INDEX_NONE || LeftUpperArmIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("True FPS Rig: Not all arm bones are valid"));
		return false;
	}

	if(CachedRightUpperArmParentBoneIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("True FPS Rig: Right Upper Arm does not have valid parent bone"));
		return false;
	}

	if(CachedLeftUpperArmParentBoneIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("True FPS Righ: Left Upper Arm does not have valid parent bone"));
		return false;
	}

	for(const FBoneParams& BoneParam : SpineBoneParams)
	{
		if(!BoneParam.Bone.IsValidToEvaluate())
		{
			UE_LOG(LogTemp, Error, TEXT("True FPS Rig: Spine parameter %s is not valid"), *BoneParam.Bone.BoneName.ToString());
			return false;
		}
	}
			
	return true;
}

#define CS_TRANSFORM(BoneIndex) GetCSTransform(Output.Pose, BoneIndex)

void FAnimNode_TrueFPSRig::Evaluate_AnyThread(FPoseContext& Output)
{
	BasePose.Evaluate(Output);
	if(!CanEvaluate()) return;

#define CHECK_VALID_BONE(BoneIndex) if(BoneIndex == INDEX_NONE) { UE_LOG(LogTemp, Error, TEXT("Bone Index %s is invalid"), *FString(#BoneIndex)); return; }

	const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();

	const int32 RightUpperArmPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(RightUpperArmIndex)).GetInt();
	const int32 RightLowerArmPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(RightLowerArmIndex)).GetInt();
	const int32 RightHandPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(RightHand.BoneIndex)).GetInt();

	const int32 LeftUpperArmPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(LeftUpperArmIndex)).GetInt();
	const int32 LeftLowerArmPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(LeftLowerArmIndex)).GetInt();
	const int32 LeftHandPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(LeftHand.BoneIndex)).GetInt();

	const int32 HeadPoseIndex = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(Head.BoneIndex)).GetInt();

	CHECK_VALID_BONE(RightUpperArmPoseIndex);
	CHECK_VALID_BONE(RightLowerArmPoseIndex);
	CHECK_VALID_BONE(RightHandPoseIndex);
	CHECK_VALID_BONE(LeftUpperArmPoseIndex);
	CHECK_VALID_BONE(LeftLowerArmPoseIndex);
	CHECK_VALID_BONE(LeftHandPoseIndex);
	CHECK_VALID_BONE(HeadPoseIndex);

	const int32 RightUpperArmParentPoseIndex = BoneContainer.GetCompactPoseParentBoneArray()[RightUpperArmPoseIndex].GetInt();
	const int32 LeftUpperArmParentPoseIndex = BoneContainer.GetCompactPoseParentBoneArray()[LeftUpperArmPoseIndex].GetInt();

	CHECK_VALID_BONE(RightUpperArmParentPoseIndex);
	CHECK_VALID_BONE(LeftUpperArmParentPoseIndex);

#undef CHECK_VALID_BONE

#define RightLowerArmIndex RightLowerArmPoseIndex
#define RightUpperArmIndex RightUpperArmPoseIndex
#define RightHandIndex RightHandPoseIndex
#define LeftLowerArmIndex LeftLowerArmPoseIndex
#define LeftUpperArmIndex LeftUpperArmPoseIndex
#define LeftHandIndex LeftHandPoseIndex
#define HeadIndex HeadPoseIndex
	
	CameraRelativeRotation.Normalize();

	if(FMath::IsNearlyZero(ArmsAlpha) && !SpineBoneParams.IsEmpty())
	{
		FQuat Temp;
		ProceduralAimOffset(Output, Temp);
		return;
	}
	
	//
	//	Cache initial transforms
	//

	// Bone-Space bone transforms
	const TArray<FTransform>& BSBoneTransforms = *reinterpret_cast<const TArray<FTransform>*>(&Output.Pose.GetBones());

	// Apply Aiming Head Rotation Offset
	const FTransform HeadTransform = BSBoneTransforms[HeadPoseIndex];
	const FTransform HeadOffset = FTransform(AimingHeadRotationOffset, FVector::ZeroVector);
	Output.Pose[FCompactPoseBoneIndex(Head.BoneIndex)].BlendWith(HeadTransform.GetRelativeTransform(HeadOffset), AimingValue);

	// Init arm transforms. Component-space.
	const FTransform CSInitRightJointTransform = CS_TRANSFORM(RightLowerArmIndex);
	const FTransform CSInitLeftJointTransform = CS_TRANSFORM(LeftLowerArmIndex);

	const FTransform CSInitRightHandTransform = BSBoneTransforms[RightHandPoseIndex] * CSInitRightJointTransform;
	const FTransform CSInitLeftHandTransform = BSBoneTransforms[LeftHandPoseIndex] * CSInitLeftJointTransform;
	
	const FTransform CSInitCameraTransform = FTransform(FRotator(0.f, MeshYawOffset, 0.f), (FTransform(CameraRelativeLocation) * CS_TRANSFORM(HeadIndex)).GetLocation());
	FTransform CSInitWeaponTransform = OriginRelativeTransform * (bRightHanded ? CSInitRightHandTransform : CSInitLeftHandTransform);// Weapon relative transform is bone-space off of primary hand

	// Apply procedural aim offset
	FQuat AccumulativeOffsetInverse;
	if(!SpineBoneParams.IsEmpty())
	{
		ProceduralAimOffset(Output, AccumulativeOffsetInverse);
	}
	else
	{
		FPoseContext StablePose(Output.AnimInstanceProxy);
		ReferencePose.Evaluate(StablePose);
		//AccumulativeOffsetInverse = FAnimNode_ProceduralAimOffset::GetAccumulativeOffsetInverse(StableBone.BoneIndex, BoneContainer.GetReferenceSkeleton(),
		//	BSBoneTransforms, *reinterpret_cast<const TArray<FTransform>*>(&RefPose.Pose.GetBones()));
		AccumulativeOffsetInverse = FAnimNode_ProceduralAimOffset::GetAccumulativeOffsetInverse(StableBone.BoneIndex, Output.Pose, StablePose.Pose);
	}

	//
	//	Post aim offset applied
	//

	// Initialize arm transform vars
	FTransform RightUpperArmTransform = CS_TRANSFORM(RightUpperArmIndex);
	FTransform RightLowerArmTransform = BSBoneTransforms[RightLowerArmIndex] * RightUpperArmTransform;
	FTransform RightHandTransform = BSBoneTransforms[RightHandIndex] * RightLowerArmTransform;

	FTransform LeftUpperArmTransform = CS_TRANSFORM(LeftUpperArmIndex);
	FTransform LeftLowerArmTransform = BSBoneTransforms[LeftLowerArmIndex] * LeftUpperArmTransform;
	FTransform LeftHandTransform = BSBoneTransforms[LeftHandIndex] * LeftLowerArmTransform;

	//
	//	Calculate weapon transform in component-space
	//
	
	// Current component-space camera transform.
	const FTransform& CSCameraTransform = FTransform(CameraRelativeRotation + FRotator(0.f, MeshYawOffset, 0.f), (FTransform(CameraRelativeLocation) * CS_TRANSFORM(HeadIndex)).GetLocation());

	// Camera transform to weapon transform with root offset and accumulative spine offset applied.
	const FTransform& CameraToWeaponTransform = CSInitWeaponTransform.GetRelativeTransform(
		FTransform(BSBoneTransforms[0].GetRotation() * AccumulativeOffsetInverse.Inverse() * CSInitCameraTransform.GetRotation(), CSInitCameraTransform.GetLocation()));
	
	// Get current weapon transform by the camera offset
	FTransform CSWeaponTransform = CameraToWeaponTransform * CSCameraTransform;
	
	// Get aiming and non-aiming transforms
	const FTransform& AimingTransform = OriginRelativeTransform.GetRelativeTransform(SightsRelativeTransform) * CSCameraTransform;
	
	FTransform NonAimingTransform = CustomWeaponOffsetTransform * CSWeaponTransform;
	NonAimingTransform.SetRotation(FQuat::FastLerp(CSInitWeaponTransform.GetRotation(), NonAimingTransform.GetRotation(), WeaponRotationAlpha).GetNormalized());
	NonAimingTransform.SetLocation(NonAimingTransform.GetLocation() * WeaponLocationAlpha + CSInitWeaponTransform.GetLocation() * (1.f - WeaponLocationAlpha));

	CSWeaponTransform = NonAimingTransform;
	//CSWeaponTransform.NormalizeRotation();
	CSWeaponTransform.BlendWith(AimingTransform, AimingValue);
	CSWeaponTransform = OffsetTransform * CSWeaponTransform;

	// If arm pull-back is enabled...
	if(ArmPullbackConfig.Config == EArmPullbackConfig::Enabled || (ArmPullbackConfig.Config == EArmPullbackConfig::AimingValue && ArmPullbackConfig.ArmPullbackThreshold > AimingValue))
	{
		// Get the max reach and current reach amount on the non-dominant arm.
		// Then we will pull the weapon back by the reach over the threshold amount.
		float ReachDist;
		float MaxReach;
		if(bRightHanded)
		{// Calculate the projected hand transform
			ReachDist = (LeftUpperArmTransform.GetLocation() - (CSInitLeftHandTransform.GetRelativeTransform(CSInitWeaponTransform) * CSWeaponTransform).GetLocation()).Size();
			MaxReach = BSBoneTransforms[LeftLowerArmIndex].GetLocation().Size() + BSBoneTransforms[LeftHandIndex].GetLocation().Size();
		}
		else
		{
			ReachDist = (RightUpperArmTransform.GetLocation() - (CSInitRightHandTransform.GetRelativeTransform(CSInitWeaponTransform) * CSWeaponTransform).GetLocation()).Size();
			MaxReach = BSBoneTransforms[RightLowerArmIndex].GetLocation().Size() + BSBoneTransforms[RightHandIndex].GetLocation().Size();
		}

		// Calculate the distance to pull the weapon back
		const float PullbackDist = MaxReach * MaxExtension - ReachDist;
		if(PullbackDist < 0)// Pull back directly towards camera
		{
			const FTransform PullbackTransform((CSWeaponTransform.GetLocation() - CSCameraTransform.GetLocation()).GetSafeNormal() * PullbackDist);
			CSWeaponTransform *= PullbackTransform;
			if(!FMath::IsNearlyZero(AimingValue))// Offset non-aiming transform as well to calculate joint transform influence later.
				NonAimingTransform *= PullbackTransform;
		}
	}
	
	
	FTransform JointInfluenceWeaponTransform = CSWeaponTransform;
	if(!FMath::IsNearlyZero(AimingJointInfluence))// Blend joint influence transform by non-aiming transform by blend amount
		JointInfluenceWeaponTransform.BlendWith(NonAimingTransform, 1.f - AimingJointInfluence);

	//
	// Right Arm IK
	//

	// The new right hand transform in component-space
	const FTransform& RightEffectorTransform = CSInitRightHandTransform.GetRelativeTransform(CSInitWeaponTransform) * (RightHandAdditiveTransform * CSWeaponTransform);

	// Removing the custom weapon offset from the weapon transform to get the original joint location without an offset then applying whatever offset specified
	/*const FVector& RightJointLocation = (CSInitRightJointTransform.GetRelativeTransform(CSInitWeaponTransform) *
		FTransform(RightJointLocationOffset) * CustomWeaponOffsetTransform.Inverse() * (RightHandAdditiveTransform * JointInfluenceWeaponTransform)).GetLocation();*/

	FTransform RightJointAdditiveTransform;
	RightJointAdditiveTransform.BlendWith(RightHandAdditiveTransform, RightHandAdditiveJointInfluence);

	// The target transform for the right joint in component-space
	const FTransform RightJointTargetTransform = RightJointAdditiveTransform * CSInitRightJointTransform.GetRelativeTransform(CSInitWeaponTransform) *
		FTransform(RightJointLocationOffset) * CustomWeaponOffsetTransform.Inverse() * JointInfluenceWeaponTransform;

	// Right joint location in component-space
	FVector RightJointLocation;
	if(RightJointClamp.IsClamping() && !FMath::IsNearlyZero(ArmsJointAlpha))
	{
		// The joint transform without any additives applied
		const FTransform JointNoAdditiveTransform = RightJointAdditiveTransform * CSInitRightJointTransform.GetRelativeTransform(CSInitWeaponTransform) *
			FTransform(RightJointLocationOffset) * CameraToWeaponTransform * CSCameraTransform;// CameraToWeaponTransform * CSCameraTransform gets the weapon transform without additives

		FTransform Offset = RightJointTargetTransform.GetRelativeTransform(JointNoAdditiveTransform);
		const FQuat Orientation = CSInitRightJointTransform.GetRotation();
		FVector OrientationOffsetLocation = Orientation.UnrotateVector(Offset.GetLocation());

		// Clamp the orientation location offset between the ranges
		ClampRange(OrientationOffsetLocation.Y, RightJointClamp.HorizontalRange);
		ClampRange(OrientationOffsetLocation.Z, RightJointClamp.VerticalRange);
		Offset.SetLocation(Orientation.RotateVector(OrientationOffsetLocation));
		
		// Set joint location by the clamped joint location blended by the target joint location
		RightJointLocation = (Offset * JointNoAdditiveTransform).GetLocation() * ArmsJointAlpha + RightJointTargetTransform.GetLocation() * (1.f - ArmsJointAlpha);
	}
	else
	{// Set to target location if no clamping is needed
		RightJointLocation = RightJointTargetTransform.GetLocation();
	}
	
	AnimationCore::SolveTwoBoneIK(RightUpperArmTransform, RightLowerArmTransform, RightHandTransform,
		RightJointLocation, RightEffectorTransform.GetLocation(), false, 0.f, 0.f);

	RightHandTransform.SetRotation(RightEffectorTransform.GetRotation());

	//
	// Left Arm IK
	//

	// The new left hand transform in component-space
	const FTransform LeftEffectorTransform = LeftHandAdditiveTransform * (CSInitLeftHandTransform.GetRelativeTransform(CSInitWeaponTransform) * CSWeaponTransform);

	FTransform LeftJointAdditiveTransform;
	LeftJointAdditiveTransform.BlendWith(LeftHandAdditiveTransform, LeftHandAdditiveJointInfluence);

	// Target transform for the left joint in component-space
	const FTransform LeftJointTargetTransform = LeftJointAdditiveTransform * CSInitLeftJointTransform.GetRelativeTransform(CSInitWeaponTransform) *
			FTransform(LeftJointLocationOffset) * CustomWeaponOffsetTransform.Inverse() * JointInfluenceWeaponTransform;

	// Left joint location in component-space
	FVector LeftJointLocation;
	if(LeftJointClamp.IsClamping() && !FMath::IsNearlyZero(ArmsJointAlpha))
	{
		// The joint transform without any additives applied
		const FTransform JointNoAdditiveTransform = LeftJointAdditiveTransform * CSInitLeftJointTransform.GetRelativeTransform(CSInitWeaponTransform) *
			FTransform(LeftJointLocationOffset) * CameraToWeaponTransform * CSCameraTransform;// CameraToWeaponTransform * CSCameraTransform gets the weapon transform without additives

		FTransform Offset = LeftJointTargetTransform.GetRelativeTransform(JointNoAdditiveTransform);
		const FQuat Orientation = CSInitLeftJointTransform.GetRotation();
		FVector OrientationOffsetLocation = Orientation.UnrotateVector(Offset.GetLocation());

		// Clamp the orientation location offset between the ranges
		ClampRange(OrientationOffsetLocation.Y, LeftJointClamp.HorizontalRange);
		ClampRange(OrientationOffsetLocation.Z, LeftJointClamp.VerticalRange);
		Offset.SetLocation(Orientation.RotateVector(OrientationOffsetLocation));

		// Set joint location by the clamped joint location blended by the target joint location
		LeftJointLocation = (Offset * JointNoAdditiveTransform).GetLocation() * ArmsJointAlpha + LeftJointTargetTransform.GetLocation() * (1.f - ArmsJointAlpha);
	}
	else
	{// Set to target location if no clamping is needed
		LeftJointLocation = LeftJointTargetTransform.GetLocation();
	}
	
	
	AnimationCore::SolveTwoBoneIK(LeftUpperArmTransform, LeftLowerArmTransform, LeftHandTransform,
		LeftJointLocation, LeftEffectorTransform.GetLocation(), false, 0.f, 0.f);

	LeftHandTransform.SetRotation(LeftEffectorTransform.GetRotation());
	
	//
	// Apply IKs
	//
	
	const float TotalArmsAlpha = Alpha * ArmsAlpha;
	Output.Pose[FCompactPoseBoneIndex(RightUpperArmIndex)].BlendWith(RightUpperArmTransform.GetRelativeTransform(CS_TRANSFORM(RightUpperArmParentPoseIndex)), TotalArmsAlpha);
	Output.Pose[FCompactPoseBoneIndex(RightLowerArmIndex)].BlendWith(RightLowerArmTransform.GetRelativeTransform(RightUpperArmTransform), TotalArmsAlpha);
	Output.Pose[FCompactPoseBoneIndex(RightHandIndex)].BlendWith(RightHandTransform.GetRelativeTransform(RightLowerArmTransform), TotalArmsAlpha);

	Output.Pose[FCompactPoseBoneIndex(LeftUpperArmIndex)].BlendWith(LeftUpperArmTransform.GetRelativeTransform(CS_TRANSFORM(LeftUpperArmParentPoseIndex)), TotalArmsAlpha);
	Output.Pose[FCompactPoseBoneIndex(LeftLowerArmIndex)].BlendWith(LeftLowerArmTransform.GetRelativeTransform(LeftUpperArmTransform), TotalArmsAlpha);
	Output.Pose[FCompactPoseBoneIndex(LeftHandIndex)].BlendWith(LeftHandTransform.GetRelativeTransform(LeftLowerArmTransform), TotalArmsAlpha);
}

#undef RightUpperArmIndex
#undef RightLowerArmIndex
#undef RightHandIndex
#undef LeftUpperArmIndex
#undef LeftLowerArmIndex
#undef LeftHandIndex

/*
 *
 *	Procedural Aim Offset
 *
 */

void FAnimNode_TrueFPSRig::ProceduralAimOffset(FPoseContext& Output, FQuat& AccumulativeOffsetInverse)
{
	if(FMath::IsNearlyZero(SpineAlpha)) return;

	FPoseContext StablePose(Output.AnimInstanceProxy);
	ReferencePose.Evaluate(StablePose);

	const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();
	const FReferenceSkeleton& RefSkel = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
	const TArray<FTransform>& RefBoneTransforms = *reinterpret_cast<const TArray<FTransform>*>(&StablePose.Pose.GetBones());
	const TArray<FTransform>& CurrBoneTransforms = (TArray<FTransform>&)Output.Pose.GetBones();
	
	//AccumulativeOffsetInverse = FAnimNode_ProceduralAimOffset::GetAccumulativeOffsetInverse(StableBone.BoneIndex, RefSkel, CurrBoneTransforms, RefBoneTransforms);
	AccumulativeOffsetInverse = FAnimNode_ProceduralAimOffset::GetAccumulativeOffsetInverse(StableBone.BoneIndex, Output.Pose, StablePose.Pose);

	// Reverse twisting if exceeds 180 degrees
	if(abs(AccumulativeOffsetInverse.GetAngle()) > PI)
		AccumulativeOffsetInverse *= FQuat(AccumulativeOffsetInverse.Vector(), -2 * PI);

	// Cache spine offset inverses to be applied later so that
	// the references to not get modified in the process of application
	TArray<FQuat, TInlineAllocator<12>> SpineOffsetInverses;
	SpineOffsetInverses.Reserve(SpineBoneParams.Num());
	for(const FBoneParams& BoneParams : SpineBoneParams)
	{
		const int32 i = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(BoneParams.Bone.BoneIndex)).GetInt();
		FQuat OffsetInverse = CurrBoneTransforms[i].GetRotation() * (CS_TRANSFORM(i).GetRotation().Inverse() * CurrBoneTransforms[0].GetRotation());
		SpineOffsetInverses.Add(MoveTemp(OffsetInverse));
	}
	
	
	// Copy rotation as to not change variable value
	// and have weird stuff happen with static values
	FRotator TargetRot = CameraRelativeRotation;
	TargetRot += CurrBoneTransforms[0].Rotator().GetInverse();// Rotate by root offset
	TargetRot *= SpineAlpha;// Apply the spine-bending alpha to the camera rotation
	TargetRot.Normalize();// Normalize to prevent weird behaviour

	// Apply compounding camera axes in the order for the desired outcome
	FQuat OrientationCameraRot(FVector::ForwardVector, 0.f);
	OrientationCameraRot *= FQuat(-OrientationCameraRot.GetRightVector(), FMath::DegreesToRadians(TargetRot.Roll));// Apply Roll
	OrientationCameraRot *= FQuat(OrientationCameraRot.GetUpVector().RotateAngleAxis(TargetRot.Roll * 2.f, FVector::RightVector), FMath::DegreesToRadians(TargetRot.Yaw));// Apply Yaw
	OrientationCameraRot *= FQuat(OrientationCameraRot.GetForwardVector().RotateAngleAxis(TargetRot.Yaw, -FVector::UpVector).RotateAngleAxis(TargetRot.Roll * 2.f, FVector::RightVector), FMath::DegreesToRadians(TargetRot.Pitch));// Apply Pitch, rotated about target yaw
	OrientationCameraRot *= AccumulativeOffsetInverse;// Apply accumulative offset inverse to correct for any additives to the Reference Pose

	// For each bone
	for(int32 i = 0; i < SpineBoneParams.Num(); i++)
	{// Reference to current spine bone transform to modify
		const FCompactPoseBoneIndex SpineIndex = BoneContainer.GetCompactPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(SpineBoneParams[i].Bone.BoneIndex));
		FTransform& SpineTransform = Output.Pose[SpineIndex];
		
		// Convert the camera rotation to axis and angle to modify axis of rotation
		FVector Axis; float Angle;
		OrientationCameraRot.ToAxisAndAngle(Axis, Angle);

		// Apply blend-weight to the spine angle
		Angle *= SpineBoneParams[i].Weight;
		
		// Rotate axis by the inverse of the spine bone-space transform offset from the local rotation.
		Axis = SpineOffsetInverses[i].RotateVector(Axis);

		// Apply offset to spine, apply alpha as lerp between base and base + offset
		const FQuat& SpineRot = SpineTransform.GetRotation();
		const FQuat& TargetSpineRot = FQuat(Axis, Angle) * SpineRot;
		SpineTransform.SetRotation(FQuat::FastLerp(SpineRot, TargetSpineRot, Alpha).GetNormalized());
	}
}

#undef CS_TRANSFORM