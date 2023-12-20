#pragma once

#include "TrueFPSCharacterState.generated.h"

USTRUCT(BlueprintType)
struct TRUEFPSSYSTEM_API FTrueFPSCharacterState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float LeanValue{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float CrouchValue{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	bool bIsDying{false};
	
	/** Time at which point the last take hit info for the actor times out and won't be replicated; Used to stop join-in-progress effects all over the screen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float LastTakeHitTimeTimeout;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float LowHealthPercentage{0.5f};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	bool bWantsToFire{false};

	// Input

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float CurrentPitchControllerInput{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float CurrentYawControllerInput{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TrueFPS")
	float CurrentRollControllerInput{0.f};

	FRotator GetCurrentControllerInput() const { return FRotator(CurrentPitchControllerInput, CurrentYawControllerInput, CurrentRollControllerInput); }

};
