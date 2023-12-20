// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSScoreboardWidgetStyle.h"

FTrueFPSScoreboardStyle::FTrueFPSScoreboardStyle()
{
}

FTrueFPSScoreboardStyle::~FTrueFPSScoreboardStyle()
{
}

const FName FTrueFPSScoreboardStyle::TypeName(TEXT("FTrueFPSScoreboardStyle"));

const FTrueFPSScoreboardStyle& FTrueFPSScoreboardStyle::GetDefault()
{
	static FTrueFPSScoreboardStyle Default;
	return Default;
}

void FTrueFPSScoreboardStyle::GetResources(TArray<const FSlateBrush*>& OutBrushes) const
{
	OutBrushes.Add(&ItemBorderBrush);
}


UTrueFPSScoreboardWidgetStyle::UTrueFPSScoreboardWidgetStyle( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	
}
