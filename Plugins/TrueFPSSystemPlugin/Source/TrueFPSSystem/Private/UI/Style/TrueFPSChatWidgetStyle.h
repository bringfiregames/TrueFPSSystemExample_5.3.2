// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateBrush.h"
#include "Styling/SlateWidgetStyle.h"
#include "Styling/SlateWidgetStyleContainerBase.h"

#include "TrueFPSChatWidgetStyle.generated.h"

/**
 * Represents the appearance of an SChatWidget
 */
USTRUCT()
struct FTrueFPSChatStyle : public FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	FTrueFPSChatStyle();
	virtual ~FTrueFPSChatStyle();

	// FSlateWidgetStyle
	virtual void GetResources(TArray<const FSlateBrush*>& OutBrushes) const override;
	static const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };
	static const FTrueFPSChatStyle& GetDefault();

	/**
	 * The style used for entering chat text
	 */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FEditableTextBoxStyle TextEntryStyle;
	FTrueFPSChatStyle& SetChatEntryStyle(const FEditableTextBoxStyle& InTextEntryStyle) { TextEntryStyle = InTextEntryStyle; return *this; }

	/**
	 * The brush used for the chat backing
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateBrush BackingBrush;
	FTrueFPSChatStyle& SetBackingBrush(const FSlateBrush& InBackingBrush) { BackingBrush = InBackingBrush; return *this; }

	/**
	 * The color used for the chat box border
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor BoxBorderColor;
	FTrueFPSChatStyle& SetBoxBorderColor(const FSlateColor& InBoxBorderColor) { BoxBorderColor = InBoxBorderColor; return *this; }

	/**
	 * The color used for the chat box text
	 */	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor TextColor;
	FTrueFPSChatStyle& SetTextColor(const FSlateColor& InTextColor) { TextColor = InTextColor; return *this; }

	/**
	 * The sound that should play when receiving a chat message
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound RxMessgeSound;
	FTrueFPSChatStyle& SetRxMessgeSound(const FSlateSound& InRxMessgeSound) { RxMessgeSound = InRxMessgeSound; return *this; }

	/**
	 * The sound that should play when sending a chat message
	 */	
	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound TxMessgeSound;
	FTrueFPSChatStyle& SetTxMessgeSound(const FSlateSound& InTxMessgeSound) { TxMessgeSound = InTxMessgeSound; return *this; }
};


/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UTrueFPSChatWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the chat appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FTrueFPSChatStyle ChatStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast< const struct FSlateWidgetStyle* >( &ChatStyle );
	}
};
