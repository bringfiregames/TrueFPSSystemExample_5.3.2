#pragma once

#ifndef __TRUEFPSSYSTEMLOADINGSCREEN_H__
#define __TRUEFPSSYSTEMLOADINGSCREEN_H__

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class ITrueFPSSystemLoadingScreenModule : public IModuleInterface
{
public:
    /** Kicks off the loading screen for in game loading (not startup) */
    virtual void StartInGameLoadingScreen() = 0;
};

#endif // __TRUEFPSSYSTEMLOADINGSCREEN_H__
