// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Sound/SlateSound.h"
#include "Styling/SlateWidgetStyle.h"
#include "Styling/SlateWidgetStyleContainerBase.h"

#include "TrueFPSOptionsWidgetStyle.generated.h"

/**
 * Represents the appearance of an FTrueFPSOptions
 */
USTRUCT()
struct FTrueFPSOptionsStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FTrueFPSOptionsStyle();
	virtual ~FTrueFPSOptionsStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FTrueFPSOptionsStyle& GetDefault();

	/**
	 * The sound the options should play when changes are accepted
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound AcceptChangesSound;
	FTrueFPSOptionsStyle& SetAcceptChangesSound(const FSlateSound& InAcceptChangesSound) { AcceptChangesSound = InAcceptChangesSound; return *this; }

	/**
	 * The sound the options should play when changes are discarded
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound DiscardChangesSound;
	FTrueFPSOptionsStyle& SetDiscardChangesSound(const FSlateSound& InDiscardChangesSound) { DiscardChangesSound = InDiscardChangesSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UTrueFPSOptionsWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the menu's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FTrueFPSOptionsStyle OptionsStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &OptionsStyle );
	}
};
