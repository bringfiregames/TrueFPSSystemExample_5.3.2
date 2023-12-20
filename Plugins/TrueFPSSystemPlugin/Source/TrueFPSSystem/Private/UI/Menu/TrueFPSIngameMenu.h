// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Widgets/TrueFPSMenuItem.h"
#include "Widgets/STrueFPSMenuWidget.h"

class FTrueFPSIngameMenu : public TSharedFromThis<FTrueFPSIngameMenu>
{
public:
	/** sets owning player controller */
	void Construct(ULocalPlayer* PlayerOwner);

	/** toggles in game menu */
	void ToggleGameMenu();

	/** is game menu currently active? */
	bool GetIsGameMenuUp() const;

	/* updates the friends list of the current owner*/
	void UpdateFriendsList();

	/* Getter for the TrueFPSFriends interface/pointer*/
	TSharedPtr<class FTrueFPSFriends> GetTrueFPSFriends(){ return TrueFPSFriends; }

protected:

	/** Owning player controller */
	ULocalPlayer* PlayerOwner;

	/** game menu container widget - used for removing */
	TSharedPtr<class SWeakWidget> GameMenuContainer;

	/** root menu item pointer */
	TSharedPtr<FTrueFPSMenuItem> RootMenuItem;

	/** main menu item pointer */
	TSharedPtr<FTrueFPSMenuItem> MainMenuItem;

	/** HUD menu widget */
	TSharedPtr<class STrueFPSMenuWidget> GameMenuWidget;	

	/** if game menu is currently opened*/
	bool bIsGameMenuUp;

	/** holds cheats menu item to toggle it's visibility */
	TSharedPtr<class FTrueFPSMenuItem> CheatsMenu;

	/** TrueFPS options */
	TSharedPtr<class FTrueFPSOptions> TrueFPSOptions;

	/** get current user index out of PlayerOwner */
	int32 GetOwnerUserIndex() const;
	/** TrueFPS friends */
	TSharedPtr<class FTrueFPSFriends> TrueFPSFriends;

	/** TrueFPS recently met users*/
	TSharedPtr<class FTrueFPSRecentlyMet> TrueFPSRecentlyMet;

	/** called when going back to previous menu */
	void OnMenuGoBack(MenuPtr Menu);
	
	/** goes back in menu structure */
	void CloseSubMenu();

	/** removes widget from viewport */
	void DetachGameMenu();
	
	/** Delegate called when user cancels confirmation dialog to exit to main menu */
	void OnCancelExitToMain();

	/** Delegate called when user confirms confirmation dialog to exit to main menu */
	void OnConfirmExitToMain();		

	/** Plays sound and calls Quit */
	void OnUIQuit();

	/** Quits the game */
	void Quit();

	/** Shows the system UI to invite friends to the game */
	void OnShowInviteUI();
};
