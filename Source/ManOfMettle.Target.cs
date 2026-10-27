using UnrealBuildTool;

// Build target for Unreal Engine 5.8. "Latest" lets newer 5.x engines pick their own defaults.
public class ManOfMettleTarget : TargetRules
{
	public ManOfMettleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Hollowreach");
		ExtraModuleNames.Add("VeilOfAsh");
		ExtraModuleNames.Add("SharedArt");
	}
}
