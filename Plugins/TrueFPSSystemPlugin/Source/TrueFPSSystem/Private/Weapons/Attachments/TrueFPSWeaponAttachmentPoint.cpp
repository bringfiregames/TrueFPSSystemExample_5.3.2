// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Attachments/TrueFPSWeaponAttachmentPoint.h"

#include "Weapons/TrueFPSWeaponBase.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/Attachments/TrueFPSWeaponAttachmentBase.h"

UTrueFPSWeaponAttachmentPoint::UTrueFPSWeaponAttachmentPoint()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UTrueFPSWeaponAttachmentPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && DefaultAttachment)
	{
		const auto& SpawnDefaultAttachment = [this]()->void
		{
			/*FActorSpawnParameters Params;
			Params.Owner = GetOwner();
			Attachment = GetWorld()->SpawnActor<AWeaponAttachmentBase>(DefaultAttachment, Params);
			if(Attachment) OnRep_Attachment();*/
			ATrueFPSWeaponAttachmentBase::SpawnAttachment(DefaultAttachment, this);
		};

		// Add a small delay to spawning the attachment so the OwningWeapon is valid
		GetWorld()->GetTimerManager().SetTimer(DefaultAttachmentDelay, SpawnDefaultAttachment, 0.1f, false);
	}
}

void UTrueFPSWeaponAttachmentPoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Attachment);
}

void UTrueFPSWeaponAttachmentPoint::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	if (Attachment) Attachment->Destroy();
}

#if WITH_EDITORONLY_DATA
void UTrueFPSWeaponAttachmentPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, DefaultAttachment))
	{
		if (DefaultAttachment)
		{
			const auto* DefObj = DefaultAttachment->GetDefaultObject<ATrueFPSWeaponAttachmentBase>();
			if(const auto* StaticMeshRoot = Cast<UStaticMeshComponent>(DefObj->GetRootComponent()))
			{
				if(!VisualizationSkeletalMesh)
					SetDefaultStaticMesh(StaticMeshRoot->GetStaticMesh());
			}
			else if(const auto* SkelMeshRoot = Cast<USkeletalMeshComponent>(DefObj->GetRootComponent()))
			{
				if(!VisualizationStaticMesh)
					SetDefaultSkeletalMesh(SkelMeshRoot->SkeletalMesh);
			}
		}
		else
		{
			SetDefaultStaticMesh(nullptr);
			SetDefaultSkeletalMesh(nullptr);
		}
	}
	
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITORONLY_DATA

void UTrueFPSWeaponAttachmentPoint::OnRep_Attachment()
{
	if (Attachment)
	{
		Attachment->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale, NAME_None);

		// Search through the linked-list of owners
		// to find the owning weapon base actor
		AActor* Owner = GetOwner();
		while (Owner)
		{
			if (ATrueFPSWeaponBase* OwningWeapon = Cast<ATrueFPSWeaponBase>(Owner))
			{
				Attachment->Internal_OnAttached(OwningWeapon);
				break;
			}

			Owner = Owner->GetOwner();
		}

		if (!Owner) Attachment->Internal_OnAttached(nullptr);
	}

	// Call blueprint event
	AttachmentChanged();
}

ATrueFPSWeaponAttachmentBase* UTrueFPSWeaponAttachmentPoint::SpawnAttachment(const TSubclassOf<ATrueFPSWeaponAttachmentBase>& AttachmentClass)
{
	if (!HasAuthority() || !AttachmentClass || IsValid(Attachment)) return nullptr;
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Attachment = GetWorld()->SpawnActor<ATrueFPSWeaponAttachmentBase>(AttachmentClass, Params);
	OnRep_Attachment();
	return Attachment;
}

void UTrueFPSWeaponAttachmentPoint::DestroyAttachment()
{
	if (!HasAuthority() || !Attachment) return;
	Attachment->Destroy();
	Attachment = nullptr;
	OnRep_Attachment();
}

// UUserWidget* UTrueFPSWeaponAttachmentPoint::GetAttachedWidget() const
// {
// 	for (const USceneComponent* Child : GetAttachChildren())
// 		if (const UWidgetComponent* Widget = Cast<UWidgetComponent>(Child))
// 			return Widget->GetWidget();
// 	return nullptr;
// }
