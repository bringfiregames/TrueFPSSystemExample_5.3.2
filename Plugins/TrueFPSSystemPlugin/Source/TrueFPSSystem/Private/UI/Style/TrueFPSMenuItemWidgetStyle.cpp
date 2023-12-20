// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSMenuItemWidgetStyle.h"

FTrueFPSMenuItemStyle::FTrueFPSMenuItemStyle()
{
}

FTrueFPSMenuItemStyle::~FTrueFPSMenuItemStyle()
{
}

const FName FTrueFPSMenuItemStyle::TypeName(TEXT("FTrueFPSMenuItemStyle"));

const FTrueFPSMenuItemStyle& FTrueFPSMenuItemStyle::GetDefault()
{
	static FTrueFPSMenuItemStyle Default;
	return Default;
}

void FTrueFPSMenuItemStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&BackgroundBrush);
	OutBrushes.Add(&LeftArrowImage);
	OutBrushes.Add(&RightArrowImage);
}


UTrueFPSMenuItemWidgetStyle::UTrueFPSMenuItemWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
