// Copyright Epic Games, Inc. All Rights Reserved.

#include "STrueFPSDemoHUD.h"

#include "UI/Style/TrueFPSStyle.h"
#include "Engine/DemoNetDriver.h"

/** Widget to represent the main replay timeline bar */
class STrueFPSReplayTimeline : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STrueFPSReplayTimeline)
		: _DemoDriver(nullptr)
		, _BackgroundBrush( FCoreStyle::Get().GetDefaultBrush() )
		, _IndicatorBrush( FCoreStyle::Get().GetDefaultBrush() )
		{}
	SLATE_ARGUMENT(TWeakObjectPtr<UDemoNetDriver>, DemoDriver)
	SLATE_ATTRIBUTE( FMargin, BackgroundPadding )
	SLATE_ATTRIBUTE( const FSlateBrush*, BackgroundBrush )
	SLATE_ATTRIBUTE( const FSlateBrush*, IndicatorBrush )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	/** Jumps the replay to the time on the bar that was clicked */
	FReply OnTimelineClicked(const FGeometry& Geometry, const FPointerEvent& Event);

	/** The demo net driver underlying the current replay */
	TWeakObjectPtr<UDemoNetDriver> DemoDriver;

	/** The FName of the image resource to show */
	TAttribute< const FSlateBrush* > BackgroundBrush;

	/** The FName of the image resource to show */
	TAttribute< const FSlateBrush* > IndicatorBrush;
};

void STrueFPSReplayTimeline::Construct(const FArguments& InArgs)
{
	DemoDriver = InArgs._DemoDriver;
	BackgroundBrush = InArgs._BackgroundBrush;
	IndicatorBrush = InArgs._IndicatorBrush;

	ChildSlot
	.Padding(InArgs._BackgroundPadding)
	[
		SNew(SBorder)
		.BorderImage(BackgroundBrush)
		.OnMouseButtonDown(this, &STrueFPSReplayTimeline::OnTimelineClicked)
	];
}

FVector2D STrueFPSReplayTimeline::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	// Just use the size of the indicator image here
	const FSlateBrush* ImageBrush = IndicatorBrush.Get();
	if (ImageBrush != nullptr)
	{
		return ImageBrush->ImageSize;
	}
	return FVector2D::ZeroVector;
}

int32 STrueFPSReplayTimeline::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 ParentLayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// Manually draw the position indicator
	const FSlateBrush* ImageBrush = IndicatorBrush.Get();
	if ((ImageBrush != nullptr) && (ImageBrush->DrawAs != ESlateBrushDrawType::NoDrawType) && DemoDriver.IsValid())
	{
		const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
		const ESlateDrawEffect DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		const FLinearColor FinalColorAndOpacity( InWidgetStyle.GetColorAndOpacityTint() * ColorAndOpacity.Get() * ImageBrush->GetTint( InWidgetStyle ) );

		// Adjust clipping rect to replay time
		const float ReplayPercent = DemoDriver->GetDemoCurrentTime() / DemoDriver->GetDemoTotalTime();

		const FVector2D Center(
			AllottedGeometry.GetLocalSize().X * ReplayPercent,
			AllottedGeometry.GetLocalSize().Y * 0.5f
		);
		
		const FVector2D Offset = Center - ImageBrush->ImageSize * 0.5f;

		const int32 IndicatorLayerId = ParentLayerId + 1;
			
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			IndicatorLayerId,
			AllottedGeometry.ToPaintGeometry(Offset, ImageBrush->ImageSize),
			ImageBrush,
			DrawEffects,
			FinalColorAndOpacity
		);

		return IndicatorLayerId;
	}

	return ParentLayerId;
}

FReply STrueFPSReplayTimeline::OnTimelineClicked(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (DemoDriver.IsValid())
	{
		const FVector2D LocalPos = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());

		const float TimelinePercentage = LocalPos.X / Geometry.GetLocalSize().X;

		DemoDriver->GotoTimeInSeconds( TimelinePercentage * DemoDriver->GetDemoTotalTime() );

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void STrueFPSDemoHUD::Construct(const FArguments& InArgs)
{	
	PlayerOwner = InArgs._PlayerOwner;
	check(PlayerOwner.IsValid());

	ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.FillHeight(8.5f)
		.VAlign(VAlign_Bottom)
		[		
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.5f)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Margin(3.0f)
				.Text(this, &STrueFPSDemoHUD::GetCurrentReplayTime)
			]

			+SHorizontalBox::Slot()
			.FillWidth(7.0f)
			[

				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SNew(STrueFPSReplayTimeline)
					.DemoDriver(PlayerOwner->GetWorld()->GetDemoNetDriver())
					.BackgroundBrush(FTrueFPSStyle::Get().GetBrush("TrueFPSSystem.ReplayTimelineBorder"))
					.BackgroundPadding(FMargin(0.0f, 3.0))
					.IndicatorBrush(FTrueFPSStyle::Get().GetBrush("TrueFPSSystem.ReplayTimelineIndicator"))
				]

				+SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Visibility(EVisibility::HitTestInvisible)
					.Margin(3.0f)
					.Text(this, &STrueFPSDemoHUD::GetPlaybackSpeed)
				]
			]

			+SHorizontalBox::Slot()
			.FillWidth(1.5f)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Margin(3.0f)
				.Text(this, &STrueFPSDemoHUD::GetTotalReplayTime)
			]
		]

		+SVerticalBox::Slot()
		.FillHeight(1.5f)
		.HAlign(HAlign_Center)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.Padding(FMargin(6.0))
			.AutoHeight()
			[
				SNew(SCheckBox)
				.IsFocusable(false)
				.Style(FCoreStyle::Get(), "ToggleButtonCheckbox")
				.IsChecked(this, &STrueFPSDemoHUD::IsPauseChecked)
				.OnCheckStateChanged(this, &STrueFPSDemoHUD::OnPauseCheckStateChanged)
				[
					SNew(SImage)
					.Image(FTrueFPSStyle::Get().GetBrush("TrueFPSSystem.ReplayPauseIcon"))
				]
			]

			+SVerticalBox::Slot()
		]
	];
}

FText STrueFPSDemoHUD::GetCurrentReplayTime() const
{
	if (!PlayerOwner.IsValid())
	{
		return FText::GetEmpty();
	}

	UDemoNetDriver* DemoDriver = PlayerOwner->GetWorld()->GetDemoNetDriver();

	if (DemoDriver == nullptr)
	{
		return FText::GetEmpty();
	}

	return FText::AsTimespan(FTimespan::FromSeconds(DemoDriver->GetDemoCurrentTime()));
}

FText STrueFPSDemoHUD::GetTotalReplayTime() const
{
	if (!PlayerOwner.IsValid())
	{
		return FText::GetEmpty();
	}

	UDemoNetDriver* DemoDriver = PlayerOwner->GetWorld()->GetDemoNetDriver();

	if (DemoDriver == nullptr)
	{
		return FText::GetEmpty();
	}

	return FText::AsTimespan(FTimespan::FromSeconds(DemoDriver->GetDemoTotalTime()));
}

FText STrueFPSDemoHUD::GetPlaybackSpeed() const
{
	if (!PlayerOwner.IsValid() || PlayerOwner->GetWorldSettings() == nullptr)
	{
		return FText::GetEmpty();
	}

	if (PlayerOwner->GetWorldSettings()->GetPauserPlayerState() == nullptr)
	{
		FNumberFormattingOptions FormatOptions = FNumberFormattingOptions()
			.SetMinimumFractionalDigits(2)
			.SetMaximumFractionalDigits(2);

		return FText::Format(NSLOCTEXT("TrueFPSSystem.HUD.Menu", "PlaybackSpeed", "{0} X"), FText::AsNumber(PlayerOwner->GetWorldSettings()->DemoPlayTimeDilation, &FormatOptions));
	}

	return NSLOCTEXT("TrueFPSSystem.HUD.Menu", "Paused", "PAUSED");
}

ECheckBoxState STrueFPSDemoHUD::IsPauseChecked() const
{
	if (PlayerOwner.IsValid() && PlayerOwner->GetWorldSettings() != nullptr && PlayerOwner->GetWorldSettings()->GetPauserPlayerState() != nullptr)
	{
		return ECheckBoxState::Checked;
	}

	return ECheckBoxState::Unchecked;
}

void STrueFPSDemoHUD::OnPauseCheckStateChanged(ECheckBoxState CheckState) const
{
	if (!PlayerOwner.IsValid())
	{
		return;
	}

	AWorldSettings* WorldSettings = PlayerOwner->GetWorldSettings();

	if (WorldSettings == nullptr)
	{
		return;
	}

	switch(CheckState)
	{
		case ECheckBoxState::Checked:
		{
			WorldSettings->SetPauserPlayerState(PlayerOwner->PlayerState);
			break;
		}

		case ECheckBoxState::Unchecked:
		{
			WorldSettings->SetPauserPlayerState(nullptr);
			break;
		}

		case ECheckBoxState::Undetermined:
		{
			break;
		}
	}
}
