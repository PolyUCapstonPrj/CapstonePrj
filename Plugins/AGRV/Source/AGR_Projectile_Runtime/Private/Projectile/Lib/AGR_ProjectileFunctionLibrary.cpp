// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Projectile/Lib/AGR_ProjectileFunctionLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_ProjectileFunctionLibrary)

UAGR_ProjectileLauncherComponent* UAGR_ProjectileFunctionLibrary::GetProjectileLauncherComponent(
    const AActor* const InActor)
{
    if(!IsValid(InActor))
    {
        return nullptr;
    }

    // Try to get the projectile launcher component from: Actor
    UAGR_ProjectileLauncherComponent* const ProjectileLauncherComponent = Cast<UAGR_ProjectileLauncherComponent>(
        InActor->GetComponentByClass(UAGR_ProjectileLauncherComponent::StaticClass()));

    return ProjectileLauncherComponent;
}

void UAGR_ProjectileFunctionLibrary::CursorAim(
    bool& bOutSuccess,
    FVector& OutAimAtLocation,
    FRotator& OutLookAtRotation,
    FHitResult& OutHitResult,
    const FVector& InAimOrigin,
    const APlayerController* InPlayerController,
    const ECollisionChannel InTraceChannel,
    const bool bInTraceComplex)
{
    bOutSuccess = false;
    OutAimAtLocation = FVector::ZeroVector;
    OutLookAtRotation = FRotator::ZeroRotator;
    OutHitResult = FHitResult{};

    if(!IsValid(InPlayerController))
    {
        return;
    }

    if(!InPlayerController->GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(InTraceChannel),
        bInTraceComplex,
        OutHitResult))
    {
        return;
    }

    OutAimAtLocation = OutHitResult.ImpactPoint;
    OutLookAtRotation = UKismetMathLibrary::FindLookAtRotation(
        InAimOrigin,
        OutAimAtLocation);

    bOutSuccess = true;
}

void UAGR_ProjectileFunctionLibrary::CameraAim(
    bool& bOutSuccess,
    FVector& OutAimAtLocation,
    FRotator& OutLookAtRotation,
    FHitResult& OutHitResult,
    const FVector& InAimOrigin,
    const APlayerController* InPlayerController,
    const float InTraceDistance,
    const ECollisionChannel InTraceChannel,
    const bool bInTraceComplex,
    const TArray<AActor*>& InActorsToIgnore,
    const FVector2D& InScreenSpaceAimCoords)
{
    bOutSuccess = false;
    OutAimAtLocation = FVector::ZeroVector;
    OutLookAtRotation = FRotator::ZeroRotator;
    OutHitResult = FHitResult{};

    if(!IsValid(InPlayerController))
    {
        return;
    }

    int32 SizeX = 0;
    int32 SizeY = 0;
    InPlayerController->GetViewportSize(SizeX, SizeY);
    const FVector2D ScreenPosition = FVector2D(SizeX, SizeY) * InScreenSpaceAimCoords;

    FVector WorldLocation;
    FVector WorldDirection;
    if(!InPlayerController->DeprojectScreenPositionToWorld(
        ScreenPosition.X,
        ScreenPosition.Y,
        WorldLocation,
        WorldDirection))
    {
        return;
    }

    const bool bHit = UKismetSystemLibrary::LineTraceSingle(
        InPlayerController,
        WorldLocation,
        WorldDirection * InTraceDistance + WorldLocation,
        UEngineTypes::ConvertToTraceType(InTraceChannel),
        bInTraceComplex,
        InActorsToIgnore,
        EDrawDebugTrace::None,
        OutHitResult,
        true);

    OutAimAtLocation = bHit ? OutHitResult.ImpactPoint : OutHitResult.TraceEnd;
    OutLookAtRotation = UKismetMathLibrary::FindLookAtRotation(InAimOrigin, OutAimAtLocation);

    bOutSuccess = true;
}

FRotator UAGR_ProjectileFunctionLibrary::FindActorVelocityRotation(const AActor* InTarget)
{
    if(!IsValid(InTarget))
    {
        return FRotator::ZeroRotator;
    }

    return FindVelocityRotation(InTarget->GetActorLocation(), InTarget->GetVelocity());
}

float UAGR_ProjectileFunctionLibrary::CalculateAngleOfEmergence(const FVector& InVelocity, const FVector& InHitNormal)
{
    // NOTE: we skip dividing the dot product by the magnitudes of both vectors because they are normalized and
    // therefore the value will always be 1.
    return FMath::Acos(FVector::DotProduct(InVelocity.GetSafeNormal(0.01), InHitNormal)) - UE_DOUBLE_HALF_PI;
}

FRotator UAGR_ProjectileFunctionLibrary::FindVelocityRotation(const FVector& InLocation, const FVector& InVelocity)
{
    return UKismetMathLibrary::FindLookAtRotation(InLocation, InLocation + InVelocity);
}

TArray<AActor*> UAGR_ProjectileFunctionLibrary::BuildActorList(AActor* const InOwner)
{
    if(!IsValid(InOwner))
    {
        return TArray<AActor*>{};
    }

    TArray<AActor*> Actors;

    Actors.Add(InOwner);
    TArray<AActor*> AttachedActorsToOwner;
    InOwner->GetAttachedActors(AttachedActorsToOwner, true, true);
    Actors.Append(MoveTemp(AttachedActorsToOwner));

    APawn* const Instigator = InOwner->GetInstigator();
    if(!IsValid(Instigator))
    {
        return MoveTemp(Actors);
    }

    Actors.Add(Instigator);
    TArray<AActor*> AttachedActorsToInstigator;
    Instigator->GetAttachedActors(AttachedActorsToInstigator, true, true);
    Actors.Append(MoveTemp(AttachedActorsToInstigator));

    return MoveTemp(Actors);
}