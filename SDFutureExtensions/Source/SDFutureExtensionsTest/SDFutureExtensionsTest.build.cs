// Copyright(c) Splash Damage. All rights reserved.
using UnrealBuildTool;

public class SDFutureExtensionsTest : ModuleRules
{
	public SDFutureExtensionsTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.AddRange(
			new string[] {

			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"SDFutureExtensions"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"SDTestHarness",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {

			}
		);
	}
}
