// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DarkDungeonRift : ModuleRules
{
	public DarkDungeonRift(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[]
		{
			ModuleDirectory,
			System.IO.Path.Combine(ModuleDirectory, "Animation"),
			System.IO.Path.Combine(ModuleDirectory, "Characters"),
			System.IO.Path.Combine(ModuleDirectory, "Core"),
			System.IO.Path.Combine(ModuleDirectory, "GAS"),
			System.IO.Path.Combine(ModuleDirectory, "Game"),
			System.IO.Path.Combine(ModuleDirectory, "Movement")
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"NetCore"
		});
	}
}
