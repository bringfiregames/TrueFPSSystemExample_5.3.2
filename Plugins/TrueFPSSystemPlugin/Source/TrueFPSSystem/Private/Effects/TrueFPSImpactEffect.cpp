// Fill out your copyright notice in the Description page of Project Settings.


#include "Effects/TrueFPSImpactEffect.h"

#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundCue.h"

ATrueFPSImpactEffect::ATrueFPSImpactEffect(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetAutoDestroyWhenFinished(true);
}

void ATrueFPSImpactEffect::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UPhysicalMaterial* HitPhysMat = SurfaceHit.PhysMaterial.Get();
	EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);

	// show particles
	UParticleSystem* ImpactFX = GetImpactFX(HitSurfaceType);
	if (ImpactFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ImpactFX, GetActorLocation(), GetActorRotation());
	}

	// show niagara particles
	TSoftObjectPtr<UNiagaraSystem> NiagaraImpactFX = GetNiagaraImpactFX(HitSurfaceType);
	if (NiagaraImpactFX.LoadSynchronous())
	{
		UNiagaraComponent* NiagaraImpactNC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, NiagaraImpactFX.Get(), GetActorLocation(), GetActorRotation());

		TArray<FVector> PositionArray;
		PositionArray.Add(SurfaceHit.ImpactPoint);
		// PositionArray.Add(GetActorLocation());
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NiagaraImpactNC, NiagaraImpactFXPositionsParam, PositionArray);

		TArray<FVector> NormalArray;
		NormalArray.Add(SurfaceHit.ImpactNormal);
		// NormalArray.Add(GetActorRotation().Vector());
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraImpactNC, NiagaraImpactFXNormalsParam, NormalArray);
	}

	// play sound
	USoundBase* ImpactSound = GetImpactSound(HitSurfaceType);
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if (DefaultDecal.DecalMaterial)
	{
		FRotator RandomDecalRotation = SurfaceHit.ImpactNormal.Rotation();
		RandomDecalRotation.Roll = FMath::FRandRange(-180.0f, 180.0f);

		UGameplayStatics::SpawnDecalAttached(DefaultDecal.DecalMaterial, FVector(1.0f, DefaultDecal.DecalSize, DefaultDecal.DecalSize),
			SurfaceHit.Component.Get(), SurfaceHit.BoneName,
			SurfaceHit.ImpactPoint, RandomDecalRotation, EAttachLocation::KeepWorldPosition,
			DefaultDecal.LifeSpan);
	}

	// show niagara particles
	TSoftObjectPtr<UNiagaraSystem> NiagaraDecal = GetNiagaraDecal(HitSurfaceType);
	if (NiagaraDecal.LoadSynchronous())
	{
		UNiagaraComponent* NiagaraDecalNC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, NiagaraDecal.Get(), FVector(ForceInitToZero), FRotator::ZeroRotator);

		NiagaraDecalNC->SetNiagaraVariableBool(NiagaraDecalTriggerParam.ToString(), true);

		TArray<int32> SurfaceTypeArray;
		SurfaceTypeArray.Add(HitSurfaceType);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(NiagaraDecalNC, NiagaraDecalSurfacesParam, SurfaceTypeArray);

		// NiagaraDecalNC->SetNiagaraVariableInt(NiagaraDecalSurfaceParam.ToString(), 0);

		TArray<FVector> PositionArray;
		PositionArray.Add(SurfaceHit.ImpactPoint);
		// PositionArray.Add(GetActorLocation());
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayPosition(NiagaraDecalNC, NiagaraDecalPositionsParam, PositionArray);

		TArray<FVector> NormalArray;
		NormalArray.Add(SurfaceHit.ImpactNormal);
		// NormalArray.Add(GetActorRotation().Vector());
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraDecalNC, NiagaraDecalNormalsParam, NormalArray);
	}
}

UParticleSystem* ATrueFPSImpactEffect::GetImpactFX(TEnumAsByte<EPhysicalSurface> SurfaceType) const
{
	UParticleSystem* ImpactFX = nullptr;

	switch (SurfaceType)
	{
		case TRUEFPS_SURFACE_Concrete:	ImpactFX = ConcreteFX; break;
		case TRUEFPS_SURFACE_Dirt:		ImpactFX = DirtFX; break;
		case TRUEFPS_SURFACE_Water:		ImpactFX = WaterFX; break;
		case TRUEFPS_SURFACE_Metal:		ImpactFX = MetalFX; break;
		case TRUEFPS_SURFACE_Wood:		ImpactFX = WoodFX; break;
		case TRUEFPS_SURFACE_Grass:		ImpactFX = GrassFX; break;
		case TRUEFPS_SURFACE_Glass:		ImpactFX = GlassFX; break;
		case TRUEFPS_SURFACE_Flesh:		ImpactFX = FleshFX; break;
		default:						ImpactFX = DefaultFX; break;
	}

	return ImpactFX;
}

TSoftObjectPtr<UNiagaraSystem> ATrueFPSImpactEffect::GetNiagaraImpactFX(TEnumAsByte<EPhysicalSurface> SurfaceType) const
{
	TSoftObjectPtr<UNiagaraSystem> ImpactFX;

	switch (SurfaceType)
	{
		case TRUEFPS_SURFACE_Concrete:	ImpactFX = NiagaraConcreteFX; break;
		case TRUEFPS_SURFACE_Dirt:		ImpactFX = NiagaraDirtFX; break;
		case TRUEFPS_SURFACE_Water:		ImpactFX = NiagaraWaterFX; break;
		case TRUEFPS_SURFACE_Metal:		ImpactFX = NiagaraMetalFX; break;
		case TRUEFPS_SURFACE_Wood:		ImpactFX = NiagaraWoodFX; break;
		case TRUEFPS_SURFACE_Grass:		ImpactFX = NiagaraGrassFX; break;
		case TRUEFPS_SURFACE_Glass:		ImpactFX = NiagaraGlassFX; break;
		case TRUEFPS_SURFACE_Flesh:		ImpactFX = NiagaraFleshFX; break;
		default:						ImpactFX = NiagaraDefaultFX; break;
	}

	return ImpactFX;
}

TSoftObjectPtr<UNiagaraSystem> ATrueFPSImpactEffect::GetNiagaraDecal(TEnumAsByte<EPhysicalSurface> SurfaceType) const
{
	TSoftObjectPtr<UNiagaraSystem> ImpactFX;

	switch (SurfaceType)
	{
		case TRUEFPS_SURFACE_Concrete:	ImpactFX = NiagaraConcreteDecal; break;
		case TRUEFPS_SURFACE_Dirt:		ImpactFX = NiagaraDirtDecal; break;
		case TRUEFPS_SURFACE_Water:		ImpactFX = NiagaraWaterDecal; break;
		case TRUEFPS_SURFACE_Metal:		ImpactFX = NiagaraMetalDecal; break;
		case TRUEFPS_SURFACE_Wood:		ImpactFX = NiagaraWoodDecal; break;
		case TRUEFPS_SURFACE_Grass:		ImpactFX = NiagaraGrassDecal; break;
		case TRUEFPS_SURFACE_Glass:		ImpactFX = NiagaraGlassDecal; break;
		case TRUEFPS_SURFACE_Flesh:		ImpactFX = NiagaraFleshDecal; break;
		default:						ImpactFX = NiagaraDefaultDecal; break;
	}

	return ImpactFX;
}

USoundBase* ATrueFPSImpactEffect::GetImpactSound(TEnumAsByte<EPhysicalSurface> SurfaceType) const
{
	USoundBase* ImpactSound = nullptr;

	switch (SurfaceType)
	{
		case TRUEFPS_SURFACE_Concrete:	ImpactSound = ConcreteSound; break;
		case TRUEFPS_SURFACE_Dirt:		ImpactSound = DirtSound; break;
		case TRUEFPS_SURFACE_Water:		ImpactSound = WaterSound; break;
		case TRUEFPS_SURFACE_Metal:		ImpactSound = MetalSound; break;
		case TRUEFPS_SURFACE_Wood:		ImpactSound = WoodSound; break;
		case TRUEFPS_SURFACE_Grass:		ImpactSound = GrassSound; break;
		case TRUEFPS_SURFACE_Glass:		ImpactSound = GlassSound; break;
		case TRUEFPS_SURFACE_Flesh:		ImpactSound = FleshSound; break;
		default:						ImpactSound = DefaultSound; break;
	}

	return ImpactSound;
}
