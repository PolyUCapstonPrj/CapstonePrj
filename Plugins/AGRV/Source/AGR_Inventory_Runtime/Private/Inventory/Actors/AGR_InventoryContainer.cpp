// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Inventory/Actors/AGR_InventoryContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_InventoryContainer)

AAGR_InventoryContainer::AAGR_InventoryContainer()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    bAlwaysRelevant = true;
    bNetLoadOnClient = false;
    bReplicates = true;
    SetMinNetUpdateFrequency(1.0f);
    SetHidden(true);
    SetReplicatingMovement(false);
    SetCanBeDamaged(false);
    bEnableAutoLODGeneration = false;

#if WITH_EDITORONLY_DATA
    bIsSpatiallyLoaded = false;
#endif

    SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));
}