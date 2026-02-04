// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AGR_AnimationFunctionLibrary.generated.h"

class UAGR_LocomotionComponent;

/**
 * AGR Animation Blueprint Function Library.
 */
UCLASS()
class AGR_ANIMATION_RUNTIME_API UAGR_AnimationFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Retrieves the AGR Locomotion Component from the actor or its instigator.
     * @param InActor Target for retrieval.
     * @return The found component, or nullptr.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Get Component")
    static UAGR_LocomotionComponent* GetLocomotionComponent(
        UPARAM(DisplayName="Actor") const AActor* InActor);
};
