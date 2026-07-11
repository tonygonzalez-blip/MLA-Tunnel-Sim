// Copyright Micrologic Associates. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MLA_Tunnel_SimEditorTarget : TargetRules
{
	public MLA_Tunnel_SimEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MLA_Tunnel_Sim");
	}
}
