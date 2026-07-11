// Copyright Micrologic Associates. All Rights Reserved.
//
// Vehicle measurement and order-queue tests: pulses, min/max car length,
// default wash binding, queue modes, and the conveyor-interlock gate.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.h"
#include "MicrologicTestHelpers.h"

using namespace MicrologicTest;

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasurePulsesOnlyRunningTest,
	"MicrologicSim.Measurement.PulsesOnlyWhileConveyorRuns", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasurePulsesOnlyRunningTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeTestConfig(), true);

	Pump(Core, 3.0f); // stopped: time passes, no travel
	TestEqual(TEXT("No pulses while stopped"), static_cast<int32>(Core.GetTotalPulses()), 0);

	StartConveyor(Core); // 0.5 s of running = 0.5 ft, still under one pulse
	TestEqual(TEXT("No pulse before a full IPP interval elapses"), static_cast<int32>(Core.GetTotalPulses()), 0);

	Pump(Core, 2.0f); // 2.5 ft total -> exactly 2 pulses at 1 pulse/ft
	TestEqual(TEXT("1 pulse per foot while running"), static_cast<int32>(Core.GetTotalPulses()), 2);

	Core.ExecuteService(StopServiceNumber);
	Pump(Core, 3.0f);
	TestEqual(TEXT("No further pulses after stopping"), static_cast<int32>(Core.GetTotalPulses()), 2);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureCarLengthTest,
	"MicrologicSim.Measurement.CarLengthMatchesEyesBrokenDistance", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureCarLengthTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeTestConfig(), true);
	StartConveyor(Core);

	RunCarIn(Core, 8.f); // eyes broken over 8 ft of travel

	TestEqual(TEXT("One car tracked"), Core.GetCars().Num(), 1);
	if (Core.GetCars().Num() == 1)
	{
		const FMLCarState& Car = Core.GetCars()[0];
		TestFalse(TEXT("Measurement complete after the eyes clear"), Car.bMeasuring);
		TestEqual(TEXT("Measured length ~= eyes-broken distance"), Car.LengthFeet, 8.f, 0.25f);
		TestEqual(TEXT("Front bumper at the eyes-clear position"), Car.FrontPositionFeet, 8.f, 0.25f);
	}
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureMinCarLengthTest,
	"MicrologicSim.Measurement.ShortCarBelowMinimumDiscarded", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureMinCarLengthTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeTestConfig(), true); // MinCarLength = 6 ft
	StartConveyor(Core);

	RunCarIn(Core, 3.f); // 3 ft < 6 ft minimum
	TestEqual(TEXT("Sub-minimum car discarded (eyes never saw a real car)"), Core.GetCars().Num(), 0);

	// A real car afterwards still measures normally.
	Pump(Core, 2.f);
	RunCarIn(Core, 8.f);
	TestEqual(TEXT("Subsequent full-length car is tracked"), Core.GetCars().Num(), 1);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureMaxCarLengthTest,
	"MicrologicSim.Measurement.MeasurementForceCompletesAtMaximum", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureMaxCarLengthTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeTestConfig(), true); // MaxCarLength = 25 ft
	StartConveyor(Core);

	Core.SetInputRawByType(EMLInputType::Entry, true);
	Pump(Core, 30.f); // eyes held broken past the 25 ft maximum

	TestEqual(TEXT("Car still tracked"), Core.GetCars().Num(), 1);
	if (Core.GetCars().Num() == 1)
	{
		const FMLCarState& Car = Core.GetCars()[0];
		TestFalse(TEXT("Measurement force-completed while the eyes are still broken"), Car.bMeasuring);
		TestEqual(TEXT("Length clamped to Maximum Car Length"), Car.LengthFeet, 25.f, 0.01f);
	}

	Core.SetInputRawByType(EMLInputType::Entry, false);
	Pump(Core, 1.f);
	TestEqual(TEXT("Clearing the eyes afterwards spawns no extra car"), Core.GetCars().Num(), 1);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureDefaultWashTest,
	"MicrologicSim.Measurement.DefaultWashAppliedWhenNothingQueued", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureDefaultWashTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f, 0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, /*bDefault=*/false);
	AddService(Config, 40, EMLServiceType::Wash, { 10 }, TEXT("Default Wash"));
	Config.RollerDefaults.DefaultWashService = 40;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);

	RunCarIn(Core, 8.f); // no order queued
	TestEqual(TEXT("One car tracked"), Core.GetCars().Num(), 1);
	if (Core.GetCars().Num() == 1)
	{
		const FMLCarState& Car = Core.GetCars()[0];
		TestTrue(TEXT("Default Wash service bound to the unqueued car"), Car.ServiceNumbers.Contains(40));
		TestTrue(TEXT("Default Wash relays programmed"), Car.ProgrammedRelays.Contains(10));
	}
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureQueuedOrderBindsTest,
	"MicrologicSim.Measurement.QueuedOrderBindsToNextMeasuredCar", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureQueuedOrderBindsTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f, 0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, /*bDefault=*/false);
	AddService(Config, 40, EMLServiceType::Wash, { 10 }, TEXT("Test Wash"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	const int32 OrderId = Core.SendOrder({ 40 });
	TestTrue(TEXT("Order accepted"), OrderId != 0);
	TestEqual(TEXT("Order waiting in the queue"), Core.GetQueue().Num(), 1);

	StartConveyor(Core);
	RunCarIn(Core, 8.f);

	TestEqual(TEXT("Queue emptied by the measured car"), Core.GetQueue().Num(), 0);
	TestEqual(TEXT("One car tracked"), Core.GetCars().Num(), 1);
	if (Core.GetCars().Num() == 1)
	{
		const FMLCarState& Car = Core.GetCars()[0];
		TestTrue(TEXT("Queued order's service bound to the car"), Car.ServiceNumbers.Contains(40));
		TestTrue(TEXT("Order relays programmed"), Car.ProgrammedRelays.Contains(10));
	}
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureQueueModeNoneTest,
	"MicrologicSim.Measurement.QueueModeNoneSecondOrderReplacesFirst", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureQueueModeNoneTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig(); // QueueMode = None
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f, 0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, false);
	AddFunctionRelay(Config, 11, EMLFunctionType::VehicleLength, 20.f, 0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, false);
	AddService(Config, 40, EMLServiceType::Wash, { 10 }, TEXT("Wash A"));
	AddService(Config, 41, EMLServiceType::Wash, { 11 }, TEXT("Wash B"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.SendOrder({ 40 });
	Core.SendOrder({ 41 });
	TestEqual(TEXT("Queue Mode None holds a single order"), Core.GetQueue().Num(), 1);
	if (Core.GetQueue().Num() == 1)
	{
		TestTrue(TEXT("Second order replaced the first"), Core.GetQueue()[0].ServiceNumbers.Contains(41));
	}

	StartConveyor(Core);
	RunCarIn(Core, 8.f);
	if (Core.GetCars().Num() == 1)
	{
		const FMLCarState& Car = Core.GetCars()[0];
		TestTrue(TEXT("Car got the replacing order"), Car.ProgrammedRelays.Contains(11));
		TestFalse(TEXT("Replaced order's relays not programmed"), Car.ProgrammedRelays.Contains(10));
	}
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureQueueModeSequentialTest,
	"MicrologicSim.Measurement.QueueModeSequentialOrdersStackFifo", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureQueueModeSequentialTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	Config.RollerDefaults.QueueMode = EMLQueueMode::Sequential;
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f, 0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, false);
	AddFunctionRelay(Config, 11, EMLFunctionType::VehicleLength, 20.f, 0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, false);
	AddService(Config, 40, EMLServiceType::Wash, { 10 }, TEXT("Wash A"));
	AddService(Config, 41, EMLServiceType::Wash, { 11 }, TEXT("Wash B"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.SendOrder({ 40 });
	Core.SendOrder({ 41 });
	TestEqual(TEXT("Sequential mode stacks both orders"), Core.GetQueue().Num(), 2);

	StartConveyor(Core);
	RunCarIn(Core, 8.f);          // car 1 takes the first order
	TestEqual(TEXT("First order popped"), Core.GetQueue().Num(), 1);
	Pump(Core, 2.f);
	RunCarIn(Core, 8.f);          // car 2 takes the second order
	TestEqual(TEXT("Second order popped"), Core.GetQueue().Num(), 0);

	TestEqual(TEXT("Both cars tracked"), Core.GetCars().Num(), 2);
	if (Core.GetCars().Num() == 2)
	{
		const FMLCarState& CarA = Core.GetCars()[0];
		const FMLCarState& CarB = Core.GetCars()[1];
		TestTrue(TEXT("First car got the first order"), CarA.ProgrammedRelays.Contains(10));
		TestFalse(TEXT("First car did not get the second order"), CarA.ProgrammedRelays.Contains(11));
		TestTrue(TEXT("Second car got the second order"), CarB.ProgrammedRelays.Contains(11));
		TestFalse(TEXT("Second car did not get the first order"), CarB.ProgrammedRelays.Contains(10));
	}
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureQueueModeRandomTest,
	"MicrologicSim.Measurement.QueueModeRandomOrdersStackFifo", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureQueueModeRandomTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	Config.RollerDefaults.QueueMode = EMLQueueMode::Random;
	AddService(Config, 40, EMLServiceType::Wash, TArray<int32>(), TEXT("Wash A"));
	AddService(Config, 41, EMLServiceType::Wash, TArray<int32>(), TEXT("Wash B"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.SendOrder({ 40 });
	Core.SendOrder({ 41 });
	TestEqual(TEXT("Random mode stacks both orders"), Core.GetQueue().Num(), 2);
	if (Core.GetQueue().Num() == 2)
	{
		TestTrue(TEXT("Orders stored in send order (FIFO)"),
			Core.GetQueue()[0].ServiceNumbers.Contains(40) &&
			Core.GetQueue()[1].ServiceNumbers.Contains(41));
	}

	// The first measured car takes the first-sent order.
	StartConveyor(Core);
	RunCarIn(Core, 8.f);
	TestEqual(TEXT("Head of the queue consumed first"), Core.GetQueue().Num(), 1);
	if (Core.GetCars().Num() == 1)
	{
		TestTrue(TEXT("First car bound to the first-sent order"),
			Core.GetCars()[0].ServiceNumbers.Contains(40));
	}
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureNoConveyorInterlockTest,
	"MicrologicSim.Measurement.NoWashWithoutConveyorInterlockInput", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureNoConveyorInterlockTest::RunTest(const FString& Parameters)
{
	// Cheat sheet, Conveyor input: "Without this input, the controller will
	// not engage a wash." Break the sim's automatic interlock wire and leave
	// channel 2 off.
	FMLControllerConfig Config = MakeTestConfig();
	Config.Sim.bAutoConveyorInterlockWire = false;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	TestTrue(TEXT("Conveyor itself runs"), Core.IsConveyorRunning());
	TestFalse(TEXT("Interlock input (ch 2) is off"), Core.GetInputCommitted(2));

	RunCarIn(Core, 8.f);
	TestEqual(TEXT("Eyes broken with no conveyor interlock: no car measured"), Core.GetCars().Num(), 0);

	// Wiring the interlock manually restores measurement.
	Core.SetInputRaw(2, true);
	Pump(Core, 2.f);
	RunCarIn(Core, 8.f);
	TestEqual(TEXT("Car measured once the interlock input is on"), Core.GetCars().Num(), 1);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicMeasureServiceOnQueuedTest,
	"MicrologicSim.Measurement.ServiceOnQueuedFiresOnSendOrder", MICROLOGIC_TEST_FLAGS)
bool FMicrologicMeasureServiceOnQueuedTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 13, EMLRelayType::Normal, false, TEXT("Queue Beacon"));
	AddService(Config, 42, EMLServiceType::Toggler, { 13 }, TEXT("Queue Beacon Toggle"));
	AddService(Config, 40, EMLServiceType::Wash, TArray<int32>(), TEXT("Wash A"));
	Config.RollerDefaults.ServiceOnQueued = 42;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	TestFalse(TEXT("Beacon off before any order"), Core.GetRelayPhysical(13));

	Core.SendOrder({ 40 });
	TestTrue(TEXT("Service-on-queued executed by SendOrder (toggler flipped)"), Core.IsRelayToggledOn(13));
	TestTrue(TEXT("Beacon relay physically on"), Core.GetRelayPhysical(13));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
