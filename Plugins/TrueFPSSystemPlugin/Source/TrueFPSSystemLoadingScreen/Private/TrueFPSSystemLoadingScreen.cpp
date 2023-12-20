
#include "TrueFPSSystemLoadingScreen.h"
#include "MoviePlayer.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SSafeZone.h"

// This module must be loaded "PreLoadingScreen" in the .uproject file, otherwise it will not hook in time!
FLinearColor Tint = FLinearColor::White;
ESlateBrushTileType::Type Tiling = ESlateBrushTileType::NoTile;
ESlateBrushImageType::Type ImageType = ESlateBrushImageType::FullColor;

struct FTrueFPSSystemLoadingScreenBrush : public FSlateDynamicImageBrush, public FGCObject
{
	FTrueFPSSystemLoadingScreenBrush( const FName InTextureName, const FVector2D& InImageSize,const FLinearColor& InTint = FLinearColor::White,  // Default value for tint
									 ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile,  // Default tiling type
									 ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor ) // Default image type
		: FSlateDynamicImageBrush( InTextureName, InImageSize, InTint, InTiling, InImageType )
	{
		SetResourceObject(LoadObject<UObject>( nullptr, *InTextureName.ToString() ));
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector)
	{
		FSlateBrush::AddReferencedObjects(Collector);
	}

	virtual FString GetReferencerName() const override
	{
		return TEXT("FTrueFPSSystemLoadingScreenBrush");
	}
};

class STrueFPSLoadingScreen2 : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STrueFPSLoadingScreen2) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		static const FName LoadingScreenName(TEXT("/TrueFPSSystemPlugin/UI/Menu/LoadingScreen.LoadingScreen"));

		//since we are not using game styles here, just load one image
		LoadingScreenBrush = MakeShareable(
			new FTrueFPSSystemLoadingScreenBrush(LoadingScreenName, FVector2D(1920, 1080), Tint, Tiling, ImageType));

		ChildSlot
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SImage)
				.Image(LoadingScreenBrush.Get())
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SSafeZone)
				.VAlign(VAlign_Bottom)
				.HAlign(HAlign_Right)
				.Padding(10.0f)
				.IsTitleSafe(true)
				[
					SNew(SThrobber)
					.Visibility(this, &STrueFPSLoadingScreen2::GetLoadIndicatorVisibility)
				]
			]
		];
	}

private:
	EVisibility GetLoadIndicatorVisibility() const
	{
		return EVisibility::Visible;
	}

	/** loading screen image brush */
	TSharedPtr<FSlateDynamicImageBrush> LoadingScreenBrush;
};

class FTrueFPSSystemLoadingScreenModule : public ITrueFPSSystemLoadingScreenModule
{
public:
	virtual void StartupModule() override
	{		
		// Load for cooker reference
		LoadObject<UObject>(nullptr, TEXT("/TrueFPSSystemPlugin/UI/Menu/LoadingScreen.LoadingScreen") );


		// Previously, we set up our startup movie here to play while the engine was initially loading. By removing this behavior, 
		// the startup movie can be set up in DefaultGame.ini or in the project settings, and is no longer hard coded

		/*
		if (IsMoviePlayerEnabled())
		{
			FLoadingScreenAttributes LoadingScreen;
			LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
			LoadingScreen.MoviePaths.Add(TEXT("LoadingScreen"));
			GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
		}
		*/
	}
	
	virtual bool IsGameModule() const override
	{
		return true;
	}

	virtual void StartInGameLoadingScreen() override
	{
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
		LoadingScreen.WidgetLoadingScreen = SNew(STrueFPSLoadingScreen2);

		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
	}
};
    
IMPLEMENT_GAME_MODULE(FTrueFPSSystemLoadingScreenModule, TrueFPSSystemLoadingScreen)