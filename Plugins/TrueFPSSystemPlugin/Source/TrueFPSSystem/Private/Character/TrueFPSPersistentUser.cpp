// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/TrueFPSPersistentUser.h"

#include "Character/TrueFPSLocalPlayer.h"
#include "Kismet/GameplayStatics.h"

UTrueFPSPersistentUser::UTrueFPSPersistentUser(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetToDefaults();
}

UTrueFPSPersistentUser* UTrueFPSPersistentUser::LoadPersistentUser(FString SlotName, const int32 UserIndex)
{
	UTrueFPSPersistentUser* Result = nullptr;
	
	// first set of player signins can happen before the UWorld exists, which means no OSS, which means no user names, which means no slotnames.
	// Persistent users aren't valid in this state.
	if (SlotName.Len() > 0)
	{
		if (!GIsBuildMachine && UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
		{
			Result = Cast<UTrueFPSPersistentUser>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
		}

		if (Result == nullptr)
		{
			// if failed to load, create a new one
			Result = Cast<UTrueFPSPersistentUser>( UGameplayStatics::CreateSaveGameObject(UTrueFPSPersistentUser::StaticClass()) );
		}
		check(Result != nullptr);
	
		Result->SlotName = SlotName;
		Result->UserIndex = UserIndex;
	}

	return Result;
}

void UTrueFPSPersistentUser::SaveIfDirty()
{
	if (bIsDirty || IsInvertedYAxisDirty() || IsAimSensitivityDirty())
	{
		SavePersistentUser();
	}
}

void UTrueFPSPersistentUser::AddMatchResult(int32 MatchKills, int32 MatchDeaths, int32 MatchBulletsFired,
	bool bIsMatchWinner)
{
	Kills += MatchKills;
	Deaths += MatchDeaths;
	BulletsFired += MatchBulletsFired;
	
	if (bIsMatchWinner)
	{
		Wins++;
	}
	else
	{
		Losses++;
	}

	bIsDirty = true;
}

void UTrueFPSPersistentUser::TellInputAboutKeybindings()
{
	TArray<APlayerController*> PlayerList;
	GEngine->GetAllLocalPlayerControllers(PlayerList);

	for (auto It = PlayerList.CreateIterator(); It; ++It)
	{
		APlayerController* PC = *It;
		if (!PC || !PC->Player || !PC->PlayerInput)
		{
			continue;
		}

		// Update key bindings for the current user only
		UTrueFPSLocalPlayer* LocalPlayer = Cast<UTrueFPSLocalPlayer>(PC->Player);
		if(!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
		{
			continue;
		}

		//set the aim sensitivity
		for (int32 Idx = 0; Idx < PC->PlayerInput->AxisMappings.Num(); Idx++)
		{
			FInputAxisKeyMapping &AxisMapping = PC->PlayerInput->AxisMappings[Idx];
			if (AxisMapping.AxisName == "Lookup" || AxisMapping.AxisName == "LookupRate" || AxisMapping.AxisName == "Turn" || AxisMapping.AxisName == "TurnRate")
			{
				AxisMapping.Scale = (AxisMapping.Scale < 0.0f) ? -GetAimSensitivity() : +GetAimSensitivity();
			}
		}
		PC->PlayerInput->ForceRebuildingKeyMaps();

		//invert it, and if does not equal our bool, invert it again
		if (PC->PlayerInput->GetInvertAxis("LookupRate") != GetInvertedYAxis())
		{
			PC->PlayerInput->InvertAxis("LookupRate");
		}

		if (PC->PlayerInput->GetInvertAxis("Lookup") != GetInvertedYAxis())
		{
			PC->PlayerInput->InvertAxis("Lookup");
		}
	}
}

int32 UTrueFPSPersistentUser::GetUserIndex() const
{
	return UserIndex;
}

void UTrueFPSPersistentUser::SetVibration(bool bVibration)
{
	bIsDirty |= bVibrationOpt != bVibration;

	bVibrationOpt = bVibration;
}

void UTrueFPSPersistentUser::SetInvertedYAxis(bool bInvert)
{
	bIsDirty |= bInvertedYAxis != bInvert;

	bInvertedYAxis = bInvert;
}

void UTrueFPSPersistentUser::SetAimSensitivity(float InSensitivity)
{
	bIsDirty |= AimSensitivity != InSensitivity;

	AimSensitivity = InSensitivity;
}

void UTrueFPSPersistentUser::SetGamma(float InGamma)
{
	bIsDirty |= Gamma != InGamma;

	Gamma = InGamma;
}

void UTrueFPSPersistentUser::SetBotsCount(int32 InCount)
{
	bIsDirty |= BotsCount != InCount;

	BotsCount = InCount;
}

void UTrueFPSPersistentUser::SetIsRecordingDemos(const bool InbIsRecordingDemos)
{
	bIsDirty |= bIsRecordingDemos != InbIsRecordingDemos;

	bIsRecordingDemos = InbIsRecordingDemos;
}

void UTrueFPSPersistentUser::SetToDefaults()
{
	bIsDirty = false;

	bVibrationOpt = true;
	bInvertedYAxis = false;
	AimSensitivity = 1.0f;
	Gamma = 2.2f;
	BotsCount = 1;
	bIsRecordingDemos = false;
}

bool UTrueFPSPersistentUser::IsAimSensitivityDirty() const
{
	bool bIsAimSensitivityDirty = false;

	// Fixme: UTrueFPSPersistentUser is not setup to work with multiple worlds.
	// For now, user settings are global to all world instances.
	if (GEngine)
	{
		TArray<APlayerController*> PlayerList;
		GEngine->GetAllLocalPlayerControllers(PlayerList);

		for (auto It = PlayerList.CreateIterator(); It; ++It)
		{
			APlayerController* PC = *It;
			if (!PC || !PC->Player || !PC->PlayerInput)
			{
				continue;
			}

			// Update key bindings for the current user only
			UTrueFPSLocalPlayer* LocalPlayer = Cast<UTrueFPSLocalPlayer>(PC->Player);
			if(!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
			{
				continue;
			}

			// check if the aim sensitivity is off anywhere
			for (int32 Idx = 0; Idx < PC->PlayerInput->AxisMappings.Num(); Idx++)
			{
				FInputAxisKeyMapping &AxisMapping = PC->PlayerInput->AxisMappings[Idx];
				if (AxisMapping.AxisName == "Lookup" || AxisMapping.AxisName == "LookupRate" || AxisMapping.AxisName == "Turn" || AxisMapping.AxisName == "TurnRate")
				{
					if (FMath::Abs(AxisMapping.Scale) != GetAimSensitivity())
					{
						bIsAimSensitivityDirty = true;
						break;
					}
				}
			}
		}
	}

	return bIsAimSensitivityDirty;
}

bool UTrueFPSPersistentUser::IsInvertedYAxisDirty() const
{
	bool bIsInvertedYAxisDirty = false;
	if (GEngine)
	{
		TArray<APlayerController*> PlayerList;
		GEngine->GetAllLocalPlayerControllers(PlayerList);

		for (auto It = PlayerList.CreateIterator(); It; ++It)
		{
			APlayerController* PC = *It;
			if (!PC || !PC->Player || !PC->PlayerInput)
			{
				continue;
			}

			// Update key bindings for the current user only
			UTrueFPSLocalPlayer* LocalPlayer = Cast<UTrueFPSLocalPlayer>(PC->Player);
			if(!LocalPlayer || LocalPlayer->GetPersistentUser() != this)
			{
				continue;
			}

			bIsInvertedYAxisDirty |= PC->PlayerInput->GetInvertAxis("Lookup") != GetInvertedYAxis();
			bIsInvertedYAxisDirty |= PC->PlayerInput->GetInvertAxis("LookupRate") != GetInvertedYAxis();
		}
	}

	return bIsInvertedYAxisDirty;
}

void UTrueFPSPersistentUser::SavePersistentUser()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName, UserIndex);
	bIsDirty = false;
}
