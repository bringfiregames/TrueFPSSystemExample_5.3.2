// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrueFPSTypes.h"
#include "TrueFPSExplosionEffect.generated.h"

//
// Spawnable effect for explosion - NOT replicated to clients
// Each explosion type should be defined as separate blueprint
//
UCLASS(Abstract, Blueprintable)
class TRUEFPSSYSTEM_API ATrueFPSExplosionEffect : public AActor
{
	GENERATED_BODY()

public:
	
	ATrueFPSExplosionEffect(const FObjectInitializer& ObjectInitializer);

	/** explosion FX */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	class UParticleSystem* ExplosionFX;

private:
	/** explosion light */
	UPROPERTY(VisibleDefaultsOnly, Category=Effect)
	class UPointLightComponent* ExplosionLight;
public:

	/** how long keep explosion light on? */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	float ExplosionLightFadeOut;

	/** explosion sound */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	class USoundCue* ExplosionSound;
	
	/** explosion decals */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	FDecalData Decal;

	/** surface data for spawning */
	UPROPERTY(BlueprintReadOnly, Category=Surface)
	FHitResult SurfaceHit;

	/** update fading light */
	virtual void Tick(float DeltaSeconds) override;

protected:
	
	/** spawn explosion */
	virtual void BeginPlay() override;

private:

	/** Point light component name */
	FName ExplosionLightComponentName;

public:
	/** Returns ExplosionLight subobject **/
	FORCEINLINE class UPointLightComponent* GetExplosionLight() const { return ExplosionLight; }
	
};
