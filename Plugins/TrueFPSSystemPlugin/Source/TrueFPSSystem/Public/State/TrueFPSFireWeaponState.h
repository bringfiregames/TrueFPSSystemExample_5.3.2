#pragma once

#include "TrueFPSFireWeaponState.generated.h"

class UNiagaraComponent;
class UParticleSystemComponent;

USTRUCT(BlueprintType)
struct TRUEFPSSYSTEM_API FTrueFPSFireWeaponState
{
	GENERATED_BODY()

	/** spawned component for muzzle FX */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	TWeakObjectPtr<UParticleSystemComponent> MuzzlePSC;

	/** spawned component for second muzzle FX (Needed for split screen) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	TWeakObjectPtr<UParticleSystemComponent> MuzzlePSCSecondary;

	/** niagara spawned component for muzzle FX */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	TWeakObjectPtr<UNiagaraComponent> NiagaraMuzzleNC;

	/** niagara spawned component for second muzzle FX (Needed for split screen) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	TWeakObjectPtr<UNiagaraComponent> NiagaraMuzzleNCSecondary;

	/** niagara spawned component for shell FX */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	TWeakObjectPtr<UNiagaraComponent> NiagaraShellNC;

	/** niagara spawned component for second shell FX (Needed for split screen) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	TWeakObjectPtr<UNiagaraComponent> NiagaraShellNCSecondary;

	// Timer Handling

	/** Handle for efficient management of StopReload timer */
	FTimerHandle TimerHandle_StopReload;

	/** Handle for efficient management of ReloadWeapon timer */
	FTimerHandle TimerHandle_ReloadWeapon;

};
