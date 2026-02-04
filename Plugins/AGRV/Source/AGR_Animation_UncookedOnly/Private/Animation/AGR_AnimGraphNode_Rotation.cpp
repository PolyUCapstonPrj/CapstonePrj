// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#include "Animation/AGR_AnimGraphNode_Rotation.h"
#include "Animation/AGR_AnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AGR_AnimGraphNode_Rotation)

#define LOCTEXT_NAMESPACE "A3Nodes"

FLinearColor UAGR_AnimGraphNode_Rotation::GetNodeTitleColor() const
{
    return FColor::FromHex(TEXT("AA0022"));
}

FLinearColor UAGR_AnimGraphNode_Rotation::GetNodeBodyTintColor() const
{
    return FColor::FromHex(TEXT("000000"));
}

FText UAGR_AnimGraphNode_Rotation::GetTooltipText() const
{
    return LOCTEXT("AGR_Rotation", "AGR Rotation");
}

FText UAGR_AnimGraphNode_Rotation::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return LOCTEXT("AGR_Rotation", "AGR Rotation");
}

void UAGR_AnimGraphNode_Rotation::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    const FName PropertyName = PropertyChangedEvent.Property != nullptr
                               ? PropertyChangedEvent.Property->GetFName()
                               : NAME_None;

    // Reconstruct node to show updates to PinFriendlyNames.
    if(PropertyName == GET_MEMBER_NAME_CHECKED(UAGR_AnimGraphNode_Rotation, Node))
    {
        ReconstructNode();
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

FText UAGR_AnimGraphNode_Rotation::GetMenuCategory() const
{
    return LOCTEXT("AGR_RotationCategory", "3Studio AGR|Animation");
}

void UAGR_AnimGraphNode_Rotation::ValidateAnimNodeDuringCompilation(
    USkeleton* ForSkeleton,
    FCompilerResultsLog& MessageLog)
{
    Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
    const UAnimBlueprintGeneratedClass* const GeneratedClass = GetAnimBlueprint()->GetAnimBlueprintGeneratedClass();
    if(!IsValid(GeneratedClass))
    {
        return;
    }

    if(!GeneratedClass->IsChildOf(UAGR_AnimInstance::StaticClass()))
    {
        MessageLog.Error(
            *FString::Printf(
                TEXT("@@ is designed to work with Animation Blueprints derived from %s!"),
                *UAGR_AnimInstance::StaticClass()->GetDisplayNameText().ToString()),
            this);
    }
}

#undef LOCTEXT_NAMESPACE