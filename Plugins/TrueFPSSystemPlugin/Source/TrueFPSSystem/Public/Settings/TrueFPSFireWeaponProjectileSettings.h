// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/DamageType.h"
#include "TrueFPSFireWeaponProjectileSettings.generated.h"

class UDamageType;
class ATrueFPSProjectile;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class TRUEFPSSYSTEM_API UTrueFPSFireWeaponProjectileSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:

	// WeaponStat

	/** damage at impact point */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|WeaponStat")
	int32 ExplosionDamage{100};

	/** radius of damage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|WeaponStat")
	float ExplosionRadius{300.f};

	/** type of damage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|WeaponStat")
	TSubclassOf<UDamageType> DamageType{UDamageType::StaticClass()};

	// Projectile

	/** projectile class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Projectile")
	TSubclassOf<ATrueFPSProjectile> ProjectileClass;

	/** life time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Projectile")
	float ProjectileLife{10.f};
	
};
