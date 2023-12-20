// Fill out your copyright notice in the Description page of Project Settings.

#include "Online/TrueFPSPlayerState.h"

#include "TrueFPSSystem.h"
#include "Character/TrueFPSCharacter.h"
#include "Character/TrueFPSPlayerController.h"
#include "Net/OnlineEngineInterface.h"
#include "Net/UnrealNetwork.h"
#include "Online/TrueFPSGameState.h"

ATrueFPSPlayerState::ATrueFPSPlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TeamNumber = 0;
	NumKills = 0;
	NumDeaths = 0;
	NumBulletsFired = 0;
	bQuitter = false;
}

void ATrueFPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TeamNumber);
	DOREPLIFETIME(ThisClass, NumKills);
	DOREPLIFETIME(ThisClass, NumDeaths);
	DOREPLIFETIME(ThisClass, MatchId);
}


void ATrueFPSPlayerState::Reset()
{
	Super::Reset();
	
	//PlayerStates persist across seamless travel.  Keep the same teams as previous match.
	//SetTeamNum(0);
	NumKills = 0;
	NumDeaths = 0;
	NumBulletsFired = 0;
	bQuitter = false;
}

void ATrueFPSPlayerState::ClientInitialize(AController* InController)
{
	Super::ClientInitialize(InController);

	UpdateTeamColors();
}

void ATrueFPSPlayerState::RegisterPlayerWithSession(bool bWasFromInvite)
{
	if (UOnlineEngineInterface::Get()->DoesSessionExist(GetWorld(), NAME_GameSession))
	{
		Super::RegisterPlayerWithSession(bWasFromInvite);
	}
}

void ATrueFPSPlayerState::UnregisterPlayerWithSession()
{
	if (!IsFromPreviousLevel() && UOnlineEngineInterface::Get()->DoesSessionExist(GetWorld(), NAME_GameSession))
	{
		Super::UnregisterPlayerWithSession();
	}
}

void ATrueFPSPlayerState::SetTeamNum(int32 NewTeamNumber)
{
	TeamNumber = NewTeamNumber;

	UpdateTeamColors();
}

void ATrueFPSPlayerState::ScoreKill(ATrueFPSPlayerState* Victim, int32 Points)
{
	NumKills++;
	ScorePoints(Points);
}

void ATrueFPSPlayerState::ScoreDeath(ATrueFPSPlayerState* KilledBy, int32 Points)
{
	NumDeaths++;
	ScorePoints(Points);
}

int32 ATrueFPSPlayerState::GetTeamNum() const
{
	return TeamNumber;
}

int32 ATrueFPSPlayerState::GetKills() const
{
	return NumKills;
}

int32 ATrueFPSPlayerState::GetDeaths() const
{
	return NumDeaths;
}

int32 ATrueFPSPlayerState::GetNumBulletsFired() const
{
	return NumBulletsFired;
}

bool ATrueFPSPlayerState::IsQuitter() const
{
	return bQuitter;
}

FString ATrueFPSPlayerState::GetMatchId() const
{
	return MatchId;
}

FString ATrueFPSPlayerState::GetShortPlayerName() const
{
	if ( GetPlayerName().Len() > MAX_PLAYER_NAME_LENGTH )
	{
		return GetPlayerName().Left(MAX_PLAYER_NAME_LENGTH) + "...";
	}
	
	return GetPlayerName();
}

void ATrueFPSPlayerState::InformAboutKill_Implementation(ATrueFPSPlayerState* KillerPlayerState,
	const UDamageType* KillerDamageType, ATrueFPSPlayerState* KilledPlayerState)
{
	// id can be nullptr for bots
	if (KillerPlayerState->GetUniqueId().IsValid())
	{	
		// search for the actual killer before calling OnKill()	
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{		
			ATrueFPSPlayerController* TestPC = Cast<ATrueFPSPlayerController>(*It);
			if (TestPC && TestPC->IsLocalController())
			{
				// a local player might not have an ID if it was created with CreateDebugPlayer.
				ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(TestPC->Player);
				FUniqueNetIdRepl LocalID = LocalPlayer->GetCachedUniqueNetId();
				if (LocalID.IsValid() &&  *LocalPlayer->GetCachedUniqueNetId() == *KillerPlayerState->GetUniqueId())
				{			
					TestPC->OnKill();
				}
			}
		}
	}
}

void ATrueFPSPlayerState::BroadcastDeath_Implementation(ATrueFPSPlayerState* KillerPlayerState,
	const UDamageType* KillerDamageType, ATrueFPSPlayerState* KilledPlayerState)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		// all local players get death messages so they can update their huds.
		ATrueFPSPlayerController* TestPC = Cast<ATrueFPSPlayerController>(*It);
		if (TestPC && TestPC->IsLocalController())
		{
			TestPC->OnDeathMessage(KillerPlayerState, this, KillerDamageType);				
		}
	}	
}

void ATrueFPSPlayerState::OnRep_TeamColor()
{
	UpdateTeamColors();
}

void ATrueFPSPlayerState::AddBulletsFired(int32 NumBullets)
{
	NumBulletsFired += NumBullets;
}

void ATrueFPSPlayerState::SetQuitter(bool bInQuitter)
{
	bQuitter = bInQuitter;
}

void ATrueFPSPlayerState::SetMatchId(const FString& CurrentMatchId)
{
	MatchId = CurrentMatchId;
}

void ATrueFPSPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ATrueFPSPlayerState* TrueFPSPlayerState = Cast<ATrueFPSPlayerState>(PlayerState);
	if (TrueFPSPlayerState)
	{
		TrueFPSPlayerState->TeamNumber = TeamNumber;
	}
}

void ATrueFPSPlayerState::UpdateTeamColors()
{
	AController* OwnerController = Cast<AController>(GetOwner());
	if (OwnerController != nullptr)
	{
		ATrueFPSCharacter* TrueFPSCharacter = Cast<ATrueFPSCharacter>(OwnerController->GetCharacter());
		if (TrueFPSCharacter != nullptr)
		{
			// ShooterCharacter->UpdateTeamColorsAllMIDs(); @todo add team colors
		}
	}
}

void ATrueFPSPlayerState::ScorePoints(int32 Points)
{
	ATrueFPSGameState* const MyGameState = GetWorld()->GetGameState<ATrueFPSGameState>();
	if (MyGameState && TeamNumber >= 0)
	{
		if (TeamNumber >= MyGameState->TeamScores.Num())
		{
			MyGameState->TeamScores.AddZeroed(TeamNumber - MyGameState->TeamScores.Num() + 1);
		}

		MyGameState->TeamScores[TeamNumber] += Points;
	}

	SetScore(GetScore() + Points);
}
