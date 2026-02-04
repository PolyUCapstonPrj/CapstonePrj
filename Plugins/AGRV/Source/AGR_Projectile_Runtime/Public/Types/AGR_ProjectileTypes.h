// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "Misc/EnumRange.h"

#include "AGR_ProjectileTypes.generated.h"

/**
 * The fire mode of the projectile launcher.
 */
UENUM(BlueprintType)
enum class EAGR_FireMode : uint8
{
    // @formatter:off
    SemiAuto              UMETA(DisplayName="Semi-Auto"),
    FullAuto              UMETA(DisplayName="Full-Auto"),
    Burst                 UMETA(DisplayName="Burst"),
    InterruptibleBurst    UMETA(DisplayName="Interruptible Burst"),
    Custom                UMETA(DisplayName="Custom"),
    // @formatter:on
};

ENUM_RANGE_BY_VALUES(
    EAGR_FireMode,
    EAGR_FireMode::SemiAuto,
    EAGR_FireMode::FullAuto,
    EAGR_FireMode::Burst,
    EAGR_FireMode::InterruptibleBurst,
    EAGR_FireMode::Custom,
);

/**
 * Holds the calculated data after checking for a possible ricochet response for the projectile.
 */
USTRUCT(Blueprintable, BlueprintType)
struct FAGR_RicochetResult
{
    GENERATED_BODY()

    // True, if the projectile should ricochet.
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile")
    bool bShouldRicochet = false;

    // The ricochet chance of the projectile on bounce.
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile")
    float RicochetChance = 0.0f;

    // The angle of incidence (incoming).
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile")
    float RicochetAngle = 0.0f;
};

/**
 * Holds the calculated data after checking for a possible penetration for the projectile.
 */
USTRUCT(Blueprintable, BlueprintType)
struct FAGR_PenetrationResult
{
    GENERATED_BODY()

    // True, if the projectile should penetrate.
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile")
    bool bShouldPenetrate = false;

    // The hit result of the penetrated object.
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile")
    FHitResult PenetrateHitResult = FHitResult{};

    // The calculated penetration depth of the projectile.
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile")
    float PenetrationDepth = 0.0f;

    // The calculated penetration ratio of the projectile based on penetration distance over depth.
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile")
    float PenetrationRatio = 0.0f;

    // The calculated transform of the projectile on entry. See: CalculateHitTransform().
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile")
    FTransform EntryTransform = FTransform::Identity;

    // The calculated transform of the projectile on exit. See: CalculateHitTransform().
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile")
    FTransform ExitTransform = FTransform::Identity;
};