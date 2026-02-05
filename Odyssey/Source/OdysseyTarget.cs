using UnrealBuildTool;
using System.Collections.Generic;

public class OdysseyTarget : TargetRules
{
	public OdysseyTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;

		ExtraModuleNames.AddRange( new string[] { "Odyssey" } );

		// Mobile optimizations
		if (Target.Platform == UnrealTargetPlatform.Android ||
			Target.Platform == UnrealTargetPlatform.IOS)
		{
			bUseUnityBuild = true;
			bUsePCHFiles = true;
			MinFilesUsingPrecompiledHeaderOverride = 1;
			bForceEnableExceptions = false;
		}
	}
}