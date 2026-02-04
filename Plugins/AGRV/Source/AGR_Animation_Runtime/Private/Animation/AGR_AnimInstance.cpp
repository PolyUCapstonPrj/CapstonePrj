// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable once CppUnusedIncludeDirective
#include "Animation/AGR_AnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "Animation/Components/AGR_LocomotionComponent.h"
#include "Animation/Lib/AGR_AnimationFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h" // Needed to assign UCapsuleComponent to UPrimitiveComponent

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_AnimInstance)

UAGR_AnimInstance::UAGR_AnimInstance()
{
    IdleThreshold = 10.0f;
    RotationSpeed = 6.0f;
    AimSpeed = 9.0f;
    StartRotatingAtAngle = 90.0f;
    AimClamp = 155.f;

    ForwardSpeed = 0.0f;
    StrafeSpeed = 0.0f;
    FallSpeed = 0.0f;

    bIdleXY = false;
    bIdle = false;
    bCrouching = false;
    bSwimming = false;
    bInAir = false;
    bFalling = false;
    bFlying = false;
    bMovingOnGround = false;

    NormalizedMovementDirection = FVector::ZeroVector;
    MovementSpeed = 0.0f;
    MovementSpeedXY = 0.0f;

    AimOffsetPitch = 0.0f;
    AimOffsetYaw = 0.0f;
    RelativeRootRotation = FRotator::ZeroRotator;
    DirectionAngle = 0.0f;
    Lean = 0.0f;
    LastUpdateRotation = FRotator::ZeroRotator;
    TargetRotation = FRotator::ZeroRotator;
    bRotate = false;
    TargetAimOffsetPitch = 0.0f;
    TargetAimOffsetYaw = 0.0f;
    LastRootRotation = FRotator::ZeroRotator;

    Owner = nullptr;
    OwningComponent = nullptr;
    LocomotionComponent = nullptr;

    OwnerVelocity = FVector::ZeroVector;
    OwnerRotation = FRotator::ZeroRotator;
    SkeletalMeshRotation = FRotator::ZeroRotator;
}

UAGR_LocomotionComponent* UAGR_AnimInstance::GetLocomotionComponent() const
{
    return LocomotionComponent;
}

void UAGR_AnimInstance::UpdateStates()
{
    if(!IsValid(Owner))
    {
        return;
    }

    if(!IsValid(OwningComponent))
    {
        return;
    }

    OwnerVelocity = Owner->GetVelocity();
    OwnerRotation = Owner->GetActorRotation();
    SkeletalMeshRotation = OwningComponent->GetComponentRotation();

    LocomotionComponent = UAGR_AnimationFunctionLibrary::GetLocomotionComponent(Owner);

    const UPawnMovementComponent* const MovementComponent = Owner->GetMovementComponent();
    if(IsValid(MovementComponent))
    {
        bCrouching = MovementComponent->IsCrouching();
        bSwimming = MovementComponent->IsSwimming();
        bFalling = MovementComponent->IsFalling();
        bFlying = MovementComponent->IsFlying();
        bMovingOnGround = MovementComponent->IsMovingOnGround();
    }
    else
    {
        bCrouching = false;
        bSwimming = false;
        bFalling = false;
        bFlying = false;
        bMovingOnGround = false;
    }

    bInAir = bFalling || bFlying;
}

void UAGR_AnimInstance::UpdateRotation(const float InDeltaSeconds)
{
    if(!IsValid(LocomotionComponent))
    {
        return;
    }

    const FQuat RotationDeltaQuat = (LastUpdateRotation - OwnerRotation).GetNormalized().Quaternion();
    RelativeRootRotation = (RelativeRootRotation.Quaternion() * RotationDeltaQuat).Rotator();
    LastUpdateRotation = OwnerRotation;

    switch(LocomotionComponent->GetRotationMethod())
    {
    case EAGR_RotationMethod::NoRotation:
        break;
    case EAGR_RotationMethod::AbsoluteRotation:
        {
            UpdateRotationAbsolute();
            break;
        }
    case EAGR_RotationMethod::RotateToMovement:
        {
            UpdateRotationToMovement();
            break;
        }
    case EAGR_RotationMethod::RotateToAim:
        {
            UpdateRotationToAim();
            break;
        }
    case EAGR_RotationMethod::RotateToAimAtAngle:
        {
            UpdateRotationToAimAtAngle();
            break;
        }
    default:
        checkNoEntry();
        break;
    }

    if(!bRotate)
    {
        Lean = 0.f;
        return;
    }

    LastRootRotation = RelativeRootRotation;
    RelativeRootRotation = FMath::RInterpTo(
        RelativeRootRotation,
        TargetRotation,
        InDeltaSeconds,
        RotationSpeed);

    Lean = (RelativeRootRotation - LastRootRotation).GetNormalized().Yaw;
}

void UAGR_AnimInstance::UpdateSpeedAndVelocity()
{
    MovementSpeed = OwnerVelocity.Size();
    MovementSpeedXY = OwnerVelocity.Size2D();
    bIdle = MovementSpeed < IdleThreshold;
    bIdleXY = MovementSpeedXY < IdleThreshold;

    const FVector AbsoluteVelocity = CalculateMeshVelocity();
    ForwardSpeed = AbsoluteVelocity.X;
    StrafeSpeed = AbsoluteVelocity.Y;
    FallSpeed = AbsoluteVelocity.Z;
}

void UAGR_AnimInstance::UpdateDirection()
{
    if(bIdleXY)
    {
        return;
    }

    if(IsValid(Owner))
    {
        const UPawnMovementComponent* const MovementComponent = Owner->GetMovementComponent();
        const FVector CurrentVelocity = MovementComponent->Velocity;
        const float MaxSpeed = MovementComponent->GetMaxSpeed();
        NormalizedMovementDirection = MaxSpeed == 0.0f
                                      ? FVector::ZeroVector
                                      : Owner->GetActorRotation().UnrotateVector(CurrentVelocity / MaxSpeed);
    }
    else
    {
        NormalizedMovementDirection = FVector::ZeroVector;
    }

    DirectionAngle = CalculateDirectionAngle();
}

void UAGR_AnimInstance::UpdateAimOffset(const float InDeltaSeconds)
{
    if(!IsValid(LocomotionComponent))
    {
        return;
    }

    switch(LocomotionComponent->GetAimMethod())
    {
    case EAGR_AimMethod::NoAimOffset:
        {
            UpdateAimOffsetNoAimOffset();
            break;
        }
    case EAGR_AimMethod::FreeLook:
        {
            UpdateAimOffsetFreeLook();
            break;
        }
    case EAGR_AimMethod::FreeLookClamped:
        {
            UpdateAimOffsetFreeLookClamped();
            break;
        }
    case EAGR_AimMethod::FreeLookClampedLocked:
        {
            UpdateAimOffsetFreeLookClampedLocked();
            break;
        }
    case EAGR_AimMethod::ForwardFacing:
        {
            UpdateAimOffsetForwardFacing();
            break;
        }
    case EAGR_AimMethod::LookAtMovementDirection:
        {
            UpdateAimOffsetLookAtMovementDirection();
            break;
        }
    default:
        checkNoEntry();
        break;
    }

    AimOffsetPitch = FMath::FInterpTo(AimOffsetPitch, TargetAimOffsetPitch, InDeltaSeconds, AimSpeed);
    AimOffsetYaw = FMath::FInterpTo(AimOffsetYaw, TargetAimOffsetYaw, InDeltaSeconds, AimSpeed);
}

bool UAGR_AnimInstance::IsRotatingLeft_Implementation() const
{
    return Lean < 0.f;
}

float UAGR_AnimInstance::GetRawAimOffsetPitch_Implementation() const
{
    if(!IsValid(Owner))
    {
        return 0.f;
    }

    const FRotator OwnerAimRotation = Owner->IsPawnControlled()
                                      ? Owner->GetControlRotation()
                                      : Owner->GetBaseAimRotation();

    return (OwnerAimRotation - SkeletalMeshRotation).GetNormalized().Pitch;
}

float UAGR_AnimInstance::GetRawAimOffsetYaw_Implementation() const
{
    return RelativeRootRotation.Yaw;
}

FVector UAGR_AnimInstance::CalculateMeshVelocity_Implementation() const
{
    const FQuat CombinedRotation = OwnerRotation.Quaternion() * RelativeRootRotation.Quaternion();
    return CombinedRotation.UnrotateVector(OwnerVelocity);
}

FRotator UAGR_AnimInstance::CalculateAimOffsetRotator_Implementation() const
{
    return FRotator{0.f, AimOffsetYaw, AimOffsetPitch};
}

float UAGR_AnimInstance::CalculateDirectionAngle_Implementation() const
{
    const FQuat CombinedRotation = RelativeRootRotation.Quaternion() * OwnerRotation.Quaternion();
    return UKismetAnimationLibrary::CalculateDirection(OwnerVelocity, CombinedRotation.Rotator());
}

float UAGR_AnimInstance::GetDeltaAimPitch_Implementation() const
{
    return TargetAimOffsetPitch - AimOffsetPitch;
}

float UAGR_AnimInstance::GetDeltaAimYaw_Implementation() const
{
    return TargetAimOffsetYaw - AimOffsetYaw;
}

bool UAGR_AnimInstance::IsRotating_Implementation() const
{
    return bRotate;
}

void UAGR_AnimInstance::UpdateRotationAbsolute()
{
    RelativeRootRotation = FRotator::ZeroRotator;
    bRotate = false;
}

void UAGR_AnimInstance::UpdateRotationToMovement()
{
    if(bIdleXY)
    {
        bRotate = false;
    }
    else
    {
        TargetRotation = OwnerRotation.UnrotateVector(OwnerVelocity).ToOrientationRotator();
        bRotate = true;
    }
}

void UAGR_AnimInstance::UpdateRotationToAim()
{
    TargetRotation = FRotator::ZeroRotator;
    bRotate = true;
}

void UAGR_AnimInstance::UpdateRotationToAimAtAngle()
{
    const bool bRotateRelativeRootRotation = FMath::Abs(RelativeRootRotation.Yaw) >= StartRotatingAtAngle;
    if(!(bRotateRelativeRootRotation || bRotate || !bIdleXY))
    {
        bRotate = false;
        return;
    }

    TargetRotation = FRotator::ZeroRotator;
    bRotate = !RelativeRootRotation.Equals(TargetRotation, 5);
}

void UAGR_AnimInstance::UpdateAimOffsetNoAimOffset()
{
    TargetAimOffsetPitch = 0.f;
    TargetAimOffsetYaw = 0.f;
}

void UAGR_AnimInstance::UpdateAimOffsetFreeLook()
{
    TargetAimOffsetPitch = GetRawAimOffsetPitch();
    TargetAimOffsetYaw = GetRawAimOffsetYaw();
}

void UAGR_AnimInstance::UpdateAimOffsetFreeLookClamped()
{
    TargetAimOffsetPitch = GetRawAimOffsetPitch();
    TargetAimOffsetYaw = FMath::Clamp(GetRawAimOffsetYaw(), -AimClamp, AimClamp);
}

void UAGR_AnimInstance::UpdateAimOffsetFreeLookClampedLocked()
{
    if(FMath::Abs(GetRawAimOffsetYaw()) <= AimClamp)
    {
        UpdateAimOffsetFreeLookClamped();
    }
}

void UAGR_AnimInstance::UpdateAimOffsetForwardFacing()
{
    TargetAimOffsetPitch = GetRawAimOffsetPitch();
    if(FMath::Abs(GetRawAimOffsetYaw()) <= AimClamp)
    {
        TargetAimOffsetYaw = GetRawAimOffsetYaw();
        return;
    }

    FVector2D InRange;
    FVector2D OutRange;
    if(GetRawAimOffsetYaw() > 0.f)
    {
        InRange = {AimClamp, 180.f};
        OutRange = {AimClamp, 0.f};
    }
    else
    {
        InRange = {-AimClamp, -180.f};
        OutRange = {-AimClamp, 0.f};
    }

    TargetAimOffsetYaw = FMath::GetMappedRangeValueClamped(InRange, OutRange, GetRawAimOffsetYaw());
}

void UAGR_AnimInstance::UpdateAimOffsetLookAtMovementDirection()
{
    TargetAimOffsetPitch = GetRawAimOffsetPitch();
    TargetAimOffsetYaw = -CalculateDirectionAngle();
}

void UAGR_AnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    Owner = TryGetPawnOwner();
    OwningComponent = GetOwningComponent();
}

void UAGR_AnimInstance::NativeBeginPlay()
{
    Super::NativeBeginPlay();
}

void UAGR_AnimInstance::NativeThreadSafeUpdateAnimation(const float DeltaSeconds)
{
    Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

    UpdateSpeedAndVelocity();
    UpdateDirection();
    UpdateRotation(DeltaSeconds);
    UpdateAimOffset(DeltaSeconds);
}

void UAGR_AnimInstance::PreUpdateAnimation(float DeltaSeconds)
{
    Super::PreUpdateAnimation(DeltaSeconds);
    UpdateStates();
}