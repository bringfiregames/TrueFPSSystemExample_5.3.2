// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrueFPSStyle.h"
#include "Slate/SlateGameResources.h"

TSharedPtr< FSlateStyleSet > FTrueFPSStyle::TrueFPSStyleInstance = nullptr;

void FTrueFPSStyle::Initialize()
{
	if ( !TrueFPSStyleInstance.IsValid() )
	{
		TrueFPSStyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle( *TrueFPSStyleInstance );
	}
}

void FTrueFPSStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle( *TrueFPSStyleInstance );
	ensure( TrueFPSStyleInstance.IsUnique() );
	TrueFPSStyleInstance.Reset();
}

FName FTrueFPSStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("TrueFPSStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( FPaths::ProjectContentDir() / "Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( FPaths::ProjectContentDir() / "Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( FPaths::ProjectContentDir() / "Slate"/ RelativePath + TEXT(".png"), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( FPaths::ProjectContentDir() / "Slate"/ RelativePath + TEXT(".ttf"), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( FPaths::ProjectContentDir() / "Slate"/ RelativePath + TEXT(".otf"), __VA_ARGS__ )

PRAGMA_DISABLE_OPTIMIZATION
TSharedRef< FSlateStyleSet > FTrueFPSStyle::Create()
{
	TSharedRef<FSlateStyleSet> StyleRef = FSlateGameResources::New(FTrueFPSStyle::GetStyleSetName(), "/TrueFPSSystemPlugin/UI/Styles", "/TrueFPSSystemPlugin/UI/Styles");
	FSlateStyleSet& Style = StyleRef.Get();

	// Load the speaker icon to be used for displaying when a user is talking
	Style.Set("TrueFPSSystem.Speaker", new IMAGE_BRUSH("Images/SoundCue_SpeakerIcon", FVector2D(32, 32)));

	// The border image used to draw the replay timeline bar
	Style.Set("TrueFPSSystem.ReplayTimelineBorder", new BOX_BRUSH("Images/ReplayTimeline", FMargin(3.0f / 8.0f)));

	// The border image used to draw the replay timeline bar
	Style.Set("TrueFPSSystem.ReplayTimelineIndicator", new IMAGE_BRUSH("Images/ReplayTimelineIndicator", FVector2D(4.0f, 26.0f)));

	// The image used to draw the replay pause button
	Style.Set("TrueFPSSystem.ReplayPauseIcon", new IMAGE_BRUSH("Images/ReplayPause", FVector2D(32.0f, 32.0f)));

	// Fonts still need to be specified in code for now
	Style.Set("TrueFPSSystem.MenuServerListTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Black", 14))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FIntPoint(-1,1))
		);

	Style.Set("TrueFPSSystem.MenuStoreListTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Black", 14))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FIntPoint(-1, 1))
	);

	Style.Set("TrueFPSSystem.ScoreboardListTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Black", 14))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FIntPoint(-1,1))
		);

	Style.Set("TrueFPSSystem.MenuProfileNameStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Black", 18))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FIntPoint(-1,1))
		);

	Style.Set("TrueFPSSystem.MenuTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Black", 20))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FIntPoint(-1,1))
		);

	Style.Set("TrueFPSSystem.MenuHeaderTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Black", 26))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FIntPoint(-1,1))
		);

	Style.Set("TrueFPSSystem.WelcomeScreen.WelcomeTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Medium", 32))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FIntPoint(-1,1))
		);

	Style.Set("TrueFPSSystem.DefaultScoreboard.Row.HeaderTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Black", 24))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D(0,1))
		);

	Style.Set("TrueFPSSystem.DefaultScoreboard.Row.StatTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Regular", 18))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D(0,1))
		);

	Style.Set("TrueFPSSystem.SplitScreenLobby.StartMatchTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Regular", 16))
		.SetColorAndOpacity(FLinearColor::Green)
		.SetShadowOffset(FVector2D(0,1))
		);

	Style.Set("TrueFPSSystem.DemoListCheckboxTextStyle", FTextBlockStyle()
		.SetFont(TTF_FONT("Fonts/Roboto-Black", 12))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FIntPoint(-1,1))
		);

	Style.Set("TrueFPSSystem.Switch.Left", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("Images/SwitchButtonLeft", FVector2D(32, 32)))
		);

	Style.Set("TrueFPSSystem.Switch.Right", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("Images/SwitchButtonRight", FVector2D(32, 32)))
		);

	Style.Set("TrueFPSSystem.Switch.Up", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("Images/SwitchButtonUp", FVector2D(32, 32)))
		);

	Style.Set("TrueFPSSystem.Switch.Down", FInlineTextImageStyle()
		.SetImage(IMAGE_BRUSH("Images/SwitchButtonDown", FVector2D(32, 32)))
		);

	return StyleRef;
}
PRAGMA_ENABLE_OPTIMIZATION

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FTrueFPSStyle::ReloadTextures()
{
	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FTrueFPSStyle::Get()
{
	return *TrueFPSStyleInstance;
}
