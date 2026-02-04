// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Inventory/Components/AGR_ItemComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AGR_InventoryFunctionLibrary.generated.h"

class UAGR_InventoryComponent;

/**
 * AGR Inventory Blueprint Function Library.
 */
UCLASS()
class AGR_INVENTORY_RUNTIME_API UAGR_InventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Gets the AGR Item Component from an actor.
     * @param InActor The actor from which to get the component.
     * @return Item component or null if not found.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Get Component")
    static UAGR_ItemComponent* GetItemComponent(
        UPARAM(DisplayName="Actor") const AActor* const InActor);

    /**
     * Gets the AGR Inventory Component from an actor.
     * @param InActor The actor from which to get the component.
     * @return Inventory component or null if not found.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Get Component")
    static UAGR_InventoryComponent* GetInventoryComponent(
        UPARAM(DisplayName="Actor") const AActor* const InActor);

    /**
     * Gets the item actor tag name that is configured in this plugin's project settings.
     * @return Item actor tag.
     */
    UFUNCTION(Blueprintable, BlueprintPure, Category="3Studio AGR|Item")
    static FName GetItemActorTag();
};