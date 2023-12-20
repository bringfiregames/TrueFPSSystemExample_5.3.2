// Fill out your copyright notice in the Description page of Project Settings.


#include "Online/TrueFPSOnlineSessionClient.h"

#include "TrueFPSGameInstance.h"

UTrueFPSOnlineSessionClient::UTrueFPSOnlineSessionClient()
{
}

void UTrueFPSOnlineSessionClient::OnSessionUserInviteAccepted(
	const bool							bWasSuccess,
	const int32							ControllerId,
	TSharedPtr< const FUniqueNetId > 	UserId,
	const FOnlineSessionSearchResult &	InviteResult
)
{
	UE_LOG(LogOnline, Verbose, TEXT("HandleSessionUserInviteAccepted: bSuccess: %d, ControllerId: %d, User: %s"), bWasSuccess, ControllerId, UserId.IsValid() ? *UserId->ToString() : TEXT("nullptr"));

	if (!bWasSuccess)
	{
		return;
	}

	if (!InviteResult.IsSessionInfoValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Invite accept returned no search result."));
		return;
	}

	if (!UserId.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Invite accept returned no user."));
		return;
	}

	UTrueFPSGameInstance* TrueFPSGameInstance = Cast<UTrueFPSGameInstance>(GetGameInstance());

	if (TrueFPSGameInstance)
	{
		FTrueFPSPendingInvite PendingInvite;

		// Set the pending invite, and then go to the initial screen, which is where we will process it
		PendingInvite.ControllerId = ControllerId;
		PendingInvite.UserId = UserId;
		PendingInvite.InviteResult = InviteResult;
		PendingInvite.bPrivilegesCheckedAndAllowed = false;

		TrueFPSGameInstance->SetPendingInvite(PendingInvite);
		TrueFPSGameInstance->GotoState(TrueFPSGameInstanceState::PendingInvite);
	}
}
