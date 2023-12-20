// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TrueFPSTypes.h"
#include "Engine/GameViewportClient.h"
#include "TrueFPSGameViewportClient.generated.h"

class STrueFPSConfirmationDialog;

struct FTrueFPSGameLoadingScreenBrush : public FSlateDynamicImageBrush, public FGCObject
{
	FTrueFPSGameLoadingScreenBrush( const FName InTextureName, const FVector2D& InImageSize )
		: FSlateDynamicImageBrush( InTextureName, InImageSize )
	{
		SetResourceObject(LoadObject<UObject>( nullptr, *InTextureName.ToString() ));
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector)
	{
		FSlateBrush::AddReferencedObjects(Collector);
	}

	virtual FString GetReferencerName() const override
	{
		return TEXT("FTrueFPSGameLoadingScreenBrush");
	}

};

class STrueFPSLoadingScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STrueFPSLoadingScreen) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	EVisibility GetLoadIndicatorVisibility() const
	{
		return EVisibility::Visible;
	}

	/** loading screen image brush */
	TSharedPtr<FSlateDynamicImageBrush> LoadingScreenBrush;
};

UCLASS(Within=Engine, transient, config=Engine)
class TRUEFPSSYSTEM_API UTrueFPSGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()
	
public:

	UTrueFPSGameViewportClient(const FObjectInitializer& ObjectInitializer);
	
	// start UGameViewportClient interface
	void NotifyPlayerAdded( int32 PlayerIndex, ULocalPlayer* AddedPlayer ) override;
	void AddViewportWidgetContent( TSharedRef<class SWidget> ViewportContent, const int32 ZOrder = 0 ) override;
	void RemoveViewportWidgetContent( TSharedRef<class SWidget> ViewportContent ) override;

	void ShowDialog(TWeakObjectPtr<ULocalPlayer> PlayerOwner, ETrueFPSDialogType::Type DialogType, const FText& Message, const FText& Confirm, const FText& Cancel, const FOnClicked& OnConfirm, const FOnClicked& OnCancel);
	void HideDialog();

	void ShowLoadingScreen();
	void HideLoadingScreen();

	bool IsShowingDialog() const { return DialogWidget.IsValid(); }

	ETrueFPSDialogType::Type GetDialogType() const;
	TWeakObjectPtr<ULocalPlayer> GetDialogOwner() const;

	TSharedPtr<STrueFPSConfirmationDialog> GetDialogWidget() { return DialogWidget; }

	//FTicker Funcs
	virtual void Tick(float DeltaSeconds) override;	

	virtual	void BeginDestroy() override;
	virtual void DetachViewportClient() override;
	void ReleaseSlateResources();

#if WITH_EDITOR
	virtual void DrawTransition(class UCanvas* Canvas) override;
#endif //WITH_EDITOR
	// end UGameViewportClient interface

	protected:
	void HideExistingWidgets();
	void ShowExistingWidgets();

	/** List of viewport content that the viewport is tracking */
	TArray<TSharedRef<class SWidget>>				ViewportContentStack;

	TArray<TSharedRef<class SWidget>>				HiddenViewportContentStack;

	TSharedPtr<class SWidget>						OldFocusWidget;

	/** Dialog widget to show temporary messages ("Controller disconnected", "Parental Controls don't allow you to play online", etc) */
	TSharedPtr<STrueFPSConfirmationDialog>			DialogWidget;

	TSharedPtr<STrueFPSLoadingScreen>				LoadingScreenWidget;
	
};
