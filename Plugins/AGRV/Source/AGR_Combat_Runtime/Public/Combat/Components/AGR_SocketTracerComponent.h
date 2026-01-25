// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Combat/Interfaces/AGR_CombatInterface.h"
#include "Components/ActorComponent.h"
#include "Types/AGR_CombatEnums.h"
#include "Types/AGR_CombatStructs.h"

#include "AGR_SocketTracerComponent.generated.h"

class UAGR_CombatComponent;
class UMeshComponent;

/**
 * Socket-based tracer component designed to detect hit results using socket-defined points and customizable trace shapes.
 */
UCLASS(
    ClassGroup="3Studio",
    Blueprintable,
    BlueprintType,
    meta=(BlueprintSpawnableComponent),
    PrioritizeCategories="3Studio AGR")
class AGR_COMBAT_RUNTIME_API UAGR_SocketTracerComponent : public UActorComponent, public IAGR_CombatInterface
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

    /** Determines how trace points are used for performing trace operations. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    EAGR_TraceMode TraceMode;

    /** Specifies the geometric shape used during the tracing operation. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    EAGR_TraceShape TraceShape;

    /** Specifies whether the trace detects a single hit or multiple hits. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    EAGR_TraceHitMode TraceHitMode;

    /** The tag that should be available on the mesh that will be used for sockets. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    FName MeshComponentTag;

    /** Add minimum of 2 sockets to this array. Preferably make fist socket the tip of the tracer. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    TArray<FName> Sockets;

    /** Overrides the segment count from the combat component by a custom count. */
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
        meta=(EditCondition="bOverrideSegmentCount", EditConditionHides, UIMin="0", ClampMin="0"),
        SaveGame)
    int32 OverriddenSegmentCount;

    /** Overrides the trace params from the combat component with custom trace params. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    bool bOverrideTraceParams;

    /** Parameters for trace operations used by the AGR tracer components. */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Combat|Setup",
        meta=(EditCondition="bOverrideTraceParams", EditConditionHides),
        SaveGame)
    FAGR_TraceParams OverriddenTraceParams;

private:
    // Set to true at begin play if all provided sockets are valid on the mesh component.
    bool bAllSocketsValid;

    // A cached value that is set on begin play.
    float SegmentLength;

    // Cached points of the previous frame used in the `MultiFrameTrace`.
    TArray<FVector> CachedSegmentPoints;

    // Cached points of the current frame.
    TArray<FVector> SegmentPoints;

    // Current state of the tracer component.
    EAGR_TracerComponentState TracerComponentState;

    // ID of the tracer.
    UPROPERTY(Replicated)
    FGameplayTag TracerId;

    // Owner of the component.
    UPROPERTY()
    TObjectPtr<AActor> Owner;

    // Combat component reference.
    UPROPERTY()
    TObjectPtr<UAGR_CombatComponent> CombatComponent;

    // Mesh component reference that will be used for tracing.
    UPROPERTY()
    TObjectPtr<UMeshComponent> MeshComponent;

    // The wrapped map of an actor to its hit result.
    UPROPERTY()
    FAGR_ActorsHitResult_Wrapper ActorsHitResult_Wrapper;

public:
    UAGR_SocketTracerComponent();

    //~ Begin UActorComponent
    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    //~ End UActorComponent

    //~ Begin IAGR_MeleeCombatInterface
    virtual void SetTracerId_Implementation(const FGameplayTag& Id) override;
    virtual const FGameplayTag GetTracerId_Implementation() override;
    // Calls `OnTraceStarted` event dispatcher when tracing starts.
    virtual void StartTracing_Implementation() override;
    // Calls `OnTraceEnded` event dispatcher when tracing ends.
    virtual void EndTracing_Implementation() override;
    //~ End IAGR_MeleeCombatInterface

private:
    /**
     * Finds and returns the first `UMeshComponent` on the owner actor with the specified tag.
     * 
     * NOTE: The component can only be a UStaticMeshComponent or USkeletalMeshComponent.
     *
     * @param Tag The component tag.
     * @return A pointer to the found `UMeshComponent`, or nullptr if no valid component is found.
     */
    UMeshComponent* FindComponentByTag(const FName Tag) const;

    /**
     * Gets the number of segments that will be distributed by the tracer component over the trace path.
     *
     * NOTE: The value can be overridden by setting `bOverrideSegmentCount` to true and assigning value to `OverriddenSegmentCount`.
     * @returns The segment count.
     */
    int32 GetSegmentCount() const;

    /**
     * Retrieves the trace parameters.
     * 
     * NOTE: The value can be overridden by setting `bOverrideTraceParams` to true and assigning value to `OverriddenTraceParams`.
     */
    const FAGR_TraceParams& GetTraceParams() const;

    /**
     * Calculates the `SegmentPoints` used for tracing along the sockets of the reference component.
     * 
     * Divides the total distance between sockets into evenly distributed segments, ensuring the desired resolution
     * for the tracer component.
     */
    void CalculateSegmentPoints();

    /**
     * Calculates the total length of the path defined by the sockets on the reference component.
     * 
     * @returns The path length or 0 if no valid sockets or component exists.
     */
    float CalculateSocketPathLength();

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
     * Updates owner and combat component references.
     */
    void UpdateReferences();

    /**
     * Validates the existence of all socket names in the `Sockets` array on the reference component.
     */
    bool ValidateSocketNames() const;
};