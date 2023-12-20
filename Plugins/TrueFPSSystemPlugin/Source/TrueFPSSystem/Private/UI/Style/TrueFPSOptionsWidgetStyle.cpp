// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSOptionsWidgetStyle.h"

FTrueFPSOptionsStyle::FTrueFPSOptionsStyle()
{
}

FTrueFPSOptionsStyle::~FTrueFPSOptionsStyle()
{
}

const FName FTrueFPSOptionsStyle::TypeName(TEXT("FTrueFPSOptionsStyle"));

const FTrueFPSOptionsStyle& FTrueFPSOptionsStyle::GetDefault()
{
	static FTrueFPSOptionsStyle Default;
	return Default;
}

void FTrueFPSOptionsStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
}


UTrueFPSOptionsWidgetStyle::UTrueFPSOptionsWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
