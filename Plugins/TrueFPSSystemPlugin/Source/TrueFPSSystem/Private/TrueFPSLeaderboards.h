// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Interfaces/OnlineLeaderboardInterface.h"

// these are normally exported from platform-specific tools
#define LEADERBOARD_STAT_SCORE			"TrueFPSAllTimeMatchResultsScore"
#define LEADERBOARD_STAT_KILLS			"TrueFPSAllTimeMatchResultsFrags"
#define LEADERBOARD_STAT_DEATHS			"TrueFPSAllTimeMatchResultsDeaths"
#define LEADERBOARD_STAT_MATCHESPLAYED	"TrueFPSAllTimeMatchResultsMatchesPlayed"

/**
 *	'AllTime' leaderboard read object
 */
class FTrueFPSAllTimeMatchResultsRead : public FOnlineLeaderboardRead
{
public:

	FTrueFPSAllTimeMatchResultsRead()
	{
		// Default properties
		LeaderboardName = FName(TEXT("TrueFPSAllTimeMatchResults"));
		SortedColumn = LEADERBOARD_STAT_SCORE;

		// Define default columns
		new (ColumnMetadata) FColumnMetaData(LEADERBOARD_STAT_SCORE, EOnlineKeyValuePairDataType::Int32);
	}
};

/**
 *	'AllTime' leaderboard write object
 */
class FTrueFPSAllTimeMatchResultsWrite : public FOnlineLeaderboardWrite
{
public:

	FTrueFPSAllTimeMatchResultsWrite()
	{
		// Default properties
		new (LeaderboardNames) FName(TEXT("TrueFPSAllTimeMatchResults"));
		RatedStat = LEADERBOARD_STAT_SCORE;
		DisplayFormat = ELeaderboardFormat::Number;
		SortMethod = ELeaderboardSort::Descending;
		UpdateMethod = ELeaderboardUpdateMethod::KeepBest;
	}
};

