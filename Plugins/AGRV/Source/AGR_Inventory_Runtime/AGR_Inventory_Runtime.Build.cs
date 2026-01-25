// Copyright 2024 3S Game Studio OU. All Rights Reserved.

using UnrealBuildTool;

public class AGR_Inventory_Runtime : ModuleRules {
    
    public AGR_Inventory_Runtime(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "GameplayTags",
            }
        );
        
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "DeveloperSettings",
            }
        );
    }
    
}