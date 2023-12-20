// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTrueFPSSystem, Log, All);

/** when you modify this, please note that this information can be saved with instances
 * also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_WEAPON		ECC_GameTraceChannel1
#define COLLISION_PROJECTILE	ECC_GameTraceChannel2
#define COLLISION_PICKUP		ECC_GameTraceChannel3

#define MAX_PLAYER_NAME_LENGTH 16

#ifndef TRUEFPS_CONSOLE_UI
/** Set to 1 to pretend we're building for console even on a PC, for testing purposes */
#define TRUEFPS_SIMULATE_CONSOLE_UI	0

#if PLATFORM_SWITCH || TRUEFPS_SIMULATE_CONSOLE_UI
#define TRUEFPS_CONSOLE_UI 1
#else
#define TRUEFPS_CONSOLE_UI 0
#endif
#endif

#ifndef TRUEFPS_XBOX_STRINGS
#define TRUEFPS_XBOX_STRINGS 0
#endif

#ifndef TRUEFPS_SHOW_QUIT_MENU_ITEM
#define TRUEFPS_SHOW_QUIT_MENU_ITEM (!TRUEFPS_CONSOLE_UI)
#endif

#ifndef TRUEFPS_SUPPORTS_OFFLINE_SPLIT_SCREEEN
#define TRUEFPS_SUPPORTS_OFFLINE_SPLIT_SCREEEN 1
#endif

// whether the platform will signal a controller pairing change on a controller disconnect. if not, we need to treat the pairing change as a request to switch profiles when the destination profile is not specified
#ifndef TRUEFPS_CONTROLLER_PAIRING_ON_DISCONNECT
#define TRUEFPS_CONTROLLER_PAIRING_ON_DISCONNECT 1
#endif

// whether the game should display an account picker when a new input device is connected, while the "please reconnect controller" message is on screen.
#ifndef TRUEFPS_CONTROLLER_PAIRING_PROMPT_FOR_NEW_USER_WHEN_RECONNECTING
#define TRUEFPS_CONTROLLER_PAIRING_PROMPT_FOR_NEW_USER_WHEN_RECONNECTING 0
#endif

class FTrueFPSSystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};