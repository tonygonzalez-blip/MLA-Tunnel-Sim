// Copyright Micrologic Associates. All Rights Reserved.
//
// Service-type tests: Wash, Service, De-programmable, Turn ON/OFF cascades,
// Override, Momentarily On/Off, Toggler, Macro, Command, and rejection of
// undefined services.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.h"
#include "MicrologicTestHelpers.h"

using namespace MicrologicTest;

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceWashTest,
	"MicrologicSim.Service.WashAddsItsRelaysPlusDefaults", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceWashTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f); // default
	AddFunctionRelay(Config, 11, EMLFunctionType::VehicleLength, 20.f,
		0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, /*bDefault=*/false);
	AddService(Config, 40, EMLServiceType::Wash, { 11 }, TEXT("Better Wash"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	Core.SendOrder({ 40 });
	StartConveyor(Core);
	RunCarIn(Core, 8.f);

	TestEqual(TEXT("One car"), Core.GetCars().Num(), 1);
	if (Core.GetCars().Num() == 1)
	{
		const FMLCarState& Car = Core.GetCars()[0];
		TestTrue(TEXT("Wash programmed its non-default relay"), Car.ProgrammedRelays.Contains(11));
		TestTrue(TEXT("Default relay rides along with every wash"), Car.ProgrammedRelays.Contains(10));
	}
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceDeprogrammableTest,
	"MicrologicSim.Service.DeprogrammableRemovesDefaultForThatCarOnly", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceDeprogrammableTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f); // default
	AddService(Config, 41, EMLServiceType::Deprogrammable, { 10 }, TEXT("Retract 10"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);

	// Car 1 carries the de-programmable service.
	Core.SendOrder({ 41 });
	RunCarIn(Core, 8.f);
	// Car 2 has no order.
	Pump(Core, 2.f);
	RunCarIn(Core, 8.f);

	TestEqual(TEXT("Two cars"), Core.GetCars().Num(), 2);
	if (Core.GetCars().Num() == 2)
	{
		TestFalse(TEXT("Deprogrammed car: default relay removed"),
			Core.GetCars()[0].ProgrammedRelays.Contains(10));
		TestTrue(TEXT("Next car unaffected: default relay back"),
			Core.GetCars()[1].ProgrammedRelays.Contains(10));
	}
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceAddOnTest,
	"MicrologicSim.Service.ServiceTypeAddsRelayToWash", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceAddOnTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 11, EMLFunctionType::VehicleLength, 20.f,
		0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, /*bDefault=*/false);
	AddService(Config, 42, EMLServiceType::Service, { 11 }, TEXT("A-La-Carte"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	Core.SendOrder({ 42 });
	StartConveyor(Core);
	RunCarIn(Core, 8.f);

	TestTrue(TEXT("Service type added its relay to the car"),
		Core.GetCars().Num() == 1 && Core.GetCars()[0].ProgrammedRelays.Contains(11));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceTurnOffCascadeTest,
	"MicrologicSim.Service.TurnOffSuppressesRelayAndAllAfterIt", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceTurnOffCascadeTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f); // defaults
	AddFunctionRelay(Config, 11, EMLFunctionType::VehicleLength, 20.f);
	AddFunctionRelay(Config, 12, EMLFunctionType::VehicleLength, 20.f);
	AddService(Config, 43, EMLServiceType::TurnOff, { 11 }, TEXT("Turn OFF from 11"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	Core.SendOrder({ 43 });
	StartConveyor(Core);
	RunCarIn(Core, 8.f);
	PumpToFront(Core, 0, 21.f); // inside every window [20,28)

	TestTrue(TEXT("Relay below the cascade (10) still fires"), Core.GetRelayPhysical(10));
	TestFalse(TEXT("Selected relay (11) turned off for the wash"), Core.GetRelayPhysical(11));
	TestFalse(TEXT("Higher-numbered relay (12) turned off too"), Core.GetRelayPhysical(12));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceTurnOnCascadeTest,
	"MicrologicSim.Service.TurnOnForcesRelayAndAllAfterIt", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceTurnOnCascadeTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	// All non-default: nothing would fire without the cascade.
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f,
		0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, false);
	AddFunctionRelay(Config, 11, EMLFunctionType::VehicleLength, 20.f,
		0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, false);
	AddFunctionRelay(Config, 12, EMLFunctionType::VehicleLength, 20.f,
		0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, false);
	AddService(Config, 44, EMLServiceType::TurnOn, { 11 }, TEXT("Turn ON from 11"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	Core.SendOrder({ 44 });
	StartConveyor(Core);
	RunCarIn(Core, 8.f);
	PumpToFront(Core, 0, 21.f); // inside every window [20,28)

	TestFalse(TEXT("Relay below the cascade (10) stays off"), Core.GetRelayPhysical(10));
	TestTrue(TEXT("Selected relay (11) forced on for the wash"), Core.GetRelayPhysical(11));
	TestTrue(TEXT("Higher-numbered relay (12) forced on too"), Core.GetRelayPhysical(12));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceOverrideTest,
	"MicrologicSim.Service.OverrideForcesFullVehicleLength", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceOverrideTest::RunTest(const FString& Parameters)
{
	// The relay's own window is booby-trapped: turn-on 2 ft AFTER the front
	// plus a Bump that suppresses the first 100 ft — via normal window logic
	// it would never fire. Override must force it for the entire vehicle
	// length (front >= D && rear < D), ignoring offsets and modifiers.
	FMLControllerConfig Config = MakeTestConfig();
	FMLRelayConfig& Relay = AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f,
		2.f, EMLTurnReference::AfterFront, 0.f, EMLTurnReference::AfterRear, /*bDefault=*/false);
	FMLModifierConfig Bump;
	Bump.Type = EMLModifierType::Bump;
	Bump.StartFeet = 0.f;
	Bump.LengthFeet = 100.f;
	Relay.Function.Modifiers.Add(Bump);
	AddService(Config, 45, EMLServiceType::Override, { 10 }, TEXT("Override 10"));
	AddService(Config, 46, EMLServiceType::Service, { 10 }, TEXT("Program 10"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);

	// Car 1: overridden.
	Core.SendOrder({ 45 });
	RunCarIn(Core, 8.f);
	TestTrue(TEXT("Override recorded on the car"),
		Core.GetCars().Num() == 1 && Core.GetCars()[0].OverrideRelays.Contains(10));
	PumpToFront(Core, 0, 20.f);
	TestTrue(TEXT("front=20: on from the moment the front reaches the device"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 21.f);
	TestTrue(TEXT("front=21: on despite the always-suppressing Bump"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 27.f);
	TestTrue(TEXT("front=27 (rear=19): on for the entire vehicle"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 28.f);
	TestFalse(TEXT("front=28 (rear=20): off once the rear passes the device"), Core.GetRelayPhysical(10));

	// Car 2: programmed normally -> window logic (offsets + bump) applies and
	// keeps it off.
	Core.SendOrder({ 46 });
	RunCarIn(Core, 8.f);
	PumpToFront(Core, 1, 23.f);
	TestFalse(TEXT("Non-overridden car: modifiers still suppress the relay"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceMomentarilyOnTest,
	"MicrologicSim.Service.MomentarilyOnDelayThenTimedPulse", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceMomentarilyOnTest::RunTest(const FString& Parameters)
{
	// Delay 1 s, Time 2 s, conveyor stopped throughout: momentaries run on
	// wall-clock time, not pulses.
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 14, EMLRelayType::Normal, false, TEXT("Wetdown"));
	FMLServiceConfig& Service = AddService(Config, 46, EMLServiceType::MomentarilyOn, { 14 }, TEXT("Wetdown"));
	Service.DelaySeconds = 1.f;
	Service.TimeSeconds = 2.f;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	TestTrue(TEXT("Momentary service accepted"), Core.ExecuteService(46));
	Pump(Core, 0.5f); // t=0.5: inside the delay
	TestFalse(TEXT("t=0.5: relay off during the 1 s delay"), Core.GetRelayPhysical(14));
	Pump(Core, 1.0f); // t=1.5: pulse active
	TestTrue(TEXT("t=1.5: relay on after the delay"), Core.GetRelayPhysical(14));
	Pump(Core, 1.0f); // t=2.5: still inside the 2 s
	TestTrue(TEXT("t=2.5: relay still on"), Core.GetRelayPhysical(14));
	Pump(Core, 1.0f); // t=3.5: past delay + time = 3 s
	TestFalse(TEXT("t=3.5: relay off after the timed pulse"), Core.GetRelayPhysical(14));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceMomentarilyOffTest,
	"MicrologicSim.Service.MomentarilyOffForcesOnRelayOff", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceMomentarilyOffTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 14, EMLRelayType::Normal, false, TEXT("Arch"));
	AddService(Config, 47, EMLServiceType::Toggler, { 14 }, TEXT("Arch Toggle"));
	FMLServiceConfig& Service = AddService(Config, 48, EMLServiceType::MomentarilyOff, { 14 }, TEXT("Arch Pause"));
	Service.TimeSeconds = 2.f;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.ExecuteService(47); // hold the relay on
	TestTrue(TEXT("Precondition: relay on via toggler"), Core.GetRelayPhysical(14));

	Core.ExecuteService(48);
	Pump(Core, 0.5f); // t=0.5: forced off
	TestFalse(TEXT("t=0.5: MomentarilyOff forces the relay off"), Core.GetRelayPhysical(14));
	Pump(Core, 1.0f); // t=1.5: still inside the 2 s
	TestFalse(TEXT("t=1.5: still forced off"), Core.GetRelayPhysical(14));
	Pump(Core, 1.0f); // t=2.5: momentary expired
	TestTrue(TEXT("t=2.5: relay returns to its on state"), Core.GetRelayPhysical(14));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceTogglerTest,
	"MicrologicSim.Service.TogglerFlipsPersistentState", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceTogglerTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 14, EMLRelayType::Normal, false, TEXT("Prep Gun"));
	AddService(Config, 47, EMLServiceType::Toggler, { 14 }, TEXT("Prep Gun Toggle"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	TestFalse(TEXT("Initially off"), Core.GetRelayPhysical(14));
	Core.ExecuteService(47);
	TestTrue(TEXT("First execute toggles on"), Core.IsRelayToggledOn(14));
	TestTrue(TEXT("Relay physically on"), Core.GetRelayPhysical(14));

	Pump(Core, 3.f); // state persists over time
	TestTrue(TEXT("Toggled state persists"), Core.GetRelayPhysical(14));

	Core.ExecuteService(47);
	TestFalse(TEXT("Second execute toggles off"), Core.IsRelayToggledOn(14));
	TestFalse(TEXT("Relay physically off"), Core.GetRelayPhysical(14));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceMacroTest,
	"MicrologicSim.Service.MacroExecutesItsSubServices", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceMacroTest::RunTest(const FString& Parameters)
{
	// Cheat sheet example: a macro can start the conveyor and call a roller.
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 16, EMLRelayType::Roller, false, TEXT("Rollers"));
	FMLServiceConfig& RollerCmd = AddService(Config, 50, EMLServiceType::Command, TArray<int32>(), TEXT("Roller Up"));
	RollerCmd.Command = EMLCommand::RollerRequest;
	FMLServiceConfig& Macro = AddService(Config, 49, EMLServiceType::Macro, TArray<int32>(), TEXT("Start + Roller"));
	Macro.MacroServiceNumbers = { StartServiceNumber, 50 };

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	TestTrue(TEXT("Macro accepted"), Core.ExecuteService(49));
	TestTrue(TEXT("Sub-service 1: conveyor started"), Core.GetConveyorState() == EMLConveyorState::Running);
	TestTrue(TEXT("Sub-service 2: roller sequence requested"), Core.IsRollerSequenceActive());
	TestTrue(TEXT("Roller relay up (phase 1 of the sequence)"), Core.GetRelayPhysical(16));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceCommandTest,
	"MicrologicSim.Service.CommandServiceTriggersRollerRequest", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceCommandTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 16, EMLRelayType::Roller, false, TEXT("Rollers"));
	FMLServiceConfig& RollerCmd = AddService(Config, 50, EMLServiceType::Command, TArray<int32>(), TEXT("Roller Up"));
	RollerCmd.Command = EMLCommand::RollerRequest;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	TestFalse(TEXT("No roller sequence before the command"), Core.IsRollerSequenceActive());
	TestTrue(TEXT("Command service accepted"), Core.ExecuteService(50));
	TestTrue(TEXT("Roller sequence started by the command service"), Core.IsRollerSequenceActive());
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicServiceUndefinedTest,
	"MicrologicSim.Service.UndefinedServiceNumberRejected", MICROLOGIC_TEST_FLAGS)
bool FMicrologicServiceUndefinedTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeTestConfig(), true);

	TestFalse(TEXT("ExecuteService on an undefined number returns false"), Core.ExecuteService(999));
	TestTrue(TEXT("Controller state untouched"), Core.GetConveyorState() == EMLConveyorState::Stopped);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
