// Copyright 2024 3S Game Studio OU. All Rights Reserved.

using UnrealBuildTool;

public class AGR_Animation_UncookedOnly : ModuleRules {
    public AGR_Animation_UncookedOnly(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "AGR_Animation_Runtime",
            }
        );
        
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "BlueprintGraph",
                "AnimGraph",
                "AnimGraphRuntime",
            }
        );
    }
    
}