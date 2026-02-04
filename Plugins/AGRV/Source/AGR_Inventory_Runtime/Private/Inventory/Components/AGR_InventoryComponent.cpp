// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScope
// ReSharper disable CppUseStructuredBinding
#include "Inventory/Components/AGR_InventoryComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Inventory/Components/AGR_ItemComponent.h"
#include "Inventory/Libs/AGR_InventoryFunctionLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetGuidLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Module/AGR_Inventory_RuntimeLogs.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_InventoryComponent)

UAGR_InventoryComponent::UAGR_InventoryComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    SetIsReplicatedByDefault(true);

    InventoryID = TEXT("");
    OwnerID = TEXT("");
    ContainerDefaultTransform = FTransform::Identity;
    InventoryContainer = nullptr;
    ActiveEquipmentSlotsMap = TMap<FGameplayTag, TObjectPtr<AActor>>{};

    ActiveEquipmentSlotsArray = TArray<FAGR_ActiveEquipmentKeyValue>{};

    UActorComponent::SetAutoActivate(true);
}

void UAGR_InventoryComponent::SerializeActiveEquipmentSlotsMap()
{
    const TArray<FAGR_ActiveEquipmentKeyValue> OldValue = ActiveEquipmentSlotsArray;

    ActiveEquipmentSlotsArray.Empty();
    for(const auto& PairMap : ActiveEquipmentSlotsMap)
    {
        ActiveEquipmentSlotsArray.Add(FAGR_ActiveEquipmentKeyValue{PairMap.Key, PairMap.Value});
    }

    OnRep_ActiveSlotsMapArray(OldValue);
}

void UAGR_InventoryComponent::DeserializeActiveEquipmentSlotsMap()
{
    ActiveEquipmentSlotsMap.Empty();
    for(const FAGR_ActiveEquipmentKeyValue& PairArray : ActiveEquipmentSlotsArray)
    {
        ActiveEquipmentSlotsMap.Add(PairArray.Slot, PairArray.Item);
    }
}

void UAGR_InventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return;
    }

    if(!Owner->HasAuthority())
    {
        return;
    }

    UWorld* const World = GetWorld();
    if(!IsValid(World))
    {
        return;
    }

    SerializeActiveEquipmentSlotsMap();

    if(InventoryID.IsEmpty())
    {
        InventoryID = UKismetGuidLibrary::NewGuid().ToString(EGuidFormats::Digits);
    }

    if(OwnerID.IsEmpty())
    {
        OwnerID = UKismetGuidLibrary::NewGuid().ToString(EGuidFormats::Digits);
    }

    FActorSpawnParameters SpawnParameters;

    APawn* const Instigator = Owner->GetInstigator();
    if(IsValid(Instigator))
    {
        SpawnParameters.Owner = Instigator->GetController();
        SpawnParameters.Instigator = Instigator;
    }
    else
    {
        SpawnParameters.Owner = Owner;
    }

    InventoryContainer = World->SpawnActor<AAGR_InventoryContainer>(
        AAGR_InventoryContainer::StaticClass(),
        ContainerDefaultTransform,
        SpawnParameters);

    InventoryContainer->InventoryID = InventoryID;
}

void UAGR_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UAGR_InventoryComponent, InventoryID);
    DOREPLIFETIME(UAGR_InventoryComponent, OwnerID);
    DOREPLIFETIME_CONDITION(UAGR_InventoryComponent, InventoryContainer, COND_InitialOnly);

    DOREPLIFETIME(UAGR_InventoryComponent, ActiveEquipmentSlotsArray);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_InventoryComponent::OnRep_ActiveSlotsMapArray(const TArray<FAGR_ActiveEquipmentKeyValue>& InOldValue)
{
    DeserializeActiveEquipmentSlotsMap();

    for(int32 i = 0; i < ActiveEquipmentSlotsArray.Num(); ++i)
    {
        if(i >= InOldValue.Num())
        {
            break;
        }

        const FAGR_ActiveEquipmentKeyValue& Current = ActiveEquipmentSlotsArray[i];
        const FAGR_ActiveEquipmentKeyValue& Old = InOldValue[i];

        const bool SameItemValuesInSlot = Current.Item == Old.Item;
        if(SameItemValuesInSlot)
        {
            continue;
        }

        const bool IsSetOrUpdated = Current.Item != nullptr;
        if(IsSetOrUpdated)
        {
            OnEquipmentSlotUpdated.Broadcast(
                Current.Item,
                EAGR_EquipmentSlotAction::Set,
                Current.Slot);
        }
        else
        {
            OnEquipmentSlotUpdated.Broadcast(Current.Item, EAGR_EquipmentSlotAction::Cleared, Current.Slot);
        }
    }
}

AActor* UAGR_InventoryComponent::GetItemInSlot_Implementation(const FGameplayTag InSlot) const
{
    const TObjectPtr<AActor>* ValuePtr = ActiveEquipmentSlotsMap.Find(InSlot);
    if(ValuePtr == nullptr)
    {
        return nullptr;
    }

    return *ValuePtr;
}

bool UAGR_InventoryComponent::IsItemEquipped_Implementation(
    TArray<FGameplayTag>& OutEquippedSlots,
    const AActor* InItem) const
{
    OutEquippedSlots.Empty();

    if(!IsValid(InItem))
    {
        return false;
    }

    TArray<FGameplayTag> Keys;
    ActiveEquipmentSlotsMap.GetKeys(Keys);

    for(const FGameplayTag& Slot : Keys)
    {
        const AActor* FoundItemInSlot = GetItemInSlot(Slot);
        if(FoundItemInSlot == InItem)
        {
            OutEquippedSlots.AddUnique(Slot);
        }
    }

    return !OutEquippedSlots.IsEmpty();
}

TArray<AActor*> UAGR_InventoryComponent::GetAllEquippedItems_Implementation() const
{
    TArray<AActor*> EquippedItems;

    TArray<FGameplayTag> Keys;
    ActiveEquipmentSlotsMap.GetKeys(Keys);

    for(const FGameplayTag& Slot : Keys)
    {
        AActor* FoundItemInSlot = GetItemInSlot(Slot);
        if(IsValid(FoundItemInSlot))
        {
            EquippedItems.AddUnique(FoundItemInSlot);
        }
    }

    return MoveTemp(EquippedItems);
}

TArray<AActor*> UAGR_InventoryComponent::GetAllItems_Implementation() const
{
    const AActor* Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return {};
    }

    const FName ItemActorTag = UAGR_InventoryFunctionLibrary::GetItemActorTag();
    if(ItemActorTag == NAME_None)
    {
        return {};
    }

    TArray<AActor*> Items;

    const APawn* Instigator = Owner->GetInstigator();
    if(Owner != Instigator && IsValid(Instigator))
    {
        Instigator->GetAttachedActors(Items);
    }
    else
    {
        Owner->GetAttachedActors(Items);
    }

    // Remove any actor that does not have the item actor tag
    for(auto It = Items.CreateIterator(); It; ++It)
    {
        if(!(*It)->Tags.Contains(ItemActorTag))
        {
            It.RemoveCurrent();
        }
    }

    if(!IsValid(InventoryContainer))
    {
        return Items;
    }

    // Also include items that are in an inventory's container (i.e. items attached to container).
    TArray<AActor*> ItemsInContainer;
    InventoryContainer->GetAttachedActors(ItemsInContainer);
    Items.Append(ItemsInContainer);

    return MoveTemp(Items);
}

// ReSharper disable once CppPassValueParameterByConstReference
TArray<AActor*> UAGR_InventoryComponent::GetAllItemsByClass_Implementation(TSubclassOf<AActor> InClass) const
{
    const TArray<AActor*> AllItems = GetAllItems();

    TArray<AActor*> Items;
    UKismetArrayLibrary::FilterArray(AllItems, InClass, Items);

    return Items;
}

TArray<AActor*> UAGR_InventoryComponent::GetAllItemsByTags_Implementation(
    const TArray<FName>& InTags,
    const bool bInMatchAll) const
{
    const TArray<AActor*> AllItems = GetAllItems();

    TArray<AActor*> Items;
    for(AActor* const Item : AllItems)
    {
        const UAGR_ItemComponent* const ItemComponent = UAGR_InventoryFunctionLibrary::GetItemComponent(Item);
        if(!IsValid(ItemComponent))
        {
            continue;
        }

        TArray<FName> ItemTags = ItemComponent->ItemTags;

        bool bAllTagsMatching = true;
        for(const FName& Tag : InTags)
        {
            const bool bContains = ItemTags.Contains(Tag);

            if(bInMatchAll)
            {
                // Case: Match all tags
                if(!bContains)
                {
                    bAllTagsMatching = false;
                    break;
                }
            }
            else
            {
                // Case: Match any tag is enough
                if(bContains)
                {
                    Items.AddUnique(Item);
                    break;
                }
            }
        }

        if(bInMatchAll && bAllTagsMatching)
        {
            Items.AddUnique(Item);
        }
    }

    return MoveTemp(Items);
}

TArray<AActor*> UAGR_InventoryComponent::GetAllItemsByGameplayTag_Implementation(
    const FGameplayTag InGameplayTag) const
{
    TArray<AActor*> Items;
    const TArray<AActor*> AllItems = GetAllItems();

    for(AActor* const Item : AllItems)
    {
        const UAGR_ItemComponent* ItemComponent = UAGR_InventoryFunctionLibrary::GetItemComponent(Item);
        if(!IsValid(ItemComponent))
        {
            continue;
        }

        // Order of comparison is important here. For example, we want to allow matching "A.1" by searching for "A".
        if(ItemComponent->ItemType.MatchesTag(InGameplayTag))
        {
            Items.AddUnique(Item);
        }
    }

    return MoveTemp(Items);
}

TArray<AActor*> UAGR_InventoryComponent::GetAllItemsByName_Implementation(
    const FText& InName,
    const bool bInCaseSensitive) const
{
    TArray<AActor*> Items;
    const TArray<AActor*> AllItems = GetAllItems();

    for(AActor* const Item : AllItems)
    {
        const UAGR_ItemComponent* ItemComponent = UAGR_InventoryFunctionLibrary::GetItemComponent(Item);
        if(!IsValid(ItemComponent))
        {
            continue;
        }

        const bool bFoundMatchingName = bInCaseSensitive
                                        ? ItemComponent->ItemName.EqualTo(InName)
                                        : ItemComponent->ItemName.EqualToCaseIgnored(InName);
        if(bFoundMatchingName)
        {
            Items.AddUnique(Item);
        }
    }

    return MoveTemp(Items);
}

FTransform UAGR_InventoryComponent::CalculateDropTransform_Implementation(
    const AActor* InItem,
    const float InDistance) const
{
    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return {};
    }

    if(!IsValid(InItem))
    {
        return {};
    }

    const UAGR_ItemComponent* ItemComponent = UAGR_InventoryFunctionLibrary::GetItemComponent(InItem);
    if(!IsValid(ItemComponent))
    {
        return {};
    }

    const APawn* Instigator = InItem->GetInstigator();
    const AActor* ReferenceActor = IsValid(Instigator) ? Instigator : Owner;

    FTransform Transform;
    Transform.SetRotation(ReferenceActor->GetActorTransform().GetRotation());

    if(ItemComponent->GetWasSimulatingPhysics())
    {
        Transform.SetLocation(
            ReferenceActor->GetActorForwardVector() * InDistance
            + ReferenceActor->GetActorTransform().GetLocation());

        return Transform;
    }

    const UWorld* World = GetWorld();
    if(!IsValid(World))
    {
        return {};
    }

    const FVector LineTraceStart = ReferenceActor->GetActorTransform().GetLocation();
    FVector LineTraceEnd = LineTraceStart;
    LineTraceEnd.Z += InDistance * -1.f;
    TArray<AActor*> ActorsToIgnore;
    ReferenceActor->GetAttachedActors(ActorsToIgnore);

    FHitResult HitResult;
    const bool bHit = UKismetSystemLibrary::LineTraceSingle(
        World,
        LineTraceStart,
        LineTraceEnd,
        UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility),
        false,
        ActorsToIgnore,
        EDrawDebugTrace::Type::None,
        HitResult,
        true);

    Transform.SetLocation(bHit ? HitResult.Location : ReferenceActor->GetActorTransform().GetLocation());

    return Transform;
}

// ReSharper disable once CppPassValueParameterByConstReference
// ReSharper disable once CppMemberFunctionMayBeStatic
int64 UAGR_InventoryComponent::CountAllStacks_Implementation(const TArray<AActor*>& InItems) const
{
    int64 StackTotal = 0;
    for(const AActor* Item : InItems)
    {
        const UAGR_ItemComponent* ItemComponent = UAGR_InventoryFunctionLibrary::GetItemComponent(Item);
        if(!IsValid(ItemComponent))
        {
            continue;
        }

        StackTotal += ItemComponent->GetStackCount();
    }

    return StackTotal;
}

bool UAGR_InventoryComponent::IsItemInInventory_Implementation(const AActor* InItem) const
{
    if(!IsValid(InItem))
    {
        return false;
    }

    const TArray<AActor*> AllItems = GetAllItems();

    return AllItems.Contains(InItem);
}

void UAGR_InventoryComponent::SetSlot(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    AActor* InItem,
    const FGameplayTag InSlot)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    if(!SlotExists(InSlot))
    {
        OutErrorMessage = FString::Printf(TEXT("Slot '%s' does not exist"), *InSlot.ToString());
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        OutErrorMessage = "Owner is invalid";
        return;
    }

    if(!Owner->HasAuthority())
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    if(InItem == nullptr)
    {
        // Clear the slot
        ActiveEquipmentSlotsMap.Emplace(InSlot, InItem);
    }
    else
    {
        // Set or update the slot
        if(!IsValid(InItem))
        {
            OutErrorMessage = "Item is invalid";
            return;
        }

        ActiveEquipmentSlotsMap.Add(InSlot, InItem);
    }

    SerializeActiveEquipmentSlotsMap();

    bOutSuccess = true;
}

bool UAGR_InventoryComponent::SlotExists_Implementation(const FGameplayTag InSlot) const
{
    return ActiveEquipmentSlotsMap.Contains(InSlot);
}

// ReSharper disable once CppUE4BlueprintCallableFunctionMayBeConst
void UAGR_InventoryComponent::AddStacks_Implementation(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    UClass* InItemClass,
    const int64 InStacks)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    if(!IsValid(InItemClass))
    {
        OutErrorMessage = "Item class is invalid";
        return;
    }

    if(InStacks <= 0)
    {
        OutErrorMessage = "Stacks to add must be greater than 0";
        return;
    }

    if(GetOwnerRole() != ROLE_Authority)
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    int64 StacksToAdd = InStacks;

    UWorld* const World = GetWorld();
    const AActor* Owner = GetOwner();
    if(!IsValid(World) || !IsValid(Owner))
    {
        OutErrorMessage = "World or owner is invalid";
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParameters.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

    while(StacksToAdd > 0)
    {
        const FTransform NewItemTransform{
            Owner->GetActorRotation(),
            Owner->GetActorLocation(),
            FVector::OneVector
        };

        AActor* Item = World->SpawnActor<AActor>(
            InItemClass,
            NewItemTransform,
            SpawnParameters);

        UAGR_ItemComponent* const ItemComponent = UAGR_InventoryFunctionLibrary::GetItemComponent(Item);
        if(!IsValid(ItemComponent))
        {
            // If we reach this point it means any actor spawned from InItemClass will never have an AGR Item component.

            OutErrorMessage = FString::Printf(
                TEXT("Actor spawned using Item Class '%s' does not have an AGR Item component"),
                *InItemClass->GetName());

            // Destroy unnecessary actor.
            World->DestroyActor(Item);

            // We have to abort here to prevent getting stuck in an infinite loop.
            return;
        }

        if(StacksToAdd <= ItemComponent->MaxStackCount)
        {
            ItemComponent->SetStackCount(StacksToAdd);
            StacksToAdd = 0;
        }
        else
        {
            ItemComponent->SetStackCount(ItemComponent->MaxStackCount);
            StacksToAdd -= ItemComponent->GetStackCount();
        }

        bool bPickUpItemSuccess = false;
        FString PickUpItemErrorMessage;
        ItemComponent->PickUpItem(
            bPickUpItemSuccess,
            PickUpItemErrorMessage,
            this,
            true,
            true,
            false);

        if(!bPickUpItemSuccess)
        {
            // NOTE: Should never fail. It most likely is a mistake by the developer.
            AGR_LOG(
                LogAGR_Inventory_Runtime,
                Error,
                "Could not pick up item while adding stacks: %s",
                *PickUpItemErrorMessage);
            return;
        }
    }

    bOutSuccess = true;
}

// ReSharper disable once CppUE4BlueprintCallableFunctionMayBeConst
void UAGR_InventoryComponent::RemoveStacks_Implementation(
    bool& bOutSuccess,
    FString& OutErrorMessage,
    TArray<AActor*>& InItems,
    const int64 InStacks)
{
    bOutSuccess = false;
    OutErrorMessage = "";

    if(InStacks <= 0)
    {
        OutErrorMessage = "Stacks to remove must be greater than 0";
        return;
    }

    if(GetOwnerRole() != ROLE_Authority)
    {
        OutErrorMessage = "Should be called with authority only";
        return;
    }

    int64 StacksToRemove = InStacks;
    for(const AActor* const Item : InItems)
    {
        UAGR_ItemComponent* const ItemComponent = UAGR_InventoryFunctionLibrary::GetItemComponent(Item);
        if(!IsValid(ItemComponent))
        {
            continue;
        }

        if(!ItemComponent->bIsStackable)
        {
            continue;
        }

        if(ItemComponent->GetStackCount() > StacksToRemove)
        {
            ItemComponent->SetStackCount(ItemComponent->GetStackCount() - StacksToRemove);
            OnItemUpdated.Broadcast(Item, EAGR_ItemUpdateType::Updated);
            break;
        }

        StacksToRemove -= ItemComponent->GetStackCount();

        bool bDestroyItemSuccess = false;
        FString DestroyItemErrorMessage;
        ItemComponent->DestroyItem(
            bDestroyItemSuccess,
            DestroyItemErrorMessage,
            this);

        if(!bDestroyItemSuccess)
        {
            OutErrorMessage = FString::Printf(
                TEXT("Remove stacks failed when destroying item (Destroying forcefully anyway): %s"),
                *DestroyItemErrorMessage);

            // Clean up by removing the actor directly.
            UWorld* World = GetWorld();
            if(!IsValid(World))
            {
                World->DestroyActor(ItemComponent->GetOwner());
            }

            return;
        }
    }

    bOutSuccess = true;
}