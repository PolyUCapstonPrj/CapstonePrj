// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScopeInitStatement
#include "Combat/Libs/AGR_CombatFunctionLibrary.h"
#include "Combat/Components/AGR_CombatComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_CombatFunctionLibrary)

UAGR_CombatComponent* UAGR_CombatFunctionLibrary::GetCombatComponent(const AActor* Actor)
{
    if(!IsValid(Actor))
    {
        return nullptr;
    }

    UClass* const ComponentClass = UAGR_CombatComponent::StaticClass();
    // Try to get the Combat component from: Actor
    UAGR_CombatComponent* CombatComponent = Cast<UAGR_CombatComponent>(
        Actor->GetComponentByClass(ComponentClass));
    if(IsValid(CombatComponent))
    {
        return CombatComponent;
    }

    const AActor* const Owner = Actor->GetOwner();
    if(IsValid(Owner))
    {
        // Try to get the Combat component from: Actor's Owner
        CombatComponent = Cast<UAGR_CombatComponent>(Owner->GetComponentByClass(ComponentClass));
        if(IsValid(CombatComponent))
        {
            return CombatComponent;
        }
    }

    const APawn* const Instigator = Actor->GetInstigator();
    if(!IsValid(Instigator))
    {
        return nullptr;
    }

    // Try to get the Combat component from: Actor's Instigator
    CombatComponent = Cast<UAGR_CombatComponent>(Instigator->GetComponentByClass(ComponentClass));
    if(IsValid(CombatComponent))
    {
        return CombatComponent;
    }

    const AController* const InstigatorController = Actor->GetInstigatorController();
    if(!IsValid(InstigatorController))
    {
        return nullptr;
    }

    // Try to get the Combat component from: Actor Instigator's Controller
    CombatComponent = Cast<UAGR_CombatComponent>(InstigatorController->GetComponentByClass(ComponentClass));
    if(IsValid(CombatComponent))
    {
        return CombatComponent;
    }

    const APlayerState* const PlayerState = InstigatorController->PlayerState;
    if(!IsValid(PlayerState))
    {
        return nullptr;
    }

    // Try to get the Combat component from: Actor Instigator Controller's Player State
    CombatComponent = Cast<UAGR_CombatComponent>(PlayerState->GetComponentByClass(ComponentClass));
    if(IsValid(CombatComponent))
    {
        return CombatComponent;
    }

    return nullptr;
}