// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionClient.h"
#include "TrueFPSOnlineSessionClient.generated.h"

UCLASS(Config = Game)
class TRUEFPSSYSTEM_API UTrueFPSOnlineSessionClient : public UOnlineSessionClient
{
	GENERATED_BODY()

public:

	UTrueFPSOnlineSessionClient();

	virtual void OnSessionUserInviteAccepted(
		const bool							bWasSuccess,
		const int32							ControllerId,
		TSharedPtr< const FUniqueNetId >	UserId,
		const FOnlineSessionSearchResult &	InviteResult
	) override;
	
};
