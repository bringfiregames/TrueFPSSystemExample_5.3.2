// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSIngameMenu.h"
#include "Online.h"
#include "OnlineSubsystemUtils.h"
#include "TrueFPSGameInstance.h"
#include "TrueFPSOptions.h"
#include "TrueFPSRecentlyMet.h"
#include "TrueFPSSystem.h"
#include "Character/TrueFPSPlayerController.h"
#include "UI/TrueFPSHUD.h"
#include "UI/Style/TrueFPSMenuSoundsWidgetStyle.h"
#include "UI/Style/TrueFPSStyle.h"

#define LOCTEXT_NAMESPACE "TrueFPSSystem.HUD.Menu"

#if PLATFORM_SWITCH
#	define FRIENDS_SUPPORTED 0
#else
#	define FRIENDS_SUPPORTED 1
#endif

#if !defined(FRIENDS_IN_INGAME_MENU)
	#define FRIENDS_IN_INGAME_MENU 1
#endif

void FTrueFPSIngameMenu::Construct(ULocalPlayer* _PlayerOwner)
{
	PlayerOwner = _PlayerOwner;
	bIsGameMenuUp = false;

	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}
	
	//todo:  don't create ingame menus for remote players.
	const UTrueFPSGameInstance* GameInstance = nullptr;
	if (PlayerOwner)
	{
		GameInstance = Cast<UTrueFPSGameInstance>(PlayerOwner->GetGameInstance());
	}

	if (!GameMenuWidget.IsValid())
	{
		SAssignNew(GameMenuWidget, STrueFPSMenuWidget)
			.PlayerOwner(MakeWeakObjectPtr(PlayerOwner))
			.Cursor(EMouseCursor::Default)
			.IsGameMenu(true);			


		int32 const OwnerUserIndex = GetOwnerUserIndex();

		// setup the exit to main menu submenu.  We wanted a confirmation to avoid a potential TRC violation.
		// fixes TTP: 322267
		TSharedPtr<FTrueFPSMenuItem> MainMenuRoot = FTrueFPSMenuItem::CreateRoot();
		MainMenuItem = MenuHelper::AddMenuItem(MainMenuRoot,LOCTEXT("Main Menu", "MAIN MENU"));
		MenuHelper::AddMenuItemSP(MainMenuItem,LOCTEXT("No", "NO"), this, &FTrueFPSIngameMenu::OnCancelExitToMain);
		MenuHelper::AddMenuItemSP(MainMenuItem,LOCTEXT("Yes", "YES"), this, &FTrueFPSIngameMenu::OnConfirmExitToMain);		

		TrueFPSOptions = MakeShareable(new FTrueFPSOptions());
		TrueFPSOptions->Construct(PlayerOwner);
		TrueFPSOptions->TellInputAboutKeybindings();
		TrueFPSOptions->OnApplyChanges.BindSP(this, &FTrueFPSIngameMenu::CloseSubMenu);

		MenuHelper::AddExistingMenuItem(RootMenuItem, TrueFPSOptions->CheatsItem.ToSharedRef());
		MenuHelper::AddExistingMenuItem(RootMenuItem, TrueFPSOptions->OptionsItem.ToSharedRef());

#if FRIENDS_SUPPORTED
		if (GameInstance && GameInstance->GetOnlineMode() == EOnlineMode::Online)
		{
#if !FRIENDS_IN_INGAME_MENU
			TrueFPSFriends = MakeShareable(new FTrueFPSFriends());
			TrueFPSFriends->Construct(PlayerOwner, OwnerUserIndex);
			TrueFPSFriends->TellInputAboutKeybindings();
			TrueFPSFriends->OnApplyChanges.BindSP(this, &FTrueFPSIngameMenu::CloseSubMenu);

			MenuHelper::AddExistingMenuItem(RootMenuItem, TrueFPSFriends->FriendsItem.ToSharedRef());

			TrueFPSRecentlyMet = MakeShareable(new FTrueFPSRecentlyMet());
			TrueFPSRecentlyMet->Construct(PlayerOwner, OwnerUserIndex);
			TrueFPSRecentlyMet->TellInputAboutKeybindings();
			TrueFPSRecentlyMet->OnApplyChanges.BindSP(this, &FTrueFPSIngameMenu::CloseSubMenu);

			MenuHelper::AddExistingMenuItem(RootMenuItem, TrueFPSRecentlyMet->RecentlyMetItem.ToSharedRef());
#endif		

#if TRUEFPS_CONSOLE_UI && INVITE_ONLINE_GAME_ENABLED			
			TSharedPtr<FTrueFPSMenuItem> ShowInvitesItem = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Invite Players", "INVITE PLAYERS (via System UI)"));
			ShowInvitesItem->OnConfirmMenuItem.BindRaw(this, &FTrueFPSIngameMenu::OnShowInviteUI);		
#endif
		}
#endif

		if (FSlateApplication::Get().SupportsSystemHelp())
		{
			TSharedPtr<FTrueFPSMenuItem> HelpSubMenu = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Help", "HELP"));
			HelpSubMenu->OnConfirmMenuItem.BindStatic([](){ FSlateApplication::Get().ShowSystemHelp(); });
		}

		MenuHelper::AddExistingMenuItem(RootMenuItem, MainMenuItem.ToSharedRef());
				
#if !TRUEFPS_CONSOLE_UI
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Quit", "QUIT"), this, &FTrueFPSIngameMenu::OnUIQuit);
#endif

		GameMenuWidget->MainMenu = GameMenuWidget->CurrentMenu = RootMenuItem->SubMenu;
		GameMenuWidget->OnMenuHidden.BindSP(this,&FTrueFPSIngameMenu::DetachGameMenu);
		GameMenuWidget->OnToggleMenu.BindSP(this,&FTrueFPSIngameMenu::ToggleGameMenu);
		GameMenuWidget->OnGoBack.BindSP(this, &FTrueFPSIngameMenu::OnMenuGoBack);
	}
}

void FTrueFPSIngameMenu::CloseSubMenu()
{
	GameMenuWidget->MenuGoBack();
}

void FTrueFPSIngameMenu::OnMenuGoBack(MenuPtr Menu)
{
	// if we are going back from options menu
	if (TrueFPSOptions.IsValid() && TrueFPSOptions->OptionsItem->SubMenu == Menu)
	{
		TrueFPSOptions->RevertChanges();
	}
}

bool FTrueFPSIngameMenu::GetIsGameMenuUp() const
{
	return bIsGameMenuUp;
}

void FTrueFPSIngameMenu::UpdateFriendsList()
{
	if (PlayerOwner)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
		if (OnlineSub)
		{
			IOnlineFriendsPtr OnlineFriendsPtr = OnlineSub->GetFriendsInterface();
			if (OnlineFriendsPtr.IsValid())
			{
				OnlineFriendsPtr->ReadFriendsList(GetOwnerUserIndex(), EFriendsLists::ToString(EFriendsLists::OnlinePlayers));
			}
		}
	}
}

void FTrueFPSIngameMenu::DetachGameMenu()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(GameMenuContainer.ToSharedRef());
	}
	bIsGameMenuUp = false;

	ATrueFPSPlayerController* const PCOwner = PlayerOwner ? Cast<ATrueFPSPlayerController>(PlayerOwner->PlayerController) : nullptr;
	if (PCOwner)
	{
		PCOwner->SetPause(false);

		// If the game is over enable the scoreboard
		ATrueFPSHUD* const TrueFPSHUD = PCOwner->GetTrueFPSHUD();
		if( ( TrueFPSHUD != nullptr ) && ( TrueFPSHUD->IsMatchOver() == true ) && ( PCOwner->IsPrimaryPlayer() == true ) )
		{
			TrueFPSHUD->ShowScoreboard( true, true );
		}
	}
}

void FTrueFPSIngameMenu::ToggleGameMenu()
{
	//Update the owner in case the menu was opened by another controller
	//UpdateMenuOwner();

	if (!GameMenuWidget.IsValid())
	{
		return;
	}

	// check for a valid user index.  could be invalid if the user signed out, in which case the 'please connect your control' ui should be up anyway.
	// in-game menu needs a valid userindex for many OSS calls.
	if (GetOwnerUserIndex() == -1)
	{
		UE_LOG(LogTrueFPSSystem, Log, TEXT("Trying to toggle in-game menu for invalid userid"));
		return;
	}

	if (bIsGameMenuUp && GameMenuWidget->CurrentMenu != RootMenuItem->SubMenu)
	{
		GameMenuWidget->MenuGoBack();
		return;
	}
	
	ATrueFPSPlayerController* const PCOwner = PlayerOwner ? Cast<ATrueFPSPlayerController>(PlayerOwner->PlayerController) : nullptr;
	if (!bIsGameMenuUp)
	{
		// Hide the scoreboard
		if (PCOwner)
		{
			ATrueFPSHUD* const TrueFPSHUD = PCOwner->GetTrueFPSHUD();
			if( TrueFPSHUD != nullptr )
			{
				TrueFPSHUD->ShowScoreboard( false );
			}
		}

		GEngine->GameViewport->AddViewportWidgetContent(
			SAssignNew(GameMenuContainer,SWeakWidget)
			.PossiblyNullContent(GameMenuWidget.ToSharedRef())
			);

		int32 const OwnerUserIndex = GetOwnerUserIndex();
		if(TrueFPSOptions.IsValid())
		{
			TrueFPSOptions->UpdateOptions();
		}
		if(TrueFPSRecentlyMet.IsValid())
		{
			TrueFPSRecentlyMet->UpdateRecentlyMet(OwnerUserIndex);
		}
		GameMenuWidget->BuildAndShowMenu();
		bIsGameMenuUp = true;

		if (PCOwner)
		{
			// Disable controls while paused
			PCOwner->SetCinematicMode(true, false, false, true, true);

			if (PCOwner->SetPause(true))
			{
				UTrueFPSGameInstance* GameInstance = Cast<UTrueFPSGameInstance>(PlayerOwner->GetGameInstance());
				GameInstance->SetPresenceForLocalPlayers(FString(TEXT("On Pause")), FVariantData(FString(TEXT("Paused"))));
			}
			
			FInputModeGameAndUI InputMode;
			PCOwner->SetInputMode(InputMode);
		}
	} 
	else
	{
		//Start hiding animation
		GameMenuWidget->HideMenu();
		if (PCOwner)
		{
			// Make sure viewport has focus
			FSlateApplication::Get().SetAllUserFocusToGameViewport();

			if (PCOwner->SetPause(false))
			{
				UTrueFPSGameInstance* GameInstance = Cast<UTrueFPSGameInstance>(PlayerOwner->GetGameInstance());
				GameInstance->SetPresenceForLocalPlayers(FString(TEXT("In Game")), FVariantData(FString(TEXT("InGame"))));
			}

			// Don't renable controls if the match is over
			ATrueFPSHUD* const TrueFPSHUD = PCOwner->GetTrueFPSHUD();
			if( ( TrueFPSHUD != nullptr ) && ( TrueFPSHUD->IsMatchOver() == false ) )
			{
				PCOwner->SetCinematicMode(false,false,false,true,true);

				FInputModeGameOnly InputMode;
				PCOwner->SetInputMode(InputMode);
			}
		}
	}
}

void FTrueFPSIngameMenu::OnCancelExitToMain()
{
	CloseSubMenu();
}

void FTrueFPSIngameMenu::OnConfirmExitToMain()
{
	UTrueFPSGameInstance* const GameInstance = Cast<UTrueFPSGameInstance>(PlayerOwner->GetGameInstance());
	if (GameInstance)
	{
		GameInstance->LabelPlayerAsQuitter(PlayerOwner);

		// tell game instance to go back to main menu state
		GameInstance->GotoState(TrueFPSGameInstanceState::MainMenu);
	}
}

void FTrueFPSIngameMenu::OnUIQuit()
{
	UTrueFPSGameInstance* const GI = Cast<UTrueFPSGameInstance>(PlayerOwner->GetGameInstance());
	if (GI)
	{
		GI->LabelPlayerAsQuitter(PlayerOwner);
	}

	GameMenuWidget->LockControls(true);
	GameMenuWidget->HideMenu();

	UWorld* const World = PlayerOwner ? PlayerOwner->GetWorld() : nullptr;
	if (World)
	{
		const FTrueFPSMenuSoundsStyle& MenuSounds = FTrueFPSStyle::Get().GetWidgetStyle<FTrueFPSMenuSoundsStyle>("DefaultTrueFPSMenuSoundsStyle");
		MenuHelper::PlaySoundAndCall(World, MenuSounds.ExitGameSound, GetOwnerUserIndex(), this, &FTrueFPSIngameMenu::Quit);
	}
}

void FTrueFPSIngameMenu::Quit()
{
	APlayerController* const PCOwner = PlayerOwner ? PlayerOwner->PlayerController : nullptr;
	if (PCOwner)
	{
		PCOwner->ConsoleCommand("quit");
	}
}

void FTrueFPSIngameMenu::OnShowInviteUI()
{
	if (PlayerOwner)
	{
		const IOnlineExternalUIPtr ExternalUI = Online::GetExternalUIInterface(PlayerOwner->GetWorld());

		if (!ExternalUI.IsValid())
		{
			UE_LOG(LogTrueFPSSystem, Warning, TEXT("OnShowInviteUI: External UI interface is not supported on this platform."));
			return;
		}

		ExternalUI->ShowInviteUI(GetOwnerUserIndex());
	}
}

int32 FTrueFPSIngameMenu::GetOwnerUserIndex() const
{
	return PlayerOwner ? PlayerOwner->GetControllerId() : 0;
}


#undef LOCTEXT_NAMESPACE
