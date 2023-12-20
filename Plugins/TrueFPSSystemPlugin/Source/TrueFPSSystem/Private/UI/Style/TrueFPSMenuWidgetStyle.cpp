// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSMenuWidgetStyle.h"

FTrueFPSMenuStyle::FTrueFPSMenuStyle()
{
}

FTrueFPSMenuStyle::~FTrueFPSMenuStyle()
{
}

const FName FTrueFPSMenuStyle::TypeName(TEXT("FTrueFPSMenuStyle"));

const FTrueFPSMenuStyle& FTrueFPSMenuStyle::GetDefault()
{
	static FTrueFPSMenuStyle Default;
	return Default;
}

void FTrueFPSMenuStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&HeaderBackgroundBrush);
	OutBrushes.Add(&LeftBackgroundBrush);
	OutBrushes.Add(&RightBackgroundBrush);
}


UTrueFPSMenuWidgetStyle::UTrueFPSMenuWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
