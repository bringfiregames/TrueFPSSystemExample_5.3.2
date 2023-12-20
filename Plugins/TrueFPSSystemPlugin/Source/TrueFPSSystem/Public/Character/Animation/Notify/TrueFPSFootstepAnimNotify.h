// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TrueFPSTypes.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TrueFPSFootstepAnimNotify.generated.h"

UENUM(BlueprintType)
enum class EFootstepType : uint8
{
	Step,
	WalkRun,
	Jump,
	Land
};

UENUM(BlueprintType)
enum class ESpawnType : uint8
{
	Location,
	Attached
};

USTRUCT(BlueprintType)
struct FFootstepFX : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Surface")
	TEnumAsByte<enum EPhysicalSurface> SurfaceType;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, Category = "Sound")
	ESpawnType SoundSpawnType;

	UPROPERTY(EditAnywhere, Category = "Sound", meta = (EditCondition = "SoundSpawnType == ETSSpawnType::Attached"))
	TEnumAsByte<enum EAttachLocation::Type> SoundAttachmentType;

	UPROPERTY(EditAnywhere, Category = "Sound")
	FVector SoundLocationOffset;

	UPROPERTY(EditAnywhere, Category = "Sound")
	FRotator SoundRotationOffset;

	UPROPERTY(EditAnywhere, Category = "Decal")
	TSoftObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, Category = "Decal")
	ESpawnType DecalSpawnType;

	UPROPERTY(EditAnywhere, Category = "Decal", meta = (EditCondition = "DecalSpawnType == ETSSpawnType::Attached"))
	TEnumAsByte<enum EAttachLocation::Type> DecalAttachmentType;

	UPROPERTY(EditAnywhere, Category = "Decal")
	float DecalLifeSpan = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Decal")
	FVector DecalSize;

	UPROPERTY(EditAnywhere, Category = "Decal")
	FVector DecalLocationOffset;

	UPROPERTY(EditAnywhere, Category = "Decal")
	FRotator DecalRotationOffset;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	ESpawnType NiagaraSpawnType;

	UPROPERTY(EditAnywhere, Category = "Niagara", meta = (EditCondition = "NiagaraSpawnType == ETSSpawnType::Attached"))
	TEnumAsByte<enum EAttachLocation::Type> NiagaraAttachmentType;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	FVector NiagaraLocationOffset;

	UPROPERTY(EditAnywhere, Category = "Niagara")
	FRotator NiagaraRotationOffset;
};

/**
 * 
 */
UCLASS()
class TRUEFPSSYSTEM_API UTrueFPSFootstepAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	virtual FString GetNotifyName_Implementation() const override;

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	UDataTable* HitDataTable;

	static FName NAME_Foot_R;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	FName FootSocketName = NAME_Foot_R;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TEnumAsByte<ETraceTypeQuery> TraceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TEnumAsByte<EDrawDebugTrace::Type> DrawDebugType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	float TraceLength = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	bool bSpawnDecal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	bool bMirrorDecalX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	bool bMirrorDecalY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decal")
	bool bMirrorDecalZ = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	bool bSpawnSound = true;

	static FName NAME_FootstepType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	FName SoundParameterName = NAME_FootstepType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	EFootstepType FootstepType = EFootstepType::Step;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	bool bOverrideMaskCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	bool bSpawnNiagara = false;
	
};
