// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TrueFPSPlayerController_Menu.h"

#include "TrueFPSStyle.h"


ATrueFPSPlayerController_Menu::ATrueFPSPlayerController_Menu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void ATrueFPSPlayerController_Menu::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	FTrueFPSStyle::Initialize();
}
