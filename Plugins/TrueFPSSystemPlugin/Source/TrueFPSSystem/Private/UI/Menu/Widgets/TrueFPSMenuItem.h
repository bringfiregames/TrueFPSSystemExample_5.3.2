// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SlateExtras.h"
#include "STrueFPSMenuItem.h"

namespace ETrueFPSMenuItemType
{
	enum Type
	{
		Root,
		Standard,
		MultiChoice,
		CustomWidget,
	};
};

/** TArray< TSharedPtr<class FTrueFPSMenuItem> > */
typedef TArray< TSharedPtr<class FTrueFPSMenuItem> > MenuPtr;

class FTrueFPSMenuInfo 
{
public:
	/** menu items array */
	MenuPtr Menu;

	/** last selection in this menu */
	int32 SelectedIndex;

	/** menu title */
	FText MenuTitle;

	/** constructor making filling required information easy */
	FTrueFPSMenuInfo(MenuPtr _Menu, int32 _SelectedIndex, FText _MenuTitle)
	{
		Menu = _Menu;
		SelectedIndex = _SelectedIndex;
		MenuTitle = MoveTemp(_MenuTitle);
	}
};

class FTrueFPSMenuItem : public TSharedFromThis<FTrueFPSMenuItem>
{
public:
	/** confirm menu item delegate */
	DECLARE_DELEGATE(FOnConfirmMenuItem);

	/** view profile delegate */
	DECLARE_DELEGATE(FOnControllerFacebuttonLeftPressed);

	/** Increment friend index counter delegate */
	DECLARE_DELEGATE(FOnControllerDownInputPressed);

	/** Decrement friend index counter delegate */
	DECLARE_DELEGATE(FOnControllerUpInputPressed);

	/** Send friend invite delegate */
	DECLARE_DELEGATE(FOnOnControllerFacebuttonDownPressed);
	
	/** multi-choice option changed, parameters are menu item itself and new multi-choice index  */
	DECLARE_DELEGATE_TwoParams(FOnOptionChanged, TSharedPtr<FTrueFPSMenuItem>, int32);

	/** delegate, which is executed by STrueFPSMenuWidget if user confirms this menu item */
	FOnConfirmMenuItem OnConfirmMenuItem;

	/** multi-choice option changed, parameters are menu item itself and new multi-choice index */
	FOnOptionChanged OnOptionChanged;

	/** delegate, which is executed by STrueFPSMenuWidget if user presses FacebuttonLeft */
	FOnControllerFacebuttonLeftPressed OnControllerFacebuttonLeftPressed;

	/** delegate, which is executed by STrueFPSMenuWidget if user presses ControllerDownInput */
	FOnControllerDownInputPressed OnControllerDownInputPressed;

	/** delegate, which is executed by STrueFPSMenuWidget if user presses ControllerUpInput */
	FOnControllerUpInputPressed OnControllerUpInputPressed;

	/** delegate, which is executed by STrueFPSMenuWidget if user presses FacebuttonDown */
	FOnOnControllerFacebuttonDownPressed OnControllerFacebuttonDownPressed;

	/** menu item type */
	ETrueFPSMenuItemType::Type MenuItemType;	

	/** if this menu item will be created when menu is opened */
	bool bVisible;

	/** sub menu if present */
	TArray< TSharedPtr<FTrueFPSMenuItem> > SubMenu;

	/** shared pointer to actual slate widget representing the menu item */
	TSharedPtr<STrueFPSMenuItem> Widget;

	/** shared pointer to actual slate widget representing the custom menu item, ie whole options screen */
	TSharedPtr<SWidget> CustomWidget;

	/** texts for multiple choice menu item (like INF AMMO ON/OFF or difficulty/resolution etc) */
	TArray<FText> MultiChoice;

	/** set to other value than -1 to limit the options range */
	int32 MinMultiChoiceIndex;

	/** set to other value than -1 to limit the options range */
	int32 MaxMultiChoiceIndex;

	/** selected multi-choice index for this menu item */
	int32 SelectedMultiChoice;

	/** constructor accepting menu item text */
	FTrueFPSMenuItem(FText _text)
	{
		bVisible = true;
		Text = MoveTemp(_text);
		MenuItemType = ETrueFPSMenuItemType::Standard;
	}

	/** custom widgets cannot contain sub menus, all functionality must be handled by custom widget itself */
	FTrueFPSMenuItem(TSharedPtr<SWidget> _Widget)
	{
		bVisible = true;
		MenuItemType = ETrueFPSMenuItemType::CustomWidget;
		CustomWidget = _Widget;
	}

	/** constructor for multi-choice item */
	FTrueFPSMenuItem(FText _text, TArray<FText> _choices, int32 DefaultIndex=0)
	{
		bVisible = true;
		Text = MoveTemp(_text);
		MenuItemType = ETrueFPSMenuItemType::MultiChoice;
		MultiChoice = MoveTemp(_choices);
		MinMultiChoiceIndex = MaxMultiChoiceIndex = -1;
		SelectedMultiChoice = DefaultIndex;
	}

	const FText& GetText() const
	{
		return Text;
	}

	void SetText(FText UpdatedText)
	{
		Text = MoveTemp(UpdatedText);
		if (Widget.IsValid())
		{
			Widget->UpdateItemText(Text);
		}
	}

	/** create special root item */
	static TSharedRef<FTrueFPSMenuItem> CreateRoot()
	{
		return MakeShareable(new FTrueFPSMenuItem());
	}

private:

	/** menu item text */
	FText Text;

	FTrueFPSMenuItem()
	{
		bVisible = false;
		MenuItemType = ETrueFPSMenuItemType::Root;
	}
};