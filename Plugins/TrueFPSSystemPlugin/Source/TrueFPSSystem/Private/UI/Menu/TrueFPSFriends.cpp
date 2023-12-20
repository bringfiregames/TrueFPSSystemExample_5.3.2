// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSFriends.h"
#include "OnlineSubsystemUtils.h"
#include "TrueFPSGameUserSettings.h"
#include "Character/TrueFPSLocalPlayer.h"
#include "Character/TrueFPSPersistentUser.h"
#include "UI/Style/TrueFPSStyle.h"
#include "UI/Style/TrueFPSOptionsWidgetStyle.h"
#include "Widgets/STrueFPSMenuWidget.h"

#define LOCTEXT_NAMESPACE "TrueFPSSystem.HUD.Menu"

void FTrueFPSFriends::Construct(ULocalPlayer* _PlayerOwner, int32 LocalUserNum_)
{
	FriendsStyle = &FTrueFPSStyle::Get().GetWidgetStyle<FTrueFPSOptionsStyle>("DefaultTrueFPSOptionsStyle");

	PlayerOwner = _PlayerOwner;
	LocalUserNum = LocalUserNum_;
	CurrFriendIndex = 0;
	MinFriendIndex = 0;
	MaxFriendIndex = 0; //initialized after the friends list is read in

	/** Friends menu root item */
	TSharedPtr<FTrueFPSMenuItem> FriendsRoot = FTrueFPSMenuItem::CreateRoot();

	//Populate the friends list
	FriendsItem = MenuHelper::AddMenuItem(FriendsRoot, LOCTEXT("Friends", "FRIENDS"));

	if (PlayerOwner)
	{
		OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
		OnlineFriendsPtr = OnlineSub->GetFriendsInterface();
	}

	UpdateFriends(LocalUserNum);

	UserSettings = CastChecked<UTrueFPSGameUserSettings>(GEngine->GetGameUserSettings());
}

void FTrueFPSFriends::OnApplySettings()
{
	ApplySettings();
}

void FTrueFPSFriends::ApplySettings()
{
	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();

		PersistentUser->SaveIfDirty();
	}

	UserSettings->ApplySettings(false);

	OnApplyChanges.ExecuteIfBound();
}

void FTrueFPSFriends::TellInputAboutKeybindings()
{
	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

UTrueFPSPersistentUser* FTrueFPSFriends::GetPersistentUser() const
{
	UTrueFPSLocalPlayer* const TrueFPSLocalPlayer = Cast<UTrueFPSLocalPlayer>(PlayerOwner);
	return TrueFPSLocalPlayer ? TrueFPSLocalPlayer->GetPersistentUser() : nullptr;
	//// Main Menu
	//AShooterPlayerController_Menu* ShooterPCM = Cast<AShooterPlayerController_Menu>(PCOwner);
	//if(ShooterPCM)
	//{
	//	return ShooterPCM->GetPersistentUser();
	//}

	//// In-game Menu
	//AShooterPlayerController* ShooterPC = Cast<AShooterPlayerController>(PCOwner);
	//if(ShooterPC)
	//{
	//	return ShooterPC->GetPersistentUser();
	//}

	//return nullptr;
}

void FTrueFPSFriends::UpdateFriends(int32 NewOwnerIndex)
{
	if (!OnlineFriendsPtr.IsValid())
	{
		return;
	}

	LocalUserNum = NewOwnerIndex;
	OnlineFriendsPtr->ReadFriendsList(LocalUserNum, EFriendsLists::ToString(EFriendsLists::OnlinePlayers), FOnReadFriendsListComplete::CreateSP(this, &FTrueFPSFriends::OnFriendsUpdated));
}

void FTrueFPSFriends::OnFriendsUpdated(int32 /*unused*/, bool bWasSuccessful, const FString& FriendListName, const FString& ErrorString)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogOnline, Warning, TEXT("Unable to update friendslist %s due to error=[%s]"), *FriendListName, *ErrorString);
		return;
	}

	MenuHelper::ClearSubMenu(FriendsItem);

	Friends.Reset();
	if (OnlineFriendsPtr->GetFriendsList(LocalUserNum, EFriendsLists::ToString(EFriendsLists::OnlinePlayers), Friends))
	{
		for (const TSharedRef<FOnlineFriend>& Friend : Friends)
		{
			TSharedRef<FTrueFPSMenuItem> FriendItem = MenuHelper::AddMenuItem(FriendsItem, FText::FromString(Friend->GetDisplayName()));
			FriendItem->OnControllerFacebuttonDownPressed.BindSP(this, &FTrueFPSFriends::ViewSelectedFriendProfile);
			FriendItem->OnControllerDownInputPressed.BindSP(this, &FTrueFPSFriends::IncrementFriendsCounter);
			FriendItem->OnControllerUpInputPressed.BindSP(this, &FTrueFPSFriends::DecrementFriendsCounter);
		}

		MaxFriendIndex = Friends.Num() - 1;
	}

	MenuHelper::AddMenuItemSP(FriendsItem, LOCTEXT("Close", "CLOSE"), this, &FTrueFPSFriends::OnApplySettings);
}

void FTrueFPSFriends::IncrementFriendsCounter()
{
	if (CurrFriendIndex + 1 <= MaxFriendIndex)
	{
		++CurrFriendIndex;
	}
}
void FTrueFPSFriends::DecrementFriendsCounter()
{
	if (CurrFriendIndex - 1 >= MinFriendIndex)
	{
		--CurrFriendIndex;
	}
}
void FTrueFPSFriends::ViewSelectedFriendProfile()
{
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid() && Friends.IsValidIndex(CurrFriendIndex))
		{
			TSharedPtr<const FUniqueNetId> Requestor = Identity->GetUniquePlayerId(LocalUserNum);
			TSharedPtr<const FUniqueNetId> Requestee = Friends[CurrFriendIndex]->GetUserId();
			
			IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
			if (ExternalUI.IsValid() && Requestor.IsValid() && Requestee.IsValid())
			{
				ExternalUI->ShowProfileUI(*Requestor, *Requestee, FOnProfileUIClosedDelegate());
			}
		}
}
}
void FTrueFPSFriends::InviteSelectedFriendToGame()
{
	// invite the user to the current gamesession
	if (OnlineSub)
	{
		IOnlineSessionPtr OnlineSessionInterface = OnlineSub->GetSessionInterface();
		if (OnlineSessionInterface.IsValid())
		{
			OnlineSessionInterface->SendSessionInviteToFriend(LocalUserNum, NAME_GameSession, *Friends[CurrFriendIndex]->GetUserId());
		}
	}
}


#undef LOCTEXT_NAMESPACE

