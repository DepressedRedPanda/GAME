using UnrealBuildTool;

public class HollowreachTarget : TargetRules
{
	public HollowreachTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Hollowreach");
		ExtraModuleNames.Add("VeilOfAsh");
		ExtraModuleNames.Add("SharedArt");
	}
}
