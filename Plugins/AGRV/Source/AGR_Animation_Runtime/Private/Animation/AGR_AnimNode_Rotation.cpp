// Copyright 2024 3S Game Studio OU. All Rights Reserved.

// ReSharper disable CppTooWideScopeInitStatement
#include "Animation/AGR_AnimNode_Rotation.h"
#include "Animation/AGR_AnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_AnimNode_Rotation)

void FAGR_AnimNode_Rotation::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
    NativeAnimNode.BasePose = BasePose;
    NativeAnimNode.Initialize_AnyThread(Context);
}

void FAGR_AnimNode_Rotation::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
    NativeAnimNode.CacheBones_AnyThread(Context);
}

void FAGR_AnimNode_Rotation::Update_AnyThread(const FAnimationUpdateContext& Context)
{
    const UObject* const AnimInstanceObject = Context.GetAnimInstanceObject();
    if(IsValid(AnimInstanceObject))
    {
        const UAGR_AnimInstance* const AnimInstance = Cast<UAGR_AnimInstance>(AnimInstanceObject);
        if(IsValid(AnimInstance))
        {
            NativeAnimNode.Yaw = AnimInstance->RelativeRootRotation.Yaw;
            NativeAnimNode.MeshToComponent = BoneRotation;
        }
    }

    NativeAnimNode.Update_AnyThread(Context);
}

void FAGR_AnimNode_Rotation::Evaluate_AnyThread(FPoseContext& Output)
{
    NativeAnimNode.Evaluate_AnyThread(Output);
}

void FAGR_AnimNode_Rotation::GatherDebugData(FNodeDebugData& DebugData)
{
    NativeAnimNode.GatherDebugData(DebugData);
}