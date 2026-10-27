using UnrealBuildTool;

public class HollowreachEditorTarget : TargetRules
{
	public HollowreachEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Hollowreach");
		ExtraModuleNames.Add("VeilOfAsh");
		ExtraModuleNames.Add("SharedArt");
	}
}
