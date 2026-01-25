// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Animation/Lib/AGR_AnimationFunctionLibrary.h"

#include "Animation/Components/AGR_LocomotionComponent.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_AnimationFunctionLibrary)

UAGR_LocomotionComponent* UAGR_AnimationFunctionLibrary::GetLocomotionComponent(const AActor* InActor)
{
    if(!IsValid(InActor))
    {
        return nullptr;
    }

    UAGR_LocomotionComponent* const Locomotion = InActor->GetComponentByClass<UAGR_LocomotionComponent>();
    if(IsValid(Locomotion))
    {
        return Locomotion;
    }

    const AActor* const Instigator = InActor->GetInstigator();
    if(!IsValid(Instigator))
    {
        return nullptr;
    }

    return Instigator->GetComponentByClass<UAGR_LocomotionComponent>();
}