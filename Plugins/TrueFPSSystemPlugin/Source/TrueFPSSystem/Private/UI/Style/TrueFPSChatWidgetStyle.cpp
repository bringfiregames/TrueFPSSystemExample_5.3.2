// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSChatWidgetStyle.h"

FTrueFPSChatStyle::FTrueFPSChatStyle()
{
}

FTrueFPSChatStyle::~FTrueFPSChatStyle()
{
}

const FName FTrueFPSChatStyle::TypeName(TEXT("FTrueFPSChatStyle"));

const FTrueFPSChatStyle& FTrueFPSChatStyle::GetDefault()
{
	static FTrueFPSChatStyle Default;
	return Default;
}

void FTrueFPSChatStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	TextEntryStyle.GetResources(OutBrushes);

	OutBrushes.Add(&BackingBrush);
}


UTrueFPSChatWidgetStyle::UTrueFPSChatWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
