// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/TrueFPSCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Character/Animation/TrueFPSAnimInstanceBase.h"
#include "TrueFPSSystem.h"
#include "Weapons/TrueFPSWeaponBase.h"
#include "Camera/CameraComponent.h"
#include "Character/TrueFPSCharacterMovement.h"
#include "Character/TrueFPSPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "Online/TrueFPSGameMode.h"
#include "Online/TrueFPSPlayerState.h"
#include "Sound/LocalPlayerSoundNode.h"
#include "UI/TrueFPSHUD.h"
#include "Weapons/TrueFPSDamageType.h"
#include "Weapons/TrueFPSFireWeaponBase.h"

ATrueFPSCharacter::ATrueFPSCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTrueFPSCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	GetMesh()->bReceivesDecals = false;
	GetMesh()->bVisibleInReflectionCaptures = true;
	GetMesh()->SetCastHiddenShadow(true);
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;

	// Create the ClientMesh
	ClientMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Client Mesh"));
	ClientMesh->SetupAttachment(GetMesh());
	ClientMesh->bCastDynamicShadow = false;
	ClientMesh->bReceivesDecals = false;
	ClientMesh->SetCollisionObjectType(ECC_Pawn);
	ClientMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClientMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClientMesh->bOnlyOwnerSee = true;
	ClientMesh->bOwnerNoSee = false;
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	bIsAiming = false;
	bWantsToRun = false;
}

void ATrueFPSCharacter::BeginDestroy()
{
	Super::BeginDestroy();

	if (!GExitPurge)
	{
		const uint32 UniqueID = GetUniqueID();
		FAudioThread::RunCommandOnAudioThread([UniqueID]()
		{
			ULocalPlayerSoundNode::GetLocallyControlledActorCache().Remove(UniqueID);
		});
	}
}

void ATrueFPSCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GetLocalRole() == ROLE_Authority)
	{
		Health = GetMaxHealth();

		if (IsValid(Settings))
		{
			// Needs to happen after character is added to rep graph
			GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::SpawnDefaultInventory);
		}
	}
}

void ATrueFPSCharacter::Tick(float DeltaTime)
{
	if (!IsValid(Settings))
	{
		Super::Tick(DeltaTime);
		return;
	}
	
	Super::Tick(DeltaTime);
	
	RefreshLeanValue(DeltaTime);

	RefreshCrouchValue(DeltaTime);

	RefreshHealthRegen(DeltaTime);

	RefreshWallAvoidanceState(DeltaTime);

	const APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bLocallyControlled = (PC ? PC->IsLocalController() : false);
	const uint32 UniqueID = GetUniqueID();
	FAudioThread::RunCommandOnAudioThread([UniqueID, bLocallyControlled]()
	{
	    ULocalPlayerSoundNode::GetLocallyControlledActorCache().Add(UniqueID, bLocallyControlled);
	});
	
	TArray<FVector> PointsToTest;
	BuildPauseReplicationCheckPoints(PointsToTest);
}

void ATrueFPSCharacter::Destroyed()
{
	Super::Destroyed();
	
	DestroyInventory();
}

void ATrueFPSCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// reattach weapon if needed
	SetCurrentWeapon(CurrentWeapon);
}

void ATrueFPSCharacter::PossessedBy(AController* C)
{
	Super::PossessedBy(C);
}

void ATrueFPSCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
}

bool ATrueFPSCharacter::IsReplicationPausedForConnection(const FNetViewer& ConnectionOwnerNetViewer)
{
	return Super::IsReplicationPausedForConnection(ConnectionOwnerNetViewer);
}

void ATrueFPSCharacter::OnReplicationPausedChanged(bool bIsReplicationPaused)
{
	GetMesh()->SetHiddenInGame(bIsReplicationPaused, true);
}

bool ATrueFPSCharacter::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == Controller || TestPC == nullptr)
	{
		return false;
	}

	ATrueFPSPlayerState* TestPlayerState = Cast<ATrueFPSPlayerState>(TestPC->PlayerState);
	ATrueFPSPlayerState* MyPlayerState = Cast<ATrueFPSPlayerState>(GetPlayerState());

	bool bIsEnemy = true;
	if (GetWorld()->GetGameState())
	{
		const ATrueFPSGameMode* DefGame = GetWorld()->GetGameState()->GetDefaultGameMode<ATrueFPSGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			bIsEnemy = DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}

	return bIsEnemy;
}

void ATrueFPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	ClientMesh->HideBoneByName(FName("neck_01"), PBO_None);
}

void ATrueFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::OnStartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::OnStopJump);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);

		// Jumping
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Triggered, this, &ThisClass::OnStartRunning);
		EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Completed, this, &ThisClass::OnStopRunning);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);

		// Attacking
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &ThisClass::OnStartFire);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &ThisClass::OnStopFire);

		// Aiming
		EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Triggered, this, &ThisClass::OnStartAiming);
		EnhancedInputComponent->BindAction(AimingAction, ETriggerEvent::Completed, this, &ThisClass::OnStopAiming);

		// Weapon Changing
		EnhancedInputComponent->BindAction(NextWeaponAction, ETriggerEvent::Triggered, this, &ThisClass::OnNextWeapon);
		EnhancedInputComponent->BindAction(PreviousWeaponAction, ETriggerEvent::Triggered, this, &ThisClass::OnPrevWeapon);

		// Reloading
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ThisClass::OnReload);

		// Leaning
		EnhancedInputComponent->BindAction(LeanLeftAction, ETriggerEvent::Triggered, this, &ThisClass::OnLeanLeft);
		EnhancedInputComponent->BindAction(LeanLeftAction, ETriggerEvent::Completed, this, &ThisClass::OnStopLean);
		EnhancedInputComponent->BindAction(LeanRightAction, ETriggerEvent::Triggered, this, &ThisClass::OnLeanRight);
		EnhancedInputComponent->BindAction(LeanRightAction, ETriggerEvent::Completed, this, &ThisClass::OnStopLean);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::OnStartCrouching);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ThisClass::OnStopCrouching);

		// Toggle Sights
		EnhancedInputComponent->BindAction(ToggleSightsAction, ETriggerEvent::Triggered, this, &ThisClass::OnToggleSights);
	}
}

void ATrueFPSCharacter::Move(const FInputActionValue& ActionValue)
{
	// input is a Vector2D
	const FVector2D MovementVector = ActionValue.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ATrueFPSCharacter::Look(const FInputActionValue& ActionValue)
{
	const auto Value{ActionValue.Get<FVector2D>()};

	const float TargetPitch = Value.Y * LookUpMouseSensitivity * -1;
	const float TargetYaw = Value.X * LookRightMouseSensitivity;

	AddControllerPitchInput(TargetPitch);
	AddControllerYawInput(TargetYaw);
}

void ATrueFPSCharacter::AddWeapon(ATrueFPSWeaponBase* Weapon)
{
	if (Weapon && GetLocalRole() == ROLE_Authority)
	{
		Weapon->OnEnterInventory(this);
		Inventory.AddUnique(Weapon);
	}
}

void ATrueFPSCharacter::RemoveWeapon(ATrueFPSWeaponBase* Weapon)
{
	if (Weapon && GetLocalRole() == ROLE_Authority)
	{
		Weapon->OnLeaveInventory();
		Inventory.RemoveSingle(Weapon);
	}
}

ATrueFPSWeaponBase* ATrueFPSCharacter::FindWeapon(TSubclassOf<ATrueFPSWeaponBase> WeaponClass)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] && Inventory[i]->IsA(WeaponClass))
		{
			return Inventory[i];
		}
	}

	return nullptr;
}

void ATrueFPSCharacter::EquipWeapon(ATrueFPSWeaponBase* Weapon)
{
	if (Weapon)
	{
		if (GetLocalRole() == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon, CurrentWeapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
	}
}

void ATrueFPSCharacter::StartWeaponFire()
{
	if (!State.bWantsToFire)
	{
		State.bWantsToFire = true;
		if (CurrentWeapon)
		{
			CurrentWeapon->StartFire();
		}
	}
}

void ATrueFPSCharacter::StopWeaponFire()
{
	if (State.bWantsToFire)
	{
		State.bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

bool ATrueFPSCharacter::CanFire() const
{
	return IsAlive();
}

bool ATrueFPSCharacter::CanReload() const
{
	return true;
}

void ATrueFPSCharacter::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming;

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetAiming(bNewAiming);
	}
}

void ATrueFPSCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;

	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetRunning(bNewRunning, bToggle);
	}
}

float ATrueFPSCharacter::PlayAnimMontage(UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName)
{
	// USkeletalMeshComponent* UseMesh = GetPawnMesh();
	const USkeletalMeshComponent* UseMesh = GetMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
	}

	return 0.0f;
}

void ATrueFPSCharacter::StopAnimMontage(UAnimMontage* AnimMontage)
{
	// USkeletalMeshComponent* UseMesh = GetPawnMesh();
	const USkeletalMeshComponent* UseMesh = GetMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance &&
		UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
	{
		UseMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOut.GetBlendTime(), AnimMontage);
	}
}

void ATrueFPSCharacter::StopAllAnimMontages() const
{
	// USkeletalMeshComponent* UseMesh = GetPawnMesh();
	const USkeletalMeshComponent* UseMesh = GetMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}

void ATrueFPSCharacter::OnStartFire()
{
	if (const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller); PC && PC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		StartWeaponFire();
	}
}

void ATrueFPSCharacter::OnStopFire()
{
	StopWeaponFire();
}

void ATrueFPSCharacter::OnStartAiming()
{
	if (const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller); PC && PC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		SetAiming(true);
	}
}

void ATrueFPSCharacter::OnStopAiming()
{
	SetAiming(false);
}

void ATrueFPSCharacter::OnNextWeapon()
{
	if (const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller); PC && PC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == nullptr || CurrentWeapon->GetState().CurrentState != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			ATrueFPSWeaponBase* NextWeapon = Inventory[(CurrentWeaponIdx + 1) % Inventory.Num()];
			EquipWeapon(NextWeapon);
		}
	}
}

void ATrueFPSCharacter::OnPrevWeapon()
{
	if (const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller); PC && PC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2 && (CurrentWeapon == nullptr || CurrentWeapon->GetState().CurrentState != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			ATrueFPSWeaponBase* PrevWeapon = Inventory[(CurrentWeaponIdx - 1 + Inventory.Num()) % Inventory.Num()];
			EquipWeapon(PrevWeapon);
		}
	}
}

void ATrueFPSCharacter::OnReload()
{
	if (const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller); PC && PC->IsGameInputAllowed())
	{
		if (ATrueFPSFireWeaponBase* CurrentFireWeapon = Cast<ATrueFPSFireWeaponBase>(CurrentWeapon))
		{
			CurrentFireWeapon->StartReload();
		}
	}
}

void ATrueFPSCharacter::OnStartJump()
{
	if (const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller); PC && PC->IsGameInputAllowed())
	{
		bPressedJump = true;
	}
}

void ATrueFPSCharacter::OnStopJump()
{
	bPressedJump = false;
	StopJumping();
}

void ATrueFPSCharacter::OnStartRunning()
{
	if (const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller); PC && PC->IsGameInputAllowed())
	{
		if (IsAiming())
		{
			SetAiming(false);
		}
	
		StopWeaponFire();
		SetRunning(true, false);
	}
}

void ATrueFPSCharacter::OnStopRunning()
{
	SetRunning(false, false);
}

void ATrueFPSCharacter::OnLeanLeft()
{
	SetLeanValue(-Settings->LeanAmount);
}

void ATrueFPSCharacter::OnLeanRight()
{
	SetLeanValue(Settings->LeanAmount);
}

void ATrueFPSCharacter::OnStopLean()
{
	SetLeanValue(0.f);
}

void ATrueFPSCharacter::OnStartCrouching()
{
	ToggleCrouching(true);
}

void ATrueFPSCharacter::OnStopCrouching()
{
	ToggleCrouching(false);
}

void ATrueFPSCharacter::OnToggleSights()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->ToggleSights();
	}
}

USkeletalMeshComponent* ATrueFPSCharacter::GetPawnMesh() const
{
	return IsAlive() && IsLocallyControlled() ? ClientMesh : GetMesh();
}

ATrueFPSWeaponBase* ATrueFPSCharacter::GetWeapon() const
{
	return CurrentWeapon;
}

int32 ATrueFPSCharacter::GetInventoryCount() const
{
	return Inventory.Num();
}

ATrueFPSWeaponBase* ATrueFPSCharacter::GetInventoryWeapon(int32 index) const
{
	return Inventory[index];
}

float ATrueFPSCharacter::GetAimingSpeedModifier() const
{
	return Settings->AimingSpeedModifier;
}

bool ATrueFPSCharacter::IsAiming() const
{
	return bIsAiming;
}

float ATrueFPSCharacter::GetRunningSpeedModifier() const
{
	return Settings->RunningSpeedModifier;
}

bool ATrueFPSCharacter::IsRunning() const
{
	if (!GetCharacterMovement())
	{
		return false;
	}

	return (bWantsToRun/* || bWantsToRunToggled*/) && !GetVelocity().IsZero() && (GetVelocity().GetSafeNormal2D() | GetActorForwardVector()) > -0.1;
}

int32 ATrueFPSCharacter::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<ATrueFPSCharacter>()->Health;
}

bool ATrueFPSCharacter::IsAlive() const
{
	return Health > 0.f;
}

float ATrueFPSCharacter::GetLowHealthPercentage() const
{
	return State.LowHealthPercentage;
}

void ATrueFPSCharacter::SetLeanValue(const float NewLeanValue)
{
	LeanValueTarget = NewLeanValue;
	
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerSetLeanValue(NewLeanValue);
	}
}

void ATrueFPSCharacter::ServerSetLeanValue_Implementation(const float NewLeanValue)
{
	SetLeanValue(NewLeanValue);
}

void ATrueFPSCharacter::ToggleCrouching(const bool bNewCrouching)
{
	if (bNewCrouching)
	{
		CrouchValueTarget = 1.f;
		Crouch();
	}
	else
	{
		CrouchValueTarget = 0.f;
		UnCrouch();
	}
	
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerToggleCrouching(bNewCrouching);
	}
}

void ATrueFPSCharacter::ServerToggleCrouching_Implementation(const bool bNewCrouching)
{
	ToggleCrouching(bNewCrouching);
}

void ATrueFPSCharacter::RefreshLeanValue(const float DeltaTime)
{
	if (const APlayerController* PC = GetController<APlayerController>(); !PC) return;

	const float OldLeanValue = State.LeanValue;
	State.LeanValue = UKismetMathLibrary::FInterpTo(State.LeanValue, LeanValueTarget, DeltaTime, Settings->AimingInterpSpeed);
	State.CurrentRollControllerInput = State.LeanValue - OldLeanValue;
	const float TargetRoll = State.CurrentRollControllerInput * 0.2f;
	AddControllerRollInput(TargetRoll);
}

void ATrueFPSCharacter::RefreshCrouchValue(const float DeltaTime)
{
	State.CrouchValue = UKismetMathLibrary::FInterpTo(State.CrouchValue, CrouchValueTarget, DeltaTime, 8.f);
}

void ATrueFPSCharacter::RefreshHealthRegen(const float DeltaTime)
{
	if (const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller); PC && PC->HasHealthRegen())
	{
		if (this->Health < this->GetMaxHealth())
		{
			this->Health += 5 * DeltaTime;
			if (Health > this->GetMaxHealth())
			{
				Health = this->GetMaxHealth();
			}
		}
	}
}

void ATrueFPSCharacter::RefreshWallAvoidanceState(const float DeltaTime)
{
	if (IsLocallyControlled() && bIsAiming)
		if (CurrentWeapon && CurrentWeapon->IsCloseToWall())
			OnStopAiming();
}

void ATrueFPSCharacter::TornOff()
{
	SetLifeSpan(25.f);
}

bool ATrueFPSCharacter::IsMoving() const
{
	return FMath::Abs(GetLastMovementInputVector().Size()) > 0.f;
}

float ATrueFPSCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller); PC && PC->HasGodMode())
	{
		return 0.f;
	}

	if (Health <= 0.f)
	{
		return 0.f;
	}

	// Modify based on game rules.
	const ATrueFPSGameMode* const Game = GetWorld()->GetAuthGameMode<ATrueFPSGameMode>();
	Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage > 0.f)
	{
		Health -= ActualDamage;
		if (Health <= 0)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
		}

		PlayHit(ActualDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : nullptr, DamageCauser, Health <= 0);

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}

	return ActualDamage;
}

void ATrueFPSCharacter::Suicide()
{
	KilledBy(this);
}

void ATrueFPSCharacter::KilledBy(APawn* EventInstigator)
{
	if (GetLocalRole() == ROLE_Authority && !State.bIsDying)
	{
		AController* Killer = nullptr;
		if (EventInstigator != nullptr)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = nullptr;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, nullptr);
	}
}

bool ATrueFPSCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer,
	AActor* DamageCauser) const
{
	if (State.bIsDying									// already dying
		|| IsValidChecked(this)								// already destroyed
		|| GetLocalRole() != ROLE_Authority				// not authority
		|| GetWorld()->GetAuthGameMode<ATrueFPSGameMode>() == nullptr
		|| GetWorld()->GetAuthGameMode<ATrueFPSGameMode>()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}

bool ATrueFPSCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer,
	AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != nullptr) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<ATrueFPSGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<ATrueFPSCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : nullptr, DamageCauser);
	return true;
}

void ATrueFPSCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
	Die(Health, FDamageEvent(dmgType.GetClass()), nullptr, nullptr);
}

void ATrueFPSCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// only to local owner: weapon change requests are locally instigated, other clients don't need it
	DOREPLIFETIME_CONDITION(ThisClass, Inventory, COND_OwnerOnly);

	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION(ThisClass, bIsAiming, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ThisClass, bWantsToRun, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ThisClass, LeanValueTarget, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ThisClass, CrouchValueTarget, COND_SkipOwner);

	DOREPLIFETIME_CONDITION(ThisClass, LastTakeHitInfo, COND_Custom);

	// everyone
	DOREPLIFETIME(ThisClass, CurrentWeapon);
	DOREPLIFETIME(ThisClass, Health);
}

void ATrueFPSCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE(ThisClass, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < State.LastTakeHitTimeTimeout);
}

void ATrueFPSCharacter::OnDeath(float KillingDamage, FDamageEvent const& DamageEvent, APawn* PawnInstigator,
	AActor* DamageCauser)
{
	if (State.bIsDying) return;

	SetReplicatingMovement(false);
	TearOff();
	State.bIsDying = true;

	if (GetLocalRole() == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);

		// play the force feedback effect on the client player controller
		ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			if (const UTrueFPSDamageType* DamageType = Cast<UTrueFPSDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()); DamageType && DamageType->KilledForceFeedback && PC->IsVibrationEnabled())
			{
				FForceFeedbackParameters FFParams;
				FFParams.Tag = "Damage";
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, FFParams);
			}
		}
	}

	// cannot use IsLocallyControlled here, because even local client's controller may be nullptr here
	if (GetNetMode() != NM_DedicatedServer && Settings->DeathSound && ClientMesh && ClientMesh->IsVisible())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Settings->DeathSound, GetActorLocation());
	}

	// remove all weapons
	DestroyInventory();

	DetachFromControllerPendingDestroy();
	StopAllAnimMontages();

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	// Death anim
	const float DeathAnimDuration = PlayAnimMontage(Settings->DeathAnim);

	// Ragdoll
	if (DeathAnimDuration > 0.f)
	{
		// Trigger ragdoll a little before the animation early so the character doesn't
		// blend back to its normal position.
		const float TriggerRagdollTime = DeathAnimDuration - 0.7f;

		// Enable blend physics so the bones are properly blending against the montage.
		GetMesh()->bBlendPhysics = true;

		// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &ThisClass::SetRagdollPhysics, FMath::Max(0.1f, TriggerRagdollTime), false);
	}
	else
	{
		SetRagdollPhysics();
	}

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void ATrueFPSCharacter::PlayHit(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator,
	AActor* DamageCauser, bool bKilled/* = false*/)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, bKilled);

		// play the force feedback effect on the client player controller
		ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			if (const UTrueFPSDamageType* DamageType = Cast<UTrueFPSDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()); DamageType && DamageType->HitForceFeedback && PC->IsVibrationEnabled())
			{
				FForceFeedbackParameters FFParams;
				FFParams.Tag = "Damage";
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, FFParams);
			}
		}
	}

	if (DamageTaken > 0.f)
	{
		ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
	}

	const ATrueFPSPlayerController* PC = Cast<ATrueFPSPlayerController>(Controller);
	if (ATrueFPSHUD* MyHUD = PC ? Cast<ATrueFPSHUD>(PC->GetHUD()) : nullptr)
	{
		MyHUD->NotifyWeaponHit(DamageTaken, DamageEvent, PawnInstigator);
	}
	
	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled())
	{
		const ATrueFPSPlayerController* InstigatorPC = Cast<ATrueFPSPlayerController>(PawnInstigator->Controller);
		if (ATrueFPSHUD* InstigatorHUD = InstigatorPC ? Cast<ATrueFPSHUD>(InstigatorPC->GetHUD()) : nullptr)
		{
			InstigatorHUD->NotifyEnemyHit(bKilled);
		}
	}
}
/**
void ATrueFPSCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsValidChecked(this))
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan(1.0f);
	}
	else
	{
		SetLifeSpan(10.0f);
	}
}
*/

void ATrueFPSCharacter::SetRagdollPhysics()
{
	bool bInRagdoll;

	if (IsValidChecked(this) && GetMesh() && GetMesh()->GetPhysicsAsset())
	{
		// initialize physics/etc
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}
	else
	{
		bInRagdoll = false;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan(1.0f);
	}
	else
	{
		SetLifeSpan(10.0f);
	}
}

void ATrueFPSCharacter::ReplicateHit(float Damage, FDamageEvent const& DamageEvent, APawn* PawnInstigator,
	AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (State.LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<ATrueFPSCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	State.LastTakeHitTimeTimeout = TimeoutTime;
}

void ATrueFPSCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
	
	PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get(), LastTakeHitInfo.bKilled);
}

void ATrueFPSCharacter::SetCurrentWeapon(ATrueFPSWeaponBase* NewWeapon, ATrueFPSWeaponBase* LastWeapon)
{
	ATrueFPSWeaponBase* LocalLastWeapon = nullptr;

	if (LastWeapon != nullptr)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// unequip previous
	if (LocalLastWeapon)
	{
		LocalLastWeapon->OnUnEquip();
	}

	CurrentWeapon = NewWeapon;

	// equip new one
	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this); // Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!

		NewWeapon->OnEquip(LastWeapon);
	}

	// notify anim instance
	if (UTrueFPSAnimInstanceBase* AnimInstance = Cast<UTrueFPSAnimInstanceBase>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->OnChangeWeapon(NewWeapon);
	}
}

void ATrueFPSCharacter::OnRep_CurrentWeapon(ATrueFPSWeaponBase* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void ATrueFPSCharacter::SpawnDefaultInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	const int32 NumWeaponClasses = Settings->DefaultWeapons.Num();
	for (int32 i = 0; i < NumWeaponClasses; i++)
	{
		if (Settings->DefaultWeapons[i])
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ATrueFPSWeaponBase* NewWeapon = GetWorld()->SpawnActor<ATrueFPSWeaponBase>(Settings->DefaultWeapons[i], SpawnInfo);
			AddWeapon(NewWeapon);
		}
	}

	// equip first weapon in inventory
	if (Inventory.Num() > 0)
	{
		EquipWeapon(Inventory[0]);
	}
}

void ATrueFPSCharacter::DestroyInventory()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		return;
	}

	// remove all weapons from inventory and destroy them
	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		if (ATrueFPSWeaponBase* Weapon = Inventory[i])
		{
			RemoveWeapon(Weapon);
			Weapon->Destroy();
		}
	}
}

void ATrueFPSCharacter::ServerEquipWeapon_Implementation(ATrueFPSWeaponBase* Weapon)
{
	EquipWeapon(Weapon);
}

void ATrueFPSCharacter::ServerSetAiming_Implementation(bool bNewAiming)
{
	SetAiming(bNewAiming);
}

void ATrueFPSCharacter::ServerSetRunning_Implementation(bool bNewRunning, bool bToggle)
{
	SetRunning(bNewRunning, bToggle);
}

void ATrueFPSCharacter::BuildPauseReplicationCheckPoints(TArray<FVector>& RelevancyCheckPoints) const
{
	const FBoxSphereBounds Bounds = GetCapsuleComponent()->CalcBounds(GetCapsuleComponent()->GetComponentTransform());
	const FBox BoundingBox = Bounds.GetBox();
	const float XDiff = Bounds.BoxExtent.X * 2;
	const float YDiff = Bounds.BoxExtent.Y * 2;

	RelevancyCheckPoints.Add(BoundingBox.Min);
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X + XDiff, BoundingBox.Min.Y, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X, BoundingBox.Min.Y + YDiff, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Min.X + XDiff, BoundingBox.Min.Y + YDiff, BoundingBox.Min.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X - XDiff, BoundingBox.Max.Y, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X, BoundingBox.Max.Y - YDiff, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(FVector(BoundingBox.Max.X - XDiff, BoundingBox.Max.Y - YDiff, BoundingBox.Max.Z));
	RelevancyCheckPoints.Add(BoundingBox.Max);
}
