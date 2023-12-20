// Copyright Epic Games, Inc. All Rights Reserved.

#include "STrueFPSConfirmationDialog.h"
#include "OnlineSubsystemUtils.h"
#include "UI/Style/TrueFPSMenuItemWidgetStyle.h"
#include "UI/Style/TrueFPSStyle.h"

#if !defined(TRUEFPS_CONTROLLER_DISCONNECT_STRICT)
	#define TRUEFPS_CONTROLLER_DISCONNECT_STRICT 0
#endif

void STrueFPSConfirmationDialog::Construct( const FArguments& InArgs )
{	
	PlayerOwner = InArgs._PlayerOwner;
	DialogType = InArgs._DialogType;

	OnConfirm = InArgs._OnConfirmClicked;
	OnCancel = InArgs._OnCancelClicked;

	const FTrueFPSMenuItemStyle* ItemStyle = &FTrueFPSStyle::Get().GetWidgetStyle<FTrueFPSMenuItemStyle>("DefaultTrueFPSMenuItemStyle");
	const FButtonStyle* ButtonStyle = &FTrueFPSStyle::Get().GetWidgetStyle<FButtonStyle>("DefaultTrueFPSButtonStyle");
	FLinearColor MenuTitleTextColor =  FLinearColor(FColor(155,164,182));
	ChildSlot
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	[					
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(20.0f)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.Padding(50.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.BorderImage(&ItemStyle->BackgroundBrush)
			.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
			[
				SNew( STextBlock )
				.TextStyle(FTrueFPSStyle::Get(), "TrueFPSSystem.MenuHeaderTextStyle")
				.ColorAndOpacity(MenuTitleTextColor)
				.Text(InArgs._MessageText)
				.WrapTextAt(500.0f)
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.Padding(20.0f)
		[
			SNew( SHorizontalBox)
			+SHorizontalBox::Slot()			
			.AutoWidth()
			.Padding(20.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew( SButton )
				.ContentPadding(100)
				.OnClicked(this, &STrueFPSConfirmationDialog::OnConfirmHandler)
				.Text(InArgs._ConfirmText)			
				.TextStyle(FTrueFPSStyle::Get(), "TrueFPSSystem.MenuHeaderTextStyle")
				.ButtonStyle(ButtonStyle)
				.IsFocusable(false)
			]

			+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(20.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew( SButton )
					.ContentPadding(100)
					.OnClicked(InArgs._OnCancelClicked)
					.Text(InArgs._CancelText)
					.TextStyle(FTrueFPSStyle::Get(), "TrueFPSSystem.MenuHeaderTextStyle")
					.ButtonStyle(ButtonStyle)
					.Visibility(InArgs._CancelText.IsEmpty() == false ? EVisibility::Visible : EVisibility::Collapsed)
					.IsFocusable(false)
				]	
		]			
	];
}

bool STrueFPSConfirmationDialog::SupportsKeyboardFocus() const
{
	return true;
}

FReply STrueFPSConfirmationDialog::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	return FReply::Handled().ReleaseMouseCapture().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly, true);
}

FReply STrueFPSConfirmationDialog::OnConfirmHandler()
{
	return ExecuteConfirm(FSlateApplication::Get().GetUserIndexForKeyboard());
}

FReply STrueFPSConfirmationDialog::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	const FKey Key = KeyEvent.GetKey();
	const int32 UserIndex = KeyEvent.GetUserIndex();

	// Filter input based on the type of this dialog
	switch (DialogType)
	{
		case ETrueFPSDialogType::Generic:
		{
			// Ignore input from users that don't own this dialog
			if (PlayerOwner != nullptr && PlayerOwner->GetControllerId() != UserIndex)
			{
				return FReply::Unhandled();
			}
			break;
		}

		case ETrueFPSDialogType::ControllerDisconnected:
		{
			// Only ignore input from controllers that are bound to local users
			if(PlayerOwner != nullptr && PlayerOwner->GetGameInstance() != nullptr)
			{
				if (PlayerOwner->GetGameInstance()->FindLocalPlayerFromControllerId(UserIndex))
				{
					return FReply::Unhandled();
				}
			}
			break;
		}
	}

	// For testing on PC
	if ((Key == EKeys::Enter || Key == EKeys::Virtual_Accept) && !KeyEvent.IsRepeat())
	{
		return ExecuteConfirm(UserIndex);
	}
	else if (Key == EKeys::Escape || Key == EKeys::Virtual_Back)
	{
		if(OnCancel.IsBound())
		{
			return OnCancel.Execute();
		}
	}

	return FReply::Unhandled();
}

FReply STrueFPSConfirmationDialog::ExecuteConfirm(const int32 UserIndex)
{
	if (OnConfirm.IsBound())
	{
		//these two cases should be combined when we move to using PlatformUserIDs rather than ControllerIDs.
#if TRUEFPS_CONTROLLER_DISCONNECT_STRICT
		bool bExecute = false;
		// For controller reconnection, bind the confirming controller to the owner of this dialog
		if (DialogType == ETrueFPSDialogType::ControllerDisconnected && PlayerOwner != nullptr)
		{
			const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
			if (OnlineSub)
			{
				const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
				if (IdentityInterface.IsValid())
				{
					TSharedPtr<const FUniqueNetId> IncomingUserId = IdentityInterface->GetUniquePlayerId(UserIndex);
					FUniqueNetIdRepl DisconnectedId = PlayerOwner->GetCachedUniqueNetId();

					if (DisconnectedId.IsValid() && IncomingUserId.IsValid() && *IncomingUserId == *DisconnectedId)
					{
						PlayerOwner->SetControllerId(UserIndex);
						bExecute = true;
					}
#if TRUEFPS_CONTROLLER_PAIRING_PROMPT_FOR_NEW_USER_WHEN_RECONNECTING
					else if (PlayerOwner->GetCachedUniqueNetId().IsValid()) //only show account picker if there is a user signed in
					{
						const IOnlineExternalUIPtr ExternalUIInterface = OnlineSub->GetExternalUIInterface();
						if (ExternalUIInterface.IsValid())
						{
							ExternalUIInterface->ShowLoginUI(UserIndex, false, true, nullptr);
						}
					}
					else
					{
						// no signed in user - just bind to this controller
						PlayerOwner->SetControllerId(UserIndex);
						bExecute = true;
					}
#endif
				}
			}
		}
		else
		{
			bExecute = true;
		}

		if (bExecute)
		{
			return OnConfirm.Execute();
		}
#else
		// For controller reconnection, bind the confirming controller to the owner of this dialog
		if (DialogType == ETrueFPSDialogType::ControllerDisconnected && PlayerOwner != nullptr)
		{
			PlayerOwner->SetControllerId(UserIndex);
		}

		return OnConfirm.Execute();
#endif
	}

	return FReply::Unhandled();
}
