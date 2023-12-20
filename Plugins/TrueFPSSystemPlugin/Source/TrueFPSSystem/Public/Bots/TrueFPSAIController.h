// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIModule/Classes/AIController.h"
#include "TrueFPSAIController.generated.h"

UCLASS()
class TRUEFPSSYSTEM_API ATrueFPSAIController : public AAIController
{
	GENERATED_BODY()

public:

	ATrueFPSAIController(const FObjectInitializer& ObjectInitializer);

private:
	
	UPROPERTY(transient)
	class UBlackboardComponent* BlackboardComp;

	/* Cached BT component */
	UPROPERTY(transient)
	class UBehaviorTreeComponent* BehaviorComp;
	
public:

	// Begin AController interface
	virtual void GameHasEnded(class AActor* EndGameFocus = nullptr, bool bIsWinner = false) override;
	virtual void BeginInactiveState() override;
	
protected:
	
	virtual void OnPossess(class APawn* InPawn) override;
	virtual void OnUnPossess() override;
	// End APlayerController interface

	public:
	void Respawn();

	void CheckAmmo(const class ATrueFPSWeaponBase* CurrentWeapon);

	void SetEnemy(class APawn* InPawn);

	class ATrueFPSCharacter* GetEnemy() const;

	/* If there is line of sight to current enemy, start firing at them */
	UFUNCTION(BlueprintCallable, Category=Behavior)
	void ShootEnemy();

	/* Finds the closest enemy and sets them as current target */
	UFUNCTION(BlueprintCallable, Category=Behavior)
	void FindClosestEnemy();

	UFUNCTION(BlueprintCallable, Category = Behavior)
	bool FindClosestEnemyWithLOS(ATrueFPSCharacter* ExcludeEnemy);
		
	bool HasWeaponLOSToEnemy(AActor* InEnemyActor, const bool bAnyEnemy) const;

	// Begin AAIController interface
	/** Update direction AI is looking based on FocalPoint */
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
	// End AAIController interface

protected:
	
	// Check of we have LOS to a character
	bool LOSTrace(ATrueFPSCharacter* InEnemyChar) const;

	int32 EnemyKeyID;
	int32 NeedAmmoKeyID;

	/** Handle for efficient management of Respawn timer */
	FTimerHandle TimerHandle_Respawn;

public:
	
	/** Returns BlackboardComp subobject **/
	FORCEINLINE UBlackboardComponent* GetBlackboardComp() const { return BlackboardComp; }
	
	/** Returns BehaviorComp subobject **/
	FORCEINLINE UBehaviorTreeComponent* GetBehaviorComp() const { return BehaviorComp; }
	
};
