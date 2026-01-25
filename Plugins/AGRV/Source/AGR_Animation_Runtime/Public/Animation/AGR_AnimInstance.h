// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/Components/AGR_LocomotionComponent.h"

#include "AGR_AnimInstance.generated.h"

class APawn;
class USkeletalMeshComponent;
struct FAGR_AnimNode_Rotation;

/**
 * Base Anim Instance using AGR Locomotion Component.
 */
UCLASS(PrioritizeCategories="3Studio AGR")
class AGR_ANIMATION_RUNTIME_API UAGR_AnimInstance : public UAnimInstance
{
    GENERATED_BODY()

    friend FAGR_AnimNode_Rotation;

protected:
    /**
     * Defines the maximum threshold for the actor's speed that will still be handled as being idle.
     */
    UPROPERTY(
        BlueprintReadOnly,
        EditDefaultsOnly,
        Category="3Studio AGR|Config|Locomotion",
        meta=(ClampMin=0.0f, Units="CentimetersPerSecond"))
    float IdleThreshold;

    /**
     * Speed of interpolation of the rotation. Set to 0 to disable interpolation.
     */
    UPROPERTY(
        BlueprintReadOnly,
        EditDefaultsOnly,
        Category="3Studio AGR|Config|Rotation",
        meta=(ClampMin=0.0f, Units="DegreesPerSecond"))
    float RotationSpeed;

    /**
     * Speed of interpolation of the aim offset. Set to 0 to disable interpolation.
     */
    UPROPERTY(
        BlueprintReadOnly,
        EditDefaultsOnly,
        Category="3Studio AGR|Config|Rotation",
        meta=(ClampMin=0.0f, Units="DegreesPerSecond"))
    float AimSpeed;

    /**
     * The character will start to turn-in-place if the horizontal aim offset goes beyond this angle.
     */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="3Studio AGR|Config|Rotation", meta=(Units="Degrees"))
    float StartRotatingAtAngle;

    /**
     * Limits horizontal aim offset.
     */
    UPROPERTY(
        BlueprintReadOnly,
        EditDefaultsOnly,
        Category="3Studio AGR|Config|Rotation",
        meta=(ClampMin=0.0f, ClampMax=180.0f, Units="Degrees"))
    float AimClamp;

    /**
     * Forward speed of the pawn (X-axis).
     */
    UPROPERTY(
        BlueprintReadOnly,
        Transient,
        VisibleInstanceOnly,
        Category="3Studio AGR|Runtime|Speed",
        meta=(Units="CentimetersPerSecond"))
    float ForwardSpeed;

    /**
     * Strafe speed of the pawn (Y-axis).
     */
    UPROPERTY(
        BlueprintReadOnly,
        Transient,
        VisibleInstanceOnly,
        Category="3Studio AGR|Runtime|Speed",
        meta=(Units="CentimetersPerSecond"))
    float StrafeSpeed;

    /**
     * Falling speed of the pawn (Z-axis).
     */
    UPROPERTY(
        BlueprintReadOnly,
        Transient,
        VisibleInstanceOnly,
        Category="3Studio AGR|Runtime|Speed",
        meta=(Units="CentimetersPerSecond"))
    float FallSpeed;

    /**
     * True, if pawn has no planar movement according to IdleThreshold; Falling (Z-axis) is ignored. 
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Movement")
    bool bIdleXY;

    /**
     * True, if pawn has no movement in any direction according to IdleThreshold.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Movement")
    bool bIdle;

    /**
     * True, if pawn is crouching.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Movement")
    bool bCrouching;

    /**
     * True, if pawn is swimming.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Movement")
    bool bSwimming;

    /**
     * True, if pawn is either flying or falling.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Movement")
    bool bInAir;

    /**
     * True, if pawn is falling.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Movement")
    bool bFalling;

    /**
     * True, if pawn is flying.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Movement")
    bool bFlying;

    /**
     * True, if pawn is moving on ground.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Movement")
    bool bMovingOnGround;

    /**
     * Normalized movement direction of the pawn.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Movement")
    FVector NormalizedMovementDirection;

    /**
     * Movement speed of the pawn.
     */
    UPROPERTY(
        BlueprintReadOnly,
        Transient,
        VisibleInstanceOnly,
        Category="3Studio AGR|Runtime|Speed",
        meta=(Units="CentimetersPerSecond"))
    float MovementSpeed;

    /**
     * Movement speed of the pawn limited to the XY-plane.
     */
    UPROPERTY(
        BlueprintReadOnly,
        Transient,
        VisibleInstanceOnly,
        Category="3Studio AGR|Runtime|Speed",
        meta=(Units="CentimetersPerSecond"))
    float MovementSpeedXY;

    /**
     * Current pitch of the aim offset.
     */
    UPROPERTY(
        BlueprintReadOnly,
        Transient,
        VisibleInstanceOnly,
        Category="3Studio AGR|Runtime|Rotation",
        meta=(Units="Degrees"))
    float AimOffsetPitch;

    /**
     * Current yaw of the aim offset.
     */
    UPROPERTY(
        BlueprintReadOnly,
        Transient,
        VisibleInstanceOnly,
        Category="3Studio AGR|Runtime|Rotation",
        meta=(Units="Degrees"))
    float AimOffsetYaw;

    /**
     * World-space rotation between the skeletal mesh and pawn root component.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Rotation")
    FRotator RelativeRootRotation;

    /**
     * The direction angle that describes the movement direction relative to the skeletal mesh's root bone rotation.
     */
    UPROPERTY(
        BlueprintReadOnly,
        Transient,
        VisibleInstanceOnly,
        Category="3Studio AGR|Runtime|Rotation",
        meta=(ClampMin=-180.0f, ClampMax=180.0f, UIMin=-180.0f, UIMax=180.0f, Units="Degrees"))
    float DirectionAngle;

    /**
     * Amount of leaning into either direction.
     */
    UPROPERTY(BlueprintReadOnly, Transient, VisibleInstanceOnly, Category="3Studio AGR|Runtime|Rotation")
    float Lean;

private:
    /**
     * World-space absolute rotation of the previous frame for the root of the pawn.
     */
    UPROPERTY(Transient)
    FRotator LastUpdateRotation;

    /**
     * Desired rotation of the skeletal mesh.
     */
    UPROPERTY(Transient)
    FRotator TargetRotation;

    /**
     * True, if the root bone of the mesh should rotate towards the desired target rotation within this frame.
     * Can be used for turn-in-place but also will be true when rotating while moving.
     */
    UPROPERTY(Transient)
    bool bRotate;

    /**
     * Desired aim offset pitch.
     */
    UPROPERTY(Transient)
    float TargetAimOffsetPitch;

    /**
     * Desired aim offset yaw.
     */
    UPROPERTY(Transient)
    float TargetAimOffsetYaw;

    /**
     * Relative rotation of the previous frame between the skeletal mesh root component and pawn root component.
     */
    UPROPERTY(Transient)
    FRotator LastRootRotation;

    /**
     * The pawn owner of this animation instance.
     */
    UPROPERTY(Transient)
    TObjectPtr<APawn> Owner;

    /**
     * The skeletal mesh component that has created this animation instance.
     */
    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> OwningComponent;

    /**
     * The AGR Locomotion component.
     */
    UPROPERTY(Transient)
    TObjectPtr<UAGR_LocomotionComponent> LocomotionComponent;

    /**
     * Velocity of the owner.
     */
    UPROPERTY(Transient)
    FVector OwnerVelocity;

    /**
     * Rotation of the owner.
     */
    UPROPERTY(Transient)
    FRotator OwnerRotation;

    /**
     * The skeletal mesh rotation in world space.
     */
    UPROPERTY(Transient)
    FRotator SkeletalMeshRotation;

public:
    UAGR_AnimInstance();

    //~ Begin UAnimInstance
    virtual void NativeInitializeAnimation() override;
    virtual void NativeBeginPlay() override;
    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
    //~ ENdUAnimInstance

protected:
    //~ Begin UAnimInstance
    virtual void PreUpdateAnimation(float DeltaSeconds) override;
    //~ ENdUAnimInstance

    /**
     * Checks if pawn is leaning to the left side.
     * @returns True, if leaning left. Otherwise, false.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    bool IsRotatingLeft() const;

    /**
     * Gets the controller's pitch that takes network replication into account.
     * Value is internally calculated by unwinding the rotation to its "shortest route" rotation.
     * @returns Aim offset pitch.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    float GetRawAimOffsetPitch() const;

    /**
     * Gets the controller's yaw that takes network replication into account.
     * @returns Aim offset yaw.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    float GetRawAimOffsetYaw() const;

    /**
     * Calculates the velocity in relation to the skeletal mesh's root bone orientation.
     * @returns Mesh velocity.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    FVector CalculateMeshVelocity() const;

    /**
     * Calculates rotation from aim offset. Useful for IK.
     * @returns Aim offset rotation.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    FRotator CalculateAimOffsetRotator() const;

    /**
     * Calculates angle for movement direction relative to the mesh's root bone rotation.
     * @returns Direction angle.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    float CalculateDirectionAngle() const;

    /**
     * Calculates the remaining aim pitch until reaching target value.
     * @return Delta aim pitch. 
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    float GetDeltaAimPitch() const;

    /**
     * Calculates the remaining aim yaw until reaching target value.
     * @return Delta aim yaw.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    float GetDeltaAimYaw() const;

    /**
     * Checks whether skeletal mesh is rotating.
     * @return True, if skeletal mesh is rotating. Otherwise, false.
     */
    UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    bool IsRotating() const;

    UFUNCTION(BlueprintPure, Category="3Studio AGR|Animation", meta=(BlueprintThreadSafe))
    UAGR_LocomotionComponent* GetLocomotionComponent() const;

private:
    void UpdateStates();

    // Thread-safe functions.
    void UpdateRotation(float InDeltaSeconds);
    void UpdateSpeedAndVelocity();
    void UpdateDirection();
    void UpdateAimOffset(float InDeltaSeconds);

    void UpdateRotationAbsolute();
    void UpdateRotationToMovement();
    void UpdateRotationToAim();
    void UpdateRotationToAimAtAngle();

    void UpdateAimOffsetNoAimOffset();
    void UpdateAimOffsetFreeLook();
    void UpdateAimOffsetFreeLookClamped();
    void UpdateAimOffsetFreeLookClampedLocked();
    void UpdateAimOffsetForwardFacing();
    void UpdateAimOffsetLookAtMovementDirection();

};