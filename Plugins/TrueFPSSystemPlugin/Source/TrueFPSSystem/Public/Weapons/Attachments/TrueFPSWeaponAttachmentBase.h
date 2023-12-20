// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrueFPSWeaponAttachmentBase.generated.h"

UCLASS()
class TRUEFPSSYSTEM_API ATrueFPSWeaponAttachmentBase : public AActor
{
	GENERATED_BODY()
public:
	
	ATrueFPSWeaponAttachmentBase();
	
	friend class UTrueFPSWeaponAttachmentPoint;

protected:
	
	virtual void Destroyed() override;

	// The weapon that owns this attachment
	UPROPERTY(BlueprintReadOnly, Meta = (AllowPrivateAccess = "true"), Category = "Attachment")
	class ATrueFPSWeaponBase* OwningWeapon;

	// Spawns weapon attachment and attaches it to the attachment point. Only call on server. If set attachment point that will be where the weapon is spawned
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Meta = (AutoCreateRefTerm = "AttachmentClass", DeterminesOutputType = "AttachmentClass", DefaultToSelf = "AttachmentPoint"), Category = "Attachment")
	static class ATrueFPSWeaponAttachmentBase* SpawnAttachment(const TSubclassOf<class ATrueFPSWeaponAttachmentBase>& AttachmentClass, class UTrueFPSWeaponAttachmentPoint* AttachmentPoint);

	// Does nothing but spawn the weapon attachment
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Meta = (AutoCreateRefTerm = "AttachmentClass, WorldTransform", DeterminesOutputType = "AttachmentClass", WorldContextObject = "WorldContextObject", HidePin = "WorldContextObject"), Category = "Attachment")
	static class ATrueFPSWeaponAttachmentBase* SpawnAttachmentUnattached(const class UObject* WorldContextObject, const TSubclassOf<class ATrueFPSWeaponAttachmentBase>& AttachmentClass, const FTransform& WorldTransform);

	template<typename T>
	static FORCEINLINE T* SpawnAttachment(const TSubclassOf<class ATrueFPSWeaponAttachmentBase>& AttachmentClass, class UTrueFPSWeaponAttachmentPoint* AttachmentPoint) {
		return Cast<T>(SpawnAttachment(AttachmentClass, AttachmentPoint));
	}

	template<typename T>
	static FORCEINLINE T* SpawnAttachment(const class UObject* WorldContextObject, const TSubclassOf<class ATrueFPSWeaponAttachmentBase>& AttachmentClass, const FTransform& WorldTransform = FTransform::Identity) {
		return Cast<T>(SpawnAttachmentUnattached(WorldContextObject, AttachmentClass, WorldTransform));
	}
	
	// Equivalent to BeginPlay but is called after replication when the OwningWeapon is valid.
	// OwningWeapon should always be valid unless the Weapon Attachment Point was placed on an
	// actor that does not eventually have it's owner derive from WeaponBase (which you should never do).
	virtual void OnAttached();
	
	UFUNCTION(BlueprintImplementableEvent, Meta = (DisplayName = "On Attached"), Category = "Attachment")
	void BP_OnAttached();

	// Equivalent to OnDestroy but is called right beforehand
	virtual void OnRemoved();
	UFUNCTION(BlueprintImplementableEvent, Meta = (DisplayName = "On Removed"), Category = "Attachment")
	void BP_OnRemoved();
	
	virtual FORCEINLINE void OnEquipped(class ACharacter* Character) {}
	UFUNCTION(BlueprintImplementableEvent, Meta = (DisplayName = "On Equipped"), Category = "Attachment")
	void BP_OnEquipped(class ACharacter* Character);
	
	virtual FORCEINLINE void OnUnequipped(class ACharacter* Character) {}
	UFUNCTION(BlueprintImplementableEvent, Meta = (DisplayName = "On Unequipped"), Category = "Attachment")
	void BP_OnUnequipped(class ACharacter* Character);

	// Called whenever the owning inventory of the owning weapon changes to a valid inventory
	virtual FORCEINLINE void OnObtained(class ACharacter* Character) {}
	UFUNCTION(BlueprintImplementableEvent, Meta = (DisplayName = "On Obtained"), Category = "Attachment")
	void BP_OnObtained(class ACharacter* Character);

	// Called whenever the owning inventory of the owning weapon becomes invalid. Also called on destroyed
	virtual FORCEINLINE void OnUnobtained(class ACharacter* Character) {}
	UFUNCTION(BlueprintImplementableEvent, Meta = (DisplayName = "On Unobtained"), Category = "Attachment")
	void BP_OnUnobtained(class ACharacter* Character);

public:
	
	UFUNCTION(BlueprintCallable, Category = "Attachment")
	void SetVisibility(const bool bVisible = false);

	template<typename T>
	FORCEINLINE T* GetOwningWeapon() const { return Cast<T>(OwningWeapon); }

private:
	
	static FORCEINLINE class UWorld* StaticGetWorld()
	{
		if(GEngine && !GEngine->GetWorldContexts().IsEmpty())
			return GEngine->GetWorldContexts()[0].World();
		return nullptr;
	}
	
	// Sets OwningWeapon reference then calls OnAttached()
	FORCEINLINE void Internal_OnAttached(class ATrueFPSWeaponBase* InOwningWeapon)
	{
		OwningWeapon = InOwningWeapon;
		OnAttached();
		BP_OnAttached();
	}
	
	FORCEINLINE void Internal_OnRemoved()
	{
		OnRemoved();
		BP_OnRemoved();
		OwningWeapon = nullptr;
	}

	UFUNCTION() FORCEINLINE void Internal_OnObtained(class ATrueFPSWeaponBase* W, class ACharacter* Character) { OnObtained(Character); BP_OnObtained(Character); }
	UFUNCTION() FORCEINLINE void Internal_OnUnobtained(class ATrueFPSWeaponBase* W, class ACharacter* Character) { OnUnobtained(Character); BP_OnUnobtained(Character); }
	UFUNCTION() FORCEINLINE void Internal_OnEquipped(class ATrueFPSWeaponBase* W, class ACharacter* Character) { OnEquipped(Character); BP_OnEquipped(Character); }
	UFUNCTION() FORCEINLINE void Internal_OnUnequipped(class ATrueFPSWeaponBase* W, class ACharacter* Character) { OnUnequipped(Character); BP_OnUnequipped(Character); }
	
};
