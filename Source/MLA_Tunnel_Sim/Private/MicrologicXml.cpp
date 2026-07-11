// Copyright Micrologic Associates. All Rights Reserved.

#include "MicrologicXml.h"
#include "XmlFile.h"

namespace
{
	FString Escape(const FString& In)
	{
		FString Out = In;
		Out.ReplaceInline(TEXT("&"), TEXT("&amp;"));
		Out.ReplaceInline(TEXT("<"), TEXT("&lt;"));
		Out.ReplaceInline(TEXT(">"), TEXT("&gt;"));
		Out.ReplaceInline(TEXT("\""), TEXT("&quot;"));
		Out.ReplaceInline(TEXT("'"), TEXT("&apos;"));
		return Out;
	}

	FString IntArrayToCsv(const TArray<int32>& Values)
	{
		FString Out;
		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			if (Index > 0)
			{
				Out += TEXT(",");
			}
			Out += FString::FromInt(Values[Index]);
		}
		return Out;
	}

	TArray<int32> CsvToIntArray(const FString& Csv)
	{
		TArray<int32> Out;
		TArray<FString> Parts;
		Csv.ParseIntoArray(Parts, TEXT(","), /*bCullEmpty=*/true);
		for (const FString& Part : Parts)
		{
			Out.Add(FCString::Atoi(*Part));
		}
		return Out;
	}

	FString BoolStr(bool bValue) { return bValue ? TEXT("true") : TEXT("false"); }
	bool StrBool(const FString& Value) { return Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value == TEXT("1"); }

	int32 AttrInt(const FXmlNode* Node, const TCHAR* Name, int32 Default = 0)
	{
		const FString Value = Node->GetAttribute(Name);
		return Value.IsEmpty() ? Default : FCString::Atoi(*Value);
	}

	float AttrFloat(const FXmlNode* Node, const TCHAR* Name, float Default = 0.f)
	{
		const FString Value = Node->GetAttribute(Name);
		return Value.IsEmpty() ? Default : FCString::Atof(*Value);
	}

	bool AttrBool(const FXmlNode* Node, const TCHAR* Name, bool bDefault = false)
	{
		const FString Value = Node->GetAttribute(Name);
		return Value.IsEmpty() ? bDefault : StrBool(Value);
	}

	FString AttrStr(const FXmlNode* Node, const TCHAR* Name, const FString& Default = FString())
	{
		const FString Value = Node->GetAttribute(Name);
		return Value.IsEmpty() ? Default : Value;
	}
}

FString MicrologicXml::ConfigToXml(const FMLControllerConfig& Config)
{
	FString Xml;
	Xml += TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	Xml += TEXT("<MicrologicConfig Version=\"1\">\n");

	const FMLCommunicationsSettings& Comms = Config.Communications;
	Xml += FString::Printf(
		TEXT("\t<Communications NumInputBoards=\"%d\" NumRelayBoards=\"%d\" InputBoardPort=\"%s\" OutputBoardPort=\"%s\" KeypadPort=\"%s\" ExitDoorEnabled=\"%s\" SonarAddress=\"%s\"/>\n"),
		Comms.NumInputBoards, Comms.NumRelayBoards,
		*Escape(Comms.InputBoardPort), *Escape(Comms.OutputBoardPort), *Escape(Comms.KeypadPort),
		*BoolStr(Comms.bExitDoorEnabled), *Escape(Comms.SonarAddress));

	const FMLConveyorSettings& Conveyor = Config.Conveyor;
	Xml += FString::Printf(
		TEXT("\t<Conveyor InchesPerSecond=\"%f\" InchesPerPulse=\"%f\" OnActivationService=\"%d\" ShutOffService=\"%d\" InactivityTimeoutSeconds=\"%f\" HornTimeSeconds=\"%f\" HornDelaySeconds=\"%f\"/>\n"),
		Conveyor.InchesPerSecond, Conveyor.InchesPerPulse, Conveyor.OnActivationService,
		Conveyor.ShutOffService, Conveyor.InactivityTimeoutSeconds, Conveyor.HornTimeSeconds,
		Conveyor.HornDelaySeconds);

	const FMLAntiCollisionSettings& AC = Config.AntiCollision;
	Xml += FString::Printf(
		TEXT("\t<AntiCollision RelayNumber=\"%d\" AfterClearsActivateService=\"%d\" SlowDownService=\"%d\" SlowDownTimeSeconds=\"%f\" SlowDownHornService=\"%d\"/>\n"),
		AC.RelayNumber, AC.AfterClearsActivateService, AC.SlowDownService,
		AC.SlowDownTimeSeconds, AC.SlowDownHornService);

	const FMLRollerDefaults& Roller = Config.RollerDefaults;
	Xml += FString::Printf(
		TEXT("\t<RollerDefaults MinCarLengthFeet=\"%f\" MaxCarLengthFeet=\"%f\" AverageCarLengthFeet=\"%f\" RollerMode=\"%d\" UpFeet=\"%f\" DownFeet=\"%f\" UpAgainFeet=\"%f\" NeedsCarQueued=\"%s\" DefaultInputDebounceSeconds=\"%f\" QueueMode=\"%d\" ServiceOnQueued=\"%d\" DefaultWashService=\"%d\"/>\n"),
		Roller.MinCarLengthFeet, Roller.MaxCarLengthFeet, Roller.AverageCarLengthFeet,
		static_cast<int32>(Roller.RollerMode), Roller.UpFeet, Roller.DownFeet, Roller.UpAgainFeet,
		*BoolStr(Roller.bNeedsCarQueuedForRollerRequest), Roller.DefaultInputDebounceSeconds,
		static_cast<int32>(Roller.QueueMode), Roller.ServiceOnQueued, Roller.DefaultWashService);

	const FMLSecuritySettings& Security = Config.Security;
	Xml += FString::Printf(
		TEXT("\t<Security RequirePin=\"%s\" PinCode=\"%s\" UiPassword=\"%s\" Username=\"%s\" Password=\"%s\"/>\n"),
		*BoolStr(Security.bRequirePinForRelayOverride), *Escape(Security.PinCode),
		*Escape(Security.UiPassword), *Escape(Security.Username), *Escape(Security.Password));

	const FMLSimSettings& Sim = Config.Sim;
	Xml += FString::Printf(
		TEXT("\t<Sim TunnelLengthFeet=\"%f\" AutoConveyorInterlockWire=\"%s\" AntiCollisionPadPositionFeet=\"%f\"/>\n"),
		Sim.TunnelLengthFeet, *BoolStr(Sim.bAutoConveyorInterlockWire), Sim.AntiCollisionPadPositionFeet);

	Xml += TEXT("\t<Inputs>\n");
	for (const FMLInputConfig& Input : Config.Inputs)
	{
		Xml += FString::Printf(
			TEXT("\t\t<Input Channel=\"%d\" Type=\"%d\" Description=\"%s\" Inverted=\"%s\" DebounceSeconds=\"%f\" TriggerServiceNumber=\"%d\"/>\n"),
			Input.Channel, static_cast<int32>(Input.Type), *Escape(Input.Description),
			*BoolStr(Input.bInverted), Input.DebounceSeconds, Input.TriggerServiceNumber);
	}
	Xml += TEXT("\t</Inputs>\n");

	Xml += TEXT("\t<Relays>\n");
	for (const FMLRelayConfig& Relay : Config.Relays)
	{
		Xml += FString::Printf(
			TEXT("\t\t<Relay RelayNumber=\"%d\" Active=\"%s\" Description=\"%s\" Default=\"%s\" Type=\"%d\" InactivityCheck=\"%s\" InterlockStartSeconds=\"%f\" InterlockStopSeconds=\"%f\" LookBackFeet=\"%f\">\n"),
			Relay.RelayNumber, *BoolStr(Relay.bActive), *Escape(Relay.Description),
			*BoolStr(Relay.bDefault), static_cast<int32>(Relay.Type), *BoolStr(Relay.bInactivityCheck),
			Relay.InterlockStartSeconds, Relay.InterlockStopSeconds, Relay.LookBackFeet);

		const FMLFunctionConfig& Fn = Relay.Function;
		Xml += FString::Printf(
			TEXT("\t\t\t<Function Type=\"%d\" DevicePositionFeet=\"%f\" TurnOnFeet=\"%f\" TurnOnReference=\"%d\" TurnOffFeet=\"%f\" TurnOffReference=\"%d\">\n"),
			static_cast<int32>(Fn.Type), Fn.DevicePositionFeet, Fn.TurnOnFeet,
			static_cast<int32>(Fn.TurnOnReference), Fn.TurnOffFeet, static_cast<int32>(Fn.TurnOffReference));
		for (const FMLModifierConfig& Mod : Fn.Modifiers)
		{
			Xml += FString::Printf(
				TEXT("\t\t\t\t<Modifier Type=\"%d\" StartFeet=\"%f\" LengthFeet=\"%f\"/>\n"),
				static_cast<int32>(Mod.Type), Mod.StartFeet, Mod.LengthFeet);
		}
		Xml += TEXT("\t\t\t</Function>\n");
		Xml += TEXT("\t\t</Relay>\n");
	}
	Xml += TEXT("\t</Relays>\n");

	Xml += TEXT("\t<Services>\n");
	for (const FMLServiceConfig& Service : Config.Services)
	{
		Xml += FString::Printf(
			TEXT("\t\t<Service ServiceNumber=\"%d\" Description=\"%s\" Type=\"%d\" RelayNumbers=\"%s\" TimeSeconds=\"%f\" DelaySeconds=\"%f\" MacroServiceNumbers=\"%s\" Command=\"%d\"/>\n"),
			Service.ServiceNumber, *Escape(Service.Description), static_cast<int32>(Service.Type),
			*IntArrayToCsv(Service.RelayNumbers), Service.TimeSeconds, Service.DelaySeconds,
			*IntArrayToCsv(Service.MacroServiceNumbers), static_cast<int32>(Service.Command));
	}
	Xml += TEXT("\t</Services>\n");

	Xml += TEXT("</MicrologicConfig>\n");
	return Xml;
}

bool MicrologicXml::ConfigFromXml(const FString& Xml, FMLControllerConfig& OutConfig)
{
	FXmlFile File(Xml, EConstructMethod::ConstructFromBuffer);
	if (!File.IsValid())
	{
		return false;
	}

	const FXmlNode* Root = File.GetRootNode();
	if (!Root || Root->GetTag() != TEXT("MicrologicConfig"))
	{
		return false;
	}

	FMLControllerConfig Config;

	if (const FXmlNode* Node = Root->FindChildNode(TEXT("Communications")))
	{
		FMLCommunicationsSettings& Comms = Config.Communications;
		Comms.NumInputBoards = AttrInt(Node, TEXT("NumInputBoards"), 1);
		Comms.NumRelayBoards = AttrInt(Node, TEXT("NumRelayBoards"), 1);
		Comms.InputBoardPort = AttrStr(Node, TEXT("InputBoardPort"));
		Comms.OutputBoardPort = AttrStr(Node, TEXT("OutputBoardPort"));
		Comms.KeypadPort = AttrStr(Node, TEXT("KeypadPort"));
		Comms.bExitDoorEnabled = AttrBool(Node, TEXT("ExitDoorEnabled"));
		Comms.SonarAddress = AttrStr(Node, TEXT("SonarAddress"));
	}

	if (const FXmlNode* Node = Root->FindChildNode(TEXT("Conveyor")))
	{
		FMLConveyorSettings& Conveyor = Config.Conveyor;
		Conveyor.InchesPerSecond = AttrFloat(Node, TEXT("InchesPerSecond"), 13.94f);
		Conveyor.InchesPerPulse = AttrFloat(Node, TEXT("InchesPerPulse"), 12.86f);
		Conveyor.OnActivationService = AttrInt(Node, TEXT("OnActivationService"));
		Conveyor.ShutOffService = AttrInt(Node, TEXT("ShutOffService"));
		Conveyor.InactivityTimeoutSeconds = AttrFloat(Node, TEXT("InactivityTimeoutSeconds"));
		Conveyor.HornTimeSeconds = AttrFloat(Node, TEXT("HornTimeSeconds"));
		Conveyor.HornDelaySeconds = AttrFloat(Node, TEXT("HornDelaySeconds"));
	}

	if (const FXmlNode* Node = Root->FindChildNode(TEXT("AntiCollision")))
	{
		FMLAntiCollisionSettings& AC = Config.AntiCollision;
		AC.RelayNumber = AttrInt(Node, TEXT("RelayNumber"));
		AC.AfterClearsActivateService = AttrInt(Node, TEXT("AfterClearsActivateService"));
		AC.SlowDownService = AttrInt(Node, TEXT("SlowDownService"));
		AC.SlowDownTimeSeconds = AttrFloat(Node, TEXT("SlowDownTimeSeconds"));
		AC.SlowDownHornService = AttrInt(Node, TEXT("SlowDownHornService"));
	}

	if (const FXmlNode* Node = Root->FindChildNode(TEXT("RollerDefaults")))
	{
		FMLRollerDefaults& Roller = Config.RollerDefaults;
		Roller.MinCarLengthFeet = AttrFloat(Node, TEXT("MinCarLengthFeet"), 6.f);
		Roller.MaxCarLengthFeet = AttrFloat(Node, TEXT("MaxCarLengthFeet"), 25.f);
		Roller.AverageCarLengthFeet = AttrFloat(Node, TEXT("AverageCarLengthFeet"), 15.f);
		Roller.RollerMode = static_cast<EMLRollerMode>(AttrInt(Node, TEXT("RollerMode")));
		Roller.UpFeet = AttrFloat(Node, TEXT("UpFeet"));
		Roller.DownFeet = AttrFloat(Node, TEXT("DownFeet"));
		Roller.UpAgainFeet = AttrFloat(Node, TEXT("UpAgainFeet"));
		Roller.bNeedsCarQueuedForRollerRequest = AttrBool(Node, TEXT("NeedsCarQueued"));
		Roller.DefaultInputDebounceSeconds = AttrFloat(Node, TEXT("DefaultInputDebounceSeconds"));
		Roller.QueueMode = static_cast<EMLQueueMode>(AttrInt(Node, TEXT("QueueMode")));
		Roller.ServiceOnQueued = AttrInt(Node, TEXT("ServiceOnQueued"));
		Roller.DefaultWashService = AttrInt(Node, TEXT("DefaultWashService"));
	}

	if (const FXmlNode* Node = Root->FindChildNode(TEXT("Security")))
	{
		FMLSecuritySettings& Security = Config.Security;
		Security.bRequirePinForRelayOverride = AttrBool(Node, TEXT("RequirePin"));
		Security.PinCode = AttrStr(Node, TEXT("PinCode"));
		Security.UiPassword = AttrStr(Node, TEXT("UiPassword"), TEXT("manager01"));
		Security.Username = AttrStr(Node, TEXT("Username"), TEXT("manager"));
		Security.Password = AttrStr(Node, TEXT("Password"), TEXT("manager01"));
	}

	if (const FXmlNode* Node = Root->FindChildNode(TEXT("Sim")))
	{
		FMLSimSettings& Sim = Config.Sim;
		Sim.TunnelLengthFeet = AttrFloat(Node, TEXT("TunnelLengthFeet"), 120.f);
		Sim.bAutoConveyorInterlockWire = AttrBool(Node, TEXT("AutoConveyorInterlockWire"), true);
		Sim.AntiCollisionPadPositionFeet = AttrFloat(Node, TEXT("AntiCollisionPadPositionFeet"), 110.f);
	}

	if (const FXmlNode* Inputs = Root->FindChildNode(TEXT("Inputs")))
	{
		for (const FXmlNode* Node : Inputs->GetChildrenNodes())
		{
			if (Node->GetTag() != TEXT("Input"))
			{
				continue;
			}
			FMLInputConfig Input;
			Input.Channel = AttrInt(Node, TEXT("Channel"));
			Input.Type = static_cast<EMLInputType>(AttrInt(Node, TEXT("Type")));
			Input.Description = AttrStr(Node, TEXT("Description"));
			Input.bInverted = AttrBool(Node, TEXT("Inverted"));
			Input.DebounceSeconds = AttrFloat(Node, TEXT("DebounceSeconds"));
			Input.TriggerServiceNumber = AttrInt(Node, TEXT("TriggerServiceNumber"));
			Config.Inputs.Add(Input);
		}
	}

	if (const FXmlNode* Relays = Root->FindChildNode(TEXT("Relays")))
	{
		for (const FXmlNode* Node : Relays->GetChildrenNodes())
		{
			if (Node->GetTag() != TEXT("Relay"))
			{
				continue;
			}
			FMLRelayConfig Relay;
			Relay.RelayNumber = AttrInt(Node, TEXT("RelayNumber"));
			Relay.bActive = AttrBool(Node, TEXT("Active"), true);
			Relay.Description = AttrStr(Node, TEXT("Description"));
			Relay.bDefault = AttrBool(Node, TEXT("Default"));
			Relay.Type = static_cast<EMLRelayType>(AttrInt(Node, TEXT("Type")));
			Relay.bInactivityCheck = AttrBool(Node, TEXT("InactivityCheck"));
			Relay.InterlockStartSeconds = AttrFloat(Node, TEXT("InterlockStartSeconds"));
			Relay.InterlockStopSeconds = AttrFloat(Node, TEXT("InterlockStopSeconds"));
			Relay.LookBackFeet = AttrFloat(Node, TEXT("LookBackFeet"));

			if (const FXmlNode* FnNode = Node->FindChildNode(TEXT("Function")))
			{
				FMLFunctionConfig& Fn = Relay.Function;
				Fn.Type = static_cast<EMLFunctionType>(AttrInt(FnNode, TEXT("Type")));
				Fn.DevicePositionFeet = AttrFloat(FnNode, TEXT("DevicePositionFeet"));
				Fn.TurnOnFeet = AttrFloat(FnNode, TEXT("TurnOnFeet"));
				Fn.TurnOnReference = static_cast<EMLTurnReference>(AttrInt(FnNode, TEXT("TurnOnReference")));
				Fn.TurnOffFeet = AttrFloat(FnNode, TEXT("TurnOffFeet"));
				Fn.TurnOffReference = static_cast<EMLTurnReference>(AttrInt(FnNode, TEXT("TurnOffReference")));

				for (const FXmlNode* ModNode : FnNode->GetChildrenNodes())
				{
					if (ModNode->GetTag() != TEXT("Modifier"))
					{
						continue;
					}
					FMLModifierConfig Mod;
					Mod.Type = static_cast<EMLModifierType>(AttrInt(ModNode, TEXT("Type")));
					Mod.StartFeet = AttrFloat(ModNode, TEXT("StartFeet"));
					Mod.LengthFeet = AttrFloat(ModNode, TEXT("LengthFeet"));
					Fn.Modifiers.Add(Mod);
				}
			}

			Config.Relays.Add(Relay);
		}
	}

	if (const FXmlNode* Services = Root->FindChildNode(TEXT("Services")))
	{
		for (const FXmlNode* Node : Services->GetChildrenNodes())
		{
			if (Node->GetTag() != TEXT("Service"))
			{
				continue;
			}
			FMLServiceConfig Service;
			Service.ServiceNumber = AttrInt(Node, TEXT("ServiceNumber"));
			Service.Description = AttrStr(Node, TEXT("Description"));
			Service.Type = static_cast<EMLServiceType>(AttrInt(Node, TEXT("Type")));
			Service.RelayNumbers = CsvToIntArray(AttrStr(Node, TEXT("RelayNumbers")));
			Service.TimeSeconds = AttrFloat(Node, TEXT("TimeSeconds"));
			Service.DelaySeconds = AttrFloat(Node, TEXT("DelaySeconds"));
			Service.MacroServiceNumbers = CsvToIntArray(AttrStr(Node, TEXT("MacroServiceNumbers")));
			Service.Command = static_cast<EMLCommand>(AttrInt(Node, TEXT("Command")));
			Config.Services.Add(Service);
		}
	}

	OutConfig = MoveTemp(Config);
	return true;
}
