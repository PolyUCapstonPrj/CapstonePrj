// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScope
#include "Projectile/Components/AGR_ProjectileLauncherComponent.h"

#include "TimerManager.h"
#include "Engine/TimerHandle.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Module/AGR_Core_RuntimeLogs.h"
#include "Module/AGR_Projectile_RuntimeLogs.h"
#include "Net/UnrealNetwork.h"
#include "Projectile/Actors/AGR_ProjectileBase.h"
#include "Projectile/Lib/AGR_ProjectileFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_ProjectileLauncherComponent)

UAGR_ProjectileLauncherComponent::UAGR_ProjectileLauncherComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.05f;
    PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostUpdateWork;
    UActorComponent::SetAutoActivate(true);
    SetIsReplicatedByDefault(true);

    // Debug
    bDebug = true;
    DebugDuration = 2.0f;

    // Weapon
    FireMode = EAGR_FireMode::SemiAuto;
    bAddOwnerSpeed = true;
    ProjectileInitialSpeed = 73'000.f; // Resembles 9mm bullet shot from an SMG
    FireInterval = 0.1f;
    FireSpread = 1.2f;
    BurstCooldown = 0.35f;
    WeaponDamage = 1.0f;
    AmmoSequence = TArray<TSubclassOf<AAGR_ProjectileBase>>{};

    // Optimization
    bLimitShotsPerFrame = true;
    bClientAimCorrectionEnabled = false;
    ClientScreenSpaceAimCoords = FVector2D{0.5f, 0.5f};
    ClientSafeSpawnDistance = 140.0f;
    ClientAimTraceDistance = 10'000.0f;
    bAutoUpdateIgnoreList = true;

    // Ammo
    AmmoCount = 20;
    BurstCount = 3;
    AmmoConsumptionPerShot = 1;
    ProjectileSpawnAmount = 1;

    // State
    bInfiniteAmmoEnabled = false;
    bSafetyLockEnabled = false;
    WeaponTags = FGameplayTagContainer{};
    AmmoSequenceIndex = 0;
    BurstCounter = 0;
    bRetriggerAllowed = true;

    // Internal
    ActorsIgnoredByProjectiles = TArray<TObjectPtr<AActor>>{};
    ClientProjectileLauncherLocation = FVector::ZeroVector;
}

void UAGR_ProjectileLauncherComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Weapon
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, FireMode, COND_InitialOrOwner);
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, ProjectileInitialSpeed, COND_InitialOrOwner);
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, FireInterval, COND_InitialOrOwner);
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, FireSpread, COND_InitialOrOwner);
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, BurstCooldown, COND_InitialOrOwner);
    DOREPLIFETIME(UAGR_ProjectileLauncherComponent, WeaponDamage);

    // Ammo
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, AmmoSequence, COND_InitialOrOwner);
    DOREPLIFETIME(UAGR_ProjectileLauncherComponent, AmmoCount);
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, BurstCount, COND_InitialOrOwner);
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, AmmoConsumptionPerShot, COND_InitialOrOwner);
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, ProjectileSpawnAmount, COND_InitialOrOwner);

    // State
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, bSafetyLockEnabled, COND_InitialOrOwner);
    DOREPLIFETIME_CONDITION(UAGR_ProjectileLauncherComponent, WeaponTags, COND_InitialOrOwner);
}

void UAGR_ProjectileLauncherComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ApplyClientAimCorrection();
}

FRotator UAGR_ProjectileLauncherComponent::CalculateFireSpread_Implementation() const
{
    return FRotator{
        FMath::FRandRange(-FireSpread, FireSpread),
        FMath::FRandRange(-FireSpread, FireSpread),
        FMath::FRandRange(-FireSpread, FireSpread)
    };
}

void UAGR_ProjectileLauncherComponent::SetAmmoCount(const int32 InNewAmmoCount)
{
    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    AmmoCount = InNewAmmoCount;
    OnRep_AmmoCount();
}

int32 UAGR_ProjectileLauncherComponent::GetAmmoCount() const
{
    return AmmoCount;
}

void UAGR_ProjectileLauncherComponent::ReloadWeapon_Implementation(
    const int32 InNewAmmoCount,
    const bool bInResetAmmoSequenceIndex)
{
    SetAmmoCount(InNewAmmoCount);

    if(bInResetAmmoSequenceIndex)
    {
        AmmoSequenceIndex = 0;
    }

    OnWeaponReloaded.Broadcast();
}

void UAGR_ProjectileLauncherComponent::ConsumeAmmo()
{
    if(IsInfiniteAmmoEnabled())
    {
        return;
    }

    SetAmmoCount(GetAmmoCount() - AmmoConsumptionPerShot);
}

void UAGR_ProjectileLauncherComponent::SetAmmoSequence(const TArray<TSubclassOf<AAGR_ProjectileBase>>& InAmmoSequence)
{
    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    AmmoSequence = InAmmoSequence;
    OnRep_AmmoSequence();

    AmmoSequenceIndex = 0;
}

bool UAGR_ProjectileLauncherComponent::IsSafetyLockEnabled() const
{
    return bSafetyLockEnabled;
}

bool UAGR_ProjectileLauncherComponent::CanFire_Implementation() const
{
    if(IsSafetyLockEnabled() || AmmoSequence.Num() <= 0)
    {
        return false;
    }

    return IsInfiniteAmmoEnabled() || GetAmmoCount() >= AmmoConsumptionPerShot;
}

void UAGR_ProjectileLauncherComponent::ApplyClientAimCorrection()
{
    if(!bClientAimCorrectionEnabled)
    {
        return;
    }

    // This is an optimization. In case the player cannot fire there is no need to run expensive aim calculations.
    if(!CanFire())
    {
        return;
    }

    bool bSuccess;
    FVector AimAtLocation;
    FRotator AimAtRotation;
    FVector AimOrigin;
    CalculateClientAimCorrection(
        bSuccess,
        AimAtLocation,
        AimAtRotation,
        AimOrigin);
    if(!bSuccess)
    {
        return;
    }

    if(GetOwner()->HasAuthority())
    {
        ClientProjectileLauncherLocation = AimOrigin;
        SetWorldRotation(AimAtRotation);
        return;
    }

    SV_UpdateClientAimOriginAndRotation(AimOrigin, AimAtRotation);
}

void UAGR_ProjectileLauncherComponent::CalculateClientAimCorrection(
    bool& bOutSuccess,
    FVector& OutAimAtLocation,
    FRotator& OutAimAtRotation,
    FVector& OutAimOrigin) const
{
    bOutSuccess = false;
    OutAimOrigin = FVector::ZeroVector;
    OutAimAtLocation = FVector::ZeroVector;
    OutAimAtRotation = FRotator::ZeroRotator;

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner))
    {
        return;
    }

    if(UKismetSystemLibrary::IsDedicatedServer(Owner))
    {
        return;
    }

    const APawn* const Instigator = Owner->GetInstigator();
    if(!IsValid(Instigator))
    {
        return;
    }

    const AController* const Controller = Instigator->GetController();
    if(!IsValid(Controller))
    {
        return;
    }

    if(!Controller->IsLocalController())
    {
        return;
    }

    const APlayerController* const PlayerController = Cast<APlayerController>(Controller);
    if(!IsValid(PlayerController))
    {
        return;
    }

    FVector AimAtLocation;
    FRotator AimAtRotation;
    FHitResult HitResult;
    const FVector AimOrigin = GetComponentLocation();
    const TArray<AActor*> IgnoredActors = BuildIgnoreActorList();
    // ReSharper disable once CppTooWideScope
    UAGR_ProjectileFunctionLibrary::CameraAim(
        bOutSuccess,
        AimAtLocation,
        AimAtRotation,
        HitResult,
        AimOrigin,
        PlayerController,
        ClientAimTraceDistance,
        ECC_Visibility,
        true,
        IgnoredActors,
        ClientScreenSpaceAimCoords);

    if(bOutSuccess)
    {
        OutAimAtLocation = AimAtLocation;
        OutAimAtRotation = AimAtRotation;
        OutAimOrigin = AimOrigin;
    }
}

void UAGR_ProjectileLauncherComponent::SetWeaponTags(const FGameplayTagContainer& InWeaponTags)
{
    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    WeaponTags = InWeaponTags;
}

void UAGR_ProjectileLauncherComponent::AddWeaponTag(const FGameplayTag& InWeaponTag)
{
    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    WeaponTags.AddTag(InWeaponTag);
}

void UAGR_ProjectileLauncherComponent::RemoveWeaponTag(const FGameplayTag& InWeaponTag)
{
    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    WeaponTags.RemoveTag(InWeaponTag);
}

void UAGR_ProjectileLauncherComponent::SetFireMode(const EAGR_FireMode InFireMode)
{
    if(FireMode == InFireMode)
    {
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    FireMode = InFireMode;

    OnRep_FireMode();
}

void UAGR_ProjectileLauncherComponent::SetSafetyLockEnabled(const bool bInEnabled)
{
    if(bSafetyLockEnabled == bInEnabled)
    {
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    bSafetyLockEnabled = bInEnabled;
    OnRep_SafetyLockEnabled();
}

void UAGR_ProjectileLauncherComponent::SV_BeginFire_Implementation()
{
    if(!CanFire())
    {
        return;
    }

    if(!bRetriggerAllowed)
    {
        return;
    }

    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    const UWorld* const World = Owner->GetWorld();
    if(!IsValid(World))
    {
        return;
    }

    if(bAutoUpdateIgnoreList)
    {
        ActorsIgnoredByProjectiles.Empty();
        ActorsIgnoredByProjectiles.Append(BuildIgnoreActorList());
    }

    switch(GetFireMode())
    {
    case EAGR_FireMode::SemiAuto:
        {
            World->GetTimerManager().SetTimer(
                FireIntervalTimerHandler,
                FireInterval,
                FTimerManagerTimerParameters{.bLoop = false, .bMaxOncePerFrame = bLimitShotsPerFrame});

            PerformSingleFire();
            break;
        }
    case EAGR_FireMode::FullAuto:
        {
            World->GetTimerManager().SetTimer(
                FireIntervalTimerHandler,
                this,
                &ThisClass::PerformSingleFire,
                FireInterval,
                FTimerManagerTimerParameters{.bLoop = true, .bMaxOncePerFrame = bLimitShotsPerFrame});

            PerformSingleFire();
            break;
        }
    case EAGR_FireMode::Burst:
    case EAGR_FireMode::InterruptibleBurst:
        {
            BurstCounter = 0;

            World->GetTimerManager().SetTimer(
                FireIntervalTimerHandler,
                this,
                &ThisClass::PerformBurstFire,
                FireInterval,
                FTimerManagerTimerParameters{.bLoop = true, .bMaxOncePerFrame = bLimitShotsPerFrame});

            PerformBurstFire();
            break;
        }
    case EAGR_FireMode::Custom:
        {
            PerformCustomBeginFire();
            break;
        }
    default:
        checkNoEntry();
    }
}

void UAGR_ProjectileLauncherComponent::SV_EndFire_Implementation()
{
    const AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    const UWorld* const World = Owner->GetWorld();
    if(!IsValid(World))
    {
        return;
    }

    switch(GetFireMode())
    {
    case EAGR_FireMode::SemiAuto:
    case EAGR_FireMode::FullAuto:
        {
            // If the delay timer handle is valid, this means we already started the process of ending fire.
            if(EndFireTimerHandle.IsValid())
            {
                return;
            }

            const float RemainingTime = World->GetTimerManager().GetTimerRemaining(FireIntervalTimerHandler);
            if(RemainingTime <= 0)
            {
                PerformSingleEndFire();
                break;
            }

            World->GetTimerManager().SetTimer(
                EndFireTimerHandle,
                this,
                &ThisClass::PerformSingleEndFire,
                RemainingTime,
                false);

            UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(Owner, FireIntervalTimerHandler);
            break;
        }
    case EAGR_FireMode::Burst:
        {
            // Ending of this fire mode is handled in PerformBurstFire().
            break;
        }
    case EAGR_FireMode::InterruptibleBurst:
        {
            // If the delay timer handle is valid, this means we already started the process of ending fire.
            if(EndFireTimerHandle.IsValid())
            {
                return;
            }

            const float RemainingTime = World->GetTimerManager().GetTimerRemaining(FireIntervalTimerHandler);
            if(RemainingTime <= 0)
            {
                // Burst finished normally 
                PerformBurstEndFire();
                break;
            }

            // Burst was interrupted.
            // Apply BurstCooldown on top of the remaining (FireRate) time as penalty for interrupting.
            World->GetTimerManager().SetTimer(
                EndFireTimerHandle,
                this,
                &ThisClass::PerformBurstEndFire,
                RemainingTime + BurstCooldown,
                false);

            UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(Owner, FireIntervalTimerHandler);

            OnBurstInterrupted.Broadcast();
            break;
        }
    case EAGR_FireMode::Custom:
        {
            PerformCustomEndFire();
            break;
        }
    default:
        checkNoEntry();
    }
}

void UAGR_ProjectileLauncherComponent::SpawnProjectile_Implementation()
{
    const TSubclassOf<AAGR_ProjectileBase> ProjectileClass = GetProjectileClass();
    if(!IsValid(ProjectileClass))
    {
        return;
    }

    AActor* const Owner = GetOwner();
    if(!IsValid(Owner) || !Owner->HasAuthority())
    {
        return;
    }

    UWorld* const World = Owner->GetWorld();
    if(!IsValid(World))
    {
        return;
    }

    APawn* const Instigator = Owner->GetInstigator();
    if(!IsValid(Instigator))
    {
        return;
    }

    const FTransform WorldTransform = GetComponentToWorld();
    const FVector SpawnLocation = GetSafeProjectileSpawnLocation(GetComponentLocation());
    const FRotator SpawnRotation = UKismetMathLibrary::ComposeRotators(
        WorldTransform.Rotator(),
        CalculateFireSpread());
    const FTransform SpawnTransform(SpawnRotation, SpawnLocation, FVector::OneVector);

    AAGR_ProjectileBase* SpawnedProjectile = World->SpawnActorDeferred<AAGR_ProjectileBase>(
        ProjectileClass,
        SpawnTransform,
        Owner,
        Instigator,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
        ESpawnActorScaleMethod::MultiplyWithRoot);

    if(!IsValid(SpawnedProjectile))
    {
        AGR_LOG(
            LogAGR_Projectile_Runtime,
            Error,
            "Could not spawn projectile of class '%s'",
            *ProjectileClass->GetName());
        return;
    }

    CycleForwardInAmmoSequence();

    SpawnedProjectile->DebugDuration = DebugDuration;
    SpawnedProjectile->bDebugDraw = bDebug;
    SpawnedProjectile->IgnoredActors = ActorsIgnoredByProjectiles;
    SpawnedProjectile->InitialSpeed = CalculateProjectileInitialSpeed();
    SpawnedProjectile->WeaponDamage = WeaponDamage;
    SpawnedProjectile->WeaponTags = WeaponTags;

    UGameplayStatics::FinishSpawningActor(SpawnedProjectile, SpawnTransform);

    OnProjectileSpawned.Broadcast();
}

void UAGR_ProjectileLauncherComponent::SetInfiniteAmmoEnabled(const bool bInEnabled)
{
    bInfiniteAmmoEnabled = bInEnabled;
}

bool UAGR_ProjectileLauncherComponent::IsInfiniteAmmoEnabled() const
{
    return bInfiniteAmmoEnabled;
}

EAGR_FireMode UAGR_ProjectileLauncherComponent::GetFireMode() const
{
    return FireMode;
}

TArray<TSubclassOf<AAGR_ProjectileBase>> UAGR_ProjectileLauncherComponent::GetAmmoSequence() const
{
    return AmmoSequence;
}

void UAGR_ProjectileLauncherComponent::PerformSingleFire_Implementation()
{
    bRetriggerAllowed = false;

    if(!CanFire())
    {
        return;
    }

    ConsumeAmmo();

    for(int32 i = 0; i < ProjectileSpawnAmount; i++)
    {
        SpawnProjectile();
    }

    OnWeaponFired.Broadcast();
}

void UAGR_ProjectileLauncherComponent::PerformSingleEndFire_Implementation()
{
    bRetriggerAllowed = true;
    UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(GetOwner(), EndFireTimerHandle);
}

void UAGR_ProjectileLauncherComponent::PerformBurstFire_Implementation()
{
    bRetriggerAllowed = false;

    const bool bEndBurstFire = !CanFire() || BurstCounter >= BurstCount;
    if(bEndBurstFire)
    {
        const AActor* const Owner = GetOwner();
        if(!IsValid(Owner) || !Owner->HasAuthority())
        {
            return;
        }

        UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(Owner, FireIntervalTimerHandler);

        const UWorld* const World = Owner->GetWorld();
        if(!IsValid(World))
        {
            return;
        }

        FTimerHandle DelayTimerHandle;
        World->GetTimerManager().SetTimer(
            DelayTimerHandle,
            this,
            &ThisClass::PerformBurstEndFire,
            BurstCooldown,
            false);
        return;
    }

    BurstCounter++;

    ConsumeAmmo();

    for(int32 i = 0; i < ProjectileSpawnAmount; i++)
    {
        SpawnProjectile();
    }

    OnWeaponFired.Broadcast();
}

void UAGR_ProjectileLauncherComponent::PerformBurstEndFire_Implementation()
{
    bRetriggerAllowed = true;
    BurstCounter = 0;
    UKismetSystemLibrary::K2_ClearAndInvalidateTimerHandle(GetOwner(), EndFireTimerHandle);
    OnBurstFinished.Broadcast();
}

void UAGR_ProjectileLauncherComponent::CycleForwardInAmmoSequence()
{
    const int32 AmmoSequenceMaxIndex = AmmoSequence.Num();
    if(AmmoSequenceMaxIndex <= 0)
    {
        AmmoSequenceIndex = 0;
        return;
    }

    AmmoSequenceIndex = (AmmoSequenceIndex + 1) % AmmoSequenceMaxIndex;
}

void UAGR_ProjectileLauncherComponent::PerformCustomBeginFire_Implementation()
{
    // Override this function to add your custom begin fire logic.
}

void UAGR_ProjectileLauncherComponent::PerformCustomEndFire_Implementation()
{
    // Override this function to add your custom end fire logic.
}

TSubclassOf<AAGR_ProjectileBase> UAGR_ProjectileLauncherComponent::GetProjectileClass_Implementation()
{
    if(!AmmoSequence.IsValidIndex(AmmoSequenceIndex))
    {
        return nullptr;
    }

    return AmmoSequence[AmmoSequenceIndex];
}

void UAGR_ProjectileLauncherComponent::SV_UpdateClientAimOriginAndRotation_Implementation(
    const FVector& InOrigin,
    const FRotator& InRotation)
{
    ClientProjectileLauncherLocation = InOrigin;
    SetWorldRotation(InRotation);
}

float UAGR_ProjectileLauncherComponent::CalculateProjectileInitialSpeed() const
{
    return bAddOwnerSpeed
           ? ProjectileInitialSpeed + GetComponentVelocity().Length()
           : ProjectileInitialSpeed;
}

TArray<AActor*> UAGR_ProjectileLauncherComponent::BuildIgnoreActorList_Implementation() const
{
    return UAGR_ProjectileFunctionLibrary::BuildActorList(GetOwner());
}

FVector UAGR_ProjectileLauncherComponent::GetSafeProjectileSpawnLocation_Implementation(
    const FVector& InSafeLocation) const
{
    if(!bClientAimCorrectionEnabled)
    {
        return InSafeLocation;
    }

    return FVector::Distance(InSafeLocation, ClientProjectileLauncherLocation) <= ClientSafeSpawnDistance
           ? ClientProjectileLauncherLocation
           : InSafeLocation;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_ProjectileLauncherComponent::OnRep_FireMode()
{
    OnFireModeUpdated.Broadcast(GetFireMode());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_ProjectileLauncherComponent::OnRep_AmmoCount()
{
    OnAmmoCountUpdated.Broadcast(GetAmmoCount());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_ProjectileLauncherComponent::OnRep_SafetyLockEnabled()
{
    OnSafetyLockUpdated.Broadcast(IsSafetyLockEnabled());

    SetComponentTickEnabled(!IsSafetyLockEnabled());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void UAGR_ProjectileLauncherComponent::OnRep_AmmoSequence()
{
    OnAmmoSequenceUpdated.Broadcast();
}