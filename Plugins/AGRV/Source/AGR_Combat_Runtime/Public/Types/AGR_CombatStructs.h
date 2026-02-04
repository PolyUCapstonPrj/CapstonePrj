// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/HitResult.h"
#include "Components/ActorComponent.h"

#include "AGR_CombatStructs.generated.h"

/**
 * This is a wrapper struct for a TMap used as a type with dynamic multicast delegates.
 */
USTRUCT(Blueprintable, BlueprintType)
struct FAGR_ActorsHitResult_Wrapper
{
    GENERATED_BODY()

    /**
     * Map of an actor to its hit result.
     */
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Combat")
    TMap<TObjectPtr<AActor>, FHitResult> ActorsHitResult;
};

/**
 * Parameters for trace operations used by the AGR tracer components.
 */
USTRUCT(Blueprintable, BlueprintType)
struct FAGR_TraceParams
{
    GENERATED_BODY()

    /**
     * The trace channel to use for the trace (e.g., camera, visibility, etc.).
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat")
    TEnumAsByte<ETraceTypeQuery> TraceChannel = UEngineTypes::ConvertToTraceType(ECC_Camera);

    /**
     * Whether to trace complex geometry (true) or simple collision shapes (false).
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat")
    bool bTraceComplex = true;

    /**
     * Whether to ignore the actor initiating the trace.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat")
    bool bIgnoreSelf = true;

    /**
     * Type of debug visualization for the trace.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat")
    TEnumAsByte<EDrawDebugTrace::Type> DrawDebugType = EDrawDebugTrace::None;

    /**
     * Duration for which debug visuals should persist (in seconds).
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat")
    float DrawTime = 0.5f;

    /**
     * Color used for single frame trace debug visuals.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat")
    FLinearColor SingleFrameTraceColor = FColor::FromHex("#428A11FF");

    /**
     * Color used for multi frame trace debug visuals.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat")
    FLinearColor MultiFrameTraceColor = FColor::FromHex("#057FFFFF");

    /**
     * Color used for debug visuals when a trace hits an actor.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat")
    FLinearColor TraceHitColor = FColor::FromHex("#FF2B55FF");

private:
    /**
     * Start position of the trace.
     */
    UPROPERTY()
    FVector Start = FVector::ZeroVector;

    /**
     * End position of the trace.
     */
    UPROPERTY()
    FVector End = FVector::ZeroVector;

    /**
     * Half size of the box for box traces.
     */
    UPROPERTY()
    FVector BoxHalfSize = FVector::ZeroVector;

    /**
     * Orientation of the box for box traces.
     */
    UPROPERTY()
    FRotator BoxOrientation = FRotator::ZeroRotator;

    /**
     * Radius for sphere traces.
     */
    UPROPERTY()
    float SphereRadius = 0.0f;

public:
    /**
     * Set the start position of the trace.
     * @param InStart Start position.
     */
    void SetStart(const FVector& InStart)
    {
        Start = InStart;
    }

    /**
     * Get the start position of the trace.
     * @return Start position.
     */
    const FVector& GetStart() const
    {
        return Start;
    };

    /**
     * Set the end position of the trace.
     * @param InEnd End position.
     */
    void SetEnd(const FVector& InEnd)
    {
        End = InEnd;
    }

    /**
     * Get the end position of the trace.
     * @return End position.
     */
    const FVector& GetEnd() const
    {
        return End;
    };

    /**
     * Set the half size of the box for box traces.
     * @param InHalfSize Half size.
     */
    void SetBoxHalfSize(const FVector& InHalfSize)
    {
        BoxHalfSize = InHalfSize;
    }

    /**
     * Get the half size of the box for box traces.
     * @return Half size.
     */
    const FVector& GetBoxHalfSize() const
    {
        return BoxHalfSize;
    };

    /**
     * Set the orientation of the box for box traces.
     * @param InOrientation Orientation.
     */
    void SetBoxOrientation(const FRotator& InOrientation)
    {
        BoxOrientation = InOrientation;
    }

    /**
     * Get the orientation of the box for box traces.
     * @return Orientation.
     */
    const FRotator& GetBoxOrientation() const
    {
        return BoxOrientation;
    };

    /**
     * Set the radius for sphere traces.
     * @param InRadius Radius.
     */
    void SetSphereRadius(const float InRadius)
    {
        SphereRadius = InRadius;
    }

    /**
     * Get the radius for sphere traces.
     * @return Radius.
     */
    float GetSphereRadius() const
    {
        return SphereRadius;
    }
};