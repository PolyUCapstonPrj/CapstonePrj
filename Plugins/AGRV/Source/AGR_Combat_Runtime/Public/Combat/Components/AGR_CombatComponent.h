// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Types/AGR_CombatEnums.h"
#include "Types/AGR_CombatStructs.h"

#include "AGR_CombatComponent.generated.h"

class UAGR_SocketTracerComponent;
class UAGR_ArcTracerComponent;

using FTraceHitResult = TMap<TObjectPtr<AActor>, FHitResult>;

/**
 * Internal struct that keeps info about a tracer component and its associated gameplay tag.
 */
USTRUCT()
struct FAGR_TracerDescriptor
{
    GENERATED_BODY()

    /**
     * Tracer ID.
     */
    UPROPERTY()
    FGameplayTag Id = FGameplayTag::EmptyTag;

    /**
     * Pointer of the tracer component.
     */
    UPROPERTY()
    TObjectPtr<UActorComponent> Component{nullptr};
};

/**
 * Component that manages combat gameplay mechanics and handles registering and triggering tracer components.
 */
UCLASS(
    ClassGroup="3Studio",
    Blueprintable,
    BlueprintType,
    meta=(BlueprintSpawnableComponent),
    PrioritizeCategories="3Studio AGR")
class AGR_COMBAT_RUNTIME_API UAGR_CombatComponent : public UActorComponent
{
    GENERATED_BODY()

private:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_TracerHit_DynamicMulticast,
        UActorComponent*,
        TracerComponent,
        const FHitResult&,
        HitResult);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
        FAGR_TracerUpdated_DynamicMulticast,
        EAGR_TracerUpdateType,
        TracerUpdateType,
        const FGameplayTag&,
        Id,
        UActorComponent*,
        TracerComponent);

public:
    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|Combat")
    FAGR_TracerHit_DynamicMulticast OnUniqueHit;

    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|Combat")
    FAGR_TracerHit_DynamicMulticast OnHit;

    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|Combat")
    FAGR_TracerUpdated_DynamicMulticast OnTracerUpdated;

    /**
     * Specifies the number of points to be used for tracing by the tracer component.
     * This value determines how many evenly distributed segments will be created along the path.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Combat|Setup",
        SaveGame,
        meta=(UIMin="0", ClampMin="0"))
    int32 SegmentCount;

    /** Parameters for trace operations used by the AGR tracer components. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Combat|Setup", SaveGame)
    FAGR_TraceParams TraceParams;

private:
    // List of registered tracers.
    UPROPERTY(Replicated)
    TArray<FAGR_TracerDescriptor> RegisteredTracers;

    // Actors to ignore when tracing.
    UPROPERTY()
    TArray<AActor*> ActorsToIgnore;

public:
    UAGR_CombatComponent();

    //~ Begin UActorComponent
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    //~ End UActorComponent

    /**
     * Registers a tracer component with the specified ID. Updates the tracer component reference in the list
     * if the ID is already registered.
     *
     * NOTE: Tracer component must implement `UAGR_CombatInterface`.
     * 
     * @param Id The gameplay tag identifying the tracer.
     * @param TracerComponent The component to register as a tracer.
     *
     * Calls `OnTracerUpdated` event dispatcher when a new tracer is registered or updated (replaced).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Combat", meta=(AutoCreateRefTerm ="Id"))
    void RegisterTracerComponent(const FGameplayTag& Id, UActorComponent* TracerComponent);

    /**
     * Unregisters a tracer component based on a direct reference to the component.
     * 
     * @param TracerComponent A pointer to the tracer component to be unregistered.
     *
     * Calls `OnTracerUpdated` event dispatcher when a tracer is unregistered.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Combat")
    void UnregisterTracerByComponent(const UActorComponent* TracerComponent);

    /**
     * Unregisters a tracer component associated with the specified gameplay tag.
     * 
     * @param Id The gameplay tag identifying the tracer component to unregister.
     *
     * Calls `OnTracerUpdated` event dispatcher when a tracer is unregistered.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Combat", meta=(AutoCreateRefTerm ="Id"))
    void UnregisterTracerById(const FGameplayTag& Id);

    /**
     * Starts a tracing operation for a tracer identified by the given gameplay tag.
     * 
     * @param Id The gameplay tag identifying the tracer component.
     *
     * Calls `OnTracerUpdated` event dispatcher when a tracer starts tracing.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|Combat", meta=(AutoCreateRefTerm="Id"))
    void StartTracingById(const FGameplayTag& Id);

    /**
     * Ends a tracing operation for a tracer identified by the given gameplay tag.
     * 
     * @param Id The gameplay tag identifying the tracer component.
     * 
     * Calls `OnTracerUpdated` event dispatcher when a tracer ends tracing.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="3Studio AGR|Combat", meta=(AutoCreateRefTerm="Id"))
    void EndTracingById(const FGameplayTag& Id) const;

    /**
     * Builds a list of actors to ignore, including the component's owner, its instigator, 
     * and any actors attached to the instigator.
     * 
     * @returns List of actors to be ignored.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Combat")
    TArray<AActor*> BuildIgnoreList() const;

    /**
     * Retrieves the built ignore list from the trace params.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Combat")
    const TArray<AActor*>& GetIgnoreList() const;

    /**
     * Gets the number of segments that will be distributed by the tracer component over the trace path.
     *
     * @returns The segment count.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Combat")
    int32 GetSegmentCount() const;

    /**
     * Retrieves the trace parameters.
     */
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Combat")
    const FAGR_TraceParams& GetTraceParams() const;

    /**
     * Clears all the registered tracers.
     *
     * Calls `OnTracerUpdated` event dispatcher on every registered tracer. 
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Combat")
    void ClearTracers();

    /**
     * Broadcasts a hit event for a tracer identified by the given gameplay tag.
     * 
     * @param Id The gameplay tag identifying the tracer that caused the hit.
     * @param HitResult The hit result containing details of the hit.
     *
     * Calls `OnHit` event dispatcher for the `HitResult`.
     */
    void BroadcastHit(const FGameplayTag& Id, const FHitResult& HitResult) const;

    /**
     * Broadcasts a unique hit event for a tracer identified by the given gameplay tag.
     *
     * NOTE: The caller should check if the hit is unique or not.
     * 
     * @param Id The gameplay tag identifying the tracer that caused the hit.
     * @param HitResult The hit result containing details of the hit.
     *
     * Calls `OnUniqueHit` event dispatcher for the `HitResult`.
     */
    void BroadcastUniqueHit(const FGameplayTag& Id, const FHitResult& HitResult) const;

private:
    /**
     * Finds the index of a registered tracer component by its id.
     *
     * @param Id The component's ID.
     * @return The index of the tracer component in `RegisteredTracers`, or INDEX_NONE if not found.
     */
    UActorComponent* FindTracerComponentById(const FGameplayTag& Id) const;
};