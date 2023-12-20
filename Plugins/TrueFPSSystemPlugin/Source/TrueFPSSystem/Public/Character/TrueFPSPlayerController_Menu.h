// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TrueFPSPlayerController_Menu.generated.h"

UCLASS()
class TRUEFPSSYSTEM_API ATrueFPSPlayerController_Menu : public APlayerController
{
	GENERATED_BODY()

public:
	
	ATrueFPSPlayerController_Menu(const FObjectInitializer& ObjectInitializer);

	/** After game is initialized */
	virtual void PostInitializeComponents() override;
	
};
