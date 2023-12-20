// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateBrush.h"
#include "Styling/SlateWidgetStyle.h"
#include "Styling/SlateWidgetStyleContainerBase.h"

#include "TrueFPSMenuItemWidgetStyle.generated.h"

/**
 * Represents the appearance of an FTrueFPSMenuItem
 */
USTRUCT()
struct FTrueFPSMenuItemStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FTrueFPSMenuItemStyle();
	virtual ~FTrueFPSMenuItemStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FTrueFPSMenuItemStyle& GetDefault();

	/**
	 * The brush used for the item background
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackgroundBrush;
	FTrueFPSMenuItemStyle& SetBackgroundBrush(const FSlateBrush& InBackgroundBrush) { BackgroundBrush = InBackgroundBrush; return *this; }

	/**
	 * The image used for the left arrow
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush LeftArrowImage;
	FTrueFPSMenuItemStyle& SetLeftArrowImage(const FSlateBrush& InLeftArrowImage) { LeftArrowImage = InLeftArrowImage; return *this; }

	/**
	 * The image used for the right arrow
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush RightArrowImage;
	FTrueFPSMenuItemStyle& SetRightArrowImage(const FSlateBrush& InRightArrowImage) { RightArrowImage = InRightArrowImage; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UTrueFPSMenuItemWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the menu's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FTrueFPSMenuItemStyle MenuItemStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &MenuItemStyle );
	}
};
