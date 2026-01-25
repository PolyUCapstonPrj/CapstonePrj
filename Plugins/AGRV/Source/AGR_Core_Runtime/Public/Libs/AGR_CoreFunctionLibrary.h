// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CollisionQueryParams.h"

#include "AGR_CoreFunctionLibrary.generated.h"

/**
 * AGR Core Blueprint Function Library.
 */
UCLASS()
class AGR_CORE_RUNTIME_API UAGR_CoreFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    /**
     * All possible object types in engine.
     */
    static const TArray<TEnumAsByte<EObjectTypeQuery>> AllObjectTypes;

public:
    /**
     * Does a collision trace along the given line and returns all hits encountered.
     * This only finds objects that are of a type specified by ObjectTypes and will be filtered by TraceChannel
     * and will return only blocking collision response.
     * 
     * @param OutHits Properties of the trace hit.
     * @param InWorldContextObject World context.
     * @param InTraceChannel The channel to trace.
     * @param InObjectTypes Array of Object Types to trace. If left empty, all object types will be selected.
     * @param InStart Start of line segment.
     * @param InEnd End of line segment.
     * @param bInTraceComplex True to test against complex collision, false to test against simplified collision.
     * @param InActorsToIgnore Array of actors that will be ignored during the line trace.
     * @param InDrawDebugType Draw debug trace type.
     * @param bInIgnoreSelf True to ignore self actor.
     * @param InTraceColor Trace color.
     * @param InTraceHitColor Trace hit color.
     * @param InDrawTime Draw time when type is not none.
     * @return True if there was a blocking hit, false otherwise.
     */
    UFUNCTION(
        BlueprintCallable,
        Category="3Studio AGR|Collision",
        DisplayName="X-Ray Trace",
        meta=(WorldContext="WorldContextObject",
            AutoCreateRefTerm="ActorsToIgnore,ObjectTypes",
            AdvancedDisplay="TraceColor,TraceHitColor,DrawTime",
            Keywords="multi,line,ray,trace,channel,object,block,overlap,collision"))
    static bool XRaytrace(
        UPARAM(DisplayName="Hits") TArray<FHitResult>& OutHits,
        const UObject* InWorldContextObject,
        UPARAM(DisplayName="Trace Channel") const ETraceTypeQuery InTraceChannel,
        UPARAM(DisplayName="Object Types") const TArray<TEnumAsByte<EObjectTypeQuery>>& InObjectTypes,
        UPARAM(DisplayName="Start") const FVector& InStart,
        UPARAM(DisplayName="End") const FVector& InEnd,
        UPARAM(DisplayName="Trace Complex") const bool bInTraceComplex,
        UPARAM(DisplayName="Actors To Ignore") const TArray<AActor*>& InActorsToIgnore,
        UPARAM(DisplayName="Draw Debug Type") const EDrawDebugTrace::Type InDrawDebugType,
        UPARAM(DisplayName="Ignore Self") const bool bInIgnoreSelf = true,
        UPARAM(DisplayName="Trace Color") const FLinearColor InTraceColor = FLinearColor::Red,
        UPARAM(DisplayName="Trace Hit Color") const FLinearColor InTraceHitColor = FLinearColor::Green,
        UPARAM(DisplayName="Draw Time") const float InDrawTime = 5.f);

private:
    /** Configure the collision params that will be used in the raycast.
     *  NOTE: This is a copied function from KismetTraceUtils class.
     */
    static FCollisionQueryParams ConfigureCollisionParams(
        const FName& TraceTag,
        const bool bTraceComplex,
        const TArray<AActor*>& ActorsToIgnore,
        const bool bIgnoreSelf,
        const UObject* WorldContextObject);

    /** Configure the collision object params that will be used in the raycast.
     *  NOTE: This is a copied function from KismetTraceUtils class.
     */
    static FCollisionObjectQueryParams ConfigureCollisionObjectParams(
        const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes);

};