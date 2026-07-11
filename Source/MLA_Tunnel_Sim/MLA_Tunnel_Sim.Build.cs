// Copyright Micrologic Associates. All Rights Reserved.

using UnrealBuildTool;

public class MLA_Tunnel_Sim : ModuleRules
{
	public MLA_Tunnel_Sim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"XmlParser"
		});
	}
}
