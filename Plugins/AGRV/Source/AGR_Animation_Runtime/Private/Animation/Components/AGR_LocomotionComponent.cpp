// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Animation/Components/AGR_LocomotionComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Module/AGR_Animation_RuntimeLogs.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_LocomotionComponent)

UAGR_LocomotionComponent::UAGR_LocomotionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    bAutoActivate = false;

    SetIsReplicatedByDefault(true);

    AnimationStates = FGameplayTagContainer{};
    bUpdateAllowedByClient = true;
    RotationMethod = EAGR_RotationMethod::AbsoluteRotation;
    AimMethod = EAGR_AimMethod::FreeLook;
    Pose = FGameplayTag{};
}

void UAGR_LocomotionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UAGR_LocomotionComponent, bUpdateAllowedByClient, COND_InitialOnly);
    DOREPLIFETIME(UAGR_LocomotionComponent, RotationMethod);
    DOREPLIFETIME(UAGR_LocomotionComponent, AimMethod);
    DOREPLIFETIME(UAGR_LocomotionComponent, Pose);
    DOREPLIFETIME(UAGR_LocomotionComponent, AnimationStates);
}

void UAGR_LocomotionComponent::BeginPlay()
{
    Super::BeginPlay();

    APawn* const OwnerPawn = GetOwner<APawn>();
    if(!IsValid(OwnerPawn))
    {
        AGR_LOG(LogAGR_Animation_Runtime, Warning, "Component %s has a non-Pawn owner!", *GetFullName());
        return;
    }

    OwnerPawn->bUseControllerRotationYaw = true;
    const ACharacter* const OwnerCharacter = Cast<ACharacter>(OwnerPawn);
    if(!IsValid(OwnerCharacter))
    {
        return;
    }

    UCharacterMovementComponent* CharacterMovement = OwnerCharacter->GetCharacterMovement();
    if(IsValid(CharacterMovement))
    {
        CharacterMovement->bUseControllerDesiredRotation = false;
        CharacterMovement->bOrientRotationToMovement = false;
    }
}

bool UAGR_LocomotionComponent::IsUpdateAllowedByClient() const
{
    return bUpdateAllowedByClient;
}

void UAGR_LocomotionComponent::SetRotationMethod(const EAGR_RotationMethod InRotationMethod)
{
    if(InRotationMethod == RotationMethod)
    {
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return;
    }

    if(Owner->HasAuthority())
    {
        InternalSetRotationMethod(InRotationMethod);
        return;
    }

    if(IsUpdateAllowedByClient())
    {
        SV_SetRotationMethod(InRotationMethod);
    }
}

EAGR_RotationMethod UAGR_LocomotionComponent::GetRotationMethod() const
{
    return RotationMethod;
}

void UAGR_LocomotionComponent::SetAimMethod(const EAGR_AimMethod InAimMethod)
{
    if(InAimMethod == AimMethod)
    {
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return;
    }

    if(Owner->HasAuthority())
    {
        InternalSetAimMethod(InAimMethod);
        return;
    }

    if(IsUpdateAllowedByClient())
    {
        SV_SetAimMethod(InAimMethod);
    }
}

EAGR_AimMethod UAGR_LocomotionComponent::GetAimMethod() const
{
    return AimMethod;
}

void UAGR_LocomotionComponent::SetPose(const FGameplayTag& InPose)
{
    if(InPose == Pose)
    {
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return;
    }

    if(Owner->HasAuthority())
    {
        InternalSetPose(InPose);
        return;
    }

    if(IsUpdateAllowedByClient())
    {
        SV_SetPose(InPose);
    }
}

const FGameplayTag& UAGR_LocomotionComponent::GetPose() const
{
    return Pose;
}

void UAGR_LocomotionComponent::SetAnimationStates(const FGameplayTagContainer& InAnimationStates)
{
    if(InAnimationStates == AnimationStates)
    {
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return;
    }

    if(Owner->HasAuthority())
    {
        InternalSetAnimationStates(InAnimationStates);
        return;
    }

    if(IsUpdateAllowedByClient())
    {
        SV_SetAnimationStates(InAnimationStates);
    }
}

void UAGR_LocomotionComponent::AddAnimationStates(const FGameplayTagContainer& InAnimationStatesToAdd)
{
    if(InAnimationStatesToAdd.IsEmpty())
    {
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return;
    }

    FGameplayTagContainer NewAnimationStates = AnimationStates;
    NewAnimationStates.AppendTags(InAnimationStatesToAdd);

    if(NewAnimationStates == AnimationStates)
    {
        // AppendTags effectively didn't change the container, i.e. all tags to add were set in the container already.
        return;
    }

    if(Owner->HasAuthority())
    {
        InternalSetAnimationStates(MoveTemp(NewAnimationStates));
        return;
    }

    if(IsUpdateAllowedByClient())
    {
        SV_SetAnimationStates(MoveTemp(NewAnimationStates));
    }
}

void UAGR_LocomotionComponent::RemoveAnimationStates(const FGameplayTagContainer& InAnimationStatesToRemove)
{
    if(InAnimationStatesToRemove.IsEmpty())
    {
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return;
    }

    FGameplayTagContainer NewAnimationStates = AnimationStates;
    NewAnimationStates.RemoveTags(InAnimationStatesToRemove);

    if(NewAnimationStates == AnimationStates)
    {
        // RemoveTags effectively didn't change the container, i.e. all tags to remove weren't set in the container.
        return;
    }

    if(Owner->HasAuthority())
    {
        InternalSetAnimationStates(MoveTemp(NewAnimationStates));
        return;
    }

    if(IsUpdateAllowedByClient())
    {
        SV_SetAnimationStates(MoveTemp(NewAnimationStates));
    }
}

const FGameplayTagContainer& UAGR_LocomotionComponent::GetAnimationStates() const
{
    return AnimationStates;
}

void UAGR_LocomotionComponent::SetUpdateAllowedByClient(const bool bInUpdateAllowedByClient)
{
    // Check if running in construction script/constructor (UActorComponent::SetAutoActivate does the same)
    if(!bRegistered || IsOwnerRunningUserConstructionScript())
    {
        bUpdateAllowedByClient = bInUpdateAllowedByClient;
    }
    else
    {
        AGR_LOG(
            LogAGR_Animation_Runtime,
            Warning,
            "SetClientCanUpdateStates called on component %s after construction!",
            *GetFullName());
    }
}

void UAGR_LocomotionComponent::InternalSetRotationMethod(const EAGR_RotationMethod InRotationMethod)
{
    const EAGR_RotationMethod OldRotationMethod = RotationMethod;
    RotationMethod = InRotationMethod;
    OnRep_RotationMethod(OldRotationMethod);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_LocomotionComponent::OnRep_RotationMethod(const EAGR_RotationMethod InOldRotationMethod)
{
    OnRotationMethodUpdated.Broadcast(InOldRotationMethod, RotationMethod);
}

void UAGR_LocomotionComponent::SV_SetRotationMethod_Implementation(
    const EAGR_RotationMethod InRotationMethod)
{
    if(InRotationMethod == RotationMethod)
    {
        return;
    }

    if(!IsUpdateAllowedByClient())
    {
        return;
    }

    InternalSetRotationMethod(InRotationMethod);
}

void UAGR_LocomotionComponent::InternalSetAimMethod(const EAGR_AimMethod InAimMethod)
{
    const EAGR_AimMethod OldAimMethod = AimMethod;
    AimMethod = InAimMethod;
    OnRep_AimMethod(OldAimMethod);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_LocomotionComponent::OnRep_AimMethod(const EAGR_AimMethod InOldAimMethod)
{
    OnAimMethodUpdated.Broadcast(InOldAimMethod, AimMethod);
}

void UAGR_LocomotionComponent::SV_SetAimMethod_Implementation(const EAGR_AimMethod InAimMethod)
{
    if(InAimMethod == AimMethod)
    {
        return;
    }

    if(!IsUpdateAllowedByClient())
    {
        return;
    }

    InternalSetAimMethod(InAimMethod);
}

void UAGR_LocomotionComponent::InternalSetPose(const FGameplayTag& InPose)
{
    const FGameplayTag OldPose = Pose;
    Pose = InPose;
    OnRep_Pose(OldPose);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_LocomotionComponent::OnRep_Pose(const FGameplayTag& InOldPose)
{
    OnPoseUpdated.Broadcast(InOldPose, Pose);
}

void UAGR_LocomotionComponent::SV_SetPose_Implementation(const FGameplayTag& InPose)
{
    if(InPose == Pose)
    {
        return;
    }

    if(!IsUpdateAllowedByClient())
    {
        return;
    }

    InternalSetPose(InPose);
}

void UAGR_LocomotionComponent::InternalSetAnimationStates(const FGameplayTagContainer& InAnimationStates)
{
    const FGameplayTagContainer OldAnimationStates = AnimationStates;
    AnimationStates = InAnimationStates;
    OnRep_AnimationStates(OldAnimationStates);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_LocomotionComponent::OnRep_AnimationStates(const FGameplayTagContainer& InOldAnimationStates)
{
    if(InOldAnimationStates == AnimationStates)
    {
        return;
    }

    const FGameplayTagContainer MatchingTags = InOldAnimationStates.FilterExact(AnimationStates);

    FGameplayTagContainer TagsAdded = AnimationStates;
    TagsAdded.RemoveTags(MatchingTags);

    FGameplayTagContainer TagsRemoved = InOldAnimationStates;
    TagsRemoved.RemoveTags(MatchingTags);

    OnAnimationStatesUpdated.Broadcast(TagsAdded, TagsRemoved);
}

void UAGR_LocomotionComponent::SV_SetAnimationStates_Implementation(const FGameplayTagContainer& InAnimationStates)
{
    if(InAnimationStates == AnimationStates)
    {
        return;
    }

    if(!IsUpdateAllowedByClient())
    {
        return;
    }

    InternalSetAnimationStates(InAnimationStates);
}