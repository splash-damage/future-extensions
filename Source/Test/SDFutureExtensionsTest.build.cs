// Copyright(c) Splash Damage. All rights reserved.
using UnrealBuildTool;

public class SDFutureExtensionsTest : ModuleRules
{
	public SDFutureExtensionsTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "Automatron",
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
			"SDFutureExtensions"
		});
	}
}
