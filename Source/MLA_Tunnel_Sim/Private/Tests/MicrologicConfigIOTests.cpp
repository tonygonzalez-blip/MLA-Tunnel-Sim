// Copyright Micrologic Associates. All Rights Reserved.
//
// Backup/Restore XML round-trip tests and factory-default config consistency.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.h"
#include "MicrologicXml.h"
#include "MicrologicTestHelpers.h"

using namespace MicrologicTest;

namespace
{
	constexpr float FloatTolerance = 1.e-3f; // %f keeps 6 decimals; quarters survive exactly

	/** A configuration with every field non-default. Float values are exact binary fractions so %f round-trips them losslessly. */
	FMLControllerConfig MakeFullyPopulatedConfig()
	{
		FMLControllerConfig Config;

		Config.Communications.NumInputBoards = 2;
		Config.Communications.NumRelayBoards = 4;
		Config.Communications.InputBoardPort = TEXT("COM5");
		Config.Communications.OutputBoardPort = TEXT("COM6");
		Config.Communications.KeypadPort = TEXT("COM7");
		Config.Communications.bExitDoorEnabled = true;
		Config.Communications.SonarAddress = TEXT("10.0.9.95");

		Config.Conveyor.InchesPerSecond = 13.25f;
		Config.Conveyor.InchesPerPulse = 12.5f;
		Config.Conveyor.OnActivationService = 5;
		Config.Conveyor.ShutOffService = 6;
		Config.Conveyor.InactivityTimeoutSeconds = 90.5f;
		Config.Conveyor.HornTimeSeconds = 2.5f;
		Config.Conveyor.HornDelaySeconds = 1.25f;

		Config.AntiCollision.RelayNumber = 19;
		Config.AntiCollision.AfterClearsActivateService = 5;
		Config.AntiCollision.SlowDownService = 7;
		Config.AntiCollision.SlowDownTimeSeconds = 3.5f;
		Config.AntiCollision.SlowDownHornService = 8;

		Config.RollerDefaults.MinCarLengthFeet = 7.5f;
		Config.RollerDefaults.MaxCarLengthFeet = 24.5f;
		Config.RollerDefaults.AverageCarLengthFeet = 16.25f;
		Config.RollerDefaults.RollerMode = EMLRollerMode::ManualFront;
		Config.RollerDefaults.UpFeet = 3.5f;
		Config.RollerDefaults.DownFeet = 2.5f;
		Config.RollerDefaults.UpAgainFeet = 9.5f;
		Config.RollerDefaults.bNeedsCarQueuedForRollerRequest = true;
		Config.RollerDefaults.DefaultInputDebounceSeconds = 0.25f;
		Config.RollerDefaults.QueueMode = EMLQueueMode::Sequential;
		Config.RollerDefaults.ServiceOnQueued = 9;
		Config.RollerDefaults.DefaultWashService = 2;

		Config.Security.bRequirePinForRelayOverride = true;
		Config.Security.PinCode = TEXT("4321");
		Config.Security.UiPassword = TEXT("vncpass");
		Config.Security.Username = TEXT("user1");
		Config.Security.Password = TEXT("pw1");

		Config.Sim.TunnelLengthFeet = 150.5f;
		Config.Sim.bAutoConveyorInterlockWire = false;
		Config.Sim.AntiCollisionPadPositionFeet = 99.75f;

		// Inputs: one trigger (inverted, debounced), one plain, one debounced.
		{
			FMLInputConfig In;
			In.Channel = 1;
			In.Type = EMLInputType::Trigger;
			In.Description = TEXT("Trigger Button");
			In.bInverted = true;
			In.DebounceSeconds = 0.5f;
			In.TriggerServiceNumber = 9;
			Config.Inputs.Add(In);
		}
		{
			FMLInputConfig In;
			In.Channel = 3;
			In.Type = EMLInputType::Entry;
			In.Description = TEXT("Photo Eyes");
			Config.Inputs.Add(In);
		}
		{
			FMLInputConfig In;
			In.Channel = 2;
			In.Type = EMLInputType::Conveyor;
			In.Description = TEXT("Interlock");
			In.DebounceSeconds = 0.75f;
			Config.Inputs.Add(In);
		}

		// Relays: one fully-loaded Normal relay with two modifiers + interlocks,
		// one Roller-type relay with a tire function and a mirror bump.
		{
			FMLRelayConfig Relay;
			Relay.RelayNumber = 6;
			Relay.bActive = false;
			Relay.Description = TEXT("Wraps");
			Relay.bDefault = true;
			Relay.Type = EMLRelayType::Normal;
			Relay.bInactivityCheck = true;
			Relay.InterlockStartSeconds = 2.5f;
			Relay.InterlockStopSeconds = 1.5f;
			Relay.LookBackFeet = 6.5f;
			Relay.Function.Type = EMLFunctionType::VehicleLength;
			Relay.Function.DevicePositionFeet = 33.25f;
			Relay.Function.TurnOnFeet = 2.25f;
			Relay.Function.TurnOnReference = EMLTurnReference::AfterFront;
			Relay.Function.TurnOffFeet = 3.75f;
			Relay.Function.TurnOffReference = EMLTurnReference::BeforeRear;
			FMLModifierConfig Bump;
			Bump.Type = EMLModifierType::Bump;
			Bump.StartFeet = 7.f;
			Bump.LengthFeet = 11.f;
			FMLModifierConfig OpenBed;
			OpenBed.Type = EMLModifierType::OpenPickupBed;
			OpenBed.StartFeet = 0.5f;
			OpenBed.LengthFeet = 1.5f;
			Relay.Function.Modifiers = { Bump, OpenBed };
			Config.Relays.Add(Relay);
		}
		{
			FMLRelayConfig Relay;
			Relay.RelayNumber = 1;
			Relay.Description = TEXT("Rollers");
			Relay.Type = EMLRelayType::Roller;
			Relay.Function.Type = EMLFunctionType::FrontTires;
			Relay.Function.DevicePositionFeet = 15.5f;
			Relay.Function.TurnOnFeet = 1.25f;
			Relay.Function.TurnOnReference = EMLTurnReference::BeforeFront;
			Relay.Function.TurnOffFeet = 1.75f;
			Relay.Function.TurnOffReference = EMLTurnReference::AfterFront;
			FMLModifierConfig MirrorBump;
			MirrorBump.Type = EMLModifierType::MirrorBump;
			MirrorBump.StartFeet = 2.5f;
			MirrorBump.LengthFeet = 1.75f;
			Relay.Function.Modifiers.Add(MirrorBump);
			Config.Relays.Add(Relay);
		}

		// Services: Wash (relay list + times), Macro (sub-service list),
		// Command, MomentarilyOff.
		{
			FMLServiceConfig Svc;
			Svc.ServiceNumber = 2;
			Svc.Description = TEXT("Better Wash");
			Svc.Type = EMLServiceType::Wash;
			Svc.RelayNumbers = { 1, 6 };
			Svc.TimeSeconds = 1.5f;
			Svc.DelaySeconds = 0.5f;
			Config.Services.Add(Svc);
		}
		{
			FMLServiceConfig Svc;
			Svc.ServiceNumber = 5;
			Svc.Description = TEXT("Start Macro");
			Svc.Type = EMLServiceType::Macro;
			Svc.MacroServiceNumbers = { 2, 9 };
			Config.Services.Add(Svc);
		}
		{
			FMLServiceConfig Svc;
			Svc.ServiceNumber = 9;
			Svc.Description = TEXT("Roller Command");
			Svc.Type = EMLServiceType::Command;
			Svc.Command = EMLCommand::RollerRequest;
			Config.Services.Add(Svc);
		}
		{
			FMLServiceConfig Svc;
			Svc.ServiceNumber = 6;
			Svc.Description = TEXT("Pause Wraps");
			Svc.Type = EMLServiceType::MomentarilyOff;
			Svc.RelayNumbers = { 6 };
			Svc.TimeSeconds = 2.5f;
			Svc.DelaySeconds = 1.25f;
			Config.Services.Add(Svc);
		}

		return Config;
	}

	void ExpectConfigsEqual(FAutomationTestBase& Test, const FMLControllerConfig& A, const FMLControllerConfig& B)
	{
		// Communications
		Test.TestEqual(TEXT("NumInputBoards"), B.Communications.NumInputBoards, A.Communications.NumInputBoards);
		Test.TestEqual(TEXT("NumRelayBoards"), B.Communications.NumRelayBoards, A.Communications.NumRelayBoards);
		Test.TestEqual(TEXT("InputBoardPort"), B.Communications.InputBoardPort, A.Communications.InputBoardPort);
		Test.TestEqual(TEXT("OutputBoardPort"), B.Communications.OutputBoardPort, A.Communications.OutputBoardPort);
		Test.TestEqual(TEXT("KeypadPort"), B.Communications.KeypadPort, A.Communications.KeypadPort);
		Test.TestTrue(TEXT("bExitDoorEnabled"), B.Communications.bExitDoorEnabled == A.Communications.bExitDoorEnabled);
		Test.TestEqual(TEXT("SonarAddress"), B.Communications.SonarAddress, A.Communications.SonarAddress);

		// Conveyor
		Test.TestEqual(TEXT("InchesPerSecond"), B.Conveyor.InchesPerSecond, A.Conveyor.InchesPerSecond, FloatTolerance);
		Test.TestEqual(TEXT("InchesPerPulse"), B.Conveyor.InchesPerPulse, A.Conveyor.InchesPerPulse, FloatTolerance);
		Test.TestEqual(TEXT("OnActivationService"), B.Conveyor.OnActivationService, A.Conveyor.OnActivationService);
		Test.TestEqual(TEXT("ShutOffService"), B.Conveyor.ShutOffService, A.Conveyor.ShutOffService);
		Test.TestEqual(TEXT("InactivityTimeoutSeconds"), B.Conveyor.InactivityTimeoutSeconds, A.Conveyor.InactivityTimeoutSeconds, FloatTolerance);
		Test.TestEqual(TEXT("HornTimeSeconds"), B.Conveyor.HornTimeSeconds, A.Conveyor.HornTimeSeconds, FloatTolerance);
		Test.TestEqual(TEXT("HornDelaySeconds"), B.Conveyor.HornDelaySeconds, A.Conveyor.HornDelaySeconds, FloatTolerance);

		// Anti-collision
		Test.TestEqual(TEXT("AC RelayNumber"), B.AntiCollision.RelayNumber, A.AntiCollision.RelayNumber);
		Test.TestEqual(TEXT("AC AfterClearsActivateService"), B.AntiCollision.AfterClearsActivateService, A.AntiCollision.AfterClearsActivateService);
		Test.TestEqual(TEXT("AC SlowDownService"), B.AntiCollision.SlowDownService, A.AntiCollision.SlowDownService);
		Test.TestEqual(TEXT("AC SlowDownTimeSeconds"), B.AntiCollision.SlowDownTimeSeconds, A.AntiCollision.SlowDownTimeSeconds, FloatTolerance);
		Test.TestEqual(TEXT("AC SlowDownHornService"), B.AntiCollision.SlowDownHornService, A.AntiCollision.SlowDownHornService);

		// Roller/Defaults
		Test.TestEqual(TEXT("MinCarLengthFeet"), B.RollerDefaults.MinCarLengthFeet, A.RollerDefaults.MinCarLengthFeet, FloatTolerance);
		Test.TestEqual(TEXT("MaxCarLengthFeet"), B.RollerDefaults.MaxCarLengthFeet, A.RollerDefaults.MaxCarLengthFeet, FloatTolerance);
		Test.TestEqual(TEXT("AverageCarLengthFeet"), B.RollerDefaults.AverageCarLengthFeet, A.RollerDefaults.AverageCarLengthFeet, FloatTolerance);
		Test.TestEqual(TEXT("RollerMode"), static_cast<int32>(B.RollerDefaults.RollerMode), static_cast<int32>(A.RollerDefaults.RollerMode));
		Test.TestEqual(TEXT("UpFeet"), B.RollerDefaults.UpFeet, A.RollerDefaults.UpFeet, FloatTolerance);
		Test.TestEqual(TEXT("DownFeet"), B.RollerDefaults.DownFeet, A.RollerDefaults.DownFeet, FloatTolerance);
		Test.TestEqual(TEXT("UpAgainFeet"), B.RollerDefaults.UpAgainFeet, A.RollerDefaults.UpAgainFeet, FloatTolerance);
		Test.TestTrue(TEXT("bNeedsCarQueuedForRollerRequest"), B.RollerDefaults.bNeedsCarQueuedForRollerRequest == A.RollerDefaults.bNeedsCarQueuedForRollerRequest);
		Test.TestEqual(TEXT("DefaultInputDebounceSeconds"), B.RollerDefaults.DefaultInputDebounceSeconds, A.RollerDefaults.DefaultInputDebounceSeconds, FloatTolerance);
		Test.TestEqual(TEXT("QueueMode"), static_cast<int32>(B.RollerDefaults.QueueMode), static_cast<int32>(A.RollerDefaults.QueueMode));
		Test.TestEqual(TEXT("ServiceOnQueued"), B.RollerDefaults.ServiceOnQueued, A.RollerDefaults.ServiceOnQueued);
		Test.TestEqual(TEXT("DefaultWashService"), B.RollerDefaults.DefaultWashService, A.RollerDefaults.DefaultWashService);

		// Security
		Test.TestTrue(TEXT("bRequirePinForRelayOverride"), B.Security.bRequirePinForRelayOverride == A.Security.bRequirePinForRelayOverride);
		Test.TestEqual(TEXT("PinCode"), B.Security.PinCode, A.Security.PinCode);
		Test.TestEqual(TEXT("UiPassword"), B.Security.UiPassword, A.Security.UiPassword);
		Test.TestEqual(TEXT("Username"), B.Security.Username, A.Security.Username);
		Test.TestEqual(TEXT("Password"), B.Security.Password, A.Security.Password);

		// Sim
		Test.TestEqual(TEXT("TunnelLengthFeet"), B.Sim.TunnelLengthFeet, A.Sim.TunnelLengthFeet, FloatTolerance);
		Test.TestTrue(TEXT("bAutoConveyorInterlockWire"), B.Sim.bAutoConveyorInterlockWire == A.Sim.bAutoConveyorInterlockWire);
		Test.TestEqual(TEXT("AntiCollisionPadPositionFeet"), B.Sim.AntiCollisionPadPositionFeet, A.Sim.AntiCollisionPadPositionFeet, FloatTolerance);

		// Inputs
		Test.TestEqual(TEXT("Input count"), B.Inputs.Num(), A.Inputs.Num());
		for (int32 Index = 0; Index < FMath::Min(A.Inputs.Num(), B.Inputs.Num()); ++Index)
		{
			const FMLInputConfig& InA = A.Inputs[Index];
			const FMLInputConfig& InB = B.Inputs[Index];
			const FString Ctx = FString::Printf(TEXT("Input[%d] "), Index);
			Test.TestEqual(*(Ctx + TEXT("Channel")), InB.Channel, InA.Channel);
			Test.TestEqual(*(Ctx + TEXT("Type")), static_cast<int32>(InB.Type), static_cast<int32>(InA.Type));
			Test.TestEqual(*(Ctx + TEXT("Description")), InB.Description, InA.Description);
			Test.TestTrue(*(Ctx + TEXT("bInverted")), InB.bInverted == InA.bInverted);
			Test.TestEqual(*(Ctx + TEXT("DebounceSeconds")), InB.DebounceSeconds, InA.DebounceSeconds, FloatTolerance);
			Test.TestEqual(*(Ctx + TEXT("TriggerServiceNumber")), InB.TriggerServiceNumber, InA.TriggerServiceNumber);
		}

		// Relays
		Test.TestEqual(TEXT("Relay count"), B.Relays.Num(), A.Relays.Num());
		for (int32 Index = 0; Index < FMath::Min(A.Relays.Num(), B.Relays.Num()); ++Index)
		{
			const FMLRelayConfig& RA = A.Relays[Index];
			const FMLRelayConfig& RB = B.Relays[Index];
			const FString Ctx = FString::Printf(TEXT("Relay[%d] "), Index);
			Test.TestEqual(*(Ctx + TEXT("RelayNumber")), RB.RelayNumber, RA.RelayNumber);
			Test.TestTrue(*(Ctx + TEXT("bActive")), RB.bActive == RA.bActive);
			Test.TestEqual(*(Ctx + TEXT("Description")), RB.Description, RA.Description);
			Test.TestTrue(*(Ctx + TEXT("bDefault")), RB.bDefault == RA.bDefault);
			Test.TestEqual(*(Ctx + TEXT("Type")), static_cast<int32>(RB.Type), static_cast<int32>(RA.Type));
			Test.TestTrue(*(Ctx + TEXT("bInactivityCheck")), RB.bInactivityCheck == RA.bInactivityCheck);
			Test.TestEqual(*(Ctx + TEXT("InterlockStartSeconds")), RB.InterlockStartSeconds, RA.InterlockStartSeconds, FloatTolerance);
			Test.TestEqual(*(Ctx + TEXT("InterlockStopSeconds")), RB.InterlockStopSeconds, RA.InterlockStopSeconds, FloatTolerance);
			Test.TestEqual(*(Ctx + TEXT("LookBackFeet")), RB.LookBackFeet, RA.LookBackFeet, FloatTolerance);
			Test.TestEqual(*(Ctx + TEXT("Function.Type")), static_cast<int32>(RB.Function.Type), static_cast<int32>(RA.Function.Type));
			Test.TestEqual(*(Ctx + TEXT("Function.DevicePositionFeet")), RB.Function.DevicePositionFeet, RA.Function.DevicePositionFeet, FloatTolerance);
			Test.TestEqual(*(Ctx + TEXT("Function.TurnOnFeet")), RB.Function.TurnOnFeet, RA.Function.TurnOnFeet, FloatTolerance);
			Test.TestEqual(*(Ctx + TEXT("Function.TurnOnReference")), static_cast<int32>(RB.Function.TurnOnReference), static_cast<int32>(RA.Function.TurnOnReference));
			Test.TestEqual(*(Ctx + TEXT("Function.TurnOffFeet")), RB.Function.TurnOffFeet, RA.Function.TurnOffFeet, FloatTolerance);
			Test.TestEqual(*(Ctx + TEXT("Function.TurnOffReference")), static_cast<int32>(RB.Function.TurnOffReference), static_cast<int32>(RA.Function.TurnOffReference));
			Test.TestEqual(*(Ctx + TEXT("Modifier count")), RB.Function.Modifiers.Num(), RA.Function.Modifiers.Num());
			for (int32 ModIndex = 0; ModIndex < FMath::Min(RA.Function.Modifiers.Num(), RB.Function.Modifiers.Num()); ++ModIndex)
			{
				const FMLModifierConfig& MA = RA.Function.Modifiers[ModIndex];
				const FMLModifierConfig& MB = RB.Function.Modifiers[ModIndex];
				const FString ModCtx = FString::Printf(TEXT("Relay[%d].Modifier[%d] "), Index, ModIndex);
				Test.TestEqual(*(ModCtx + TEXT("Type")), static_cast<int32>(MB.Type), static_cast<int32>(MA.Type));
				Test.TestEqual(*(ModCtx + TEXT("StartFeet")), MB.StartFeet, MA.StartFeet, FloatTolerance);
				Test.TestEqual(*(ModCtx + TEXT("LengthFeet")), MB.LengthFeet, MA.LengthFeet, FloatTolerance);
			}
		}

		// Services
		Test.TestEqual(TEXT("Service count"), B.Services.Num(), A.Services.Num());
		for (int32 Index = 0; Index < FMath::Min(A.Services.Num(), B.Services.Num()); ++Index)
		{
			const FMLServiceConfig& SA = A.Services[Index];
			const FMLServiceConfig& SB = B.Services[Index];
			const FString Ctx = FString::Printf(TEXT("Service[%d] "), Index);
			Test.TestEqual(*(Ctx + TEXT("ServiceNumber")), SB.ServiceNumber, SA.ServiceNumber);
			Test.TestEqual(*(Ctx + TEXT("Description")), SB.Description, SA.Description);
			Test.TestEqual(*(Ctx + TEXT("Type")), static_cast<int32>(SB.Type), static_cast<int32>(SA.Type));
			Test.TestTrue(*(Ctx + TEXT("RelayNumbers")), SB.RelayNumbers == SA.RelayNumbers);
			Test.TestEqual(*(Ctx + TEXT("TimeSeconds")), SB.TimeSeconds, SA.TimeSeconds, FloatTolerance);
			Test.TestEqual(*(Ctx + TEXT("DelaySeconds")), SB.DelaySeconds, SA.DelaySeconds, FloatTolerance);
			Test.TestTrue(*(Ctx + TEXT("MacroServiceNumbers")), SB.MacroServiceNumbers == SA.MacroServiceNumbers);
			Test.TestEqual(*(Ctx + TEXT("Command")), static_cast<int32>(SB.Command), static_cast<int32>(SA.Command));
		}
	}
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConfigXmlRoundTripTest,
	"MicrologicSim.ConfigIO.XmlRoundTripsFullyPopulatedConfig", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConfigXmlRoundTripTest::RunTest(const FString& Parameters)
{
	const FMLControllerConfig Original = MakeFullyPopulatedConfig();

	const FString Xml = MicrologicXml::ConfigToXml(Original);
	TestTrue(TEXT("Serializer produced XML"), !Xml.IsEmpty());

	FMLControllerConfig Restored;
	TestTrue(TEXT("Round-trip parse succeeded"), MicrologicXml::ConfigFromXml(Xml, Restored));

	ExpectConfigsEqual(*this, Original, Restored);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConfigXmlMalformedTest,
	"MicrologicSim.ConfigIO.MalformedXmlReturnsFalse", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConfigXmlMalformedTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Out;

	TestFalse(TEXT("Truncated document rejected"),
		MicrologicXml::ConfigFromXml(TEXT("<MicrologicConfig><Communications"), Out));
	TestFalse(TEXT("Non-XML text rejected"),
		MicrologicXml::ConfigFromXml(TEXT("this is not xml at all"), Out));
	TestFalse(TEXT("Wrong root tag rejected"),
		MicrologicXml::ConfigFromXml(TEXT("<?xml version=\"1.0\"?><SomeOtherRoot></SomeOtherRoot>"), Out));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConfigXmlEmptyTest,
	"MicrologicSim.ConfigIO.EmptyStringReturnsFalse", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConfigXmlEmptyTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Out;
	TestFalse(TEXT("Empty string rejected"), MicrologicXml::ConfigFromXml(FString(), Out));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConfigXmlEscapingTest,
	"MicrologicSim.ConfigIO.SpecialCharactersSurviveRoundTrip", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConfigXmlEscapingTest::RunTest(const FString& Parameters)
{
	const FString Nasty = TEXT("Foam <\"Extra\"> & 'Wax'");

	FMLControllerConfig Config;
	{
		FMLInputConfig In;
		In.Channel = 1;
		In.Type = EMLInputType::None;
		In.Description = Nasty;
		Config.Inputs.Add(In);
	}
	{
		FMLRelayConfig Relay;
		Relay.RelayNumber = 1;
		Relay.Description = Nasty;
		Config.Relays.Add(Relay);
	}
	{
		FMLServiceConfig Svc;
		Svc.ServiceNumber = 1;
		Svc.Type = EMLServiceType::Wash;
		Svc.Description = Nasty;
		Config.Services.Add(Svc);
	}

	const FString Xml = MicrologicXml::ConfigToXml(Config);
	TestFalse(TEXT("Raw special characters were escaped in the document"),
		Xml.Contains(TEXT("Foam <\"Extra\"")));

	FMLControllerConfig Restored;
	TestTrue(TEXT("Escaped document parses"), MicrologicXml::ConfigFromXml(Xml, Restored));
	TestEqual(TEXT("Restored input count"), Restored.Inputs.Num(), 1);
	TestEqual(TEXT("Restored relay count"), Restored.Relays.Num(), 1);
	TestEqual(TEXT("Restored service count"), Restored.Services.Num(), 1);

	if (Restored.Inputs.Num() == 1 && Restored.Relays.Num() == 1 && Restored.Services.Num() == 1)
	{
		TestTrue(TEXT("Input description survived escaping"),
			Restored.Inputs[0].Description.Equals(Nasty, ESearchCase::CaseSensitive));
		TestTrue(TEXT("Relay description survived escaping"),
			Restored.Relays[0].Description.Equals(Nasty, ESearchCase::CaseSensitive));
		TestTrue(TEXT("Service description survived escaping"),
			Restored.Services[0].Description.Equals(Nasty, ESearchCase::CaseSensitive));
	}
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConfigFactoryConsistencyTest,
	"MicrologicSim.ConfigIO.FactoryDefaultConfigIsInternallyConsistent", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConfigFactoryConsistencyTest::RunTest(const FString& Parameters)
{
	const FMLControllerConfig Config = UMicrologicControllerSubsystem::MakeFactoryDefaultConfig();

	const int32 MaxInputChannel = Config.Communications.NumInputBoards * 16;
	const int32 MaxRelayNumber = Config.Communications.NumRelayBoards * 8;

	// Relay numbers unique and within the board count.
	TSet<int32> RelayNumbers;
	for (const FMLRelayConfig& Relay : Config.Relays)
	{
		TestTrue(FString::Printf(TEXT("Relay %d within 1..%d"), Relay.RelayNumber, MaxRelayNumber),
			Relay.RelayNumber >= 1 && Relay.RelayNumber <= MaxRelayNumber);
		TestFalse(FString::Printf(TEXT("Relay %d not duplicated"), Relay.RelayNumber),
			RelayNumbers.Contains(Relay.RelayNumber));
		RelayNumbers.Add(Relay.RelayNumber);
	}

	// Input channels unique and within the board count.
	TSet<int32> InputChannels;
	for (const FMLInputConfig& Input : Config.Inputs)
	{
		TestTrue(FString::Printf(TEXT("Input channel %d within 1..%d"), Input.Channel, MaxInputChannel),
			Input.Channel >= 1 && Input.Channel <= MaxInputChannel);
		TestFalse(FString::Printf(TEXT("Input channel %d not duplicated"), Input.Channel),
			InputChannels.Contains(Input.Channel));
		InputChannels.Add(Input.Channel);
	}

	// Service numbers unique; collect for reference checks.
	TSet<int32> ServiceNumbers;
	for (const FMLServiceConfig& Service : Config.Services)
	{
		TestFalse(FString::Printf(TEXT("Service %d not duplicated"), Service.ServiceNumber),
			ServiceNumbers.Contains(Service.ServiceNumber));
		ServiceNumbers.Add(Service.ServiceNumber);
	}

	// Every relay a service touches must exist; every macro sub-service must exist.
	for (const FMLServiceConfig& Service : Config.Services)
	{
		for (const int32 Relay : Service.RelayNumbers)
		{
			TestTrue(FString::Printf(TEXT("Service %d references existing relay %d"), Service.ServiceNumber, Relay),
				RelayNumbers.Contains(Relay));
		}
		for (const int32 Sub : Service.MacroServiceNumbers)
		{
			TestTrue(FString::Printf(TEXT("Macro service %d references existing service %d"), Service.ServiceNumber, Sub),
				ServiceNumbers.Contains(Sub));
		}
	}

	// Trigger inputs must point at existing services.
	for (const FMLInputConfig& Input : Config.Inputs)
	{
		if (Input.Type == EMLInputType::Trigger && Input.TriggerServiceNumber != 0)
		{
			TestTrue(FString::Printf(TEXT("Trigger input ch %d references existing service %d"), Input.Channel, Input.TriggerServiceNumber),
				ServiceNumbers.Contains(Input.TriggerServiceNumber));
		}
	}

	// Designated service buttons must exist.
	auto CheckServiceRef = [this, &ServiceNumbers](const TCHAR* What, int32 ServiceNumber)
	{
		if (ServiceNumber != 0)
		{
			TestTrue(FString::Printf(TEXT("%s (service %d) exists in Services"), What, ServiceNumber),
				ServiceNumbers.Contains(ServiceNumber));
		}
	};
	CheckServiceRef(TEXT("Conveyor On Activation Button"), Config.Conveyor.OnActivationService);
	CheckServiceRef(TEXT("Conveyor Shut Off Button"), Config.Conveyor.ShutOffService);
	CheckServiceRef(TEXT("After Anti-Collision Clears Activate Button"), Config.AntiCollision.AfterClearsActivateService);
	CheckServiceRef(TEXT("Anti-Collision Slow Down service"), Config.AntiCollision.SlowDownService);
	CheckServiceRef(TEXT("Anti-Collision Slow Down Horn service"), Config.AntiCollision.SlowDownHornService);
	CheckServiceRef(TEXT("Service On Queued"), Config.RollerDefaults.ServiceOnQueued);
	CheckServiceRef(TEXT("Default Wash If None Programmed"), Config.RollerDefaults.DefaultWashService);

	// The anti-collision ghost relay must exist.
	if (Config.AntiCollision.RelayNumber != 0)
	{
		TestTrue(FString::Printf(TEXT("Anti-collision relay %d exists in Relays"), Config.AntiCollision.RelayNumber),
			RelayNumbers.Contains(Config.AntiCollision.RelayNumber));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
