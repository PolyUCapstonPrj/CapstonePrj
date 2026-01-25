// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable CppUseStructuredBinding
// ReSharper disable CppTooWideScope
#include "Combat/Components/AGR_CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Module/AGR_Combat_RuntimeLogs.h"
#include "Combat/Components/AGR_ArcTracerComponent.h"
#include "Combat/Components/AGR_SocketTracerComponent.h"
#include "Combat/Interfaces/AGR_CombatInterface.h"
#include "Engine/HitResult.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_CombatComponent)

UAGR_CombatComponent::UAGR_CombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);

    // Setup
    SegmentCount = 20;
    TraceParams = FAGR_TraceParams{};
    RegisteredTracers = TArray<FAGR_TracerDescriptor>{};
    ActorsToIgnore = TArray<AActor*>{};
}

void UAGR_CombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams Params;
    Params.bIsPushBased = true;
    DOREPLIFETIME_WITH_PARAMS_FAST(UAGR_CombatComponent, RegisteredTracers, Params);
}

void UAGR_CombatComponent::RegisterTracerComponent(const FGameplayTag& Id, UActorComponent* TracerComponent)
{
    if(GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    if(!IsValid(TracerComponent))
    {
        return;
    }

    if(!Id.IsValid())
    {
        return;
    }

    if(!TracerComponent->Implements<UAGR_CombatInterface>())
    {
        AGR_LOG(
            LogAGR_Combat_Runtime,
            Error,
            "Failed to register tracer component '%s' due to missing implementation of interface UAGR_CombatInterface.",
            *GetNameSafe(TracerComponent));
        return;
    }

    // If an existing tracer is already registered with the same ID, remove it and add the new tracer.
    for(auto It = RegisteredTracers.CreateIterator(); It; ++It)
    {
        if(It->Id.MatchesTagExact(Id))
        {
            OnTracerUpdated.Broadcast(EAGR_TracerUpdateType::Unregistered, Id, It->Component.Get());
            It.RemoveCurrent();
            break;
        }
    }

    RegisteredTracers.Add(FAGR_TracerDescriptor{Id, TracerComponent});
    IAGR_CombatInterface::Execute_SetTracerId(TracerComponent, Id);
    OnTracerUpdated.Broadcast(EAGR_TracerUpdateType::Registered, Id, TracerComponent);
}

void UAGR_CombatComponent::UnregisterTracerByComponent(const UActorComponent* TracerComponent)
{
    if(GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    if(!IsValid(TracerComponent))
    {
        return;
    }

    for(auto It = RegisteredTracers.CreateIterator(); It; ++It)
    {
        if(It->Component == TracerComponent)
        {
            OnTracerUpdated.Broadcast(EAGR_TracerUpdateType::Unregistered, It->Id, It->Component.Get());
            It.RemoveCurrent();
            return;
        }
    }
}

void UAGR_CombatComponent::UnregisterTracerById(const FGameplayTag& Id)
{
    if(GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    if(!Id.IsValid())
    {
        return;
    }

    for(auto It = RegisteredTracers.CreateIterator(); It; ++It)
    {
        if(It->Id.MatchesTagExact(Id))
        {
            OnTracerUpdated.Broadcast(EAGR_TracerUpdateType::Unregistered, It->Id, It->Component.Get());
            It.RemoveCurrent();
            return;
        }
    }
}

void UAGR_CombatComponent::StartTracingById(const FGameplayTag& Id)
{
    UActorComponent* TracerComponent = FindTracerComponentById(Id);
    if(!IsValid(TracerComponent))
    {
        return;
    }

    ActorsToIgnore = BuildIgnoreList_Implementation();
    if(TracerComponent->Implements<UAGR_CombatInterface>())
    {
        IAGR_CombatInterface::Execute_StartTracing(TracerComponent);
        OnTracerUpdated.Broadcast(EAGR_TracerUpdateType::Started, Id, TracerComponent);
    }
}

void UAGR_CombatComponent::EndTracingById(const FGameplayTag& Id) const
{
    UActorComponent* TracerComponent = FindTracerComponentById(Id);
    if(!IsValid(TracerComponent))
    {
        return;
    }

    if(TracerComponent->Implements<UAGR_CombatInterface>())
    {
        IAGR_CombatInterface::Execute_EndTracing(TracerComponent);
        OnTracerUpdated.Broadcast(EAGR_TracerUpdateType::Ended, Id, TracerComponent);
    }
}

TArray<AActor*> UAGR_CombatComponent::BuildIgnoreList_Implementation() const
{
    AActor* Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return {};
    }

    TArray<AActor*> IgnoreList;
    IgnoreList.Add(Owner);
    APawn* Instigator = Owner->GetInstigator();
    if(!IsValid(Instigator))
    {
        return IgnoreList;
    }

    IgnoreList.Add(Instigator);
    TArray<AActor*> AttachedActors;
    Instigator->GetAttachedActors(AttachedActors, true, true);
    IgnoreList.Append(AttachedActors);
    return IgnoreList;
}

const TArray<AActor*>& UAGR_CombatComponent::GetIgnoreList() const
{
    return ActorsToIgnore;
}

int32 UAGR_CombatComponent::GetSegmentCount() const
{
    return SegmentCount;
}

const FAGR_TraceParams& UAGR_CombatComponent::GetTraceParams() const
{
    return TraceParams;
}

void UAGR_CombatComponent::ClearTracers()
{
    if(GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    for(auto It = RegisteredTracers.CreateIterator(); It; ++It)
    {
        OnTracerUpdated.Broadcast(EAGR_TracerUpdateType::Unregistered, It->Id, It->Component.Get());
        It.RemoveCurrent();
    }
}

void UAGR_CombatComponent::BroadcastHit(const FGameplayTag& Id, const FHitResult& HitResult) const
{
    UActorComponent* TracerComponent = FindTracerComponentById(Id);
    if(!IsValid(TracerComponent))
    {
        return;
    }

    OnHit.Broadcast(TracerComponent, HitResult);
}

void UAGR_CombatComponent::BroadcastUniqueHit(const FGameplayTag& Id, const FHitResult& HitResult) const
{
    UActorComponent* TracerComponent = FindTracerComponentById(Id);
    if(!IsValid(TracerComponent))
    {
        return;
    }

    OnUniqueHit.Broadcast(TracerComponent, HitResult);
}

UActorComponent* UAGR_CombatComponent::FindTracerComponentById(const FGameplayTag& Id) const
{
    for(const FAGR_TracerDescriptor& TracerDescriptor : RegisteredTracers)
    {
        if(TracerDescriptor.Id.MatchesTagExact(Id))
        {
            return TracerDescriptor.Component.Get();
        }
    }

    return nullptr;
}