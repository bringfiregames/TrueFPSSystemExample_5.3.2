// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSMainMenu.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "TrueFPSGameInstance.h"
#include "TrueFPSGameUserSettings.h"
#include "TrueFPSGameViewportClient.h"
#include "TrueFPSSystem.h"
#include "Character/TrueFPSLocalPlayer.h"
#include "Character/TrueFPSPersistentUser.h"
#include "GenericPlatform/GenericPlatformChunkInstall.h"
#include "Online/TrueFPSGameMode.h"
#include "Online/TrueFPSOnlineGameSettings.h"
#include "TrueFPSSystemLoadingScreen/Public/TrueFPSSystemLoadingScreen.h"
#include "UI/Style/TrueFPSMenuSoundsWidgetStyle.h"
#include "UI/Style/TrueFPSStyle.h"
#include "UI/Widgets/STrueFPSConfirmationDialog.h"
#include "UI/Widgets/STrueFPSSplitScreenLobbyWidget.h"
#include "Widgets/STrueFPSDemoList.h"
#include "Widgets/STrueFPSLeaderboard.h"
#include "Widgets/STrueFPSMenuWidget.h"
#include "Widgets/STrueFPSOnlineStore.h"
#include "Widgets/STrueFPSServerList.h"

#define LOCTEXT_NAMESPACE "TrueFPSSystem.HUD.Menu"

#define MAX_BOT_COUNT 8

static const FString MapNames[] = { TEXT("Sanctuary"), TEXT("Highrise") };
static const FString JoinMapNames[] = { TEXT("Any"), TEXT("Sanctuary"), TEXT("Highrise") };
static const FName PackageNames[] = { TEXT("Sanctuary.umap"), TEXT("Highrise.umap") };
static const int DefaultTDMMap = 1;
static const int DefaultFFAMap = 0; 
static const float QuickmatchUIAnimationTimeDuration = 30.f;

//use an EMap index, get back the ChunkIndex that map should be part of.
//Instead of this mapping we should really use the AssetRegistry to query for chunk mappings, but maps aren't members of the AssetRegistry yet.
static const int ChunkMapping[] = { 1, 2 };

#if PLATFORM_SWITCH
#	define LOGIN_REQUIRED_FOR_ONLINE_PLAY 1
#else
#	define LOGIN_REQUIRED_FOR_ONLINE_PLAY 0
#endif

#if PLATFORM_SWITCH
#	define CONSOLE_LAN_SUPPORTED 1
#else
#	define CONSOLE_LAN_SUPPORTED 0
#endif

#if !defined(TRUEFPS_XBOX_MENU)
	#define TRUEFPS_XBOX_MENU 0
#endif

FTrueFPSMainMenu::~FTrueFPSMainMenu()
{
	CleanupOnlinePrivilegeTask();
}

void FTrueFPSMainMenu::Construct(TWeakObjectPtr<UTrueFPSGameInstance> _GameInstance, TWeakObjectPtr<ULocalPlayer> _PlayerOwner)
{
	// We'll attempt login if we are loading the main menu while still logged out
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = _PlayerOwner->GetControllerId();

		if (Identity->GetLoginStatus(ControllerId) != ELoginStatus::LoggedIn)
		{
			OnLoginCompleteConstructDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FTrueFPSMainMenu::OnLoginCompleteConstruct));

			Identity->AutoLogin(ControllerId);
		}
		else
		{
			if (GameInstance.IsValid())
			{
				GameInstance->SetPresenceForLocalPlayer(ControllerId, FString(TEXT("In Menu")), FVariantData(FString(TEXT("OnMenu"))));
			}
		}
	}

	bShowingDownloadPct = false;
	bAnimateQuickmatchSearchingUI = false;
	bUsedInputToCancelQuickmatchSearch = false;
	bQuickmatchSearchRequestCanceled = false;
	bIncQuickMAlpha = false;
	PlayerOwner = _PlayerOwner;
	MatchType = EMatchType::Custom;

	check(_GameInstance.IsValid());

	GameInstance = _GameInstance;
	PlayerOwner = _PlayerOwner;

	OnCancelMatchmakingCompleteDelegate = FOnCancelMatchmakingCompleteDelegate::CreateSP(this, &FTrueFPSMainMenu::OnCancelMatchmakingComplete);
	
	// read user settings
#if TRUEFPS_CONSOLE_UI
	bIsLanMatch = FParse::Param(FCommandLine::Get(), TEXT("forcelan"));
#else
	UTrueFPSGameUserSettings* const UserSettings = CastChecked<UTrueFPSGameUserSettings>(GEngine->GetGameUserSettings());
	bIsLanMatch = UserSettings->IsLanMatch();
	bIsDedicatedServer = UserSettings->IsDedicatedServer();
#endif

	BotsCountOpt = 1;
	bIsRecordingDemo = false;
	bIsQuitting = false;

	if(GetPersistentUser())
	{
		BotsCountOpt = GetPersistentUser()->GetBotsCount();
		bIsRecordingDemo = GetPersistentUser()->IsRecordingDemos();
	}		

	// number entries 0 up to MAX_BOX_COUNT
	TArray<FText> BotsCountList;
	for (int32 i = 0; i <= MAX_BOT_COUNT; i++)
	{
		BotsCountList.Add(FText::AsNumber(i));
	}
	
	TArray<FText> MapList;
	for (int32 i = 0; i < UE_ARRAY_COUNT(MapNames); ++i)
	{
		MapList.Add(FText::FromString(MapNames[i]));		
	}

	TArray<FText> JoinMapList;
	for (int32 i = 0; i < UE_ARRAY_COUNT(JoinMapNames); ++i)
	{
		JoinMapList.Add(FText::FromString(JoinMapNames[i]));
	}

	TArray<FText> OnOffList;
	OnOffList.Add( LOCTEXT("Off","OFF") );
	OnOffList.Add( LOCTEXT("On","ON") );

	TrueFPSOptions = MakeShareable(new FTrueFPSOptions()); 
	TrueFPSOptions->Construct(GetPlayerOwner());
	TrueFPSOptions->TellInputAboutKeybindings();
	TrueFPSOptions->OnApplyChanges.BindSP(this, &FTrueFPSMainMenu::CloseSubMenu);

	//Now that we are here, build our menu 
	MenuWidget.Reset();
	MenuWidgetContainer.Reset();

	TArray<FString> Keys;
	GConfig->GetSingleLineArray(TEXT("/Script/SwitchRuntimeSettings.SwitchRuntimeSettings"), TEXT("LeaderboardMap"), Keys, GEngineIni);

	if (GEngine && GEngine->GameViewport)
	{		
		SAssignNew(MenuWidget, STrueFPSMenuWidget)
			.Cursor(EMouseCursor::Default)
			.PlayerOwner(GetPlayerOwner())
			.IsGameMenu(false);

		SAssignNew(MenuWidgetContainer, SWeakWidget)
			.PossiblyNullContent(MenuWidget);		

		TSharedPtr<FTrueFPSMenuItem> RootMenuItem;

				
		SAssignNew(SplitScreenLobbyWidget, STrueFPSSplitScreenLobby)
			.PlayerOwner(GetPlayerOwner())
			.OnCancelClicked(FOnClicked::CreateSP(this, &FTrueFPSMainMenu::OnSplitScreenBackedOut)) 
			.OnPlayClicked(FOnClicked::CreateSP(this, &FTrueFPSMainMenu::OnSplitScreenPlay));

		FText Msg = LOCTEXT("No matches could be found", "No matches could be found");
		FText OKButtonString = NSLOCTEXT("DialogButtons", "OKAY", "OK");
		QuickMatchFailureWidget = SNew(STrueFPSConfirmationDialog).PlayerOwner(PlayerOwner)			
			.MessageText(Msg)
			.ConfirmText(OKButtonString)
			.CancelText(FText())
			.OnConfirmClicked(FOnClicked::CreateRaw(this, &FTrueFPSMainMenu::OnQuickMatchFailureUICancel))
			.OnCancelClicked(FOnClicked::CreateRaw(this, &FTrueFPSMainMenu::OnQuickMatchFailureUICancel));

		Msg = LOCTEXT("Searching for Match...", "SEARCHING FOR MATCH...");
		OKButtonString = LOCTEXT("Stop", "STOP");
		QuickMatchSearchingWidget = SNew(STrueFPSConfirmationDialog).PlayerOwner(PlayerOwner)			
			.MessageText(Msg)
			.ConfirmText(OKButtonString)
			.CancelText(FText())
			.OnConfirmClicked(FOnClicked::CreateRaw(this, &FTrueFPSMainMenu::OnQuickMatchSearchingUICancel))
			.OnCancelClicked(FOnClicked::CreateRaw(this, &FTrueFPSMainMenu::OnQuickMatchSearchingUICancel));

		SAssignNew(SplitScreenLobbyWidgetContainer, SWeakWidget)
			.PossiblyNullContent(SplitScreenLobbyWidget);		

		SAssignNew(QuickMatchFailureWidgetContainer, SWeakWidget)
			.PossiblyNullContent(QuickMatchFailureWidget);	

		SAssignNew(QuickMatchSearchingWidgetContainer, SWeakWidget)
			.PossiblyNullContent(QuickMatchSearchingWidget);

		FText StoppingOKButtonString = LOCTEXT("Stopping", "STOPPING...");
		QuickMatchStoppingWidget = SNew(STrueFPSConfirmationDialog).PlayerOwner(PlayerOwner)			
			.MessageText(Msg)
			.ConfirmText(StoppingOKButtonString)
			.CancelText(FText())
			.OnConfirmClicked(FOnClicked())
			.OnCancelClicked(FOnClicked());

		SAssignNew(QuickMatchStoppingWidgetContainer, SWeakWidget)
			.PossiblyNullContent(QuickMatchStoppingWidget);

#if TRUEFPS_XBOX_MENU
		TSharedPtr<FTrueFPSMenuItem> MenuItem;

		// HOST ONLINE menu option
		{
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("HostCustom", "HOST CUSTOM"), this, &FTrueFPSMainMenu::OnHostOnlineSelected);

			// submenu under "HOST ONLINE"
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FTrueFPSMainMenu::OnSplitScreenSelected);

			TSharedPtr<FTrueFPSMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FTrueFPSMainMenu::BotCountOptionChanged);				
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOnlineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
		}

		// JOIN menu option
		{
			// JOIN menu option
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("FindCustom", "FIND CUSTOM"), this, &FTrueFPSMainMenu::OnJoinServer);

			// Server list widget that will be called up if appropriate
			MenuHelper::AddCustomMenuItem(JoinServerItem,SAssignNew(ServerListWidget,STrueFPSServerList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
		}

		// QUICK MATCH menu option
		{
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("QuickMatch", "QUICK MATCH"), this, &FTrueFPSMainMenu::OnQuickMatchSelected);
		}

		// HOST OFFLINE menu option
		{
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("PlayOffline", "PLAY OFFLINE"),this, &FTrueFPSMainMenu::OnHostOfflineSelected);

			// submenu under "HOST OFFLINE"
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FTrueFPSMainMenu::OnSplitScreenSelected);

			TSharedPtr<FTrueFPSMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FTrueFPSMainMenu::BotCountOptionChanged);				
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOfflineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
		}
#elif TRUEFPS_CONSOLE_UI
		TSharedPtr<FTrueFPSMenuItem> MenuItem;

#if HOST_ONLINE_GAMEMODE_ENABLED
		// HOST ONLINE menu option
		{
			HostOnlineMenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("HostOnline", "HOST ONLINE"), this, &FTrueFPSMainMenu::OnHostOnlineSelected);

			// submenu under "HOST ONLINE"
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
			MenuHelper::AddMenuItemSP(HostOnlineMenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FTrueFPSMainMenu::OnSplitScreenSelectedHostOnlineLoginRequired);
#else
			MenuHelper::AddMenuItemSP(HostOnlineMenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FTrueFPSMainMenu::OnSplitScreenSelectedHostOnline);
#endif

			TSharedPtr<FTrueFPSMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(HostOnlineMenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FTrueFPSMainMenu::BotCountOptionChanged);
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOnlineMapOption = MenuHelper::AddMenuOption(HostOnlineMenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
#if CONSOLE_LAN_SUPPORTED
			HostLANItem = MenuHelper::AddMenuOptionSP(HostOnlineMenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FTrueFPSMainMenu::LanMatchChanged);
			HostLANItem->SelectedMultiChoice = bIsLanMatch;
#endif
		}
#endif //HOST_ONLINE_GAMEMODE_ENABLED
		// HOST OFFLINE menu option
		{
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("HostOffline", "HOST OFFLINE"),this, &FTrueFPSMainMenu::OnHostOfflineSelected);

			// submenu under "HOST OFFLINE"
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FTrueFPSMainMenu::OnSplitScreenSelected);

			TSharedPtr<FTrueFPSMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FTrueFPSMainMenu::BotCountOptionChanged);				
			NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;																

			HostOfflineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);
		}

		// QUICK MATCH menu option
		{
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("QuickMatch", "QUICK MATCH"), this, &FTrueFPSMainMenu::OnQuickMatchSelectedLoginRequired);
#else
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("QuickMatch", "QUICK MATCH"), this, &FTrueFPSMainMenu::OnQuickMatchSelected);
#endif
		}

#if JOIN_ONLINE_GAME_ENABLED
		// JOIN menu option
		{
			// JOIN menu option
			MenuItem = MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Join", "JOIN"), this, &FTrueFPSMainMenu::OnJoinSelected);

			// submenu under "join"
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("Server", "SERVER"), this, &FTrueFPSMainMenu::OnJoinServerLoginRequired);
#else
			MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("Server", "SERVER"), this, &FTrueFPSMainMenu::OnJoinServer);
#endif
			JoinMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), JoinMapList);

			// Server list widget that will be called up if appropriate
			MenuHelper::AddCustomMenuItem(JoinServerItem,SAssignNew(ServerListWidget,STrueFPSServerList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));

#if CONSOLE_LAN_SUPPORTED
			JoinLANItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FTrueFPSMainMenu::LanMatchChanged);
			JoinLANItem->SelectedMultiChoice = bIsLanMatch;
#endif
		}
#endif //JOIN_ONLINE_GAME_ENABLED

#else
		TSharedPtr<FTrueFPSMenuItem> MenuItem;
		// HOST menu option
		MenuItem = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Host", "HOST"));

		// submenu under "host"
		MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("FFALong", "FREE FOR ALL"), this, &FTrueFPSMainMenu::OnUIHostFreeForAll);
		MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("TDMLong", "TEAM DEATHMATCH"), this, &FTrueFPSMainMenu::OnUIHostTeamDeathMatch);

		TSharedPtr<FTrueFPSMenuItem> NumberOfBotsOption = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("NumberOfBots", "NUMBER OF BOTS"), BotsCountList, this, &FTrueFPSMainMenu::BotCountOptionChanged);
		NumberOfBotsOption->SelectedMultiChoice = BotsCountOpt;

		HostOnlineMapOption = MenuHelper::AddMenuOption(MenuItem, LOCTEXT("SELECTED_LEVEL", "Map"), MapList);

		HostLANItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FTrueFPSMainMenu::LanMatchChanged);
		HostLANItem->SelectedMultiChoice = bIsLanMatch;

		RecordDemoItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("RecordDemo", "Record Demo"), OnOffList, this, &FTrueFPSMainMenu::RecordDemoChanged);
		RecordDemoItem->SelectedMultiChoice = bIsRecordingDemo;

		// JOIN menu option
		MenuItem = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Join", "JOIN"));

		// submenu under "join"
		MenuHelper::AddMenuItemSP(MenuItem, LOCTEXT("Server", "SERVER"), this, &FTrueFPSMainMenu::OnJoinServer);
		JoinLANItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("LanMatch", "LAN"), OnOffList, this, &FTrueFPSMainMenu::LanMatchChanged);
		JoinLANItem->SelectedMultiChoice = bIsLanMatch;

		DedicatedItem = MenuHelper::AddMenuOptionSP(MenuItem, LOCTEXT("Dedicated", "Dedicated"), OnOffList, this, &FTrueFPSMainMenu::DedicatedServerChanged);
		DedicatedItem->SelectedMultiChoice = bIsDedicatedServer;

		// Server list widget that will be called up if appropriate
		MenuHelper::AddCustomMenuItem(JoinServerItem,SAssignNew(ServerListWidget,STrueFPSServerList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
#endif

		// Leaderboards
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Leaderboards", "LEADERBOARDS"), this, &FTrueFPSMainMenu::OnShowLeaderboard);
		MenuHelper::AddCustomMenuItem(LeaderboardItem,SAssignNew(LeaderboardWidget,STrueFPSLeaderboard).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));

#if ONLINE_STORE_ENABLED
		// Purchases
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Store", "ONLINE STORE"), this, &FTrueFPSMainMenu::OnShowOnlineStore);
		MenuHelper::AddCustomMenuItem(OnlineStoreItem, SAssignNew(OnlineStoreWidget, STrueFPSOnlineStore).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
#endif //ONLINE_STORE_ENABLED
#if !TRUEFPS_CONSOLE_UI

		// Demos
		{
			MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Demos", "DEMOS"), this, &FTrueFPSMainMenu::OnShowDemoBrowser);
			MenuHelper::AddCustomMenuItem(DemoBrowserItem,SAssignNew(DemoListWidget,STrueFPSDemoList).OwnerWidget(MenuWidget).PlayerOwner(GetPlayerOwner()));
		}
#endif

		// Options
		MenuHelper::AddExistingMenuItem(RootMenuItem, TrueFPSOptions->OptionsItem.ToSharedRef());

		if(FSlateApplication::Get().SupportsSystemHelp())
		{
			TSharedPtr<FTrueFPSMenuItem> HelpSubMenu = MenuHelper::AddMenuItem(RootMenuItem, LOCTEXT("Help", "HELP"));
			HelpSubMenu->OnConfirmMenuItem.BindStatic([](){ FSlateApplication::Get().ShowSystemHelp(); });
		}

		// QUIT option (for PC)
#if TRUEFPS_SHOW_QUIT_MENU_ITEM
		MenuHelper::AddMenuItemSP(RootMenuItem, LOCTEXT("Quit", "QUIT"), this, &FTrueFPSMainMenu::OnUIQuit);
#endif

		MenuWidget->CurrentMenuTitle = LOCTEXT("MainMenu","MAIN MENU");
		MenuWidget->OnGoBack.BindSP(this, &FTrueFPSMainMenu::OnMenuGoBack);
		MenuWidget->MainMenu = MenuWidget->CurrentMenu = RootMenuItem->SubMenu;
		MenuWidget->OnMenuHidden.BindSP(this, &FTrueFPSMainMenu::OnMenuHidden);

		TrueFPSOptions->UpdateOptions();
		MenuWidget->BuildAndShowMenu();
	}
}

void FTrueFPSMainMenu::AddMenuToGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		UGameViewportClient* GVC = GEngine->GameViewport;
		GVC->AddViewportWidgetContent(MenuWidgetContainer.ToSharedRef());
		GVC->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
	}
}

void FTrueFPSMainMenu::RemoveMenuFromGameViewport()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidgetContainer.ToSharedRef());
	}
}

void FTrueFPSMainMenu::Tick(float DeltaSeconds)
{
	if (bAnimateQuickmatchSearchingUI)
	{
		FLinearColor QuickMColor = QuickMatchSearchingWidget->GetColorAndOpacity();
		if (bIncQuickMAlpha)
		{
			if (QuickMColor.A >= 1.f)
			{
				bIncQuickMAlpha = false;
			}
			else
			{
				QuickMColor.A += DeltaSeconds;
			}
		}
		else
		{
			if (QuickMColor.A <= .1f)
			{
				bIncQuickMAlpha = true;
			}
			else
			{
				QuickMColor.A -= DeltaSeconds;
			}
		}
		QuickMatchSearchingWidget->SetColorAndOpacity(QuickMColor);
		QuickMatchStoppingWidget->SetColorAndOpacity(QuickMColor);
	}

	IPlatformChunkInstall* ChunkInstaller = FPlatformMisc::GetPlatformChunkInstall();
	if (ChunkInstaller)
	{
		EMap SelectedMap = GetSelectedMap();
		// use assetregistry when maps are added to it.
		int32 MapChunk = ChunkMapping[(int)SelectedMap];
		EChunkLocation::Type ChunkLocation = ChunkInstaller->GetPakchunkLocation(MapChunk);

		FText UpdatedText;
		bool bUpdateText = false;
		if (ChunkLocation == EChunkLocation::NotAvailable)
		{			
			float PercentComplete = FMath::Min(ChunkInstaller->GetChunkProgress(MapChunk, EChunkProgressReportingType::PercentageComplete), 100.0f);									
			UpdatedText = FText::FromString(FString::Printf(TEXT("%s %4.0f%%"),*LOCTEXT("SELECTED_LEVEL", "Map").ToString(), PercentComplete));
			bUpdateText = true;
			bShowingDownloadPct = true;
		}
		else if (bShowingDownloadPct)
		{
			UpdatedText = LOCTEXT("SELECTED_LEVEL", "Map");			
			bUpdateText = true;
			bShowingDownloadPct = false;			
		}

		if (bUpdateText)
		{
			if (GameInstance.IsValid() && GameInstance->GetOnlineMode() != EOnlineMode::Offline && HostOnlineMapOption.IsValid())
			{
				HostOnlineMapOption->SetText(UpdatedText);
			}
			else if (HostOfflineMapOption.IsValid())
			{
				HostOfflineMapOption->SetText(UpdatedText);
			}
		}
	}
}

TStatId FTrueFPSMainMenu::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FTrueFPSMainMenu, STATGROUP_Tickables);
}

void FTrueFPSMainMenu::OnMenuHidden()
{	
#if TRUEFPS_CONSOLE_UI
	if (bIsQuitting)
	{
		RemoveMenuFromGameViewport();
	}
	// Menu was hidden from the top-level main menu, on consoles show the welcome screen again.
	else if ( ensure(GameInstance.IsValid()))
	{
		GameInstance->GotoState(TrueFPSGameInstanceState::WelcomeScreen);
	}
#else
	RemoveMenuFromGameViewport();
#endif
}

void FTrueFPSMainMenu::OnQuickMatchSelectedLoginRequired()
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = GetPlayerOwner()->GetControllerId();

		OnLoginCompleteQuickmatchDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FTrueFPSMainMenu::OnLoginCompleteQuickmatch));
		Identity->Login(ControllerId, FOnlineAccountCredentials());
	}
}

void FTrueFPSMainMenu::OnLoginCompleteQuickmatch(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteQuickmatchDelegateHandle);
	}

	OnQuickMatchSelected();
}

void FTrueFPSMainMenu::OnQuickMatchSelected()
{
	bQuickmatchSearchRequestCanceled = false;
#if TRUEFPS_CONSOLE_UI
	if ( !ValidatePlayerForOnlinePlay(GetPlayerOwner()) )
	{
		return;
	}
#endif

	StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FTrueFPSMainMenu::OnUserCanPlayOnlineQuickMatch));
}

void FTrueFPSMainMenu::OnUserCanPlayOnlineQuickMatch(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	MenuWidget->LockControls(false);
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{
		if (GameInstance.IsValid())
		{
			GameInstance->SetOnlineMode(EOnlineMode::Online);
		}

		MatchType = EMatchType::Quick;

		SplitScreenLobbyWidget->SetIsJoining(false);

		// Skip splitscreen for PS4
#if MAX_LOCAL_PLAYERS == 1
		BeginQuickMatchSearch();
#else
		UGameViewportClient* const GVC = GEngine->GameViewport;

		RemoveMenuFromGameViewport();
		GVC->AddViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());

		SplitScreenLobbyWidget->Clear();
		FSlateApplication::Get().SetKeyboardFocus(SplitScreenLobbyWidget);
#endif
	}
	else if (GameInstance.IsValid())
	{

		GameInstance->DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
	}
}

FReply FTrueFPSMainMenu::OnConfirmGeneric()
{
	UTrueFPSGameViewportClient* TrueFPSViewport = Cast<UTrueFPSGameViewportClient>(GameInstance->GetGameViewportClient());
	if (TrueFPSViewport)
	{
		TrueFPSViewport->HideDialog();
	}

	return FReply::Handled();
}

void FTrueFPSMainMenu::BeginQuickMatchSearch()
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if(!Sessions.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Quick match is not supported: couldn't find online session interface."));
		return;
	}

	if (GetPlayerOwnerControllerId() == -1)
	{
		UE_LOG(LogOnline, Warning, TEXT("Quick match is not supported: Could not get controller id from player owner"));
		return;
	}

	QuickMatchSearchSettings = MakeShared<FTrueFPSOnlineSearchSettings>(false, true);
	QuickMatchSearchSettings->QuerySettings.Set(SEARCH_MATCHMAKING_QUEUE, FString("TeamDeathmatch"), EOnlineComparisonOp::Equals);
	QuickMatchSearchSettings->QuerySettings.Set(SEARCH_XBOX_LIVE_HOPPER_NAME, FString("TeamDeathmatch"), EOnlineComparisonOp::Equals);
	QuickMatchSearchSettings->QuerySettings.Set(SEARCH_XBOX_LIVE_SESSION_TEMPLATE_NAME, FString("MatchSession"), EOnlineComparisonOp::Equals);
	QuickMatchSearchSettings->TimeoutInSeconds = 120.0f;

	FTrueFPSOnlineSessionSettings SessionSettings(false, true, 8);
	SessionSettings.Set(SETTING_GAMEMODE, FString("TDM"), EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::DontAdvertise);
	SessionSettings.Set(SETTING_MATCHING_TIMEOUT, 120.0f, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_SESSION_TEMPLATE_NAME, FString("GameSession"), EOnlineDataAdvertisementType::DontAdvertise);

	TSharedRef<FOnlineSessionSearch> QuickMatchSearchSettingsRef = QuickMatchSearchSettings.ToSharedRef();
	
	DisplayQuickmatchSearchingUI();

	// Perform matchmaking with all local players
	TArray<FSessionMatchmakingUser> LocalPlayers;
	for (auto It = GameInstance->GetLocalPlayerIterator(); It; ++It)
	{
		FUniqueNetIdRepl PlayerId = (*It)->GetPreferredUniqueNetId();
		if (PlayerId.IsValid())
		{
			FSessionMatchmakingUser LocalPlayer = {(*PlayerId).AsShared()};
			LocalPlayers.Emplace(LocalPlayer);
		}
	}

	FOnStartMatchmakingComplete CompletionDelegate;
	CompletionDelegate.BindSP(this, &FTrueFPSMainMenu::OnMatchmakingComplete);
	if (!Sessions->StartMatchmaking(LocalPlayers, NAME_GameSession, SessionSettings, QuickMatchSearchSettingsRef, CompletionDelegate))
	{
		OnMatchmakingComplete(NAME_GameSession, FOnlineError(false), FSessionMatchmakingResults());
	}
}


void FTrueFPSMainMenu::OnSplitScreenSelectedHostOnlineLoginRequired()
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = GetPlayerOwner()->GetControllerId();

		if (bIsLanMatch)
		{
			Identity->Logout(ControllerId);
			OnSplitScreenSelected();
		}
		else
		{
			OnLoginCompleteHostOnlineDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FTrueFPSMainMenu::OnLoginCompleteHostOnline));
			Identity->Login(ControllerId, FOnlineAccountCredentials());
		}
	}
}

void FTrueFPSMainMenu::OnSplitScreenSelected()
{
	if (!IsMapReady())
	{
		return;
	}

	RemoveMenuFromGameViewport();

#if MAX_LOCAL_PLAYERS == 1 || !TRUEFPS_SUPPORTS_OFFLINE_SPLIT_SCREEEN
	if (!TRUEFPS_SUPPORTS_OFFLINE_SPLIT_SCREEEN || (GameInstance.IsValid() && GameInstance->GetOnlineMode() == EOnlineMode::Online))
	{
		OnUIHostTeamDeathMatch();
	}
	else
#endif
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->AddViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());

		SplitScreenLobbyWidget->Clear();
		FSlateApplication::Get().SetKeyboardFocus(SplitScreenLobbyWidget);
	}
}

void FTrueFPSMainMenu::OnHostOnlineSelected()
{
#if TRUEFPS_CONSOLE_UI
	if (!ValidatePlayerIsSignedIn(GetPlayerOwner()))
	{
		return;
	}
#endif

	MatchType = EMatchType::Custom;

	EOnlineMode NewOnlineMode = bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online;
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(NewOnlineMode);
	}
	SplitScreenLobbyWidget->SetIsJoining(false);
	MenuWidget->EnterSubMenu();
}

void FTrueFPSMainMenu::OnUserCanPlayHostOnline(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	MenuWidget->LockControls(false);
	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)	
	{
		OnSplitScreenSelected();
	}
	else if (GameInstance.IsValid())
	{
		GameInstance->DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
	}
}

void FTrueFPSMainMenu::OnLoginCompleteConstruct(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteConstructDelegateHandle);
	}

	if (GameInstance.IsValid())
	{
		GameInstance->SetPresenceForLocalPlayer(LocalUserNum, FString(TEXT("In Menu")), FVariantData(FString(TEXT("OnMenu"))));
	}
}

void FTrueFPSMainMenu::OnLoginCompleteHostOnline(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	Online::GetIdentityInterface(GetTickableGameObjectWorld())->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteHostOnlineDelegateHandle);

	OnSplitScreenSelectedHostOnline();
}

void FTrueFPSMainMenu::OnSplitScreenSelectedHostOnline()
{
#if TRUEFPS_CONSOLE_UI
	if (!ValidatePlayerForOnlinePlay(GetPlayerOwner()))
	{
		return;
	}
#endif

	StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FTrueFPSMainMenu::OnUserCanPlayHostOnline));
}
void FTrueFPSMainMenu::StartOnlinePrivilegeTask(const IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	if (GameInstance.IsValid())
	{
		// Lock controls for the duration of the async task
		MenuWidget->LockControls(true);
		FUniqueNetIdRepl UserId;
		if (PlayerOwner.IsValid())
		{
			UserId = PlayerOwner->GetPreferredUniqueNetId();
		}
		GameInstance->StartOnlinePrivilegeTask(Delegate, EUserPrivileges::CanPlayOnline, UserId.GetUniqueNetId());
	}	
}

void FTrueFPSMainMenu::CleanupOnlinePrivilegeTask()
{
	if (GameInstance.IsValid())
	{
		GameInstance->CleanupOnlinePrivilegeTask();
	}
}

void FTrueFPSMainMenu::OnHostOfflineSelected()
{
	MatchType = EMatchType::Custom;

#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
	Online::GetIdentityInterface(GetTickableGameObjectWorld())->Logout(GetPlayerOwner()->GetControllerId());
#endif

	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(EOnlineMode::Offline);
	}
	SplitScreenLobbyWidget->SetIsJoining( false );

	MenuWidget->EnterSubMenu();
}

FReply FTrueFPSMainMenu::OnSplitScreenBackedOut()
{	
	SplitScreenLobbyWidget->Clear();
	SplitScreenBackedOut();
	return FReply::Handled();
}

FReply FTrueFPSMainMenu::OnSplitScreenPlay()
{
	switch ( MatchType )
	{
		case EMatchType::Custom:
		{
#if TRUEFPS_CONSOLE_UI
			if ( SplitScreenLobbyWidget->GetIsJoining() )
			{
#if 1
				// Until we can make split-screen menu support sub-menus, we need to do it this way
				if (GEngine && GEngine->GameViewport)
				{
					GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());
				}
				AddMenuToGameViewport();

				FSlateApplication::Get().SetKeyboardFocus(MenuWidget);	

				// Grab the map filter if there is one
				FString SelectedMapFilterName = TEXT("ANY");
				if (JoinMapOption.IsValid())
				{
					int32 FilterChoice = JoinMapOption->SelectedMultiChoice;
					if (FilterChoice != INDEX_NONE)
					{
						SelectedMapFilterName = JoinMapOption->MultiChoice[FilterChoice].ToString();
					}
				}


				MenuWidget->NextMenu = JoinServerItem->SubMenu;
				ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
				ServerListWidget->UpdateServerList();
				MenuWidget->EnterSubMenu();
#else
				SplitScreenLobbyWidget->NextMenu = JoinServerItem->SubMenu;
				ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
				ServerListWidget->UpdateServerList();
				SplitScreenLobbyWidget->EnterSubMenu();
#endif
			}
			else
#endif
			{
				if (GEngine && GEngine->GameViewport)
				{
					GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());
				}
				OnUIHostTeamDeathMatch();
			}
			break;
		}

		case EMatchType::Quick:
		{
			if (GEngine && GEngine->GameViewport)
			{
				GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());
			}
			BeginQuickMatchSearch();
			break;
		}
	}

	return FReply::Handled();
}

void FTrueFPSMainMenu::SplitScreenBackedOut()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());	
	}
	AddMenuToGameViewport();

	FSlateApplication::Get().SetKeyboardFocus(MenuWidget);	
}

void FTrueFPSMainMenu::HelperQuickMatchSearchingUICancel(bool bShouldRemoveSession)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if (bShouldRemoveSession && Sessions.IsValid())
	{
		if (PlayerOwner.IsValid() && PlayerOwner->GetPreferredUniqueNetId().IsValid())
		{
			UGameViewportClient* const GVC = GEngine->GameViewport;
			GVC->RemoveViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
			GVC->AddViewportWidgetContent(QuickMatchStoppingWidgetContainer.ToSharedRef());
			FSlateApplication::Get().SetKeyboardFocus(QuickMatchStoppingWidgetContainer);
			
			OnCancelMatchmakingCompleteDelegateHandle = Sessions->AddOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegate);
			Sessions->CancelMatchmaking(*PlayerOwner->GetPreferredUniqueNetId(), NAME_GameSession);
		}
	}
	else
	{
		UGameViewportClient* const GVC = GEngine->GameViewport;
		GVC->RemoveViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
		AddMenuToGameViewport();
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
	}
}

FReply FTrueFPSMainMenu::OnQuickMatchSearchingUICancel()
{
	HelperQuickMatchSearchingUICancel(true);
	bUsedInputToCancelQuickmatchSearch = true;
	bQuickmatchSearchRequestCanceled = true;
	return FReply::Handled();
}

FReply FTrueFPSMainMenu::OnQuickMatchFailureUICancel()
{
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(QuickMatchFailureWidgetContainer.ToSharedRef());
	}
	AddMenuToGameViewport();
	FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
	return FReply::Handled();
}

void FTrueFPSMainMenu::DisplayQuickmatchFailureUI()
{
	UGameViewportClient* const GVC = GEngine->GameViewport;
	RemoveMenuFromGameViewport();
	GVC->AddViewportWidgetContent(QuickMatchFailureWidgetContainer.ToSharedRef());
	FSlateApplication::Get().SetKeyboardFocus(QuickMatchFailureWidget);
}

void FTrueFPSMainMenu::DisplayQuickmatchSearchingUI()
{
	UGameViewportClient* const GVC = GEngine->GameViewport;
	RemoveMenuFromGameViewport();
	GVC->AddViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
	FSlateApplication::Get().SetKeyboardFocus(QuickMatchSearchingWidget);
	bAnimateQuickmatchSearchingUI = true;
}

void FTrueFPSMainMenu::OnMatchmakingComplete(FName SessionName, const FOnlineError& ErrorDetails, const FSessionMatchmakingResults& Results)
{
	const bool bWasSuccessful = ErrorDetails.WasSuccessful();
	IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: Couldn't find session interface."));
		return;
	}

	if (bQuickmatchSearchRequestCanceled && bUsedInputToCancelQuickmatchSearch)
	{
		bQuickmatchSearchRequestCanceled = false;
		// Clean up the session in case we get this event after canceling
		if (bWasSuccessful)
		{
			if (PlayerOwner.IsValid() && PlayerOwner->GetPreferredUniqueNetId().IsValid())
			{
				SessionInterface->DestroySession(NAME_GameSession);
			}
		}
		return;
	}

	if (bAnimateQuickmatchSearchingUI)
	{
		bAnimateQuickmatchSearchingUI = false;
		HelperQuickMatchSearchingUICancel(false);
		bUsedInputToCancelQuickmatchSearch = false;
	}
	else
	{
		return;
	}

	if (!bWasSuccessful)
	{
		UE_LOG(LogOnline, Warning, TEXT("Matchmaking was unsuccessful."));
		DisplayQuickmatchFailureUI();
		return;
	}

	UE_LOG(LogOnline, Log, TEXT("Matchmaking successful! Session name is %s."), *SessionName.ToString());

	if (GetPlayerOwner() == nullptr)
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: No owner."));
		return;
	}

	FNamedOnlineSession* MatchmadeSession = SessionInterface->GetNamedSession(SessionName);

	if (!MatchmadeSession)
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: No session."));
		return;
	}

	if(!MatchmadeSession->OwningUserId.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("OnMatchmakingComplete: No session owner/host."));
		return;
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(QuickMatchSearchingWidgetContainer.ToSharedRef());
	}
	bAnimateQuickmatchSearchingUI = false;

	UE_LOG(LogOnline, Log, TEXT("OnMatchmakingComplete: Session host is %d."), *MatchmadeSession->OwningUserId->ToString());

	if (ensure(GameInstance.IsValid()))
	{
		MenuWidget->LockControls(true);

		IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetTickableGameObjectWorld());
		if (Subsystem != nullptr && Subsystem->IsLocalPlayer(*MatchmadeSession->OwningUserId))
		{
			// This console is the host, start the map.
			GameInstance->BeginHostingQuickMatch();
		}
		else
		{
			// We are the client, join the host.
			GameInstance->TravelToSession(SessionName);
		}
	}
}

FTrueFPSMainMenu::EMap FTrueFPSMainMenu::GetSelectedMap() const
{
	if (GameInstance.IsValid() && GameInstance->GetOnlineMode() != EOnlineMode::Offline && HostOnlineMapOption.IsValid())
	{
		return (EMap)HostOnlineMapOption->SelectedMultiChoice;
	}
	else if (HostOfflineMapOption.IsValid())
	{
		return (EMap)HostOfflineMapOption->SelectedMultiChoice;
	}

	return EMap::ESancturary;	// Need to return something (we can hit this path in cooking)
}

void FTrueFPSMainMenu::CloseSubMenu()
{
	MenuWidget->MenuGoBack(true);
}

void FTrueFPSMainMenu::OnMenuGoBack(MenuPtr Menu)
{
	// if we are going back from options menu
	if (TrueFPSOptions->OptionsItem->SubMenu == Menu)
	{
		TrueFPSOptions->RevertChanges();
	}

	// if we've backed all the way out we need to make sure online is false.
	if (MenuWidget->GetMenuLevel() == 1)
	{
		GameInstance->SetOnlineMode(EOnlineMode::Offline);
	}
}

void FTrueFPSMainMenu::BotCountOptionChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	BotsCountOpt = MultiOptionIndex;

	if(GetPersistentUser())
	{
		GetPersistentUser()->SetBotsCount(BotsCountOpt);
	}
}

void FTrueFPSMainMenu::LanMatchChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	if (HostLANItem.IsValid())
	{
		HostLANItem->SelectedMultiChoice = MultiOptionIndex;
	}

	check(JoinLANItem.IsValid());
	JoinLANItem->SelectedMultiChoice = MultiOptionIndex;
	bIsLanMatch = MultiOptionIndex > 0;
	UTrueFPSGameUserSettings* UserSettings = CastChecked<UTrueFPSGameUserSettings>(GEngine->GetGameUserSettings());
	UserSettings->SetLanMatch(bIsLanMatch);

	EOnlineMode NewOnlineMode = bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online;
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(NewOnlineMode);
	}
}

void FTrueFPSMainMenu::DedicatedServerChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	check(DedicatedItem.IsValid());
	DedicatedItem->SelectedMultiChoice = MultiOptionIndex;
	bIsDedicatedServer = MultiOptionIndex > 0;
	UTrueFPSGameUserSettings* UserSettings = CastChecked<UTrueFPSGameUserSettings>(GEngine->GetGameUserSettings());
	UserSettings->SetDedicatedServer(bIsDedicatedServer);
}

void FTrueFPSMainMenu::RecordDemoChanged(TSharedPtr<FTrueFPSMenuItem> MenuItem, int32 MultiOptionIndex)
{
	if (RecordDemoItem.IsValid())
	{
		RecordDemoItem->SelectedMultiChoice = MultiOptionIndex;
	}

	bIsRecordingDemo = MultiOptionIndex > 0;

	if(GetPersistentUser())
	{
		GetPersistentUser()->SetIsRecordingDemos(bIsRecordingDemo);
		GetPersistentUser()->SaveIfDirty();
	}
}

void FTrueFPSMainMenu::OnUIHostFreeForAll()
{
#if WITH_EDITOR
	if (GIsEditor == true)
	{
		return;
	}
#endif
	if (!IsMapReady())
	{
		return;
	}

#if !TRUEFPS_CONSOLE_UI
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online);
	}
#endif

	MenuWidget->LockControls(true);
	MenuWidget->HideMenu();

	UWorld* World = GetTickableGameObjectWorld();
	const int32 ControllerId = GetPlayerOwnerControllerId();

	if (World && ControllerId != -1)
	{
		const FTrueFPSMenuSoundsStyle& MenuSounds = FTrueFPSStyle::Get().GetWidgetStyle<FTrueFPSMenuSoundsStyle>("DefaultTrueFPSMenuSoundsStyle");
		MenuHelper::PlaySoundAndCall(World, MenuSounds.StartGameSound, ControllerId, this, &FTrueFPSMainMenu::HostFreeForAll);
	}
}

void FTrueFPSMainMenu::OnUIHostTeamDeathMatch()
{
#if WITH_EDITOR
	if (GIsEditor == true)
	{
		return;
	}
#endif
	if (!IsMapReady())
	{
		return;
	}

#if !TRUEFPS_CONSOLE_UI
	if (GameInstance.IsValid())
	{
		GameInstance->SetOnlineMode(bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online);
	}
#endif

	MenuWidget->LockControls(true);
	MenuWidget->HideMenu();

	if (GetTickableGameObjectWorld() && GetPlayerOwnerControllerId() != -1)
	{
		const FTrueFPSMenuSoundsStyle& MenuSounds = FTrueFPSStyle::Get().GetWidgetStyle<FTrueFPSMenuSoundsStyle>("DefaultTrueFPSMenuSoundsStyle");
			MenuHelper::PlaySoundAndCall(GetTickableGameObjectWorld(), MenuSounds.StartGameSound, GetPlayerOwnerControllerId(), this, &FTrueFPSMainMenu::HostTeamDeathMatch);
	}
}

void FTrueFPSMainMenu::HostGame(const FString& GameType)
{	
	if (ensure(GameInstance.IsValid()) && GetPlayerOwner() != nullptr)
	{
		FString const StartURL = FString::Printf(TEXT("/Game/Maps/%s?game=%s%s%s?%s=%d%s"), *GetMapName(), *GameType, GameInstance->GetOnlineMode() != EOnlineMode::Offline ? TEXT("?listen") : TEXT(""), GameInstance->GetOnlineMode() == EOnlineMode::LAN ? TEXT("?bIsLanMatch") : TEXT(""), *ATrueFPSGameMode::GetBotsCountOptionName(), BotsCountOpt, bIsRecordingDemo ? TEXT("?DemoRec") : TEXT("") );

		// Game instance will handle success, failure and dialogs
		GameInstance->HostGame(GetPlayerOwner(), GameType, StartURL);
	}
}

void FTrueFPSMainMenu::HostFreeForAll()
{
	HostGame(TEXT("FFA"));
}

void FTrueFPSMainMenu::HostTeamDeathMatch()
{
	HostGame(TEXT("TDM"));
}

FReply FTrueFPSMainMenu::OnConfirm()
{
	if (GEngine && GEngine->GameViewport)
	{
		UTrueFPSGameViewportClient * TrueFPSViewport = Cast<UTrueFPSGameViewportClient>(GEngine->GameViewport);

		if (TrueFPSViewport)
		{
			// Hide the previous dialog
			TrueFPSViewport->HideDialog();
		}
	}

	return FReply::Handled();
}

bool FTrueFPSMainMenu::ValidatePlayerForOnlinePlay(ULocalPlayer* LocalPlayer)
{
	if (!ensure(GameInstance.IsValid()))
	{
		return false;
	}

	return GameInstance->ValidatePlayerForOnlinePlay(LocalPlayer);
}

bool FTrueFPSMainMenu::ValidatePlayerIsSignedIn(ULocalPlayer* LocalPlayer)
{
	if (!ensure(GameInstance.IsValid()))
	{
		return false;
	}

	return GameInstance->ValidatePlayerIsSignedIn(LocalPlayer);
}

void FTrueFPSMainMenu::OnJoinServerLoginRequired()
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		int32 ControllerId = GetPlayerOwner()->GetControllerId();
	
		if (bIsLanMatch)
		{
			Identity->Logout(ControllerId);
			OnUserCanPlayOnlineJoin(*GetPlayerOwner()->GetCachedUniqueNetId(), EUserPrivileges::CanPlayOnline, (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures);
		}
		else
		{
			OnLoginCompleteJoinDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(ControllerId, FOnLoginCompleteDelegate::CreateRaw(this, &FTrueFPSMainMenu::OnLoginCompleteJoin));
			Identity->Login(ControllerId, FOnlineAccountCredentials());
		}
	}
}

void FTrueFPSMainMenu::OnLoginCompleteJoin(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr Identity = Online::GetIdentityInterface(GetTickableGameObjectWorld());
	if (Identity.IsValid())
	{
		Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, OnLoginCompleteJoinDelegateHandle);
	}

	OnJoinServer();
}

void FTrueFPSMainMenu::OnJoinSelected()
{
#if TRUEFPS_CONSOLE_UI
	if (!ValidatePlayerIsSignedIn(GetPlayerOwner()))
	{
		return;
	}
#endif

	MenuWidget->EnterSubMenu();
}

void FTrueFPSMainMenu::OnJoinServer()
{
#if TRUEFPS_CONSOLE_UI
	if ( !ValidatePlayerForOnlinePlay(GetPlayerOwner()) )
	{
		return;
	}
#endif

	StartOnlinePrivilegeTask(IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateSP(this, &FTrueFPSMainMenu::OnUserCanPlayOnlineJoin));
}

void FTrueFPSMainMenu::OnUserCanPlayOnlineJoin(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, uint32 PrivilegeResults)
{
	CleanupOnlinePrivilegeTask();
	MenuWidget->LockControls(false);

	if (PrivilegeResults == (uint32)IOnlineIdentity::EPrivilegeResults::NoFailures)
	{

		//make sure to switch to custom match type so we don't instead use Quick type
		MatchType = EMatchType::Custom;

		if (GameInstance.IsValid())
		{
			GameInstance->SetOnlineMode(bIsLanMatch ? EOnlineMode::LAN : EOnlineMode::Online);
		}

		MatchType = EMatchType::Custom;
		// Grab the map filter if there is one
		FString SelectedMapFilterName("Any");
		if( JoinMapOption.IsValid())
		{
			int32 FilterChoice = JoinMapOption->SelectedMultiChoice;
			if( FilterChoice != INDEX_NONE )
			{
				SelectedMapFilterName = JoinMapOption->MultiChoice[FilterChoice].ToString();
			}
		}

#if TRUEFPS_CONSOLE_UI
		UGameViewportClient* const GVC = GEngine->GameViewport;
#if MAX_LOCAL_PLAYERS == 1
		// Show server menu (skip splitscreen)
		AddMenuToGameViewport();
		FSlateApplication::Get().SetKeyboardFocus(MenuWidget);

		MenuWidget->NextMenu = JoinServerItem->SubMenu;
		ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
		ServerListWidget->UpdateServerList();
		MenuWidget->EnterSubMenu();
#else
		// Show splitscreen menu
		RemoveMenuFromGameViewport();	
		GVC->AddViewportWidgetContent(SplitScreenLobbyWidgetContainer.ToSharedRef());

		SplitScreenLobbyWidget->Clear();
		FSlateApplication::Get().SetKeyboardFocus(SplitScreenLobbyWidget);

		SplitScreenLobbyWidget->SetIsJoining( true );
#endif
#else
		MenuWidget->NextMenu = JoinServerItem->SubMenu;
		//FString SelectedMapFilterName = JoinMapOption->MultiChoice[JoinMapOption->SelectedMultiChoice].ToString();

		ServerListWidget->BeginServerSearch(bIsLanMatch, bIsDedicatedServer, SelectedMapFilterName);
		ServerListWidget->UpdateServerList();
		MenuWidget->EnterSubMenu();
#endif
	}
	else if (GameInstance.IsValid())
	{
		GameInstance->DisplayOnlinePrivilegeFailureDialogs(UserId, Privilege, PrivilegeResults);
	}
}

void FTrueFPSMainMenu::OnShowLeaderboard()
{
	MenuWidget->NextMenu = LeaderboardItem->SubMenu;
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
	LeaderboardWidget->ReadStatsLoginRequired();
#else
	LeaderboardWidget->ReadStats();
#endif
	MenuWidget->EnterSubMenu();
}

void FTrueFPSMainMenu::OnShowOnlineStore()
{
	MenuWidget->NextMenu = OnlineStoreItem->SubMenu;
#if LOGIN_REQUIRED_FOR_ONLINE_PLAY
	UE_LOG(LogOnline, Warning, TEXT("You need to be logged in before using the store"));
#endif
	OnlineStoreWidget->BeginGettingOffers();
	MenuWidget->EnterSubMenu();
}

void FTrueFPSMainMenu::OnShowDemoBrowser()
{
	MenuWidget->NextMenu = DemoBrowserItem->SubMenu;
	DemoListWidget->BuildDemoList();
	MenuWidget->EnterSubMenu();
}

void FTrueFPSMainMenu::OnUIQuit()
{
	bIsQuitting = true;
	LockAndHideMenu();

	const FTrueFPSMenuSoundsStyle& MenuSounds = FTrueFPSStyle::Get().GetWidgetStyle<FTrueFPSMenuSoundsStyle>("DefaultTrueFPSMenuSoundsStyle");

	if (GetTickableGameObjectWorld() != nullptr && GetPlayerOwnerControllerId() != -1)
	{
		FSlateApplication::Get().PlaySound(MenuSounds.ExitGameSound, GetPlayerOwnerControllerId());
		MenuHelper::PlaySoundAndCall(GetTickableGameObjectWorld(), MenuSounds.ExitGameSound, GetPlayerOwnerControllerId(), this, &FTrueFPSMainMenu::Quit);
	}
}

void FTrueFPSMainMenu::Quit()
{
	if (ensure(GameInstance.IsValid()))
	{
		UGameViewportClient* const Viewport = GameInstance->GetGameViewportClient();
		if (ensure(Viewport)) 
		{
			Viewport->ConsoleCommand("quit");
		}
	}
}

void FTrueFPSMainMenu::LockAndHideMenu()
{
	MenuWidget->LockControls(true);
	MenuWidget->HideMenu();
}

void FTrueFPSMainMenu::DisplayLoadingScreen()
{
	ITrueFPSSystemLoadingScreenModule* LoadingScreenModule = FModuleManager::LoadModulePtr<ITrueFPSSystemLoadingScreenModule>("TrueFPSGameLoadingScreen");
	if( LoadingScreenModule != nullptr )
	{
		LoadingScreenModule->StartInGameLoadingScreen();
	}
}

bool FTrueFPSMainMenu::IsMapReady() const
{
	bool bReady = true;
	IPlatformChunkInstall* ChunkInstaller = FPlatformMisc::GetPlatformChunkInstall();
	if (ChunkInstaller)
	{
		EMap SelectedMap = GetSelectedMap();
		// should use the AssetRegistry as soon as maps are added to the AssetRegistry
		int32 MapChunk = ChunkMapping[(int)SelectedMap];
		EChunkLocation::Type ChunkLocation = ChunkInstaller->GetPakchunkLocation(MapChunk);
		if (ChunkLocation == EChunkLocation::NotAvailable)
		{			
			bReady = false;
		}
	}
	return bReady;
}

UTrueFPSPersistentUser* FTrueFPSMainMenu::GetPersistentUser() const
{
	UTrueFPSLocalPlayer* const TrueFPSLocalPlayer = Cast<UTrueFPSLocalPlayer>(GetPlayerOwner());
	return TrueFPSLocalPlayer ? TrueFPSLocalPlayer->GetPersistentUser() : nullptr;
}

UWorld* FTrueFPSMainMenu::GetTickableGameObjectWorld() const
{
	ULocalPlayer* LocalPlayerOwner = GetPlayerOwner();
	return (LocalPlayerOwner ? LocalPlayerOwner->GetWorld() : nullptr);
}

ULocalPlayer* FTrueFPSMainMenu::GetPlayerOwner() const
{
	return PlayerOwner.Get();
}

int32 FTrueFPSMainMenu::GetPlayerOwnerControllerId() const
{
	return ( PlayerOwner.IsValid() ) ? PlayerOwner->GetControllerId() : -1;
}

FString FTrueFPSMainMenu::GetMapName() const
{
	return MapNames[(int)GetSelectedMap()];
}

void FTrueFPSMainMenu::OnCancelMatchmakingComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetTickableGameObjectWorld());
	if (Sessions.IsValid())
	{
		Sessions->ClearOnCancelMatchmakingCompleteDelegate_Handle(OnCancelMatchmakingCompleteDelegateHandle);
	}

	bAnimateQuickmatchSearchingUI = false;
	UGameViewportClient* const GVC = GEngine->GameViewport;
	GVC->RemoveViewportWidgetContent(QuickMatchStoppingWidgetContainer.ToSharedRef());
	AddMenuToGameViewport();
	FSlateApplication::Get().SetKeyboardFocus(MenuWidget);
}

#undef LOCTEXT_NAMESPACE
