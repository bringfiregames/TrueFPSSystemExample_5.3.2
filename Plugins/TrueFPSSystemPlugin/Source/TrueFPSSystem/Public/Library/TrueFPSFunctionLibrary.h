// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Net/RepLayout.h"
#include "TrueFPSFunctionLibrary.generated.h"

#define PRINT(...) if(GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::White, FString::Printf(##__VA_ARGS__))
#define PRINTAUTH(...) if(GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::White, AUTH + ": " + FString::Printf(##__VA_ARGS__))
#define PRINT_COLOR(Color, ...) if(GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, Color, FString::Printf(##__VA_ARGS__))
#define AUTHTOSTRING(bAuth) FString(bAuth ? "Server" : "Client")
#define AUTH AUTHTOSTRING(HasAuthority())
#define PRINTLINE PRINT(TEXT("%s"), *FString(__FUNCTION__))
#define PRINTLINEAUTH PRINT(TEXT("%s: %s"), *AUTH, *FString(__FUNCTION__))

// Default Object: GetClass()->GetDefaultObject<ThisClass>()
#define DEFOBJ GetClass()->GetDefaultObject<ThisClass>()

#define CONCATE_(X, Y) X##Y
#define CONCATE(X, Y) CONCATE_(X, Y)

// Pre-requisite to calling ACCESS_PRIVATE_MEMBER to be able to access a private member. CLASS, MEMBER, TYPES
#define ALLOW_PRIVATE_ACCESS(CLASS, MEMBER, ...) \
	template<typename Only, __VA_ARGS__ CLASS::*Member> \
	struct CONCATE(MEMBER, __LINE__) { friend __VA_ARGS__ CLASS::*Access(Only*) { return Member; } }; \
	template<typename> struct Only_##MEMBER; \
	template<> struct Only_##MEMBER<CLASS> { friend __VA_ARGS__ CLASS::*Access(Only_##MEMBER<CLASS>*); }; \
	template struct CONCATE(MEMBER, __LINE__)<Only_##MEMBER<CLASS>, &CLASS::MEMBER>

// Access private member from an object. Must call ALLOW_PRIVATE_ACCESS in global-scope first
#define ACCESS_PRIVATE_MEMBER(OBJECT, MEMBER) \
	((OBJECT).*Access((Only_##MEMBER<std::remove_reference<decltype(OBJECT)>::type>*)nullptr))

UENUM(BlueprintType)
enum class EBoneSpaceTransform : uint8
{
	ParentBoneSpace		UMETA(DisplayName = "Parent Bone-Space"),
	WorldSpace			UMETA(DisplayName = "World-Space"),
};


DECLARE_DYNAMIC_DELEGATE(FPlaceholderDelegate);



ALLOW_PRIVATE_ACCESS(FAnimInstanceProxy, PoseSnapshots, TArray<FPoseSnapshot>);
class UAnimInstanceAccessor : public UAnimInstance { friend class UTrueFPSFunctionLibrary; };

ALLOW_PRIVATE_ACCESS(FRepLayout, Parents, TArray<FRepParentCmd>);

/**
 * 
 */
UCLASS()
class TRUEFPSSYSTEM_API UTrueFPSFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	template<typename EnumType>
	static FORCEINLINE FString EnumValueToString(const EnumType EnumValue)
	{
		static_assert(TIsEnum<EnumType>::Value, TEXT("EnumValueToString: Input is not an enum type"));
		return StaticEnum<EnumType>()->GetNameStringByIndex((int32)EnumValue);
	}

	template<typename EnumType>
	static FORCEINLINE FName EnumValueName(const EnumType EnumValue)
	{
		return FName(EnumValueToString<EnumType>(EnumValue));
	}

	// Returns nullptr if AnimInstance or AnimInstanceProxy are invalid. Direct access.
	static FORCEINLINE TArray<FPoseSnapshot>* GetPoseSnapshots(class UAnimInstance* AnimInstance)
	{
		if(IsValid(AnimInstance))
			if(FAnimInstanceProxy* Proxy = UAnimInstanceAccessor::GetProxyOnGameThreadStatic<FAnimInstanceProxy>(AnimInstance))
				return &ACCESS_PRIVATE_MEMBER(*Proxy, PoseSnapshots);// Black-magic to access private-member variable
		return nullptr;
	}

	// Caches pose snapshot within the passed in AnimInstance that can be accessed within it using the Pose Snapshot node. If a
	// pose snapshot has already been cached with the same name this will override it with the new pose snapshot.
	UFUNCTION(BlueprintCallable, Meta = (AutoCreateRefTerm = "SnapshotName"), Category = "Weapon System Function Library|Animation")
	static void CachePoseSnapshot(const FPoseSnapshot& PoseSnapshot, class UAnimInstance* AnimInstance, const FName& SnapshotName)
	{
		if(!AnimInstance || !PoseSnapshot.bIsValid)
		{
			if(!AnimInstance) { UE_LOG(LogTemp, Error, TEXT("%s: AnimInstance is invalid"), *FString(__FUNCTION__)); }
			if(!PoseSnapshot.bIsValid) { UE_LOG(LogTemp, Error, TEXT("%s: PoseSnapshot is invalid"), *FString(__FUNCTION__)); }
			return;
		}
		
		TArray<FPoseSnapshot>* ProxySnapshots = GetPoseSnapshots(AnimInstance);
		if(!ProxySnapshots) return;
		
		FPoseSnapshot* Snapshot = ProxySnapshots->FindByPredicate([SnapshotName](const FPoseSnapshot& Snapshot)->bool { return Snapshot.SnapshotName == SnapshotName; });
		if(!Snapshot)
			Snapshot = &(*ProxySnapshots)[ProxySnapshots->AddDefaulted()];
		
		*Snapshot = PoseSnapshot;
		Snapshot->SnapshotName = SnapshotName;
	}

	static const FPoseSnapshot EmptyPose;

	UFUNCTION(BlueprintPure, Meta = (AutoCreateRefTerm = "SnapshotName"), Category = "Weapon System Function Library|Animation")
	static const FPoseSnapshot& GetCachedPoseSnapshot(class UAnimInstance* AnimInstance, const FName& SnapshotName, bool& bOutSuccess)
	{
		bOutSuccess = true;
		if(AnimInstance && SnapshotName.IsValid())
			if(const TArray<FPoseSnapshot>* PoseSnapshots = GetPoseSnapshots(AnimInstance))
				if(const FPoseSnapshot* PoseSnapshot = PoseSnapshots->FindByPredicate([SnapshotName](const FPoseSnapshot& PoseSnapshot)->bool { return PoseSnapshot.SnapshotName == SnapshotName; }))
					return *PoseSnapshot;

		bOutSuccess = false;
		return EmptyPose;
	}

	// Returns true if a snapshot under that name exists and was removed.
	UFUNCTION(BlueprintCallable, Meta = (AutoCreateRefTerm = "SnapshotName"), Category = "Weapon System Function Library|Animation")
	static bool RemoveCachedPoseSnapshot(class UAnimInstance* AnimInstance, const FName& SnapshotName)
	{
		if(AnimInstance && SnapshotName.IsValid())
			if(TArray<FPoseSnapshot>* PoseSnapshots = GetPoseSnapshots(AnimInstance))
				for(int32 i = 0; i < PoseSnapshots->Num(); i++)
					if((*PoseSnapshots)[i].SnapshotName == SnapshotName)
					{
						PoseSnapshots->RemoveAt(i);
						return true;
					}
		// Else fails
		return false;
	}

	// WARNING: Do not place in Anim-Graph
	UFUNCTION(BlueprintPure, Meta = (AutoCreateRefTerm = "SnapshotName", DefaultToSelf = "AnimInstance"), Category = "Weapon System Function Library|Animation")
	static FORCEINLINE bool CachedPoseSnapshotIsValid(class UAnimInstance* AnimInstance, const FName& SnapshotName)
	{
		bool bOutSuccess = false;
		const FPoseSnapshot& PoseSnapshot = GetCachedPoseSnapshot(AnimInstance, SnapshotName, bOutSuccess);
		return bOutSuccess && PoseSnapshot.bIsValid;
	}

	UFUNCTION(BlueprintPure, Meta = (AutoCreateRefTerm = "BoneName"), Category = "Weapon System Function Library|Animation")
	static FORCEINLINE FTransform GetBoneTransformFromAnimationSequence(const FName& BoneName, const class UAnimSequence* AnimationSequence,
		const float ExtractionTime = 0.f, const EBoneSpaceTransform Space = EBoneSpaceTransform::ParentBoneSpace)
	{
		if(!AnimationSequence || !BoneName.IsValid()) return FTransform::Identity;
		
		int32 BoneIndex = AnimationSequence->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(BoneName);
		if(BoneIndex == INDEX_NONE) return FTransform::Identity;

		FTransform BoneTransform;
		AnimationSequence->GetBoneTransform(BoneTransform, BoneIndex, ExtractionTime, true);

		if(Space == EBoneSpaceTransform::WorldSpace)
		{
			while(true)
			{
				const int32 ParentIndex = AnimationSequence->GetSkeleton()->GetReferenceSkeleton().GetParentIndex(BoneIndex);
				if(ParentIndex == INDEX_NONE) break; 

				FTransform ParentTransform;
				AnimationSequence->GetBoneTransform(ParentTransform, ParentIndex, ExtractionTime, true);

				BoneTransform *= ParentTransform;
				BoneIndex = ParentIndex;
			}
		}

		return BoneTransform;
	}

	UFUNCTION(BlueprintCallable, Category = "Weapon System Function Library|Animation")
	static void GetAnimationSequenceAsPoseSnapshot(FPoseSnapshot& OutPoseSnapshot, const class UAnimSequence* AnimSequence, const class USkeletalMeshComponent* TargetMesh, const float ExtractionTime = 0.f)
	{
		if(!AnimSequence || !TargetMesh) return;
		const FReferenceSkeleton& RefSkel = AnimSequence->GetSkeleton()->GetReferenceSkeleton();
	}

	// For replication stuff to avoid calling GetObjectClassRepLayout from the NetDriver which won't compile because it's not exported
	static FORCEINLINE TSharedPtr<FRepLayout> GetObjectClassRepLayout(class UNetDriver* NetDriver, class UClass* Class)
	{
		if(!NetDriver) return nullptr;
		auto& RepLayoutMap = NetDriver->RepLayoutMap;
		TSharedPtr<FRepLayout>* RepLayoutPtr = RepLayoutMap.Find(Class);
		if(!RepLayoutPtr)
		{
			const ECreateRepLayoutFlags Flags = NetDriver->MaySendProperties() ? ECreateRepLayoutFlags::MaySendProperties : ECreateRepLayoutFlags::None;
			RepLayoutPtr = &RepLayoutMap.Add(Class, FRepLayout::CreateFromClass(Class, NetDriver->ServerConnection, Flags));
		}
		return *RepLayoutPtr;
	}
	
	// Calls PreReplication on object and passes in required variables.
	// Must have a function with the exact signature: void PreReplication(IRepChangedPropertyTracker&)
	template<typename UserClass>
	static FORCEINLINE void CallPreReplication(UserClass* Object)
	{
		static_assert(TIsClass<UserClass>::Value, TEXT("UTrueFPSFunctionLibrary::CallPreReplication UserClass is not a valid UCLASS"));
		if(IsValid(Object)) Object->PreReplication(*FindOrCreatePropertyTracker(Object).Get());
	}
	
	// Calls PreReplication on each object and passes in required variables.
	// Must have a function with the exact signature: void PreReplication(IRepChangedPropertyTracker&)
	template<typename UserClass>
	static FORCEINLINE void CallPreReplicationList(const TArray<UserClass*>& Objects)
	{
		static_assert(TIsClass<UserClass>::Value, TEXT("UTrueFPSFunctionLibrary::CallPreReplicationList UserClass is not a valid UCLASS"));
		for(UserClass* const Object : Objects)
			if(IsValid(Object))
				Object->PreReplication(*FindOrCreatePropertyTracker(Object).Get());
	}

protected:
	
	UFUNCTION(BlueprintPure, Meta = (AutoCreateRefTerm = "Class", DeterminesOutputType = "Class", AllowPrivateAccess = "true"), Category = "Weapon System Function Library")
	static FORCEINLINE class UObject* GetDefaultObject(const TSubclassOf<class UObject>& Class) { return Class ? Class->GetDefaultObject() : nullptr; }
	
};
