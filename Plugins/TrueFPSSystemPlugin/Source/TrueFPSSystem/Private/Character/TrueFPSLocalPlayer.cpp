// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/TrueFPSLocalPlayer.h"

#include "Character/TrueFPSPersistentUser.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemUtils.h"
#include "TrueFPSGameInstance.h"
#include "TrueFPSSystem.h"

UTrueFPSLocalPlayer::UTrueFPSLocalPlayer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UTrueFPSLocalPlayer::SetControllerId(int32 NewControllerId)
{
	ULocalPlayer::SetControllerId(NewControllerId);

	FString SaveGameName = GetNickname();

#if PLATFORM_SWITCH
	// on Switch, the displayable nickname can change, so we can't use it as a save ID (explicitly stated in docs, so changing for pre-cert)
	FPlatformMisc::GetUniqueStringNameForControllerId(GetControllerId(), SaveGameName);
#endif

	// if we changed controllerid / user, then we need to load the appropriate persistent user.
	if (PersistentUser != nullptr && ( GetControllerId() != PersistentUser->GetUserIndex() || SaveGameName != PersistentUser->GetName() ) )
	{
		PersistentUser->SaveIfDirty();
		PersistentUser = nullptr;
	}

	if (!PersistentUser)
	{
		LoadPersistentUser();
	}
}

FString UTrueFPSLocalPlayer::GetNickname() const
{
	FString UserNickName = Super::GetNickname();

	if ( UserNickName.Len() > MAX_PLAYER_NAME_LENGTH )
	{
		UserNickName = UserNickName.Left( MAX_PLAYER_NAME_LENGTH ) + "...";
	}

	bool bReplace = (UserNickName.Len() == 0);

	// Check for duplicate nicknames...and prevent reentry
	static bool bReentry = false;
	if(!bReentry)
	{
		bReentry = true;
		UTrueFPSGameInstance* GameInstance = GetWorld() != nullptr ? Cast<UTrueFPSGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
		if(GameInstance)
		{
			// Check all the names that occur before ours that are the same
			const TArray<ULocalPlayer*>& LocalPlayers = GameInstance->GetLocalPlayers();
			for (int i = 0; i < LocalPlayers.Num(); ++i)
			{
				const ULocalPlayer* LocalPlayer = LocalPlayers[i];
				if( this == LocalPlayer)
				{
					break;
				}
		
				if( UserNickName == LocalPlayer->GetNickname())
				{
					bReplace = true;
					break;
				}
			}
		}
		bReentry = false;
	}

	if ( bReplace )
	{
		UserNickName = FString::Printf( TEXT( "Player%i" ), GetControllerId() + 1 );
	}	

	return UserNickName;
}

UTrueFPSPersistentUser* UTrueFPSLocalPlayer::GetPersistentUser() const
{
	// if persistent data isn't loaded yet, load it
	if (PersistentUser == nullptr)
	{
		UTrueFPSLocalPlayer* const MutableThis = const_cast<UTrueFPSLocalPlayer*>(this);
		// casting away constness to enable caching implementation behavior
		MutableThis->LoadPersistentUser();
	}
	return PersistentUser;
}

void UTrueFPSLocalPlayer::LoadPersistentUser()
{
	FString SaveGameName = GetNickname();

#if PLATFORM_SWITCH
	// on Switch, the displayable nickname can change, so we can't use it as a save ID (explicitly stated in docs, so changing for pre-cert)
	FPlatformMisc::GetUniqueStringNameForControllerId(GetControllerId(), SaveGameName);
#endif

	// if we changed controllerid / user, then we need to load the appropriate persistent user.
	if (PersistentUser != nullptr && ( GetControllerId() != PersistentUser->GetUserIndex() || SaveGameName != PersistentUser->GetName() ) )
	{
		PersistentUser->SaveIfDirty();
		PersistentUser = nullptr;
	}

	if (PersistentUser == nullptr)
	{
		// Use the platform id here to be resilient in the face of controller swapping and similar situations.
		//FPlatformUserId PlatformId = GetControllerId();

		FPlatformUserId PlatformId = FPlatformUserId::CreateFromInternalId(GetControllerId());

		IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetWorld());
		if (Identity.IsValid() && GetPreferredUniqueNetId().IsValid())
		{
			PlatformId = Identity->GetPlatformUserIdFromUniqueNetId(*GetPreferredUniqueNetId());
		}

		PersistentUser = UTrueFPSPersistentUser::LoadPersistentUser(SaveGameName, PlatformId );
	}
}
