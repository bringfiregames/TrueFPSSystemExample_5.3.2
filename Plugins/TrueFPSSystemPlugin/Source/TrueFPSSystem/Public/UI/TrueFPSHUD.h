// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TrueFPSTypes.h"
#include "Engine/Canvas.h"
#include "GameFramework/HUD.h"
#include "Interfaces/VoiceInterface.h"
#include "TrueFPSHUD.generated.h"

struct FHitData
{
	/** Last hit time. */
	float HitTime;
	
	/** strength of hit icon */
	float HitPercentage;

	/** Initialise defaults. */
	FHitData()
	{
		HitTime = 0.0f;
		HitPercentage = 0.0f;
	}
};

struct FDeathMessage
{
	/** Name of player scoring kill. */
	FString KillerDesc;

	/** Name of killed player. */
	FString VictimDesc;

	/** Killer is local player. */
	uint8 bKillerIsOwner : 1;
	
	/** Victim is local player. */
	uint8 bVictimIsOwner : 1;

	/** Team number of the killer. */
	int32 KillerTeamNum;

	/** Team number of the victim. */
	int32 VictimTeamNum; 

	/** timestamp for removing message */
	float HideTime;

	/** What killed the player. */
	TWeakObjectPtr<class UTrueFPSDamageType> DamageType;

	/** Initialise defaults. */
	FDeathMessage()
		: bKillerIsOwner(false)
		, bVictimIsOwner(false)
		, KillerTeamNum(0)
		, VictimTeamNum(0)		
		, HideTime(0.f)
	{
	}
};

/**
 * 
 */
UCLASS()
class TRUEFPSSYSTEM_API ATrueFPSHUD : public AHUD
{
	GENERATED_BODY()

public:

	ATrueFPSHUD(const FObjectInitializer& ObjectInitializer);

virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Main HUD update loop. */
	virtual void DrawHUD() override;

	/**
	 * Sent from pawn hit, used to calculate hit notification overlay for drawing.
	 *
	 * @param	DamageTaken		The amount of damage.
	 * @param	DamageEvent		The actual damage event.
	 * @param	PawnInstigator	The pawn that did the damage.
	 */
	void NotifyWeaponHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator);

	/** Sent from TrueFPSWeaponBase, shows NO AMMO text. */
	void NotifyOutOfAmmo();

	/** Notifies we have hit the enemy. */
	void NotifyEnemyHit(bool bKilled = false);

	/** 
	 * Set state of current match.
	 *
	 * @param	NewState	The new match state.
	 */
	void SetMatchState(ETrueFPSMatchState::Type NewState);

	/** Get state of current match. */
	ETrueFPSMatchState::Type GetMatchState() const;

	/** Turns off scoreboard if it is being displayed */
	void ConditionalCloseScoreboard(bool bFocus = false);

	/** Toggles scoreboard */
	void ToggleScoreboard();

	/** 
	 * Toggles in game scoreboard.
	 * Note:Will not display if the game menu is visible.

	 * @param	bEnable	Required scoreboard display state.
	 * @param	bFocus	Give keyboard focus to the scoreboard.
	 * @return	true, if the scoreboard visibility changed
	 */
	bool ShowScoreboard(bool bEnable, bool bFocus = false);

	/** 
	 * Add death message.
	 *
	 * @param	KillerPlayerState	Player that did the killings state.
	 * @param	VictimPlayerState	Played that was killed state.
	 * @param	KillerDamageType	The type of damaged that caused the death.
	 */
	void ShowDeathMessage(class ATrueFPSPlayerState* KillerPlayerState, class ATrueFPSPlayerState* VictimPlayerState, const UDamageType* KillerDamageType);

	/*
	 * Toggle chat window visibility.
	 *
	 */
	void ToggleChat();

	/** 
	 * Set chat window visibility.
	 *
	 * @param	RequiredVisibility	The required visibility.
	 */
	void SetChatVisibilty( const EVisibility RequiredVisibility );

	/*
	 * Add a string to the chat window.
	 *
	 * @param	ChatString	The string to add.
	 * @param	bWantFocus	Should we set the chat window to focus
	 */
	void AddChatLine(const FText& ChatString, bool bWantFocus);

	/* Is the match over (IE Is the state Won or Lost). */
	bool IsMatchOver() const;

	/** 
	 * Set if should draw death messages
	 *
	 * @param	bValue	The new value.
	 */
	FORCEINLINE void SetShouldDrawDeathMessages( const bool bValue ) { bShouldDrawDeathMessages = bValue; }

	/** 
	 * Set if should draw match time
	 *
	 * @param	bValue	The new value.
	 */
	FORCEINLINE void SetShouldDrawMatchTime( const bool bValue ) { bShouldDrawMatchTime = bValue; }

	/** 
	 * Set if should draw kills
	 *
	 * @param	bValue	The new value.
	 */
	FORCEINLINE void SetShouldDrawKills( const bool bValue ) { bShouldDrawKills = bValue; }
		
protected:
	
	/** Floor for automatic hud scaling. */
	static const float MinHudScale;

	/** Reflex Time Accumulator */
	float TimePassed;

	/** Reflex Timers */
	float LatencyTotal, LatencyGame, LatencyRender, Framerate;

	/** Lighter HUD color. */
	FColor HUDLight;

	/** Darker HUD color. */
	FColor HUDDark;

	/** When we got last notice about out of ammo. */
	float NoAmmoNotifyTime;

	/** How long notice is fading out. */
	float NoAmmoFadeOutTime;

	/** Most recent hit time, used to check if we need to draw hit indicator at all. */
	float LastHitTime;

	/** How long till hit notify fades out completely. */
	float HitNotifyDisplayTime;

	/** When we last time hit the enemy. */
	float LastEnemyHitTime;

	/** If last enemy hit killed him. */
	bool bLastEnemyHitKilled;

	/** How long till enemy hit notify fades out completely. */
	float LastEnemyHitDisplayTime;

	/** If should draw death messages. */
	bool bShouldDrawDeathMessages;

	/** If should draw match time. */
	bool bShouldDrawMatchTime;

	/** If should draw kills. */
	bool bShouldDrawKills;

	/** Icons for hit indicator. */
	UPROPERTY()
	FCanvasIcon HitNotifyIcon[8];

	/** kills background icon. */
	UPROPERTY()
	FCanvasIcon KillsBg;

	/** Match timer and player position background icon. */
	UPROPERTY()
	FCanvasIcon TimePlaceBg;

	/** Primary weapon background icon. */
	UPROPERTY()
	FCanvasIcon PrimaryWeapBg;

	/** Secondary weapon background icon */
	UPROPERTY()
	FCanvasIcon SecondaryWeapBg;

	/** Crosshair icons (left, top, right, bottom and center). */
	UPROPERTY()
	FCanvasIcon Crosshair[5];

	/** On crosshair indicator that we hit someone. */
	UPROPERTY()
	FCanvasIcon HitNotifyCrosshair;

	/** Death messages background icon. */
	UPROPERTY()
	FCanvasIcon DeathMessagesBg;

	/** Health bar background icon. */
	UPROPERTY()
	FCanvasIcon HealthBarBg;

	/** Health bar icon. */
	UPROPERTY()
	FCanvasIcon HealthBar;

	/** Health icon on the health bar. */
	UPROPERTY()
	FCanvasIcon HealthIcon;

	/** Kills icon. */
	UPROPERTY()
	FCanvasIcon KillsIcon;

	/** Bigger killed icon. */
	UPROPERTY()
	FCanvasIcon KilledIcon;

	/** Timer icon. */
	UPROPERTY()
	FCanvasIcon TimerIcon;

	/** Podium icon. */
	UPROPERTY()
	FCanvasIcon PlaceIcon;

	/** UI scaling factor for other resolutions than Full HD. */
	float ScaleUI;

	/** Current animation pulse value. */
	float PulseValue;

	/** FontRenderInfo enabling casting shadow.s */
	FFontRenderInfo ShadowedFont;

	/** Big "KILLED [PLAYER]" message text above the crosshair. */
	FText CenteredKillMessage;

	/** last time we killed someone. */
	float LastKillTime;

	/** How long the message will fade out. */
	float KillFadeOutTime;

	/** Offsets to display hit indicator parts. */
	FVector2D Offsets[8];

	/** Texture for hit indicator. */
	UPROPERTY()
	UTexture2D* HitNotifyTexture;

	/** texture for HUD elements. */
	UPROPERTY()
	UTexture2D* HUDMainTexture;

	/** Texture for HUD elements. */
	UPROPERTY()
	UTexture2D* HUDAssets02Texture;

	/** Overlay shown when health is low. */
	UPROPERTY()
	UTexture2D* LowHealthOverlayTexture;

	/** Large font - used for ammo display etc. */
	UPROPERTY()
	UFont* BigFont;

	/** Normal font - used for death messages and such. */
	UPROPERTY()
	UFont* NormalFont;

	/** General offset for HUD elements. */
	float Offset;

	/** Runtime data for hit indicator. */
	FHitData HitNotifyData[8];

	/** Active death messages. */
	TArray<FDeathMessage> DeathMessages;

	/** State of match. */
	ETrueFPSMatchState::Type MatchState;

	/** Is the scoreboard widget on screen? */
	uint32 bIsScoreBoardVisible:1;

	/** Scoreboard widget. */
	TSharedPtr<class STrueFPSScoreboardWidget>	ScoreboardWidget;

	/** Scoreboard widget overlay. */
	TSharedPtr<class SOverlay>	ScoreboardWidgetOverlay;

	/** Scoreboard widget container - used for removing */
	TSharedPtr<class SWidget> ScoreboardWidgetContainer;

	/** Chatbox widget. */
	TSharedPtr<class SChatWidget> ChatWidget;

	/** Array of information strings to render (Waiting to respawn etc) */
	TArray<FCanvasTextItem> InfoItems;

	/** Called every time game is started. */
	virtual void PostInitializeComponents() override;

	/** 
	 * Converts floating point seconds to MM:SS string.
	 *
	 * @param TimeSeconds		The time to get a string for.
	 */
	FString GetTimeString(float TimeSeconds);

	/** Draws weapon HUD. */
	void DrawWeaponHUD();

	/** Draws kills information. */
	void DrawKills();

	/** Draw player's health bar. */
	void DrawHealth();

	/** Draws match timer and player position. */
	void DrawMatchTimerAndPosition();

	/** Draws weapon crosshair. */
	void DrawCrosshair();
	
	/** Draws hit indicator. */
	void DrawHitIndicator();

	/** Draw death messages. */
	void DrawDeathMessages();

	/** Draw NVIDIA reflex timers */
	void DrawNVIDIAReflexTimers();

	/** Draw Performance timer */
	void DrawPerfTimer(const FString& Label, const FString& Value, float PosX, float PosY);

	/** Delegate for telling other methods when players have started/stopped talking */
	FOnPlayerTalkingStateChangedDelegate OnPlayerTalkingStateChangedDelegate;
	void OnPlayerTalkingStateChanged(TSharedRef<const FUniqueNetId> TalkingPlayerId, bool bIsTalking);

	/*
	 * Draw the most recently killed player if needed
	 *
	 * @return Returns the bottom of the message or 0 if not drawn.
	 */
	float DrawRecentlyKilledPlayer();

	/** Temporary helper for drawing text-in-a-box. */
	void DrawDebugInfoString(const FString& Text, float PosX, float PosY, bool bAlignLeft, bool bAlignTop, const FColor& TextColor);

	/** helper for getting uv coords in normalized top,left, bottom, right format */
	void MakeUV(FCanvasIcon& Icon, FVector2D& UV0, FVector2D& UV1, uint16 U, uint16 V, uint16 UL, uint16 VL);

	/*
	 * Create the chat widget if it doesn't already exist.
	 *
	 * @return		true if the widget was created.
	 */
	bool TryCreateChatWidget();

	/*
	 * Add information string that will be displayed on the hud. They are added as required and rendered together to prevent overlaps 
	 * 
	 * @param InInfoString	InInfoString
	*/
	void AddMatchInfoString(const FCanvasTextItem InfoItem);

	/*
	* Render the info messages.
	*
	* @param YOffset	YOffset from top of canvas to start drawing the messages
	* @param ScaleUI	UI Scale factor
	* @param TextScale	Text scale factor
	*
	* @returns The next Y position to draw any further strings
	*/
	float ShowInfoItems(float YOffset, float TextScale);
	
};
