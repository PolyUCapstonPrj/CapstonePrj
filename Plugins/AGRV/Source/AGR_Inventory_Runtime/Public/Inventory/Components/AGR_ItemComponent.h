// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/Components/AGR_InventoryComponent.h"

#include "AGR_ItemComponent.generated.h"

/**
 * AGR Item component allows to turn any actor into an item that works in single and multiplayer scenarios.
 * 
 * AGR Items come with the following features:
 * - Pick up item
 * - Drop item
 * - Use item
 * - Equip item
 * - Unequip item
 * - Destroy item
 * - Change visibility & simulate physics of item
 * - Stacking
 */
UCLASS(
    ClassGroup="3Studio",
    Blueprintable,
    BlueprintType,
    meta=(BlueprintSpawnableComponent),
    PrioritizeCategories="3Studio AGR")
class AGR_INVENTORY_RUNTIME_API UAGR_ItemComponent : public UActorComponent
{
    GENERATED_BODY()

private:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_ItemChangedVisibility_Delegate,
        const bool,
        bVisibility,
        const bool,
        bSimulatePhysics);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
        FAGR_ItemPickedUp_Delegate,
        const UAGR_InventoryComponent*,
        InventoryComponent);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_ItemDropped_Delegate,
        const UAGR_InventoryComponent*,
        InventoryComponent,
        const FVector&,
        DropLocation);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(
        FAGR_ItemDestroyed_Delegate);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_ItemUsed_Delegate,
        const UAGR_InventoryComponent*,
        InventoryComponent,
        const FGameplayTag&,
        ActionType);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_ItemEquipped_Delegate,
        const UAGR_InventoryComponent*,
        InventoryComponent,
        const FGameplayTag&,
        EquipmentSlot);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_ItemUnequipped_Delegate,
        const UAGR_InventoryComponent*,
        InventoryComponent,
        const FGameplayTag&,
        EquipmentSlot);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_StackCountUpdated_DynamicMulticast,
        const int64,
        OldStackCount,
        const int64,
        NewStackCount);

public:
    /**
     * Broadcast when the item visibility changed.
     */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_ItemChangedVisibility_Delegate OnItemChangedVisibility;

    /**
     * Broadcast when the item was picked up.
     */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_ItemPickedUp_Delegate OnItemPickedUp;

    /**
     * Broadcast when the item was dropped.
     */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_ItemDropped_Delegate OnItemDropped;

    /**
     * Broadcast when the item was destroyed.
     */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_ItemDestroyed_Delegate OnItemDestroyed;

    /**
     * Broadcast when the item was used.
     */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_ItemUsed_Delegate OnItemUsed;

    /**
     * Broadcast when the item was equipped.
     */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_ItemEquipped_Delegate OnItemEquipped;

    /**
     * Broadcast when the item was unequipped.
     */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_ItemUnequipped_Delegate OnItemUnequipped;

    /**
     * Broadcast when the stack count gets updated.
     */
    UPROPERTY(BlueprintAssignable)
    FAGR_StackCountUpdated_DynamicMulticast OnStackCountUpdated;

    /**
     * Name of this item.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Config",
        Replicated,
        SaveGame,
        meta=(ExposeOnSpawn))
    FText ItemName;

    /**
     * Type of this item.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Config",
        Replicated,
        SaveGame,
        meta=(ExposeOnSpawn))
    FGameplayTag ItemType;

    /**
     * Tags of this item.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Config",
        Replicated,
        SaveGame,
        meta=(ExposeOnSpawn))
    TArray<FName> ItemTags;

    /**
     * Whether this item can be stacked or not.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Stacking",
        Replicated,
        SaveGame,
        meta=(ExposeOnSpawn))
    bool bIsStackable;

    /**
     * The maximum stack count of this item type.
     * Only applicable when bIsStackable is enabled.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Stacking",
        Replicated,
        SaveGame,
        meta=(ExposeOnSpawn, EditCondition = "bIsStackable", EditConditionHides, ClampMin=0))
    int64 MaxStackCount;

private:
    /**
     * The current stack count contained in this item instance.
     * Only applicable when bIsStackable is enabled.
     */
    UPROPERTY(
        EditAnywhere,
        Category="3Studio AGR|Stacking",
        ReplicatedUsing="OnRep_StackCount",
        SaveGame,
        meta=(EditCondition = "bIsStackable", EditConditionHides, ClampMin=0))
    int64 StackCount;

    /**
     * Internal flag to be able to restore the previous state of simulating physics for this item.
     */
    UPROPERTY(
        SaveGame)
    bool bWasSimulatingPhysics;

    /**
     * The ID of this item.
     */
    UPROPERTY(
        Replicated,
        SaveGame)
    FString ItemID;

    /**
     * The ID of the inventory this item belongs to.
     */
    UPROPERTY(
        Replicated,
        SaveGame)
    FString InventoryID;

    /**
     * The ID of the owner this item belongs to.
     */
    UPROPERTY(
        Replicated,
        SaveGame)
    FString OwnerID;

public:
    UAGR_ItemComponent(const FObjectInitializer& ObjectInitializer);

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    //~ End UActorComponent Interface

    /**
     * Sets new ID for this item.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if new ID was set.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InItemID New item ID.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Item", BlueprintNativeEvent)
    void SetItemID(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Item ID") const FString& InItemID);

    /**
     * Picks up this item.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if item was picked up.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InInventoryComponent Inventory component of an actor that picks up this item.
     * @param bInHideOnPickUp If true this item will become invisible after picked up.
     * @param bInAutoStack If true will try to stack this item with other already existing instances of this item in
     * the actors inventory.
     * @param bInForceUpdateOwner If true will overwrite items owner ID to inventories ID.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Item Actions", BlueprintNativeEvent)
    void PickUpItem(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Inventory Component") const UAGR_InventoryComponent* InInventoryComponent,
        UPARAM(DisplayName="Hide on Pick Up") const bool bInHideOnPickUp = true,
        UPARAM(DisplayName="Auto Stack") const bool bInAutoStack = true,
        UPARAM(DisplayName="Force Update Owner") const bool bInForceUpdateOwner = false);

    /**
     * Equips this item.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if item was equipped.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InInventoryComponent Inventory component of an actor that equips this item.
     * @param InSlot Slot into which one to equip the item.
     * @param bInVisible If true will make item visible.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Item Actions", BlueprintNativeEvent)
    void EquipItem(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Inventory Component") UAGR_InventoryComponent* InInventoryComponent,
        UPARAM(DisplayName="Slot") const FGameplayTag InSlot,
        UPARAM(DisplayName="Visible") const bool bInVisible);

    /**
     * Unequips this item.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if item was unequipped.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InInventoryComponent Inventory component of an actor that unequips this item.
     * @param InSlot Slot from which one to unequip the item.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Item Actions", BlueprintNativeEvent)
    void UnequipItem(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Inventory Component") UAGR_InventoryComponent* InInventoryComponent,
        UPARAM(DisplayName="Slot") const FGameplayTag InSlot);

    /**
     * Uses this item.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if item was used.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InInventoryComponent Inventory component of an actor that uses this item.
     * @param InActionType Type of action to call on this item.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Item Actions", BlueprintNativeEvent)
    void UseItem(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Inventory Component") const UAGR_InventoryComponent* InInventoryComponent,
        UPARAM(DisplayName="Action Type") const FGameplayTag InActionType);

    /**
     * Drops this item.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if item was dropped.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InInventoryComponent Inventory component of an actor that drops this item.
     * @param InDropLocation Items location after it is dropped.
     * @param InDropRotation Items rotations after it is dropped.
     * @param bInClearOwnerID If true clear item's owner ID.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Item Actions", BlueprintNativeEvent)
    void DropItem(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Inventory Component") UAGR_InventoryComponent* InInventoryComponent,
        UPARAM(DisplayName="Drop Location") const FVector& InDropLocation,
        UPARAM(DisplayName="Drop Rotation") const FRotator& InDropRotation,
        UPARAM(DisplayName="Clear Owner ID") const bool bInClearOwnerID = true);

    /**
     * Destroys this item.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if item was destroyed.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InInventoryComponent Inventory component of an actor that destroys this item.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Item Actions", BlueprintNativeEvent)
    void DestroyItem(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Inventory Component") const UAGR_InventoryComponent* InInventoryComponent);

    /**
     * Splits this item stack into two stacks with one of them containing the specified amount of items and the second
     * one with the rest of them. This will update this item and also create a new one and automatically pick it up.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if item stack was split.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InInventoryComponent Inventory component of an actor that splits the item.
     * @param InStack Amount of items to have in a new stack. Cannot be less than this item has items in its stack.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Utility", BlueprintNativeEvent)
    void SplitStack(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Inventory Component") const UAGR_InventoryComponent* InInventoryComponent,
        UPARAM(DisplayName="Stack") const int64 InStack);

    /**
     * Sets this item's visibility and state of simulating physics.
     *
     * State of simulating physics will be changed only if the set root component is a subclass of primitive component.
     * Collisions will be disabled while the item is invisible and re-enabled when turned back on to be visible.
     * @param bInVisible If true item will become visible.
     * @param bInSimulatePhysics If true state of simulating physics will be restored. This parameter has no effect when
     * bVisible is set to false and is only used when bVisible is set to true.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|Utility", BlueprintNativeEvent)
    void SetItemSimulationState(
        UPARAM(DisplayName="Visible") const bool bInVisible,
        UPARAM(DisplayName="Simulate Physics") const bool bInSimulatePhysics);

    /**
     * Gets the internal flag to be able to restore the previous state of simulating physics for this item.
     */
    UFUNCTION(
        BlueprintCallable,
        BlueprintPure,
        Category="3Studio AGR|Item")
    bool GetWasSimulatingPhysics() const;

    /**
     * Gets the ID of this item.
     */
    UFUNCTION(
        BlueprintCallable,
        BlueprintPure,
        Category="3Studio AGR|Item")
    FString GetItemID() const;

    /**
     * Gets the ID of the inventory this item belongs to.
     */
    UFUNCTION(
        BlueprintCallable,
        BlueprintPure,
        Category="3Studio AGR|Item")
    FString GetInventoryID() const;

    /**
     * Gets the ID of the owner this item belongs to.
     */
    UFUNCTION(
        BlueprintCallable,
        BlueprintPure,
        Category="3Studio AGR|Item")
    FString GetOwnerID() const;

    /**
     * Gets the stack count of this item.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Item")
    int64 GetStackCount() const;

    /**
     * Sets the stack count of this item.
     * 
     * @param NewStackCount New stack count.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Item")
    void SetStackCount(const int64 NewStackCount);

protected:
    /**
     * This function will be called automatically when StackCount is updated via network-replication.
     * 
     * Calls the OnStackCountUpdated event dispatcher.
     */
    UFUNCTION()
    void OnRep_StackCount(const int64 OldStackCount);

private:
    /**
     * Sets the ownership of this item.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if ownership was set.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InInventoryComponent Inventory component of an actor.
     * @param bInForceUpdateOwner If true, ownership will always be updated. Otherwise, ownership will only be updated
     * if this item has no owner.
     */
    void SetItemOwnership(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        const UAGR_InventoryComponent* InInventoryComponent,
        const bool bInForceUpdateOwner = false);
};