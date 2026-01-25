// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Types/AGR_ProjectileTypes.h"

#include "AGR_ProjectileBase.generated.h"

class UProjectileMovementComponent;
class UAGR_Projectile_ProjectSettings;
class UAGR_ProjectileMovementComponent;
class USphereComponent;

/**
 * AGR Projectile Base is the low-level class for implementing AGR Projectiles and using them with the AGR Projectile
 * Launcher Component. It also extends the movement logic by using the AGR Projectile Movement Component.
 *
 * Usually, what you are looking for is the AGR Projectile class that will cover most cases unless your implementation
 * requires to customize the root component of this actor. So if you choose to use this class be mindful about the
 * required setup explained below.
 * 
 * >>> MANDATORY SETUP <<<
 * 
 * Ensure that you swap out the 'Component Class' in the 'ProjectileRoot' property with a subclass of UPrimitiveComponent.
 *
 * Swap out component in C++:
 * - Use the ObjectInitializer of the constructor.
 * - See AGR Projectile for an example.
 *
 * Swap out component in Blueprints:
 * - Open your child blueprint of AGR Projectile Base and select the component called ProjectileRoot.
 * - In the details panel, change the Component Class. Note that only C++ subclasses are supported.
 */
UCLASS(Abstract, PrioritizeCategories="3Studio AGR")
class AGR_PROJECTILE_RUNTIME_API AAGR_ProjectileBase : public AActor
{
    GENERATED_BODY()

public:
    /**
     * The root component of the projectile actor.
     *
     * Why is this property USceneComponent and not UPrimitiveComponent?
     * - UPrimitiveComponent is an abstract class while USceneComponent is the closest super non-abstract class.
     * - In order to make swapping out this component work properly, this compromise was chosen.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="3Studio AGR|Components")
    TObjectPtr<USceneComponent> ProjectileRoot;

    /**
     * Projectile movement component.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="3Studio AGR|Components")
    TObjectPtr<UAGR_ProjectileMovementComponent> ProjectileMovementComponent;

    /**
     * Radius of the projectile.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="3Studio AGR|Projectile", meta=(Units="cm"))
    float ProjectileRadius;

    /**
     * Collision channel to use for projectile penetration logic.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Projectile")
    TEnumAsByte<ETraceTypeQuery> PenetrationTraceChannel;

    /**
     * Initial speed of the projectile.
     */
    UPROPERTY(BlueprintReadOnly, Category="3Studio AGR|Projectile", meta=(ExposeOnSpawn, Units="CentimetersPerSecond"))
    float InitialSpeed;

    /**
     * Set to true to enable projectile debug visualization. This will show the projectile's trajectory and events
     * like ricochets, penetrations, and terminal hits.
     */
    UPROPERTY(BlueprintReadOnly, Category="3Studio AGR|Debug", meta=(ExposeOnSpawn))
    bool bDebugDraw;

    /**
     * The duration for displaying the projectile debug visualization.
     */
    UPROPERTY(BlueprintReadOnly, Category="3Studio AGR|Debug", meta=(ExposeOnSpawn, Units="Seconds"))
    float DebugDuration;

    /**
     * Array of actors to be ignored. This list will be automatically applied after the projectile and its
     * components have been initialized. Changing this variable later will have no effect.
     */
    UPROPERTY(BlueprintReadOnly, Category="3Studio AGR|Projectile", meta=(ExposeOnSpawn))
    TArray<TObjectPtr<AActor>> IgnoredActors;

    /**
     * GameplayTagContainer passed by the Projectile Launcher component.
     *
     * Examples:
     * - Tags could describe upgrades that were applied by the weapon at the time of firing.
     * - Apply a GameplayEffect when a target is hit by this projectile.
     */
    UPROPERTY(BlueprintReadWrite, Category="3Studio AGR|Projectile", meta=(ExposeOnSpawn))
    FGameplayTagContainer WeaponTags;

    /**
     * Weapon damage value passed by the Projectile Launcher component.
     *
     * Example:
     * It could be used to calculate the actual damage dealt by this projectile without having to know its parent
     * actor.
     */
    UPROPERTY(BlueprintReadOnly, Category="3Studio AGR|Projectile", meta=(ExposeOnSpawn))
    float WeaponDamage;

    /**
     * How many bounces can the projectile make before being stopped.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditDefaultsOnly,
        Category="3Studio AGR|Projectile",
        meta=(UIMin="0", UIMax="3", ClampMin="0"))
    int32 BounceLimit;

    /**
     * Most-influencing factor for projectile ricochet chance.
     * Other factors affecting the ricochet chance are the properties of the physical material.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditDefaultsOnly,
        Category="3Studio AGR|Projectile",
        meta=(UIMin="0", UIMax="100", ClampMin="0"))
    float RicochetFactor;

    /**
     * How many centimeters of material of shear strength (Default: 6) it can penetrate at a velocity of 10000.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditDefaultsOnly,
        Category="3Studio AGR|Projectile",
        meta=(UIMin="0", UIMax="100", ClampMin="0"))
    float PenetrationPower;

    /**
     * Set to true if proper rotation of the bullet is important on multiplayer.
     */
    UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="3Studio AGR|Optimization")
    bool bServerCorrection;

private:
    /**
     * Stores the projectile location of the previous frame.
     */
    UPROPERTY(Transient)
    FVector DebugLastFrameLocation;

    /**
     * Stores the number of bounces that the projectile had already made.
     */
    UPROPERTY(Transient)
    int32 Bounces;

    /**
     * Stores the projectile spawn location.
     */
    UPROPERTY(Transient)
    FVector SpawnLocation;

    /**
     * Cached velocity of the last impact event.
     */
    UPROPERTY(Transient)
    FVector LastImpactVelocity;

    /**
     * Store the reference of the projectile project settings
     */
    UPROPERTY(Transient)
    TObjectPtr<UAGR_Projectile_ProjectSettings> ProjectileProjectSettings;

public:
    AAGR_ProjectileBase(const FObjectInitializer& ObjectInitializer);

    //~ Begin AActor Interface
    virtual void Tick(const float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;
    //~ End AActor Interface

    /**
     * Called when the projectile has come to a stop (velocity is below simulation threshold, bounces are disabled,
     * or it is forcibly stopped).
     * @param InHitResult The last hit result of the projectile.
     */
    UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Transform")
    void OnProjectileTerminalHit(
        UPARAM(DisplayName="Hit Result") const FHitResult& InHitResult);

    /**
     * Called when the projectile has ricocheted (bounced).
     * @param InVelocity The velocity of the projectile.
     * @param InHitResult The hit result of the projectile.
     * @param InBounces The number of bounces that the projectile has already made.
     * @param InRicochetResult The ricochet result data the ricochet was based on.
     * @param InHitTransform The calculated hit transform of the projectile. See: CalculateHitTransform().
     * @param InBounceTransform The calculated bounce transform of the projectile. See: CalculateBounceTransform().
     */
    UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Transform")
    void OnProjectileRicocheted(
        UPARAM(DisplayName="Velocity") const FVector& InVelocity,
        UPARAM(DisplayName="Hit Result") const FHitResult& InHitResult,
        UPARAM(DisplayName="Bounces") const int32 InBounces,
        UPARAM(DisplayName="Ricochet Result") const FAGR_RicochetResult& InRicochetResult,
        UPARAM(DisplayName="Hit Transform") const FTransform& InHitTransform,
        UPARAM(DisplayName="Bounce Transform") const FTransform& InBounceTransform);

    /**
     * Called when the projectile has penetrated an object.
     * @param InVelocity The velocity of the projectile.
     * @param InEntryHitResult The hit result of the projectile on entry.
     * @param InPenetrationResult The penetration result data the penetration was based on.
     */
    UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Transform")
    void OnProjectilePenetrated(
        UPARAM(DisplayName="Velocity") const FVector& InVelocity,
        UPARAM(DisplayName="Entry Hit Result") const FHitResult& InEntryHitResult,
        UPARAM(DisplayName="Penetration Result") const FAGR_PenetrationResult& InPenetrationResult);

protected:
    //~ Begin AActor Interface
    virtual void BeginPlay() override;
    //~ End AActor Interface

    /**
     * Checks if the projectile can penetrate a specific object by calculating the penetration depth.
     *
     * Properties that influence the penetration depth calculation:
     * - Projectile .............: Penetration Power
     * - Projectile .............: Velocity (on impact)
     * - Object Physical Material: Density
     * - Object Physical Material: Shear Strength
     *
     * For objects to be penetrable, the object needs to have visible geometry on both sides where a penetration can
     * start (enter) and end (exit). Objects without thickness, e.g. a simple plane model, should use a two-sided
     * Material (Shader).
     *
     * @param OutPenetrationResult The penetration result data that was calculated.
     * @param InImpactHitResult The hit result where the projectile had hit before.
     * @param InVelocity The velocity of the projectile on hit.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Projectile")
    void CanPenetrate(
        UPARAM(DisplayName="Penetration Result") FAGR_PenetrationResult& OutPenetrationResult,
        UPARAM(DisplayName="Impact Hit Result") const FHitResult& InImpactHitResult,
        UPARAM(DisplayName="Velocity") const FVector& InVelocity);

    /**
     * Checks if the projectile can ricochet (bounce) when impacting on a specific object by calculating its ricochet
     * chance. The chance depends on the impact angle and Physical Material of the hit object.
     * @param OutRicochetResult The ricochet result data that was calculated.
     * @param InHitResult The hit result where the projectile had hit before.
     * @param InVelocity The velocity of the projectile on hit.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, BlueprintNativeEvent, Category="3Studio AGR|Projectile")
    void CanRicochet(
        UPARAM(DisplayName="Ricochet Result") FAGR_RicochetResult& OutRicochetResult,
        UPARAM(DisplayName="Impact Hit Result") const FHitResult& InHitResult,
        UPARAM(DisplayName="Velocity") const FVector& InVelocity) const;

    /**
     * Calculates the transform that contains the impact location and the rotation along the velocity direction.
     * If the velocity is nearly zero, the negated impact normal is used instead to look towards the hit object.
     * @param InHitResult The hit result of the projectile.
     * @param InVelocity The velocity of the projectile.
     * @returns The hit transform of the projectile.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|Transform")
    static FTransform CalculateHitTransform(
        UPARAM(DisplayName="Hit Result") const FHitResult& InHitResult,
        UPARAM(DisplayName="Velocity") const FVector& InVelocity);

    /**
     * Calculates the transformation that includes the impact location and the rotation along the expected bounce
     * velocity direction. To determine the bounce velocity direction, we first simulate the velocity limitation
     * applied by the projectile movement component during a bounce. Then, the velocity vector is negated and
     * mirrored along the impact normal.
     * 
     * @param InHitResult The hit result of the projectile.
     * @param InProjectileMovementComponent The projectile component to use for the bounce calculation.
     * @returns The bounce transform of the projectile.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|Transform")
    static FTransform CalculateBounceTransform(
        UPARAM(DisplayName="Hit Result") const FHitResult& InHitResult,
        UPARAM(DisplayName="Projectile Movement Component")
        const UProjectileMovementComponent* InProjectileMovementComponent);

    /**
     * Calculates the projectile's new velocity and penetration power that should be set after a penetration was
     * performed.
     *
     * This function will be called by PerformPenetration() to apply post-penetration modifications to the projectile.
     *
     * @param OutNewVelocity The new velocity that will be applied to the projectile.
     * @param OutNewPenetrationPower The new penetration power that will be applied to the projectile.
     * @param InImpactHitResult The projectile impact hit result.
     * @param InCurrentVelocity The projectile current velocity.
     * @param InPenetrationRatio The projectile penetration ratio.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Projectile")
    void CalculatePostPenetrationValues(
        UPARAM(DisplayName="New Velocity") FVector& OutNewVelocity,
        UPARAM(DisplayName="New Penetration Power") float& OutNewPenetrationPower,
        UPARAM(DisplayName="Impact Hit Result") const FHitResult& InImpactHitResult,
        UPARAM(DisplayName="Current Velocity") const FVector& InCurrentVelocity,
        UPARAM(DisplayName="Penetration Ratio") const float& InPenetrationRatio);

    /**
     * Calculates the depth that the projectile can penetrate through an object with certain material attributes.
     * @param InPenetrationPower The projectile penetration power.
     * @param InVelocity The projectile velocity.
     * @param InDensity The density of the object the projectile is penetrating.
     * @param InHardness The hardness of the object the projectile is penetrating.
     * @returns The penetration depth.
     */
    static float CalculatePenetrationDepth(
        const float InPenetrationPower,
        const float InVelocity,
        const float InDensity,
        const float InHardness);

    /**
     * Moves the projectile to the exit location when penetrating an object depending on the hit result, impact velocity
     * and the penetration ratio.
     *
     * The projectile's velocity and penetration power will be changed to the new values returned by
     * CalculatePostPenetrationValues().
     * 
     * @param InHitResult The hit result when the projectile hit an object.
     * @param InVelocity The projectile current velocity.
     * @param InPenetrationRatio The penetration ratio that will modify the projectile velocity after penetrating.
     */
    void PerformPenetration(
        const FHitResult& InHitResult,
        const FVector& InVelocity,
        const float InPenetrationRatio);

    /**
     * Uses IgnoredActors to set ignored actors for the projectile root component when moving.
     */
    void TryApplyActorIgnoreList(USceneComponent* SceneComponent);

    /**
     * If 'bServerCorrection' is set to true, the server will update the projectile rotation based on its velocity.
     */
    void ApplyServerProjectileCorrection();

    /***/
    static UAGR_Projectile_ProjectSettings* GetProjectileProjectSettings();

private:
    /**
     * Called when projectile has come to a stop (velocity is below simulation threshold, bounces are disabled, or it is
     * forcibly stopped).
     * @param InHitResult The hit result when the projectile has stopped. 
     */
    UFUNCTION()
    void HandleProjectileStop(const FHitResult& InHitResult);

    /**
     * Called when projectile impacts something and bounces are enabled.
     * @param InHitResult The hit result when the projectile has bounced.
     * @param InImpactVelocity The velocity of the projectile when it has bounced.
     */
    UFUNCTION()
    void HandleProjectileBounce(const FHitResult& InHitResult, const FVector& InImpactVelocity);

    /**
     * Draw a debug sphere on the hit location of the projectile and then draw a line from the hit location to the
     * last known location of the projectile taken from DebugLastFrameLocation.
     * @param InHitResult The hit result on where to draw the debug sphere.
     * @param InColor The color of the debug sphere.
     */
    void DrawDebugProjectileHit(const FHitResult& InHitResult, const FColor& InColor);

    /**
     * Draw a debug line from the last known location of the projectile taken from DebugLastFrameLocation to the
     * specified location.
     *
     * The color will be interpolated between 'StartColor' and 'EndColor' depending on the projectile's speed.
     * @param InSpeed The current speed of the projectile.
     * @param InLineEndLocation The end location of the debug line.
     * @param InStartColor The color used at the start when drawing the debug line.
     * @param InEndColor The color used at the end when drawing the debug line.
     * @param InThickness the debug line thickness.
     */
    void DrawDebugLineForProjectile(
        const float InSpeed,
        const FVector& InLineEndLocation,
        const FColor& InStartColor,
        const FColor& InEndColor,
        const float InThickness);

    /**
     * Draws a debug line of the projectile debug to trace its movement.
     * Only the server has the updated projectile location in order to draw the debug lines correctly.
     */
    void DrawDebugLineForProjectileServerOnly();
};