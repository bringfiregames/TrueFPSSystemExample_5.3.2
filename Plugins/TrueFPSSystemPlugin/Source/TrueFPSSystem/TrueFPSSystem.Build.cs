// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TrueFPSSystem : ModuleRules
{
	public TrueFPSSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDefinitions.Add("HOST_ONLINE_GAMEMODE_ENABLED=" + HostOnlineGameEnabled);
		PublicDefinitions.Add("JOIN_ONLINE_GAME_ENABLED=" + JoinOnlineGameEnabled);
		PublicDefinitions.Add("INVITE_ONLINE_GAME_ENABLED=" + InviteOnlineGameEnabled);
		PublicDefinitions.Add("ONLINE_STORE_ENABLED=" + OnlineStoreEnabled);

		PrivateIncludePaths.AddRange(
			new string[] { 
				"TrueFPSSystem/Private",
				"TrueFPSSystem/Private/UI",
				"TrueFPSSystem/Private/UI/Menu",
				"TrueFPSSystem/Private/UI/Style",
				"TrueFPSSystem/Private/UI/Widgets",
			}
		);
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject", 
				"Engine", 
				"InputCore", 
				"NavigationSystem",
				"AIModule",
				"PhysicsCore",
				"AnimationCore",
				"AnimGraphRuntime", 
				"AnimGraph", 
				"GameplayTags", 
				"GameplayTasks",
				"Niagara",
				"EnhancedInput",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"AssetRegistry",
				"NetCore",
				"Gauntlet",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"TrueFPSSystemLoadingScreen",
				"Json",
				"ApplicationCore",
				"ReplicationGraph",
				"PakFile",
				"RHI",
				"GameplayCameras"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				"OnlineSubsystemNull",
				"NetworkReplayStreaming",
				"NullNetworkReplayStreaming",
				"HttpNetworkReplayStreaming",
				"LocalFileNetworkReplayStreaming"
			}
			);

		PrivateIncludePathModuleNames.AddRange(
			new string[]
			{
				"NetworkReplayStreaming"
			}
			);

		if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
		{
			PrivateDependencyModuleNames.Add("GameplayDebugger");
			PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
		}
		
		if (Target.Version.MajorVersion >= 5 || (Target.Version.MajorVersion == 4 && Target.Version.MinorVersion >= 27))
		{
			PrivateDependencyModuleNames.Add("GameplayCameras");
		}
	}
	
	protected virtual int HostOnlineGameEnabled 
	{ 
		get 
		{ 
			return 1; 
		} 
	}

	protected virtual int JoinOnlineGameEnabled
	{
		get
		{
			return 1;
		}
	}

	protected virtual int InviteOnlineGameEnabled
	{
		get
		{
			return 1;
		}
	}

	protected virtual int OnlineStoreEnabled
	{
		get
		{
			return 1;
		}
	}
}
