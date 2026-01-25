// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Projectile/Actors/AGR_Projectile.h"

#include "Projectile/Components/AGR_ProjectileSphereComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_Projectile)

AAGR_Projectile::AAGR_Projectile(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UAGR_ProjectileSphereComponent>("ProjectileRoot"))
{
}