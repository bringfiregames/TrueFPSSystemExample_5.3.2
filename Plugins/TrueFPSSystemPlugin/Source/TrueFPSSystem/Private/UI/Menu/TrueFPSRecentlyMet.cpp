// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSRecentlyMet.h"
#include "OnlineSubsystemUtils.h"
#include "TrueFPSGameUserSettings.h"
#include "Character/TrueFPSLocalPlayer.h"
#include "Character/TrueFPSPersistentUser.h"
#include "Online/TrueFPSGameState.h"
#include "UI/Style/TrueFPSStyle.h"
#include "Widgets/STrueFPSMenuWidget.h"
#include "UI/Style/TrueFPSOptionsWidgetStyle.h"

#define LOCTEXT_NAMESPACE "TrueFPSSystem.HUD.Menu"

void FTrueFPSRecentlyMet::Construct(ULocalPlayer* _PlayerOwner, int32 LocalUserNum_)
{
	RecentlyMetStyle = &FTrueFPSStyle::Get().GetWidgetStyle<FTrueFPSOptionsStyle>("DefaultTrueFPSOptionsStyle");

	PlayerOwner = _PlayerOwner;
	LocalUserNum = LocalUserNum_;
	CurrRecentlyMetIndex = 0;
	MinRecentlyMetIndex = 0;
	MaxRecentlyMetIndex = 0; //initialized after the recently met list is (read in/set)

	/** Recently Met menu items */
	RecentlyMetRoot = FTrueFPSMenuItem::CreateRoot();
	RecentlyMetItem = MenuHelper::AddMenuItem(RecentlyMetRoot, LOCTEXT("Recently Met", "RECENTLY MET"));

	/** Init online items */
	if (PlayerOwner)
	{
		OnlineSub = Online::GetSubsystem(PlayerOwner->GetWorld());
	}

	UserSettings = CastChecked<UTrueFPSGameUserSettings>(GEngine->GetGameUserSettings());	
}

void FTrueFPSRecentlyMet::OnApplySettings()
{
	ApplySettings();
}

void FTrueFPSRecentlyMet::ApplySettings()
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

void FTrueFPSRecentlyMet::TellInputAboutKeybindings()
{
	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

UTrueFPSPersistentUser* FTrueFPSRecentlyMet::GetPersistentUser() const
{
	UTrueFPSLocalPlayer* const SLP = Cast<UTrueFPSLocalPlayer>(PlayerOwner);
	return SLP ? SLP->GetPersistentUser() : nullptr;
}

void FTrueFPSRecentlyMet::UpdateRecentlyMet(int32 NewOwnerIndex)
{
	LocalUserNum = NewOwnerIndex;
	
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid())
		{
			LocalUsername = Identity->GetPlayerNickname(LocalUserNum);
		}
	}
	
	MenuHelper::ClearSubMenu(RecentlyMetItem);
	MaxRecentlyMetIndex = 0;

	ATrueFPSGameState* const MyGameState = PlayerOwner->GetWorld()->GetGameState<ATrueFPSGameState>();
	if (MyGameState != nullptr)
	{
		MetPlayerArray = MyGameState->PlayerArray;

		for (int32 i = 0; i < MetPlayerArray.Num(); ++i)
		{
			const APlayerState* PlayerState = MetPlayerArray[i];
			FString Username = PlayerState->GetHumanReadableName();
			if (Username != LocalUsername && PlayerState->IsABot() == false)
			{
				TSharedPtr<FTrueFPSMenuItem> UserItem = MenuHelper::AddMenuItem(RecentlyMetItem, FText::FromString(Username));
				UserItem->OnControllerDownInputPressed.BindRaw(this, &FTrueFPSRecentlyMet::IncrementRecentlyMetCounter);
				UserItem->OnControllerUpInputPressed.BindRaw(this, &FTrueFPSRecentlyMet::DecrementRecentlyMetCounter);
				UserItem->OnControllerFacebuttonDownPressed.BindRaw(this, &FTrueFPSRecentlyMet::ViewSelectedUsersProfile);
			}
			else
			{
				MetPlayerArray.RemoveAt(i);
				--i; //we just deleted an item, so we need to go make sure i doesn't increment again, otherwise it would skip the player that was supposed to be looked at next
			}
		}

		MaxRecentlyMetIndex = MetPlayerArray.Num() - 1;
	}

	MenuHelper::AddMenuItemSP(RecentlyMetItem, LOCTEXT("Close", "CLOSE"), this, &FTrueFPSRecentlyMet::OnApplySettings);
}

void FTrueFPSRecentlyMet::IncrementRecentlyMetCounter()
{
	if (CurrRecentlyMetIndex + 1 <= MaxRecentlyMetIndex)
	{
		++CurrRecentlyMetIndex;
	}
}
void FTrueFPSRecentlyMet::DecrementRecentlyMetCounter()
{
	if (CurrRecentlyMetIndex - 1 >= MinRecentlyMetIndex)
	{
		--CurrRecentlyMetIndex;
	}
}
void FTrueFPSRecentlyMet::ViewSelectedUsersProfile()
{
	if (OnlineSub)
	{
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
		if (Identity.IsValid() && MetPlayerArray.IsValidIndex(CurrRecentlyMetIndex))
		{
			const APlayerState* PlayerState = MetPlayerArray[CurrRecentlyMetIndex];
		
			TSharedPtr<const FUniqueNetId> Requestor = Identity->GetUniquePlayerId(LocalUserNum);
			TSharedPtr<const FUniqueNetId> Requestee = PlayerState->GetUniqueId().GetUniqueNetId();
			
			IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
			if (ExternalUI.IsValid() && Requestor.IsValid() && Requestee.IsValid())
			{
				ExternalUI->ShowProfileUI(*Requestor, *Requestee, FOnProfileUIClosedDelegate());
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE
