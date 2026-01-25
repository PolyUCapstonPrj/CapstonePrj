// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "AGR_Projectile_ProjectSettings.generated.h"

/**
 * AGR projectile project settings.
 */
UCLASS(Config="AGR", DefaultConfig)
class AGR_PROJECTILE_RUNTIME_API UAGR_Projectile_ProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(config, EditDefaultsOnly, Category = "Debug")
    FColor DebugColor_TraceStart = FColor::Green;

    UPROPERTY(config, EditDefaultsOnly, Category = "Debug")
    FColor DebugColor_TraceEnd = FColor::Red;

    UPROPERTY(config, EditDefaultsOnly, Category = "Debug")
    FColor DebugColor_TerminalHit = FColor::Red;

    UPROPERTY(config, EditDefaultsOnly, Category = "Debug")
    FColor DebugColor_PenetrationEnter = FColor::Green;

    UPROPERTY(config, EditDefaultsOnly, Category = "Debug")
    FColor DebugColor_PenetrationExit = FColor::Blue;

    UPROPERTY(config, EditDefaultsOnly, Category = "Debug")
    FColor DebugColor_Ricochet = FColor::Cyan;

    UPROPERTY(config, EditDefaultsOnly, Category = "Debug")
    FColor DebugColor_XRayTrace = FColor::Yellow;

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
        return TEXT("AGR Projectile");
    };

#if WITH_EDITOR
    virtual FText GetSectionText() const override
    {
        return FText::FromString(TEXT("AGR Projectile"));
    };
#endif

    static UAGR_Projectile_ProjectSettings* Get()
    {
        const auto PluginProjectSettings = GetMutableDefault<UAGR_Projectile_ProjectSettings>();
        return IsValid(PluginProjectSettings) ? PluginProjectSettings : nullptr;
    }
};