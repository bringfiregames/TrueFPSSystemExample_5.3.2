// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrueFPSTypes.h"
#include "TrueFPSImpactEffect.generated.h"

class USoundBase;
class UParticleSystem;
class UNiagaraSystem;

//
// Spawnable effect for weapon hit impact - NOT replicated to clients
// Each impact type should be defined as separate blueprint
//
UCLASS(Abstract, Blueprintable)
class TRUEFPSSYSTEM_API ATrueFPSImpactEffect : public AActor
{
	GENERATED_BODY()
	
public:
	
	ATrueFPSImpactEffect(const FObjectInitializer& ObjectInitializer);

	/** default impact FX used when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	UParticleSystem* DefaultFX;

	/** impact FX on concrete */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* ConcreteFX;

	/** impact FX on dirt */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* DirtFX;

	/** impact FX on water */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* WaterFX;

	/** impact FX on metal */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* MetalFX;

	/** impact FX on wood */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* WoodFX;

	/** impact FX on glass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* GlassFX;

	/** impact FX on grass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* GrassFX;

	/** impact FX on flesh */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* FleshFX;

	/** default impact FX used when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	TSoftObjectPtr<UNiagaraSystem> NiagaraDefaultFX;

	/** impact FX on concrete */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraConcreteFX;

	/** impact FX on dirt */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraDirtFX;

	/** impact FX on water */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraWaterFX;

	/** impact FX on metal */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraMetalFX;

	/** impact FX on wood */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraWoodFX;

	/** impact FX on glass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraGlassFX;

	/** impact FX on grass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraGrassFX;

	/** impact FX on flesh */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraFleshFX;

	/** niagara impact fx impact positions param */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	FName NiagaraImpactFXPositionsParam;

	/** niagara impact fx impact normals param */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	FName NiagaraImpactFXNormalsParam;

	/** default impact sound used when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	USoundBase* DefaultSound;

	/** impact FX on concrete */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundBase* ConcreteSound;

	/** impact FX on dirt */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundBase* DirtSound;

	/** impact FX on water */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundBase* WaterSound;

	/** impact FX on metal */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundBase* MetalSound;

	/** impact FX on wood */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundBase* WoodSound;

	/** impact FX on glass */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundBase* GlassSound;

	/** impact FX on grass */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundBase* GrassSound;

	/** impact FX on flesh */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundBase* FleshSound;

	/** default decal when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	struct FDecalData DefaultDecal;

	/** impact Decal on concrete */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	TSoftObjectPtr<UNiagaraSystem> NiagaraDefaultDecal;

	/** impact Decal on concrete */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraConcreteDecal;

	/** impact Decal on dirt */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraDirtDecal;

	/** impact Decal on water */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraWaterDecal;

	/** impact Decal on metal */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraMetalDecal;

	/** impact Decal on wood */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraWoodDecal;

	/** impact Decal on glass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraGlassDecal;

	/** impact Decal on grass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraGrassDecal;

	/** impact Decal on flesh */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	TSoftObjectPtr<UNiagaraSystem> NiagaraFleshDecal;

	/** niagara decal trigger param */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	FName NiagaraDecalTriggerParam;

	/** niagara decal surfaces param */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	FName NiagaraDecalSurfacesParam;

	/** niagara decal surfaces param */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	FName NiagaraDecalSurfaceParam;

	/** niagara decal positions param */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	FName NiagaraDecalPositionsParam;

	/** niagara decal normals param */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	FName NiagaraDecalNormalsParam;

	/** surface data for spawning */
	UPROPERTY(BlueprintReadOnly, Category=Surface)
	FHitResult SurfaceHit;

	/** spawn effect */
	virtual void PostInitializeComponents() override;

protected:

	/** get FX for material type */
	UParticleSystem* GetImpactFX(TEnumAsByte<EPhysicalSurface> SurfaceType) const;

	/** get FX for material type */
	TSoftObjectPtr<UNiagaraSystem> GetNiagaraImpactFX(TEnumAsByte<EPhysicalSurface> SurfaceType) const;

	/** get niagara decal for material type */
	TSoftObjectPtr<UNiagaraSystem> GetNiagaraDecal(TEnumAsByte<EPhysicalSurface> SurfaceType) const;

	/** get sound for material type */
	USoundBase* GetImpactSound(TEnumAsByte<EPhysicalSurface> SurfaceType) const;

};
