// Fill out your copyright notice in the Description page of Project Settings.

#include "Online/TrueFPSGameState.h"

#include "TrueFPSGameInstance.h"
#include "Character/TrueFPSPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Online/TrueFPSGameMode.h"
#include "Online/TrueFPSPlayerState.h"

// Sets default values
ATrueFPSGameState::ATrueFPSGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void ATrueFPSGameState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	// DOREPLIFETIME( ThisClass, NumTeams );
	DOREPLIFETIME( ThisClass, RemainingTime );
	// DOREPLIFETIME( ThisClass, bTimerPaused );
	DOREPLIFETIME( ThisClass, TeamScores );
}

void ATrueFPSGameState::GetRankedMap(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const
{
	OutRankedMap.Empty();

	//first, we need to go over all the PlayerStates, grab their score, and rank them
	TMultiMap<int32, ATrueFPSPlayerState*> SortedMap;
	for(int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		int32 Score = 0;
		ATrueFPSPlayerState* CurPlayerState = Cast<ATrueFPSPlayerState>(PlayerArray[i]);
		if (CurPlayerState && (CurPlayerState->GetTeamNum() == TeamIndex))
		{
			SortedMap.Add(FMath::TruncToInt(CurPlayerState->GetScore()), CurPlayerState);
		}
	}

	//sort by the keys
	SortedMap.KeySort(TGreater<int32>());

	//now, add them back to the ranked map
	OutRankedMap.Empty();

	int32 Rank = 0;
	for(TMultiMap<int32, ATrueFPSPlayerState*>::TIterator It(SortedMap); It; ++It)
	{
		OutRankedMap.Add(Rank++, It.Value());
	}
}

void ATrueFPSGameState::RequestFinishAndExitToMainMenu()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		ATrueFPSGameMode* const GameMode = Cast<ATrueFPSGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestFinishAndExitToMainMenu();
		}
	}
	else
	{
		// we are client, handle our own business
		UTrueFPSGameInstance* GameInstance = Cast<UTrueFPSGameInstance>(GetGameInstance());
		if (GameInstance)
		{
			GameInstance->RemoveSplitScreenPlayers();
		}

		ATrueFPSPlayerController* const PrimaryPC = Cast<ATrueFPSPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		if (PrimaryPC)
		{
			PrimaryPC->HandleReturnToMainMenu();
		}
	}
}

