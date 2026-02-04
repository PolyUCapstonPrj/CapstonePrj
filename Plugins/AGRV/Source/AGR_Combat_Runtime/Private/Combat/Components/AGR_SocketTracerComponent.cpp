// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable CppUseStructuredBinding
// ReSharper disable CppTooWideScope
#include "Combat/Components/AGR_SocketTracerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "Combat/Libs/AGR_CombatFunctionLibrary.h"
#include "Combat/Components/AGR_CombatComponent.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Module/AGR_Combat_RuntimeLogs.h"
#include "Utils/AGR_CombatUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_SocketTracerComponent)

UAGR_SocketTracerComponent::UAGR_SocketTracerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickInterval = 1 / 30.0f;
    SetIsReplicatedByDefault(true);

    // setup
    TraceMode = EAGR_TraceMode::Raycast;
    TraceShape = EAGR_TraceShape::Line;
    TraceHitMode = EAGR_TraceHitMode::SingleHit;
    MeshComponentTag = FName("CharacterMesh");
    Sockets.Reset();
    bOverrideSegmentCount = false;
    OverriddenSegmentCount = 5;
    bOverrideTraceParams = false;
    OverriddenTraceParams = FAGR_TraceParams{};

    // Internal
    bAllSocketsValid = true;
    SegmentLength = 0.0f;
    CachedSegmentPoints = TArray<FVector>{};
    SegmentPoints = TArray<FVector>{};
    TracerComponentState = EAGR_TracerComponentState::Ended;
    TracerId = FGameplayTag::EmptyTag;
    Owner = nullptr;
    CombatComponent = nullptr;
    MeshComponent = nullptr;
    ActorsHitResult_Wrapper = FAGR_ActorsHitResult_Wrapper{};
}

void UAGR_SocketTracerComponent::BeginPlay()
{
    Super::BeginPlay();

    UpdateReferences();
    if(!IsValid(MeshComponent.Get()))
    {
        AGR_LOG(
            LogAGR_Combat_Runtime,
            Error,
            "Could not find mesh component by tag '%s'.",
            *MeshComponentTag.ToString());
        return;
    }

    bAllSocketsValid = ValidateSocketNames();
    if(!bAllSocketsValid)
    {
        return;
    }

    SegmentLength = CalculateSocketPathLength() / GetSegmentCount();
}

void UAGR_SocketTracerComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if(!IsValid(CombatComponent.Get()))
    {
        return;
    }

    CalculateSegmentPoints();
    switch(TraceMode)
    {
    case EAGR_TraceMode::Raycast:
        {
            for(int32 SegmentIndex = 1; SegmentIndex < SegmentPoints.Num(); ++SegmentIndex)
            {
                if(TracerComponentState == EAGR_TracerComponentState::Ended)
                {
                    return;
                }

                FAGR_TraceParams TraceParams = GetTraceParams();
                TraceParams.SetStart(SegmentPoints[SegmentIndex]);
                TraceParams.SetEnd(SegmentPoints[SegmentIndex - 1]);
                TArray<FHitResult> Hits = AGR_CombatUtils::ShapeTraceByChannel(
                    this,
                    TraceShape,
                    SegmentLength,
                    TraceHitMode,
                    TraceParams,
                    CombatComponent->GetIgnoreList());

                BroadcastHits(MoveTemp(Hits));
            }

            break;
        }
    case EAGR_TraceMode::Sweep:
        {
            // Tracing requires at least 2 frames.
            if(CachedSegmentPoints.IsEmpty())
            {
                CachedSegmentPoints = SegmentPoints;
                break;
            }

            for(int32 SegmentIndex = 0; SegmentIndex < SegmentPoints.Num(); ++SegmentIndex)
            {
                if(TracerComponentState == EAGR_TracerComponentState::Ended)
                {
                    return;
                }

                FAGR_TraceParams TraceParams = GetTraceParams();
                TraceParams.SetStart(SegmentPoints[SegmentIndex]);
                TraceParams.SetEnd(CachedSegmentPoints[SegmentIndex]);
                TArray<FHitResult> Hits = AGR_CombatUtils::ShapeTraceByChannel(
                    this,
                    TraceShape,
                    SegmentLength,
                    TraceHitMode,
                    TraceParams,
                    CombatComponent->GetIgnoreList());

                BroadcastHits(MoveTemp(Hits));
            }

            CachedSegmentPoints = SegmentPoints;
            break;
        }
    default:
        {
            checkNoEntry();
            break;
        };
    }
}

void UAGR_SocketTracerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UAGR_SocketTracerComponent, TracerId);
}

void UAGR_SocketTracerComponent::SetTracerId_Implementation(const FGameplayTag& Id)
{
    TracerId = Id;
}

const FGameplayTag UAGR_SocketTracerComponent::GetTracerId_Implementation()
{
    return TracerId;
}

void UAGR_SocketTracerComponent::StartTracing_Implementation()
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

    TracerComponentState = EAGR_TracerComponentState::Started;
    CachedSegmentPoints.Reset();
    ActorsHitResult_Wrapper.ActorsHitResult.Reset();

    UpdateReferences();
    SetComponentTickEnabled(true);
    OnTraceStarted.Broadcast();
}

void UAGR_SocketTracerComponent::EndTracing_Implementation()
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
    SetComponentTickEnabled(false);
    OnTraceEnded.Broadcast(ActorsHitResult_Wrapper);

    CachedSegmentPoints.Reset();
    ActorsHitResult_Wrapper.ActorsHitResult.Reset();
}

UMeshComponent* UAGR_SocketTracerComponent::FindComponentByTag(const FName Tag) const
{
    if(!IsValid(Owner.Get()))
    {
        return nullptr;
    }

    TArray<UActorComponent*> Components = Owner->GetComponentsByTag(
        UMeshComponent::StaticClass(),
        Tag);

    if(Components.Num() <= 0)
    {
        return nullptr;
    }

    // Casting will always succeed since we searched for this specific class and have at least 1 element.
    UMeshComponent* Component = Cast<UMeshComponent>(Components[0]);
    const bool bStaticMesh = Component->GetClass()->IsChildOf(UStaticMeshComponent::StaticClass());
    const bool bSkeletalMesh = Component->GetClass()->IsChildOf(USkeletalMeshComponent::StaticClass());

    if(!bStaticMesh && !bSkeletalMesh)
    {
        AGR_LOG(
            LogAGR_Combat_Runtime,
            Error,
            "Mesh component '%s' must be a direct or a subclass of UStaticMeshComponent or USkeletalMeshComponent",
            *GetNameSafe(Component));
        return nullptr;
    }

    return Component;
}

int32 UAGR_SocketTracerComponent::GetSegmentCount() const
{
    int32 Count = 1;
    if(bOverrideSegmentCount)
    {
        Count = OverriddenSegmentCount;
    }
    else
    {
        if(IsValid(CombatComponent.Get()))
        {
            Count = CombatComponent->GetSegmentCount();
        }
    }

    Count = FMath::Max(1, Count); // Prevent potential #div/0 cases
    return Count;
}

const FAGR_TraceParams& UAGR_SocketTracerComponent::GetTraceParams() const
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

void UAGR_SocketTracerComponent::CalculateSegmentPoints()
{
    SegmentPoints.Reset();
    if(!bAllSocketsValid)
    {
        AGR_LOG(
            LogAGR_Combat_Runtime,
            Error,
            "Invalid Sockets found on the mesh component '%s' with owner '%s'.",
            *GetNameSafe(MeshComponent.Get()),
            *GetNameSafe(Owner.Get()));
        return;
    }

    if(!IsValid(MeshComponent.Get()))
    {
        AGR_LOG(
            LogAGR_Combat_Runtime,
            Error,
            "Invalid mesh component for actor '%s'.",
            *GetNameSafe(Owner.Get()));
        return;
    }

    if(Sockets.Num() < 2)
    {
        AGR_LOG(
            LogAGR_Combat_Runtime,
            Error,
            "At least 2 sockets should be available on the mesh component '%s' for actor '%s'.",
            *MeshComponent.GetName(),
            *GetNameSafe(Owner.Get()));
        return;
    }

    const int32 SegmentCount = GetSegmentCount();
    // if segment count doesn't increase resolution of the shape to trace with, then just use the socket location.
    if(Sockets.Num() >= SegmentCount)
    {
        for(const FName Socket : Sockets)
        {
            SegmentPoints.Add(MeshComponent->GetSocketLocation(Socket));
        }

        return;
    }

    FVector CurrentSocketLocation = MeshComponent->GetSocketLocation(Sockets[0]);
    SegmentPoints.Add(CurrentSocketLocation);
    FVector NextSocketLocation = MeshComponent->GetSocketLocation(Sockets[1]);

    int32 SocketIndex = 1;
    for(int32 SegmentIndex = 1; SegmentIndex < SegmentCount; ++SegmentIndex)
    {
        float RemainingSegmentLength = SegmentLength;
        float DistanceBetweenSockets = UKismetMathLibrary::Vector_Distance(CurrentSocketLocation, NextSocketLocation);

        // The while loop ensures that the required segment length is achieved by skipping over sockets that are too close.
        // If the distance between the current pair of sockets is less than the remaining segment length.
        // the algorithm reduces the remaining length by the distance to the next socket and advances to the next socket. 
        // This process continues until either:
        // - A socket pair is found where the remaining segment length can be accommodated, or
        // - All sockets are processed, at which point the last socket location is added as the final point.
        while(DistanceBetweenSockets < RemainingSegmentLength)
        {
            RemainingSegmentLength -= DistanceBetweenSockets;
            ++SocketIndex;
            CurrentSocketLocation = MoveTemp(NextSocketLocation);

            if(!Sockets.IsValidIndex(SocketIndex))
            {
                SegmentPoints.Add(MeshComponent->GetSocketLocation(Sockets.Last()));
                return;
            }

            NextSocketLocation = MeshComponent->GetSocketLocation(Sockets[SocketIndex]);
            DistanceBetweenSockets = UKismetMathLibrary::Vector_Distance(CurrentSocketLocation, NextSocketLocation);
        }

        if(RemainingSegmentLength <= 0.0f)
        {
            SegmentPoints.Add(MeshComponent->GetSocketLocation(Sockets.Last()));
            return;
        }

        FVector SegmentPoint = CurrentSocketLocation
                               + UKismetMathLibrary::Normal(NextSocketLocation - CurrentSocketLocation, 0.0001f)
                               * RemainingSegmentLength;
        SegmentPoints.Add(SegmentPoint);
        CurrentSocketLocation = MoveTemp(SegmentPoint);
    }
}

float UAGR_SocketTracerComponent::CalculateSocketPathLength()
{
    if(!bAllSocketsValid)
    {
        return 0.0f;
    }

    if(Sockets.Num() <= 0)
    {
        return 0.0f;
    }

    if(!IsValid(MeshComponent.Get()))
    {
        return 0.0f;
    }

    float Len = 0.0f;
    FVector PrevLocation = MeshComponent->GetSocketLocation(Sockets[0]);
    for(int32 i = 1; i < Sockets.Num(); ++i)
    {
        const FVector CurrLocation = MeshComponent->GetSocketLocation(Sockets[i]);
        Len += UKismetMathLibrary::Vector_Distance(PrevLocation, CurrLocation);
        PrevLocation = CurrLocation;
    }

    return Len;
}

void UAGR_SocketTracerComponent::BroadcastHits(const TArray<FHitResult>& HitResults)
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

        const bool bHitActorFirstTime = ActorsHitResult_Wrapper.ActorsHitResult.Find(HitResult.GetActor()) == nullptr;
        if(!bHitActorFirstTime)
        {
            continue;
        }

        ActorsHitResult_Wrapper.ActorsHitResult.Add(HitResult.GetActor(), HitResult);
        CombatComponent->BroadcastUniqueHit(TracerId, HitResult);
        OnUniqueHit.Broadcast(HitResult);
    }
}

void UAGR_SocketTracerComponent::UpdateReferences()
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
    MeshComponent = FindComponentByTag(MeshComponentTag);
}

bool UAGR_SocketTracerComponent::ValidateSocketNames() const
{
    if(!IsValid(MeshComponent.Get()))
    {
        return false;
    }

    for(const FName Socket : Sockets)
    {
        if(!MeshComponent->DoesSocketExist(Socket))
        {
            AGR_LOG(
                LogAGR_Combat_Runtime,
                Warning,
                "Socket '%s' does not exist on the mesh component '%s' with owner '%s'.",
                *Socket.ToString(),
                *GetNameSafe(MeshComponent.Get()),
                *GetNameSafe(Owner.Get()));

            return false;
        }
    }

    return true;
}