// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Inventory/Components/AGR_InventoryComponent.h"
#include "UObject/Interface.h"

#include "AGR_ItemInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UAGR_ItemInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * This interface allows to add custom logic for AGR Items.
 * Simply add this interface to your actor representing an item which uses the AGR Item component.
 */
class AGR_INVENTORY_RUNTIME_API IAGR_ItemInterface
{
    GENERATED_BODY()

public:
    /**
     * Checks if this item can be picked up.
     * @param InventoryComponent Inventory component of an actor that tries to pick up this item.
     * @return True if item can be picked up, otherwise false.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Item")
    bool CanItemBePickedUp(const UAGR_InventoryComponent* InventoryComponent) const;

    /**
     * Checks if this item can be dropped.
     * @param InventoryComponent Inventory component of an actor that tries to drop this item.
     * @return True if item can be dropped, otherwise false.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Item")
    bool CanItemBeDropped(const UAGR_InventoryComponent* InventoryComponent) const;

    /**
     * Checks if this item can be destroyed.
     * @param InventoryComponent Inventory component of an actor that tries to destroy this item.
     * @return True if item can be destroyed, otherwise false.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Item")
    bool CanItemBeDestroyed(const UAGR_InventoryComponent* InventoryComponent) const;

    /**
     * Checks if this item can be used.
     * @param InventoryComponent Inventory component of an actor that tries to use this item.
     * @param ActionType Type of action how the item will be used.
     * @return True if item can be used, otherwise false.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Item")
    bool CanItemBeUsed(const UAGR_InventoryComponent* InventoryComponent, const FGameplayTag ActionType) const;

    /**
     * Checks if this item can be equipped.
     * @param InventoryComponent Inventory component of an actor that tries to equip this item.
     * @param Slot Slot into which the item will be equipped.
     * @return True if item can be equipped, otherwise false.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Item")
    bool CanItemBeEquipped(const UAGR_InventoryComponent* InventoryComponent, const FGameplayTag Slot) const;

    /**
     * Checks if this item can be unequipped.
     * @param InventoryComponent Inventory component of an actor that tries to unequip this item.
     * @param Slot Slot from which the item will be unequipped.
     * @return True if item can be unequipped, otherwise false.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Item")
    bool CanItemBeUnequipped(const UAGR_InventoryComponent* InventoryComponent, const FGameplayTag Slot) const;
};