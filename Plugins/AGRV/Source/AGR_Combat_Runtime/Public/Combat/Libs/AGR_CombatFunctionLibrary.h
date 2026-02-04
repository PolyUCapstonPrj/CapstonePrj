// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AGR_CombatFunctionLibrary.generated.h"

class UAGR_CombatComponent;

/**
 * AGR Combat Blueprint Function Library.
 */
UCLASS()
class AGR_COMBAT_RUNTIME_API UAGR_CombatFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Gets the AGR Combat Component from an actor.
     * 
     * @param Actor The actor from which to get the component.
     * @return Combat component or null if not found.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Get Component")
    static UAGR_CombatComponent* GetCombatComponent(const AActor* Actor);
};