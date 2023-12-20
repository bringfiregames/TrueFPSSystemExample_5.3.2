// Fill out your copyright notice in the Description page of Project Settings.

#include "Sound/LocalPlayerSoundNode.h"
#include "SoundDefinitions.h"

#define LOCTEXT_NAMESPACE "LocalPlayerSoundNode"

TMap<uint32, bool> ULocalPlayerSoundNode::LocallyControlledActorCache;

ULocalPlayerSoundNode::ULocalPlayerSoundNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void ULocalPlayerSoundNode::ParseNodes(FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash,
	FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances)
{
	bool bLocallyControlled = false;
	if (bool* LocallyControlledPtr = LocallyControlledActorCache.Find(ActiveSound.GetOwnerID()))
	{
		bLocallyControlled = *LocallyControlledPtr;
	}

	const int32 PlayIndex = bLocallyControlled ? 0 : 1;

	if (PlayIndex < ChildNodes.Num() && ChildNodes[PlayIndex])
	{
		ChildNodes[PlayIndex]->ParseNodes(AudioDevice, GetNodeWaveInstanceHash(NodeWaveInstanceHash, ChildNodes[PlayIndex], PlayIndex), ActiveSound, ParseParams, WaveInstances);
	}
}

int32 ULocalPlayerSoundNode::GetMaxChildNodes() const
{
	return 2;
}

int32 ULocalPlayerSoundNode::GetMinChildNodes() const
{
	return 2;
}

#if WITH_EDITOR
FText ULocalPlayerSoundNode::GetInputPinName(int32 PinIndex) const
{
	return (PinIndex == 0) ? LOCTEXT("LocalPlayerLabel", "Local") : LOCTEXT("RemotePlayerLabel","Remote");
}
#endif

#undef LOCTEXT_NAMESPACE
