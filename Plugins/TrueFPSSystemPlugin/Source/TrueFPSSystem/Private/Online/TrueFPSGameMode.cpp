// Fill out your copyright notice in the Description page of Project Settings.

#include "Online/TrueFPSGameMode.h"

#include "EngineUtils.h"
#include "TrueFPSGameInstance.h"
#include "TrueFPSTeamStart.h"
#include "Bots/TrueFPSAIController.h"
#include "Character/TrueFPSCharacter.h"
#include "Character/TrueFPSPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Engine/PlayerStartPIE.h"
#include "Kismet/GameplayStatics.h"
#include "Online/TrueFPSGameSession.h"
#include "Online/TrueFPSGameState.h"
#include "Online/TrueFPSPlayerState.h"
#include "UI/TrueFPSHUD.h"

ATrueFPSGameMode::ATrueFPSGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Should be set in Blueprints
	// static ConstructorHelpers::FClassFinder<APawn> PlayerPawnOb(TEXT("/Game/Blueprints/Pawns/PlayerPawn"));
	// DefaultPawnClass = PlayerPawnOb.Class;

	// Should be set in Blueprints
	// static ConstructorHelpers::FClassFinder<APawn> BotPawnOb(TEXT("/Game/Blueprints/Pawns/BotPawn"));
	// BotPawnClass = BotPawnOb.Class;

	HUDClass = ATrueFPSHUD::StaticClass();
	PlayerControllerClass = ATrueFPSPlayerController::StaticClass();
	PlayerStateClass = ATrueFPSPlayerState::StaticClass();
	// SpectatorClass = ATrueFPSSpectatorPawn::StaticClass();
	GameStateClass = ATrueFPSGameState::StaticClass();
	// ReplaySpectatorPlayerControllerClass = ATrueFPSDemoSpectator::StaticClass();

	MinRespawnDelay = 5.0f;

	bAllowBots = true;	
	bNeedsBotCreation = true;
	bUseSeamlessTravel = FParse::Param(FCommandLine::Get(), TEXT("NoSeamlessTravel")) ? false : true;
}

void ATrueFPSGameMode::SetAllowBots(bool bInAllowBots, int32 InMaxBots)
{
	bAllowBots = bInAllowBots;
	MaxBots = InMaxBots;
}

void ATrueFPSGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &ThisClass::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);
}

void ATrueFPSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	const int32 BotsCountOptionValue = BotsAmount > 0 ? BotsAmount : UGameplayStatics::GetIntOption(Options, GetBotsCountOptionName(), 0);
	SetAllowBots(BotsCountOptionValue > 0 ? true : false, BotsCountOptionValue);	
	Super::InitGame(MapName, Options, ErrorMessage);

	const UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance && Cast<UTrueFPSGameInstance>(GameInstance)->GetOnlineMode() != EOnlineMode::Offline)
	{
		bPauseable = false;
	}
}

void ATrueFPSGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	ATrueFPSGameState* const MyGameState = Cast<ATrueFPSGameState>(GameState);
	const bool bMatchIsOver = MyGameState && MyGameState->HasMatchEnded();
	if( bMatchIsOver )
	{
		ErrorMessage = TEXT("Match is over!");
	}
	else
	{
		// GameSession can be nullptr if the match is over
		Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	}
}

void ATrueFPSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// update spectator location for client
	ATrueFPSPlayerController* NewPC = Cast<ATrueFPSPlayerController>(NewPlayer);
	if (NewPC && NewPC->GetPawn() == nullptr)
	{
		NewPC->ClientSetSpectatorCamera(NewPC->GetSpawnLocation(), NewPC->GetControlRotation());
	}

	// notify new player if match is already in progress
	if (NewPC && IsMatchInProgress())
	{
		NewPC->ClientGameStarted();
		NewPC->ClientStartOnlineGame();
	}
}

void ATrueFPSGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(NewPlayer);
	if (PC)
	{
		ATrueFPSHUD* TrueFPSHUD = Cast<ATrueFPSHUD>(PC->GetHUD());
		if (TrueFPSHUD)
		{
			// Passing HUD draw settings
			TrueFPSHUD->SetShouldDrawDeathMessages(bDrawDeathMessages);
			TrueFPSHUD->SetShouldDrawMatchTime(bDrawMatchTime);
			TrueFPSHUD->SetShouldDrawKills(bDrawKills);
		}
		
		PC->ClientGameStarted();
	}
}

AActor* ATrueFPSGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> PreferredSpawns;
	TArray<APlayerStart*> FallbackSpawns;

	APlayerStart* BestStart = nullptr;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* TestSpawn = *It;
		if (TestSpawn->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			BestStart = TestSpawn;
			break;
		}
		else
		{
			if (IsSpawnpointAllowed(TestSpawn, Player))
			{
				if (IsSpawnpointPreferred(TestSpawn, Player))
				{
					PreferredSpawns.Add(TestSpawn);
				}
				else
				{
					FallbackSpawns.Add(TestSpawn);
				}
			}
		}
	}

	
	if (BestStart == nullptr)
	{
		if (PreferredSpawns.Num() > 0)
		{
			BestStart = PreferredSpawns[FMath::RandHelper(PreferredSpawns.Num())];
		}
		else if (FallbackSpawns.Num() > 0)
		{
			BestStart = FallbackSpawns[FMath::RandHelper(FallbackSpawns.Num())];
		}
	}

	return BestStart ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
}

bool ATrueFPSGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

UClass* ATrueFPSGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (InController->IsA<ATrueFPSAIController>())
	{
		return BotPawnClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

float ATrueFPSGameMode::ModifyDamage(float Damage, AActor* DamagedActor, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser) const
{
	float ActualDamage = Damage;

	ATrueFPSCharacter* DamagedPawn = Cast<ATrueFPSCharacter>(DamagedActor);
	if (DamagedPawn && EventInstigator)
	{
		ATrueFPSPlayerState* DamagedPlayerState = Cast<ATrueFPSPlayerState>(DamagedPawn->GetPlayerState());
		ATrueFPSPlayerState* InstigatorPlayerState = Cast<ATrueFPSPlayerState>(EventInstigator->PlayerState);

		// disable friendly fire
		if (!CanDealDamage(InstigatorPlayerState, DamagedPlayerState))
		{
			ActualDamage = 0.0f;
		}

		// scale self instigated damage
		if (InstigatorPlayerState == DamagedPlayerState)
		{
			ActualDamage *= DamageSelfScale;
		}
	}

	return ActualDamage;
}

void ATrueFPSGameMode::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn,
	const UDamageType* DamageType)
{
	ATrueFPSPlayerState* KillerPlayerState = Killer ? Cast<ATrueFPSPlayerState>(Killer->PlayerState) : nullptr;
	ATrueFPSPlayerState* VictimPlayerState = KilledPlayer ? Cast<ATrueFPSPlayerState>(KilledPlayer->PlayerState) : nullptr;

	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		KillerPlayerState->ScoreKill(VictimPlayerState, KillScore);
		KillerPlayerState->InformAboutKill(KillerPlayerState, DamageType, VictimPlayerState);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->ScoreDeath(KillerPlayerState, DeathScore);
		VictimPlayerState->BroadcastDeath(KillerPlayerState, DamageType, VictimPlayerState);
	}
}

bool ATrueFPSGameMode::CanDealDamage(ATrueFPSPlayerState* DamageInstigator, ATrueFPSPlayerState* DamagedPlayer) const
{
	return true;
}

bool ATrueFPSGameMode::AllowCheats(APlayerController* P)
{
	return true;
}

void ATrueFPSGameMode::DefaultTimer()
{
	// don't update timers for Play In Editor mode, it's not real match
	if (GetWorld()->IsPlayInEditor())
	{
		// start match if necessary.
		if (GetMatchState() == MatchState::WaitingToStart)
		{
			StartMatch();
		}
		return;
	}

	ATrueFPSGameState* const MyGameState = Cast<ATrueFPSGameState>(GameState);
	if (MyGameState && MyGameState->RemainingTime > 0 && !MyGameState->bTimerPaused)
	{
		MyGameState->RemainingTime--;
		
		if (MyGameState->RemainingTime <= 0)
		{
			if (GetMatchState() == MatchState::WaitingPostMatch)
			{
				RestartGame();
			}
			else if (GetMatchState() == MatchState::InProgress)
			{
				FinishMatch();

				// Send end round events
				for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
				{
					ATrueFPSPlayerController* PlayerController = Cast<ATrueFPSPlayerController>(*It);
					
					if (PlayerController && MyGameState)
					{
						ATrueFPSPlayerState* PlayerState = Cast<ATrueFPSPlayerState>((*It)->PlayerState);
						const bool bIsWinner = IsWinner(PlayerState);
					
						PlayerController->ClientSendRoundEndEvent(bIsWinner, MyGameState->ElapsedTime);
					}
				}
			}
			else if (GetMatchState() == MatchState::WaitingToStart)
			{
				StartMatch();
			}
		}
	}
}

void ATrueFPSGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	if (bNeedsBotCreation)
	{
		CreateBotControllers();
		bNeedsBotCreation = false;
	}

	if (bDelayedStart)
	{
		// start warmup if needed
		ATrueFPSGameState* const MyGameState = Cast<ATrueFPSGameState>(GameState);
		if (MyGameState && MyGameState->RemainingTime == 0)
		{
			const bool bWantsMatchWarmup = !GetWorld()->IsPlayInEditor();
			if (bWantsMatchWarmup && WarmupTime > 0)
			{
				MyGameState->RemainingTime = WarmupTime;
			}
			else
			{
				MyGameState->RemainingTime = 0.0f;
			}
		}
	}
}

void ATrueFPSGameMode::HandleMatchHasStarted()
{
	bNeedsBotCreation = true;
	Super::HandleMatchHasStarted();

	ATrueFPSGameState* const MyGameState = Cast<ATrueFPSGameState>(GameState);
	MyGameState->RemainingTime = RoundTime;	
	StartBots();	

	// notify players
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(*It);
		if (PC)
		{
			PC->ClientGameStarted();
		}
	}
}

void ATrueFPSGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void ATrueFPSGameMode::RestartGame()
{
	// Hide the scoreboard too !
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		ATrueFPSPlayerController* PlayerController = Cast<ATrueFPSPlayerController>(*It);
		if (PlayerController != nullptr)
		{
			ATrueFPSHUD* TrueFPSHUD = Cast<ATrueFPSHUD>(PlayerController->GetHUD());
			if (TrueFPSHUD != nullptr)
			{
				// Passing true to bFocus here ensures that focus is returned to the game viewport.
				TrueFPSHUD->ShowScoreboard(false, true);
			}
		}
	}

	Super::RestartGame();
}

void ATrueFPSGameMode::CreateBotControllers()
{
	UWorld* World = GetWorld();
	int32 ExistingBots = 0;
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		ATrueFPSAIController* AIC = Cast<ATrueFPSAIController>(*It);
		if (AIC)
		{
			++ExistingBots;
		}
	}

	// Create any necessary AIControllers.  Hold off on Pawn creation until pawns are actually necessary or need recreating.	
	int32 BotNum = ExistingBots;
	for (int32 i = 0; i < MaxBots - ExistingBots; ++i)
	{
		CreateBot(BotNum + i);
	}
}

ATrueFPSAIController* ATrueFPSGameMode::CreateBot(int32 BotNum)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = nullptr;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = nullptr;

	UWorld* World = GetWorld();
	ATrueFPSAIController* AIC = World->SpawnActor<ATrueFPSAIController>(SpawnInfo);
	InitBot(AIC, BotNum);

	return AIC;
}

void ATrueFPSGameMode::PostInitProperties()
{
	Super::PostInitProperties();
	if (PlatformPlayerControllerClass != nullptr)
	{
		PlayerControllerClass = PlatformPlayerControllerClass;
	}
}

void ATrueFPSGameMode::StartBots()
{
	// checking number of existing human player.
	UWorld* World = GetWorld();
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		ATrueFPSAIController* AIC = Cast<ATrueFPSAIController>(*It);
		if (AIC)
		{
			RestartPlayer(AIC);
		}
	}
}

void ATrueFPSGameMode::InitBot(ATrueFPSAIController* AIController, int32 BotNum)
{
	if (AIController)
	{
		if (AIController->PlayerState)
		{
			FString BotName = FString::Printf(TEXT("Bot %d"), BotNum);
			AIController->PlayerState->SetPlayerName(BotName);
		}		
	}
}

void ATrueFPSGameMode::DetermineMatchWinner()
{
	// nothing to do here
}

bool ATrueFPSGameMode::IsWinner(ATrueFPSPlayerState* PlayerState) const
{
	return false;
}

bool ATrueFPSGameMode::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{
	ATrueFPSTeamStart* TrueFPSSpawnPoint = Cast<ATrueFPSTeamStart>(SpawnPoint);
	if (TrueFPSSpawnPoint)
	{
		ATrueFPSAIController* AIController = Cast<ATrueFPSAIController>(Player);
		if (TrueFPSSpawnPoint->bNotForBots && AIController)
		{
			return false;
		}

		if (TrueFPSSpawnPoint->bNotForPlayers && AIController == nullptr)
		{
			return false;
		}
		return true;
	}

	return false;
}

bool ATrueFPSGameMode::IsSpawnpointPreferred(APlayerStart* SpawnPoint, AController* Player) const
{
	ACharacter* MyPawn = Cast<ACharacter>((*DefaultPawnClass)->GetDefaultObject<ACharacter>());	
	ATrueFPSAIController* AIController = Cast<ATrueFPSAIController>(Player);
	if( AIController != nullptr )
	{
		MyPawn = Cast<ACharacter>(BotPawnClass->GetDefaultObject<ACharacter>());
	}
	
	if (MyPawn)
	{
		const FVector SpawnLocation = SpawnPoint->GetActorLocation();
		for (ACharacter* OtherPawn : TActorRange<ACharacter>(GetWorld()))
		{
			if (OtherPawn != MyPawn)
			{
				const float CombinedHeight = (MyPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) * 2.0f;
				const float CombinedRadius = MyPawn->GetCapsuleComponent()->GetScaledCapsuleRadius() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleRadius();
				const FVector OtherLocation = OtherPawn->GetActorLocation();

				// check if player start overlaps this pawn
				if (FMath::Abs(SpawnLocation.Z - OtherLocation.Z) < CombinedHeight && (SpawnLocation - OtherLocation).Size2D() < CombinedRadius)
				{
					return false;
				}
			}
		}
	}
	else
	{
		return false;
	}
	
	return true;
}

TSubclassOf<AGameSession> ATrueFPSGameMode::GetGameSessionClass() const
{
	return ATrueFPSGameSession::StaticClass();
}

void ATrueFPSGameMode::FinishMatch()
{
	ATrueFPSGameState* const MyGameState = Cast<ATrueFPSGameState>(GameState);
	if (IsMatchInProgress())
	{
		EndMatch();
		DetermineMatchWinner();		

		// notify players
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			ATrueFPSPlayerState* PlayerState = Cast<ATrueFPSPlayerState>((*It)->PlayerState);
			const bool bIsWinner = IsWinner(PlayerState);

			(*It)->GameHasEnded(nullptr, bIsWinner);
		}

		// lock all pawns
		// pawns are not marked as keep for seamless travel, so we will create new pawns on the next match rather than
		// turning these back on.
		for (APawn* Pawn : TActorRange<APawn>(GetWorld()))
		{
			Pawn->TurnOff();
		}

		// set up to restart the match
		MyGameState->RemainingTime = TimeBetweenMatches;
	}
}

void ATrueFPSGameMode::RequestFinishAndExitToMainMenu()
{
	FinishMatch();

	UTrueFPSGameInstance* const GameInstance = Cast<UTrueFPSGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->RemoveSplitScreenPlayers();
	}

	ATrueFPSPlayerController* LocalPrimaryController = nullptr;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		ATrueFPSPlayerController* Controller = Cast<ATrueFPSPlayerController>(*Iterator);

		if (Controller == nullptr)
		{
			continue;
		}

		if (!Controller->IsLocalController())
		{
			const FText RemoteReturnReason = NSLOCTEXT("NetworkErrors", "HostHasLeft", "Host has left the game.");
			Controller->ClientReturnToMainMenuWithTextReason(RemoteReturnReason);
		}
		else
		{
			LocalPrimaryController = Controller;
		}
	}

	// GameInstance should be calling this from an EndState.  So call the PC function that performs cleanup, not the one that sets GI state.
	if (LocalPrimaryController != nullptr)
	{
		LocalPrimaryController->HandleReturnToMainMenu();
	}
}

FString ATrueFPSGameMode::GetBotsCountOptionName()
{
	return FString(TEXT("Bots"));
}


