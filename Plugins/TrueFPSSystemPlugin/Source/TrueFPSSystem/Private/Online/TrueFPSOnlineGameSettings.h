// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSessionSettings.h"

/**
 * General session settings for a TrueFPS game
 */
class FTrueFPSOnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FTrueFPSOnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4);
	virtual ~FTrueFPSOnlineSessionSettings() {}
};

/**
 * General search setting for a TrueFPS game
 */
class FTrueFPSOnlineSearchSettings : public FOnlineSessionSearch
{
public:
	FTrueFPSOnlineSearchSettings(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FTrueFPSOnlineSearchSettings() {}
};

/**
 * Search settings for an empty dedicated server to host a match
 */
class FTrueFPSOnlineSearchSettingsEmptyDedicated : public FTrueFPSOnlineSearchSettings
{
public:
	FTrueFPSOnlineSearchSettingsEmptyDedicated(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FTrueFPSOnlineSearchSettingsEmptyDedicated() {}
};
