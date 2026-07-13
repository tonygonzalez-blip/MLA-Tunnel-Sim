// Copyright Micrologic Associates. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MLA_Tunnel_SimTarget : TargetRules
{
	public MLA_Tunnel_SimTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MLA_Tunnel_Sim");
	}
}
