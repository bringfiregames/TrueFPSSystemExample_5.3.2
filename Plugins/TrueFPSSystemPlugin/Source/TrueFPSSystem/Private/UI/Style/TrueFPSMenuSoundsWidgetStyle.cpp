// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSMenuSoundsWidgetStyle.h"

FTrueFPSMenuSoundsStyle::FTrueFPSMenuSoundsStyle()
{
}

FTrueFPSMenuSoundsStyle::~FTrueFPSMenuSoundsStyle()
{
}

const FName FTrueFPSMenuSoundsStyle::TypeName(TEXT("FTrueFPSMenuSoundsStyle"));

const FTrueFPSMenuSoundsStyle& FTrueFPSMenuSoundsStyle::GetDefault()
{
	static FTrueFPSMenuSoundsStyle Default;
	return Default;
}

void FTrueFPSMenuSoundsStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
}


UTrueFPSMenuSoundsWidgetStyle::UTrueFPSMenuSoundsWidgetStyle( const FObjectInitializer& ObjectInitializer ) : Super(ObjectInitializer)
{
}
