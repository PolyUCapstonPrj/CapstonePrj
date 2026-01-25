// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Animation/AnimNotifies/AGR_TraceAnimNotifyState.h"
#include "Combat/Libs/AGR_CombatFunctionLibrary.h"
#include "Combat/Components/AGR_CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_TraceAnimNotifyState)

void UAGR_TraceAnimNotifyState::NotifyBegin(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    float TotalDuration,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if(!IsValid(MeshComp))
    {
        return;
    }

    UAGR_CombatComponent* CombatComponent = UAGR_CombatFunctionLibrary::GetCombatComponent(MeshComp->GetOwner());
    if(!IsValid(CombatComponent))
    {
        return;
    }

    CombatComponent->StartTracingById(TracerId);
}

void UAGR_TraceAnimNotifyState::NotifyEnd(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if(!IsValid(MeshComp))
    {
        return;
    }

    UAGR_CombatComponent* CombatComponent = UAGR_CombatFunctionLibrary::GetCombatComponent(MeshComp->GetOwner());
    if(!IsValid(CombatComponent))
    {
        return;
    }

    CombatComponent->EndTracingById(TracerId);
}