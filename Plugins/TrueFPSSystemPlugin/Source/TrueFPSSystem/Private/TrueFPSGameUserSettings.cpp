// Fill out your copyright notice in the Description page of Project Settings.

#include "TrueFPSGameUserSettings.h"

#include "Performance/LatencyMarkerModule.h"
#include "Performance/MaxTickRateHandlerModule.h"

UTrueFPSGameUserSettings::UTrueFPSGameUserSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetToDefaults();
}

void UTrueFPSGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	GraphicsQuality = 1;	
	bIsLanMatch = true;
	bIsDedicatedServer = false;
	bIsForceSystemResolution = false;
	NVIDIAReflex = 1;
}

void UTrueFPSGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	if (GraphicsQuality == 0)
	{
		ScalabilityQuality.SetFromSingleQualityLevel(1);
	}
	else
	{
		ScalabilityQuality.SetFromSingleQualityLevel(3);
	}

	InitNVIDIAReflex();

	Super::ApplySettings(bCheckForCommandLineOverrides);
}

void UTrueFPSGameUserSettings::InitNVIDIAReflex()
{
	UTrueFPSGameUserSettings* const UserSettings = CastChecked<UTrueFPSGameUserSettings>(GEngine->GetGameUserSettings());
	if (UserSettings == nullptr)
		return;

	TArray<IMaxTickRateHandlerModule*> MaxTickRateHandlerModules = IModularFeatures::Get().GetModularFeatureImplementations<IMaxTickRateHandlerModule>(IMaxTickRateHandlerModule::GetModularFeatureName());
	for (IMaxTickRateHandlerModule* MaxTickRateHandlerModule : MaxTickRateHandlerModules)
	{
		if (UserSettings->GetNVIDIAReflex() != 0)
		{
			MaxTickRateHandlerModule->SetEnabled(true);
			MaxTickRateHandlerModule->SetFlags(UserSettings->GetNVIDIAReflex() == 1 ? 1 : 3);
		}
		else
		{
			MaxTickRateHandlerModule->SetEnabled(false);
		}
	}

	TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get().GetModularFeatureImplementations<ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
	for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
	{
		if (UserSettings->GetLatencyFlashIndicator() != 0)
		{
			LatencyMarkerModule->SetFlashIndicatorEnabled(true);
		}
		else
		{
			LatencyMarkerModule->SetFlashIndicatorEnabled(false);
		}
	}
}