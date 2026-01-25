// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "AGR_ProjectileBase.h"

#include "AGR_Projectile.generated.h"

/**
 * AGR Projectiles have the ability to ricochet and penetrate objects depending on the projectile's properties as well
 * as the Physical Material of hit objects if assigned.
 *
 * Use the AGR Projectile Launcher Component to implement weapons that shall fire those projectiles.
 */
UCLASS(Abstract)
class AGR_PROJECTILE_RUNTIME_API AAGR_Projectile : public AAGR_ProjectileBase
{
    GENERATED_BODY()

public:
    AAGR_Projectile(const FObjectInitializer& ObjectInitializer);
};