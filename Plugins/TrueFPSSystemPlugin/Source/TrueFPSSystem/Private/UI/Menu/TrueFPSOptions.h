// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Widgets/TrueFPSMenuItem.h"

class UTrueFPSPersistentUser;
/** supported resolutions */
const FIntPoint DefaultTrueFPSResolutions[] = { FIntPoint(1024,768), FIntPoint(1280,720), FIntPoint(1920,1080) };

/** supported resolutions count*/
const int32 DefaultTrueFPSResCount = UE_ARRAY_COUNT(DefaultTrueFPSResolutions);

/** delegate called when changes are applied */
DECLARE_DELEGATE(FOnApplyChanges);

class UTrueFPSGameUserSettings;

class FTrueFPSOptions : public TSharedFromThis<FTrueFPSOptions>
{
public:
	/** sets owning player controller */
	void Construct(ULocalPlayer* InPlayerOwner);

	/** get current options values for display */
	void UpdateOptions();

	/** UI callback for applying settings, plays sound */
	void OnApplySettings();

	/** applies changes in game settings */
	void ApplySettings();

	/** needed because we can recreate the subsystem that stores it */
	void TellInputAboutKeybindings();

	/** reverts non-saved changes in game settings */
	void RevertChanges();

	/** holds options menu item */
	TSharedPtr<FTrueFPSMenuItem> OptionsItem;

	/** holds cheats menu item */
	TSharedPtr<FTrueFPSMenuItem> CheatsItem;

	/** called when changes were applied - can be used to close submenu */
	FOnApplyChanges OnApplyChanges;

protected:
	/** User settings pointer */
	UTrueFPSGameUserSettings* UserSettings;

	/** video resolution option changed handler */
	void VideoResolutionOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** graphics quality option changed handler */
	void GraphicsQualityOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** infinite ammo option changed handler */
	void InfiniteAmmoOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** infinite clip option changed handler */
	void InfiniteClipOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** freeze timer option changed handler */
	void FreezeTimerOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** health regen option changed handler */
	void HealthRegenOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** aim sensitivity option changed handler */
	void AimSensitivityOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** controller vibration toggle handler */
	void ToggleVibration(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** invert y axis option changed handler */
	void InvertYAxisOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** change NVIDIA reflex settings */
	void NVIDIAReflexChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** change NVIDIA reflex settings */
	void LatencyFlashIndicatorChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** change framerate visibility */
	void FramerateOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** change game to render latency visibility */
	void GameToRenderOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** change game latency visiblity */
	void GameLatencyOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** change render latency visiblity */
	void RenderLatencyOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** full screen option changed handler */
	void FullScreenOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** gamma correction option changed handler */
	void GammaOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex);

	/** try to match current resolution with selected index */
	int32 GetCurrentResolutionIndex(FIntPoint CurrentRes);

	/** Get current mouse y-axis inverted option index*/
	int32 GetCurrentMouseYAxisInvertedIndex();

	/** get current mouse sensitivity option index */
	int32 GetCurrentMouseSensitivityIndex();

	/** get current gamma index */
	int32 GetCurrentGammaIndex();

	/** get current user index out of PlayerOwner */
	int32 GetOwnerUserIndex() const;

	/** Get the persistence user associated with PlayerOwner*/
	UTrueFPSPersistentUser* GetPersistentUser() const;

	/** Owning player controller */
	ULocalPlayer* PlayerOwner;

	/** holds vibration option menu item */
	TSharedPtr<FTrueFPSMenuItem> VibrationOption;

	/** holds NVIDIA reflex option menu item */
	TSharedPtr<FTrueFPSMenuItem> NVIDIAReflexOption;
	
	/** holds NVIDIA reflex option menu item */
	TSharedPtr<FTrueFPSMenuItem> LatencyFlashIndicatorOption;

	/** holds invert y axis option menu item */
	TSharedPtr<FTrueFPSMenuItem> InvertYAxisOption;

	/** holds aim sensitivity option menu item */
	TSharedPtr<FTrueFPSMenuItem> AimSensitivityOption;

	/** holds mouse sensitivity option menu item */
	TSharedPtr<FTrueFPSMenuItem> VideoResolutionOption;

	/** holds graphics quality option menu item */
	TSharedPtr<FTrueFPSMenuItem> GraphicsQualityOption;

	/** holds gamma correction option menu item */
	TSharedPtr<FTrueFPSMenuItem> GammaOption;

	/** holds framerate option menu item */
	TSharedPtr<FTrueFPSMenuItem> FramerateOption;

	/** holds game to render latency option menu item */
	TSharedPtr<FTrueFPSMenuItem> GameToRenderOption;

	/** holds game latency option menu item */
	TSharedPtr<FTrueFPSMenuItem> GameLatencyOption;

	/** holds render latency option menu item */
	TSharedPtr<FTrueFPSMenuItem> RenderLatencyOption;

	/** holds full screen option menu item */
	TSharedPtr<FTrueFPSMenuItem> FullScreenOption;

	/** graphics quality option */
	int32 GraphicsQualityOpt;

	/** NVIDIA Reflex option */
	int32 NVIDIAReflexOpt;
	
	/** Latency Flash Indicator option */
	int32 LatencyFlashIndicatorOpt;

	/** minimum sensitivity index */
	int32 MinSensitivity;

	/** current sensitivity set in options */
	float SensitivityOpt;

	/** current gamma correction set in options */
	float GammaOpt;

	/** Framerate visiblity option */
	int32 FramerateOpt;

	/** Game to render latency visiblity option */
	int32 GameToRenderOpt;

	/** Game latency visiblity option */
	int32 GameLatencyOpt;

	/** Render latency visiblity option */
	int32 RenderLatencyOpt;

	/** full screen setting set in options */
	EWindowMode::Type bFullScreenOpt;

	/** controller vibration setting set in options */
	uint8 bVibrationOpt : 1;

	/** invert mouse setting set in options */
	uint8 bInvertYAxisOpt : 1;

	/** resolution setting set in options */
	FIntPoint ResolutionOpt;

	/** available resolutions list */
	TArray<FIntPoint> Resolutions;

	/** style used for the TrueFPS options */
	const struct FTrueFPSOptionsStyle *OptionsStyle;
};