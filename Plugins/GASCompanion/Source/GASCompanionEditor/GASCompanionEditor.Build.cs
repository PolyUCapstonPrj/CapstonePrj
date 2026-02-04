// Copyright 2021 Mickael Daniel. All Rights Reserved.

using UnrealBuildTool;

public class GASCompanionEditor : ModuleRules
{
	public GASCompanionEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayAbilities",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AppFramework",
				"ApplicationCore",
				"CoreUObject",
				"Engine",
				"EngineSettings",
				"GASCompanion",
				"GameProjectGeneration",
				"GameplayTags",
				"InputCore",
				"Projects",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd",
			}
		);
	}
}
