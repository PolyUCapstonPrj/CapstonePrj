// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Inventory/Actors/AGR_InventoryContainer.h"
#include "Types/AGR_InventoryTypes.h"

#include "AGR_InventoryComponent.generated.h"

class UAGR_ItemComponent;

/**
 * Holds info about which item is equipped to what slot.
 *
 * This structure is used to flatten a map key/value entry so the data can be stored in a TArray. Since UE's network
 * replication system does not support TMap we serialize/deserialize this data to/from an internal TArray this way.
 */
USTRUCT()
struct FAGR_ActiveEquipmentKeyValue
{
    GENERATED_BODY()

    /**
     * This is the Active Equipment Map's key.
     */
    UPROPERTY()
    FGameplayTag Slot{};

    /**
     * This is the Active Equipment Map's value.
     */
    UPROPERTY()
    TObjectPtr<AActor> Item{nullptr};
};

/**
 * AGR Inventory component is a flexible and fully network-replicated inventory solution that works in tandem with
 * the AGR Item component. Every inventory component makes use of an internal inventory container object to manage
 * items.
 */
UCLASS(
    ClassGroup="3Studio",
    Blueprintable,
    BlueprintType,
    meta=(BlueprintSpawnableComponent),
    PrioritizeCategories="3Studio AGR")
class AGR_INVENTORY_RUNTIME_API UAGR_InventoryComponent : public UActorComponent
{
    GENERATED_BODY()

    friend UAGR_ItemComponent;

private:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
        FAGR_EquipmentSlotUpdated_Delegate,
        const AActor*,
        Item,
        const EAGR_EquipmentSlotAction,
        SlotAction,
        const FGameplayTag&,
        Slot);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_ItemUpdated_Delegate,
        const AActor*,
        Item,
        const EAGR_ItemUpdateType,
        UpdateType);

public:
    /**
     * Broadcast when the equipment slot was updated.
     */
    UPROPERTY(BlueprintAssignable)
    FAGR_EquipmentSlotUpdated_Delegate OnEquipmentSlotUpdated;

    /**
     * Broadcast when the item was updated.
     */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_ItemUpdated_Delegate OnItemUpdated;

    /**
     * The ID of this inventory.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Config",
        Replicated,
        SaveGame)
    FString InventoryID;

    /**
     * The ID of the owner this inventory belongs to.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Config",
        Replicated,
        SaveGame,
        meta=(ExposeOnSpawn))
    FString OwnerID;

    /**
     * The default transform where the inventory container will be automatically spawned on begin play.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditDefaultsOnly,
        Category="3Studio AGR|Config")
    FTransform ContainerDefaultTransform;

    /**
     * Internal container to "store" items that exist in this inventory but are currently unequipped.
     */
    UPROPERTY(
        BlueprintReadWrite,
        Replicated,
        Category="3Studio AGR|References")
    TObjectPtr<AAGR_InventoryContainer> InventoryContainer;

    /**
     * Maps equipment slots to currently equipped items.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Equipment",
        SaveGame,
        meta=(ExposeOnSpawn))
    TMap<FGameplayTag, TObjectPtr<AActor>> ActiveEquipmentSlotsMap;

private:
    /**
     * Internal helper array to support replication of ActiveEquipmentSlotsMap.
     */
    UPROPERTY(ReplicatedUsing="OnRep_ActiveSlotsMapArray")
    TArray<FAGR_ActiveEquipmentKeyValue> ActiveEquipmentSlotsArray;

public:
    UAGR_InventoryComponent(const FObjectInitializer& ObjectInitializer);

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    //~ End UActorComponent Interface

    /**
     * Gets the item equipped in the given slot if available.
     * @param InSlot The slot from which to potentially get an equipped item.
     * @return Item equipped in slot if found. Otherwise, null.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Equipment", BlueprintNativeEvent)
    AActor* GetItemInSlot(
        UPARAM(DisplayName="Slot") const FGameplayTag InSlot) const;

    /**
     * Checks if this item is equipped in this inventory in one or more slots.
     * @param OutEquippedSlots Slots in which this item was found equipped.
     * @param InItem Item to check for being equipped.
     * @return True if item is equipped in at least one slot. Otherwise, false.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Equipment", BlueprintNativeEvent)
    bool IsItemEquipped(
        UPARAM(DisplayName="Equipped Slots") TArray<FGameplayTag>& OutEquippedSlots,
        UPARAM(DisplayName="Item") const AActor* InItem) const;

    /**
     * Gets all equipped items in this inventory.
     * @return All equipped items.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Equipment", BlueprintNativeEvent)
    TArray<AActor*> GetAllEquippedItems() const;

    /**
     * Gets all items in this inventory.
     * @return Array of all items in this inventory.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="3Studio AGR|Items", BlueprintNativeEvent)
    TArray<AActor*> GetAllItems() const;

    /**
     * Gets all items filtered by class in this inventory.
     * @param InClass Class of the items to filter by.
     * @return All items in this inventory of the specified class.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="3Studio AGR|Items", BlueprintNativeEvent)
    TArray<AActor*> GetAllItemsByClass(
        UPARAM(DisplayName="Class") TSubclassOf<AActor> InClass) const;

    /**
     * Gets all items filtered by tags in this inventory.
     * @param InTags Tags of the items to filter by.
     * @param bInMatchAll If true will return only items that match all of the specified tags.
     * @return All items filtered by tags in this inventory.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="3Studio AGR|Items", BlueprintNativeEvent)
    TArray<AActor*> GetAllItemsByTags(
        UPARAM(DisplayName="Tags ") const TArray<FName>& InTags,
        UPARAM(DisplayName="Match All") const bool bInMatchAll = false) const;

    /**
     * Gets all items filtered by gameplay tag in this inventory.
     * @param InGameplayTag Gameplay tag of the items to filter by.
     * @return All items filtered by gameplay tag in this inventory.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="3Studio AGR|Items", BlueprintNativeEvent)
    TArray<AActor*> GetAllItemsByGameplayTag(
        UPARAM(DisplayName="Gameplay Tag") const FGameplayTag InGameplayTag) const;

    /**
     * Gets all items filtered by name in this inventory.
     * @param InName Name of the items to filter by.
     * @param bInCaseSensitive If true, the text comparison of names will be case-sensitive.
     * @return All items filtered by name in this inventory.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="3Studio AGR|Items", BlueprintNativeEvent)
    TArray<AActor*> GetAllItemsByName(
        UPARAM(DisplayName="Name") const FText& InName,
        UPARAM(DisplayName="Case Sensitive") const bool bInCaseSensitive = false) const;

    /**
     * Calculates a transform for where to drop the item from this inventory.
     * @param InItem Item to drop.
     * @param InDistance The distance from the instigator that the item will be dropped to.
     * @return Transform for where to drop the item.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Helper Functions", BlueprintNativeEvent)
    FTransform CalculateDropTransform(
        UPARAM(DisplayName="Item") const AActor* InItem,
        UPARAM(DisplayName="Distance") const float InDistance) const;

    /**
     * Counts total number of specified items in the inventory.
     * @param InItems Items of which to count total amount of stacks for.
     * @return Total amount stacks for the specified items in the inventory.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Helper Functions", BlueprintNativeEvent)
    int64 CountAllStacks(
        UPARAM(DisplayName="Items") const TArray<AActor*>& InItems) const;

    /**
     * Checks if the specified item is in this inventory.
     * @param InItem Item to check for if it is in this inventory.
     * @return True if item is in this inventory. Otherwise, false.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Helper Functions", BlueprintNativeEvent)
    bool IsItemInInventory(
        UPARAM(DisplayName="Item") const AActor* InItem) const;

    /**
     * Checks is the specified slot exists.
     * @param InSlot Slot to check if it exists.
     * @return True if slot exists, otherwise false.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Helper Functions", BlueprintNativeEvent)
    bool SlotExists(
        UPARAM(DisplayName="Slot") const FGameplayTag InSlot) const;

    /**
     * Adds new stacks to this inventory of specified item class.
     *
     * This function should only be called with authority.
     * @param bOutSuccess True if number of stacked items were added to this inventory.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InItemClass Class of the items to add.
     * @param InStacks Amount of items to add.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Helper Functions", BlueprintNativeEvent)
    void AddStacks(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Item Class") UClass* InItemClass,
        UPARAM(DisplayName="Stacks") const int64 InStacks);

    /**
     * Remove specified number of stacked items from this inventory.
     * 
     * This function should only be called with authority.
     * @param bOutSuccess True if number of stacked items were removed from this inventory.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InItems Arrays of items from which to remove.
     * @param InStacks Number of items to remove from stacks.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Helper Functions", BlueprintNativeEvent)
    void RemoveStacks(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Error Message") FString& OutErrorMessage,
        UPARAM(DisplayName="Items", ref) TArray<AActor*>& InItems,
        UPARAM(DisplayName="Stacks") const int64 InStacks);

private:
    /**
     * Sets an item in the specified slot. In order to clear a slot, set the Item value to null.
     *
     * This function should only be called with authority.
     * @param bOutSuccess True if item was added to slot.
     * @param OutErrorMessage If failed, error message, otherwise empty.
     * @param InItem Item to set. If null, the slot will be cleared.
     * @param InSlot Slot in which the item is set.
     */
    void SetSlot(
        bool& bOutSuccess,
        FString& OutErrorMessage,
        AActor* InItem,
        const FGameplayTag InSlot);

    /**
     * Convert the TMap to an array of key-value pairs.
     */
    void SerializeActiveEquipmentSlotsMap();

    /**
     * Convert the array of key-value pairs back to a TMap.
     */
    void DeserializeActiveEquipmentSlotsMap();

    /**
     * This function will be called automatically when ActiveEquipmentSlotsArray is updated via replication.
     */
    UFUNCTION()
    void OnRep_ActiveSlotsMapArray(const TArray<FAGR_ActiveEquipmentKeyValue>& InOldValue);
};