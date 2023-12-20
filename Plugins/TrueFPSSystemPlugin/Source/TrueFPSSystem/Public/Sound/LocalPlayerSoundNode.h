// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundNode.h"
#include "LocalPlayerSoundNode.generated.h"

/**
 * 
 */
UCLASS(HideCategories=Object, EditInlineNew)
class TRUEFPSSYSTEM_API ULocalPlayerSoundNode : public USoundNode
{
	GENERATED_BODY()

public:

	ULocalPlayerSoundNode(const FObjectInitializer& ObjectInitializer);
	
	virtual void ParseNodes(FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound,
		const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances) override;

	virtual int32 GetMaxChildNodes() const override;
	virtual int32 GetMinChildNodes() const override;

#if WITH_EDITOR
	virtual FText GetInputPinName(int32 PinIndex) const override;
#endif

	static TMap<uint32, bool>& GetLocallyControlledActorCache()
	{
		check(IsInAudioThread());
		return LocallyControlledActorCache;
	}

private:

	static TMap<uint32, bool> LocallyControlledActorCache;
	
};
