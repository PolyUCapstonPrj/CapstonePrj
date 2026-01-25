// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "Animation/AGR_AnimNode_Rotation.h"

#include "AGR_AnimGraphNode_Rotation.generated.h"

/**
 * This is a wrapper around the native UAnimGraphNode_RotateRootBone.
 * It uses pre-configured data from UAGR_AnimInstance.
 */
UCLASS(MinimalAPI)
class UAGR_AnimGraphNode_Rotation : public UAnimGraphNode_Base
{
    GENERATED_BODY()

public:
    /**
     * The runtime AnimNode handling this AnimGraphNode's functionality.
     */
    UPROPERTY(EditAnywhere, Category="Settings")
    FAGR_AnimNode_Rotation Node;

public:
    //~ Begin UEdGraphNode Interface
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FLinearColor GetNodeBodyTintColor() const override;
    virtual FText GetTooltipText() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual FText GetMenuCategory() const override;
    //~ End UEdGraphNode Interface

    //~ Begin UAnimGraphNode_Base Interface
    virtual void ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog) override;
    //~ End UAnimGraphNode_Base Interface
};