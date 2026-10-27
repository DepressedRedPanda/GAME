using UnrealBuildTool;

public class Hollowreach : ModuleRules
{
	public Hollowreach(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets every file include siblings as "Folder/File.h".
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"PhysicsCore",
			"SharedArt",
			"VeilOfAsh"
		});
	}
}
