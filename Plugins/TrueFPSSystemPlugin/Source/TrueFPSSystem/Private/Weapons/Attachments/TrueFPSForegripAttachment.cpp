// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Attachments/TrueFPSForegripAttachment.h"

#include "GameFramework/Character.h"
#include "Library/TrueFPSFunctionLibrary.h"
#include "Weapons/TrueFPSWeaponBase.h"
#include "Weapons/Attachments/TrueFPSPoseableHandComponent.h"

ATrueFPSForegripAttachment::ATrueFPSForegripAttachment()
{
	PrimaryActorTick.bCanEverTick = false;

	Foregrip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Foregrip"));
	Foregrip->SetCastShadow(false);
	Foregrip->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	Foregrip->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	RootComponent = Foregrip;

	HandAttachPoint = CreateDefaultSubobject<UTrueFPSPoseableHandComponent>(TEXT("Off-Hand Attachment Point"));
	HandAttachPoint->SetupAttachment(Foregrip);
}

void ATrueFPSForegripAttachment::OnRemoved()
{
	if(!OwningWeapon) return;

	// Remove hand offset transform and remove hand cached pose
	OwningWeapon->GetState().OffHandAdditiveTransform = FTransform::Identity;
	if(bCachePose && bRemoveCache)
		if(const ACharacter* Character = OwningWeapon->GetPawnOwner())
			UTrueFPSFunctionLibrary::RemoveCachedPoseSnapshot(Character->GetMesh()->GetAnimInstance(), CachedPoseName);
}

void ATrueFPSForegripAttachment::SetHandPlacement() const
{
	// Set weapon off-hand offset transform to the hand attachment point transform
	// relative to the weapon's reference animation's off-hand world-transform
	// (not just the off-hand world-transform because it could already be offset when
	// calculating an offset and thus be incorrect. This operates under the assumption that
	// the dominant-hand to weapon transform remains constant).
	
	ATrueFPSWeaponBase* Weapon = Cast<ATrueFPSWeaponBase>(OwningWeapon);
	if (!Weapon) return;
	
	const ACharacter* Character = Weapon->GetPawnOwner();
	if (!Character || !Character->GetMesh()->GetAnimInstance() || !Weapon->GetSettings()->AnimPose) return;
	
	FCompactPose OutPose;
	FBlendedCurve OutCurve;
#if ENGINE_MAJOR_VERSION < 5
	FStackCustomAttributes OutAttr;
#else//ENGINE_MAJOR_VERSION
	UE::Anim::FStackAttributeContainer OutAttr;
#endif//ENGINE_MAJOR_VERSION
	OutPose.SetBoneContainer(&Character->GetMesh()->GetAnimInstance()->GetRequiredBones());
	FAnimationPoseData OutData(OutPose, OutCurve, OutAttr);

	if (Weapon->GetSettings()->AnimPose)
	{
		Weapon->GetSettings()->AnimPose->GetAnimationPose(OutData, FAnimExtractContext(0.f, false));
	}
	else
	{
		OutData.GetPose().ResetToRefPose();
	}

	const FReferenceSkeleton& RefSkel = Weapon->GetSettings()->AnimPose->GetSkeleton()->GetReferenceSkeleton();
	const TArray<FTransform>& BSBoneTransforms = *reinterpret_cast<const TArray<FTransform>*>(&OutPose.GetBones());
		
	const int32 OffHandIndex = RefSkel.FindBoneIndex(OffHandName);
	const int32 DomHandIndex = RefSkel.FindBoneIndex(DomHandName);
	if (OffHandIndex == INDEX_NONE || DomHandIndex == INDEX_NONE) return;

	const FTransform& OffHandWorldTransform = FAnimationRuntime::GetComponentSpaceTransform(RefSkel, BSBoneTransforms, OffHandIndex);
	const FTransform& DomHandWorldTransform = FAnimationRuntime::GetComponentSpaceTransform(RefSkel, BSBoneTransforms, DomHandIndex);

	const FTransform& DomHandToOffHandTransform = OffHandWorldTransform.GetRelativeTransform(DomHandWorldTransform);
	const FTransform& OffHandRefWorldTransform = DomHandToOffHandTransform * Weapon->GetPawnOwner()->GetMesh()->GetSocketTransform(DomHandName);
		
	// const FTransform& ForegripOffset = HandAttachPoint->GetComponentTransform().GetRelativeTransform(OffHandRefWorldTransform);
	// const FTransform& ForegripOffset = HandAttachPoint->GetComponentTransform().GetRelativeTransform(OffHandWorldTransform * Character->GetMesh()->GetComponentTransform());

	const FTransform& WeaponSocketParentOffsetTransform = Character->GetMesh()->GetSocketTransform(WeaponSocketName.IsValid() ? WeaponSocketName : DomHandName, RTS_ParentBoneSpace);
	const FTransform& RefWeaponTransform = Weapon->GetSettings()->PlacementTransform * WeaponSocketParentOffsetTransform * DomHandWorldTransform;
	const FTransform& ForegripOffset = (HandAttachPoint->GetComponentTransform().GetRelativeTransform(Weapon->GetActorTransform()) * RefWeaponTransform).GetRelativeTransform(OffHandWorldTransform);
		
	Weapon->GetState().OffHandAdditiveTransform = FTransform(ForegripOffset.GetRotation() * RotOffset.Quaternion(), ForegripOffset.GetLocation(), ForegripOffset.GetScale3D());
	
	if (bCachePose)
	{ // Cache hand attachment pose for use in animation
		const FPoseSnapshot& CachedPoseSnapshot = HandAttachPoint->GetCachedPoseSnapshot();
		UTrueFPSFunctionLibrary::CachePoseSnapshot(CachedPoseSnapshot, Character->GetMesh()->GetAnimInstance(), CachedPoseName);
	}
}
