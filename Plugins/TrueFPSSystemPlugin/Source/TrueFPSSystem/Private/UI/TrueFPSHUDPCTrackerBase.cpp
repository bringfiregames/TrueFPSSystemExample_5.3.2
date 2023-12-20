// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSHUDPCTrackerBase.h"

#include "Online/TrueFPSGameState.h"
#include "Character/TrueFPSPlayerController.h"

/** Initialize with a world context. */
void TrueFPSHUDPCTrackerBase::Init( const FLocalPlayerContext& InContext )
{
	Context = InContext;
}

TWeakObjectPtr<ATrueFPSPlayerController> TrueFPSHUDPCTrackerBase::GetPlayerController() const
{
	if ( ensureMsgf( Context.IsValid(), TEXT("Game context must be initialized!") ) )
	{
		APlayerController* PC = Context.GetPlayerController();
		ATrueFPSPlayerController* TrueFPSPlayerController = Cast<ATrueFPSPlayerController>(PC);
		return MakeWeakObjectPtr(TrueFPSPlayerController);
	}
	else
	{
		return nullptr;
	}
}


UWorld* TrueFPSHUDPCTrackerBase::GetWorld() const
{
	if ( ensureMsgf( Context.IsValid(), TEXT("Game context must be initialized!") ) )
	{
		return Context.GetWorld();
	}
	else
	{
		return nullptr;
	}
}

ATrueFPSGameState* TrueFPSHUDPCTrackerBase::GetGameState() const
{
	if ( ensureMsgf( Context.IsValid(), TEXT("Game context must be initialized!") ) )
	{
		return Context.GetWorld()->GetGameState<ATrueFPSGameState>();
	}
	else
	{
		return nullptr;
	}
}

const FLocalPlayerContext& TrueFPSHUDPCTrackerBase::GetContext() const
{
	return Context;
}



