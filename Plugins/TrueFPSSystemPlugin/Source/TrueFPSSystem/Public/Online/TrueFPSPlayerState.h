// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TrueFPSPlayerState.generated.h"

UCLASS()
class TRUEFPSSYSTEM_API ATrueFPSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	
	ATrueFPSPlayerState(const FObjectInitializer& ObjectInitializer);

	// Begin APlayerState interface
	/** clear scores */
	virtual void Reset() override;

	/**
	 * Set the team 
	 *
	 * @param	InController	The controller to initialize state with
	 */
	virtual void ClientInitialize(class AController* InController) override;

	virtual void RegisterPlayerWithSession(bool bWasFromInvite) override;
	virtual void UnregisterPlayerWithSession() override;

	// End APlayerState interface

	/**
	 * Set new team and update pawn. Also updates player character team colors.
	 *
	 * @param	NewTeamNumber	Team we want to be on.
	 */
	void SetTeamNum(int32 NewTeamNumber);

	/** player killed someone */
	void ScoreKill(ATrueFPSPlayerState* Victim, int32 Points);

	/** player died */
	void ScoreDeath(ATrueFPSPlayerState* KilledBy, int32 Points);

	/** get current team */
	int32 GetTeamNum() const;

	/** get number of kills */
	int32 GetKills() const;

	/** get number of deaths */
	int32 GetDeaths() const;

	/** get number of bullets fired this match */
	int32 GetNumBulletsFired() const;

	/** get whether the player quit the match */
	bool IsQuitter() const;

	/** get match id that the player is in */
	FString GetMatchId() const;

	/** gets truncated player name to fit in death log and scoreboards */
	FString GetShortPlayerName() const;

	/** Sends kill (excluding self) to clients */
	UFUNCTION(Reliable, Client)
	void InformAboutKill(class ATrueFPSPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class ATrueFPSPlayerState* KilledPlayerState);

	/** broadcast death to local clients */
	UFUNCTION(Reliable, NetMulticast)
	void BroadcastDeath(class ATrueFPSPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class ATrueFPSPlayerState* KilledPlayerState);

	/** replicate team colors. Updated the players mesh colors appropriately */
	UFUNCTION()
	void OnRep_TeamColor();

	// We don't need stats about amount of ammo fired to be server authenticated, so just increment these with local functions
	void AddBulletsFired(int32 NumBullets);

	/** Set whether the player is a quitter */
	void SetQuitter(bool bInQuitter);

	/** Set the player's current match id */
	void SetMatchId(const FString& CurrentMatchId);

	virtual void CopyProperties(class APlayerState* PlayerState) override;
	
protected:

	/** Set the mesh colors based on the current teamnum variable */
	void UpdateTeamColors();

	/** team number */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_TeamColor)
	int32 TeamNumber;

	/** number of kills */
	UPROPERTY(Transient, Replicated)
	int32 NumKills;

	/** number of deaths */
	UPROPERTY(Transient, Replicated)
	int32 NumDeaths;

	/** number of bullets fired this match */
	UPROPERTY()
	int32 NumBulletsFired;

	/** whether the user quit the match */
	UPROPERTY()
	uint8 bQuitter : 1;

	/** Match id */
	UPROPERTY(Replicated)
	FString MatchId;

	/** helper for scoring points */
	void ScorePoints(int32 Points);
	
};
