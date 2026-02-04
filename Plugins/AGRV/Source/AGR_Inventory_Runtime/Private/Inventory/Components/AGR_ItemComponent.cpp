// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScopeInitStatement
#include "Inventory//Components/AGR_ItemComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Inventory/Interfaces/AGR_ItemInterface.h"
#include "Inventory/Libs/AGR_InventoryFunctionLibrary.h"
#include "Kismet/KismetGuidLibrary.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_ItemComponent)

UAGR_ItemComponent::UAGR_ItemComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);

    ItemName = FText{};
    ItemType = FGameplayTag{};
    ItemTags = TArray<FName>{};
    bIsStackable = false;
    StackCount = 1;
    MaxStackCount = 1;

    bWasSimulatingPhysics = false;
    ItemID = TEXT("");
    InventoryID = TEXT("");
    OwnerID = TEXT("");

    UActorComponent::SetAutoActivate(true);
}

void UAGR_ItemComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if(IsValid(Owner))
    {
        Owner->Tags.Add(UAGR_InventoryFunctionLibrary::GetItemActorTag());
        Owner->bNetUseOwnerRelevancy = true;
    }

    if(ItemID.IsEmpty())
    {
        ItemID = UKismetGuidLibrary::NewGuid().ToString();
    }
}

void UAGR_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UAGR_ItemComponent, ItemName);
    DOREPLIFETIME(UAGR_ItemComponent, ItemType);
    DOREPLIFETIME(UAGR_ItemComponent, ItemTags);
    DOREPLIFETIME(UAGR_ItemComponent, bIsStackable);
    DOREPLIFETIME(UAGR_ItemComponent, StackCount);
    DOREPLIFETIME(UAGR_ItemComponent, MaxStackCount);
    DOREPLIFETIME(UAGR_ItemComponent, ItemID);
    DOREPLIFETIME(UAGR_ItemComponent, InventoryID);
    DOREPLIFETIME(UAGR_ItemComponent, OwnerID);
}

void UAGR_ItemComponent::SetItemID_Implementation(bool& bOutSuccess, FString& OutErrorMessage, const FString& InItemID)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    if(GetOwnerRole() != ROLE_Authority)
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    ItemID = InItemID;

    bOutSuccess = true;
}

void UAGR_ItemComponent::PickUpItem_Implementation(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    const UAGR_InventoryComponent* InInventoryComponent,
    const bool bInHideOnPickUp,
    const bool bInAutoStack,
    const bool bInForceUpdateOwner)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !IsValid(InInventoryComponent))
    {
        OutErrorMessage = "Owner or Inventory component is invalid";
        return;
    }

    if(!Owner->HasAuthority())
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    if(Owner->Implements<UAGR_ItemInterface>())
    {
        if(!IAGR_ItemInterface::Execute_CanItemBePickedUp(Owner, InInventoryComponent))
        {
            OutErrorMessage = "CanItemBePickedUp returned false";
            return;
        }
    }

    // Prevent picking up an item that is already in the target inventory
    if(InInventoryComponent->IsItemInInventory(Owner))
    {
        OutErrorMessage = "Item is already in inventory";
        return;
    }

    SetItemSimulationState(!bInHideOnPickUp, false);

    const bool bWillStackOnPickup = bIsStackable && bInAutoStack;
    if(!bWillStackOnPickup)
    {
        bool bSetItemOwnershipSuccess = false;
        FString SetItemOwnershipErrorMessage;
        SetItemOwnership(
            bSetItemOwnershipSuccess,
            SetItemOwnershipErrorMessage,
            InInventoryComponent,
            bInForceUpdateOwner);

        if(!bSetItemOwnershipSuccess)
        {
            // NOTE: Should never fail. It most likely is a mistake by the developer.
            OutErrorMessage = FString::Printf(
                TEXT("Pick up item failed when setting item ownership: %s"),
                *SetItemOwnershipErrorMessage);
            return;
        }

        const FAttachmentTransformRules AttachmentTransformRules{
            EAttachmentRule::SnapToTarget,
            EAttachmentRule::KeepWorld,
            EAttachmentRule::KeepWorld,
            false
        };
        Owner->AttachToActor(InInventoryComponent->InventoryContainer, AttachmentTransformRules);

        OnItemPickedUp.Broadcast(InInventoryComponent);
        InInventoryComponent->OnItemUpdated.Broadcast(Owner, EAGR_ItemUpdateType::PickedUp);

        bOutSuccess = true;

        return;
    }

    int64 RemainingStackCount = GetStackCount();
    // Get all items in inventory of the same class and try to stack this item with them.
    TArray<AActor*> Items = InInventoryComponent->GetAllItemsByClass(Owner->GetClass());
    for(const AActor* const Item : Items)
    {
        UAGR_ItemComponent* const ItemComponentToStackWith =
            UAGR_InventoryFunctionLibrary::GetItemComponent(Item);

        if(!IsValid(ItemComponentToStackWith))
        {
            continue;
        }

        const int64 AvailableStackCount = ItemComponentToStackWith->MaxStackCount
                                          - ItemComponentToStackWith->GetStackCount();
        if(AvailableStackCount <= 0)
        {
            continue;
        }

        // Fill up the current item in inventory till max stack count
        if(RemainingStackCount > AvailableStackCount)
        {
            ItemComponentToStackWith->SetStackCount(ItemComponentToStackWith->MaxStackCount);
            RemainingStackCount -= AvailableStackCount;
            InInventoryComponent->OnItemUpdated.Broadcast(Item, EAGR_ItemUpdateType::Updated);
            continue;
        }

        // Fill up the current item in inventory with the remaining stack count
        ItemComponentToStackWith->SetStackCount(ItemComponentToStackWith->GetStackCount() + RemainingStackCount);
        RemainingStackCount = 0;
        InInventoryComponent->OnItemUpdated.Broadcast(Item, EAGR_ItemUpdateType::Updated);

        // All stacks including any remaining stack count has been distributed among inventory items of the same class.
        break;
    }

    if(RemainingStackCount <= 0)
    {
        // If item was stacked fully with other items destroy this item
        Owner->Destroy();
    }
    else
    {
        // After stacking step finally "pick up" this item

        bool bSetItemOwnershipSuccess = false;
        FString SetItemOwnershipErrorMessage;
        SetItemOwnership(
            bSetItemOwnershipSuccess,
            SetItemOwnershipErrorMessage,
            InInventoryComponent,
            false);

        if(!bSetItemOwnershipSuccess)
        {
            // NOTE: Should never fail. It most likely is a mistake by the developer.
            OutErrorMessage = FString::Printf(
                TEXT("Pick up item failed when setting item ownership after stacking: %s"),
                *SetItemOwnershipErrorMessage);
            return;
        }

        SetStackCount(RemainingStackCount);

        const FAttachmentTransformRules AttachmentTransformRules{
            EAttachmentRule::SnapToTarget,
            EAttachmentRule::KeepWorld,
            EAttachmentRule::KeepWorld,
            false
        };
        Owner->AttachToActor(InInventoryComponent->InventoryContainer, AttachmentTransformRules);

        OnItemPickedUp.Broadcast(InInventoryComponent);
        InInventoryComponent->OnItemUpdated.Broadcast(Owner, EAGR_ItemUpdateType::PickedUp);
    }

    bOutSuccess = true;
}

void UAGR_ItemComponent::EquipItem_Implementation(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    UAGR_InventoryComponent* InInventoryComponent,
    const FGameplayTag InSlot,
    const bool bInVisible)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !IsValid(InInventoryComponent))
    {
        OutErrorMessage = "Owner or Inventory component is invalid";
        return;
    }

    if(!Owner->HasAuthority())
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    if(Owner->Implements<UAGR_ItemInterface>())
    {
        if(!IAGR_ItemInterface::Execute_CanItemBeEquipped(Owner, InInventoryComponent, InSlot))
        {
            OutErrorMessage = "CanItemBeEquipped returned false";
            return;
        }
    }

    if(!InInventoryComponent->SlotExists(InSlot))
    {
        OutErrorMessage = FString::Printf(TEXT("Slot '%s' does not exist"), *InSlot.ToString());
        return;
    }

    // Ensure that the target slot will be empty.
    const AActor* ItemInSlot = InInventoryComponent->GetItemInSlot(InSlot);
    if(IsValid(ItemInSlot))
    {
        UAGR_ItemComponent* const ItemComponentInSlot
            = UAGR_InventoryFunctionLibrary::GetItemComponent(ItemInSlot);

        if(!IsValid(ItemComponentInSlot))
        {
            OutErrorMessage = FString::Printf(
                TEXT("Equip item failed: Actor '%s' in target slot '%s' has no AGR Item component"),
                *ItemInSlot->GetName(),
                *InSlot.ToString());
            return;
        }

        bool bUnequipItemSuccess = false;
        FString UnequipItemErrorMessage;
        ItemComponentInSlot->UnequipItem(
            bUnequipItemSuccess,
            UnequipItemErrorMessage,
            InInventoryComponent,
            InSlot);

        if(!bUnequipItemSuccess)
        {
            OutErrorMessage = FString::Printf(
                TEXT("Equip item failed when trying to free target slot '%s': %s"),
                *InSlot.ToString(),
                *UnequipItemErrorMessage);
            return;
        }
    }

    // Set or update the slot
    bool bSetSlotSuccess = false;
    FString SetSlotErrorMessage;
    InInventoryComponent->SetSlot(
        bSetSlotSuccess,
        SetSlotErrorMessage,
        Owner,
        InSlot);

    if(!bSetSlotSuccess)
    {
        // NOTE: Should never fail. It most likely is a mistake by the developer.
        OutErrorMessage = FString::Printf(
            TEXT("Equip item failed when trying to add item to target slot '%s': %s"),
            *InSlot.ToString(),
            *SetSlotErrorMessage);
        return;
    }

    SetItemSimulationState(bInVisible, false);

    OnItemEquipped.Broadcast(InInventoryComponent, InSlot);
    InInventoryComponent->OnItemUpdated.Broadcast(Owner, EAGR_ItemUpdateType::Equipped);

    bOutSuccess = true;
}

void UAGR_ItemComponent::UnequipItem_Implementation(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    UAGR_InventoryComponent* InInventoryComponent,
    const FGameplayTag InSlot)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !IsValid(InInventoryComponent))
    {
        OutErrorMessage = "Owner or Inventory component is invalid";
        return;
    }

    if(!Owner->HasAuthority())
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    if(Owner->Implements<UAGR_ItemInterface>())
    {
        if(!IAGR_ItemInterface::Execute_CanItemBeUnequipped(Owner, InInventoryComponent, InSlot))
        {
            OutErrorMessage = "CanItemBeUnequipped returned false";
            return;
        }
    }

    if(!InInventoryComponent->SlotExists(InSlot))
    {
        OutErrorMessage = FString::Printf(TEXT("Slot '%s' does not exist"), *InSlot.ToString());
        return;
    }

    // Clear the slot
    bool bSetSlotSuccess = false;
    FString SetSlotErrorMessage;
    InInventoryComponent->SetSlot(
        bSetSlotSuccess,
        SetSlotErrorMessage,
        nullptr,
        InSlot);

    if(!bSetSlotSuccess)
    {
        OutErrorMessage = FString::Printf(TEXT("Unequip item failed: %s"), *SetSlotErrorMessage);
        return;
    }

    SetItemSimulationState(false, false);

    const FAttachmentTransformRules AttachmentTransformRules{
        EAttachmentRule::SnapToTarget,
        EAttachmentRule::KeepWorld,
        EAttachmentRule::KeepWorld,
        false
    };
    Owner->AttachToActor(InInventoryComponent->InventoryContainer, AttachmentTransformRules);

    OnItemUnequipped.Broadcast(InInventoryComponent, InSlot);
    InInventoryComponent->OnItemUpdated.Broadcast(Owner, EAGR_ItemUpdateType::Unequipped);

    bOutSuccess = true;
}

// ReSharper disable once CppUE4BlueprintCallableFunctionMayBeConst
void UAGR_ItemComponent::UseItem_Implementation(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    const UAGR_InventoryComponent* InInventoryComponent,
    const FGameplayTag InActionType)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !IsValid(InInventoryComponent))
    {
        OutErrorMessage = "Owner or Inventory component is invalid";
        return;
    }

    if(!Owner->HasAuthority())
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    if(Owner->Implements<UAGR_ItemInterface>())
    {
        if(!IAGR_ItemInterface::Execute_CanItemBeUsed(Owner, InInventoryComponent, InActionType))
        {
            OutErrorMessage = "CanItemBeUsed returned false";
            return;
        }
    }

    OnItemUsed.Broadcast(InInventoryComponent, InActionType);
    InInventoryComponent->OnItemUpdated.Broadcast(Owner, EAGR_ItemUpdateType::Used);

    bOutSuccess = true;
}

void UAGR_ItemComponent::DropItem_Implementation(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    UAGR_InventoryComponent* InInventoryComponent,
    const FVector& InDropLocation,
    const FRotator& InDropRotation,
    const bool bInClearOwnerID)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !IsValid(InInventoryComponent))
    {
        OutErrorMessage = "Owner or Inventory component is invalid";
        return;
    }

    if(!Owner->HasAuthority())
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    if(Owner->Implements<UAGR_ItemInterface>())
    {
        if(!IAGR_ItemInterface::Execute_CanItemBeDropped(Owner, InInventoryComponent))
        {
            OutErrorMessage = "CanItemBeDropped returned false";
            return;
        }
    }

    TArray<FGameplayTag> EquippedInSlots;
    if(InInventoryComponent->IsItemEquipped(EquippedInSlots, Owner))
    {
        for(const FGameplayTag& Slot : EquippedInSlots)
        {
            bool bUnequipItemSuccess = false;
            FString UnequipItemErrorMessage;
            UnequipItem(
                bUnequipItemSuccess,
                UnequipItemErrorMessage,
                InInventoryComponent,
                Slot);

            if(!bUnequipItemSuccess)
            {
                OutErrorMessage = FString::Printf(
                    TEXT("Drop item failed when trying to unequip from slot '%s': %s"),
                    *Slot.ToString(),
                    *UnequipItemErrorMessage);
                return;
            }
        }
    }

    const FDetachmentTransformRules DetachmentTransformRules{
        EDetachmentRule::KeepWorld,
        EDetachmentRule::KeepWorld,
        EDetachmentRule::KeepWorld,
        false
    };
    Owner->DetachFromActor(DetachmentTransformRules);

    Owner->SetActorLocationAndRotation(
        InDropLocation,
        InDropRotation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

    SetItemSimulationState(true, true);

    Owner->SetOwner(nullptr);
    Owner->SetInstigator(nullptr);
    InventoryID.Empty();

    if(bInClearOwnerID)
    {
        OwnerID.Empty();
    }

    OnItemDropped.Broadcast(InInventoryComponent, InDropLocation);
    InInventoryComponent->OnItemUpdated.Broadcast(Owner, EAGR_ItemUpdateType::Dropped);

    bOutSuccess = true;
}

// ReSharper disable once CppUE4BlueprintCallableFunctionMayBeConst
void UAGR_ItemComponent::DestroyItem_Implementation(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    const UAGR_InventoryComponent* InInventoryComponent)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !IsValid(InInventoryComponent))
    {
        OutErrorMessage = "Owner or Inventory component is invalid";
        return;
    }

    if(!Owner->HasAuthority())
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    if(Owner->Implements<UAGR_ItemInterface>())
    {
        if(!IAGR_ItemInterface::Execute_CanItemBeDestroyed(Owner, InInventoryComponent))
        {
            OutErrorMessage = "CanItemBeDestroyed returned false";
            return;
        }
    }

    OnItemDestroyed.Broadcast();
    InInventoryComponent->OnItemUpdated.Broadcast(Owner, EAGR_ItemUpdateType::BeforeDestroy);

    const FDetachmentTransformRules DetachmentTransformRules{
        EDetachmentRule::KeepWorld,
        EDetachmentRule::KeepWorld,
        EDetachmentRule::KeepWorld,
        false
    };
    Owner->DetachFromActor(DetachmentTransformRules);

    Owner->Destroy();

    bOutSuccess = true;
}

void UAGR_ItemComponent::SplitStack_Implementation(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    const UAGR_InventoryComponent* InInventoryComponent,
    const int64 InStack)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !IsValid(InInventoryComponent))
    {
        OutErrorMessage = "Owner or Inventory component is invalid";
        return;
    }

    if(!Owner->HasAuthority())
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    UWorld* const World = GetWorld();
    if(!IsValid(World))
    {
        OutErrorMessage = "World is invalid";
        return;
    }

    if(InStack <= 0 || InStack >= GetStackCount())
    {
        OutErrorMessage = FString::Printf(TEXT("Stack must be in range of [1..%lld]"), GetStackCount() - 1);
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParameters.Owner = Owner->GetOwner();
    SpawnParameters.Instigator = Owner->GetInstigator();

    const AActor* NewItem = World->SpawnActor<AActor>(
        Owner->GetClass(),
        Owner->GetActorTransform(),
        SpawnParameters);

    UAGR_ItemComponent* NewItemComponent = UAGR_InventoryFunctionLibrary::GetItemComponent(NewItem);
    if(!IsValid(NewItemComponent))
    {
        OutErrorMessage = "Newly split item actor does not have an AGR Item component";
        return;
    }

    NewItemComponent->SetStackCount(InStack);
    SetStackCount(GetStackCount() - InStack);
    InInventoryComponent->OnItemUpdated.Broadcast(Owner, EAGR_ItemUpdateType::Updated);

    bool bPickUpItemSuccess = false;
    FString PickUpItemErrorMessage;
    NewItemComponent->PickUpItem(
        bPickUpItemSuccess,
        PickUpItemErrorMessage,
        InInventoryComponent,
        true,
        false,
        false);

    if(!bPickUpItemSuccess)
    {
        // NOTE: Should never fail. It most likely is a mistake by the developer.
        OutErrorMessage = FString::Printf(
            TEXT("Split stack failed when picking up newly split item: %s"),
            *PickUpItemErrorMessage);
        return;
    }

    bOutSuccess = true;
}

void UAGR_ItemComponent::SetItemSimulationState_Implementation(const bool bInVisible, const bool bInSimulatePhysics)
{
    AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return;
    }

    // Only if the root component is a subclass of primitive component we will switch its state of simulating physics.
    UPrimitiveComponent* const SimulatingComponent = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    if(IsValid(SimulatingComponent))
    {
        if(bInVisible)
        {
            if(bInSimulatePhysics)
            {
                SimulatingComponent->SetSimulatePhysics(bWasSimulatingPhysics);
            }
        }
        else
        {
            if(!bWasSimulatingPhysics)
            {
                bWasSimulatingPhysics = SimulatingComponent->IsSimulatingPhysics(NAME_None);
            }

            if(bWasSimulatingPhysics)
            {
                SimulatingComponent->SetSimulatePhysics(false);
            }
        }
    }

    Owner->SetActorEnableCollision(bInVisible);
    Owner->SetActorHiddenInGame(!bInVisible);

    OnItemChangedVisibility.Broadcast(bInVisible, bInSimulatePhysics);
}

bool UAGR_ItemComponent::GetWasSimulatingPhysics() const
{
    return bWasSimulatingPhysics;
}

FString UAGR_ItemComponent::GetItemID() const
{
    return ItemID;
}

FString UAGR_ItemComponent::GetInventoryID() const
{
    return InventoryID;
}

FString UAGR_ItemComponent::GetOwnerID() const
{
    return OwnerID;
}

int64 UAGR_ItemComponent::GetStackCount() const
{
    return StackCount;
}

void UAGR_ItemComponent::SetStackCount(const int64 NewStackCount)
{
    if(GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    const int64 OldStackCount = StackCount;
    StackCount = NewStackCount;
    OnRep_StackCount(OldStackCount);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_ItemComponent::OnRep_StackCount(const int64 OldStackCount)
{
    OnStackCountUpdated.Broadcast(OldStackCount, StackCount);
}

void UAGR_ItemComponent::SetItemOwnership(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    const UAGR_InventoryComponent* InInventoryComponent,
    const bool bInForceUpdateOwner)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !IsValid(InInventoryComponent))
    {
        OutErrorMessage = "Owner or Inventory component is invalid";
        return;
    }

    if(!Owner->HasAuthority())
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    AActor* const InventoryComponentOwner = InInventoryComponent->GetOwner();
    if(!IsValid(InventoryComponentOwner))
    {
        OutErrorMessage = "Inventory component's owner is invalid";
        return;
    }

    Owner->SetOwner(InventoryComponentOwner);

    APawn* const InventoryComponentInstigator = InventoryComponentOwner->GetInstigator();
    Owner->SetInstigator(InventoryComponentInstigator);

    InventoryID = InInventoryComponent->InventoryID;

    if(OwnerID.IsEmpty())
    {
        OwnerID = InventoryID;
    }
    else
    {
        if(bInForceUpdateOwner)
        {
            OwnerID = InventoryID;
        }
    }

    bOutSuccess = true;
}