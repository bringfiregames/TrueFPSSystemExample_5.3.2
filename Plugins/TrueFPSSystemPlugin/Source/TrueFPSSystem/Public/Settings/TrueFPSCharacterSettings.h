#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TrueFPSCharacterSettings.generated.h"

class ATrueFPSWeaponBase;
class USoundCue;
class UAnimMontage;

UCLASS(Blueprintable, BlueprintType)
class TRUEFPSSYSTEM_API UTrueFPSCharacterSettings : public UDataAsset
{
	GENERATED_BODY()

public:

	// Animations

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animation")
	FName RightHandName{"hand_r"};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animation")
	FName LeftHandName{"hand_l"};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animation")
	bool bRightHanded{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animation")
	float LeanAmount{45.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animation")
	float AimingSpeedModifier{0.5f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Animation")
	float RunningSpeedModifier{1.5f};

	UPROPERTY(EditDefaultsOnly, Category = "Config|Animation")
	TObjectPtr<UAnimMontage> DeathAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Config|Pawn")
	TObjectPtr<USoundCue> DeathSound;

	// Weapons

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Weapon")
	TArray<TSubclassOf<ATrueFPSWeaponBase>> DefaultWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Weapon|Animation")
	float AimingInterpSpeed{10.f};

	// Camera

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Camera", meta = (ClampMin = 0, ClampMax = 90))
	float CameraPitchClamp{88.9f};
	
};
