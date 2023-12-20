// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSSystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "UI/Style/TrueFPSStyle.h"

#define LOCTEXT_NAMESPACE "FTrueFPSSystemPluginModule"

void FTrueFPSSystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// InitializeTrueFPSSystemDelegates();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	//Hot reload hack
	FSlateStyleRegistry::UnRegisterSlateStyle(FTrueFPSStyle::GetStyleSetName());
	FTrueFPSStyle::Initialize();
}

void FTrueFPSSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	FTrueFPSStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTrueFPSSystemModule, TrueFPSSystem)

DEFINE_LOG_CATEGORY(LogTrueFPSSystem)