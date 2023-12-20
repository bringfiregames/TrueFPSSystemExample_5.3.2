// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSDemoPlaybackMenu.h"

#include "TrueFPSSystem.h"
#include "TrueFPSGameInstance.h"
#include "Character/TrueFPSPlayerController.h"
#include "UI/Style/TrueFPSMenuSoundsWidgetStyle.h"
#include "UI/Style/TrueFPSStyle.h"
#include "Widgets/STrueFPSMenuWidget.h"

#define LOCTEXT_NAMESPACE "TrueFPSSystem.HUD.Menu"

void FTrueFPSDemoPlaybackMenu::Construct( ULocalPlayer* _PlayerOwner )
{
	PlayerOwner = _PlayerOwner;
	bIsAddedToViewport = false;

	if ( !GEngine || !GEngine->GameViewport )
	{
		return;
	}
	
	if ( !GameMenuWidget.IsValid() )
	{
		SAssignNew( GameMenuWidget, STrueFPSMenuWidget )
			.PlayerOwner( MakeWeakObjectPtr( PlayerOwner ) )
			.Cursor( EMouseCursor::Default )
			.IsGameMenu( true );			

		TSharedPtr<FTrueFPSMenuItem> MainMenuRoot = FTrueFPSMenuItem::CreateRoot();
		MainMenuItem = MenuHelper::AddMenuItem(MainMenuRoot,LOCTEXT( "Main Menu", "MAIN MENU" ) );

		MenuHelper::AddMenuItemSP( MainMenuItem, LOCTEXT( "No", "NO" ), this, &FTrueFPSDemoPlaybackMenu::OnCancelExitToMain );
		MenuHelper::AddMenuItemSP( MainMenuItem, LOCTEXT( "Yes", "YES" ), this, &FTrueFPSDemoPlaybackMenu::OnConfirmExitToMain );

		MenuHelper::AddExistingMenuItem( RootMenuItem, MainMenuItem.ToSharedRef() );
				
#if !TRUEFPS_CONSOLE_UI
		MenuHelper::AddMenuItemSP( RootMenuItem, LOCTEXT("Quit", "QUIT"), this, &FTrueFPSDemoPlaybackMenu::OnUIQuit );
#endif

		GameMenuWidget->MainMenu = GameMenuWidget->CurrentMenu = RootMenuItem->SubMenu;
		GameMenuWidget->OnMenuHidden.BindSP( this, &FTrueFPSDemoPlaybackMenu::DetachGameMenu );
		GameMenuWidget->OnToggleMenu.BindSP( this, &FTrueFPSDemoPlaybackMenu::ToggleGameMenu );
		GameMenuWidget->OnGoBack.BindSP( this, &FTrueFPSDemoPlaybackMenu::OnMenuGoBack );
	}
}

void FTrueFPSDemoPlaybackMenu::CloseSubMenu()
{
	GameMenuWidget->MenuGoBack();
}

void FTrueFPSDemoPlaybackMenu::OnMenuGoBack(MenuPtr Menu)
{
}

void FTrueFPSDemoPlaybackMenu::DetachGameMenu()
{
	if ( GEngine && GEngine->GameViewport )
	{
		GEngine->GameViewport->RemoveViewportWidgetContent( GameMenuContainer.ToSharedRef() );
	}

	bIsAddedToViewport = false;
}

void FTrueFPSDemoPlaybackMenu::ToggleGameMenu()
{
	if ( !GameMenuWidget.IsValid( ))
	{
		return;
	}

	if ( bIsAddedToViewport && GameMenuWidget->CurrentMenu != RootMenuItem->SubMenu )
	{
		GameMenuWidget->MenuGoBack();
		return;
	}
	
	if ( !bIsAddedToViewport )
	{
		GEngine->GameViewport->AddViewportWidgetContent( SAssignNew( GameMenuContainer, SWeakWidget ).PossiblyNullContent( GameMenuWidget.ToSharedRef() ) );

		GameMenuWidget->BuildAndShowMenu();

		bIsAddedToViewport = true;
	} 
	else
	{
		// Start hiding animation
		GameMenuWidget->HideMenu();

		ATrueFPSPlayerController* const PCOwner = PlayerOwner ? Cast<ATrueFPSPlayerController>(PlayerOwner->PlayerController) : nullptr;

		if ( PCOwner )
		{
			// Make sure viewport has focus
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
}

void FTrueFPSDemoPlaybackMenu::OnCancelExitToMain()
{
	CloseSubMenu();
}

void FTrueFPSDemoPlaybackMenu::OnConfirmExitToMain()
{
	UTrueFPSGameInstance* const GameInstance = Cast<UTrueFPSGameInstance>( PlayerOwner->GetGameInstance() );

	if ( GameInstance )
	{
		// tell game instance to go back to main menu state
		GameInstance->GotoState( TrueFPSGameInstanceState::MainMenu );
	}
}

void FTrueFPSDemoPlaybackMenu::OnUIQuit()
{
	UTrueFPSGameInstance* const GameInstance = Cast<UTrueFPSGameInstance>( PlayerOwner->GetGameInstance() );

	GameMenuWidget->LockControls( true );
	GameMenuWidget->HideMenu();

	UWorld* const World = PlayerOwner ? PlayerOwner->GetWorld() : nullptr;
	if ( World )
	{
		const FTrueFPSMenuSoundsStyle& MenuSounds = FTrueFPSStyle::Get().GetWidgetStyle< FTrueFPSMenuSoundsStyle >( "DefaultTrueFPSMenuSoundsStyle" );
		MenuHelper::PlaySoundAndCall( World, MenuSounds.ExitGameSound, GetOwnerUserIndex(), this, &FTrueFPSDemoPlaybackMenu::Quit );
	}
}

void FTrueFPSDemoPlaybackMenu::Quit()
{
	APlayerController* const PCOwner = PlayerOwner ? PlayerOwner->PlayerController : nullptr;

	if ( PCOwner )
	{
		PCOwner->ConsoleCommand( "quit" );
	}
}

int32 FTrueFPSDemoPlaybackMenu::GetOwnerUserIndex() const
{
	return PlayerOwner ? PlayerOwner->GetControllerId() : 0;
}

#undef LOCTEXT_NAMESPACE
