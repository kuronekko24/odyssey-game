using UnrealBuildTool;

public class Odyssey : ModuleRules
{
	public Odyssey(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory,
			System.IO.Path.Combine(ModuleDirectory, "Procedural"),
			System.IO.Path.Combine(ModuleDirectory, "Combat"),
			System.IO.Path.Combine(ModuleDirectory, "Crafting"),
			System.IO.Path.Combine(ModuleDirectory, "Economy"),
			System.IO.Path.Combine(ModuleDirectory, "Social")
		});

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"ApplicationCore"
		});

		// Mobile-specific modules
		if (Target.Platform == UnrealTargetPlatform.Android ||
			Target.Platform == UnrealTargetPlatform.IOS)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"Launch"
			});
		}
	}
}
