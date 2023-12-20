// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSMessageMenu.h"

#include "TrueFPSGameInstance.h"
#include "TrueFPSGameViewportClient.h"

#define LOCTEXT_NAMESPACE "TrueFPSSystem.HUD.Menu"

void FTrueFPSMessageMenu::Construct(TWeakObjectPtr<UTrueFPSGameInstance> InGameInstance, TWeakObjectPtr<ULocalPlayer> InPlayerOwner, const FText& Message, const FText& OKButtonText, const FText& CancelButtonText, const FName& InPendingNextState)
{
	GameInstance			= InGameInstance;
	PlayerOwner				= InPlayerOwner;
	PendingNextState		= InPendingNextState;

	if ( ensure( GameInstance.IsValid() ) )
	{
		UTrueFPSGameViewportClient* TrueFPSGameViewportClient = Cast<UTrueFPSGameViewportClient>( GameInstance->GetGameViewportClient() );

		if ( TrueFPSGameViewportClient )
		{
			// Hide the previous dialog
			TrueFPSGameViewportClient->HideDialog();

			// Show the new one
			TrueFPSGameViewportClient->ShowDialog( 
				PlayerOwner,
				ETrueFPSDialogType::Generic,
				Message, 
				OKButtonText, 
				CancelButtonText, 
				FOnClicked::CreateRaw(this, &FTrueFPSMessageMenu::OnClickedOK),
				FOnClicked::CreateRaw(this, &FTrueFPSMessageMenu::OnClickedCancel)
			);
		}
	}
}

void FTrueFPSMessageMenu::RemoveFromGameViewport()
{
	if ( ensure( GameInstance.IsValid() ) )
	{
		UTrueFPSGameViewportClient * TrueFPSGameViewportClient = Cast<UTrueFPSGameViewportClient>( GameInstance->GetGameViewportClient() );

		if ( TrueFPSGameViewportClient )
		{
			// Hide the previous dialog
			TrueFPSGameViewportClient->HideDialog();
		}
	}
}

void FTrueFPSMessageMenu::HideDialogAndGotoNextState()
{
	RemoveFromGameViewport();

	if ( ensure( GameInstance.IsValid() ) )
	{
		GameInstance->GotoState( PendingNextState );
	}
};

FReply FTrueFPSMessageMenu::OnClickedOK()
{
	OKButtonDelegate.ExecuteIfBound();
	HideDialogAndGotoNextState();
	return FReply::Handled();
}

FReply FTrueFPSMessageMenu::OnClickedCancel()
{
	CancelButtonDelegate.ExecuteIfBound();
	HideDialogAndGotoNextState();
	return FReply::Handled();
}

void FTrueFPSMessageMenu::SetOKClickedDelegate(FMessageMenuButtonClicked InButtonDelegate)
{
	OKButtonDelegate = InButtonDelegate;
}

void FTrueFPSMessageMenu::SetCancelClickedDelegate(FMessageMenuButtonClicked InButtonDelegate)
{
	CancelButtonDelegate = InButtonDelegate;
}


#undef LOCTEXT_NAMESPACE
