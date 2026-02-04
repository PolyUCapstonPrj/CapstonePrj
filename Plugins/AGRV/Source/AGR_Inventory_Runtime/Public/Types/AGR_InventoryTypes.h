// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/EnumRange.h"

#include "AGR_InventoryTypes.generated.h"

/**
 * The action done to an equipment slot.
 */
UENUM(BlueprintType)
enum class EAGR_EquipmentSlotAction : uint8
{
    Set,
    Cleared,
};

ENUM_RANGE_BY_VALUES(
    EAGR_EquipmentSlotAction,
    EAGR_EquipmentSlotAction::Set,
    EAGR_EquipmentSlotAction::Cleared);

/**
 * The type of update done to an item.
 */
UENUM(BlueprintType)
enum class EAGR_ItemUpdateType : uint8
{
    PickedUp,
    Updated,
    Used,
    Equipped,
    Unequipped,
    Dropped,
    BeforeDestroy
};

ENUM_RANGE_BY_VALUES(
    EAGR_ItemUpdateType,
    EAGR_ItemUpdateType::PickedUp,
    EAGR_ItemUpdateType::Updated,
    EAGR_ItemUpdateType::Used,
    EAGR_ItemUpdateType::Equipped,
    EAGR_ItemUpdateType::Unequipped,
    EAGR_ItemUpdateType::Dropped,
    EAGR_ItemUpdateType::BeforeDestroy);