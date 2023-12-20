// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "TrueFPSLocalPlayer.generated.h"

/**
 * 
 */
UCLASS()
class TRUEFPSSYSTEM_API UTrueFPSLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

public:

UTrueFPSLocalPlayer(const FObjectInitializer& ObjectInitializer);

	virtual void SetControllerId(int32 NewControllerId) override;

	virtual FString GetNickname() const;

	class UTrueFPSPersistentUser* GetPersistentUser() const;
	
	/** Initializes the PersistentUser */
	void LoadPersistentUser();

private:
	/** Persistent user data stored between sessions (i.e. the user's savegame) */
	UPROPERTY()
	class UTrueFPSPersistentUser* PersistentUser;
};
