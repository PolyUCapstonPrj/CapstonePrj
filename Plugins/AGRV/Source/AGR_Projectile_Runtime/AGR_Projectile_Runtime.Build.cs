// Copyright 2024 3S Game Studio OU. All Rights Reserved.

using UnrealBuildTool;

public class AGR_Projectile_Runtime : ModuleRules {

    public AGR_Projectile_Runtime(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "GameplayTags",
                "DeveloperSettings",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "AGR_Core_Runtime",
                "PhysicsCore",
            }
        );
    }

}