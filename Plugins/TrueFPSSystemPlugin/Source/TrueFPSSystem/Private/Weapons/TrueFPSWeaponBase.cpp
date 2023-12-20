// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/TrueFPSWeaponBase.h"

#include "TrueFPSSystem.h"
#include "Character/TrueFPSCharacterInterface.h"
#include "GameFramework/Character.h"
#include "VisualizationMacros.h"
#include "Bots/TrueFPSAIController.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/Attachments/TrueFPSSightsAttachment.h"
#include "Weapons/Attachments/TrueFPSWeaponAttachmentBase.h"
#include "Weapons/Attachments/TrueFPSWeaponAttachmentPoint.h"

ATrueFPSWeaponBase::ATrueFPSWeaponBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh1P"));
	Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->CastShadow = true;
	Mesh1P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = Mesh1P;

	Mesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh3P"));
	Mesh3P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh3P->bReceivesDecals = false;
	Mesh3P->CastShadow = true;
	Mesh3P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh3P->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	Mesh3P->SetupAttachment(Mesh1P);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

void ATrueFPSWeaponBase::BeginPlay()
{
	check(IsValid(Settings));
	
#if WITH_EDITORONLY_DATA
	if (OriginWidgetComp)
	{
		OriginWidgetComp->DestroyComponent();
		OriginWidgetComp = nullptr;
	}
	
	if (AimOffsetComp)
	{
		AimOffsetComp->DestroyComponent();
		AimOffsetComp = nullptr;
	}
#endif// WITH_EDITORONLY_DATA
	
	Super::BeginPlay();

	// By default, the sights relative transform should equal whatever
	// the GetDefaultSightsRelativeTransform implementation returns.
	TargetSightsRelativeTransform = GetDefaultSightsRelativeTransform();
}

void ATrueFPSWeaponBase::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleRecoil(DeltaTime);
	
	RefreshCameraRecoil(DeltaTime);

	// Smooth sights toggling
	State.SightsRelativeTransform = UKismetMathLibrary::TInterpTo(State.SightsRelativeTransform, TargetSightsRelativeTransform, DeltaTime, 8.f);

	// Smooth wall offset
	State.WallOffsetTransformAlpha = UKismetMathLibrary::FInterpTo(State.WallOffsetTransformAlpha, CalculateWallOffsetTransformAlpha(), DeltaTime, 8.f);
}

void ATrueFPSWeaponBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	DetachMeshFromPawn();
}

void ATrueFPSWeaponBase::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}

void ATrueFPSWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME( ThisClass, MyPawn );
	DOREPLIFETIME( ThisClass, TargetSightsRelativeTransform );

	DOREPLIFETIME_CONDITION( ThisClass, BurstCounter, COND_SkipOwner );
}

void ATrueFPSWeaponBase::OnEquip(const ATrueFPSWeaponBase* LastWeapon)
{
	AttachMeshToPawn();

	State.bPendingEquip = true;
	DetermineWeaponState();

	// Only play animation if last weapon is valid
	if (LastWeapon)
	{
		float Duration = PlayWeaponAnimation(Settings->EquipAnim);
		if (Duration <= 0.0f)
		{
			// failsafe
			Duration = 0.5f;
		}
		State.EquipStartedTime = GetWorld()->GetTimeSeconds();
		State.EquipDuration = Duration;

		GetWorldTimerManager().SetTimer(State.TimerHandle_OnEquipFinished, this, &ThisClass::OnEquipFinished, Duration, false);
	}
	else
	{
		OnEquipFinished();
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		PlayWeaponSound(Settings->EquipSound);
	}
}

void ATrueFPSWeaponBase::OnEquipFinished()
{
	AttachMeshToPawn();

	State.bIsEquipped = true;
	State.bPendingEquip = false;

	// Determine the state so that the can reload checks will work
	DetermineWeaponState(); 
}

void ATrueFPSWeaponBase::OnUnEquip()
{
	DetachMeshFromPawn();
	State.bIsEquipped = false;
	StopFire();

	if (State.bPendingEquip)
	{
		StopWeaponAnimation(Settings->EquipAnim);
		State.bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(State.TimerHandle_OnEquipFinished);
	}

	DetermineWeaponState();
}

void ATrueFPSWeaponBase::OnEnterInventory(ACharacter* NewOwner)
{
	SetOwningPawn(NewOwner);
}

void ATrueFPSWeaponBase::OnLeaveInventory()
{
	if (IsAttachedToPawn())
	{
		OnUnEquip();
	}

	if (GetLocalRole() == ROLE_Authority)
	{
		SetOwningPawn(nullptr);
	}
}

bool ATrueFPSWeaponBase::IsEquipped() const
{
	return State.bIsEquipped;
}

bool ATrueFPSWeaponBase::IsAttachedToPawn() const
{
	return State.bIsEquipped || State.bPendingEquip;
}

void ATrueFPSWeaponBase::StartFire()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!State.bWantsToFire)
	{
		State.bWantsToFire = true;
		DetermineWeaponState();
	}
}

void ATrueFPSWeaponBase::StopFire()
{
	if ((GetLocalRole() < ROLE_Authority) && MyPawn && MyPawn->IsLocallyControlled())
	{
		ServerStopFire();
	}

	if (State.bWantsToFire)
	{
		State.bWantsToFire = false;
		DetermineWeaponState();
	}
}

void ATrueFPSWeaponBase::ToggleSights()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerToggleSights();
	}
	else
	{
		TArray<ATrueFPSSightsAttachment*> AllSights;
		GetAttachments(AllSights);

		if (State.CurrentSights.IsValid())
		{
			const int32 CurrentIndex = AllSights.Find(State.CurrentSights.Get());
			State.CurrentSights = CurrentIndex != INDEX_NONE && AllSights.IsValidIndex(CurrentIndex + 1) ? AllSights[CurrentIndex + 1] : nullptr;
		}
		else if (AllSights.IsValidIndex(0))
		{
			State.CurrentSights = AllSights[0];
		}
		
		TargetSightsRelativeTransform = State.CurrentSights.IsValid() ? State.CurrentSights->GetSightsWorldTransform().GetRelativeTransform(GetActorTransform()) : GetDefaultSightsRelativeTransform();
	}
}

bool ATrueFPSWeaponBase::CanFire() const
{
	const bool bCanFire = MyPawn && MyPawn->Implements<UTrueFPSCharacterInterface>() && ITrueFPSCharacterInterface::Execute_TrueFPSInterface_CanFire(MyPawn);
	const bool bStateOKToFire = ( ( State.CurrentState ==  EWeaponState::Idle ) || ( State.CurrentState == EWeaponState::Firing) );	
	return (( bCanFire == true ) && ( bStateOKToFire == true ));
}

bool ATrueFPSWeaponBase::CanAim() const
{
	const bool bIsCloseToWall = IsCloseToWall();
	return (bIsCloseToWall == false);
}

USkeletalMeshComponent* ATrueFPSWeaponBase::GetWeaponMesh() const
{
	// return (MyPawn != nullptr && IsFirstPerson()) ? Mesh1P : Mesh3P;
	return Mesh1P;
}

bool ATrueFPSWeaponBase::IsFirstPerson() const
{
	if (!MyPawn) return false;
	if (!MyPawn->Implements<UTrueFPSCharacterInterface>()) return false;
	return MyPawn->IsLocallyControlled() && ITrueFPSCharacterInterface::Execute_TrueFPSInterface_IsAlive(MyPawn);
}

void ATrueFPSWeaponBase::SetOwningPawn(ACharacter* NewOwner)
{
	if (MyPawn != NewOwner)
	{
		SetInstigator(NewOwner);
		MyPawn = NewOwner;
		// net owner for RPC calls
		SetOwner(NewOwner);
	}
}

float ATrueFPSWeaponBase::GetEquipStartedTime() const
{
	return State.EquipStartedTime;
}

float ATrueFPSWeaponBase::GetEquipDuration() const
{
	return State.EquipDuration;
}

void ATrueFPSWeaponBase::GetAttachments(TArray<ATrueFPSWeaponAttachmentBase*>& OutAttachments) const
{
	TArray<UTrueFPSWeaponAttachmentPoint*> AttachmentPoints;
	GetAttachmentPoints(AttachmentPoints);

	for (const UTrueFPSWeaponAttachmentPoint* AttachmentPoint : AttachmentPoints)
		if (ATrueFPSWeaponAttachmentBase* Attachment = AttachmentPoint->GetAttachment())
			OutAttachments.Add(Attachment);
}

template <typename T>
void ATrueFPSWeaponBase::GetAttachments(TArray<T*>& OutAttachments) const
{
	static_assert(TIsClass<T>::Value, TEXT("ATrueFPSWeaponBase::GetAttachments template parameter is not a class type"));

	TArray<UTrueFPSWeaponAttachmentPoint*> AttachmentPoints;
	GetAttachmentPoints(AttachmentPoints);

	for (const UTrueFPSWeaponAttachmentPoint* AttachmentPoint : AttachmentPoints)
		if (T* Attachment = Cast<T>(AttachmentPoint->GetAttachment()))
			OutAttachments.Add(Attachment);
}

void ATrueFPSWeaponBase::GetAttachmentsOfClass(TArray<ATrueFPSWeaponAttachmentBase*>& OutAttachments, const TSubclassOf<ATrueFPSWeaponAttachmentBase>& Class) const
{
	if(!Class) return;
	
	TArray<UTrueFPSWeaponAttachmentPoint*> AttachmentPoints;
	GetAttachmentPoints(AttachmentPoints);

	for (const UTrueFPSWeaponAttachmentPoint* AttachmentPoint : AttachmentPoints)
		if (IsValid(AttachmentPoint->GetAttachment()) && AttachmentPoint->GetAttachment()->IsA(Class))
			OutAttachments.Add(AttachmentPoint->GetAttachment());
}

void ATrueFPSWeaponBase::GetAttachmentPoints(TArray<UTrueFPSWeaponAttachmentPoint*>& OutAttachmentPoints) const
{
	GetComponents<UTrueFPSWeaponAttachmentPoint>(OutAttachmentPoints);
}

void ATrueFPSWeaponBase::ServerStartFire_Implementation()
{
	StartFire();
}

void ATrueFPSWeaponBase::ServerStopFire_Implementation()
{
	StopFire();
}

void ATrueFPSWeaponBase::ServerToggleSights_Implementation()
{
	ToggleSights();
}

void ATrueFPSWeaponBase::OnRep_MyPawn()
{
	if (MyPawn)
	{
		OnEnterInventory(MyPawn);
	}
	else
	{
		OnLeaveInventory();
	}
}

void ATrueFPSWeaponBase::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void ATrueFPSWeaponBase::SimulateWeaponFire()
{
	if (GetLocalRole() == ROLE_Authority && State.CurrentState != EWeaponState::Firing)
	{
		return;
	}

	if (!Settings->bLoopedFireAnim || !State.bPlayingFireAnim)
	{
		PlayWeaponAnimation(Settings->FireAnim);
		State.bPlayingFireAnim = true;
	}

	if (Settings->bLoopedFireSound)
	{
		if (!State.FireAC.IsValid())
		{
			State.FireAC = PlayWeaponSound(Settings->FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(Settings->FireSound);
	}

	// Adding Recoil Animation
	AddRecoilInstance(Settings->Recoil);

	if (APlayerController* PC = (MyPawn != nullptr) ? Cast<APlayerController>(MyPawn->Controller) : nullptr; PC != nullptr && PC->IsLocalController())
	{
		ApplyCameraRecoil();
		
		if (Settings->FireCameraShake != nullptr)
		{
			PC->ClientStartCameraShake(Settings->FireCameraShake, 1);
		}

		if (Settings->FireForceFeedback != nullptr/* && PC->IsVibrationEnabled()*/) // TODO: Setup Vibration Configurable
		{
			FForceFeedbackParameters FFParams;
			FFParams.Tag = "Weapon";
			PC->ClientPlayForceFeedback(Settings->FireForceFeedback, FFParams);
		}
	}
}

void ATrueFPSWeaponBase::StopSimulatingWeaponFire()
{
	if (Settings && Settings->bLoopedFireAnim && State.bPlayingFireAnim)
	{
		StopWeaponAnimation(Settings->FireAnim);
		State.bPlayingFireAnim = false;
	}

	if (Settings && State.FireAC.IsValid())
	{
		State.FireAC->FadeOut(0.1f, 0.0f);
		State.FireAC = nullptr;

		PlayWeaponSound(Settings->FireFinishSound);
	}
}

void ATrueFPSWeaponBase::ServerHandleFiring_Implementation()
{
	const bool bShouldIncrementBurstCounter = CanFire();

	HandleFiring();

	if (bShouldIncrementBurstCounter)
	{
		// update firing FX on remote clients
		BurstCounter++;
	}
}

void ATrueFPSWeaponBase::HandleReFiring()
{
	// Update TimerIntervalAdjustment
	const UWorld* MyWorld = GetWorld();

	const float SlackTimeThisFrame = FMath::Max(0.0f, (MyWorld->TimeSeconds - State.LastFireTime) - Settings->TimeBetweenShots);

	if (Settings->bAllowAutomaticWeaponCatchup)
	{
		State.TimerIntervalAdjustment -= SlackTimeThisFrame;
	}

	HandleFiring();
}

void ATrueFPSWeaponBase::HandleFiring()
{
	if (CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			FireWeapon();
			
			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
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

		// setup re-fire timer
		State.bRefiring = (State.CurrentState == EWeaponState::Firing && Settings->TimeBetweenShots > 0.0f);
		if (State.bRefiring)
		{
			GetWorldTimerManager().SetTimer(State.TimerHandle_HandleFiring, this, &ThisClass::HandleReFiring, FMath::Max<float>(Settings->TimeBetweenShots + State.TimerIntervalAdjustment, SMALL_NUMBER), false);
			State.TimerIntervalAdjustment = 0.f;
		}
	}

	State.LastFireTime = GetWorld()->GetTimeSeconds();
}

void ATrueFPSWeaponBase::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (State.LastFireTime > 0 && Settings->TimeBetweenShots > 0.0f &&
		State.LastFireTime + Settings->TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(State.TimerHandle_HandleFiring, this, &ThisClass::HandleFiring, State.LastFireTime + Settings->TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void ATrueFPSWeaponBase::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	//if (GetNetMode() != NM_DedicatedServer)
	//{
	StopSimulatingWeaponFire();
	//}
	
	GetWorldTimerManager().ClearTimer(State.TimerHandle_HandleFiring);
	State.bRefiring = false;

	// reset firing interval adjustment
	State.TimerIntervalAdjustment = 0.0f;
}

void ATrueFPSWeaponBase::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = State.CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	State.CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void ATrueFPSWeaponBase::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (State.bIsEquipped)
	{
		if ( ( State.bWantsToFire == true ) && ( CanFire() == true ))
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

void ATrueFPSWeaponBase::HandleRecoil(const float DeltaSeconds)
{
	FRecoilInstance& RecoilInstance = State.CurrentRecoilInstance;
		
	const float CurrentTime = RecoilInstance.CurrentTime += DeltaSeconds * RecoilInstance.PlayRate;

	if (CurrentTime >= RecoilInstance.Lifetime)
	{
		State.RecoilOffset = FTransform::Identity;
		return;
	}

	const FVector CurveRotationVector = RecoilInstance.RotationCurve->GetVectorValue(CurrentTime);
	const FRotator CurveRotation(CurveRotationVector.Y, CurveRotationVector.Z, CurveRotationVector.X);
	const FVector CurveLocation(RecoilInstance.LocationCurve->GetVectorValue(CurrentTime));
	const FTransform CurrentValue(CurveRotation, CurveLocation);

	State.RecoilOffset = CurrentValue;
}

void ATrueFPSWeaponBase::ApplyCameraRecoil()
{
	if (!MyPawn) return;
	const APlayerController* PC = MyPawn->GetController<APlayerController>();
	if (!PC) return;
	
	if (IsValid(Settings->CameraRecoilCurve))
	{
		State.bResetCameraRecoil = false;
		State.CameraRecoilCurrentTime += Settings->CameraRecoilProgressRate;

		const FVector RecoilVec = Settings->CameraRecoilCurve->GetVectorValue(State.CameraRecoilCurrentTime) * Settings->CameraRecoilMultiplier;

		State.TargetCameraRecoilAmount += FVector2D(RecoilVec.X, RecoilVec.Y);
		State.bShouldUpdateCameraRecoil = true;

		GetWorldTimerManager().ClearTimer(State.TimerHandle_CameraRecoilRecovery);
		GetWorldTimerManager().SetTimer(State.TimerHandle_CameraRecoilRecovery, this, &ThisClass::ResetCameraRecoil, Settings->CameraRecoilRecoveryTime);
	}
}

void ATrueFPSWeaponBase::ResetCameraRecoil()
{
	State.bResetCameraRecoil = true;
	State.CameraRecoilCurrentTime = 0.f;
	State.bShouldUpdateCameraRecoil = true;
}

void ATrueFPSWeaponBase::RefreshCameraRecoil(float DeltaTime)
{
	if (!MyPawn) return;
	if (!MyPawn->Implements<UTrueFPSCharacterInterface>()) return;
	APlayerController* PC = MyPawn->GetController<APlayerController>();
	if (!PC) return;

	if (!State.bShouldUpdateCameraRecoil) return;

	const FRotator ControllerInput = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetLastUpdatedRotationValues(MyPawn);

	const FVector2D RecoverySubtract = FVector2D(
		ClampCameraRecoilRecovery(ControllerInput.Yaw, State.TargetCameraRecoilRecoveryAmount.X),
		ClampCameraRecoilRecovery(ControllerInput.Pitch, State.TargetCameraRecoilRecoveryAmount.Y));

	State.TargetCameraRecoilRecoveryAmount -= RecoverySubtract;

	if (State.bResetCameraRecoil)
	{
		const FVector2D TargetRecoveryInterp = UKismetMathLibrary::Vector2DInterpTo(State.TargetCameraRecoilRecoveryAmount, FVector2D::ZeroVector, DeltaTime, 4.f);
		const FVector2D TargetRecovery = State.TargetCameraRecoilRecoveryAmount - TargetRecoveryInterp;
		
		AddCameraRecoilControlRotation(TargetRecovery);

		State.TargetCameraRecoilRecoveryAmount = TargetRecoveryInterp;
		if (TargetRecoveryInterp.IsZero()) State.bShouldUpdateCameraRecoil = false;
	}
	else
	{
		const FVector2D TargetRecoilInterp = UKismetMathLibrary::Vector2DInterpTo(State.TargetCameraRecoilAmount, FVector2D::ZeroVector, DeltaTime, Settings->CameraRecoilRecoverySpeed);
		const FVector2D TargetRecoil = State.TargetCameraRecoilAmount - TargetRecoilInterp;

		AddCameraRecoilControlRotation(TargetRecoil);

		State.TargetCameraRecoilRecoveryAmount = State.TargetCameraRecoilRecoveryAmount + FVector2D(-TargetRecoil.X, -TargetRecoil.Y);
		State.TargetCameraRecoilAmount = TargetRecoilInterp;

		if (TargetRecoilInterp.IsZero()) State.bShouldUpdateCameraRecoil = false;
	}
}

float ATrueFPSWeaponBase::ClampCameraRecoilRecovery(const float Value, const float Clamp) const
{
	if (Clamp > 0.f) return FMath::Clamp(Value, 0.f, Clamp);
	return FMath::Clamp(Value, Clamp, 0.f);
}

void ATrueFPSWeaponBase::AddCameraRecoilControlRotation(const FVector2D& Amount) const
{
	MyPawn->AddControllerYawInput(Amount.X);
	MyPawn->AddControllerPitchInput(Amount.Y);
}

void ATrueFPSWeaponBase::AttachMeshToPawn()
{
	if (MyPawn)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		const FName AttachPoint = Settings->AttachPoint;
		if(MyPawn->IsLocallyControlled() == true)
		{
			USkeletalMeshComponent* PawnMesh1p = MyPawn->Implements<UTrueFPSCharacterInterface>() ? ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetSpecificPawnMesh(MyPawn, true) : MyPawn->GetMesh();
			USkeletalMeshComponent* PawnMesh3p = MyPawn->Implements<UTrueFPSCharacterInterface>() ? ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetSpecificPawnMesh(MyPawn, false) : MyPawn->GetMesh();
			Mesh1P->SetHiddenInGame(false);
			Mesh3P->SetHiddenInGame(true);
			Mesh1P->AttachToComponent(PawnMesh1p, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
			Mesh3P->AttachToComponent(PawnMesh3p, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
			Mesh1P->SetRelativeTransform(Settings->PlacementTransform);
			Mesh3P->SetRelativeTransform(Settings->PlacementTransform);
		}
		else
		{
			USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
			USkeletalMeshComponent* UsePawnMesh = MyPawn->GetMesh();
			UseWeaponMesh->AttachToComponent(UsePawnMesh, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
			UseWeaponMesh->SetRelativeTransform(Settings->PlacementTransform);
			UseWeaponMesh->SetHiddenInGame( false );
		}

		TArray<ATrueFPSWeaponAttachmentBase*> CurrentAttachments;
		GetAttachments(CurrentAttachments);
		for (const auto& CurrentAttachment : CurrentAttachments)
			if (CurrentAttachment)
				CurrentAttachment->SetVisibility(true);
	}
}

void ATrueFPSWeaponBase::DetachMeshFromPawn() const
{
	Mesh1P->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	Mesh1P->SetHiddenInGame(true);

	Mesh3P->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	Mesh3P->SetHiddenInGame(true);

	TArray<ATrueFPSWeaponAttachmentBase*> CurrentAttachments;
	GetAttachments(CurrentAttachments);
	for (const auto& CurrentAttachment : CurrentAttachments)
		if (CurrentAttachment)
			CurrentAttachment->SetVisibility(false);
}

UAudioComponent* ATrueFPSWeaponBase::PlayWeaponSound(USoundBase* Sound)
{
	UAudioComponent* AC = nullptr;
	if (Sound && MyPawn)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, MyPawn->GetRootComponent(), Settings->SoundsAttachPoint);
	}

	return AC;
}

float ATrueFPSWeaponBase::PlayWeaponAnimation(const FWeaponAnim& Animation) const
{
	float Duration = 0.0f;
	if (MyPawn)
	{
		const bool bIsFirstPerson = IsFirstPerson();

		if (UAnimMontage* PawnAnim = bIsFirstPerson ? Animation.Pawn1P : Animation.Pawn3P)
		{
			Duration = MyPawn->PlayAnimMontage(PawnAnim);
		}

		if (UAnimMontage* WeaponAnim = bIsFirstPerson ? Animation.Weapon1P : Animation.Weapon3P)
		{
			GetWeaponMesh()->PlayAnimation(WeaponAnim, false);
		}
	}

	return Duration;
}

void ATrueFPSWeaponBase::StopWeaponAnimation(const FWeaponAnim& Animation) const
{
	if (MyPawn)
	{
		const bool bIsFirstPerson = IsFirstPerson();

		if (UAnimMontage* UseAnim = bIsFirstPerson ? Animation.Pawn1P : Animation.Pawn3P)
		{
			MyPawn->StopAnimMontage(UseAnim);
		}
	}
}

FVector ATrueFPSWeaponBase::GetAdjustedAim() const
{
	const APlayerController* const PlayerController = GetInstigatorController<APlayerController>();
	FVector FinalAim = FVector::ZeroVector;
	// If we have a player controller use it for the aim
	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (GetInstigator())
	{
		// Now see if we have an AI controller - we will want to get the aim from there if we do
		if(const ATrueFPSAIController* AIController = MyPawn ? Cast<ATrueFPSAIController>(MyPawn->Controller) : nullptr; AIController != nullptr )
		{
			FinalAim = AIController->GetControlRotation().Vector();
		}
		else
		{			
			FinalAim = GetInstigator()->GetBaseAimRotation().Vector();
		}
	}

	return FinalAim;
}

FVector ATrueFPSWeaponBase::GetCameraAim() const
{
	const APlayerController* const PlayerController = GetInstigatorController<APlayerController>();
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (GetInstigator())
	{
		FinalAim = GetInstigator()->GetBaseAimRotation().Vector();		
	}

	return FinalAim;
}

FVector ATrueFPSWeaponBase::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	const APlayerController* PC = MyPawn ? Cast<APlayerController>(MyPawn->Controller) : nullptr;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((GetInstigator()->GetActorLocation() - OutStartTrace) | AimDir);
	}

	return OutStartTrace;
}

FTransform ATrueFPSWeaponBase::GetOffsetTransform() const
{
	const FTransform DefaultOffsetTransform = State.RecoilOffset * State.OffsetTransform;
	
	return UKismetMathLibrary::TLerp(DefaultOffsetTransform, DefaultOffsetTransform * Settings->WallOffsetTransform, GetWallOffsetTransformAlpha());
}

FTransform ATrueFPSWeaponBase::GetDefaultSightsRelativeTransform() const
{
	return FTransform(FVector(-Settings->AimOffset, 0.f, 0.f)) * GetWeaponMesh()->GetSocketTransform(Settings->SightSocketName, RTS_Actor);
}

FTransform ATrueFPSWeaponBase::GetSightsWorldTransform() const
{
	return State.SightsRelativeTransform * GetActorTransform();
}

FTransform ATrueFPSWeaponBase::GetOriginRelativeTransform() const
{
	const FQuat Rot(Settings->OriginRelativeTransform.GetRotation());
	return FTransform(Rot, Rot.RotateVector(Settings->OriginRelativeTransform.GetLocation()), Settings->OriginRelativeTransform.GetScale3D());
}

FTransform ATrueFPSWeaponBase::GetOriginWorldTransform() const
{
	return GetOriginRelativeTransform() * GetActorTransform();
}

FTransform ATrueFPSWeaponBase::GetOrientationRelativeTransform(const float AimingValue) const
{
	if (AimingValue <= 0.f) return GetOriginRelativeTransform();

	// Get the OriginRelativeTransform value with it's origin-space
	// Y location being equal to the sights origin-space Y location.
	FTransform OriginCenteredTransform = GetOriginRelativeTransform();
	OriginCenteredTransform.SetLocation(OriginCenteredTransform.GetLocation() +
		OriginCenteredTransform.GetRotation().RotateVector(FVector(0.f, Settings->OriginRelativeTransform.GetRotation().UnrotateVector(State.SightsRelativeTransform.GetLocation()).Y, 0.f)));

	// Get the centered origin position rotated about the sights transform
	const FTransform SightsOrigin(State.SightsRelativeTransform.GetRotation(),
		(OriginCenteredTransform.GetRelativeTransform(State.SightsRelativeTransform) * State.SightsRelativeTransform.GetRotation() * Settings->OriginRelativeTransform.GetRotation().Inverse() * State.SightsRelativeTransform).GetLocation());

	// Blend between the standard origin transform and the
	// sights origin by the aiming value.
	FTransform BlendedTransform;
	BlendedTransform.Blend(GetOriginRelativeTransform(), SightsOrigin, AimingValue);
	return BlendedTransform;
}

FTransform ATrueFPSWeaponBase::GetOrientationWorldTransform(const float AimingValue) const
{
	return GetOrientationRelativeTransform(AimingValue) * GetActorTransform();
}

float ATrueFPSWeaponBase::CalculateWallOffsetTransformAlpha() const
{
	if (!GetPawnOwner()) return 0.f;
	if (!GetPawnOwner()->Implements<UTrueFPSCharacterInterface>()) return 0.f;
	
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(GetWallOffsetTransformAlpha), false, this);
	TraceParams.AddIgnoredActor(GetInstigator());
	
	const FVector StartTrace = ITrueFPSCharacterInterface::Execute_TrueFPSInterface_GetFirstPersonCameraTarget(GetPawnOwner());
	const FVector EndTrace = StartTrace + GetPawnOwner()->GetBaseAimRotation().Vector() * Settings->WallOffsetTransformMaxDistance;

	FHitResult Hit(ForceInit);
	if (GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_Visibility, TraceParams))
	{
		const float WallMinDist = Settings->WallOffsetTransformMinDistance;
		const float WallMaxDist = Settings->WallOffsetTransformMaxDistance;
	
		return 1.f - FMath::Clamp((Hit.Distance - WallMinDist) / (WallMaxDist - WallMinDist), 0.f, 1.f);
	}

	return 0.f;
}

#if WITH_EDITORONLY_DATA
void ATrueFPSWeaponBase::RegisterAllComponents()
{
	Super::RegisterAllComponents();

	if (!IsValid(Settings)) return;
	
	if(!OriginWidgetComp)
	{
		if(bShowOriginWidget)
		{
			OriginWidgetComp = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transactional | RF_TextExportTransient);
			if(OriginWidgetComp)
			{
				UStaticMesh* const WidgetMesh(OriginVisualizationMesh ? OriginVisualizationMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Niagara/DefaultAssets/S_Gnomon.S_Gnomon")));
				
				OriginWidgetComp->SetupAttachment(RootComponent);
				OriginWidgetComp->SetStaticMesh(WidgetMesh);
				OriginWidgetComp->SetUsingAbsoluteScale(true);
				OriginWidgetComp->SetIsVisualizationComponent(true);
				OriginWidgetComp->SetCastShadow(false);
				OriginWidgetComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
				OriginWidgetComp->CreationMethod = EComponentCreationMethod::Instance;
				OriginWidgetComp->SetComponentTickEnabled(false);
				OriginWidgetComp->SetHiddenInGame(true);
				OriginWidgetComp->SetRelativeTransform(FTransform(
					Settings->OriginRelativeTransform.GetRotation(),
					Settings->OriginRelativeTransform.GetRotation().RotateVector(
					Settings->OriginRelativeTransform.GetLocation()),
					Settings->OriginRelativeTransform.GetScale3D()));
				OriginWidgetComp->RegisterComponentWithWorld(GetWorld());
			}
		}
	}
	else if(!bShowOriginWidget)
	{
		OriginWidgetComp->DestroyComponent();
		OriginWidgetComp = nullptr;
	}

	STATIC_MESH_VISUALIZER_ACTOR_IMPLEMENTATION(AimOffset,
		AimOffsetComp->SetRelativeTransform(GetDefaultSightsRelativeTransform());
	);
}
#endif// WITH_EDITORONLY_DATA
