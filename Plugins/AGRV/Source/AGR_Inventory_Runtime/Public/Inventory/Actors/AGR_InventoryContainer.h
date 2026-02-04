// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "AGR_InventoryContainer.generated.h"

/**
 * AGR Inventory Container is an actor that is used by the AGR Inventory and will be automatically spawned by it.
 * It is a helper actor to temporarily store instantiated item actors.
 *
 * Reasons:
 * - We would have used PlayerState, but we need to have a Scene Root to attach actors to.
 * - Avoid garbage collection.
 * - Retain item state of any complexity.
 * - Putting an item into inventory will make the item become attached to this container to avoid constant transform
 *   updates which need to be avoided in a multiplayer scenario.
 * - It helps with persistence of player inventories between sessions including re-joining.
 */
UCLASS(NotBlueprintable)
class AGR_INVENTORY_RUNTIME_API AAGR_InventoryContainer : public AActor
{
    GENERATED_BODY()

public:
    /**
     * The ID of the inventory this container belongs to.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Inventory",
        SaveGame,
        meta=(ExposeOnSpawn))
    FString InventoryID;

public:
    AAGR_InventoryContainer();
};