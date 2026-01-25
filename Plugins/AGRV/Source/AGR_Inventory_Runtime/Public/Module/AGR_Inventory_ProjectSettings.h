// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "AGR_Inventory_ProjectSettings.generated.h"

/**
 * AGR Inventory Project Settings.
 */
UCLASS(Config="AGR", DefaultConfig)
class AGR_INVENTORY_RUNTIME_API UAGR_Inventory_ProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    const FName DEFAULT_ITEM_ACTOR_TAG = TEXT("AGRItem");

    /**
     * This tag will be automatically added to the actor which has an AGR Item component attached to it.
     * It is used to identify Actors as AGR Items.
     */
    UPROPERTY(config, EditDefaultsOnly, Category = "Inventory")
    FName ItemActorTag = DEFAULT_ITEM_ACTOR_TAG;

    virtual FName GetContainerName() const override
    {
        return TEXT("Project");
    };

    virtual FName GetCategoryName() const override
    {
        return TEXT("3Studio");
    };

    virtual FName GetSectionName() const override
    {
        return TEXT("AGR Inventory");
    };

#if WITH_EDITOR
    virtual FText GetSectionText() const override
    {
        return FText::FromString(TEXT("AGR Inventory"));
    };

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
    {
        Super::PostEditChangeProperty(PropertyChangedEvent);

        const FName PropertyName = PropertyChangedEvent.Property != nullptr
                                   ? PropertyChangedEvent.Property->GetFName()
                                   : NAME_None;

        if(PropertyName == GET_MEMBER_NAME_CHECKED(UAGR_Inventory_ProjectSettings, ItemActorTag))
        {
            if(ItemActorTag == NAME_None)
            {
                ItemActorTag = DEFAULT_ITEM_ACTOR_TAG;
            }
        }

        SaveConfig();
    }
#endif

    static const UAGR_Inventory_ProjectSettings* Get()
    {
        const auto PluginProjectSettings = GetDefault<UAGR_Inventory_ProjectSettings>();
        return IsValid(PluginProjectSettings) ? PluginProjectSettings : nullptr;
    }
};