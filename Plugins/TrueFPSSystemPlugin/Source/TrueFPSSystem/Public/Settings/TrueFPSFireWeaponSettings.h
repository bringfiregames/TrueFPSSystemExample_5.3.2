// 

#pragma once

#include "CoreMinimal.h"
#include "TrueFPSWeaponSettings.h"
#include "Engine/Canvas.h"
#include "Engine/DataAsset.h"
#include "TrueFPSFireWeaponSettings.generated.h"

class UStaticMesh;
class UNiagaraSystem;
class UParticleSystem;

UCLASS(Blueprintable, BlueprintType)
class TRUEFPSSYSTEM_API UTrueFPSFireWeaponSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:

	// Stats

	/** failsafe reload duration if weapon doesn't have any animation for it */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Stats")
	float NoAnimReloadDuration{1.f};

	// Ammo

	/** inifite ammo for reloads */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ammo")
	bool bInfiniteAmmo{false};

	/** infinite ammo in clip, no reload required */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ammo")
	bool bInfiniteClip{false};

	/** max ammo */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ammo")
	int32 MaxAmmo{100};

	/** clip size */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ammo")
	int32 AmmoPerClip{20};

	/** initial clips */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ammo")
	int32 InitialClips{4};

	/** shots made for each fire. Useful for shotguns */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Ammo")
	int32 ShotsByFiring{1};

	// Animations

	/** reload animations */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animations")
	FWeaponAnim ReloadAnim;

	// Sounds

	/** reload sound */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Sounds")
	TObjectPtr<USoundBase> ReloadSound;

	// Effects

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects")
	FName MuzzleAttachPoint{"Muzzle"};

	/** FX for muzzle flash */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Muzzle")
	TObjectPtr<UParticleSystem> MuzzleFX;
	
	/** niagara FX for muzzle flash */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Muzzle")
	TObjectPtr<UNiagaraSystem> NiagaraMuzzleFX;

	/** niagara param name for muzzle flash trigger */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Muzzle")
	FName NiagaraMuzzleTriggerParam{"User.Trigger"};

	/** niagara FX for shell eject */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Shell")
	TObjectPtr<UNiagaraSystem> NiagaraShellFX;

	/** shell eject mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Shell")
	TObjectPtr<UStaticMesh> ShellMesh;

	/** shell eject socket name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Shell")
	FName ShellEjectSocketName{"ShellEject"};

	/** niagara param name for shell eject trigger */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Shell")
	FName NiagaraShellTriggerParam{"User.Trigger"};

	/** niagara param name for shell eject mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Shell")
	FName NiagaraShellMeshParam{"User.ShellEjectStaticMesh"};

	/** is muzzle FX looped? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Muzzle")
	bool bLoopedMuzzleFX{false};

	/** is muzzle FX looped? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Effects|Shell")
	bool bLoopedShellFX{false};

	// HUD
	
	/** bullet icon used to draw current clip (left side) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	FCanvasIcon PrimaryClipIcon;

	/** bullet icon used to draw secondary clip (left side) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	FCanvasIcon SecondaryClipIcon;

	/** how many icons to draw per clip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	float AmmoIconsCount;

	/** defines spacing between primary ammo icons (left side) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	int32 PrimaryClipIconOffset;

	/** defines spacing between secondary ammo icons (left side) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|HUD")
	int32 SecondaryClipIconOffset;
	
};
