#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveVector.h"
#include "Engine/DamageEvents.h"
#include "Engine/DataTable.h"

#include "TrueFPSTypes.generated.h"

class UNiagaraSystem;
class USoundBase;

namespace ETrueFPSMatchState
{
	enum Type
	{
		Warmup,
		Playing,
		Won,
		Lost,
	};
}

namespace ETrueFPSCrosshairDirection
{
	enum Type
	{
		Left=0,
		Right=1,
		Top=2,
		Bottom=3,
		Center=4
	};
}

namespace ETrueFPSHudPosition
{
	enum Type
	{
		Left=0,
		FrontLeft=1,
		Front=2,
		FrontRight=3,
		Right=4,
		BackRight=5,
		Back=6,
		BackLeft=7,
	};
}

namespace ETrueFPSDialogType
{
	enum Type
	{
		None,
		Generic,
		ControllerDisconnected
	};
}

#define TRUEFPS_SURFACE_Default		SurfaceType_Default
#define TRUEFPS_SURFACE_Flesh		SurfaceType1
#define TRUEFPS_SURFACE_Concrete	SurfaceType2
#define TRUEFPS_SURFACE_Glass		SurfaceType3
#define TRUEFPS_SURFACE_Dirt		SurfaceType4
#define TRUEFPS_SURFACE_Water		SurfaceType5
#define TRUEFPS_SURFACE_Metal		SurfaceType6
#define TRUEFPS_SURFACE_Wood		SurfaceType7
#define TRUEFPS_SURFACE_Grass		SurfaceType8

/* Returns the enumeration value as string. */
template <typename Enumeration>
static FORCEINLINE FString GetEnumerationToString(const Enumeration InValue)
{
	return StaticEnum<Enumeration>()->GetNameStringByValue(static_cast<int64>(InValue));
}

USTRUCT(BlueprintType)
struct FDecalData
{
	GENERATED_USTRUCT_BODY()

	/** material */
	UPROPERTY(EditDefaultsOnly, Category=Decal)
	UMaterial* DecalMaterial;

	/** quad size (width & height) */
	UPROPERTY(EditDefaultsOnly, Category=Decal)
	float DecalSize;

	/** lifespan */
	UPROPERTY(EditDefaultsOnly, Category=Decal)
	float LifeSpan;

	/** defaults */
	FDecalData()
		: DecalMaterial(nullptr)
		, DecalSize(256.f)
		, LifeSpan(10.f)
	{
	}
};

/** replicated information on a hit we've taken */
USTRUCT(BlueprintType)
struct FTakeHitInfo
{
	GENERATED_BODY()

public:

	/** The amount of damage actually applied */
	UPROPERTY()
	float ActualDamage;

	/** The damage type we were hit with. */
	UPROPERTY()
	UClass* DamageTypeClass;

	/** Who hit us */
	UPROPERTY()
	TWeakObjectPtr<class ACharacter> PawnInstigator;

	/** Who actually caused the damage */
	UPROPERTY()
	TWeakObjectPtr<class AActor> DamageCauser;

	/** Specifies which DamageEvent below describes the damage received. */
	UPROPERTY()
	int32 DamageEventClassID;

	/** Rather this was a kill */
	UPROPERTY()
	uint32 bKilled:1;

private:

	/** A rolling counter used to ensure the struct is dirty and will replicate. */
	UPROPERTY()
	uint8 EnsureReplicationByte;

	/** Describes general damage. */
	UPROPERTY()
	FDamageEvent GeneralDamageEvent;

	/** Describes point damage, if that is what was received. */
	UPROPERTY()
	FPointDamageEvent PointDamageEvent;

	/** Describes radial damage, if that is what was received. */
	UPROPERTY()
	FRadialDamageEvent RadialDamageEvent;

public:
	
	FTakeHitInfo()
		: ActualDamage(0)
		, DamageTypeClass(nullptr)
		, PawnInstigator(nullptr)
		, DamageCauser(nullptr)
		, DamageEventClassID(0)
		, bKilled(false)
		, EnsureReplicationByte(0)
	{}

	FDamageEvent& GetDamageEvent()
	{
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			if (PointDamageEvent.DamageTypeClass == nullptr)
			{
				PointDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return PointDamageEvent;

		case FRadialDamageEvent::ClassID:
			if (RadialDamageEvent.DamageTypeClass == nullptr)
			{
				RadialDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return RadialDamageEvent;

		default:
			if (GeneralDamageEvent.DamageTypeClass == nullptr)
			{
				GeneralDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return GeneralDamageEvent;
		}
	}
	
	void SetDamageEvent(const FDamageEvent& DamageEvent)
	{
		DamageEventClassID = DamageEvent.GetTypeID();
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			PointDamageEvent = *((FPointDamageEvent const*)(&DamageEvent));
			break;
		case FRadialDamageEvent::ClassID:
			RadialDamageEvent = *((FRadialDamageEvent const*)(&DamageEvent));
			break;
		default:
			GeneralDamageEvent = DamageEvent;
		}

		DamageTypeClass = DamageEvent.DamageTypeClass;
	}
	
	void EnsureReplication()
	{
		EnsureReplicationByte++;
	}
};

USTRUCT(BlueprintType)
struct FRecoilParams
{
	GENERATED_BODY()

	FRecoilParams(){}
	FRecoilParams(class UCurveVector* LocationCurve, class UCurveVector* RotationCurve, const float PlayRate = 1.f, const float Magnitude = 1.f)
		: LocationCurve(LocationCurve), RotationCurve(RotationCurve), PlayRate(PlayRate), Magnitude(Magnitude) {}

	FORCEINLINE bool IsValid() const { return LocationCurve != nullptr; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UCurveVector* LocationCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UCurveVector* RotationCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Magnitude = 1.f;
};

USTRUCT(BlueprintType)
struct FRecoilInstance
{
	GENERATED_BODY()
	
	FRecoilInstance(){}
	FRecoilInstance(const FRecoilParams& RecoilParams) : LocationCurve(RecoilParams.LocationCurve), RotationCurve(RecoilParams.RotationCurve), PlayRate(RecoilParams.PlayRate)
	{
		if (!LocationCurve || !RotationCurve) return;
		
		float MinTime;
		LocationCurve->GetTimeRange(MinTime, Lifetime);
	}

	FORCEINLINE bool IsValid() const
	{
		if (!LocationCurve || !RotationCurve) return false;

		float MinTime1;
		float MaxTime1;
		LocationCurve->GetTimeRange(MinTime1, MaxTime1);

		float MinTime2;
		float MaxTime2;
		RotationCurve->GetTimeRange(MinTime2, MaxTime2);
		
		if (MinTime1 != MinTime2 || MaxTime1 != MaxTime2) return false;

		return true;
	}
	
	class UCurveVector* LocationCurve = nullptr;
	class UCurveVector* RotationCurve = nullptr;
	
	float PlayRate = 1.f;
	
	float CurrentTime = 0.f;
	float Lifetime = 0.f;
};

UENUM(BlueprintType)
enum class EAnimState : uint8
{
	Default,
	Rifle,
	Pistol,
	Shotgun
};

// Left / Right hand
UENUM(BlueprintType)
enum class ELaterality : uint8
{
	Left, Right
};