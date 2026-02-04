// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "AGR_CombatEnums.generated.h"

/** 
 * Determines how trace points are used for performing trace operations.
 */
UENUM(BlueprintType)
enum class EAGR_TraceMode : uint8
{
    // @formatter:off
    /** Trace along each arc segment from start to end. */
    Raycast        UMETA(DisplayName="Raycast"),
    /** Trace between the current and previous points on the segment, as if sweeping through the arc. */
    Sweep         UMETA(DisplayName="Sweep"),
    // @formatter:on
};

/** 
 * Specifies the geometric shape used during the trace operation.
 */
UENUM(BlueprintType)
enum class EAGR_TraceShape : uint8
{
    // @formatter:off
    Line          UMETA(DisplayName="Line"),
    Box           UMETA(DisplayName="Box"),
    Sphere        UMETA(DisplayName="Sphere"),
    // @formatter:oN
};

/** 
 * Specifies whether the AGR tracer component should detect a single hit or multiple hits.
 */
UENUM(BlueprintType)
enum class EAGR_TraceHitMode: uint8
{
    // @formatter:off
    SingleHit           UMETA(DisplayName="Single Hit"),
    MultipleHits        UMETA(DisplayName="Multiple Hits"),
    // @formatter:oN
};

/** 
 * Specifies the state transitions of an AGR tracer component during its lifecycle.
 */
UENUM(Blueprintable, BlueprintType)
enum class EAGR_TracerUpdateType : uint8
{
    // @formatter:off
    Registered          UMETA(DisplayName="Registered"),
    Unregistered        UMETA(DisplayName="Unregistered"),
    Started             UMETA(DisplayName="Started"),
    Ended               UMETA(DisplayName="Ended"),
    // @formatter:on
};

/** 
 * Internal tracer component state. 
 */
UENUM()
enum class EAGR_TracerComponentState : uint8
{
    // @formatter:off
    Started         UMETA(DisplayName="Started"),
    Ended           UMETA(DisplayName="Ended"),
    // @formatter:on
};