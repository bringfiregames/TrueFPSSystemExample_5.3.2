// Copyright Epic Games, Inc. All Rights Reserved.

#include "STrueFPSMenuWidget.h"
#include "TrueFPSSystem.h"
#include "Engine/Console.h"
#include "TrueFPSMenuItem.h"
#include "STrueFPSMenuItem.h"
#include "TrueFPSGameInstance.h"
#include "TrueFPSGameUserSettings.h"
#include "Character/TrueFPSLocalPlayer.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "UI/TrueFPSUIHelpers.h"
#include "UI/Style/TrueFPSMenuWidgetStyle.h"
#include "UI/Style/TrueFPSStyle.h"

#define LOCTEXT_NAMESPACE "STrueFPSMenuWidget"

#if !defined(PROFILE_SWAPPING)
#define PROFILE_SWAPPING	0
#endif

void STrueFPSMenuWidget::Construct(const FArguments& InArgs)
{
	MenuStyle = &FTrueFPSStyle::Get().GetWidgetStyle<FTrueFPSMenuStyle>("DefaultTrueFPSMenuStyle");

	bControlsLocked = false;
	bConsoleVisible = false;
	OutlineWidth = 20.0f;
	SelectedIndex = 0;
	PlayerOwner = InArgs._PlayerOwner;
	bGameMenu = InArgs._IsGameMenu;
	ControllerHideMenuKey = EKeys::Gamepad_Special_Right;
	Visibility.Bind(this, &STrueFPSMenuWidget::GetSlateVisibility);
	FLinearColor MenuTitleTextColor =  FLinearColor(FColor(155,164,182));
	MenuHeaderHeight = 62.0f;
	MenuHeaderWidth = 287.0f;

	// Calculate the size of the profile box based on the string it'll contain (+ padding)
	const FText PlayerName = PlayerOwner.IsValid() ? FText::FromString(PlayerOwner->GetNickname()) : FText::GetEmpty();
	const FText ProfileSwap = TrueFPSUIHelpers::Get().GetProfileSwapText();
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo PlayerNameFontInfo = FTrueFPSStyle::Get().GetWidgetStyle<FTextBlockStyle>("TrueFPSSystem.MenuProfileNameStyle").Font;
	const FSlateFontInfo ProfileSwapFontInfo = FTrueFPSStyle::Get().GetWidgetStyle<FTextBlockStyle>("TrueFPSSystem.MenuServerListTextStyle").Font;
	MenuProfileWidth = FMath::Max( FontMeasure->Measure( PlayerName, PlayerNameFontInfo, 1.0f ).X, FontMeasure->Measure( ProfileSwap.ToString(), ProfileSwapFontInfo, 1.0f ).X ) + 32.0f;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Fill)
			.Padding( GetProfileSwapOffset() )
			[
				SNew(SBox)
				.WidthOverride(MenuProfileWidth)
				[
					SNew(SImage)
					.Visibility(this, &STrueFPSMenuWidget::GetProfileSwapVisibility)
					.ColorAndOpacity(this, &STrueFPSMenuWidget::GetHeaderColor)
					.Image(&MenuStyle->HeaderBackgroundBrush)
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Fill)
			.Padding( GetProfileSwapOffset() )
			[
				SNew(SVerticalBox)
				.Visibility(this, &STrueFPSMenuWidget::GetProfileSwapVisibility)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 16.0f, 10.0f, 16.0f, 1.0f )
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.TextStyle(FTrueFPSStyle::Get(), "TrueFPSSystem.MenuProfileNameStyle")
					.ColorAndOpacity(MenuTitleTextColor)
					.Text(PlayerName)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.Padding( 16.0f, 1.0f, 16.0f, 10.0f )
				[
					SNew(STextBlock)
					.TextStyle(FTrueFPSStyle::Get(), "TrueFPSSystem.MenuServerListTextStyle")
					.ColorAndOpacity(MenuTitleTextColor)
					.Text(ProfileSwap)
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(TAttribute<FMargin>(this,&STrueFPSMenuWidget::GetMenuOffset))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Fill)
					[
						SNew(SBox)
						.WidthOverride(MenuHeaderWidth)
						.HeightOverride(MenuHeaderHeight)
						[
							SNew(SImage)
							.ColorAndOpacity(this, &STrueFPSMenuWidget::GetHeaderColor)
							.Image(&MenuStyle->HeaderBackgroundBrush)
						]
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Fill)
					[
						SNew(SBox)
						.WidthOverride(MenuHeaderWidth)
						.HeightOverride(MenuHeaderHeight)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.TextStyle(FTrueFPSStyle::Get(), "TrueFPSSystem.MenuHeaderTextStyle")
							.ColorAndOpacity(MenuTitleTextColor)
							.Text(this,&STrueFPSMenuWidget::GetMenuTitle)
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
					.ColorAndOpacity(this, &STrueFPSMenuWidget::GetBottomColor)
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Left)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SVerticalBox)
							.Clipping(EWidgetClipping::ClipToBounds)

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(TAttribute<FMargin>(this,&STrueFPSMenuWidget::GetLeftMenuOffset))
							[
								SNew(SBorder)
								.BorderImage(&MenuStyle->LeftBackgroundBrush)
								.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
								.Padding(FMargin(OutlineWidth))
								.DesiredSizeScale(this, &STrueFPSMenuWidget::GetBottomScale)
								.VAlign(VAlign_Top)
								.HAlign(HAlign_Left)
								[
									SAssignNew(LeftBox, SVerticalBox)
									.Clipping(EWidgetClipping::ClipToBounds)
								]
							]
						]
						
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SVerticalBox)
							.Clipping(EWidgetClipping::ClipToBounds)

							+ SVerticalBox::Slot()
							.Padding(TAttribute<FMargin>(this,&STrueFPSMenuWidget::GetSubMenuOffset))
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(&MenuStyle->RightBackgroundBrush)
								.BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
								.Padding(FMargin(OutlineWidth))
								.DesiredSizeScale(this, &STrueFPSMenuWidget::GetBottomScale)
								.VAlign(VAlign_Top)
								.HAlign(HAlign_Left)
								[
									SAssignNew(RightBox, SVerticalBox)
									.Clipping(EWidgetClipping::ClipToBounds)
								]
							]
						]
					]
				]
			]
		]
	];
}

EVisibility STrueFPSMenuWidget::GetSlateVisibility() const
{
	return bConsoleVisible ? EVisibility::HitTestInvisible : EVisibility::Visible;
}

FText STrueFPSMenuWidget::GetMenuTitle() const 
{
	return CurrentMenuTitle;
}

FMargin STrueFPSMenuWidget::GetProfileSwapOffset() const
{
	return FMargin(0.0f, 50.0f, 50.0f, 0.0f);
}

bool STrueFPSMenuWidget::IsProfileSwapActive() const
{
#if PROFILE_SWAPPING
	// Dont' show if ingame or not on the main menu screen
	return !bGameMenu && MenuHistory.Num() == 0 ? true : false;
#else
	return false;
#endif
}

EVisibility STrueFPSMenuWidget::GetProfileSwapVisibility() const
{
	return IsProfileSwapActive() ? EVisibility::Visible : EVisibility::Collapsed;
}

UWorld* STrueFPSMenuWidget::GetWorld() const
{
	return PlayerOwner.IsValid() ? PlayerOwner->GetWorld() : nullptr;
}

bool STrueFPSMenuWidget::ProfileUISwap(const int ControllerIndex) const
{
	if (IsProfileSwapActive())
	{
		const FOnLoginUIClosedDelegate Delegate = FOnLoginUIClosedDelegate::CreateSP( const_cast<STrueFPSMenuWidget*>(this), &STrueFPSMenuWidget::HandleProfileUISwapClosed );
		if ( TrueFPSUIHelpers::Get().ProfileSwapUI(GetWorld(), ControllerIndex, false, &Delegate) )
		{
			UTrueFPSGameInstance* GameInstance = PlayerOwner.IsValid() ? Cast< UTrueFPSGameInstance >( PlayerOwner->GetGameInstance() ) : nullptr;

			if ( GameInstance != nullptr )
			{
				GameInstance->SetIgnorePairingChangeForControllerId( ControllerIndex );
			}
			return true;
		}
	}
	return false;
}

void STrueFPSMenuWidget::HandleProfileUISwapClosed(TSharedPtr<const FUniqueNetId> UniqueId, const int ControllerIndex, const FOnlineError& Error)
{
	UTrueFPSGameInstance * GameInstance = PlayerOwner.IsValid() ? Cast< UTrueFPSGameInstance >( PlayerOwner->GetGameInstance() ) : nullptr;

	if ( GameInstance != nullptr )
	{
		GameInstance->SetIgnorePairingChangeForControllerId( -1 );
	}

	// If the id is null, the user backed out
	if( UniqueId.IsValid() && PlayerOwner.IsValid() )
	{
		// If the id is the same, the user picked the existing profile
		// (use the cached unique net id, since we want to compare to the user that was selected at "press start"
		FUniqueNetIdRepl OwnerId = PlayerOwner->GetCachedUniqueNetId();
		if( OwnerId.IsValid() && UniqueId.IsValid() && *OwnerId == *UniqueId)
		{
			return;
		}

		// Go back to the welcome screen.
		HideMenu();
	}

	UTrueFPSLocalPlayer* LocalPlayer = Cast<UTrueFPSLocalPlayer>(PlayerOwner.Get());
	LocalPlayer->LoadPersistentUser();
}

void STrueFPSMenuWidget::LockControls(bool bEnable)
{
	bControlsLocked = bEnable;
}

int32 STrueFPSMenuWidget::GetOwnerUserIndex()
{
	return PlayerOwner.IsValid() ? PlayerOwner->GetControllerId() : 0;
}

int32 STrueFPSMenuWidget::GetMenuLevel()
{
	return MenuHistory.Num();
}

void STrueFPSMenuWidget::BuildAndShowMenu()
{
	//grab the user settings
	UTrueFPSGameUserSettings* UserSettings = CastChecked<UTrueFPSGameUserSettings>(GEngine->GetGameUserSettings());
	ScreenRes = UserSettings->GetScreenResolution();

	//Build left menu panel
	bLeftMenuChanging = false;
	bGoingBack = false;
	BuildLeftPanel(bGoingBack);

	//sets up whole main menu animations and launches them
	SetupAnimations();

	// Set up right side and launch animation if there is any submenu
	if (CurrentMenu.Num() > 0 && CurrentMenu.IsValidIndex(SelectedIndex) && CurrentMenu[SelectedIndex]->bVisible)
	{
		NextMenu = CurrentMenu[SelectedIndex]->SubMenu;
		if (NextMenu.Num() > 0)
		{
			BuildRightPanel();
			bSubMenuChanging = true;
		}
	}

	bMenuHiding = false;
	FSlateApplication::Get().PlaySound(MenuStyle->MenuEnterSound, GetOwnerUserIndex());
}

void STrueFPSMenuWidget::HideMenu()
{
	if (!bMenuHiding)
	{
		if(MenuWidgetAnimation.IsAtEnd())
		{
			MenuWidgetAnimation.PlayReverse(this->AsShared());
		}
		else
		{
			MenuWidgetAnimation.Reverse();
		}
		bMenuHiding = true;
	}
}


void STrueFPSMenuWidget::SetupAnimations()
{
	//Setup a curve
	const float StartDelay = 0.0f;
	const float SecondDelay = bGameMenu ? 0.f : 0.3f;
	const float AnimDuration = 0.5f;
	const float MenuChangeDuration = 0.2f;

	//always animate the menu from the same side of the screen; it looks silly when it disappears to one place and reappears from another
	AnimNumber = 1;

	MenuWidgetAnimation = FCurveSequence();
	SubMenuWidgetAnimation = FCurveSequence();
	SubMenuScrollOutCurve = SubMenuWidgetAnimation.AddCurve(0,MenuChangeDuration,ECurveEaseFunction::QuadInOut);

	MenuWidgetAnimation = FCurveSequence();
	LeftMenuWidgetAnimation = FCurveSequence();
	LeftMenuScrollOutCurve = LeftMenuWidgetAnimation.AddCurve(0,MenuChangeDuration,ECurveEaseFunction::QuadInOut);
	LeftMenuWidgetAnimation.Play(this->AsShared());

	//Define the fade in animation
	TopColorCurve = MenuWidgetAnimation.AddCurve(StartDelay, AnimDuration, ECurveEaseFunction::QuadInOut);

	//now, we want these to animate some time later

	//rolling out
	BottomScaleYCurve = MenuWidgetAnimation.AddCurve(StartDelay+SecondDelay, AnimDuration, ECurveEaseFunction::QuadInOut);
	//fading in
	BottomColorCurve = MenuWidgetAnimation.AddCurve(StartDelay+SecondDelay, AnimDuration, ECurveEaseFunction::QuadInOut);
	//moving from left side off screen
	ButtonsPosXCurve = MenuWidgetAnimation.AddCurve(StartDelay+SecondDelay, AnimDuration, ECurveEaseFunction::QuadInOut);
}

void STrueFPSMenuWidget::BuildLeftPanel(bool bInGoingBack)
{
	if (CurrentMenu.Num() == 0)
	{
		//do not build anything if we do not have any active menu
		return;
	}
	LeftBox->ClearChildren();
	int32 PreviousIndex = -1;
	if (bLeftMenuChanging)
	{
		if (bInGoingBack && MenuHistory.Num() > 0)
		{
			FTrueFPSMenuInfo MenuInfo = MenuHistory.Pop();
			CurrentMenu = MenuInfo.Menu;
			CurrentMenuTitle = MenuInfo.MenuTitle;
			PreviousIndex = MenuInfo.SelectedIndex;
			if (CurrentMenu.Num() > 0 && CurrentMenu[PreviousIndex]->SubMenu.Num() > 0)
			{
				NextMenu = CurrentMenu[PreviousIndex]->SubMenu;
				bSubMenuChanging = true;
			}
		} 
		else if (PendingLeftMenu.Num() > 0)
		{
			MenuHistory.Push(FTrueFPSMenuInfo(CurrentMenu, SelectedIndex, CurrentMenuTitle));
			CurrentMenuTitle = CurrentMenu[SelectedIndex]->GetText();
			CurrentMenu = PendingLeftMenu;
		}
	}
	SelectedIndex = PreviousIndex;
	//Setup the buttons
	for(int32 i = 0; i < CurrentMenu.Num(); ++i)
	{
		if (CurrentMenu[i]->bVisible)
		{
			TSharedPtr<SWidget> TmpWidget;
			if (CurrentMenu[i]->MenuItemType == ETrueFPSMenuItemType::Standard)
			{
				TmpWidget = SAssignNew(CurrentMenu[i]->Widget, STrueFPSMenuItem)
					.PlayerOwner(PlayerOwner)
					.OnClicked(this, &STrueFPSMenuWidget::ButtonClicked, i)
					.Text(CurrentMenu[i]->GetText())
					.bIsMultichoice(false);
			} 
			else if (CurrentMenu[i]->MenuItemType == ETrueFPSMenuItemType::MultiChoice)
			{
				TmpWidget = SAssignNew(CurrentMenu[i]->Widget, STrueFPSMenuItem)
					.PlayerOwner(PlayerOwner)
					.OnClicked(this, &STrueFPSMenuWidget::ButtonClicked, i)
					.Text(CurrentMenu[i]->GetText() )
					.bIsMultichoice(true)
					.OnArrowPressed(this, &STrueFPSMenuWidget::ChangeOption)
					.OptionText(this, &STrueFPSMenuWidget::GetOptionText, CurrentMenu[i]);
				UpdateArrows(CurrentMenu[i]);
			}
			else if (CurrentMenu[i]->MenuItemType == ETrueFPSMenuItemType::CustomWidget)
			{
				TmpWidget = CurrentMenu[i]->CustomWidget;
			}
			if (TmpWidget.IsValid())
			{
				//set first selection to first valid widget
				if (SelectedIndex == -1)
				{
					SelectedIndex = i;
				}
				LeftBox->AddSlot()	.HAlign(HAlign_Left)	.AutoHeight()
				[
					TmpWidget.ToSharedRef()
				];
			}
		}
	}
	
	
	TSharedPtr<FTrueFPSMenuItem> FirstMenuItem = CurrentMenu.IsValidIndex(SelectedIndex) ? CurrentMenu[SelectedIndex] : nullptr;
	if (FirstMenuItem.IsValid() && FirstMenuItem->MenuItemType != ETrueFPSMenuItemType::CustomWidget)
	{
		FirstMenuItem->Widget->SetMenuItemActive(true);
		FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
	}
}

FText STrueFPSMenuWidget::GetOptionText(TSharedPtr<FTrueFPSMenuItem> MenuItem) const
{
	FText Result = FText::GetEmpty();
	if (MenuItem->SelectedMultiChoice > -1 && MenuItem->SelectedMultiChoice < MenuItem->MultiChoice.Num())
	{
		Result = MenuItem->MultiChoice[MenuItem->SelectedMultiChoice];
	}
	return Result;
}

void STrueFPSMenuWidget::BuildRightPanel()
{
	RightBox->ClearChildren();
	
	if (NextMenu.Num() == 0) return;

	for(int32 i = 0; i < NextMenu.Num(); ++i)
	{
		if (NextMenu[i]->bVisible)
		{
			TSharedPtr<STrueFPSMenuItem> TmpButton;
			//Custom menu items are not supported in the right panel
			if (NextMenu[i]->MenuItemType == ETrueFPSMenuItemType::Standard)
			{
				TmpButton = SAssignNew(NextMenu[i]->Widget, STrueFPSMenuItem)
					.PlayerOwner(PlayerOwner)
					.Text(NextMenu[i]->GetText())
					.InactiveTextAlpha(0.3f)
					.bIsMultichoice(false);
			}
			else if (NextMenu[i]->MenuItemType == ETrueFPSMenuItemType::MultiChoice)
			{
				TmpButton = SAssignNew(NextMenu[i]->Widget, STrueFPSMenuItem)
					.PlayerOwner(PlayerOwner)
					.Text(NextMenu[i]->GetText() )
					.InactiveTextAlpha(0.3f)
					.bIsMultichoice(true)
					.OptionText(this, &STrueFPSMenuWidget::GetOptionText, NextMenu[i]);
			}
			if(TmpButton.IsValid())
			{
				RightBox->AddSlot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					TmpButton.ToSharedRef()
				];
			}
		}
	}
}

void STrueFPSMenuWidget::UpdateArrows(TSharedPtr<FTrueFPSMenuItem> MenuItem)
{
	const int32 MinIndex = MenuItem->MinMultiChoiceIndex > -1 ? MenuItem->MinMultiChoiceIndex : 0;
	const int32 MaxIndex = MenuItem->MaxMultiChoiceIndex > -1 ? MenuItem->MaxMultiChoiceIndex : MenuItem->MultiChoice.Num()-1;
	const int32 CurIndex = MenuItem->SelectedMultiChoice;
	if (CurIndex > MinIndex)
	{
		MenuItem->Widget->LeftArrowVisible = EVisibility::Visible;
	} 
	else
	{
		MenuItem->Widget->LeftArrowVisible = EVisibility::Collapsed;
	}
	if (CurIndex < MaxIndex)
	{
		MenuItem->Widget->RightArrowVisible = EVisibility::Visible;
	}
	else
	{
		MenuItem->Widget->RightArrowVisible = EVisibility::Collapsed;
	}
}

void STrueFPSMenuWidget::EnterSubMenu()
{
	bLeftMenuChanging = true;
	bGoingBack = false;
	FSlateApplication::Get().PlaySound(MenuStyle->MenuEnterSound, GetOwnerUserIndex());
}

void STrueFPSMenuWidget::MenuGoBack(bool bSilent)
{
	if (MenuHistory.Num() > 0)
	{
		if (!bSilent)
		{
			FSlateApplication::Get().PlaySound(MenuStyle->MenuBackSound, GetOwnerUserIndex());
		}
		bLeftMenuChanging = true;
		bGoingBack = true;
		OnGoBack.ExecuteIfBound(CurrentMenu);
	}
	else if (bGameMenu) // only when it's in-game menu variant
	{
		if (!bSilent)
		{
			FSlateApplication::Get().PlaySound(MenuStyle->MenuBackSound, GetOwnerUserIndex());
		}
		OnToggleMenu.ExecuteIfBound();
	}
	else
	{
#if TRUEFPS_CONSOLE_UI
		// Go back to the welcome screen.
		HideMenu();
#endif
	}
}

void STrueFPSMenuWidget::ConfirmMenuItem()
{
	if (CurrentMenu[SelectedIndex]->OnConfirmMenuItem.IsBound())
	{
		CurrentMenu[SelectedIndex]->OnConfirmMenuItem.Execute();
	} 
	else if (CurrentMenu[SelectedIndex]->SubMenu.Num() > 0)
	{
		EnterSubMenu();
	}
}

void STrueFPSMenuWidget::ControllerFacebuttonLeftPressed()
{
	if (CurrentMenu[SelectedIndex]->OnControllerFacebuttonLeftPressed.IsBound())
	{
		CurrentMenu[SelectedIndex]->OnControllerFacebuttonLeftPressed.Execute();
	} 
}

void STrueFPSMenuWidget::ControllerUpInputPressed()
{
	if (CurrentMenu[SelectedIndex]->OnControllerUpInputPressed.IsBound())
	{
		CurrentMenu[SelectedIndex]->OnControllerUpInputPressed.Execute();
	}
}

void STrueFPSMenuWidget::ControllerDownInputPressed()
{
	if (CurrentMenu[SelectedIndex]->OnControllerDownInputPressed.IsBound())
	{
		CurrentMenu[SelectedIndex]->OnControllerDownInputPressed.Execute();
	}
}

void STrueFPSMenuWidget::ControllerFacebuttonDownPressed()
{
	if (CurrentMenu[SelectedIndex]->OnControllerFacebuttonDownPressed.IsBound())
	{
		CurrentMenu[SelectedIndex]->OnControllerFacebuttonDownPressed.Execute();
	}
}

void STrueFPSMenuWidget::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	//Always tick the super
	SCompoundWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );

	//ugly code seeing if the console is open
	UConsole* ViewportConsole = (GEngine !=nullptr && GEngine->GameViewport != nullptr) ? GEngine->GameViewport->ViewportConsole : nullptr;
	if (ViewportConsole != nullptr && (ViewportConsole->ConsoleState == "Typing" || ViewportConsole->ConsoleState == "Open"))
	{
		if (!bConsoleVisible)
		{
			bConsoleVisible = true;
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
	else
	{
		if (bConsoleVisible)
		{
			bConsoleVisible = false;
			FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
		}
	}

	if (GEngine && GEngine->GameViewport)
	{
		FVector2D Size = FVector2D::ZeroVector;
		GEngine->GameViewport->GetViewportSize(Size);
		if (!Size.IsZero())
		{
			ScreenRes = (Size / AllottedGeometry.Scale).IntPoint();
		}
	}

	if (MenuWidgetAnimation.IsAtStart() && !bMenuHiding)
	{
		//Start the menu widget animation, set keyboard focus
		FadeIn();
	}
	else if (MenuWidgetAnimation.IsAtStart() && bMenuHiding)
	{
		bMenuHiding = false;
		//Send event, so menu can be removed
		OnMenuHidden.ExecuteIfBound();
	}

	if (MenuWidgetAnimation.IsAtEnd())
	{
		if (bLeftMenuChanging)
		{
			if (LeftMenuWidgetAnimation.IsAtEnd())
			{
				PendingLeftMenu = NextMenu;
				if (NextMenu.Num() > 0
					&& NextMenu.Top()->SubMenu.Num() > 0)
				{
					NextMenu = NextMenu.Top()->SubMenu;
				} 
				else 
				{
					NextMenu.Reset();
				}
				bSubMenuChanging = true;

				LeftMenuWidgetAnimation.PlayReverse(this->AsShared());
			}
			if (!LeftMenuWidgetAnimation.IsPlaying())
			{
				if (CurrentMenu.Num() > 0)
				{
					BuildLeftPanel(bGoingBack);
					LeftMenuWidgetAnimation.Play(this->AsShared());
				}
				//Focus the custom widget
				if (CurrentMenu.Num() == 1 && CurrentMenu.Top()->MenuItemType == ETrueFPSMenuItemType::CustomWidget)
				{
					FSlateApplication::Get().SetKeyboardFocus(CurrentMenu.Top()->CustomWidget);
				}
				bLeftMenuChanging = false;
				RightBox->ClearChildren();
			}
		}
		if (bSubMenuChanging)
		{
			if (SubMenuWidgetAnimation.IsAtEnd())
			{
				SubMenuWidgetAnimation.PlayReverse(this->AsShared());
			}
			if (!SubMenuWidgetAnimation.IsPlaying())
			{
				if (NextMenu.Num() > 0)
				{
					BuildRightPanel();
					SubMenuWidgetAnimation.Play(this->AsShared());
				}
				bSubMenuChanging = false;
			}
		}
	}
}

FMargin STrueFPSMenuWidget::GetMenuOffset() const
{
	const float WidgetWidth = LeftBox->GetDesiredSize().X + RightBox->GetDesiredSize().X;
	const float WidgetHeight = LeftBox->GetDesiredSize().Y + MenuHeaderHeight;
	const float OffsetX = (ScreenRes.X - WidgetWidth - OutlineWidth*2)/2;
	const float AnimProgress = ButtonsPosXCurve.GetLerp();
	FMargin Result;

	switch (AnimNumber)
	{
		case 0:
			Result = FMargin(OffsetX + ScreenRes.X - AnimProgress*ScreenRes.X, (ScreenRes.Y - WidgetHeight)/2, 0, 0);
		break;
		case 1:
			Result = FMargin(OffsetX - ScreenRes.X + AnimProgress*ScreenRes.X, (ScreenRes.Y - WidgetHeight)/2, 0, 0);
		break;
		case 2:
			Result = FMargin(OffsetX, (ScreenRes.Y - WidgetHeight)/2 + ScreenRes.Y - AnimProgress*ScreenRes.Y, 0, 0);
		break;
		case 3:
			Result = FMargin(OffsetX, (ScreenRes.Y - WidgetHeight)/2 + -ScreenRes.Y + AnimProgress*ScreenRes.Y, 0, 0);
		break;
	}
	return Result;
}

FMargin STrueFPSMenuWidget::GetLeftMenuOffset() const
{
	const float LeftBoxSizeX = LeftBox->GetDesiredSize().X + OutlineWidth * 2;
	return FMargin(0, 0,-LeftBoxSizeX + LeftMenuScrollOutCurve.GetLerp() * LeftBoxSizeX,0);
}

FMargin STrueFPSMenuWidget::GetSubMenuOffset() const
{
	const float RightBoxSizeX = RightBox->GetDesiredSize().X + OutlineWidth * 2;
	return FMargin(0, 0,-RightBoxSizeX + SubMenuScrollOutCurve.GetLerp() * RightBoxSizeX,0);
}


FVector2D STrueFPSMenuWidget::GetBottomScale() const
{
	return FVector2D(BottomScaleYCurve.GetLerp(), BottomScaleYCurve.GetLerp());
}

FLinearColor STrueFPSMenuWidget::GetBottomColor() const
{
	return FMath::Lerp(FLinearColor(1,1,1,0), FLinearColor(1,1,1,1), BottomColorCurve.GetLerp());
}

FLinearColor STrueFPSMenuWidget::GetTopColor() const
{
	return FMath::Lerp(FLinearColor(1,1,1,0), FLinearColor(1,1,1,1), TopColorCurve.GetLerp());
}

FSlateColor STrueFPSMenuWidget::GetHeaderColor() const
{
	return CurrentMenuTitle.IsEmpty() ? FLinearColor::Transparent : FLinearColor::White;
}

FReply STrueFPSMenuWidget::ButtonClicked(int32 ButtonIndex)
{
	if (bControlsLocked)
	{
		return FReply::Handled();
	}

	if (SelectedIndex != ButtonIndex)
	{
		TSharedPtr<STrueFPSMenuItem> MenuItem = CurrentMenu[SelectedIndex]->Widget;
		MenuItem->SetMenuItemActive(false);
		SelectedIndex = ButtonIndex;
		MenuItem = CurrentMenu[SelectedIndex]->Widget;
		MenuItem->SetMenuItemActive(true);
		NextMenu = CurrentMenu[SelectedIndex]->SubMenu;
		bSubMenuChanging = true;
		FSlateApplication::Get().PlaySound(MenuStyle->MenuItemChangeSound, GetOwnerUserIndex());
	}
	else if (SelectedIndex == ButtonIndex)
	{
		ConfirmMenuItem();
	}

	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
}

void STrueFPSMenuWidget::FadeIn()
{
	//Start the menu widget playing
	MenuWidgetAnimation.Play(this->AsShared());

	//Go into UI mode
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
}

FReply STrueFPSMenuWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	//If we clicked anywhere, jump to the end
	if(MenuWidgetAnimation.IsPlaying())
	{
		MenuWidgetAnimation.JumpToEnd();
	}

	//Set the keyboard focus 
	return FReply::Handled()
		.SetUserFocus(SharedThis(this), EFocusCause::SetDirectly);
}

void STrueFPSMenuWidget::ChangeOption(int32 MoveBy)
{
	TSharedPtr<FTrueFPSMenuItem> MenuItem = CurrentMenu[SelectedIndex];

	const int32 MinIndex = MenuItem->MinMultiChoiceIndex > -1 ? MenuItem->MinMultiChoiceIndex : 0;
	const int32 MaxIndex = MenuItem->MaxMultiChoiceIndex > -1 ? MenuItem->MaxMultiChoiceIndex : MenuItem->MultiChoice.Num()-1;
	const int32 CurIndex = MenuItem->SelectedMultiChoice;

	if (MenuItem->MenuItemType == ETrueFPSMenuItemType::MultiChoice)
	{
		if ( CurIndex + MoveBy >= MinIndex && CurIndex + MoveBy <= MaxIndex)
		{
			MenuItem->SelectedMultiChoice += MoveBy;
			MenuItem->OnOptionChanged.ExecuteIfBound(MenuItem, MenuItem->SelectedMultiChoice);
			FSlateApplication::Get().PlaySound(MenuStyle->OptionChangeSound, GetOwnerUserIndex());
		}
		UpdateArrows(MenuItem);
	}
}

int32 STrueFPSMenuWidget::GetNextValidIndex(int32 MoveBy)
{
	int32 Result = SelectedIndex;
	if (MoveBy != 0 && SelectedIndex + MoveBy > -1 && SelectedIndex+MoveBy < CurrentMenu.Num())
	{
		Result = SelectedIndex + MoveBy;
		//look for non-hidden menu item
		while (!CurrentMenu[Result]->Widget.IsValid())
		{
			MoveBy > 0 ? Result++ : Result--;
			//when moved outside of array, just return current selection
			if (!CurrentMenu.IsValidIndex(Result))
			{
				Result = SelectedIndex;
				break;
			}
		}
	}
	return Result;
}

FReply STrueFPSMenuWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Result = FReply::Unhandled();
	const int32 UserIndex = InKeyEvent.GetUserIndex();
	bool bEventUserCanInteract = GetOwnerUserIndex() == -1 || UserIndex == GetOwnerUserIndex();

	if (!bControlsLocked && bEventUserCanInteract)
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
		{
			ControllerUpInputPressed();
			int32 NextValidIndex = GetNextValidIndex(-1);
			if (NextValidIndex != SelectedIndex)
			{
				ButtonClicked(NextValidIndex);
			}
			Result = FReply::Handled();
		}
		else if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
		{
			ControllerDownInputPressed();
			int32 NextValidIndex = GetNextValidIndex(1);
			if (NextValidIndex != SelectedIndex)
			{
				ButtonClicked(NextValidIndex);
			}
			Result = FReply::Handled();
		}
		else if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
		{
			ChangeOption(-1);
			Result = FReply::Handled();
		}
		else if (Key == EKeys::Right ||Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
		{
			ChangeOption(1);
			Result = FReply::Handled();
		}
		else if (Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::SpaceBar)
		{
			ProfileUISwap(UserIndex);
			Result = FReply::Handled();
		}
		else if (Key == EKeys::Enter)
		{
			ConfirmMenuItem();
			Result = FReply::Handled();
		}
		else if (Key == EKeys::Virtual_Accept && !InKeyEvent.IsRepeat())
		{
			ControllerFacebuttonDownPressed();
			ConfirmMenuItem();
			Result = FReply::Handled();
		}
		else if ((Key == EKeys::Escape || Key == EKeys::Virtual_Back || Key == EKeys::Gamepad_Special_Left || Key == EKeys::Global_Back || Key == EKeys::Global_View) && !InKeyEvent.IsRepeat())
		{
			MenuGoBack();
			Result = FReply::Handled();
		}
		else if (Key == EKeys::Gamepad_FaceButton_Left)
		{
			ControllerFacebuttonLeftPressed();
			Result = FReply::Handled();
		}
		else if ((Key == ControllerHideMenuKey || Key == EKeys::Global_Play || Key == EKeys::Global_Menu) && !InKeyEvent.IsRepeat())
		{
			OnToggleMenu.ExecuteIfBound();
			Result = FReply::Handled();
		}
	}
	return Result;
}

FReply STrueFPSMenuWidget::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	//Focus the custom widget
	if (CurrentMenu.Num() == 1 && CurrentMenu.Top()->MenuItemType == ETrueFPSMenuItemType::CustomWidget)
	{
		return FReply::Handled().SetUserFocus(CurrentMenu.Top()->CustomWidget.ToSharedRef(), EFocusCause::SetDirectly);
	}

	return FReply::Handled().ReleaseMouseCapture().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly, true);
}

#undef LOCTEXT_NAMESPACE
