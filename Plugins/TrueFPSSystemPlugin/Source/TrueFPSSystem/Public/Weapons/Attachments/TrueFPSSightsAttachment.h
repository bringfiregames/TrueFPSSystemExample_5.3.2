// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TrueFPSWeaponAttachmentBase.h"
#include "Weapons/Attachments/TrueFPSWeaponAttachmentPoint.h"
#include "TrueFPSSightsAttachment.generated.h"

UCLASS(Abstract, Meta = (DisplayName = "Sights Attachment"))
class TRUEFPSSYSTEM_API ATrueFPSSightsAttachment : public ATrueFPSWeaponAttachmentBase
{
	GENERATED_BODY()

public:
	
	ATrueFPSSightsAttachment();

protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess = "true"), Category = "Components")
	class UStaticMeshComponent* Sights;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess = "true"), Category = "Components")
	class UTrueFPSVisualizationComponent* SightsPoint;

public:
	
	FORCEINLINE class UStaticMeshComponent* GetSights() const { return Sights; }
	FORCEINLINE class UTrueFPSVisualizationComponent* GetSightsPoint() const { return SightsPoint; }
	
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Sights")
	FTransform GetSightsWorldTransform() const;

	// The distance applied on-top of the Get Sights World Transform
	// to increase the distance from the camera to the sights when aiming.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sights")
	float AimOffset = 0.f;
	
};

// Derived from Weapon Attachment Point. All it does is set the sights attachment
// to the assigned aim offset value on-spawned
UCLASS(ClassGroup = (Custom), Meta = (BlueprintSpawnableComponent))
class TRUEFPSSYSTEM_API UTrueFPSSightsAttachmentPoint : public UTrueFPSWeaponAttachmentPoint
{
	GENERATED_BODY()

public:
	
	// Sets the sights Aim Offset variable to this when spawned
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configurations")
	float AimOffset = 0.f;

	virtual FORCEINLINE void OnRep_Attachment() override
	{
		Super::OnRep_Attachment();
		if (ATrueFPSSightsAttachment* Sights = Cast<ATrueFPSSightsAttachment>(Attachment))
			Sights->AimOffset = AimOffset;
	}

protected:

#if WITH_EDITORONLY_DATA
	class UStaticMeshComponent* AimOffsetComp = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Meta = (DisplayName = "Show Aim Offset Visualization Mesh"), Category = "Configurations|Visualizer")
	bool bShowAimOffset = false;
	
	UPROPERTY(EditDefaultsOnly, Meta = (DisplayName = "Aim Offset Visualization Mesh", EditCondition = bShowAimOffset), Category = "Configurations|Visualizer")
	class UStaticMesh* AimOffsetMesh;
	
	virtual void OnRegister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
#endif // WITH_EDITORONLY_DATA
	
};
