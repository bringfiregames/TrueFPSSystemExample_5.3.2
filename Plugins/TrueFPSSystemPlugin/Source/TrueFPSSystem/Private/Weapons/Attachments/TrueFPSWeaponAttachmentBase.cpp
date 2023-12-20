// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Attachments/TrueFPSWeaponAttachmentBase.h"

#include "Weapons/TrueFPSWeaponBase.h"
#include "GameFramework/Character.h"
#include "Weapons/Attachments/TrueFPSWeaponAttachmentPoint.h"

ATrueFPSWeaponAttachmentBase::ATrueFPSWeaponAttachmentBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ATrueFPSWeaponAttachmentBase::Destroyed()
{
	Internal_OnRemoved();
	Super::Destroyed();
}

ATrueFPSWeaponAttachmentBase* ATrueFPSWeaponAttachmentBase::SpawnAttachment(const TSubclassOf<ATrueFPSWeaponAttachmentBase>& AttachmentClass, UTrueFPSWeaponAttachmentPoint* AttachmentPoint)
{
	if (!AttachmentClass || !AttachmentPoint) return nullptr;
	
	FActorSpawnParameters Params;
	Params.Owner = AttachmentPoint->GetOwner();
	ATrueFPSWeaponAttachmentBase* NewAttachment = AttachmentPoint->GetWorld()->SpawnActor<ATrueFPSWeaponAttachmentBase>(AttachmentClass, Params);
	AttachmentPoint->SetAttachment(NewAttachment);

	return NewAttachment;
}

ATrueFPSWeaponAttachmentBase* ATrueFPSWeaponAttachmentBase::SpawnAttachmentUnattached(const UObject* WorldContextObject, const TSubclassOf<ATrueFPSWeaponAttachmentBase>& AttachmentClass, const FTransform& WorldTransform)
{
	if (WorldContextObject && AttachmentClass)
		if (UWorld* World = WorldContextObject->GetWorld())
			return World->SpawnActor<ATrueFPSWeaponAttachmentBase>(AttachmentClass, WorldTransform);
	
	return nullptr;
}

void ATrueFPSWeaponAttachmentBase::OnAttached()
{
	if (!OwningWeapon || !OwningWeapon->GetPawnOwner() || (OwningWeapon && !OwningWeapon->IsEquipped()))
		SetVisibility(false);

	if (OwningWeapon)
	{
		// Bind functions from owning weapon delegates
		// OwningWeapon->ObtainedDelegate.AddDynamic(this, &ThisClass::Internal_OnObtained);
		// OwningWeapon->UnobtainedDelegate.AddDynamic(this, &ThisClass::Internal_OnUnobtained);
		// OwningWeapon->EquippedDelegate.AddDynamic(this, &ThisClass::Internal_OnEquipped);
		// OwningWeapon->EquippedDelegate.AddDynamic(this, &ThisClass::Internal_OnUnequipped);

		// Call obtained if owning weapon is currently obtained on attached.
		if (IsValid(OwningWeapon->GetPawnOwner()))
			OnObtained(OwningWeapon->GetPawnOwner());

		// Call equipped if owning weapon is currently being equipped by character inventory.
		/*if(const UInventoryComponent* OwningInventory = OwningWeapon->GetOwningInventory())
			if(OwningInventory->GetCurrentWeapon() == OwningWeapon)
				OnEquipped();*/
		
		if (OwningWeapon->IsEquipped())
			OnEquipped(OwningWeapon->GetPawnOwner());
	}
}

void ATrueFPSWeaponAttachmentBase::OnRemoved()
{
	if (OwningWeapon)
	{
		// OwningWeapon->ObtainedDelegate.RemoveAll(this);
		// OwningWeapon->UnobtainedDelegate.RemoveAll(this);
	}
}

void ATrueFPSWeaponAttachmentBase::SetVisibility(const bool bVisible)
{ // Set all owning prim comps to bVisible
	TArray<UPrimitiveComponent*> Prims;
	GetComponents<UPrimitiveComponent>(Prims);
	for (UPrimitiveComponent* Prim : Prims)
		Prim->SetVisibility(bVisible);
}
