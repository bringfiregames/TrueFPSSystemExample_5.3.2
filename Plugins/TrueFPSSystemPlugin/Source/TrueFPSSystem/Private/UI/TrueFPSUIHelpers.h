// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Interfaces/OnlineExternalUIInterface.h"

/** Singleton that contains commonly used UI actions */
class TrueFPSUIHelpers
{
public:
	/** gets the singleton. */
	static TrueFPSUIHelpers& Get()
	{
		static TrueFPSUIHelpers Singleton;
		return Singleton;
	}

	/** fetches the string for displaying the profile */
	FText GetProfileOpenText() const;

	/** profile open ui handler */
	bool ProfileOpenedUI(UWorld* World, const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate* Delegate) const;

	/** fetches the string for swapping the profile */
	FText GetProfileSwapText() const;

	/** profile swap ui handler */
	bool ProfileSwapUI(UWorld* World, const int ControllerIndex, bool bShowOnlineOnly, const FOnLoginUIClosedDelegate* Delegate) const;
};