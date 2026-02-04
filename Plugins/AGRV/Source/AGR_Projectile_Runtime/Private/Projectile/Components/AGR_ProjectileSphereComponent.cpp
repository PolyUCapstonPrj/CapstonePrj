// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Projectile/Components/AGR_ProjectileSphereComponent.h"

#include "Engine/CollisionProfile.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_ProjectileSphereComponent)

UAGR_ProjectileSphereComponent::UAGR_ProjectileSphereComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bEnableAutoLODGeneration = false;
    UPrimitiveComponent::GetBodyInstance()->SetMassOverride(0.2, false);
    bApplyImpulseOnDamage = false;
    UPrimitiveComponent::GetBodyInstance()->bNotifyRigidBodyCollision = true; // Simulation generates hit events
    SetGenerateOverlapEvents(false);
    CanCharacterStepUpOn = ECB_No;
    UPrimitiveComponent::SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
    UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    UPrimitiveComponent::SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    UPrimitiveComponent::SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    bTraceComplexOnMove = false;
    bReturnMaterialOnMove = true;
    bReceiveMobileCSMShadows = false;
}