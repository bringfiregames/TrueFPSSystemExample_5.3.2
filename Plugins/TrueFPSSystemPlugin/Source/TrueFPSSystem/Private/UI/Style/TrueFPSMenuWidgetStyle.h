// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Sound/SlateSound.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateWidgetStyle.h"
#include "Styling/SlateWidgetStyleContainerBase.h"

#include "TrueFPSMenuWidgetStyle.generated.h"

/**
 * Represents the appearance of an STrueFPSMenuWidget
 */
USTRUCT()
struct FTrueFPSMenuStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FTrueFPSMenuStyle();
	virtual ~FTrueFPSMenuStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FTrueFPSMenuStyle& GetDefault();

	/**
	 * The brush used for the header background
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush HeaderBackgroundBrush;
	FTrueFPSMenuStyle& SetHeaderBackgroundBrush(const FSlateBrush& InHeaderBackgroundBrush) { HeaderBackgroundBrush = InHeaderBackgroundBrush; return *this; }

	/**
	 * The brush used for the left side of the menu
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush LeftBackgroundBrush;
	FTrueFPSMenuStyle& SetLeftBackgroundBrush(const FSlateBrush& InLeftBackgroundBrush) { LeftBackgroundBrush = InLeftBackgroundBrush; return *this; }

	/**
	 * The brush used for the right side of the menu
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush RightBackgroundBrush;
	FTrueFPSMenuStyle& SetRightBackgroundBrush(const FSlateBrush& InRightBackgroundBrush) { RightBackgroundBrush = InRightBackgroundBrush; return *this; }

	/**
	 * The sound that should play when entering a sub-menu
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound MenuEnterSound;
	FTrueFPSMenuStyle& SetMenuEnterSound(const FSlateSound& InMenuEnterSound) { MenuEnterSound = InMenuEnterSound; return *this; }

	/**
	 * The sound that should play when leaving a sub-menu
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound MenuBackSound;
	FTrueFPSMenuStyle& SetMenuBackSound(const FSlateSound& InMenuBackSound) { MenuBackSound = InMenuBackSound; return *this; }

	/**
	 * The sound that should play when changing an option
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound OptionChangeSound;
	FTrueFPSMenuStyle& SetOptionChangeSound(const FSlateSound& InOptionChangeSound) { OptionChangeSound = InOptionChangeSound; return *this; }

	/**
	 * The sound that should play when changing the selected menu item
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound MenuItemChangeSound;
	FTrueFPSMenuStyle& SetMenuItemChangeSound(const FSlateSound& InMenuItemChangeSound) { MenuItemChangeSound = InMenuItemChangeSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UTrueFPSMenuWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the menu's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FTrueFPSMenuStyle MenuStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &MenuStyle );
	}
};
