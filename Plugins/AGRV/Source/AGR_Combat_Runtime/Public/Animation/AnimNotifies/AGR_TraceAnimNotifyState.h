// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

#include "AGR_TraceAnimNotifyState.generated.h"

/**
 * AnimNotifyState used to define the component tag and trigger actions for begin and end of the state.
 */
UCLASS(meta = (DisplayName = "AGR Trace"))
class AGR_COMBAT_RUNTIME_API UAGR_TraceAnimNotifyState : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category="3Studio AGR|Combat")
    FGameplayTag TracerId;

public:
    //~ Begin UAnimNotifyState Interface
    virtual void NotifyBegin(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        float TotalDuration,
        const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;
    //~ End UAnimNotifyState Interface
};