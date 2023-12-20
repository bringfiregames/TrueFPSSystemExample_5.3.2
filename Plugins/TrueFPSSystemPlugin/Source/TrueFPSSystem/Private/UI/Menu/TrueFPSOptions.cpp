// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSOptions.h"

#include "TrueFPSGameUserSettings.h"
#include "Character/TrueFPSLocalPlayer.h"
#include "Character/TrueFPSPersistentUser.h"
#include "Character/TrueFPSPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Online/TrueFPSGameState.h"
#include "Performance/LatencyMarkerModule.h"
#include "UI/Style/TrueFPSOptionsWidgetStyle.h"
#include "UI/Style/TrueFPSStyle.h"
#include "Widgets/STrueFPSMenuWidget.h"

#define LOCTEXT_NAMESPACE "TrueFPSSystem.HUD.Menu"

void FTrueFPSOptions::Construct(ULocalPlayer* InPlayerOwner)
{
	OptionsStyle = &FTrueFPSStyle::Get().GetWidgetStyle<FTrueFPSOptionsStyle>("DefaultTrueFPSOptionsStyle");

	PlayerOwner = InPlayerOwner;
	MinSensitivity = 1;
	
	TArray<FText> ResolutionList;
	TArray<FText> OnOffList;
	TArray<FText> SensitivityList;
	TArray<FText> GammaList;
	TArray<FText> LowHighList;
	TArray<FText> NVReflexList;
	TArray<FText> ShowHideList;

	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetInitialDisplayMetrics(DisplayMetrics);

	bool bAddedNativeResolution = false;
	const FIntPoint NativeResolution(DisplayMetrics.PrimaryDisplayWidth, DisplayMetrics.PrimaryDisplayHeight);

	for (int32 i = 0; i < DefaultTrueFPSResCount; i++)
	{
		if (DefaultTrueFPSResolutions[i].X <= DisplayMetrics.PrimaryDisplayWidth && DefaultTrueFPSResolutions[i].Y <= DisplayMetrics.PrimaryDisplayHeight)
		{
			ResolutionList.Add(FText::Format(FText::FromString("{0}x{1}"), FText::FromString(FString::FromInt(DefaultTrueFPSResolutions[i].X)), FText::FromString(FString::FromInt(DefaultTrueFPSResolutions[i].Y))));
			Resolutions.Add(DefaultTrueFPSResolutions[i]);

			bAddedNativeResolution = bAddedNativeResolution || (DefaultTrueFPSResolutions[i] == NativeResolution);
		}
	}

	// Always make sure that the native resolution is available
	if (!bAddedNativeResolution)
	{
		ResolutionList.Add(FText::Format(FText::FromString("{0}x{1}"), FText::FromString(FString::FromInt(NativeResolution.X)), FText::FromString(FString::FromInt(NativeResolution.Y))));
		Resolutions.Add(NativeResolution);
	}

	OnOffList.Add(LOCTEXT("Off","OFF"));
	OnOffList.Add(LOCTEXT("On","ON"));

	LowHighList.Add(LOCTEXT("Low","LOW"));
	LowHighList.Add(LOCTEXT("High","HIGH"));

	ShowHideList.Add(LOCTEXT("Hide", "HIDE"));
	ShowHideList.Add(LOCTEXT("Show", "SHOW"));

	NVReflexList.Add(LOCTEXT("Off", "OFF"));
	NVReflexList.Add(LOCTEXT("On", "ON"));
	NVReflexList.Add(LOCTEXT("On+Boost", "ON+BOOST"));

	//Mouse sensitivity 0-50
	for (int32 i = 0; i < 51; i++)
	{
		SensitivityList.Add(FText::AsNumber(i));
	}

	for (int32 i = -50; i < 51; i++)
	{
		GammaList.Add(FText::AsNumber(i));
	}

	/** Options menu root item */
	TSharedPtr<FTrueFPSMenuItem> OptionsRoot = FTrueFPSMenuItem::CreateRoot();

	/** Cheats menu root item */
	TSharedPtr<FTrueFPSMenuItem> CheatsRoot = FTrueFPSMenuItem::CreateRoot();

	CheatsItem = MenuHelper::AddMenuItem(CheatsRoot,LOCTEXT("Cheats", "CHEATS"));
	MenuHelper::AddMenuOptionSP(CheatsItem, LOCTEXT("InfiniteAmmo", "INFINITE AMMO"), OnOffList, this, &FTrueFPSOptions::InfiniteAmmoOptionChanged);
	MenuHelper::AddMenuOptionSP(CheatsItem, LOCTEXT("InfiniteClip", "INFINITE CLIP"), OnOffList, this, &FTrueFPSOptions::InfiniteClipOptionChanged);
	MenuHelper::AddMenuOptionSP(CheatsItem, LOCTEXT("FreezeMatchTimer", "FREEZE MATCH TIMER"), OnOffList, this, &FTrueFPSOptions::FreezeTimerOptionChanged);
	MenuHelper::AddMenuOptionSP(CheatsItem, LOCTEXT("HealthRegen", "HP REGENERATION"), OnOffList, this, &FTrueFPSOptions::HealthRegenOptionChanged);

	OptionsItem = MenuHelper::AddMenuItem(OptionsRoot,LOCTEXT("Options", "OPTIONS"));
#if PLATFORM_DESKTOP
	VideoResolutionOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("Resolution", "RESOLUTION"), ResolutionList, this, &FTrueFPSOptions::VideoResolutionOptionChanged);
	GraphicsQualityOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("Quality", "QUALITY"),LowHighList, this, &FTrueFPSOptions::GraphicsQualityOptionChanged);
	FullScreenOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("FullScreen", "FULL SCREEN"),OnOffList, this, &FTrueFPSOptions::FullScreenOptionChanged);

	TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get().GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
	bool bLatencyModuleEnabled = false;
	for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
	{
		LatencyMarkerModule->SetEnabled(true);
		if (LatencyMarkerModule->GetEnabled())
		{
			bLatencyModuleEnabled = true;
			break;
		}
	}

	if (bLatencyModuleEnabled)
	{
		NVIDIAReflexOption = MenuHelper::AddMenuOptionSP(OptionsItem, LOCTEXT("NVIDIA Reflex Low Latency", "NVIDIA REFLEX LOW LATENCY"), NVReflexList, this, &FTrueFPSOptions::NVIDIAReflexChanged);
		LatencyFlashIndicatorOption = MenuHelper::AddMenuOptionSP(OptionsItem, LOCTEXT("Latency Flash Indicator", "LATENCY FLASH INDICATOR"), OnOffList, this, &FTrueFPSOptions::LatencyFlashIndicatorChanged);
		FramerateOption = MenuHelper::AddMenuOptionSP(OptionsItem, LOCTEXT("Show frame rate", "CLIENT FPS"), ShowHideList, this, &FTrueFPSOptions::FramerateOptionChanged);
		GameToRenderOption = MenuHelper::AddMenuOptionSP(OptionsItem, LOCTEXT("Show game to render latency", "GAME TO RENDER LATENCY"), ShowHideList, this, &FTrueFPSOptions::GameToRenderOptionChanged);
		GameLatencyOption = MenuHelper::AddMenuOptionSP(OptionsItem, LOCTEXT("Show game latency", "GAME LATENCY"), ShowHideList, this, &FTrueFPSOptions::GameLatencyOptionChanged);
		RenderLatencyOption = MenuHelper::AddMenuOptionSP(OptionsItem, LOCTEXT("Show render latency", "RENDER LATENCY"), ShowHideList, this, &FTrueFPSOptions::RenderLatencyOptionChanged);
	}
#endif
	GammaOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("Gamma", "GAMMA CORRECTION"),GammaList, this, &FTrueFPSOptions::GammaOptionChanged);
	AimSensitivityOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("AimSensitivity", "AIM SENSITIVITY"),SensitivityList, this, &FTrueFPSOptions::AimSensitivityOptionChanged);
	InvertYAxisOption = MenuHelper::AddMenuOptionSP(OptionsItem,LOCTEXT("InvertYAxis", "INVERT Y AXIS"),OnOffList, this, &FTrueFPSOptions::InvertYAxisOptionChanged);
	VibrationOption = MenuHelper::AddMenuOptionSP(OptionsItem, LOCTEXT("Vibration", "VIBRATION"), OnOffList, this, &FTrueFPSOptions::ToggleVibration);
	

	MenuHelper::AddMenuItemSP(OptionsItem,LOCTEXT("ApplyChanges", "APPLY CHANGES"), this, &FTrueFPSOptions::OnApplySettings);

	//Do not allow to set aim sensitivity to 0
	AimSensitivityOption->MinMultiChoiceIndex = MinSensitivity;
    
    //Default vibration to On.
	VibrationOption->SelectedMultiChoice = 1;

	UserSettings = CastChecked<UTrueFPSGameUserSettings>(GEngine->GetGameUserSettings());
	bFullScreenOpt = UserSettings->GetFullscreenMode();
	GraphicsQualityOpt = UserSettings->GetGraphicsQuality();
	NVIDIAReflexOpt = UserSettings->GetNVIDIAReflex();
	LatencyFlashIndicatorOpt = UserSettings->GetLatencyFlashIndicator();
	FramerateOpt = UserSettings->GetFramerateVisibility();
	GameToRenderOpt = UserSettings->GetGameToRenderVisibility();
	GameLatencyOpt = UserSettings->GetGameLatencyVisibility();
	RenderLatencyOpt = UserSettings->GetRenderLatencyVisibility();

	if (UserSettings->IsForceSystemResolution())
	{
		// store the current system resolution
	 	ResolutionOpt = FIntPoint(GSystemResolution.ResX, GSystemResolution.ResY);
	}
	else
	{
		ResolutionOpt = UserSettings->GetScreenResolution();
	}

	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		bInvertYAxisOpt = PersistentUser->GetInvertedYAxis();
		SensitivityOpt = PersistentUser->GetAimSensitivity();
		GammaOpt = PersistentUser->GetGamma();
		bVibrationOpt = PersistentUser->GetVibration();
	}
	else
	{
		bVibrationOpt = true;
		bInvertYAxisOpt = false;
		SensitivityOpt = 1.0f;
		GammaOpt = 2.2f;
	}

	if (ensure(PlayerOwner != nullptr))
	{
		APlayerController* BaseController = Cast<APlayerController>(UGameplayStatics::GetPlayerControllerFromID(PlayerOwner->GetWorld(), PlayerOwner->GetControllerId()));
		ensure(BaseController);
		if (BaseController)
		{
			ATrueFPSPlayerController* TrueFPSPlayerController = Cast<ATrueFPSPlayerController>(BaseController);
			if (TrueFPSPlayerController)
			{
				TrueFPSPlayerController->SetIsVibrationEnabled(bVibrationOpt);
			}
			else
			{
				// We are in the menus and therefore don't need to do anything as the controller is different
				// and can't store the vibration setting.
			}
		}
	}
}

void FTrueFPSOptions::OnApplySettings()
{
	FSlateApplication::Get().PlaySound(OptionsStyle->AcceptChangesSound, GetOwnerUserIndex());
	ApplySettings();
}

void FTrueFPSOptions::ApplySettings()
{
	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->SetAimSensitivity(SensitivityOpt);
		PersistentUser->SetInvertedYAxis(bInvertYAxisOpt);
		PersistentUser->SetGamma(GammaOpt);
		PersistentUser->SetVibration(bVibrationOpt);
		PersistentUser->TellInputAboutKeybindings();

		PersistentUser->SaveIfDirty();
	}

	if (UserSettings->IsForceSystemResolution())
	{
		// store the current system resolution
		ResolutionOpt = FIntPoint(GSystemResolution.ResX, GSystemResolution.ResY);
	}
	UserSettings->SetScreenResolution(ResolutionOpt);
	UserSettings->SetFullscreenMode(bFullScreenOpt);
	UserSettings->SetGraphicsQuality(GraphicsQualityOpt);
	UserSettings->SetNVIDIAReflex(NVIDIAReflexOpt);
	UserSettings->SetLatencyFlashIndicator(LatencyFlashIndicatorOpt);
	UserSettings->SetFramerateVisibility(FramerateOpt);
	UserSettings->SetGameToRenderVisibility(GameToRenderOpt);
	UserSettings->SetGameLatencyVisibility(GameLatencyOpt);
	UserSettings->SetRenderLatencyVisibility(RenderLatencyOpt);
	UserSettings->ApplySettings(false);

	OnApplyChanges.ExecuteIfBound();
}

void FTrueFPSOptions::TellInputAboutKeybindings()
{
	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

void FTrueFPSOptions::RevertChanges()
{
	FSlateApplication::Get().PlaySound(OptionsStyle->DiscardChangesSound, GetOwnerUserIndex());
	UpdateOptions();
	GEngine->DisplayGamma =  2.2f + 2.0f * (-0.5f + GammaOption->SelectedMultiChoice / 100.0f);
}

int32 FTrueFPSOptions::GetCurrentResolutionIndex(FIntPoint CurrentRes)
{
	int32 Result = 0; // return first valid resolution if match not found
	for (int32 i = 0; i < Resolutions.Num(); i++)
	{
		if (Resolutions[i] == CurrentRes)
		{
			Result = i;
			break;
		}
	}
	return Result;
}

int32 FTrueFPSOptions::GetCurrentMouseYAxisInvertedIndex()
{
	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		return InvertYAxisOption->SelectedMultiChoice = PersistentUser->GetInvertedYAxis() ? 1 : 0;
	}
	else
	{
		return 0;
	}
}

int32 FTrueFPSOptions::GetCurrentMouseSensitivityIndex()
{
	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		//mouse sensitivity is a floating point value ranged from 0.0f to 1.0f
		int32 IntSensitivity = FMath::RoundToInt((PersistentUser->GetAimSensitivity() - 0.5f) * 10.0f);
		//Clamp to valid index range
		return FMath::Clamp(IntSensitivity, MinSensitivity, 100);
	}

	return FMath::RoundToInt((1.0f - 0.5f) * 10.0f);
}

int32 FTrueFPSOptions::GetCurrentGammaIndex()
{
	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		//reverse gamma calculation
		int32 GammaIndex = FMath::TruncToInt(((PersistentUser->GetGamma() - 2.2f) / 2.0f + 0.5f) * 100);
		//Clamp to valid index range
		return FMath::Clamp(GammaIndex, 0, 100);
	}

	return FMath::TruncToInt(((2.2f - 2.2f) / 2.0f + 0.5f) * 100);
}

int32 FTrueFPSOptions::GetOwnerUserIndex() const
{
	return PlayerOwner ? PlayerOwner->GetControllerId() : 0;
}

UTrueFPSPersistentUser* FTrueFPSOptions::GetPersistentUser() const
{
	UTrueFPSLocalPlayer* const SLP = Cast<UTrueFPSLocalPlayer>(PlayerOwner);
	if (SLP)
	{
		return SLP->GetPersistentUser();
	}

	return nullptr;
}

void FTrueFPSOptions::UpdateOptions()
{
#if UE_BUILD_SHIPPING
	CheatsItem->bVisible = false;
#else
	//Toggle Cheat menu visibility depending if we are client or server
	UWorld* const World = PlayerOwner->GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		CheatsItem->bVisible = false;
	}
	else
	{
		CheatsItem->bVisible = true;
	}
#endif

	//grab the user settings
	UTrueFPSPersistentUser* const PersistentUser = GetPersistentUser();
	if (PersistentUser)
	{
		// Update bInvertYAxisOpt, SensitivityOpt and GammaOpt because the TrueFPSOptions can be created without the controller having a player
		// by the in-game menu which will leave them with default values
		bInvertYAxisOpt = PersistentUser->GetInvertedYAxis();
		SensitivityOpt = PersistentUser->GetAimSensitivity();
		GammaOpt = PersistentUser->GetGamma();
		bVibrationOpt = PersistentUser->GetVibration();
	} 

	InvertYAxisOption->SelectedMultiChoice =  GetCurrentMouseYAxisInvertedIndex();
	AimSensitivityOption->SelectedMultiChoice = GetCurrentMouseSensitivityIndex();
	GammaOption->SelectedMultiChoice = GetCurrentGammaIndex();
	VibrationOption->SelectedMultiChoice = bVibrationOpt ? 1 : 0;

	GammaOptionChanged(GammaOption, GammaOption->SelectedMultiChoice);
#if PLATFORM_DESKTOP
	VideoResolutionOption->SelectedMultiChoice = GetCurrentResolutionIndex(UserSettings->GetScreenResolution());
	GraphicsQualityOption->SelectedMultiChoice = UserSettings->GetGraphicsQuality();
	FullScreenOption->SelectedMultiChoice = UserSettings->GetFullscreenMode() != EWindowMode::Windowed ? 1 : 0;
	if (NVIDIAReflexOption.IsValid())
	{
		NVIDIAReflexOption->SelectedMultiChoice = UserSettings->GetNVIDIAReflex();
		LatencyFlashIndicatorOption->SelectedMultiChoice = UserSettings->GetLatencyFlashIndicator();
		FramerateOption->SelectedMultiChoice = UserSettings->GetFramerateVisibility();
		GameToRenderOption->SelectedMultiChoice = UserSettings->GetGameToRenderVisibility();
		GameLatencyOption->SelectedMultiChoice = UserSettings->GetGameLatencyVisibility();
		RenderLatencyOption->SelectedMultiChoice = UserSettings->GetRenderLatencyVisibility();
	}
#endif
}

void FTrueFPSOptions::InfiniteAmmoOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	UWorld* const World = PlayerOwner->GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			ATrueFPSPlayerController* TrueFPSPlayerController = Cast<ATrueFPSPlayerController>(*It);
			if (TrueFPSPlayerController)
			{
				TrueFPSPlayerController->SetInfiniteAmmo(MultiOptionIndex > 0 ? true : false);
			}
		}
	}
}

void FTrueFPSOptions::InfiniteClipOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	UWorld* const World = PlayerOwner->GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			ATrueFPSPlayerController* const TrueFPSPlayerController = Cast<ATrueFPSPlayerController>(*It);
			if (TrueFPSPlayerController)
			{
				TrueFPSPlayerController->SetInfiniteClip(MultiOptionIndex > 0 ? true : false);
			}
		}
	}
}

void FTrueFPSOptions::FreezeTimerOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	UWorld* const World = PlayerOwner->GetWorld();
	ATrueFPSGameState* const GameState = World ? World->GetGameState<ATrueFPSGameState>() : nullptr;
	if (GameState)
	{
		GameState->bTimerPaused = MultiOptionIndex > 0  ? true : false;
	}
}


void FTrueFPSOptions::HealthRegenOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	UWorld* const World = PlayerOwner->GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			ATrueFPSPlayerController* const TrueFPSPlayerController = Cast<ATrueFPSPlayerController>(*It);
			if (TrueFPSPlayerController)
			{
				TrueFPSPlayerController->SetHealthRegen(MultiOptionIndex > 0 ? true : false);
			}
		}
	}
}

void FTrueFPSOptions::VideoResolutionOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	ResolutionOpt = Resolutions[MultiOptionIndex];
}

void FTrueFPSOptions::GraphicsQualityOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	GraphicsQualityOpt = MultiOptionIndex;
}

void FTrueFPSOptions::FullScreenOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	static auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.FullScreenMode"));
	EWindowMode::Type FullScreenMode = CVar->GetValueOnGameThread() == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Fullscreen;
	bFullScreenOpt = MultiOptionIndex == 0 ? EWindowMode::Windowed : FullScreenMode;
}

void FTrueFPSOptions::AimSensitivityOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	SensitivityOpt = 0.5f + (MultiOptionIndex / 10.0f);
}

void FTrueFPSOptions::GammaOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	GammaOpt = 2.2f + 2.0f * (-0.5f + MultiOptionIndex / 100.0f);
	GEngine->DisplayGamma = GammaOpt;
}

void FTrueFPSOptions::ToggleVibration(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	bVibrationOpt = MultiOptionIndex > 0 ? true : false;
	APlayerController* BaseController = Cast<APlayerController>(UGameplayStatics::GetPlayerController(PlayerOwner->GetWorld(), GetOwnerUserIndex()));
	ATrueFPSPlayerController* TrueFPSPlayerController = Cast<ATrueFPSPlayerController>(UGameplayStatics::GetPlayerController(PlayerOwner->GetWorld(), GetOwnerUserIndex()));
	ensure(BaseController);
    if(BaseController)
    {
		if (TrueFPSPlayerController)
		{
			TrueFPSPlayerController->SetIsVibrationEnabled(bVibrationOpt);
		}
		else
		{
			// We are in the menus and therefore don't need to do anything as the controller is different
			// and can't store the vibration setting.
		}
    }
}

void FTrueFPSOptions::InvertYAxisOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	bInvertYAxisOpt = MultiOptionIndex > 0  ? true : false;
}

void FTrueFPSOptions::NVIDIAReflexChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	NVIDIAReflexOpt = MultiOptionIndex;
}

void FTrueFPSOptions::LatencyFlashIndicatorChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	LatencyFlashIndicatorOpt = MultiOptionIndex;
}

void FTrueFPSOptions::FramerateOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	FramerateOpt = MultiOptionIndex;
}

void FTrueFPSOptions::GameToRenderOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	GameToRenderOpt = MultiOptionIndex;
}

void FTrueFPSOptions::GameLatencyOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	GameLatencyOpt = MultiOptionIndex;
}

void FTrueFPSOptions::RenderLatencyOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	RenderLatencyOpt = MultiOptionIndex;
}

#undef LOCTEXT_NAMESPACE
