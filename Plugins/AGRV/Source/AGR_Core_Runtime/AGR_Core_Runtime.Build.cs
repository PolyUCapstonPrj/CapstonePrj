// Copyright 2024 3S Game Studio OU. All Rights Reserved.

using UnrealBuildTool;

public class AGR_Core_Runtime : ModuleRules {

    public AGR_Core_Runtime(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
            }
        );
    }

}