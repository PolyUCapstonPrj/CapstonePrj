// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/SphereComponent.h"

#include "AGR_ProjectileSphereComponent.generated.h"

/**
 * A Sphere Component that is configured to work with AGR Projectiles.
 * See AAGR_Projectile.
 */
UCLASS(ClassGroup="3Studio", Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent))
class AGR_PROJECTILE_RUNTIME_API UAGR_ProjectileSphereComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    UAGR_ProjectileSphereComponent(const FObjectInitializer& ObjectInitializer);
};