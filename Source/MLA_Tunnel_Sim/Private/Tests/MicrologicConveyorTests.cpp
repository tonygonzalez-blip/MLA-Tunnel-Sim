// Copyright Micrologic Associates. All Rights Reserved.
//
// Conveyor state-machine tests: start/horn sequence, stop circuits, stall,
// exit door, conveyor-type relays, and the inactivity timeout.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.h"
#include "MicrologicTestHelpers.h"

using namespace MicrologicTest;

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorStartNoHornTest,
	"MicrologicSim.Conveyor.StartRunsImmediatelyWithoutHorn", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorStartNoHornTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeTestConfig(), /*bReload=*/true);

	TestTrue(TEXT("Conveyor boots Stopped"), Core.GetConveyorState() == EMLConveyorState::Stopped);

	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("No horn configured: Running immediately after the On Activation service"),
		Core.GetConveyorState() == EMLConveyorState::Running);

	Core.ExecuteService(StopServiceNumber);
	TestTrue(TEXT("Shut Off service stops the conveyor"), Core.GetConveyorState() == EMLConveyorState::Stopped);

	Core.PressStartCircuit(1);
	TestTrue(TEXT("START circuit press: Running immediately with no horn"),
		Core.GetConveyorState() == EMLConveyorState::Running);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorHornSequenceTest,
	"MicrologicSim.Conveyor.HornDelayThenHornThenRunning", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorHornSequenceTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	Config.Conveyor.HornDelaySeconds = 1.f;
	Config.Conveyor.HornTimeSeconds = 2.f;
	AddRelay(Config, 10, EMLRelayType::Horn, false, TEXT("Horn"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("t=0: HornDelay state"), Core.GetConveyorState() == EMLConveyorState::HornDelay);
	TestFalse(TEXT("t=0: horn relay off during delay"), Core.GetRelayPhysical(10));
	TestFalse(TEXT("t=0: conveyor not moving during delay"), Core.IsConveyorRunning());

	Pump(Core, 0.5f); // t = 0.5, still inside the 1 s delay
	TestTrue(TEXT("t=0.5: still HornDelay"), Core.GetConveyorState() == EMLConveyorState::HornDelay);
	TestFalse(TEXT("t=0.5: horn relay still off"), Core.GetRelayPhysical(10));

	Pump(Core, 1.0f); // t = 1.5, Horn began at t = 1
	TestTrue(TEXT("t=1.5: Horn state"), Core.GetConveyorState() == EMLConveyorState::Horn);
	TestTrue(TEXT("t=1.5: horn relay ON during Horn phase"), Core.GetRelayPhysical(10));
	TestFalse(TEXT("t=1.5: conveyor not moving during horn"), Core.IsConveyorRunning());

	Pump(Core, 1.0f); // t = 2.5, still inside the 2 s horn
	TestTrue(TEXT("t=2.5: still Horn"), Core.GetConveyorState() == EMLConveyorState::Horn);
	TestTrue(TEXT("t=2.5: horn relay still ON"), Core.GetRelayPhysical(10));

	Pump(Core, 1.0f); // t = 3.5, Running began at t = 3
	TestTrue(TEXT("t=3.5: Running"), Core.GetConveyorState() == EMLConveyorState::Running);
	TestFalse(TEXT("t=3.5: horn relay off once running"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorStopCircuitTest,
	"MicrologicSim.Conveyor.StopCircuitKillsAndBlocksRestart", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorStopCircuitTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeTestConfig(), true);
	StartConveyor(Core);
	TestTrue(TEXT("Precondition: Running"), Core.GetConveyorState() == EMLConveyorState::Running);

	Core.SetStopCircuit(2, /*bClosed=*/false); // open the NC circuit
	TestTrue(TEXT("Open STOP circuit kills the conveyor instantly"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);
	TestTrue(TEXT("Stop reason is StopCircuit"), Core.GetLastStopReason() == EMLStopReason::StopCircuit);

	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Restart blocked while any STOP circuit is open"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);

	Core.SetStopCircuit(2, true);
	TestTrue(TEXT("Re-closing the circuit does not auto-start"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);

	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Start allowed once the circuit is closed again"),
		Core.GetConveyorState() == EMLConveyorState::Running);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorStallTest,
	"MicrologicSim.Conveyor.StallInputStopsAndBlocks", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorStallTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeTestConfig(), true);
	StartConveyor(Core);
	TestTrue(TEXT("Precondition: Running"), Core.GetConveyorState() == EMLConveyorState::Running);

	Core.SetInputRawByType(EMLInputType::Stall, true);
	TestTrue(TEXT("Stall input stops the conveyor"), Core.GetConveyorState() == EMLConveyorState::Stopped);
	TestTrue(TEXT("Stop reason is Stall"), Core.GetLastStopReason() == EMLStopReason::Stall);

	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Restart blocked while the stall input is engaged"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);

	Core.SetInputRawByType(EMLInputType::Stall, false);
	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Start allowed once the stall input clears"),
		Core.GetConveyorState() == EMLConveyorState::Running);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorExitDoorTest,
	"MicrologicSim.Conveyor.ExitDoorStopsAndGatesStart", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorExitDoorTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	Config.Communications.bExitDoorEnabled = true; // ShutOffService already 21

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	// Door input starts OFF: start must be refused.
	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Start blocked while exit door input is off"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);

	Core.SetInputRawByType(EMLInputType::ExitDoor, true);
	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Start allowed with the exit door input on"),
		Core.GetConveyorState() == EMLConveyorState::Running);

	Core.SetInputRawByType(EMLInputType::ExitDoor, false);
	TestTrue(TEXT("Door input dropping stops the running conveyor"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);
	TestTrue(TEXT("Stop reason is ExitDoor"), Core.GetLastStopReason() == EMLStopReason::ExitDoor);

	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Restart still blocked while the door is off"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);

	Core.SetInputRawByType(EMLInputType::ExitDoor, true);
	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Door back on allows the conveyor again"),
		Core.GetConveyorState() == EMLConveyorState::Running);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorExitDoorNoShutOffTest,
	"MicrologicSim.Conveyor.ExitDoorIgnoredWithoutShutOffService", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorExitDoorNoShutOffTest::RunTest(const FString& Parameters)
{
	// Cheat sheet: the Exit Door feature only works if the Shut Off Button is
	// defined in the Conveyor tab.
	FMLControllerConfig Config = MakeTestConfig();
	Config.Communications.bExitDoorEnabled = true;
	Config.Conveyor.ShutOffService = 0;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	// Door off, but with no Shut Off service the feature is inert.
	Core.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Start allowed with door off when ShutOffService = 0"),
		Core.GetConveyorState() == EMLConveyorState::Running);

	// A door falling edge while running must not stop the conveyor either.
	Core.SetInputRawByType(EMLInputType::ExitDoor, true);
	Core.SetInputRawByType(EMLInputType::ExitDoor, false);
	TestTrue(TEXT("Door falling edge ignored when ShutOffService = 0"),
		Core.GetConveyorState() == EMLConveyorState::Running);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorRelayTypeTest,
	"MicrologicSim.Conveyor.ConveyorTypeRelayFollowsRunningState", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorRelayTypeTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 11, EMLRelayType::Conveyor, false, TEXT("Conveyor Motor Light"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	TestFalse(TEXT("Conveyor relay off while stopped"), Core.GetRelayPhysical(11));

	StartConveyor(Core);
	TestTrue(TEXT("Conveyor relay on while running"), Core.GetRelayPhysical(11));

	Core.ExecuteService(StopServiceNumber);
	TestFalse(TEXT("Conveyor relay off again after stop"), Core.GetRelayPhysical(11));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorInactivityTimeoutTest,
	"MicrologicSim.Conveyor.InactivityTimeoutStopsEmptyTunnel", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorInactivityTimeoutTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	Config.Conveyor.InactivityTimeoutSeconds = 5.f;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core); // ~0.5 s of empty running so far

	Pump(Core, 3.5f); // ~4.0 s empty: below the 5 s timeout
	TestTrue(TEXT("Still running below the inactivity timeout"),
		Core.GetConveyorState() == EMLConveyorState::Running);

	Pump(Core, 2.0f); // ~6.0 s empty: past the timeout
	TestTrue(TEXT("Conveyor auto-stopped after ~5 s of empty tunnel"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);
	TestTrue(TEXT("Stop reason is Inactivity"), Core.GetLastStopReason() == EMLStopReason::Inactivity);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorInactivityCheckRelayTest,
	"MicrologicSim.Conveyor.InactivityCheckRelayBlocksAutoStop", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorInactivityCheckRelayTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	Config.Conveyor.InactivityTimeoutSeconds = 5.f;
	AddRelay(Config, 12, EMLRelayType::Normal, false, TEXT("Prep Gun")).bInactivityCheck = true;
	AddService(Config, 30, EMLServiceType::Toggler, { 12 }, TEXT("Prep Gun Toggle"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.ExecuteService(30); // hold the inactivity-check relay ON
	TestTrue(TEXT("Precondition: inactivity-check relay is on"), Core.GetRelayPhysical(12));

	StartConveyor(Core);
	Pump(Core, 8.0f); // well past the 5 s timeout
	TestTrue(TEXT("Conveyor keeps running while an Inactivity Check relay is on"),
		Core.GetConveyorState() == EMLConveyorState::Running);

	Core.ExecuteService(30); // release the relay
	Pump(Core, 0.5f);
	TestTrue(TEXT("Conveyor stops once the checked relay turns off"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);
	TestTrue(TEXT("Stop reason is Inactivity"), Core.GetLastStopReason() == EMLStopReason::Inactivity);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicConveyorInactivityRestartTest,
	"MicrologicSim.Conveyor.NewOrderRestartsInactivityStoppedConveyor", MICROLOGIC_TEST_FLAGS)
bool FMicrologicConveyorInactivityRestartTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	Config.Conveyor.InactivityTimeoutSeconds = 5.f;
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f, 0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, /*bDefault=*/false);
	AddService(Config, 40, EMLServiceType::Wash, { 10 }, TEXT("Test Wash"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	Pump(Core, 6.0f);
	TestTrue(TEXT("Precondition: stopped for inactivity"),
		Core.GetConveyorState() == EMLConveyorState::Stopped &&
		Core.GetLastStopReason() == EMLStopReason::Inactivity);

	const int32 OrderId = Core.SendOrder({ 40 });
	TestTrue(TEXT("Order accepted"), OrderId != 0);
	TestTrue(TEXT("New order restarts the conveyor (On Activation service fired)"),
		Core.GetConveyorState() == EMLConveyorState::Running);

	// The On Activation button is a MomentarilyOn on relay 23 — it should be
	// pulsing right after the restart.
	Pump(Core, 0.3f);
	TestTrue(TEXT("On Activation momentary relay pulsed by the restart"),
		Core.GetRelayPhysical(StartRelayNumber));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
