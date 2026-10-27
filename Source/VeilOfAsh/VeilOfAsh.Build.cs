using UnrealBuildTool;

// Retained solely to deserialize the original /Script/VeilOfAsh.AshSave type.
public class VeilOfAsh : ModuleRules
{
    public VeilOfAsh(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.Add(ModuleDirectory);
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
    }
}
