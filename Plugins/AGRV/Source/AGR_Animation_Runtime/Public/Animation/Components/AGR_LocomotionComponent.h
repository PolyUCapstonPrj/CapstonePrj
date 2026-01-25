// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Misc/EnumRange.h"

#include "AGR_LocomotionComponent.generated.h"

/**
 * The rotation method that the "AGR Rotation" node should use.
 */
UENUM(BlueprintType)
enum class EAGR_RotationMethod : uint8
{
    // @formatter:off
    NoRotation              UMETA(DisplayName="No Rotation"),
    AbsoluteRotation        UMETA(DisplayName="Absolute Rotation"),
    RotateToMovement        UMETA(DisplayName="Rotate to Movement"),
    RotateToAim             UMETA(DisplayName="Rotate to Aim"),
    RotateToAimAtAngle      UMETA(DisplayName="Rotate to Aim at Angle"),
    // @formatter:on
};

ENUM_RANGE_BY_VALUES(
    EAGR_RotationMethod,
    EAGR_RotationMethod::NoRotation,
    EAGR_RotationMethod::AbsoluteRotation,
    EAGR_RotationMethod::RotateToMovement,
    EAGR_RotationMethod::RotateToAim,
    EAGR_RotationMethod::RotateToAimAtAngle);

/**
 * The aim method for how the pawn control rotation should be interpreted as aim offset by AGR Anim Instance.
 */
UENUM(BlueprintType)
enum class EAGR_AimMethod : uint8
{
    // @formatter:off
    NoAimOffset             UMETA(DisplayName="No Aim Offset"),
    FreeLook                UMETA(DisplayName="Free Look"),
    FreeLookClamped         UMETA(DisplayName="Free Look (Clamped)"),
    FreeLookClampedLocked   UMETA(DisplayName="Free Look (Clamped & Locked)"),
    ForwardFacing           UMETA(DisplayName="Forward Facing"),
    LookAtMovementDirection UMETA(DisplayName="Look at Movement Direction"),
    // @formatter:on
};

ENUM_RANGE_BY_VALUES(
    EAGR_AimMethod,
    EAGR_AimMethod::NoAimOffset,
    EAGR_AimMethod::FreeLook,
    EAGR_AimMethod::FreeLookClamped,
    EAGR_AimMethod::FreeLookClampedLocked,
    EAGR_AimMethod::ForwardFacing,
    EAGR_AimMethod::LookAtMovementDirection);

/**
 * The AGR Locomotion Component is responsible for multiplayer management of aim offset, rotation methods and animation
 * pose state updates. It works in tandem with the AGR Anim Instance.
 */
UCLASS(
    ClassGroup="3Studio",
    Blueprintable,
    BlueprintType,
    meta=(BlueprintSpawnableComponent),
    PrioritizeCategories="3Studio AGR")
class AGR_ANIMATION_RUNTIME_API UAGR_LocomotionComponent : public UActorComponent
{
    GENERATED_BODY()

private:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_RotationMethodUpdated_Delegate,
        const EAGR_RotationMethod,
        OldRotationMethod,
        const EAGR_RotationMethod,
        NewRotationMethod);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_AimMethodUpdated_Delegate,
        const EAGR_AimMethod,
        OldAimMethod,
        const EAGR_AimMethod,
        NewAimMethod);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_PoseUpdated_Delegate,
        const FGameplayTag&,
        OldPose,
        const FGameplayTag&,
        NewPose);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FAGR_AnimationStatesUpdated_Delegate,
        const FGameplayTagContainer&,
        TagsAdded,
        const FGameplayTagContainer&,
        TagsRemoved);

public:
    /**
     * Broadcast when the RotationMethod of the component changes.
     */
    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|State")
    FAGR_RotationMethodUpdated_Delegate OnRotationMethodUpdated;

    /**
     * Broadcast when the AimMethod of the component changes.
     */
    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|State")
    FAGR_AimMethodUpdated_Delegate OnAimMethodUpdated;

    /**
     * Broadcast when the Pose of the component changes.
     */
    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|State")
    FAGR_PoseUpdated_Delegate OnPoseUpdated;

    /**
     * Broadcast when the AnimationStates of the component changes.
     */
    UPROPERTY(BlueprintAssignable, Category="3Studio AGR|State")
    FAGR_AnimationStatesUpdated_Delegate OnAnimationStatesUpdated;

private:
    /**
     * If false, all changes to animation poses done by the client will only be local and never replicated.
     *
     * NOTE: This value cannot be changed at run-time.
     */
    UPROPERTY(EditDefaultsOnly, Replicated, Category="3Studio AGR|Config")
    bool bUpdateAllowedByClient;

    /**
     * Controls how the actor is rotated.
     */
    UPROPERTY(EditDefaultsOnly, ReplicatedUsing="OnRep_RotationMethod", Category="3Studio AGR|State")
    EAGR_RotationMethod RotationMethod;

    /**
     * Controls how the actor is aiming.
     */
    UPROPERTY(EditDefaultsOnly, ReplicatedUsing="OnRep_AimMethod", Category="3Studio AGR|State")
    EAGR_AimMethod AimMethod;

    /**
     * Current pose represented by a tag.
     */
    UPROPERTY(EditDefaultsOnly, ReplicatedUsing="OnRep_Pose", Category="3Studio AGR|State")
    FGameplayTag Pose;

    /**
     * Gameplay tag container that is used to store animation states.
     */
    UPROPERTY(
        EditAnywhere,
        ReplicatedUsing="OnRep_AnimationStates",
        Category="3Studio AGR|State",
        SaveGame)
    FGameplayTagContainer AnimationStates;

public:
    UAGR_LocomotionComponent();

    //~ Begin UActorComponent Interface
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    //~ End UActorComponent Interface

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|State")
    bool IsUpdateAllowedByClient() const;

    /**
     * Sets the rotation method (Network replicated).
     * @param InRotationMethod The new rotation method.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|State")
    void SetRotationMethod(UPARAM(DisplayName="Rotation Method") const EAGR_RotationMethod InRotationMethod);

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|State", meta=(BlueprintThreadSafe))
    EAGR_RotationMethod GetRotationMethod() const;

    /**
     * Sets the aim method (Network replicated).
     * @param InAimMethod The new aim method.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|State")
    void SetAimMethod(UPARAM(DisplayName="Aim Method") const EAGR_AimMethod InAimMethod);

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|State", meta=(BlueprintThreadSafe))
    EAGR_AimMethod GetAimMethod() const;

    /**
     * Sets the pose (Network replicated).
     * @param InPose The new pose.
     */
    UFUNCTION(BlueprintCallable, Category="3Studio AGR|State", meta=(AutoCreateRefTerm="InPose"))
    void SetPose(UPARAM(DisplayName="Pose") const FGameplayTag& InPose);

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|State", meta=(BlueprintThreadSafe))
    const FGameplayTag& GetPose() const;

    /**
     * Sets AnimationStates (Network replicated).
     * @param InAnimationStates Gameplay tag container to set AnimationStates to.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|State")
    void SetAnimationStates(UPARAM(DisplayName="Animation States") const FGameplayTagContainer& InAnimationStates);

    /**
     * Adds tags to the AnimationStates gameplay tag container.
     * @param InAnimationStatesToAdd Gameplay tag container with tags to add to AnimationStates.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|State")
    void AddAnimationStates(
        UPARAM(DisplayName="Animation States to Add") const FGameplayTagContainer& InAnimationStatesToAdd);

    /**
     * Removes tags from the AnimationStates gameplay tag container.
     * @param InAnimationStatesToRemove Gameplay tag container with tags to remove from AnimationStates.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="3Studio AGR|State")
    void RemoveAnimationStates(
        UPARAM(DisplayName="Animation States to Remove") const FGameplayTagContainer& InAnimationStatesToRemove);

    /***/
    UFUNCTION(BlueprintPure, Category="3Studio AGR|State", meta=(BlueprintThreadSafe))
    const FGameplayTagContainer& GetAnimationStates() const;

protected:
    /**
     * Sets the replication behavior for the client's changes to animation poses. 
     * @note Should only be called during construction.
     * @param bInUpdateAllowedByClient if true, changes to animation poses will be replicated, false otherwise.
     */
    void SetUpdateAllowedByClient(const bool bInUpdateAllowedByClient);

private:
    /**
     * Directly sets the RotationMethod value and broadcasts the change. 
     * @param InRotationMethod The value to set RotationMethod to.
     * @note This function does not check if current method is the same.
     */
    void InternalSetRotationMethod(const EAGR_RotationMethod InRotationMethod);

    /**
     * Replication handler for RotationMethod.
     * @param InOldRotationMethod Previous RotationMethod value.
     */
    UFUNCTION()
    void OnRep_RotationMethod(const EAGR_RotationMethod InOldRotationMethod);

    /**
     * Sets the RotationMethod value on the server.
     * @param InRotationMethod The value to set RotationMethod to.
     */
    UFUNCTION(Server, Reliable)
    void SV_SetRotationMethod(const EAGR_RotationMethod InRotationMethod);

    /**
     * Directly sets the AimMethod value and broadcasts the change. 
     * @param InAimMethod The value to set AimMethod to.
     * @note This function does not check if current method is the same.
     */
    void InternalSetAimMethod(const EAGR_AimMethod InAimMethod);

    /**
     * Replication handler for AimMethod.
     * @param InOldAimMethod Previous AimMethod value.
     */
    UFUNCTION()
    void OnRep_AimMethod(const EAGR_AimMethod InOldAimMethod);

    /**
     * Sets the AimMethod value on the server.
     * @param InAimMethod The value to set AimMethod to.
     */
    UFUNCTION(Server, Reliable)
    void SV_SetAimMethod(const EAGR_AimMethod InAimMethod);

    /**
     * Directly sets the Pose value and broadcasts the change. 
     * @param InPose The value to set Pose to.
     * @note This function does not check if current value is the same.
     */
    void InternalSetPose(const FGameplayTag& InPose);

    /**
     * Replication handler for Pose.
     * @param InOldPose Previous Pose value.
     */
    UFUNCTION()
    void OnRep_Pose(const FGameplayTag& InOldPose);

    /**
     * Sets the Pose value on the server.
     * @param InPose The value to set Pose to.
     */
    UFUNCTION(Server, Reliable)
    void SV_SetPose(const FGameplayTag& InPose);

    /**
     * Directly sets the AnimationStates value and broadcasts the change. 
     * @param InAnimationStates The value to set AnimationStates to.
     * @note This function does not check if current value is the same.
     */
    void InternalSetAnimationStates(const FGameplayTagContainer& InAnimationStates);

    /**
     * Replication handler for AnimationStates.
     * @param InOldAnimationStates Previous AnimationStates value.
     */
    UFUNCTION()
    void OnRep_AnimationStates(const FGameplayTagContainer& InOldAnimationStates);

    /**
     * Set the AnimationStates value on the server.
     * @param InAnimationStates The value to set AnimationStates to.
     */
    UFUNCTION(Server, Reliable)
    void SV_SetAnimationStates(const FGameplayTagContainer& InAnimationStates);
};