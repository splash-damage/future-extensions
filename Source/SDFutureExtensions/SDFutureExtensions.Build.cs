// Copyright(c) Splash Damage. All rights reserved.
using UnrealBuildTool;
using System.IO;

public class SDFutureExtensions : ModuleRules
{
	public SDFutureExtensions(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePathModuleNames.AddRange(new string[] {
			"Core"
		});

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
		});
	}
}