// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TrueFPSAnimInstanceSettings.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class TRUEFPSSYSTEM_API UTrueFPSAnimInstanceSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float MaxMoveSpeed{600.f};

	// The amount the weapon pitches towards the camera's pitch value. Should be a very small value or zero
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float WeaponPitchTiltMultiplier{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float AccumulativeRotationReturnInterpSpeed{30.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float AccumulativeRotationInterpSpeed{5.f};

	// The rotation interp speed on non-local players' viewports
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float NonLocalCameraRotationInterpSpeed{20.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float VelocityInterpSpeed{10.f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float VelocitySwaySpeed{10.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float VelocitySwayWeight{10.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float LandingImpactBobMultiplier{1.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float MinMoveSpeedToApplyMovementSway{100.f};
	
};
