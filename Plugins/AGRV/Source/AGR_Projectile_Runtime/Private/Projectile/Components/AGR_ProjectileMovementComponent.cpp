// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScope
#include "Projectile/Components/AGR_ProjectileMovementComponent.h"

#include "Engine/World.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Projectile/Lib/AGR_ProjectileFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_ProjectileMovementComponent)

UAGR_ProjectileMovementComponent::UAGR_ProjectileMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bBounceAngleAffectsFriction = true;
    bForceSubStepping = true;
    bInterpMovement = true;
    bInterpRotation = true;
    bShouldBounce = true;
    bThrottleInterpolation = true;
    bAutoRegisterPhysicsVolumeUpdates = false;
    Bounciness = 1.0f;
    BounceVelocityStopSimulatingThreshold = 20.0f;
    Friction = 1.0f;
    MaxSpeed = 150'000.0f;
}

bool UAGR_ProjectileMovementComponent::HandleDeflection(
    FHitResult& Hit,
    const FVector& OldVelocity,
    const uint32 NumBounces,
    float& SubTickTimeRemaining)
{
    const FVector Normal = ConstrainNormalToPlane(Hit.Normal);

    // Multiple hits within very short time period?
    const bool bMultiHit = (PreviousHitTime < 1.f && Hit.Time <= UE_KINDA_SMALL_NUMBER);

    // if velocity still into wall (after HandleBlockingHit() had a chance to adjust), slide along wall
    const float DotTolerance = 0.01f;
    bIsSliding = (bMultiHit && FVector::Coincident(PreviousHitNormal, Normal)) ||
                 ((Velocity.GetSafeNormal() | Normal) <= DotTolerance);

    if(bIsSliding)
    {
        if(bMultiHit && (PreviousHitNormal | Normal) <= 0.f)
        {
            //90 degree or less corner, so use cross product for direction
            FVector NewDir = (Normal ^ PreviousHitNormal);
            NewDir = NewDir.GetSafeNormal();
            Velocity = Velocity.ProjectOnToNormal(NewDir);
            if((OldVelocity | Velocity) < 0.f)
            {
                Velocity *= -1.f;
            }
            Velocity = ConstrainDirectionToPlane(Velocity);
        }
        else
        {
            // BEGIN: 3Studio
            //adjust to move along new wall
            // Velocity = ComputeSlideVector(Velocity, 1.f, Normal, Hit);
            // END: 3Studio
        }

        // Check min velocity.
        if(IsVelocityUnderSimulationThreshold())
        {
            StopSimulating(Hit);
            return false;
        }

        // Velocity is now parallel to the impact surface.
        if(SubTickTimeRemaining > UE_KINDA_SMALL_NUMBER)
        {
            if(!HandleSliding(Hit, SubTickTimeRemaining))
            {
                return false;
            }
        }
    }

    return true;
}