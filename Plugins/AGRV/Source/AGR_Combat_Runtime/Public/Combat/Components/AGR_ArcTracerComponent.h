// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Combat/Interfaces/AGR_CombatInterface.h"
#include "Components/SceneComponent.h"
#include "Types/AGR_CombatEnums.h"
#include "Types/AGR_CombatStructs.h"
#include "Curves/CurveFloat.h"

#include "AGR_ArcTracerComponent.generated.h"

class UAGR_CombatComponent;

/**
 * Arc-based tracer component designed to detect hit results by drawing a custom arc using the component configuration
 */
UCLASS(
    ClassGroup="3Studio",
    Blueprintable,
    BlueprintType,
    meta=(BlueprintSpawnableComponent),
    PrioritizeCategories="3Studio AGR")
class AGR_COMBAT_RUNTIME_API UAGR_ArcTracerComponent : public USceneComponent, public IAGR_CombatInterface
{
    GENERATED_BODY()

private:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAGR_TraceStarted_DynamicMulticast);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
        FAGR_TracerHit_DynamicMulticast,
        const FHitResult&,
        HitResult);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
        FAGR_TraceEnded_DynamicMulticast,
        const FAGR_ActorsHitResult_Wrapper&,
        TraceHitResult);

public:
    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|Combat")
    FAGR_TraceStarted_DynamicMulticast OnTraceStarted;

    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|Combat")
    FAGR_TracerHit_DynamicMulticast OnHit;

    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|Combat")
    FAGR_TracerHit_DynamicMulticast OnUniqueHit;

    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|Combat")
    FAGR_TraceEnded_DynamicMulticast OnTraceEnded;

    /**
     * Determines how trace points are used for performing trace operations.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    EAGR_TraceMode TraceMode;

    /**
     * Specifies the geometric shape used during the tracing operation.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    EAGR_TraceShape TraceShape;

    /**
     * Specifies whether the trace detects a single hit or multiple hits.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    EAGR_TraceHitMode TraceHitMode;

    /**
     * Controls the duration over which traces are performed for an arc.
     *
     * NOTE: If `ArcTraceDuration` is less than or equal to 0, all traces are performed instantly in a single frame.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Combat|Setup",
        SaveGame,
        meta=(ForceUnits="Seconds", UIMin="0", ClampMin="0"))
    float ArcTraceDuration;

    /** Angle of the arc in degrees.*/
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Combat|Setup",
        SaveGame,
        meta=(ForceUnits="Degrees", UIMin="1", UIMax="360", ClampMin="1", ClampMax="360"))
    float ArcAngle;

    /** Radius of the arc */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    float ArcRadius;

    /**
     * A curve asset used to dynamically adjust the radius of the arc based on segment index.
     * 
     * - If valid, this curve provides a multiplier for each segment, scaling the arc radius.
     * - The resulting multiplier scales the end point of each trace segment, effectively modifying the arc's overall shape.
     * - If the multiplier is less than 1.0, it is clamped to 1.0 to prevent shrinking below the default radius.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup")
    TObjectPtr<UCurveFloat> ArcRadiusModifier;

    /**
     * Overrides the segment count from the combat component by a custom count.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    bool bOverrideSegmentCount;

    /**
     * Specifies the number of points to be used for tracing by the tracer component.
     * This value determines how many evenly distributed segments will be created along the path.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Combat|Setup",
        meta=(EditCondition="bOverrideSegmentCount", EditConditionHides),
        SaveGame)
    int32 OverriddenSegmentCount;

    /**
     * Overrides the trace params from the combat component with custom trace params.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    bool bOverrideTraceParams;

    /**
     * Parameters for trace operations used by the AGR tracer components.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Combat|Setup",
        meta=(EditCondition="bOverrideTraceParams", EditConditionHides),
        SaveGame)
    FAGR_TraceParams OverriddenTraceParams;

private:
    // A cached value that is updated when a new trace is started.
    float SegmentLength;

    // A cached value that is updated when a new trace is started.
    int32 SegmentCount;

    // A cached value that is updated when a new trace is started.
    float SegmentCountFloat;

    // Index of `TracesStartEnd` being used in the looping timer.
    int32 TimerSegmentIndex;

    // Keeps the start and end points of the segments.
    TArray<TPair<FVector, FVector>> SegmentStartEnd;

    // Timer handle used when `ArcTraceDuration` > 0 in an arc trace. 
    FTimerHandle TimerHandle;

    // Cached points used in the `MultiFrameTrace`.
    TArray<FVector> PreviousSegmentPoints;

    // Current state of the tracer component.
    EAGR_TracerComponentState TracerComponentState;

    // Owner of the component.
    UPROPERTY()
    TObjectPtr<AActor> Owner;

    // Combat component reference.
    UPROPERTY()
    TObjectPtr<UAGR_CombatComponent> CombatComponent;

    // ID of the tracer.
    UPROPERTY(Replicated)
    FGameplayTag TracerId;

    // The wrapped map of an actor to its hit result.
    UPROPERTY()
    FAGR_ActorsHitResult_Wrapper ActorsHitResult_Wrapper;

public:
    UAGR_ArcTracerComponent();

    //~ Begin UActorComponent
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    //~ End UActorComponent

    //~ Begin IAGR_CombatInterface
    virtual void SetTracerId_Implementation(const FGameplayTag& Id) override;
    virtual const FGameplayTag GetTracerId_Implementation() override;
    // Calls `OnTraceStarted` event dispatcher when tracing starts.
    virtual void StartTracing_Implementation() override;
    // Calls `OnTraceEnded` event dispatcher when tracing ends.
    virtual void EndTracing_Implementation() override;
    //~ End IAGR_CombatInterface

private:
    /**
     * Updates owner and combat component references.
     */
    void UpdateReferences();

    /**
     * Gets the number of segments that will be distributed by the tracer component over the trace path.
     *
     * NOTE: The value can be overridden by setting `bOverrideSegmentCount` to true and assigning value to `SegmentCount`.
     * @returns The segment count.
     */
    int32 GetSegmentCount() const;

    /**
     * Retrieves the trace parameters.
     * 
     * NOTE: The value can be overridden by setting `bOverrideTraceParams` to true and assigning value to `TraceParams`.
     */
    const FAGR_TraceParams& GetTraceParams() const;

    /**
     * Calculates the direction vector for a specific segment within an arc, based on the index of the segment.
     * The function divides the arc into equal segments and computes the angular offset for the given segment index.
     *
     * @param SegmentIndex The index of the segment within the arc. Must be in the range [0, GetSegmentCount() - 1].
     * @returns A normalized direction vector representing the orientation of the segment in world space.
     */
    FVector CalculateSegmentDirection(const uint32 SegmentIndex) const;

    /**
     * Calculates the length of an individual segment in an arc tracer.
     * 
     * @param SegmentCount The segment count.
     * @param ArcRadius Arc radius.
     * @param ArcAngle Arc angle.
     * 
     * @return The length of a single arc segment or 0.0f if the segment count is zero or less.
     */
    static float CalculateSegmentLength(const int32 SegmentCount, const float ArcRadius, const float ArcAngle);

    /**
     * Performs an instant single-frame trace for the specified segment index in the arc tracer.
     * 
     * @param SegmentIndex The index of the segment to trace.
     */
    void InstantSingleFrameTrace(const int32 SegmentIndex);

    /**
     * Performs a single-frame trace at regular intervals based on a timer.
     */
    void TimedSingleFrameTrace();

    /**
     * Performs an instant multi-frame trace for the specified segment index in the arc tracer.
     * 
     * @param SegmentIndex The index of the segment to trace.
     */
    void InstantMultiFrameTrace(const int32 SegmentIndex);

    /**
     * Performs a multi-frame trace at regular intervals based on a timer.
     */
    void TimedMultiFrameTrace();

    /**
     * Processes an array of hit results and triggers events for each hit and unique hits.
     *
     * @param HitResults An array of hit results to be processed.
     * 
     * Calls `OnHit` event dispatcher for every hit in the `Hits` array.
     * Calls `OnUniqueHit` event dispatcher for unique actors in the `Hits` array.
     */
    void BroadcastHits(const TArray<FHitResult>& HitResults);

    /**
     * Clears the active timer and invalidates the timer handle.
     */
    void ClearAndInvalidateTimer();
};