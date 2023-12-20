// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSWelcomeMenu.h"
#include "TrueFPSSystem.h"
#include "OnlineSubsystemUtils.h"
#include "TrueFPSGameInstance.h"
#include "TrueFPSGameViewportClient.h"
#include "UI/Style/TrueFPSStyle.h"

#define LOCTEXT_NAMESPACE "TrueFPSSystem.HUD.Menu"

class STrueFPSWelcomeMenuWidget : public SCompoundWidget
{
	/** The menu that owns this widget. */
	FTrueFPSWelcomeMenu* MenuOwner;

	/** Animate the text so that the screen isn't static, for console cert requirements. */
	FCurveSequence TextAnimation;

	/** The actual curve that animates the text. */
	FCurveHandle TextColorCurve;

	TSharedPtr<SRichTextBlock> PressPlayText;

	/* On the first tick ensure we have set the keyboard focus */
	bool bFirstTick = true;

	SLATE_BEGIN_ARGS( STrueFPSWelcomeMenuWidget )
	{}

	SLATE_ARGUMENT(FTrueFPSWelcomeMenu*, MenuOwner)

	SLATE_END_ARGS()

	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	UWorld* GetWorld() const
	{
		if (MenuOwner && MenuOwner->GetGameInstance().IsValid())
		{
			return MenuOwner->GetGameInstance()->GetWorld();
		}

		return nullptr;
	}

	void Construct( const FArguments& InArgs )
	{
		MenuOwner = InArgs._MenuOwner;
		
		TextAnimation = FCurveSequence();
		const float AnimDuration = 1.5f;
		TextColorCurve = TextAnimation.AddCurve(0, AnimDuration, ECurveEaseFunction::QuadInOut);

		ChildSlot
		[
			SNew(SBorder)
			.Padding(30.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[ 
				SAssignNew( PressPlayText, SRichTextBlock )
#if PLATFORM_SWITCH
				.Text(LOCTEXT("PressStartSwitch", "PRESS <img src=\"TrueFPSSystem.Switch.Right\"/> TO PLAY"))
#elif TRUEFPS_XBOX_STRINGS
				.Text( LOCTEXT("PressStartXboxOne", "PRESS A TO PLAY" ) )
#else
				.Text( LOCTEXT("PressStartPC", "PRESS ENTER TO PLAY" ) )
#endif
				.TextStyle( FTrueFPSStyle::Get(), "TrueFPSSystem.WelcomeScreen.WelcomeTextStyle" )
				.DecoratorStyleSet(&FTrueFPSStyle::Get())
				+ SRichTextBlock::ImageDecorator()
			]
		];
	}

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		// During construction we may miss out on setting focus of the keyboard due to the GameViewport not having its Parent pointer
		// setup. If this happens we will be unable to set the keyboard focus in AddToGameViewport()
		if (bFirstTick)
		{
			bFirstTick = false;
			FSlateApplication::Get().SetKeyboardFocus(this->AsShared());
		}

		if(!TextAnimation.IsPlaying())
		{
			if(TextAnimation.IsAtEnd())
			{
				TextAnimation.PlayReverse(this->AsShared());
			}
			else
			{
				TextAnimation.Play(this->AsShared());
			}
		}

		PressPlayText->SetRenderOpacity(FMath::Lerp(0.5f, 1.0f, TextColorCurve.GetLerp()));
	}

	virtual FReply OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override
	{
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::Enter)
		{
			TSharedPtr<const FUniqueNetId> UserId;
			const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
			if (OnlineSub)
			{
				const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
				if (IdentityInterface.IsValid())
				{
					UserId = IdentityInterface->GetUniquePlayerId(InKeyEvent.GetUserIndex());
				}
			}
			MenuOwner->HandleLoginUIClosed(UserId, InKeyEvent.GetUserIndex());
		}
		else if (!MenuOwner->GetControlsLocked() && Key == EKeys::Virtual_Accept)
		{
			bool bSkipToMainMenu = true;

			{
				const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
				if (OnlineSub)
				{
					const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
					if (IdentityInterface.IsValid())
					{
						TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
						const bool bIsLicensed = GenericApplication->ApplicationLicenseValid();

						const ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(InKeyEvent.GetUserIndex());
						if (LoginStatus == ELoginStatus::NotLoggedIn || !bIsLicensed)
						{
							// Show the account picker.
							const IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
							if (ExternalUI.IsValid())
							{
								ExternalUI->ShowLoginUI(InKeyEvent.GetUserIndex(), false, true, FOnLoginUIClosedDelegate::CreateSP(MenuOwner, &FTrueFPSWelcomeMenu::HandleLoginUIClosed));
								bSkipToMainMenu = false;
							}
						}
					}
				}
			}

			if (bSkipToMainMenu)
			{
				const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
				if (OnlineSub)
				{
					const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
					if (IdentityInterface.IsValid())
					{
						TSharedPtr<const FUniqueNetId> UserId = IdentityInterface->GetUniquePlayerId(InKeyEvent.GetUserIndex());
						// If we couldn't show the external login UI for any reason, or if the user is
						// already logged in, just advance to the main menu immediately.
						MenuOwner->HandleLoginUIClosed(UserId, InKeyEvent.GetUserIndex());
					}
				}
			}

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override
	{
		bFirstTick = true;
	}

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override
	{
		return FReply::Handled().ReleaseMouseCapture().SetUserFocus(SharedThis( this ), EFocusCause::SetDirectly, true);
	}
};

void FTrueFPSWelcomeMenu::Construct( TWeakObjectPtr< UTrueFPSGameInstance > InGameInstance )
{
	bControlsLocked = false;
	GameInstance = InGameInstance;
	PendingControllerIndex = -1;

	MenuWidget = SNew( STrueFPSWelcomeMenuWidget )
		.MenuOwner(this);	
}

void FTrueFPSWelcomeMenu::AddToGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(MenuWidget.ToSharedRef());
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
	}
}

void FTrueFPSWelcomeMenu::RemoveFromGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
	}
}

void FTrueFPSWelcomeMenu::HandleLoginUIClosed(TSharedPtr<const FUniqueNetId> UniqueId, const int ControllerIndex, const FOnlineError& Error)
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return;
	}

	UTrueFPSGameViewportClient* TrueFPSViewport = Cast<UTrueFPSGameViewportClient>( GameInstance->GetGameViewportClient() );

	TSharedPtr<GenericApplication> GenericApplication = FSlateApplication::Get().GetPlatformApplication();
	const bool bIsLicensed = GenericApplication->ApplicationLicenseValid();

	// If they don't currently have a license, let them know, but don't let them proceed
	if (!bIsLicensed && TrueFPSViewport != nullptr)
	{
		const FText StopReason	= NSLOCTEXT( "ProfileMessages", "NeedLicense", "The signed in users do not have a license for this game. Please purchase TrueFPSSystem from the Xbox Marketplace or sign in a user with a valid license." );
		const FText OKButton	= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

		TrueFPSViewport->ShowDialog( 
			nullptr,
			ETrueFPSDialogType::Generic,
			StopReason,
			OKButton,
			FText::GetEmpty(),
			FOnClicked::CreateRaw(this, &FTrueFPSWelcomeMenu::OnConfirmGeneric),
			FOnClicked::CreateRaw(this, &FTrueFPSWelcomeMenu::OnConfirmGeneric)
			);
		return;
	}

	PendingControllerIndex = ControllerIndex;

	if (UniqueId.IsValid())
	{
		// Next step, check privileges
		const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GameInstance->GetWorld());
		if (OnlineSub)
		{
			const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
			if (IdentityInterface.IsValid())
			{
				IdentityInterface->GetUserPrivilege(*UniqueId, EUserPrivileges::CanPlay, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FTrueFPSWelcomeMenu::OnUserCanPlay));

				// We'll also attempt login at this stage, if we're not already logged in
				if (IdentityInterface->GetLoginStatus(ControllerIndex) != ELoginStatus::LoggedIn)
				{
					IdentityInterface->AutoLogin(ControllerIndex);
				}
			}
		}
	}
	else
	{
		// Show a warning that your progress won't be saved if you continue without logging in. 
		if (TrueFPSViewport != nullptr)
		{
			TrueFPSViewport->ShowDialog( 
				nullptr,
				ETrueFPSDialogType::Generic,
				NSLOCTEXT("ProfileMessages", "ProgressWillNotBeSaved", "If you continue without signing in, your progress will not be saved."),
#if TRUEFPS_XBOX_STRINGS
				NSLOCTEXT("DialogButtons", "AContinue", "A - Continue"),
				NSLOCTEXT("DialogButtons", "BBack", "B - Back"),
#else
				NSLOCTEXT("DialogButtons", "EnterContinue", "Enter - Continue"),
				NSLOCTEXT("DialogButtons", "EscBack", "Esc - Back"),
#endif
				FOnClicked::CreateRaw(this, &FTrueFPSWelcomeMenu::OnContinueWithoutSavingConfirm),
				FOnClicked::CreateRaw(this, &FTrueFPSWelcomeMenu::OnConfirmGeneric)
			);
		}
	}
}

void FTrueFPSWelcomeMenu::SetControllerAndAdvanceToMainMenu(const int ControllerIndex)
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return;
	}

	ULocalPlayer * NewPlayerOwner = GameInstance->GetFirstGamePlayer();

	if ( NewPlayerOwner != nullptr && ControllerIndex != -1 )
	{
		NewPlayerOwner->SetControllerId(ControllerIndex);
		NewPlayerOwner->SetCachedUniqueNetId(NewPlayerOwner->GetUniqueNetIdFromCachedControllerId().GetUniqueNetId());

		// tell gameinstance to transition to main menu
		GameInstance->GotoState(TrueFPSGameInstanceState::MainMenu);
	}	
}

FReply FTrueFPSWelcomeMenu::OnContinueWithoutSavingConfirm()
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return FReply::Handled();
	}

	UTrueFPSGameViewportClient * TrueFPSViewport = Cast<UTrueFPSGameViewportClient>( GameInstance->GetGameViewportClient() );

	if (TrueFPSViewport != nullptr)
	{
		TrueFPSViewport->HideDialog();
	}

	SetControllerAndAdvanceToMainMenu(PendingControllerIndex);
	return FReply::Handled();
}

FReply FTrueFPSWelcomeMenu::OnConfirmGeneric()
{
	if ( !ensure( GameInstance.IsValid() ) )
	{
		return FReply::Handled();
	}

	UTrueFPSGameViewportClient * TrueFPSViewport = Cast<UTrueFPSGameViewportClient>( GameInstance->GetGameViewportClient() );

	if (TrueFPSViewport != nullptr)
	{
		TrueFPSViewport->HideDialog();
	}

	return FReply::Handled();
}

void FTrueFPSWelcomeMenu::OnUserCanPlay(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		SetControllerAndAdvanceToMainMenu(PendingControllerIndex);
	}
	else
	{
		UTrueFPSGameViewportClient * TrueFPSViewport = Cast<UTrueFPSGameViewportClient>( GameInstance->GetGameViewportClient() );

		if ( TrueFPSViewport != nullptr )
		{
			const FText ReturnReason = NSLOCTEXT("PrivilegeFailures", "CannotPlayAgeRestriction", "You cannot play this game due to age restrictions.");
			const FText OKButton = NSLOCTEXT("DialogButtons", "OKAY", "OK");

			TrueFPSViewport->ShowDialog( 
				nullptr,
				ETrueFPSDialogType::Generic,
				ReturnReason,
				OKButton,
				FText::GetEmpty(),
				FOnClicked::CreateRaw(this, &FTrueFPSWelcomeMenu::OnConfirmGeneric),
				FOnClicked::CreateRaw(this, &FTrueFPSWelcomeMenu::OnConfirmGeneric)
			);
		}
	}
}

#undef LOCTEXT_NAMESPACE
