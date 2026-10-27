using UnrealBuildTool;
public class VeilOfAsh : ModuleRules
{
    public VeilOfAsh(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "AIModule", "SharedArt"
        });
    }
}
