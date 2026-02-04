// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScope
#include "Projectile/Actors/AGR_ProjectileBase.h"

#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "Libs/AGR_CoreFunctionLibrary.h"
#include "Module/AGR_Projectile_ProjectSettings.h"
#include "Module/AGR_Projectile_RuntimeLogs.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Projectile/Components/AGR_ProjectileMovementComponent.h"
#include "Projectile/Components/AGR_ProjectileSphereComponent.h"
#include "Projectile/Lib/AGR_ProjectileFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_ProjectileBase)

AAGR_ProjectileBase::AAGR_ProjectileBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    AAGR_ProjectileBase::SetReplicateMovement(true);
    bNetLoadOnClient = false;

    // Projectile, expose on spawn
    ProjectileRadius = 0.9f;
    PenetrationTraceChannel = TraceTypeQuery1;
    InitialSpeed = 0.0f; // Will be set by the Projectile Launcher component
    bDebugDraw = false;
    DebugDuration = 5.0f;
    IgnoredActors = {};

    // User data, expose on spawn
    WeaponTags = FGameplayTagContainer{};
    WeaponDamage = 1.f;

    // Projectile
    BounceLimit = 1;
    RicochetFactor = 1.0f;
    PenetrationPower = 10.f;

    // optimization
    bServerCorrection = false;

    // internal
    DebugLastFrameLocation = FVector::ZeroVector;
    Bounces = 0;
    SpawnLocation = FVector::ZeroVector;
    LastImpactVelocity = FVector::ZeroVector;

    ProjectileRoot = CreateDefaultSubobject<USceneComponent>("ProjectileRoot");
    SetRootComponent(ProjectileRoot);

    ProjectileMovementComponent = CreateDefaultSubobject<UAGR_ProjectileMovementComponent>(
        "ProjectileMovementComponent");
}

void AAGR_ProjectileBase::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    check(IsValid(ProjectileMovementComponent));
    ProjectileMovementComponent->InitialSpeed = InitialSpeed;
    ProjectileMovementComponent->bRotationFollowsVelocity = !bServerCorrection;

    UAGR_ProjectileSphereComponent* ProjectileSphereComponent = Cast<UAGR_ProjectileSphereComponent>(RootComponent);
    if(IsValid(ProjectileSphereComponent))
    {
        ProjectileSphereComponent->SetSphereRadius(ProjectileRadius);
    }

    TryApplyActorIgnoreList(RootComponent);
}

void AAGR_ProjectileBase::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    DrawDebugLineForProjectileServerOnly();
    ApplyServerProjectileCorrection();
}

void AAGR_ProjectileBase::OnProjectileTerminalHit_Implementation(const FHitResult& InHitResult)
{
    // Override this function to add your custom event handler.
}

void AAGR_ProjectileBase::OnProjectileRicocheted_Implementation(
    const FVector& InVelocity,
    const FHitResult& InHitResult,
    const int32 InBounces,
    const FAGR_RicochetResult& InRicochetResult,
    const FTransform& HitTransform,
    const FTransform& BounceTransform)
{
    // Override this function to add your custom event handler.
}

void AAGR_ProjectileBase::OnProjectilePenetrated_Implementation(
    const FVector& InVelocity,
    const FHitResult& InEntryHitResult,
    const FAGR_PenetrationResult& InPenetrationResult)
{
    // Override this function to add your custom event handler.
}

void AAGR_ProjectileBase::BeginPlay()
{
    Super::BeginPlay();

    if(!HasAuthority())
    {
        return;
    }

    SpawnLocation = GetActorLocation();
    DebugLastFrameLocation = SpawnLocation;

    ProjectileProjectSettings = GetProjectileProjectSettings();

    check(IsValid(ProjectileMovementComponent));

    ProjectileMovementComponent->OnProjectileStop.AddUniqueDynamic(
        this,
        &AAGR_ProjectileBase::HandleProjectileStop);

    ProjectileMovementComponent->OnProjectileBounce.AddUniqueDynamic(
        this,
        &AAGR_ProjectileBase::HandleProjectileBounce);
}

void AAGR_ProjectileBase::CanPenetrate_Implementation(
    FAGR_PenetrationResult& OutPenetrationResult,
    const FHitResult& InImpactHitResult,
    const FVector& InVelocity)
{
    OutPenetrationResult = FAGR_PenetrationResult{};

    const UWorld* World = GetWorld();
    if(!IsValid(World))
    {
        return;
    }

    const UPhysicalMaterial& PhysMaterial = InImpactHitResult.PhysMaterial.IsValid()
                                            ? *InImpactHitResult.PhysMaterial
                                            : *GetDefault<UPhysicalMaterial>();

    OutPenetrationResult.PenetrationDepth = CalculatePenetrationDepth(
        PenetrationPower,
        InVelocity.Length() / (100.0f * 100.0f),
        PhysMaterial.Density,
        PhysMaterial.Strength.ShearStrength);

    if(OutPenetrationResult.PenetrationDepth <= 0.0f)
    {
        return;
    }

    TArray<FHitResult> HitResults;
    const FVector TraceEnd = InImpactHitResult.ImpactPoint;
    const FVector TraceStart =
        TraceEnd + UKismetMathLibrary::Normal(InVelocity, 0.0001f) * OutPenetrationResult.PenetrationDepth;

    FColor TraceColor = FColor::Black;

#if ENABLE_DRAW_DEBUG
    TraceColor = ProjectileProjectSettings->DebugColor_XRayTrace;
#endif

    /*
     * If no hit was registered, it can have the following reasons:
     * - Mesh is one-sided so there is no visible geometry on the other side that could serve as penetration exit point.
     * - Mesh is set to ignore the penetration trace channel.
     */
    const bool bHit = UAGR_CoreFunctionLibrary::XRaytrace(
        HitResults,
        World,
        PenetrationTraceChannel,
        {},
        TraceStart,
        TraceEnd,
        true,
        IgnoredActors,
        bDebugDraw ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
        true,
        TraceColor,
        TraceColor,
        DebugDuration);

    if(!bHit)
    {
        return;
    }

    OutPenetrationResult.PenetrateHitResult = HitResults.Last();

    // Distance will be zero if the trace started and ended in the same object, i.e. the mesh is too thick to penetrate.
    if(OutPenetrationResult.PenetrateHitResult.Distance <= 0.0f)
    {
        return;
    }

    OutPenetrationResult.PenetrationRatio = OutPenetrationResult.PenetrateHitResult.Distance
                                            / OutPenetrationResult.PenetrationDepth;

    OutPenetrationResult.EntryTransform = CalculateHitTransform(
        InImpactHitResult,
        InVelocity);

    OutPenetrationResult.ExitTransform = CalculateHitTransform(
        OutPenetrationResult.PenetrateHitResult,
        UKismetMathLibrary::NegateVector(InVelocity));

    OutPenetrationResult.bShouldPenetrate = true;
}

void AAGR_ProjectileBase::CanRicochet_Implementation(
    FAGR_RicochetResult& OutRicochetResult,
    const FHitResult& InHitResult,
    const FVector& InVelocity) const
{
    OutRicochetResult = FAGR_RicochetResult{};

    OutRicochetResult.RicochetAngle = UAGR_ProjectileFunctionLibrary::CalculateAngleOfEmergence(
        InVelocity,
        InHitResult.ImpactNormal);

    const UPhysicalMaterial& PhysMaterial = InHitResult.PhysMaterial.IsValid()
                                            ? *InHitResult.PhysMaterial
                                            : *GetDefault<UPhysicalMaterial>();
    // Ranges of factors:
    //   Friction     :  0.0 ...   1.0
    //   Restitution  :  0.0 ...   1.0
    //   Density      :  0.0 ... ~20.0
    //   RicochetAngle: >0.0 ...  90.0
    const float Friction = FMath::Max(0.0f, 1.0f - PhysMaterial.Friction);
    const float Restitution = PhysMaterial.Restitution;
    const float Density = (1.0f / (1.0f + PhysMaterial.Density));
    const float AngleCosine = FMath::Cos(OutRicochetResult.RicochetAngle);

    OutRicochetResult.RicochetChance = FMath::Clamp(
        RicochetFactor * Friction * Restitution * Density * AngleCosine,
        0.0f,
        1.0f);

    OutRicochetResult.bShouldRicochet = UKismetMathLibrary::RandomBoolWithWeight(OutRicochetResult.RicochetChance);
}

FTransform AAGR_ProjectileBase::CalculateHitTransform(
    const FHitResult& InHitResult,
    const FVector& InVelocity)
{
    const FVector TargetVector = InHitResult.ImpactPoint
                                 + (UKismetMathLibrary::Vector_IsNearlyZero(InVelocity, 0.0001)
                                    ? UKismetMathLibrary::NegateVector(InHitResult.ImpactNormal)
                                    : InVelocity);

    const FRotator HitRotation = UKismetMathLibrary::FindLookAtRotation(InHitResult.ImpactPoint, TargetVector);

    FTransform HitTransform;
    HitTransform.SetLocation(InHitResult.ImpactPoint);
    HitTransform.SetRotation(HitRotation.Quaternion());
    HitTransform.SetScale3D(FVector::OneVector);

    return HitTransform;
}

FTransform AAGR_ProjectileBase::CalculateBounceTransform(
    const FHitResult& InHitResult,
    const UProjectileMovementComponent* InProjectileMovementComponent)
{
    if(!IsValid(InProjectileMovementComponent))
    {
        return FTransform{};
    }

    const FVector ActualVelocity = InProjectileMovementComponent->LimitVelocity(
        InProjectileMovementComponent->Velocity);

    const FVector TargetVector = InHitResult.ImpactPoint
                                 + UKismetMathLibrary::MirrorVectorByNormal(
                                     UKismetMathLibrary::NegateVector(ActualVelocity),
                                     InHitResult.ImpactNormal);

    const FRotator BounceRotation = UKismetMathLibrary::FindLookAtRotation(InHitResult.ImpactPoint, TargetVector);

    FTransform BounceTransform;
    BounceTransform.SetLocation(InHitResult.ImpactPoint);
    BounceTransform.SetRotation(BounceRotation.Quaternion());
    BounceTransform.SetScale3D(FVector::OneVector);

    return BounceTransform;
}

void AAGR_ProjectileBase::CalculatePostPenetrationValues_Implementation(
    FVector& OutNewVelocity,
    float& OutNewPenetrationPower,
    const FHitResult& InImpactHitResult,
    const FVector& InCurrentVelocity,
    const float& InPenetrationRatio)
{
    const float PenetrationRatio = FMath::Clamp(InPenetrationRatio, 0.0f, 1.0f);
    OutNewVelocity = InCurrentVelocity * FMath::Sqrt(PenetrationRatio);
    PenetrationPower *= PenetrationRatio;
}

void AAGR_ProjectileBase::HandleProjectileStop(const FHitResult& InHitResult)
{
    OnProjectileTerminalHit(InHitResult);

#if ENABLE_DRAW_DEBUG
    DrawDebugProjectileHit(InHitResult, ProjectileProjectSettings->DebugColor_TerminalHit);
#endif

    Destroy();
}

void AAGR_ProjectileBase::HandleProjectileBounce(
    const FHitResult& InHitResult,
    const FVector& InImpactVelocity)
{
    LastImpactVelocity = InImpactVelocity;

    FAGR_RicochetResult RicochetResult;
    CanRicochet(RicochetResult, InHitResult, InImpactVelocity);
    if(RicochetResult.bShouldRicochet)
    {
        const bool bCanBounce = Bounces < BounceLimit;
        if(!bCanBounce)
        {
            if(IsValid(ProjectileMovementComponent))
            {
                ProjectileMovementComponent->StopSimulating(InHitResult);
            }

            return;
        }

        Bounces++;

        const FTransform HitTransform = CalculateHitTransform(
            InHitResult,
            InImpactVelocity);

        const FTransform BounceTransform = CalculateBounceTransform(
            InHitResult,
            ProjectileMovementComponent);

#if ENABLE_DRAW_DEBUG
        DrawDebugProjectileHit(InHitResult, ProjectileProjectSettings->DebugColor_Ricochet);
#endif

        OnProjectileRicocheted(
            InImpactVelocity,
            InHitResult,
            Bounces,
            RicochetResult,
            HitTransform,
            BounceTransform);
        return;
    }

    FAGR_PenetrationResult PenetrationResult;
    CanPenetrate(PenetrationResult, InHitResult, InImpactVelocity);
    if(PenetrationResult.bShouldPenetrate)
    {
#if ENABLE_DRAW_DEBUG
        DrawDebugProjectileHit(
            InHitResult,
            ProjectileProjectSettings->DebugColor_PenetrationEnter);
        DrawDebugProjectileHit(
            PenetrationResult.PenetrateHitResult,
            ProjectileProjectSettings->DebugColor_PenetrationExit);
#endif

        PerformPenetration(
            PenetrationResult.PenetrateHitResult,
            InImpactVelocity,
            PenetrationResult.PenetrationRatio);

        OnProjectilePenetrated(
            InImpactVelocity,
            InHitResult,
            PenetrationResult);
        return;
    }

    if(IsValid(ProjectileMovementComponent))
    {
        ProjectileMovementComponent->StopSimulating(InHitResult);
    }
}

float AAGR_ProjectileBase::CalculatePenetrationDepth(
    const float InPenetrationPower,
    const float InVelocity,
    const float InDensity,
    const float InHardness)
{
    if(InDensity <= 0.0f || InHardness <= 0.0f)
    {
        return 0.0f;
    }

    return InPenetrationPower * InVelocity * InVelocity / (InDensity * InHardness * 2.0f);
}

void AAGR_ProjectileBase::PerformPenetration(
    const FHitResult& InHitResult,
    const FVector& InVelocity,
    const float InPenetrationRatio)
{
    if(InPenetrationRatio <= 0)
    {
        AGR_LOG(LogAGR_Projectile_Runtime, Error, "Penetration ratio must be greater than zero.");
        return;
    }

    const FVector ExitLocation = UKismetMathLibrary::Normal(InVelocity, 0.0001)
                                 * ProjectileRadius
                                 + InHitResult.ImpactPoint;
    SetActorLocation(
        ExitLocation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

    if(!IsValid(ProjectileMovementComponent))
    {
        return;
    }

    CalculatePostPenetrationValues(
        ProjectileMovementComponent->Velocity,
        PenetrationPower,
        InHitResult,
        InVelocity,
        InPenetrationRatio);

    ProjectileMovementComponent->UpdateComponentVelocity();
}

void AAGR_ProjectileBase::TryApplyActorIgnoreList(USceneComponent* SceneComponent)
{
    UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent);
    if(!IsValid(PrimitiveComponent))
    {
        AGR_LOG(
            LogAGR_Projectile_Runtime,
            Error,
            "Your AAGR_ProjectileBase subclass '%s' uses an invalid setup! Ensure that the 'Component Class' of"
            " 'ProjectileRoot' is a subclass of UPrimitiveComponent. For more details, see header file of the class.",
            *GetClass()->GetName());
        return;
    }

    PrimitiveComponent->ClearMoveIgnoreActors();
    for(const TObjectPtr<AActor>& Actor : IgnoredActors)
    {
        PrimitiveComponent->IgnoreActorWhenMoving(Actor, true);
    }
}

void AAGR_ProjectileBase::ApplyServerProjectileCorrection()
{
    if(!bServerCorrection)
    {
        return;
    }

    if(!HasAuthority())
    {
        return;
    }

    SetActorRotation(
        UAGR_ProjectileFunctionLibrary::FindActorVelocityRotation(this),
        ETeleportType::TeleportPhysics);
}

UAGR_Projectile_ProjectSettings* AAGR_ProjectileBase::GetProjectileProjectSettings()
{
    UAGR_Projectile_ProjectSettings* PluginProjectSettings = UAGR_Projectile_ProjectSettings::Get();
    check(IsValid(PluginProjectSettings));
    return PluginProjectSettings;
}

void AAGR_ProjectileBase::DrawDebugProjectileHit(
    const FHitResult& InHitResult,
    const FColor& InColor)
{
    if(!bDebugDraw)
    {
        return;
    }

    // Only the server have the updated projectile location in order to draw the debug lines correctly.
    if(!HasAuthority())
    {
        return;
    }

    const UWorld* World = GetWorld();
    if(!IsValid(World))
    {
        return;
    }

    DrawDebugSphere(
        World,
        InHitResult.ImpactPoint,
        ProjectileRadius,
        12,
        InColor,
        false,
        DebugDuration,
        SDPG_World,
        0.5f);

    DrawDebugLineForProjectile(
        LastImpactVelocity.Length(),
        InHitResult.ImpactPoint,
        ProjectileProjectSettings->DebugColor_TraceStart,
        ProjectileProjectSettings->DebugColor_TraceEnd,
        0.5f);
}

void AAGR_ProjectileBase::DrawDebugLineForProjectile(
    const float InSpeed,
    const FVector& InLineEndLocation,
    const FColor& InStartColor,
    const FColor& InEndColor,
    const float InThickness)
{
    const UWorld* World = GetWorld();
    if(!IsValid(World))
    {
        return;
    }

    if(InitialSpeed <= 0)
    {
        return;
    }

    const FLinearColor DebugColor = UKismetMathLibrary::LinearColorLerp(
        InStartColor,
        InEndColor,
        1 - InSpeed / InitialSpeed);
    DrawDebugLine(
        World,
        DebugLastFrameLocation,
        InLineEndLocation,
        DebugColor.ToFColor(false),
        false,
        DebugDuration,
        SDPG_World,
        InThickness);

    DebugLastFrameLocation = InLineEndLocation;
}

void AAGR_ProjectileBase::DrawDebugLineForProjectileServerOnly()
{
    if(!HasAuthority())
    {
        return;
    }

#if ENABLE_DRAW_DEBUG
    if(bDebugDraw)
    {
        DrawDebugLineForProjectile(
            GetVelocity().Length(),
            GetActorLocation(),
            ProjectileProjectSettings->DebugColor_TraceStart,
            ProjectileProjectSettings->DebugColor_TraceEnd,
            0.5f);
    }
#endif
}