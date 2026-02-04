// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppUseStructuredBinding
// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable CppTooWideScope
#include "Combat/Components/AGR_ArcTracerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Combat/Libs/AGR_CombatFunctionLibrary.h"
#include "Combat/Components/AGR_CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/HitResult.h"
#include "Utils/AGR_CombatUtils.h"
#include "Module/AGR_Combat_RuntimeLogs.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_ArcTracerComponent)

UAGR_ArcTracerComponent::UAGR_ArcTracerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);

    // setup
    TraceMode = EAGR_TraceMode::Raycast;
    TraceShape = EAGR_TraceShape::Sphere;
    TraceHitMode = EAGR_TraceHitMode::SingleHit;
    ArcTraceDuration = 0.0f;
    ArcAngle = 90.0f;
    ArcRadius = 100.0f;
    ArcRadiusModifier = nullptr;
    bOverrideSegmentCount = false;
    OverriddenSegmentCount = 5;
    bOverrideTraceParams = false;
    OverriddenTraceParams = FAGR_TraceParams{};

    // internal
    SegmentLength = 0.0f;
    SegmentCount = 0;
    SegmentCountFloat = 0.0f;
    TimerSegmentIndex = 0;
    SegmentStartEnd = TArray<TPair<FVector, FVector>>{};
    TimerHandle = FTimerHandle{};
    PreviousSegmentPoints = TArray<FVector>{};
    TracerComponentState = EAGR_TracerComponentState::Ended;
    Owner = nullptr;
    CombatComponent = nullptr;
    TracerId = FGameplayTag::EmptyTag;
    ActorsHitResult_Wrapper = FAGR_ActorsHitResult_Wrapper{};
}

void UAGR_ArcTracerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UAGR_ArcTracerComponent, TracerId);
}

void UAGR_ArcTracerComponent::SetTracerId_Implementation(const FGameplayTag& Id)
{
    TracerId = Id;
}

const FGameplayTag UAGR_ArcTracerComponent::GetTracerId_Implementation()
{
    return TracerId;
}

void UAGR_ArcTracerComponent::StartTracing_Implementation()
{
    if(TracerComponentState == EAGR_TracerComponentState::Started)
    {
        AGR_LOG(
            LogAGR_Combat_Runtime,
            Warning,
            "[%s] Tracing already started! Attempting to start again. (State: %s)",
            *GetNameSafe(this),
            *UEnum::GetValueAsString(TracerComponentState));
        return;
    }

    const UWorld* World = GetWorld();
    if(!IsValid(World))
    {
        return;
    }

    TracerComponentState = EAGR_TracerComponentState::Started;
    SegmentStartEnd.Reset();
    ActorsHitResult_Wrapper.ActorsHitResult.Reset();
    PreviousSegmentPoints.Reset();
    ClearAndInvalidateTimer();

    UpdateReferences();
    OnTraceStarted.Broadcast();

    // Cache the segment values not to recalculate them everytime for efficiency.
    SegmentCount = GetSegmentCount();
    // NOTE: SegmentCountFloat can be safely used in divisions as it can never be less than 1.
    SegmentCountFloat = static_cast<float>(SegmentCount);
    SegmentLength = CalculateSegmentLength(SegmentCount, ArcRadius, ArcAngle);

    const bool bArcRadiusModifierValid = IsValid(ArcRadiusModifier.Get());
    FVector WorldLocation = GetComponentToWorld().GetLocation();

    for(int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
    {
        float ArcRadiusMultiplier = 1.0f;
        if(bArcRadiusModifierValid)
        {
            const float NormalizedTime = (SegmentIndex + 1) / SegmentCountFloat;
            ArcRadiusMultiplier = ArcRadiusModifier->GetFloatValue(NormalizedTime);
        }

        const FVector SegmentDirection = CalculateSegmentDirection(SegmentIndex);
        FVector End = WorldLocation + SegmentDirection * ArcRadius * ArcRadiusMultiplier;
        SegmentStartEnd.Add(TPair<FVector, FVector>(MoveTemp(WorldLocation), MoveTemp(End)));
    }

    switch(TraceMode)
    {
    case EAGR_TraceMode::Raycast:
        {
            if(ArcTraceDuration <= 0.0f)
            {
                for(int32 SegmentIndex = 0; SegmentIndex < SegmentStartEnd.Num(); ++SegmentIndex)
                {
                    if(TracerComponentState == EAGR_TracerComponentState::Ended)
                    {
                        return;
                    }

                    InstantSingleFrameTrace(SegmentIndex);
                }

                break;
            }

            TimerSegmentIndex = 0;
            const float Rate = ArcTraceDuration / SegmentCountFloat;
            World->GetTimerManager().SetTimer(
                TimerHandle,
                this,
                &ThisClass::TimedSingleFrameTrace,
                Rate,
                FTimerManagerTimerParameters{.bLoop = true, .bMaxOncePerFrame = false});
            break;
        }
    case EAGR_TraceMode::Sweep:
        {
            if(ArcTraceDuration <= 0.0f)
            {
                for(int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
                {
                    if(TracerComponentState == EAGR_TracerComponentState::Ended)
                    {
                        return;
                    }

                    InstantMultiFrameTrace(SegmentIndex);
                }

                break;
            }

            TimerSegmentIndex = 0;
            const float Rate = ArcTraceDuration / SegmentCountFloat;
            World->GetTimerManager().SetTimer(
                TimerHandle,
                this,
                &ThisClass::TimedMultiFrameTrace,
                Rate,
                FTimerManagerTimerParameters{.bLoop = true, .bMaxOncePerFrame = false});
            break;
        }
    default:
        {
            checkNoEntry()
            break;
        };
    }
}

void UAGR_ArcTracerComponent::EndTracing_Implementation()
{
    if(TracerComponentState == EAGR_TracerComponentState::Ended)
    {
        AGR_LOG(
            LogAGR_Combat_Runtime,
            Warning,
            "[%s] Tracing already ended! Attempting to end again. (State: %s)",
            *GetNameSafe(this),
            *UEnum::GetValueAsString(TracerComponentState));
        return;
    }

    TracerComponentState = EAGR_TracerComponentState::Ended;
    OnTraceEnded.Broadcast(ActorsHitResult_Wrapper);

    ClearAndInvalidateTimer();
    SegmentStartEnd.Reset();
    PreviousSegmentPoints.Reset();
    ActorsHitResult_Wrapper.ActorsHitResult.Reset();
}

void UAGR_ArcTracerComponent::UpdateReferences()
{
    if(!IsValid(Owner.Get()))
    {
        Owner = GetOwner();
        if(!IsValid(Owner.Get()))
        {
            return;
        }
    }

    CombatComponent = UAGR_CombatFunctionLibrary::GetCombatComponent(Owner.Get());
}

int32 UAGR_ArcTracerComponent::GetSegmentCount() const
{
    if(bOverrideSegmentCount)
    {
        return OverriddenSegmentCount;
    }

    if(!IsValid(CombatComponent.Get()))
    {
        return 0;
    }

    return CombatComponent->GetSegmentCount();
}

const FAGR_TraceParams& UAGR_ArcTracerComponent::GetTraceParams() const
{
    if(bOverrideTraceParams)
    {
        return OverriddenTraceParams;
    }

    if(!IsValid(CombatComponent.Get()))
    {
        // In an error case, return 'OverriddenTraceParams' as the default because a local or temporary variable
        // as a const reference cannot be returned.
        return OverriddenTraceParams;
    }

    return CombatComponent->GetTraceParams();
}

FVector UAGR_ArcTracerComponent::CalculateSegmentDirection(const uint32 SegmentIndex) const
{
    if(SegmentCount <= 0)
    {
        return FVector::ZeroVector;
    }

    // Calculate the angular position of the segment.
    const float SegmentSizeDegrees = ArcAngle / SegmentCountFloat;
    const float SegmentAngleDegrees = (ArcAngle / 2.0f) - (SegmentSizeDegrees * SegmentIndex);
    const float SegmentAngleRadians = FMath::DegreesToRadians(SegmentAngleDegrees);

    const FVector ForwardVector = GetComponentToWorld().GetRotation().GetForwardVector();
    const FVector RightVector = GetComponentToWorld().GetRotation().GetRightVector();

    const FVector Direction = (
        (ForwardVector * FMath::Cos(SegmentAngleRadians))
        + (RightVector * FMath::Sin(SegmentAngleRadians))
    ).GetSafeNormal(0.0001f);

    return Direction;
}

float UAGR_ArcTracerComponent::CalculateSegmentLength(
    const int32 SegmentCount,
    const float ArcRadius,
    const float ArcAngle)
{
    if(SegmentCount <= 0)
    {
        return 0.0f;
    }

    const float ArcLength = (ArcRadius * ArcAngle) / 360.0f;
    return ArcLength / static_cast<float>(SegmentCount);
}

void UAGR_ArcTracerComponent::InstantSingleFrameTrace(const int32 SegmentIndex)
{
    if(!SegmentStartEnd.IsValidIndex(SegmentIndex))
    {
        return;
    }

    if(!IsValid(CombatComponent.Get()))
    {
        return;
    }

    FAGR_TraceParams TraceParams = GetTraceParams();
    TraceParams.SetStart(SegmentStartEnd[SegmentIndex].Key);
    TraceParams.SetEnd(SegmentStartEnd[SegmentIndex].Value);
    TArray<FHitResult> HitResults = AGR_CombatUtils::ShapeTraceByChannel(
        this,
        TraceShape,
        SegmentLength,
        TraceHitMode,
        TraceParams,
        CombatComponent->GetIgnoreList());

    BroadcastHits(MoveTemp(HitResults));
}

void UAGR_ArcTracerComponent::TimedSingleFrameTrace()
{
    if(TracerComponentState == EAGR_TracerComponentState::Ended)
    {
        Execute_EndTracing(this);
        return;
    }

    if(!SegmentStartEnd.IsValidIndex(TimerSegmentIndex))
    {
        Execute_EndTracing(this);
        return;
    }

    InstantSingleFrameTrace(TimerSegmentIndex);
    ++TimerSegmentIndex;
}

void UAGR_ArcTracerComponent::InstantMultiFrameTrace(const int32 SegmentIndex)
{
    if(!SegmentStartEnd.IsValidIndex(SegmentIndex))
    {
        return;
    }

    if(!IsValid(CombatComponent.Get()))
    {
        return;
    }

    TArray<FVector> SegmentPoints = {};
    const FVector Start = SegmentStartEnd[SegmentIndex].Key;
    const FVector End = SegmentStartEnd[SegmentIndex].Value;
    for(int32 PointIndex = 0; PointIndex < SegmentCount; ++PointIndex)
    {
        const FVector SegmentPoint = FVector{Start + ((End - Start) * PointIndex / SegmentCountFloat)};
        // Don't draw first segment.
        if(SegmentIndex <= 0)
        {
            SegmentPoints.Add(SegmentPoint);
            continue;
        }

        if(!PreviousSegmentPoints.IsValidIndex(PointIndex))
        {
            break;
        }

        FAGR_TraceParams TraceParams = GetTraceParams();
        TraceParams.SetStart(PreviousSegmentPoints[PointIndex]);
        TraceParams.SetEnd(SegmentPoint);
        TArray<FHitResult> HitResults = AGR_CombatUtils::ShapeTraceByChannel(
            this,
            TraceShape,
            SegmentLength,
            TraceHitMode,
            TraceParams,
            CombatComponent->GetIgnoreList());

        SegmentPoints.Add(SegmentPoint);
        BroadcastHits(MoveTemp(HitResults));
    };

    PreviousSegmentPoints = MoveTemp(SegmentPoints);
}

void UAGR_ArcTracerComponent::TimedMultiFrameTrace()
{
    if(TracerComponentState == EAGR_TracerComponentState::Ended)
    {
        Execute_EndTracing(this);
        return;
    }

    if(!SegmentStartEnd.IsValidIndex(TimerSegmentIndex))
    {
        Execute_EndTracing(this);
        return;
    }

    InstantMultiFrameTrace(TimerSegmentIndex);
    ++TimerSegmentIndex;
}

void UAGR_ArcTracerComponent::BroadcastHits(const TArray<FHitResult>& HitResults)
{
    if(HitResults.Num() <= 0)
    {
        return;
    }

    if(!IsValid(CombatComponent.Get()))
    {
        return;
    }

    for(const FHitResult& HitResult : HitResults)
    {
        CombatComponent->BroadcastHit(TracerId, HitResult);
        OnHit.Broadcast(HitResult);

        const AActor* HitActor = HitResult.GetActor();
        if(!IsValid(HitActor))
        {
            continue;
        }

        const bool bHitActorFirstTime = ActorsHitResult_Wrapper.ActorsHitResult.Find(HitActor) == nullptr;
        if(!bHitActorFirstTime)
        {
            continue;
        }

        ActorsHitResult_Wrapper.ActorsHitResult.Add(HitResult.GetActor(), HitResult);
        CombatComponent->BroadcastUniqueHit(TracerId, HitResult);
        OnUniqueHit.Broadcast(HitResult);
    }
}

void UAGR_ArcTracerComponent::ClearAndInvalidateTimer()
{
    if(!TimerHandle.IsValid())
    {
        return;
    }

    const UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);
    if(IsValid(World))
    {
        World->GetTimerManager().ClearTimer(TimerHandle);
    }
}