// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TrueFPSFireWeaponInstantSettings.generated.h"

class UNiagaraSystem;
class UParticleSystem;
class ATrueFPSImpactEffect;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class TRUEFPSSYSTEM_API UTrueFPSFireWeaponInstantSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:

	// Accuracy

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Accuracy")
	float WeaponSpread{5.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Accuracy")
	float TargetingSpreadMod{0.25f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Accuracy")
	float FiringSpreadIncrement{1.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Accuracy")
	float FiringSpreadMax{10.f};

	// WeaponStat

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|WeaponStat")
	float WeaponRange{10000.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|WeaponStat")
	int32 HitDamage{10};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|WeaponStat")
	TSubclassOf<UDamageType> DamageType{UDamageType::StaticClass()};

	// HitVerification

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HitVerification")
	float ClientSideHitLeeway{200.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HitVerification")
	float AllowedViewDotHitDir{0.8f};

	// Effects

	/** impact effects */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects")
	TSubclassOf<ATrueFPSImpactEffect> ImpactTemplate;

	/** smoke trail */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects")
	TObjectPtr<UParticleSystem> TrailFX;

	/** param name for beam target in smoke trail */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects")
	FName TrailTargetParam;

	/** niagara smoke trail */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects")
	TObjectPtr<UNiagaraSystem> NiagaraTrailFX;

	/** niagara param name for beam target in smoke trail */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects")
	FName NiagaraTrailTargetParam{"User.ImpactPositions"};

	/** niagara param name for beam trigger in smoke trail */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects")
	FName NiagaraTrailTriggerParam{"User.Trigger"};
	
};
