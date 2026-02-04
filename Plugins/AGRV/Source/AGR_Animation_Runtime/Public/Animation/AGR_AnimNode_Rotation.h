// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNodes/AnimNode_RotateRootBone.h"

#include "AGR_AnimNode_Rotation.generated.h"

/**
 * This is a wrapper around the native FAnimNode_RotateRootBone.
 * It uses pre-configured data from UAGR_AnimInstance.
 */
USTRUCT(BlueprintInternalUseOnly)
struct AGR_ANIMATION_RUNTIME_API FAGR_AnimNode_Rotation : public FAnimNode_Base
{
    GENERATED_BODY()

public:
    /**
     * The local-space pose link to another node
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Links")
    FPoseLink BasePose;

    /**
     * Rotation to be applied on the skeletal mesh's root bone.
     */
    UPROPERTY(EditAnywhere, Category="3Studio AGR", meta=(PinShownByDefault))
    FRotator BoneRotation;

private:
    FAnimNode_RotateRootBone NativeAnimNode;

public:
    //~ Begin FAnimNode_Base Interface
    virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    virtual void Evaluate_AnyThread(FPoseContext& Output) override;
    virtual void GatherDebugData(FNodeDebugData& DebugData) override;
    //~ End FAnimNode_Base Interface
};