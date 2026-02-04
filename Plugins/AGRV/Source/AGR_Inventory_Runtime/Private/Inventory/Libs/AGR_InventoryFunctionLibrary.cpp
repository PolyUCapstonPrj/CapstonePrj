// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Inventory/Libs/AGR_InventoryFunctionLibrary.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Module/AGR_Inventory_ProjectSettings.h"
#include "Module/AGR_Inventory_RuntimeLogs.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_InventoryFunctionLibrary)

UAGR_ItemComponent* UAGR_InventoryFunctionLibrary::GetItemComponent(const AActor* const InActor)
{
    if(!IsValid(InActor))
    {
        return nullptr;
    }

    // Try to get the item component from: Actor
    UAGR_ItemComponent* const ItemComponent = Cast<UAGR_ItemComponent>(
        InActor->GetComponentByClass(UAGR_ItemComponent::StaticClass()));
    if(IsValid(ItemComponent))
    {
        return ItemComponent;
    }

    return nullptr;
}

UAGR_InventoryComponent* UAGR_InventoryFunctionLibrary::GetInventoryComponent(const AActor* const InActor)
{
    if(!IsValid(InActor))
    {
        return nullptr;
    }

    // Try to get the inventory component from: Actor
    UAGR_InventoryComponent* InventoryComponent = Cast<UAGR_InventoryComponent>(
        InActor->GetComponentByClass(UAGR_InventoryComponent::StaticClass()));
    if(IsValid(InventoryComponent))
    {
        return InventoryComponent;
    }

    const AActor* const Owner = InActor->GetOwner();
    if(IsValid(Owner))
    {
        // Try to get the inventory component from: Actor's Owner
        InventoryComponent = Cast<UAGR_InventoryComponent>(
            Owner->GetComponentByClass(UAGR_InventoryComponent::StaticClass()));
        if(IsValid(InventoryComponent))
        {
            return InventoryComponent;
        }
    }

    const APawn* const Instigator = InActor->GetInstigator();
    if(IsValid(Instigator))
    {
        // Try to get the inventory component from: Actor's Instigator
        InventoryComponent = Cast<UAGR_InventoryComponent>(
            Instigator->GetComponentByClass(UAGR_InventoryComponent::StaticClass()));
        if(IsValid(InventoryComponent))
        {
            return InventoryComponent;
        }
    }

    const AController* const InstigatorController = InActor->GetInstigatorController();
    if(IsValid(InstigatorController))
    {
        // Try to get the inventory component from: Actor Instigator's Controller
        InventoryComponent = Cast<UAGR_InventoryComponent>(
            InstigatorController->GetComponentByClass(UAGR_InventoryComponent::StaticClass()));
        if(IsValid(InventoryComponent))
        {
            return InventoryComponent;
        }

        const APlayerState* const PlayerState = InstigatorController->PlayerState;
        if(IsValid(PlayerState))
        {
            // Try to get the inventory component from: Actor Instigator Controller's Player State
            InventoryComponent = Cast<UAGR_InventoryComponent>(
                PlayerState->GetComponentByClass(UAGR_InventoryComponent::StaticClass()));
            if(IsValid(InventoryComponent))
            {
                return InventoryComponent;
            }
        }
    }

    return nullptr;
}

FName UAGR_InventoryFunctionLibrary::GetItemActorTag()
{
    const UAGR_Inventory_ProjectSettings* PluginProjectSettings = UAGR_Inventory_ProjectSettings::Get();
    if(!IsValid(PluginProjectSettings))
    {
        AGR_LOG(
            LogAGR_Inventory_Runtime,
            Error,
            "Could not get AGR Plugin Project Settings.");
        return NAME_None;
    }

    return PluginProjectSettings->ItemActorTag;
}