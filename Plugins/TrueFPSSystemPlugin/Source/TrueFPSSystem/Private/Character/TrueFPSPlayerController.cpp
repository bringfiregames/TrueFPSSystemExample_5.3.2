// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TrueFPSPlayerController.h"

#include "Character/TrueFPSCharacter.h"
#include "Character/TrueFPSPlayerCameraManager.h"
#include "Weapons/TrueFPSWeaponBase.h"
#include "Online.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystem.h"
#include "TrueFPSGameInstance.h"
#include "TrueFPSGameViewportClient.h"
#include "TrueFPSLeaderboards.h"
#include "Character/TrueFPSLocalPlayer.h"
#include "Character/TrueFPSPersistentUser.h"
#include "Net/UnrealNetwork.h"
#include "Online/TrueFPSGameMode.h"
#include "Online/TrueFPSPlayerState.h"
#include "Sound/LocalPlayerSoundNode.h"
#include "UI/TrueFPSHUD.h"
#include "UI/Menu/TrueFPSFriends.h"
#include "UI/Menu/TrueFPSIngameMenu.h"
#include "UI/Style/TrueFPSStyle.h"

#define  ACH_FRAG_SOMEONE	TEXT("ACH_FRAG_SOMEONE")
#define  ACH_SOME_KILLS		TEXT("ACH_SOME_KILLS")
#define  ACH_LOTS_KILLS		TEXT("ACH_LOTS_KILLS")
#define  ACH_FINISH_MATCH	TEXT("ACH_FINISH_MATCH")
#define  ACH_LOTS_MATCHES	TEXT("ACH_LOTS_MATCHES")
#define  ACH_FIRST_WIN		TEXT("ACH_FIRST_WIN")
#define  ACH_LOTS_WIN		TEXT("ACH_LOTS_WIN")
#define  ACH_MANY_WIN		TEXT("ACH_MANY_WIN")
#define  ACH_SHOOT_BULLETS	TEXT("ACH_SHOOT_BULLETS")
#define  ACH_SHOOT_ROCKETS	TEXT("ACH_SHOOT_ROCKETS")
#define  ACH_GOOD_SCORE		TEXT("ACH_GOOD_SCORE")
#define  ACH_GREAT_SCORE	TEXT("ACH_GREAT_SCORE")
#define  ACH_PLAY_SANCTUARY	TEXT("ACH_PLAY_SANCTUARY")
#define  ACH_PLAY_HIGHRISE	TEXT("ACH_PLAY_HIGHRISE")

static const int32 SomeKillsCount = 10;
static const int32 LotsKillsCount = 20;
static const int32 LotsMatchesCount = 5;
static const int32 LotsWinsCount = 3;
static const int32 ManyWinsCount = 5;
static const int32 LotsBulletsCount = 100;
static const int32 LotsRocketsCount = 10;
static const int32 GoodScoreCount = 10;
static const int32 GreatScoreCount = 15;

#if !defined(TRACK_STATS_LOCALLY)
	#define TRACK_STATS_LOCALLY 1
#endif

ATrueFPSPlayerController::ATrueFPSPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerCameraManagerClass = ATrueFPSPlayerCameraManager::StaticClass();
	// CheatClass = UTrueFPSCheatManager::StaticClass();
	bAllowGameActions = true;
	bGameEndedFrame = false;
	LastDeathLocation = FVector::ZeroVector;

	ServerSayString = TEXT("Say");
	TrueFPSFriendUpdateTimer = 0.0f;
	bHasSentStartEvents = false;

	StatMatchesPlayed = 0;
	StatKills = 0;
	StatDeaths = 0;
	bHasQueriedPlatformStats = false;
	bHasQueriedPlatformAchievements = false;
	bHasInitializedInputComponent = false;
}

void ATrueFPSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if(!bHasInitializedInputComponent)
	{
		// UI input
		InputComponent->BindAction("InGameMenu", IE_Pressed, this, &ThisClass::OnToggleInGameMenu);
		InputComponent->BindAction("Scoreboard", IE_Pressed, this, &ThisClass::OnShowScoreboard);
		InputComponent->BindAction("Scoreboard", IE_Released, this, &ThisClass::OnHideScoreboard);
		InputComponent->BindAction("ConditionalCloseScoreboard", IE_Pressed, this, &ThisClass::OnConditionalCloseScoreboard);
		InputComponent->BindAction("ToggleScoreboard", IE_Pressed, this, &ThisClass::OnToggleScoreboard);

		// voice chat
		InputComponent->BindAction("PushToTalk", IE_Pressed, this, &APlayerController::StartTalking);
		InputComponent->BindAction("PushToTalk", IE_Released, this, &APlayerController::StopTalking);

		InputComponent->BindAction("ToggleChat", IE_Pressed, this, &ThisClass::ToggleChatWindow);

		bHasInitializedInputComponent = true;
	}
}

void ATrueFPSPlayerController::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME_CONDITION( ThisClass, bInfiniteAmmo, COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( ThisClass, bInfiniteClip, COND_OwnerOnly );

	DOREPLIFETIME(ThisClass, bHealthRegen);
}

void ATrueFPSPlayerController::SetInfiniteAmmo(bool bEnable)
{
	bInfiniteAmmo = bEnable;
}

void ATrueFPSPlayerController::SetInfiniteClip(bool bEnable)
{
	bInfiniteClip = bEnable;
}

void ATrueFPSPlayerController::SetHealthRegen(bool bEnable)
{
	bHealthRegen = bEnable;
}

void ATrueFPSPlayerController::SetGodMode(bool bEnable)
{
	bGodMode = bEnable;
}

void ATrueFPSPlayerController::SetIsVibrationEnabled(bool bEnable)
{
	bIsVibrationEnabled = bEnable;
}

bool ATrueFPSPlayerController::HasInfiniteAmmo() const
{
	return bInfiniteAmmo;
}

bool ATrueFPSPlayerController::HasInfiniteClip() const
{
	return bInfiniteClip;
}

bool ATrueFPSPlayerController::HasHealthRegen() const
{
	return bHealthRegen;
}

bool ATrueFPSPlayerController::HasGodMode() const
{
	return bGodMode;
}

bool ATrueFPSPlayerController::IsVibrationEnabled() const
{
	return bIsVibrationEnabled;
}

void ATrueFPSPlayerController::ClientSetSpectatorCamera_Implementation(FVector CameraLocation, FRotator CameraRotation)
{
	SetInitialLocationAndRotation(CameraLocation, CameraRotation);
	SetViewTarget(this);
}

void ATrueFPSPlayerController::ClientGameStarted_Implementation()
{
	bAllowGameActions = true;

	// Enable controls mode now the game has started
	SetIgnoreMoveInput(false);

	ATrueFPSHUD* TrueFPSHUD = GetTrueFPSHUD();
	if (TrueFPSHUD)
	{
		TrueFPSHUD->SetMatchState(ETrueFPSMatchState::Playing);
		TrueFPSHUD->ShowScoreboard(false);
	}
	bGameEndedFrame = false;
	
	// QueryAchievements(); @todo implement achievements
	// QueryStats();

	const UWorld* World = GetWorld();

	// Send round start event
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if(LocalPlayer != nullptr && World != nullptr && Events.IsValid())
	{
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			// Generate a new session id
			Events->SetPlayerSessionId(*UniqueId, FGuid::NewGuid());

			FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());

			// Fire session start event for all cases
			FOnlineEventParms Params;
			Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
			Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "MapName" ), FVariantData( MapName ) );
			
			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionStart"), Params);

			// Online matches require the MultiplayerRoundStart event as well
			UTrueFPSGameInstance* SGI = Cast<UTrueFPSGameInstance>(World->GetGameInstance());
			
			if (SGI && (SGI->GetOnlineMode() == EOnlineMode::Online))
			{
				FOnlineEventParms MultiplayerParams;
			
				// @todo: fill in with real values
				MultiplayerParams.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
				MultiplayerParams.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
				MultiplayerParams.Add( TEXT( "MatchTypeId" ), FVariantData( (int32)1 ) ); // @todo abstract the specific meaning of this value across platforms
				MultiplayerParams.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
				
				Events->TriggerEvent(*UniqueId, TEXT("MultiplayerRoundStart"), MultiplayerParams);
			}

			bHasSentStartEvents = true;
		}
	}
}

void ATrueFPSPlayerController::ClientStartOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	ATrueFPSPlayerState* TrueFPSPlayerState = Cast<ATrueFPSPlayerState>(PlayerState);
	if (TrueFPSPlayerState)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && (Sessions->GetNamedSession(TrueFPSPlayerState->SessionName) != nullptr))
			{
				UE_LOG(LogOnline, Log, TEXT("Starting session %s on client"), *TrueFPSPlayerState->SessionName.ToString() );
				Sessions->StartSession(TrueFPSPlayerState->SessionName);
			}
		}
	}
	else
	{
		// Keep retrying until player state is replicated
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ClientStartOnlineGame, this, &ThisClass::ClientStartOnlineGame_Implementation, 0.2f, false);
	}
}

void ATrueFPSPlayerController::ClientEndOnlineGame_Implementation()
{
	if (!IsPrimaryPlayer())
		return;

	ATrueFPSPlayerState* TrueFPSPlayerState = Cast<ATrueFPSPlayerState>(PlayerState);
	if (TrueFPSPlayerState)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && (Sessions->GetNamedSession(TrueFPSPlayerState->SessionName) != nullptr))
			{
				UE_LOG(LogOnline, Log, TEXT("Ending session %s on client"), *TrueFPSPlayerState->SessionName.ToString() );
				Sessions->EndSession(TrueFPSPlayerState->SessionName);
			}
		}
	}
}

void ATrueFPSPlayerController::ClientGameEnded_Implementation(AActor* EndGameFocus, bool bIsWinner)
{
	Super::ClientGameEnded_Implementation(EndGameFocus, bIsWinner);
	
	// Disable controls now the game has ended
	SetIgnoreMoveInput(true);

	bAllowGameActions = false;

	// Make sure that we still have valid view target
	SetViewTarget(GetPawn());

	ATrueFPSHUD* TrueFPSHUD = GetTrueFPSHUD();
	if (TrueFPSHUD)
	{
		TrueFPSHUD->SetMatchState(bIsWinner ? ETrueFPSMatchState::Won : ETrueFPSMatchState::Lost);
	}

	UpdateSaveFileOnGameEnd(bIsWinner);
	UpdateAchievementsOnGameEnd();
	UpdateLeaderboardsOnGameEnd();
	UpdateStatsOnGameEnd(bIsWinner);

	// Flag that the game has just ended (if it's ended due to host loss we want to wait for ClientReturnToMainMenu_Implementation first, incase we don't want to process)
	bGameEndedFrame = true;
}

void ATrueFPSPlayerController::SimulateInputKey(FKey Key, bool bPressed)
{
	InputKey(Key, bPressed ? IE_Pressed : IE_Released, 1, false);
}

void ATrueFPSPlayerController::ServerCheat_Implementation(const FString& Msg)
{
	if (CheatManager)
	{
		ClientMessage(ConsoleCommand(Msg));
	}
}

void ATrueFPSPlayerController::ClientTeamMessage_Implementation(APlayerState* SenderPlayerState, const FString& S,
	FName Type, float MsgLifeTime)
{
	ATrueFPSHUD* TrueFPSHUD = Cast<ATrueFPSHUD>(GetHUD());
	if (TrueFPSHUD)
	{
		if( Type == ServerSayString )
		{
			if( SenderPlayerState != PlayerState  )
			{
				TrueFPSHUD->AddChatLine(FText::FromString(S), false);
			}
		}
	}
}

void ATrueFPSPlayerController::ToggleChatWindow()
{
	ATrueFPSHUD* TrueFPSHUD = Cast<ATrueFPSHUD>(GetHUD());
	if (TrueFPSHUD)
	{
		TrueFPSHUD->ToggleChat();
	}
}

void ATrueFPSPlayerController::Say(const FString& Msg)
{
	ServerSay(Msg.Left(128));
}

void ATrueFPSPlayerController::ServerSay_Implementation(const FString& Msg)
{
	GetWorld()->GetAuthGameMode<ATrueFPSGameMode>()->Broadcast(this, Msg, ServerSayString);
}

void ATrueFPSPlayerController::ShowInGameMenu()
{
	ATrueFPSHUD* TrueFPSHUD = GetTrueFPSHUD();	
	if(TrueFPSIngameMenu.IsValid() && !TrueFPSIngameMenu->GetIsGameMenuUp() && TrueFPSHUD && (TrueFPSHUD->IsMatchOver() == false))
	{
		TrueFPSIngameMenu->ToggleGameMenu();
	}
}

void ATrueFPSPlayerController::OnConditionalCloseScoreboard()
{
	ATrueFPSHUD* TrueFPSHUD = GetTrueFPSHUD();
	if(TrueFPSHUD && ( TrueFPSHUD->IsMatchOver() == false ))
	{
		TrueFPSHUD->ConditionalCloseScoreboard();
	}
}

void ATrueFPSPlayerController::OnToggleScoreboard()
{
	ATrueFPSHUD* TrueFPSHUD = GetTrueFPSHUD();
	if(TrueFPSHUD && ( TrueFPSHUD->IsMatchOver() == false ))
	{
		TrueFPSHUD->ToggleScoreboard();
	}
}

void ATrueFPSPlayerController::OnShowScoreboard()
{
	ATrueFPSHUD* TrueFPSHUD = GetTrueFPSHUD();
	if(TrueFPSHUD)
	{
		TrueFPSHUD->ShowScoreboard(true);
	}
}

void ATrueFPSPlayerController::OnHideScoreboard()
{
	ATrueFPSHUD* TrueFPSHUD = GetTrueFPSHUD();
	// If have a valid match and the match is over - hide the scoreboard
	if( (TrueFPSHUD != nullptr ) && ( TrueFPSHUD->IsMatchOver() == false ) )
	{
		TrueFPSHUD->ShowScoreboard(false);
	}
}

bool ATrueFPSPlayerController::IsGameInputAllowed() const
{
	return bAllowGameActions && !bCinematicMode;
}

void ATrueFPSPlayerController::CleanupSessionOnReturnToMenu()
{
	const UWorld* World = GetWorld();
	UTrueFPSGameInstance * SGI = World != nullptr ? Cast<UTrueFPSGameInstance>( World->GetGameInstance() ) : nullptr;

	if ( ensure( SGI != nullptr ) )
	{
		SGI->CleanupSessionOnReturnToMenu();
	}
}

void ATrueFPSPlayerController::OnQueryAchievementsComplete(const FUniqueNetId& PlayerId, const bool bWasSuccessful)
{
	UE_LOG(LogOnline, Display, TEXT("ATrueFPSPlayerController::OnQueryAchievementsComplete(bWasSuccessful = %s)"), bWasSuccessful ? TEXT("TRUE") : TEXT("FALSE"));
}

void ATrueFPSPlayerController::OnLeaderboardReadComplete(bool bWasSuccessful)
{
	if (ReadObject.IsValid() && ReadObject->ReadState == EOnlineAsyncTaskState::Done && !bHasQueriedPlatformStats)
	{
		bHasQueriedPlatformStats = true;
		ClearLeaderboardDelegate();

		// We should only have one stat.
		if (bWasSuccessful && ReadObject->Rows.Num() == 1)
		{
			FOnlineStatsRow& RowData = ReadObject->Rows[0];
			if (const FVariantData* KillData = RowData.Columns.Find(LEADERBOARD_STAT_KILLS))
			{
				KillData->GetValue(StatKills);
			}

			if (const FVariantData* DeathData = RowData.Columns.Find(LEADERBOARD_STAT_DEATHS))
			{
				DeathData->GetValue(StatDeaths);
			}

			if (const FVariantData* MatchData = RowData.Columns.Find(LEADERBOARD_STAT_MATCHESPLAYED))
			{
				MatchData->GetValue(StatMatchesPlayed);
			}

			UE_LOG(LogOnline, Log, TEXT("Fetched player stat data. Kills %d Deaths %d Matches %d"), StatKills, StatDeaths, StatMatchesPlayed);
		}
	}
}

void ATrueFPSPlayerController::SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD,
	bool bAffectsMovement, bool bAffectsTurning)
{
	Super::SetCinematicMode(bInCinematicMode, bHidePlayer, bAffectsHUD, bAffectsMovement, bAffectsTurning);

	// If we have a pawn we need to determine if we should show/hide the weapon
	ATrueFPSCharacter* MyPawn = Cast<ATrueFPSCharacter>(GetPawn());
	ATrueFPSWeaponBase* MyWeapon = MyPawn ? MyPawn->GetWeapon() : nullptr;
	if (MyWeapon)
	{
		if (bInCinematicMode && bHidePlayer)
		{
			MyWeapon->SetActorHiddenInGame(true);
		}
		else if (!bCinematicMode)
		{
			MyWeapon->SetActorHiddenInGame(false);
		}
	}
}

bool ATrueFPSPlayerController::IsMoveInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsMoveInputIgnored();
	}
}

bool ATrueFPSPlayerController::IsLookInputIgnored() const
{
	if (IsInState(NAME_Spectating))
	{
		return false;
	}
	else
	{
		return Super::IsLookInputIgnored();
	}
}

void ATrueFPSPlayerController::InitInputSystem()
{
	Super::InitInputSystem();

	UTrueFPSPersistentUser* PersistentUser = GetPersistentUser();
	if(PersistentUser)
	{
		PersistentUser->TellInputAboutKeybindings();
	}
}

bool ATrueFPSPlayerController::SetPause(bool bPause, FCanUnpause CanUnpauseDelegate)
{
	const bool Result = APlayerController::SetPause(bPause, CanUnpauseDelegate);

	// Update rich presence.
	const UWorld* World = GetWorld();
	const IOnlinePresencePtr PresenceInterface = Online::GetPresenceInterface(World);
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	FUniqueNetIdRepl UserId = LocalPlayer ? LocalPlayer->GetCachedUniqueNetId() : FUniqueNetIdRepl();

	// Don't send pause events while online since the game doesn't actually pause
	if(GetNetMode() == NM_Standalone && Events.IsValid() && PlayerState->GetUniqueId().IsValid())
	{
		FOnlineEventParms Params;
		Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
		Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
		if(Result && bPause)
		{
			Events->TriggerEvent(*PlayerState->GetUniqueId(), TEXT("PlayerSessionPause"), Params);
		}
		else
		{
			Events->TriggerEvent(*PlayerState->GetUniqueId(), TEXT("PlayerSessionResume"), Params);
		}
	}

	return Result;
}

FVector ATrueFPSPlayerController::GetFocalLocation() const
{
	const ATrueFPSCharacter* TrueFPSCharacter = Cast<ATrueFPSCharacter>(GetPawn());

	// On death we want to use the player's death cam location rather than the location of where the pawn is at the moment
	// This guarantees that the clients see their death cam correctly, as their pawns have delayed destruction.
	if (TrueFPSCharacter && TrueFPSCharacter->GetIsDying())
	{
		return GetSpawnLocation();
	}

	return Super::GetFocalLocation();
}

void ATrueFPSPlayerController::QueryAchievements()
{
	if (bHasQueriedPlatformAchievements)
	{
		return;
	}
	bHasQueriedPlatformAchievements = true;
	// precache achievements
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetControllerId() != -1)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if(OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());

				if (UserId.IsValid())
				{
					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();

					if (Achievements.IsValid())
					{
						Achievements->QueryAchievements( *UserId.Get(), FOnQueryAchievementsCompleteDelegate::CreateUObject( this, &ATrueFPSPlayerController::OnQueryAchievementsComplete ));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot read achievements."));
	}
}

void ATrueFPSPlayerController::QueryStats()
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetControllerId() != -1)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());

				if (UserId.IsValid())
				{
					IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
					if (Leaderboards.IsValid() && !bHasQueriedPlatformStats)
					{
						TArray<TSharedRef<const FUniqueNetId>> QueryPlayers;
						QueryPlayers.Add(UserId.ToSharedRef());

						LeaderboardReadCompleteDelegateHandle = Leaderboards->OnLeaderboardReadCompleteDelegates.AddUObject(this, &ATrueFPSPlayerController::OnLeaderboardReadComplete);
						ReadObject = MakeShareable(new FTrueFPSAllTimeMatchResultsRead());
						FOnlineLeaderboardReadRef ReadObjectRef = ReadObject.ToSharedRef();
						if (Leaderboards->ReadLeaderboards(QueryPlayers, ReadObjectRef))
						{
							UE_LOG(LogOnline, Log, TEXT("Started process to fetch stats for current user."));
						}
						else
						{
							UE_LOG(LogOnline, Warning, TEXT("Could not start leaderboard fetch process. This will affect stat writes for this session."));
						}
					}
				}
			}
		}
	}
}

void ATrueFPSPlayerController::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer( InPlayer );

	if (ULocalPlayer* const LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		//Build menu only after game is initialized
		TrueFPSIngameMenu = MakeShareable(new FTrueFPSIngameMenu());
		TrueFPSIngameMenu->Construct(Cast<ULocalPlayer>(Player));

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}
}

void ATrueFPSPlayerController::PreClientTravel(const FString& PendingURL, ETravelType TravelType,
	bool bIsSeamlessTravel)
{
	Super::PreClientTravel( PendingURL, TravelType, bIsSeamlessTravel );

	if (const UWorld* World = GetWorld())
	{
		UTrueFPSGameViewportClient* TrueFPSGameViewportClient = Cast<UTrueFPSGameViewportClient>( World->GetGameViewport() );

		if ( TrueFPSGameViewportClient != nullptr )
		{
			TrueFPSGameViewportClient->ShowLoadingScreen();
		}
		
		ATrueFPSHUD* TrueFPSHUD = Cast<ATrueFPSHUD>(GetHUD());
		if (TrueFPSHUD != nullptr)
		{
			// Passing true to bFocus here ensures that focus is returned to the game viewport.
			TrueFPSHUD->ShowScoreboard(false, true);
		}
	}
}

bool ATrueFPSPlayerController::FindDeathCameraSpot(FVector& CameraLocation, FRotator& CameraRotation)
{
	const FVector PawnLocation = GetPawn()->GetActorLocation();
	FRotator ViewDir = GetControlRotation();
	ViewDir.Pitch = -45.0f;

	const float YawOffsets[] = { 0.0f, -180.0f, 90.0f, -90.0f, 45.0f, -45.0f, 135.0f, -135.0f };
	const float CameraOffset = 600.0f;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(DeathCamera), true, GetPawn());

	FHitResult HitResult;
	for (int32 i = 0; i < UE_ARRAY_COUNT(YawOffsets); i++)
	{
		FRotator CameraDir = ViewDir;
		CameraDir.Yaw += YawOffsets[i];
		CameraDir.Normalize();

		const FVector TestLocation = PawnLocation - CameraDir.Vector() * CameraOffset;
		
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitResult, PawnLocation, TestLocation, ECC_Camera, TraceParams);

		if (!bBlocked)
		{
			CameraLocation = TestLocation;
			CameraRotation = CameraDir;
			return true;
		}
	}

	return false;
}

void ATrueFPSPlayerController::BeginDestroy()
{
	Super::BeginDestroy();
	ClearLeaderboardDelegate();

	// clear any online subsystem references
	TrueFPSIngameMenu = nullptr;

	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			ULocalPlayerSoundNode::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}
}

void ATrueFPSPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	FTrueFPSStyle::Initialize();
	TrueFPSFriendUpdateTimer = 0;
}

void ATrueFPSPlayerController::ClearLeaderboardDelegate()
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
		if (Leaderboards.IsValid())
		{
			Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(LeaderboardReadCompleteDelegateHandle);
		}
	}
}

void ATrueFPSPlayerController::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (IsGameMenuVisible())
	{
		if (TrueFPSFriendUpdateTimer > 0)
		{
			TrueFPSFriendUpdateTimer -= DeltaTime;
		}
		else
		{
			TSharedPtr<class FTrueFPSFriends> TrueFPSFriends = TrueFPSIngameMenu->GetTrueFPSFriends();
			ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
			if (TrueFPSFriends.IsValid() && LocalPlayer && LocalPlayer->GetControllerId() >= 0)
			{
				TrueFPSFriends->UpdateFriends(LocalPlayer->GetControllerId());
			}
			
			// Make sure the time between calls is long enough that we won't trigger (0x80552C81) and not exceed the web api rate limit
			// That value is currently 75 requests / 15 minutes.
			TrueFPSFriendUpdateTimer = 15;
		}
	}

	// Is this the first frame after the game has ended
	if(bGameEndedFrame)
	{
		bGameEndedFrame = false;

		// ONLY PUT CODE HERE WHICH YOU DON'T WANT TO BE DONE DUE TO HOST LOSS

		// Do we need to show the end of round scoreboard?
		if (IsPrimaryPlayer())
		{
			ATrueFPSHUD* TrueFPSHUD = GetTrueFPSHUD();
			if (TrueFPSHUD)
			{
				TrueFPSHUD->ShowScoreboard(true, true);
			}
		}
	}

	const bool bLocallyControlled = IsLocalController();
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
		ULocalPlayerSoundNode::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});
}

void ATrueFPSPlayerController::FailedToSpawnPawn()
{
	if(StateName == NAME_Inactive)
	{
		BeginInactiveState();
	}
	Super::FailedToSpawnPawn();
}

void ATrueFPSPlayerController::PawnPendingDestroy(APawn* P)
{
	LastDeathLocation = P->GetActorLocation();
	FVector CameraLocation = LastDeathLocation + FVector(0, 0, 300.0f);
	FRotator CameraRotation(-90.0f, 0.0f, 0.0f);
	FindDeathCameraSpot(CameraLocation, CameraRotation);

	Super::PawnPendingDestroy(P);

	ClientSetSpectatorCamera(CameraLocation, CameraRotation);
}

void ATrueFPSPlayerController::UnFreeze()
{
	ServerRestartPlayer();
}

void ATrueFPSPlayerController::GameHasEnded(AActor* EndGameFocus, bool bIsWinner)
{
	Super::GameHasEnded(EndGameFocus, bIsWinner);
}

void ATrueFPSPlayerController::ClientReturnToMainMenu_Implementation(const FString& ReturnReason)
{
	const UWorld* World = GetWorld();
	UTrueFPSGameInstance* SGI = World != nullptr ? Cast<UTrueFPSGameInstance>(World->GetGameInstance()) : nullptr;

	if ( !ensure( SGI != nullptr ) )
	{
		return;
	}

	if ( GetNetMode() == NM_Client )
	{
		const FText ReturnReason	= NSLOCTEXT( "NetworkErrors", "HostQuit", "The host has quit the match." );
		const FText OKButton		= NSLOCTEXT( "DialogButtons", "OKAY", "OK" );

		SGI->ShowMessageThenGotoState( ReturnReason, OKButton, FText::GetEmpty(), TrueFPSGameInstanceState::MainMenu );
	}
	else
	{
		SGI->GotoState(TrueFPSGameInstanceState::MainMenu);
	}

	// Clear the flag so we don't do normal end of round stuff next
	bGameEndedFrame = false;
}

void ATrueFPSPlayerController::Suicide()
{
	if ( IsInState(NAME_Playing) )
	{
		ServerSuicide();
	}
}

void ATrueFPSPlayerController::ServerSuicide_Implementation()
{
	if ( (GetPawn() != nullptr) && ((GetWorld()->TimeSeconds - GetPawn()->CreationTime > 1) || (GetNetMode() == NM_Standalone)) )
	{
		ATrueFPSCharacter* MyPawn = Cast<ATrueFPSCharacter>(GetPawn());
		if (MyPawn)
		{
			MyPawn->Suicide();
		}
	}
}

void ATrueFPSPlayerController::ClientSendRoundEndEvent_Implementation(bool bIsWinner, int32 ExpendedTimeInSeconds)
{
	const UWorld* World = GetWorld();
	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

	if(bHasSentStartEvents && LocalPlayer != nullptr && World != nullptr && Events.IsValid())
	{	
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetPreferredUniqueNetId();

		if (UniqueId.IsValid())
		{
			FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());
			ATrueFPSPlayerState* TrueFPSPlayerState = Cast<ATrueFPSPlayerState>(PlayerState);
			int32 PlayerScore = TrueFPSPlayerState ? TrueFPSPlayerState->GetScore() : 0;
			int32 PlayerDeaths = TrueFPSPlayerState ? TrueFPSPlayerState->GetDeaths() : 0;
			int32 PlayerKills = TrueFPSPlayerState ? TrueFPSPlayerState->GetKills() : 0;
			
			// Fire session end event for all cases
			FOnlineEventParms Params;
			Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
			Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "ExitStatusId" ), FVariantData( (int32)0 ) ); // unused
			Params.Add( TEXT( "PlayerScore" ), FVariantData( (int32)PlayerScore ) );
			Params.Add( TEXT( "PlayerWon" ), FVariantData( (bool)bIsWinner ) );
			Params.Add( TEXT( "MapName" ), FVariantData( MapName ) );
			Params.Add( TEXT( "MapNameString" ), FVariantData( MapName ) ); // @todo workaround for a bug in backend service, remove when fixed
			
			Events->TriggerEvent(*UniqueId, TEXT("PlayerSessionEnd"), Params);

			// Update all time results
			FOnlineEventParms AllTimeMatchParams;
			AllTimeMatchParams.Add(TEXT("TrueFPSAllTimeMatchResultsScore"), FVariantData((uint64)PlayerScore));
			AllTimeMatchParams.Add(TEXT("TrueFPSAllTimeMatchResultsDeaths"), FVariantData((int32)PlayerDeaths));
			AllTimeMatchParams.Add(TEXT("TrueFPSAllTimeMatchResultsFrags"), FVariantData((int32)PlayerKills));
			AllTimeMatchParams.Add(TEXT("TrueFPSAllTimeMatchResultsMatchesPlayed"), FVariantData((int32)1));

			Events->TriggerEvent(*UniqueId, TEXT("TrueFPSAllTimeMatchResults"), AllTimeMatchParams);

			// Online matches require the MultiplayerRoundEnd event as well
			UTrueFPSGameInstance* SGI = Cast<UTrueFPSGameInstance>(World->GetGameInstance());
			if (SGI && (SGI->GetOnlineMode() == EOnlineMode::Online))
			{
				FOnlineEventParms MultiplayerParams;
				MultiplayerParams.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
				MultiplayerParams.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
				MultiplayerParams.Add( TEXT( "MatchTypeId" ), FVariantData( (int32)1 ) ); // @todo abstract the specific meaning of this value across platforms
				MultiplayerParams.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused
				MultiplayerParams.Add( TEXT( "TimeInSeconds" ), FVariantData( (float)ExpendedTimeInSeconds ) );
				MultiplayerParams.Add( TEXT( "ExitStatusId" ), FVariantData( (int32)0 ) ); // unused
					
				Events->TriggerEvent(*UniqueId, TEXT("MultiplayerRoundEnd"), MultiplayerParams);
			}
		}

		bHasSentStartEvents = false;
	}
}

void ATrueFPSPlayerController::OnKill()
{
	// TODO: Implement Achievements
	// UpdateAchievementProgress(ACH_FRAG_SOMEONE, 100.0f);

	const UWorld* World = GetWorld();

	const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
	const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

	if (Events.IsValid() && Identity.IsValid())
	{
		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
		if (LocalPlayer)
		{
			int32 UserIndex = LocalPlayer->GetControllerId();
			TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);			
			if (UniqueID.IsValid())
			{			
				ATrueFPSCharacter* TrueFPSCharacter = Cast<ATrueFPSCharacter>(GetCharacter());
				// If player is dead, use location stored during pawn cleanup.
				FVector Location = TrueFPSCharacter ? TrueFPSCharacter->GetActorLocation() : LastDeathLocation;
				ATrueFPSWeaponBase* Weapon = TrueFPSCharacter ? TrueFPSCharacter->GetWeapon() : 0;
				int32 WeaponType = Weapon ? (int32) Weapon->GetAmmoType() : 0;

				FOnlineEventParms Params;		

				Params.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
				Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused

				Params.Add( TEXT( "PlayerRoleId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "PlayerWeaponId" ), FVariantData( (int32)WeaponType ) );
				Params.Add( TEXT( "EnemyRoleId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "EnemyWeaponId" ), FVariantData( (int32)0 ) ); // untracked			
				Params.Add( TEXT( "KillTypeId" ), FVariantData( (int32)0 ) ); // unused
				Params.Add( TEXT( "LocationX" ), FVariantData( Location.X ) );
				Params.Add( TEXT( "LocationY" ), FVariantData( Location.Y ) );
				Params.Add( TEXT( "LocationZ" ), FVariantData( Location.Z ) );
			
				Events->TriggerEvent(*UniqueID, TEXT("KillOponent"), Params);				
			}
		}
	}
}

void ATrueFPSPlayerController::HandleReturnToMainMenu()
{
	// OnHideScoreboard();
	// CleanupSessionOnReturnToMenu();
}

void ATrueFPSPlayerController::OnDeathMessage(ATrueFPSPlayerState* KillerPlayerState,
                                              ATrueFPSPlayerState* KilledPlayerState, const UDamageType* KillerDamageType)
{
	ATrueFPSHUD* TrueFPSHUD = GetTrueFPSHUD();
	if (TrueFPSHUD)
	{
		TrueFPSHUD->ShowDeathMessage(KillerPlayerState, KilledPlayerState, KillerDamageType);		
	}

	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer && LocalPlayer->GetCachedUniqueNetId().IsValid() && KilledPlayerState->GetUniqueId().IsValid())
	{
		// if this controller is the player who died, update the hero stat.
		if (*LocalPlayer->GetCachedUniqueNetId() == *KilledPlayerState->GetUniqueId())
		{
			const UWorld* World = GetWorld();
			const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
			const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

			if (Events.IsValid() && Identity.IsValid())
			{							
				const int32 UserIndex = LocalPlayer->GetControllerId();
				TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
				if (UniqueID.IsValid())
				{				
					ATrueFPSCharacter* TrueFPSCharacter = Cast<ATrueFPSCharacter>(GetCharacter());
					ATrueFPSWeaponBase* Weapon = TrueFPSCharacter ? TrueFPSCharacter->GetWeapon() : nullptr;

					FVector Location = TrueFPSCharacter ? TrueFPSCharacter->GetActorLocation() : FVector::ZeroVector;
					int32 WeaponType = Weapon ? (int32)Weapon->GetAmmoType() : 0;

					FOnlineEventParms Params;
					Params.Add( TEXT( "SectionId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "GameplayModeId" ), FVariantData( (int32)1 ) ); // @todo determine game mode (ffa v tdm)
					Params.Add( TEXT( "DifficultyLevelId" ), FVariantData( (int32)0 ) ); // unused

					Params.Add( TEXT( "PlayerRoleId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "PlayerWeaponId" ), FVariantData( (int32)WeaponType ) );
					Params.Add( TEXT( "EnemyRoleId" ), FVariantData( (int32)0 ) ); // unused
					Params.Add( TEXT( "EnemyWeaponId" ), FVariantData( (int32)0 ) ); // untracked
				
					Params.Add( TEXT( "LocationX" ), FVariantData( Location.X ) );
					Params.Add( TEXT( "LocationY" ), FVariantData( Location.Y ) );
					Params.Add( TEXT( "LocationZ" ), FVariantData( Location.Z ) );
										
					Events->TriggerEvent(*UniqueID, TEXT("PlayerDeath"), Params);
				}
			}
		}
	}
}

void ATrueFPSPlayerController::OnToggleInGameMenu()
{
	if( GEngine->GameViewport == nullptr )
	{
		return;
	}

	// this is not ideal, but necessary to prevent both players from pausing at the same time on the same frame
	UWorld* GameWorld = GEngine->GameViewport->GetWorld();

	for(auto It = GameWorld->GetControllerIterator(); It; ++It)
	{
		ATrueFPSPlayerController* Controller = Cast<ATrueFPSPlayerController>(*It);
		if(Controller && Controller->IsPaused())
		{
			return;
		}
	}

	// if no one's paused, pause
	// if (ShooterIngameMenu.IsValid()) @todo implement ingame menu
	// {
	// 	ShooterIngameMenu->ToggleGameMenu();
	// }
}

bool ATrueFPSPlayerController::IsGameMenuVisible() const
{
	bool Result = false; 
	// if (ShooterIngameMenu.IsValid()) @todo implement ingame menu
	// {
	// 	Result = ShooterIngameMenu->GetIsGameMenuUp();
	// } 

	return Result;
}

void ATrueFPSPlayerController::UpdateAchievementProgress(const FString& Id, float Percent)
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if(OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				FUniqueNetIdRepl UserId = LocalPlayer->GetCachedUniqueNetId();

				if (UserId.IsValid())
				{

					IOnlineAchievementsPtr Achievements = OnlineSub->GetAchievementsInterface();
					if (Achievements.IsValid() && (!WriteObject.IsValid() || WriteObject->WriteState != EOnlineAsyncTaskState::InProgress))
					{
						WriteObject = MakeShareable(new FOnlineAchievementsWrite());
						WriteObject->SetFloatStat(*Id, Percent);

						FOnlineAchievementsWriteRef WriteObjectRef = WriteObject.ToSharedRef();
						Achievements->WriteAchievements(*UserId, WriteObjectRef);
					}
					else
					{
						UE_LOG(LogOnline, Warning, TEXT("No valid achievement interface or another write is in progress."));
					}
				}
				else
				{
					UE_LOG(LogOnline, Warning, TEXT("No valid user id for this controller."));
				}
			}
			else
			{
				UE_LOG(LogOnline, Warning, TEXT("No valid identity interface."));
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("No default online subsystem."));
		}
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("No local player, cannot update achievements."));
	}
}

ATrueFPSHUD* ATrueFPSPlayerController::GetTrueFPSHUD() const
{
	return Cast<ATrueFPSHUD>(GetHUD());
}

UTrueFPSPersistentUser* ATrueFPSPlayerController::GetPersistentUser() const
{
	UTrueFPSLocalPlayer* const TrueFPSLocalPlayer = Cast<UTrueFPSLocalPlayer>(Player);
	return TrueFPSLocalPlayer ? TrueFPSLocalPlayer->GetPersistentUser() : nullptr;
}

void ATrueFPSPlayerController::UpdateAchievementsOnGameEnd()
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer)
	{
		ATrueFPSPlayerState* TrueFPSPlayerState = Cast<ATrueFPSPlayerState>(PlayerState);
		if (TrueFPSPlayerState)
		{			
			const UTrueFPSPersistentUser*  PersistentUser = GetPersistentUser();

			if (PersistentUser)
			{						
				const int32 Wins = PersistentUser->GetWins();
				const int32 Losses = PersistentUser->GetLosses();
				const int32 Matches = Wins + Losses;

				const int32 TotalKills = PersistentUser->GetKills();
				const int32 MatchScore = (int32)TrueFPSPlayerState->GetScore();

				const int32 TotalBulletsFired = PersistentUser->GetBulletsFired();
			
				float TotalGameAchievement = 0;
				float CurrentGameAchievement = 0;
			
				///////////////////////////////////////
				// Kill achievements
				if (TotalKills >= 1)
				{
					CurrentGameAchievement += 100.0f;
				}
				TotalGameAchievement += 100;

				{
					float fSomeKillPct = ((float)TotalKills / (float)SomeKillsCount) * 100.0f;
					fSomeKillPct = FMath::RoundToFloat(fSomeKillPct);
					UpdateAchievementProgress(ACH_SOME_KILLS, fSomeKillPct);

					CurrentGameAchievement += FMath::Min(fSomeKillPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{
					float fLotsKillPct = ((float)TotalKills / (float)LotsKillsCount) * 100.0f;
					fLotsKillPct = FMath::RoundToFloat(fLotsKillPct);
					UpdateAchievementProgress(ACH_LOTS_KILLS, fLotsKillPct);

					CurrentGameAchievement += FMath::Min(fLotsKillPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Match Achievements
				{
					UpdateAchievementProgress(ACH_FINISH_MATCH, 100.0f);

					CurrentGameAchievement += 100;
					TotalGameAchievement += 100;
				}
			
				{
					float fLotsRoundsPct = ((float)Matches / (float)LotsMatchesCount) * 100.0f;
					fLotsRoundsPct = FMath::RoundToFloat(fLotsRoundsPct);
					UpdateAchievementProgress(ACH_LOTS_MATCHES, fLotsRoundsPct);

					CurrentGameAchievement += FMath::Min(fLotsRoundsPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Win Achievements
				if (Wins >= 1)
				{
					UpdateAchievementProgress(ACH_FIRST_WIN, 100.0f);

					CurrentGameAchievement += 100.0f;
				}
				TotalGameAchievement += 100;

				{			
					float fLotsWinPct = ((float)Wins / (float)LotsWinsCount) * 100.0f;
					fLotsWinPct = FMath::RoundToInt(fLotsWinPct);
					UpdateAchievementProgress(ACH_LOTS_WIN, fLotsWinPct);

					CurrentGameAchievement += FMath::Min(fLotsWinPct, 100.0f);
					TotalGameAchievement += 100;
				}

				{			
					float fManyWinPct = ((float)Wins / (float)ManyWinsCount) * 100.0f;
					fManyWinPct = FMath::RoundToInt(fManyWinPct);
					UpdateAchievementProgress(ACH_MANY_WIN, fManyWinPct);

					CurrentGameAchievement += FMath::Min(fManyWinPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Ammo Achievements
				{
					float fLotsBulletsPct = ((float)TotalBulletsFired / (float)LotsBulletsCount) * 100.0f;
					fLotsBulletsPct = FMath::RoundToFloat(fLotsBulletsPct);
					UpdateAchievementProgress(ACH_SHOOT_BULLETS, fLotsBulletsPct);

					CurrentGameAchievement += FMath::Min(fLotsBulletsPct, 100.0f);
					TotalGameAchievement += 100;
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Score Achievements
				{
					float fGoodScorePct = ((float)MatchScore / (float)GoodScoreCount) * 100.0f;
					fGoodScorePct = FMath::RoundToFloat(fGoodScorePct);
					UpdateAchievementProgress(ACH_GOOD_SCORE, fGoodScorePct);
				}

				{
					float fGreatScorePct = ((float)MatchScore / (float)GreatScoreCount) * 100.0f;
					fGreatScorePct = FMath::RoundToFloat(fGreatScorePct);
					UpdateAchievementProgress(ACH_GREAT_SCORE, fGreatScorePct);
				}
				///////////////////////////////////////

				///////////////////////////////////////
				// Map Play Achievements
				UWorld* World = GetWorld();
				if (World)
				{			
					FString MapName = *FPackageName::GetShortName(World->PersistentLevel->GetOutermost()->GetName());
					if (MapName.Find(TEXT("Highrise")) != -1)
					{
						UpdateAchievementProgress(ACH_PLAY_HIGHRISE, 100.0f);
					}
					else if (MapName.Find(TEXT("Sanctuary")) != -1)
					{
						UpdateAchievementProgress(ACH_PLAY_SANCTUARY, 100.0f);
					}
				}
				///////////////////////////////////////			

				const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
				const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

				if (Events.IsValid() && Identity.IsValid())
				{							
					const int32 UserIndex = LocalPlayer->GetControllerId();
					TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
					if (UniqueID.IsValid())
					{				
						FOnlineEventParms Params;

						float fGamePct = (CurrentGameAchievement / TotalGameAchievement) * 100.0f;
						fGamePct = FMath::RoundToFloat(fGamePct);
						Params.Add( TEXT( "CompletionPercent" ), FVariantData( (float)fGamePct ) );
						if (UniqueID.IsValid())
						{				
							Events->TriggerEvent(*UniqueID, TEXT("GameProgress"), Params);
						}
					}
				}
			}
		}
	}
}

void ATrueFPSPlayerController::UpdateLeaderboardsOnGameEnd()
{
	UTrueFPSLocalPlayer* LocalPlayer = Cast<UTrueFPSLocalPlayer>(Player);
	if (LocalPlayer)
	{
		// update leaderboards - note this does not respect existing scores and overwrites them. We would first need to read the leaderboards if we wanted to do that.
		IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
			if (Identity.IsValid())
			{
				TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalPlayer->GetControllerId());
				if (UserId.IsValid())
				{
					IOnlineLeaderboardsPtr Leaderboards = OnlineSub->GetLeaderboardsInterface();
					if (Leaderboards.IsValid())
					{
						ATrueFPSPlayerState* TrueFPSPlayerState = Cast<ATrueFPSPlayerState>(PlayerState);
						if (TrueFPSPlayerState)
						{
							FTrueFPSAllTimeMatchResultsWrite ResultsWriteObject;
							int32 MatchWriteData = 1;
							int32 KillsWriteData = TrueFPSPlayerState->GetKills();
							int32 DeathsWriteData = TrueFPSPlayerState->GetDeaths();

#if TRACK_STATS_LOCALLY
							StatMatchesPlayed = (MatchWriteData += StatMatchesPlayed);
							StatKills = (KillsWriteData += StatKills);
							StatDeaths = (DeathsWriteData += StatDeaths);
#endif

							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_SCORE, KillsWriteData);
							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_KILLS, KillsWriteData);
							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_DEATHS, DeathsWriteData);
							ResultsWriteObject.SetIntStat(LEADERBOARD_STAT_MATCHESPLAYED, MatchWriteData);

							// the call will copy the user id and write object to its own memory
							Leaderboards->WriteLeaderboards(TrueFPSPlayerState->SessionName, *UserId, ResultsWriteObject);
							Leaderboards->FlushLeaderboards(TEXT("TRUEFPSSYSTEM"));
						}
					}
				}
			}
		}
	}
}

void ATrueFPSPlayerController::UpdateStatsOnGameEnd(bool bIsWinner)
{
	const IOnlineStatsPtr Stats = Online::GetStatsInterface(GetWorld());
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	ATrueFPSPlayerState* TrueFPSPlayerState = Cast<ATrueFPSPlayerState>(PlayerState);

	if (Stats.IsValid() && LocalPlayer != nullptr && TrueFPSPlayerState != nullptr)
	{
		FUniqueNetIdRepl UniqueId = LocalPlayer->GetCachedUniqueNetId();

		if (UniqueId.IsValid() )
		{
			TArray<FOnlineStatsUserUpdatedStats> UpdatedUserStats;

			FOnlineStatsUserUpdatedStats& UpdatedStats = UpdatedUserStats.Emplace_GetRef( UniqueId.GetUniqueNetId().ToSharedRef() );
			UpdatedStats.Stats.Add( TEXT("Kills"), FOnlineStatUpdate( TrueFPSPlayerState->GetKills(), FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			UpdatedStats.Stats.Add( TEXT("Deaths"), FOnlineStatUpdate( TrueFPSPlayerState->GetDeaths(), FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			UpdatedStats.Stats.Add( TEXT("RoundsPlayed"), FOnlineStatUpdate( 1, FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			if (bIsWinner)
			{
				UpdatedStats.Stats.Add( TEXT("RoundsWon"), FOnlineStatUpdate( 1, FOnlineStatUpdate::EOnlineStatModificationType::Sum ) );
			}

			Stats->UpdateStats( UniqueId.GetUniqueNetId().ToSharedRef(), UpdatedUserStats, FOnlineStatsUpdateStatsComplete() );
		}
	}
}

void ATrueFPSPlayerController::UpdateSaveFileOnGameEnd(bool bIsWinner)
{
	ATrueFPSPlayerState* TrueFPSPlayerState = Cast<ATrueFPSPlayerState>(PlayerState);
	if (TrueFPSPlayerState)
	{
		// update local saved profile
		UTrueFPSPersistentUser* const PersistentUser = GetPersistentUser();
		if (PersistentUser)
		{
			PersistentUser->AddMatchResult(TrueFPSPlayerState->GetKills(), TrueFPSPlayerState->GetDeaths(), TrueFPSPlayerState->GetNumBulletsFired(), bIsWinner);
			PersistentUser->SaveIfDirty();
		}
	}
}
