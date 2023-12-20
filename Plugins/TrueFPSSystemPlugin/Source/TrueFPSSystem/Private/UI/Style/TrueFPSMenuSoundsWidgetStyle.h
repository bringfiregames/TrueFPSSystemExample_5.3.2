// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Sound/SlateSound.h"
#include "Styling/SlateWidgetStyle.h"
#include "Styling/SlateWidgetStyleContainerBase.h"

#include "TrueFPSMenuSoundsWidgetStyle.generated.h"

/**
 * Represents the common menu sounds used in the TrueFPS game
 */
USTRUCT()
struct FTrueFPSMenuSoundsStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FTrueFPSMenuSoundsStyle();
	virtual ~FTrueFPSMenuSoundsStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FTrueFPSMenuSoundsStyle& GetDefault();

	/**
	 * The sound that should play when starting the game
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound StartGameSound;
	FTrueFPSMenuSoundsStyle& SetStartGameSound(const FSlateSound& InStartGameSound) { StartGameSound = InStartGameSound; return *this; }

	/**
	 * The sound that should play when exiting the game
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound ExitGameSound;
	FTrueFPSMenuSoundsStyle& SetExitGameSound(const FSlateSound& InExitGameSound) { ExitGameSound = InExitGameSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UTrueFPSMenuSoundsWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the sounds */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FTrueFPSMenuSoundsStyle SoundsStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &SoundsStyle );
	}
};
