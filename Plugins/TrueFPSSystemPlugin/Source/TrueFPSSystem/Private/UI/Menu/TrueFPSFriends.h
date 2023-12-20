// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "OnlineSubsystem.h"
#include "OnlineSubsystemTypes.h"
#include "SlateBasics.h"
#include "SlateExtras.h"

class UTrueFPSPersistentUser;
class UTrueFPSGameUserSettings;
class FTrueFPSMenuItem;

/** delegate called when changes are applied */
DECLARE_DELEGATE(FOnApplyChanges);

/** delegate called when a menu item is confirmed */
DECLARE_DELEGATE(FOnConfirmMenuItem);

/** delegate called when facebutton_left is pressed */
DECLARE_DELEGATE(FOnControllerFacebuttonLeftPressed);

/** delegate called down on the gamepad is pressed*/
DECLARE_DELEGATE(FOnControllerDownInputPressed);

/** delegate called up on the gamepad is pressed*/
DECLARE_DELEGATE(FOnControllerUpInputPressed);

/** delegate called when facebutton_down is pressed */
DECLARE_DELEGATE(FOnOnControllerFacebuttonDownPressed);

class FTrueFPSFriends : public TSharedFromThis<FTrueFPSFriends>
{
public:
	/** sets owning player controller */
	void Construct(ULocalPlayer* _PlayerOwner, int32 LocalUserNum);

	/** get current Friends values for display */
	void UpdateFriends(int32 NewOwnerIndex);

	/** UI callback for applying settings, plays sound */
	void OnApplySettings();

	/** applies changes in game settings */
	void ApplySettings();

	/** needed because we can recreate the subsystem that stores it */
	void TellInputAboutKeybindings();

	/** increment the counter keeping track of which user we're looking at */
	void IncrementFriendsCounter();

	/** decrement the counter keeping track of which user we're looking at */
	void DecrementFriendsCounter();

	/** view the profile of the selected user */
	void ViewSelectedFriendProfile();

	/** send an invite to the selected user */
	void InviteSelectedFriendToGame();

	/** holds Friends menu item */
	TSharedPtr<FTrueFPSMenuItem> FriendsItem;

	/** called when changes were applied - can be used to close submenu */
	FOnApplyChanges OnApplyChanges;

	/** delegate, which is executed by STrueFPSMenuWidget if user confirms this menu item */
	FOnConfirmMenuItem OnConfirmMenuItem;

	/** delegate, which is executed by STrueFPSMenuWidget if facebutton_left is pressed */
	FOnControllerFacebuttonLeftPressed OnControllerFacebuttonLeftPressed;

	/** delegate, which is executed by STrueFPSMenuWidget if down input is pressed */
	FOnControllerDownInputPressed OnControllerDownInputPressed;

	/** delegate, which is executed by STrueFPSMenuWidget if up input is pressed */
	FOnControllerUpInputPressed OnControllerUpInputPressed;

	/** delegate, which is executed by STrueFPSMenuWidget if facebutton_down is pressed */
	FOnOnControllerFacebuttonDownPressed OnControllerFacebuttonDownPressed;

	int32 LocalUserNum;
	int32 CurrFriendIndex;
	int32 MinFriendIndex;
	int32 MaxFriendIndex;

	TArray< TSharedRef<FOnlineFriend> > Friends;

	IOnlineSubsystem* OnlineSub;
	IOnlineFriendsPtr OnlineFriendsPtr;

protected:
	void OnFriendsUpdated(int32 UpdatedPlayerIndex, bool bWasSuccessful, const FString& FriendListName, const FString& ErrorString);

	/** User settings pointer */
	UTrueFPSGameUserSettings* UserSettings;

	/** Get the persistence user associated with PCOwner*/
	UTrueFPSPersistentUser* GetPersistentUser() const;

	/** Owning player controller */
	ULocalPlayer* PlayerOwner;

	/** style used for the TrueFPS Friends */
	const struct FTrueFPSOptionsStyle *FriendsStyle;
};