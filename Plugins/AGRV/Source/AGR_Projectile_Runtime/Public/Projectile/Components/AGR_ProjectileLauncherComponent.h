// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Types/AGR_ProjectileTypes.h"

#include "AGR_ProjectileLauncherComponent.generated.h"

class AAGR_ProjectileBase;

/**
 * AGR Projectile Launcher component should be attached to an actor to become a type of weapon.
 * Functions in this component fully manage firing including different fire modes and provide a wide range of
 * configurations for project-specific use-cases.
 */
UCLASS(
    ClassGroup="3Studio",
    Blueprintable,
    BlueprintType,
    meta=(BlueprintSpawnableComponent),
    PrioritizeCategories="3Studio AGR")
class AGR_PROJECTILE_RUNTIME_API UAGR_ProjectileLauncherComponent : public USceneComponent
{
    GENERATED_BODY()

private:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAGR_WeaponFired_Delegate);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAGR_ProjectileSpawned_Delegate);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAGR_BurstFinished_Delegate);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAGR_BurstInterrupted_Delegate);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAGR_WeaponReloaded_Delegate);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAGR_AmmoSequenceUpdated_Delegate);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
        FAGR_AmmoCountUpdated_Delegate,
        const int32,
        AmmoCount);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
        FAGR_SafetyLockUpdated_Delegate,
        const bool,
        bSafetyLockEnabled);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
        FAGR_FireModeUpdated_Delegate,
        const EAGR_FireMode,
        FireMode);

public:
    /** Broadcast when the weapon has fired. */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_WeaponFired_Delegate OnWeaponFired;

    /** Broadcast when the weapon has spawned a projectile. */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_ProjectileSpawned_Delegate OnProjectileSpawned;

    /** Broadcast when firing in burst fire mode has finished. */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_BurstFinished_Delegate OnBurstFinished;

    /** Broadcast when firing in burst fire mode was interrupted. */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_BurstInterrupted_Delegate OnBurstInterrupted;

    /** Broadcast when the weapon has reloaded. */
    UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly)
    FAGR_WeaponReloaded_Delegate OnWeaponReloaded;

    /** Broadcast when the weapon's ammo sequence was updated. */
    UPROPERTY(BlueprintAssignable)
    FAGR_AmmoSequenceUpdated_Delegate OnAmmoSequenceUpdated;

    /** Broadcast when the ammo count was updated. */
    UPROPERTY(BlueprintAssignable)
    FAGR_AmmoCountUpdated_Delegate OnAmmoCountUpdated;

    /** Broadcast when the state of the weapon safety lock was updated. */
    UPROPERTY(BlueprintAssignable)
    FAGR_SafetyLockUpdated_Delegate OnSafetyLockUpdated;

    /** Broadcast when the weapon fire mode was updated. */
    UPROPERTY(BlueprintAssignable)
    FAGR_FireModeUpdated_Delegate OnFireModeUpdated;

    /**
     * The projectile's initial speed when spawned.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Weapon",
        Replicated,
        SaveGame,
        meta=(ClampMin=0.0f, Units="CentimetersPerSecond"))
    float ProjectileInitialSpeed;

    /**
     * If true, the Projectile Launcher component's velocity will be added to the initial speed of the projectile.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Weapon", SaveGame)
    bool bAddOwnerSpeed;

    /**
     * The interval at which the weapon can fire.
     *
     * Example:
     * FireInterval = 1, this means that you are allowed to shoot 1 projectile every second.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Weapon",
        Replicated,
        SaveGame,
        meta=(ClampMin=0.0f, Units="Seconds"))
    float FireInterval;

    /**
     * This value is used to calculate a randomized angle for a new projectile.
     *
     * See CalculateFireSpread() for more details.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Weapon",
        Replicated,
        SaveGame,
        meta=(ClampMin=0.0f, ClampMax=90.0f, UIMin=0.0f, UiMax=45.0f, Units="Degrees"))
    float FireSpread;

    /**
     * The maximum amount of ammo that will be shot when calling BeginFire().
     * 
     * Applies to the following fire modes only:
     * - Burst
     * - InterruptibleBurst
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Weapon", Replicated, SaveGame, meta=(ClampMin=0))
    int32 BurstCount;

    /**
     * The delay before another burst fire can be triggered.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Weapon",
        Replicated,
        SaveGame,
        meta=(ClampMin=0.0f, Units="Seconds"))
    float BurstCooldown;

    /**
     * Set this variable to true in order to avoid unwanted collisions between multiple projectiles spawned at the same
     * frame.
     *
     * Consider this example:
     * If the fire rate is faster than the frame rate, it is possible that more than one projectile will be spawned
     * at the same frame.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Optimization", SaveGame)
    bool bLimitShotsPerFrame;

    /**
     * The maximum allowed distance between the Projectile Launcher component's location on the server and client.
     * If the distance exceeds this value, the client's location will be discarded.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Optimization",
        SaveGame,
        meta=(ClampMin=0.0f, Units="cm"))
    float ClientSafeSpawnDistance;

    /**
     * If true, a client will be allowed to send its aim origin and rotation to the server.
     * The update will occur on tick of the component.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Optimization", SaveGame)
    bool bClientAimCorrectionEnabled;

    /**
     * The screen-space coordinates (0.0 - 1.0) to use when calculating the client's camera aim.
     * This defines where the crosshair is.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Optimization", SaveGame)
    FVector2D ClientScreenSpaceAimCoords;

    /**
     * The distance to trace from ClientScreenSpaceAimCoords into the world.
     * It is used for calculating client aim correction (see: CalculateClientAimCorrection).
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Optimization",
        SaveGame,
        meta=(ClampMin=0.0f, Units="cm"))
    float ClientAimTraceDistance;

    /**
     * The amount of ammo to consume per shot.
     */
    UPROPERTY(
        BlueprintReadWrite,
        EditAnywhere,
        Category="3Studio AGR|Ammo",
        Replicated,
        SaveGame,
        meta=(ClampMin=0, UIMin=1))
    int32 AmmoConsumptionPerShot;

    /**
     * The amount of projectiles to spawn per shot.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Ammo", Replicated, SaveGame, meta=(ClampMin=1))
    int32 ProjectileSpawnAmount;

    /**
     * If set to true, ActorsIgnoredByProjectiles will be updated every time BeginFire() is called.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|Optimization")
    bool bAutoUpdateIgnoreList;

    /**
     * Weapon damage value passed to spawned projectiles.
     *
     * Example:
     * It could be used to calculate the actual damage dealt by this projectile without having to know its parent
     * actor.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="3Studio AGR|User Data", Replicated, SaveGame)
    float WeaponDamage;

    /**
     * GameplayTagContainer passed to spawned projectiles.
     *
     * Examples:
     * - Tags could describe upgrades that were applied by the weapon at the time of firing.
     * - Apply a GameplayEffect when a target is hit by this projectile.
     */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="3Studio AGR|User Data", Replicated, SaveGame)
    FGameplayTagContainer WeaponTags;

protected:
    /**
     * This timer handler is used to control the fire interval by keeping track of when to allow to fire again (retrigger).
     */
    UPROPERTY(BlueprintReadOnly, Category="3Studio AGR|State", Transient)
    FTimerHandle FireIntervalTimerHandler;

    /**
     * If the safety lock is enabled it will prevent firing.
     */
    UPROPERTY(EditAnywhere, Category="3Studio AGR|State", ReplicatedUsing="OnRep_SafetyLockEnabled", SaveGame)
    bool bSafetyLockEnabled;

    /**
     * The amount of available ammo for shooting.
     */
    UPROPERTY(
        EditAnywhere,
        Category="3Studio AGR|Ammo",
        ReplicatedUsing="OnRep_AmmoCount",
        SaveGame)
    int32 AmmoCount;

    /**
     * If set to true, no ammo will be consumed when firing. It also allows to fire without having ammo.
     */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="3Studio AGR|State", SaveGame)
    bool bInfiniteAmmoEnabled;

    /**
     * This is the current index of the ammo sequence which points to a projectile class that may be spawned next.
     */
    UPROPERTY(SaveGame)
    int32 AmmoSequenceIndex;

    /**
     * If true, the weapon is allowed to fire (i.e. trigger) again.
     * Otherwise, it prevents multiple shoot events from overlapping.
     */
    UPROPERTY(BlueprintReadOnly, Category="3Studio AGR|State", Transient)
    bool bRetriggerAllowed;

    /**
     * Counts the number of already fired shots during a burst fire.
     */
    UPROPERTY(BlueprintReadOnly, Category="3Studio AGR|State", Transient)
    int32 BurstCounter;

    /**
     * Debug flag passed to spawned projectiles.
     * 
     * Set to true to enable projectile debug visualization. This will show the projectile's trajectory and events
     * like ricochets, penetrations, and terminal hits.
     */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="3Studio AGR|Debug")
    bool bDebug;

    /**
     * Debug duration value passed to spawned projectiles.
     * 
     * The duration for displaying the projectile debug visualization.
     */
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="3Studio AGR|Debug", meta=(Units="Seconds"))
    float DebugDuration;

private:
    /**
     * The weapon's fire mode that defines the firing behavior.
     */
    UPROPERTY(EditAnywhere, Category="3Studio AGR|Weapon", ReplicatedUsing="OnRep_FireMode", SaveGame)
    EAGR_FireMode FireMode;

    /**
     * Defines a sequence of AGR Projectile classes that will be cycled through (with wrapping) when firing a shot.
     * 
     * Note that this is not intended to work like a magazine for a gun but to implement special behavior.
     * 
     * Examples:
     * - Fire normal projectiles only .......................... :  1x NormalProjectile
     * - Fire explosive projectiles only ....................... :  1x ExplosiveProjectile
     * - Every 5th shot spawns an explosive projectile ......... :  4x NormalProjectile    +  1x ExplosiveProjectile
     * - The first shot of a reloaded weapon deals double damage :  1x DoubleDmgProjectile + 29x NormalProjectile
     * - The last  shot of a reloaded weapon deals double damage : 29x NormalProjectile    +  1x DoubleDmgProjectile
     * - etc.
     * 
     * If you need to customize how Projectile classes are selected when firing, override GetProjectileClass().
     */
    UPROPERTY(EditAnywhere, Category="3Studio AGR|Ammo", ReplicatedUsing="OnRep_AmmoSequence", SaveGame)
    TArray<TSubclassOf<AAGR_ProjectileBase>> AmmoSequence;

    /**
     * Actors that will be ignored by spawned projectiles.
     *
     * If bAutoUpdateIgnoreList is set to true ignored actors will be automatically updated on BeginFire().
     * Remember to call UpdateIgnoreList() manually if bAutoUpdateIgnoreList is set to false.
     */
    UPROPERTY(Transient)
    TArray<TObjectPtr<AActor>> ActorsIgnoredByProjectiles;

    /**
     * Stores the client-reported Projectile Launcher component location.
     * It will only be updated if bClientAimCorrectionEnabled is set to true.
     *
     * This location may be used to spawn projectiles depending on ClientSafeSpawnDistance.
     */
    UPROPERTY(Transient)
    FVector ClientProjectileLauncherLocation;

    /**
     * Internal timer handler used for tracking when to end firing the weapon.
     */
    UPROPERTY(Transient)
    FTimerHandle EndFireTimerHandle;

public:
    UAGR_ProjectileLauncherComponent(const FObjectInitializer& ObjectInitializer);

    //~ Begin USceneComponent Interface
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    //~ End USceneComponent Interface

    //~ Begin UActorComponent Interface
    virtual void TickComponent(
        const float DeltaTime,
        const ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Weapon")
    EAGR_FireMode GetFireMode() const;

    /**
     * Sets the fire mode of the projectile launcher.
     * @param InFireMode The new fire mode.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Weapon")
    void SetFireMode(UPARAM(DisplayName="Fire Mode") const EAGR_FireMode InFireMode);

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Weapon")
    bool IsSafetyLockEnabled() const;

    /**
     * Sets the state of the safety lock of the projectile launcher.
     * @param bInEnabled If true, the projectile launcher can fire. Otherwise, firing is prevented.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Weapon")
    void SetSafetyLockEnabled(UPARAM(DisplayName="Enabled") const bool bInEnabled);

    /**
     * Checks if the weapon can fire in two steps:
     * 1. Safety Lock needs to be disabled and a non-empty ammo sequence must be defined.
     *    (See: bSafetyLockEnabled and AmmoSequence)
     * 2. Is there enough available ammo to consume?
     *    (See: AmmoCount and AmmoConsumptionPerShot)
     *
     * Exception for step 2: If bInfiniteAmmoEnabled is set to true, the check for enough ammo will be skipped.
     * 
     * @returns True if the weapon can fire. Otherwise, false.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Weapon")
    bool CanFire() const;

    /**
     * When called on an owning client, this will invoke an RPC on the server.
     * 
     * The server will run checks whether the weapon can be currently fired and if so it will perform firing
     * the weapon according to the selected fire mode.
     */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category="3Studio AGR|Weapon", meta=(Keywords="trigger,shoot"))
    void SV_BeginFire();

    /**
     * When called on an owning client, this will invoke an RPC on the server.
     * 
     * The server will end firing the weapon according to the selected fire mode.
     *
     * Calls the OnBurstInterrupted event dispatcher when a burst fire action was interrupted.
     * Calls the OnBurstFinished event dispatcher when a burst fire action is finished.
     */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category="3Studio AGR|Weapon", meta=(Keywords="trigger,shoot"))
    void SV_EndFire();

    /**
     * Sets a new ammo count.
     *
     * @param InNewAmmoCount New amount of ammo.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Ammo")
    void SetAmmoCount(UPARAM(DisplayName="New Ammo Count") const int32 InNewAmmoCount);

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Ammo")
    int32 GetAmmoCount() const;

    /**
     * Reloads the weapon by setting a new ammo count.
     *
     * Calls the OnWeaponReloaded event dispatcher.
     * @param InNewAmmoCount New amount of ammo.
     * @param bInResetAmmoSequenceIndex If true, AmmoSequenceIndex will be reset.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly, Category="3Studio AGR|Weapon")
    void ReloadWeapon(
        UPARAM(DisplayName="New Ammo Count") const int32 InNewAmmoCount,
        UPARAM(DisplayName="Reset Ammo Sequence Index") const bool bInResetAmmoSequenceIndex);

    /**
     * Reduces AmmoCount by AmmoConsumptionPerShot but only if bInfiniteAmmoEnabled is set to false.
     *
     * Note that this function does not prevent the AmmoCount to become a negative value.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|Ammo")
    void ConsumeAmmo();

    /**
     * Spawns a new projectile actor using the class that is returned from GetProjectileClass().
     * 
     * The projectile's spawn location and rotation will be determined by GetSafeProjectileSpawnLocation() and
     * CalculateFireSpread() respectively.
     *
     * Calls the OnProjectileSpawned event dispatcher after successfully spawning the projectile.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintAuthorityOnly, Category="3Studio AGR|Weapon")
    void SpawnProjectile();

    /**
     * If infinite ammo is enabled it will make the Projectile Launcher completely skip ammo consumption logic.
     * See: CanFire(), ConsumeAmmo()
     * 
     * @param bInEnabled If true, ammo will be infinite.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Ammo")
    void SetInfiniteAmmoEnabled(UPARAM(DisplayName="Enabled") const bool bInEnabled);

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Ammo")
    bool IsInfiniteAmmoEnabled() const;

    /**
     * Calculates the spread that will be used when spawning projectiles. The default implementation uses the
     * 'FireSpread' variable with randomness to achieve a random spread on each axis.
     * 
     * Override this function for custom calculation logic.
     *
     * @returns The spread rotator.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Aiming")
    FRotator CalculateFireSpread() const;

    /**
     * Sets a new ammo sequence and resets the AmmoSequenceIndex.
     * 
     * Calls the OnAmmoSequenceUpdated event dispatcher when a new ammo sequence was set.
     * @InAmmoSequence The new ammo sequence.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|Ammo")
    void SetAmmoSequence(
        UPARAM(DisplayName="Ammo Sequence") const TArray<TSubclassOf<AAGR_ProjectileBase>>& InAmmoSequence);

    /**
     * This function calculates the client's camera aim origin and rotation and sends it to the server.
     * 
     * The aim correction is only applied under the following conditions:
     * - bClientAimCorrectionEnabled is set to true.
     * - The function is executed on a locally controlled client.
     * - The weapon can fire.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|Aiming")
    void ApplyClientAimCorrection();

    /**
     * Calculates the client aim correction for the owning local player controller.
     * 
     * @param bOutSuccess True if the calculation was successful. Otherwise, false.
     * @param OutAimAtLocation The location the camera is aiming at.
     * @param OutAimAtRotation The rotation needed to look at the impact point of a detected object the camera is aiming at.
     * @param OutAimOrigin The location the aim started from.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="3Studio AGR|Aiming")
    void CalculateClientAimCorrection(
        UPARAM(DisplayName="Success") bool& bOutSuccess,
        UPARAM(DisplayName="Aim at Location") FVector& OutAimAtLocation,
        UPARAM(DisplayName="Aim at Rotation") FRotator& OutAimAtRotation,
        UPARAM(DisplayName="Aim Origin") FVector& OutAimOrigin) const;

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|Ammo")
    TArray<TSubclassOf<AAGR_ProjectileBase>> GetAmmoSequence() const;

    /**
     * Sets WeaponTags array.
     * @param InWeaponTags Input array to set.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|User Data")
    void SetWeaponTags(UPARAM(DisplayName="Weapon Tags") const FGameplayTagContainer& InWeaponTags);

    /**
     * Adds weapon tag to the WeaponTags array.
     * @param InWeaponTag Tag to add to the array.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|User Data")
    void AddWeaponTag(UPARAM(DisplayName="Weapon Tag") const FGameplayTag& InWeaponTag);

    /**
     * Removes weapon tag from the WeaponTags array.
     * @param InWeaponTag Tag to remove from the array.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|User Data")
    void RemoveWeaponTag(UPARAM(DisplayName="Weapon Tag") const FGameplayTag& InWeaponTag);

protected:
    /**
     * Initiates a single fire action.
     *
     * It ensures that the weapon is in a state to fire via CanFire().
     *
     * If the weapon can fire, it will execute the following actions:
     * - Consumes the specified amount of ammo (AmmoConsumptionPerShot)
     * - Spawns one or more projectiles (ProjectileSpawnAmount)
     *
     * This function may be called multiple times in a row to simulate full-auto firing.
     *
     * Calls the OnWeaponFired event dispatcher after spawning all projectiles.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
    void PerformSingleFire();

    /**
     * Ends a single fire action so that BeginFire() can trigger another fire action again.  
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
    void PerformSingleEndFire();

    /**
     * Initiates a burst fire action.
     *
     * This function will be called multiple times in a row to perform shots during a burst fire action.
     * 
     * It ensures that:
     * - The weapon is in a state to fire via CanFire()
     * - The burst fire action is ended whenever it would exceed the number of shots (BurstCount).
     *
     * If the weapon can fire, it will execute the following actions:
     * - Increments the BurstCounter
     * - Consumes the specified amount of ammo (AmmoConsumptionPerShot)
     * - Spawns one or more projectiles (ProjectileSpawnAmount)
     * - Calls the OnWeaponFired event dispatcher after spawning all projectiles.
     *
     * If the weapon cannot fire or all burst shots were fired, a burst cooldown timer will be activated that
     * handles ending the burst fire action.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
    void PerformBurstFire();

    /**
     * Ends a burst fire action so that BeginFire() can trigger another fire action again.
     * Resets the BurstCounter.
     * 
     * Calls the OnBurstFinished event dispatcher when burst fire finished or was interrupted.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
    void PerformBurstEndFire();

    /**
     * Initiates a custom fire action.
     *
     * Remember to call the OnWeaponFired event dispatcher after spawning all projectiles.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
    void PerformCustomBeginFire();

    /**
     * Ends a custom fire action.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
    void PerformCustomEndFire();

    /**
     * Measures the distance between the InSafeLocation and the last known ClientProjectileLauncherLocation to determine
     * whether it is safe to use the client's location or not. If the distance is too high, InSafeLocation will be
     * returned instead.
     *
     * If bClientAimCorrectionEnabled is set to false this function will always return InSafeLocation because the
     * client's location would be outdated.
     * @param InSafeLocation The safe location to use for determining the distance.
     */
    UFUNCTION(BlueprintNativeEvent)
    FVector GetSafeProjectileSpawnLocation(
        UPARAM(DisplayName="Safe Location") const FVector& InSafeLocation) const;

    /**
     * This function will be called when a new projectile needs to be spawned.
     * The default implementation makes use of the Ammo Sequence.
     *
     * Override this function if you need to customize the logic of what projectile class should be used to spawn next.
     * @return The projectile class to spawn
     */
    UFUNCTION(BlueprintNativeEvent)
    TSubclassOf<AAGR_ProjectileBase> GetProjectileClass();

    /**
     * Advances the AmmoSequenceIndex forward by one. When the end of the sequence is reached the index will wrap
     * around to the start again.
     */
    void CycleForwardInAmmoSequence();

    /**
     * This function will be called automatically when FireMode is updated via network-replication.
     * 
     * Calls the OnFireModeUpdated event dispatcher.
     */
    UFUNCTION()
    void OnRep_FireMode();

    /**
     * This function will be called automatically when AmmoCount is updated via network-replication.
     * 
     * Calls the OnAmmoCountUpdated event dispatcher.
     */
    UFUNCTION()
    void OnRep_AmmoCount();

    /**
     * This function will be called automatically when bSafety is updated via network-replication.
     * 
     * Calls the OnSafetyLockUpdated event dispatcher.
     */
    UFUNCTION()
    void OnRep_SafetyLockEnabled();

    /**
     * This function will be called automatically when AmmoSequence is updated via network-replication.
     * 
     * Calls the OnAmmoSequenceUpdated event dispatcher.
     */
    UFUNCTION()
    void OnRep_AmmoSequence();

    /**
     * Calculates the projectile's initial speed and optionally adds the owner's speed to it if bAddOwnerSpeed is true.
     */
    float CalculateProjectileInitialSpeed() const;

    /**
     * Builds an ignore list of all actors attached to the given owner and its instigator.
     * The owner and instigator will be included in this ignore list.
     *
     * @returns List of actors to ignore.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="3Studio AGR|Helper")
    TArray<AActor*> BuildIgnoreActorList() const;

private:
    /**
     * Sends the client's aim origin and rotation to the server.
     */
    UFUNCTION(Server, Unreliable)
    void SV_UpdateClientAimOriginAndRotation(const FVector& InOrigin, const FRotator& InRotation);
};