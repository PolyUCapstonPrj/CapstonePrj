// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "Types/AGR_CombatEnums.h"
#include "Types/AGR_CombatStructs.h"
#include "Kismet/KismetSystemLibrary.h"

namespace AGR_CombatUtils
{
    /**
     * Performs a line trace by channel.
     * 
     * @param ActorComponent The actor component performing the trace.
     * @param TraceHitMode Determines if the trace detects a single hit or multiple hits.
     * @param TraceParams Parameters defining the trace operation.
     * @param ActorsToIgnore Actors to ignore when tracing.
     * @returns list of hit results from the line trace.
     */
    static TArray<FHitResult> LineTraceByChannel(
        const UActorComponent* ActorComponent,
        const EAGR_TraceHitMode TraceHitMode,
        const FAGR_TraceParams& TraceParams,
        const TArray<AActor*>& ActorsToIgnore)
    {
        TArray<FHitResult> HitResults;
        if(!IsValid(ActorComponent))
        {
            return HitResults;
        }

        switch(TraceHitMode)
        {
        case EAGR_TraceHitMode::SingleHit:
            {
                FHitResult HitResult;
                const bool bHit = UKismetSystemLibrary::LineTraceSingle(
                    ActorComponent,
                    TraceParams.GetStart(),
                    TraceParams.GetEnd(),
                    TraceParams.TraceChannel,
                    TraceParams.bTraceComplex,
                    ActorsToIgnore,
                    TraceParams.DrawDebugType,
                    HitResult,
                    TraceParams.bIgnoreSelf,
                    TraceParams.SingleFrameTraceColor,
                    TraceParams.TraceHitColor,
                    TraceParams.DrawTime);

                if(bHit)
                {
                    HitResults.Add(MoveTemp(HitResult));
                }

                break;
            };
        case EAGR_TraceHitMode::MultipleHits:
            {
                UKismetSystemLibrary::LineTraceMulti(
                    ActorComponent,
                    TraceParams.GetStart(),
                    TraceParams.GetEnd(),
                    TraceParams.TraceChannel,
                    TraceParams.bTraceComplex,
                    ActorsToIgnore,
                    TraceParams.DrawDebugType,
                    HitResults,
                    TraceParams.bIgnoreSelf,
                    TraceParams.MultiFrameTraceColor,
                    TraceParams.TraceHitColor,
                    TraceParams.DrawTime);
                break;
            };
        default:
            {
                checkNoEntry();
                return HitResults;
            };
        }

        return HitResults;
    };

    /**
     * Performs a box trace by channel.
     * 
     * @param ActorComponent The actor component performing the trace.
     * @param TraceHitMode Determines if the trace detects a single hit or multiple hits.
     * @param TraceParams Parameters defining the trace operation.
     * @param ActorsToIgnore Actors to ignore when tracing.
     * @returns list of hit results from the box trace.
     */
    static TArray<FHitResult> BoxTraceByChannel(
        const UActorComponent* ActorComponent,
        const EAGR_TraceHitMode TraceHitMode,
        const FAGR_TraceParams& TraceParams,
        const TArray<AActor*>& ActorsToIgnore)
    {
        TArray<FHitResult> HitResults;
        if(!IsValid(ActorComponent))
        {
            return HitResults;
        }

        switch(TraceHitMode)
        {
        case EAGR_TraceHitMode::SingleHit:
            {
                FHitResult HitResult;
                const bool bHit = UKismetSystemLibrary::BoxTraceSingle(
                    ActorComponent,
                    TraceParams.GetStart(),
                    TraceParams.GetEnd(),
                    TraceParams.GetBoxHalfSize(),
                    TraceParams.GetBoxOrientation(),
                    TraceParams.TraceChannel,
                    TraceParams.bTraceComplex,
                    ActorsToIgnore,
                    TraceParams.DrawDebugType,
                    HitResult,
                    TraceParams.bIgnoreSelf,
                    TraceParams.SingleFrameTraceColor,
                    TraceParams.TraceHitColor,
                    TraceParams.DrawTime);

                if(bHit)
                {
                    HitResults.Add(MoveTemp(HitResult));
                }

                break;
            };
        case EAGR_TraceHitMode::MultipleHits:
            {
                UKismetSystemLibrary::BoxTraceMulti(
                    ActorComponent,
                    TraceParams.GetStart(),
                    TraceParams.GetEnd(),
                    TraceParams.GetBoxHalfSize(),
                    TraceParams.GetBoxOrientation(),
                    TraceParams.TraceChannel,
                    TraceParams.bTraceComplex,
                    ActorsToIgnore,
                    TraceParams.DrawDebugType,
                    HitResults,
                    TraceParams.bIgnoreSelf,
                    TraceParams.MultiFrameTraceColor,
                    TraceParams.TraceHitColor,
                    TraceParams.DrawTime);
                break;
            };
        default:
            {
                checkNoEntry();
                return HitResults;
            };
        }

        return HitResults;
    };

    /**
     * Performs a sphere trace by channel.
     * 
     * @param ActorComponent The actor component performing the trace.
     * @param TraceHitMode Determines if the trace detects a single hit or multiple hits.
     * @param TraceParams Parameters defining the trace operation.
     * @param ActorsToIgnore Actors to ignore when tracing.
     * @returns list of hit results from the sphere trace.
     */
    static TArray<FHitResult> SphereTraceByChannel(
        const UActorComponent* ActorComponent,
        const EAGR_TraceHitMode TraceHitMode,
        const FAGR_TraceParams& TraceParams,
        const TArray<AActor*>& ActorsToIgnore)
    {
        TArray<FHitResult> HitResults;
        if(!IsValid(ActorComponent))
        {
            return HitResults;
        }

        switch(TraceHitMode)
        {
        case EAGR_TraceHitMode::SingleHit:
            {
                FHitResult HitResult;
                const bool bHit = UKismetSystemLibrary::SphereTraceSingle(
                    ActorComponent,
                    TraceParams.GetStart(),
                    TraceParams.GetEnd(),
                    TraceParams.GetSphereRadius(),
                    TraceParams.TraceChannel,
                    TraceParams.bTraceComplex,
                    ActorsToIgnore,
                    TraceParams.DrawDebugType,
                    HitResult,
                    TraceParams.bIgnoreSelf,
                    TraceParams.SingleFrameTraceColor,
                    TraceParams.TraceHitColor,
                    TraceParams.DrawTime);

                if(bHit)
                {
                    HitResults.Add(MoveTemp(HitResult));
                }

                break;
            };
        case EAGR_TraceHitMode::MultipleHits:
            {
                UKismetSystemLibrary::SphereTraceMulti(
                    ActorComponent,
                    TraceParams.GetStart(),
                    TraceParams.GetEnd(),
                    TraceParams.GetSphereRadius(),
                    TraceParams.TraceChannel,
                    TraceParams.bTraceComplex,
                    ActorsToIgnore,
                    TraceParams.DrawDebugType,
                    HitResults,
                    TraceParams.bIgnoreSelf,
                    TraceParams.MultiFrameTraceColor,
                    TraceParams.TraceHitColor,
                    TraceParams.DrawTime);
                break;
            };
        default:
            {
                checkNoEntry();
                return HitResults;
            };
        }

        return HitResults;
    };

    /**
     * Performs a trace using the specified shape (line, box, or sphere) by channel.
     * 
     * @param ActorComponent The actor component performing the trace.
     * @param TraceShape The shape of the trace (e.g., line, box, or sphere).
     * @param TraceHitMode Determines if the trace detects a single hit or multiple hits.
     * @param ShapeBoundsExtent The size or radius of the shape.
     * @param TraceParams Parameters defining the trace operation.
     * @param ActorsToIgnore Actors to ignore when tracing.
     * @return list of hit results from the trace of the selected shape.
     */
    static TArray<FHitResult> ShapeTraceByChannel(
        const UActorComponent* ActorComponent,
        const EAGR_TraceShape TraceShape,
        const float ShapeBoundsExtent,
        const EAGR_TraceHitMode TraceHitMode,
        const FAGR_TraceParams& TraceParams,
        const TArray<AActor*>& ActorsToIgnore)
    {
        switch(TraceShape)
        {
        case EAGR_TraceShape::Line:
            {
                return LineTraceByChannel(ActorComponent, TraceHitMode, TraceParams, ActorsToIgnore);
            }
        case EAGR_TraceShape::Box:
            {
                const FRotator Orientation = UKismetMathLibrary::FindLookAtRotation(
                    TraceParams.GetStart(),
                    TraceParams.GetEnd());
                FAGR_TraceParams ModifiedTraceParams = TraceParams;
                ModifiedTraceParams.SetBoxOrientation(Orientation);
                ModifiedTraceParams.SetBoxHalfSize(FVector(ShapeBoundsExtent / 2.0f));
                return BoxTraceByChannel(ActorComponent, TraceHitMode, ModifiedTraceParams, ActorsToIgnore);
            }
        case EAGR_TraceShape::Sphere:
            {
                FAGR_TraceParams ModifiedTraceParams = TraceParams;
                ModifiedTraceParams.SetSphereRadius(ShapeBoundsExtent / 2.0f);
                return SphereTraceByChannel(ActorComponent, TraceHitMode, ModifiedTraceParams, ActorsToIgnore);
            }
        default:
            {
                checkNoEntry();
                return {};
            };
        }
    };

}