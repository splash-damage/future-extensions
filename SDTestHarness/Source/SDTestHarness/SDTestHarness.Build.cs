using UnrealBuildTool;
using System.IO;

public class SDTestHarness : ModuleRules
{
	public SDTestHarness(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDefinitions.Add("WITH_SDTEST_EXTENSIONS=0");
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}