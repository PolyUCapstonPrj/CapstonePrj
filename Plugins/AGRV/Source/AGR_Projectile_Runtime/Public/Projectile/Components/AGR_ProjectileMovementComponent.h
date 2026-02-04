// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "AGR_ProjectileMovementComponent.generated.h"

class AAGR_ProjectileBase;

/**
 * Custom implementation of the original Projectile Movement Component to support projectile penetration logic of
 * AGR Projectiles.
 */
UCLASS(ClassGroup="3Studio", Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent))
class AGR_PROJECTILE_RUNTIME_API UAGR_ProjectileMovementComponent : public UProjectileMovementComponent
{
    GENERATED_BODY()

public:
    UAGR_ProjectileMovementComponent(const FObjectInitializer& ObjectInitializer);

protected:
    /**
     * NOTE: Code and comments from engine source with little adjustments.
     *
     * Handle a blocking hit after HandleBlockingHit() returns a result indicating that deflection occured.
     * Default implementation increments NumBounces, checks conditions that could indicate a slide, and calls HandleSliding() if necessary.
     * 
     * @param  Hit					Blocking hit that occurred. May be changed to indicate the last hit result of further movement.
     * @param  OldVelocity			Velocity at the start of the simulation update sub-step. Current Velocity may differ (as a result of a bounce).
     * @param  NumBounces			Number of bounces that have occurred thus far in the tick.
     * @param  SubTickTimeRemaining	Time remaining in the simulation sub-step. May be changed to indicate change to remaining time.
     * @return True if simulation of the projectile should continue, false otherwise.
     * @see HandleSliding()
     */
    virtual bool HandleDeflection(
        FHitResult& Hit,
        const FVector& OldVelocity,
        const uint32 NumBounces,
        float& SubTickTimeRemaining) override;
};