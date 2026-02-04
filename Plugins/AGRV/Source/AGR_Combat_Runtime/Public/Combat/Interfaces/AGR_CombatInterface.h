// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "AGR_CombatInterface.generated.h"

struct FGameplayTag;

UINTERFACE(MinimalAPI, Blueprintable)
class UAGR_CombatInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for AGR tracer components used in the combat system to enable communication between the combat component
 * and the tracer component. 
 */
class AGR_COMBAT_RUNTIME_API IAGR_CombatInterface
{
    GENERATED_BODY()

public:
    /**
     * Assigns a unique ID to the tracer component.
     * 
     * @param Id The gameplay tag as ID.
     */
    UFUNCTION(BlueprintNativeEvent, Category="3Studio AGR|Combat")
    void SetTracerId(const FGameplayTag& Id);

    /**
     * Retrieves the tracer ID assigned to the tracer component.
     * 
     * @returns The gameplay tag as ID.
     */
    UFUNCTION(BlueprintNativeEvent, Category="3Studio AGR|Combat")
    const FGameplayTag GetTracerId();

    /**
     * Initiates the tracing process for the tracer component.
     */
    UFUNCTION(BlueprintNativeEvent, Category="3Studio AGR|Combat")
    void StartTracing();

    /**
     * Stops the tracing process for the tracer component.
     */
    UFUNCTION(BlueprintNativeEvent, Category="3Studio AGR|Combat")
    void EndTracing();
};