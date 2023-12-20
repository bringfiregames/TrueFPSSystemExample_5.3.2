// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/TrueFPSFireWeaponBase.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TrueFPSSystem.h"
#include "Bots/TrueFPSAIController.h"
#include "Character/TrueFPSCharacterInterface.h"
#include "Character/TrueFPSPlayerController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Online/TrueFPSPlayerState.h"

ATrueFPSFireWeaponBase::ATrueFPSFireWeaponBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void ATrueFPSFireWeaponBase::BeginPlay()
{
	check(IsValid(FireSettings));
	
	Super::BeginPlay();
}

void ATrueFPSFireWeaponBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!IsValid(Settings)) return;

	if (FireSettings->InitialClips > 0)
	{
		CurrentAmmoInClip = FireSettings->AmmoPerClip;
		CurrentAmmo = FireSettings->AmmoPerClip * FireSettings->InitialClips;
	}
}

void ATrueFPSFireWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION( ThisClass, CurrentAmmo, COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( ThisClass, CurrentAmmoInClip, COND_OwnerOnly );

	DOREPLIFETIME_CONDITION( ThisClass, bPendingReload,	COND_SkipOwner );
}

int32 ATrueFPSFireWeaponBase::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

int32 ATrueFPSFireWeaponBase::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

int32 ATrueFPSFireWeaponBase::GetAmmoPerClip() const
{
	return FireSettings->AmmoPerClip;
}

int32 ATrueFPSFireWeaponBase::GetMaxAmmo() const
{
	return FireSettings->MaxAmmo;
}

bool ATrueFPSFireWeaponBase::HasInfiniteAmmo() const
{
	const ATrueFPSPlayerController* MyPC = (MyPawn != nullptr) ? Cast<const ATrueFPSPlayerController>(MyPawn->Controller) : nullptr;
	return FireSettings->bInfiniteAmmo || (MyPC && MyPC->HasInfiniteAmmo());
}

bool ATrueFPSFireWeaponBase::HasInfiniteClip() const
{
	const ATrueFPSPlayerController* MyPC = (MyPawn != nullptr) ? Cast<const ATrueFPSPlayerController>(MyPawn->Controller) : nullptr;
	return FireSettings->bInfiniteClip || (MyPC && MyPC->HasInfiniteClip());
}

void ATrueFPSFireWeaponBase::GiveAmmo(int AddAmount)
{
	const int32 MissingAmmo = FMath::Max(0, FireSettings->MaxAmmo - CurrentAmmo);
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	CurrentAmmo += AddAmount;

	if (ATrueFPSAIController* BotAI = MyPawn ? Cast<ATrueFPSAIController>(MyPawn->GetController()) : nullptr)
	{
		BotAI->CheckAmmo(this);
	}
	
	// start reload if clip was empty
	if (GetCurrentAmmoInClip() <= 0 &&
		CanReload() &&
		MyPawn && MyPawn->Implements<UTrueFPSCharacterInterface>() && (ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetCurrentWeapon(MyPawn) == this))
	{
		ClientStartReload();
	}
}

void ATrueFPSFireWeaponBase::UseAmmo()
{
	if (!HasInfiniteAmmo())
	{
		CurrentAmmoInClip--;
	}

	if (!HasInfiniteAmmo() && !HasInfiniteClip())
	{
		CurrentAmmo--;
	}

	ATrueFPSAIController* BotAI = MyPawn ? Cast<ATrueFPSAIController>(MyPawn->GetController()) : nullptr;
	const ATrueFPSPlayerController* PlayerController = MyPawn ? Cast<ATrueFPSPlayerController>(MyPawn->GetController()) : nullptr;
	if (BotAI)
	{
		BotAI->CheckAmmo(this);
	}
	else if(PlayerController)
	{
		if (ATrueFPSPlayerState* PlayerState = Cast<ATrueFPSPlayerState>(PlayerController->PlayerState))
		{
			PlayerState->AddBulletsFired(1);
		}
	}
}

void ATrueFPSFireWeaponBase::OnUnEquip()
{
	if (bPendingReload)
	{
		StopWeaponAnimation(FireSettings->ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(FireState.TimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(FireState.TimerHandle_ReloadWeapon);
	}
	
	Super::OnUnEquip();
}

void ATrueFPSFireWeaponBase::OnEquipFinished()
{
	Super::OnEquipFinished();
	
	if (MyPawn)
	{
		// try to reload empty clip
		if (MyPawn->IsLocallyControlled() &&
			CurrentAmmoInClip <= 0 &&
			CanReload())
		{
			StartReload();
		}
	}
}

void ATrueFPSFireWeaponBase::StartReload(bool bFromReplication)
{
	if (!bFromReplication && GetLocalRole() < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(FireSettings->ReloadAnim);		
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = FireSettings->NoAnimReloadDuration;
		}

		GetWorldTimerManager().SetTimer(FireState.TimerHandle_StopReload, this, &ThisClass::StopReload, AnimDuration, false);
		if (GetLocalRole() == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(FireState.TimerHandle_ReloadWeapon, this, &ThisClass::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}
		
		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			PlayWeaponSound(FireSettings->ReloadSound);
		}
	}
}

void ATrueFPSFireWeaponBase::StopReload()
{
	if (State.CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(FireSettings->ReloadAnim);
	}
}

void ATrueFPSFireWeaponBase::ReloadWeapon()
{
	int32 ClipDelta = FMath::Min(FireSettings->AmmoPerClip - CurrentAmmoInClip, CurrentAmmo - CurrentAmmoInClip);

	if (HasInfiniteClip())
	{
		ClipDelta = FireSettings->AmmoPerClip - CurrentAmmoInClip;
	}

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
	}

	if (HasInfiniteClip())
	{
		CurrentAmmo = FMath::Max(CurrentAmmoInClip, CurrentAmmo);
	}
}
void ATrueFPSFireWeaponBase::ClientStartReload_Implementation()
{
	StartReload();
}

bool ATrueFPSFireWeaponBase::CanFire() const
{
	return ( Super::CanFire() ) && ( bPendingReload == false );
}

bool ATrueFPSFireWeaponBase::CanReload() const
{
	const bool bCanReload = (!MyPawn || (MyPawn && MyPawn->Implements<UTrueFPSCharacterInterface>() && ITrueFPSCharacterInterface::Execute_TrueFPSInterface_CanReload(MyPawn)));
	const bool bGotAmmo = (CurrentAmmoInClip < FireSettings->AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
	const bool bStateOKToReload = ((State.CurrentState == EWeaponState::Idle) || (State.CurrentState == EWeaponState::Firing));
	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOKToReload == true));	
}

void ATrueFPSFireWeaponBase::ServerStartReload_Implementation()
{
	StartReload();
}

void ATrueFPSFireWeaponBase::ServerStopReload_Implementation()
{
	StopReload();
}

void ATrueFPSFireWeaponBase::SimulateWeaponFire()
{
	Super::SimulateWeaponFire();
	
	if (IsValid(FireSettings->MuzzleFX))
	{
		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		if (!FireSettings->bLoopedMuzzleFX || FireState.MuzzlePSC == nullptr)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if( (MyPawn != nullptr ) && ( MyPawn->IsLocallyControlled() == true ) )
			{
				if(const AController* PlayerCon = MyPawn->GetController(); PlayerCon != nullptr )
				{
					// Mesh1P->GetSocketLocation(MuzzleAttachPoint);
					FireState.MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(FireSettings->MuzzleFX, Mesh1P, FireSettings->MuzzleAttachPoint);
					FireState.MuzzlePSC->bOwnerNoSee = false;
					FireState.MuzzlePSC->bOnlyOwnerSee = true;

					// Mesh3P->GetSocketLocation(MuzzleAttachPoint);
					FireState.MuzzlePSCSecondary = UGameplayStatics::SpawnEmitterAttached(FireSettings->MuzzleFX, Mesh3P, FireSettings->MuzzleAttachPoint);
					FireState.MuzzlePSCSecondary->bOwnerNoSee = true;
					FireState.MuzzlePSCSecondary->bOnlyOwnerSee = false;				
				}				
			}
			else
			{
				FireState.MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(FireSettings->MuzzleFX, UseWeaponMesh, FireSettings->MuzzleAttachPoint);
			}
		}
	}

	if (IsValid(FireSettings->NiagaraMuzzleFX))
	{
		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		if (!FireSettings->bLoopedMuzzleFX || FireState.NiagaraMuzzleNC == nullptr)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if( (MyPawn != nullptr ) && ( MyPawn->IsLocallyControlled() == true ) )
			{
				if(const AController* PlayerCon = MyPawn->GetController(); PlayerCon != nullptr )
				{
					FireState.NiagaraMuzzleNC = UNiagaraFunctionLibrary::SpawnSystemAttached(FireSettings->NiagaraMuzzleFX, Mesh1P, FireSettings->MuzzleAttachPoint, FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
					FireState.NiagaraMuzzleNC->bOwnerNoSee = false;
					FireState.NiagaraMuzzleNC->bOnlyOwnerSee = true;
					FireState.NiagaraMuzzleNC->SetNiagaraVariableBool(FireSettings->NiagaraMuzzleTriggerParam.ToString(), true);

					FireState.NiagaraMuzzleNCSecondary = UNiagaraFunctionLibrary::SpawnSystemAttached(FireSettings->NiagaraMuzzleFX, Mesh3P, FireSettings->MuzzleAttachPoint, FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
					FireState.NiagaraMuzzleNCSecondary->bOwnerNoSee = true;
					FireState.NiagaraMuzzleNCSecondary->bOnlyOwnerSee = false;
					FireState.NiagaraMuzzleNCSecondary->SetNiagaraVariableBool(FireSettings->NiagaraMuzzleTriggerParam.ToString(), true);
				}				
			}
			else
			{
				FireState.NiagaraMuzzleNC = UNiagaraFunctionLibrary::SpawnSystemAttached(FireSettings->NiagaraMuzzleFX, UseWeaponMesh, FireSettings->MuzzleAttachPoint, FVector(ForceInit), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
				FireState.NiagaraMuzzleNC->SetNiagaraVariableBool(FireSettings->NiagaraMuzzleTriggerParam.ToString(), true);
			}
		}
	}

	if (IsValid(FireSettings->NiagaraShellFX) && IsValid(FireSettings->ShellMesh))
	{
		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		if (!FireSettings->bLoopedShellFX || FireState.NiagaraShellNC == nullptr)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if( (MyPawn != nullptr ) && ( MyPawn->IsLocallyControlled() == true ) )
			{
				if(const AController* PlayerCon = MyPawn->GetController(); PlayerCon != nullptr )
				{
					FireState.NiagaraShellNC = UNiagaraFunctionLibrary::SpawnSystemAttached(FireSettings->NiagaraShellFX, Mesh1P, NAME_None, Mesh1P->GetSocketTransform(FireSettings->ShellEjectSocketName, RTS_Actor).GetLocation(), FRotator(0.f, 90.f, 0.f), EAttachLocation::KeepRelativeOffset, true);
					FireState.NiagaraShellNC->bOwnerNoSee = false;
					FireState.NiagaraShellNC->bOnlyOwnerSee = true;
					FireState.NiagaraShellNC->SetNiagaraVariableBool(FireSettings->NiagaraShellTriggerParam.ToString(), true);
					FireState.NiagaraShellNC->SetVariableStaticMesh(FireSettings->NiagaraShellMeshParam, FireSettings->ShellMesh.Get());

					FireState.NiagaraShellNCSecondary = UNiagaraFunctionLibrary::SpawnSystemAttached(FireSettings->NiagaraShellFX.Get(), Mesh3P, NAME_None, Mesh3P->GetSocketTransform(FireSettings->ShellEjectSocketName, RTS_Actor).GetLocation(), FRotator(0.f, 90.f, 0.f), EAttachLocation::KeepRelativeOffset, true);
					FireState.NiagaraShellNCSecondary->bOwnerNoSee = true;
					FireState.NiagaraShellNCSecondary->bOnlyOwnerSee = false;
					FireState.NiagaraShellNCSecondary->SetNiagaraVariableBool(FireSettings->NiagaraShellTriggerParam.ToString(), true);
					FireState.NiagaraShellNCSecondary->SetVariableStaticMesh(FireSettings->NiagaraShellMeshParam, FireSettings->ShellMesh.Get());
				}				
			}
			else
			{
				FireState.NiagaraShellNC = UNiagaraFunctionLibrary::SpawnSystemAttached(FireSettings->NiagaraShellFX.Get(), UseWeaponMesh, NAME_None, UseWeaponMesh->GetSocketTransform(FireSettings->ShellEjectSocketName, RTS_Actor).GetLocation(), FRotator(0.f, 90.f, 0.f), EAttachLocation::KeepRelativeOffset, true);
				FireState.NiagaraShellNC->SetNiagaraVariableBool(FireSettings->NiagaraShellTriggerParam.ToString(), true);
				FireState.NiagaraShellNC->SetVariableStaticMesh(FireSettings->NiagaraShellMeshParam, FireSettings->ShellMesh.Get());
			}
		}
	}
}

void ATrueFPSFireWeaponBase::StopSimulatingWeaponFire()
{
	Super::StopSimulatingWeaponFire();
	
	if (FireSettings && FireSettings->bLoopedMuzzleFX)
	{
		if( FireState.MuzzlePSC.IsValid() )
		{
			FireState.MuzzlePSC->DeactivateSystem();
			FireState.MuzzlePSC = nullptr;
		}
		if( FireState.MuzzlePSCSecondary.IsValid() )
		{
			FireState.MuzzlePSCSecondary->DeactivateSystem();
			FireState.MuzzlePSCSecondary = nullptr;
		}
	}
}

void ATrueFPSFireWeaponBase::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}

void ATrueFPSFireWeaponBase::ServerHandleFiring_Implementation()
{
	Super::ServerHandleFiring_Implementation();

	if ((CurrentAmmoInClip > 0 && CanFire()))
	{
		// update ammo
		UseAmmo();
	}
}

void ATrueFPSFireWeaponBase::HandleFiring()
{
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			if (FireSettings->ShotsByFiring > 1)
			{
				for (int32 i = 0; i < FireSettings->ShotsByFiring; i++)
				{
					FireWeapon();
				}
			}
			else
			{
				FireWeapon();
			}

			UseAmmo();
			
			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !State.bRefiring)
		{
			PlayWeaponSound(Settings->OutOfAmmoSound);
		}
		
		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}
	else
	{
		OnBurstFinished();
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		// local client will notify server
		if (GetLocalRole() < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// reload after firing last round
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		State.bRefiring = (State.CurrentState == EWeaponState::Firing && Settings->TimeBetweenShots > 0.0f);
		if (State.bRefiring)
		{
			GetWorldTimerManager().SetTimer(State.TimerHandle_HandleFiring, this, &ThisClass::HandleReFiring, FMath::Max<float>(Settings->TimeBetweenShots + State.TimerIntervalAdjustment, SMALL_NUMBER), false);
			State.TimerIntervalAdjustment = 0.f;
		}
	}

	State.LastFireTime = GetWorld()->GetTimeSeconds();
}

void ATrueFPSFireWeaponBase::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (State.bIsEquipped)
	{
		if( bPendingReload  )
		{
			if( CanReload() == false )
			{
				NewState = State.CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}		
		else if ( (bPendingReload == false ) && ( State.bWantsToFire == true ) && ( CanFire() == true ))
		{
			NewState = EWeaponState::Firing;
		}
	}
	else if (State.bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}

	SetWeaponState(NewState);
}

FVector ATrueFPSFireWeaponBase::GetAdjustedAim() const
{
	// If it's close to wall, use muzzle's rotation
	if (IsCloseToWall()) return GetMuzzleDirection();

	return Super::GetAdjustedAim();
}


FVector ATrueFPSFireWeaponBase::GetMuzzleLocation() const
{
	const USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketLocation(FireSettings->MuzzleAttachPoint);
}

FVector ATrueFPSFireWeaponBase::GetMuzzleDirection() const
{
	const USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketRotation(FireSettings->MuzzleAttachPoint).Vector();
}

FHitResult ATrueFPSFireWeaponBase::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(WeaponTrace), true, GetInstigator());
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

	return Hit;
}

FVector ATrueFPSFireWeaponBase::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	const ATrueFPSPlayerController* PC = MyPawn ? Cast<ATrueFPSPlayerController>(MyPawn->Controller) : nullptr;
	const ATrueFPSAIController* AIPC = MyPawn ? Cast<ATrueFPSAIController>(MyPawn->Controller) : nullptr;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((GetInstigator()->GetActorLocation() - OutStartTrace) | AimDir);
	}
	else if (AIPC)
	{
		OutStartTrace = GetMuzzleLocation();
	}

	return OutStartTrace;
}
