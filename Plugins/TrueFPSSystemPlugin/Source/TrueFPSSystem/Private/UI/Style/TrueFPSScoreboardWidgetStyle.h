// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Sound/SlateSound.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateWidgetStyle.h"
#include "Styling/SlateWidgetStyleContainerBase.h"

#include "TrueFPSScoreboardWidgetStyle.generated.h"

/**
 * Represents the appearance of an STrueFPSScoreboardWidget
 */
USTRUCT()
struct FTrueFPSScoreboardStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FTrueFPSScoreboardStyle();
	virtual ~FTrueFPSScoreboardStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FTrueFPSScoreboardStyle& GetDefault();

	/**
	 * The brush used for the item border
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush ItemBorderBrush;
	FTrueFPSScoreboardStyle& SetItemBorderBrush(const FSlateBrush& InItemBorderBrush) { ItemBorderBrush = InItemBorderBrush; return *this; }

	/**
	 * The color used for the kill stat
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor KillStatColor;
	FTrueFPSScoreboardStyle& SetKillStatColor(const FSlateColor& InKillStatColor) { KillStatColor = InKillStatColor; return *this; }

	/**
	 * The color used for the death stat
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor DeathStatColor;
	FTrueFPSScoreboardStyle& SetDeathStatColor(const FSlateColor& InDeathStatColor) { DeathStatColor = InDeathStatColor; return *this; }

	/**
	 * The color used for the score stat
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ScoreStatColor;
	FTrueFPSScoreboardStyle& SetScoreStatColor(const FSlateColor& InScoreStatColor) { ScoreStatColor = InScoreStatColor; return *this; }

	/**
	 * The sound that should play when the highlighted player changes in the scoreboard
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound PlayerChangeSound;
	FTrueFPSScoreboardStyle& SetPlayerChangeSound(const FSlateSound& InPlayerChangeSound) { PlayerChangeSound = InPlayerChangeSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UTrueFPSScoreboardWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the scoreboard's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FTrueFPSScoreboardStyle ScoreboardStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &ScoreboardStyle );
	}
};
