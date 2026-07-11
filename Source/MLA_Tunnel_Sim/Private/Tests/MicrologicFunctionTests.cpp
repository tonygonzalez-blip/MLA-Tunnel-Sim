// Copyright Micrologic Associates. All Rights Reserved.
//
// Relay function-window tests. Geometry reminder (1 ft = 1 s = 1 pulse):
// a window "on at front >= X, off at front >= Y" is asserted by pumping the
// car's front bumper to exact integer feet and checking either side of each
// boundary pulse.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.h"
#include "MicrologicTestHelpers.h"

using namespace MicrologicTest;

namespace
{
	/** 8-ft car with tire-switch hits 1 ft and 5 ft behind the front bumper. */
	void RunCarWithTwoTires(FMicrologicSimCore& Core)
	{
		Core.SetInputRawByType(EMLInputType::Entry, true);
		Pump(Core, 1.f);
		PulseTireSwitch(Core); // tire offset 1 ft
		Pump(Core, 4.f);
		PulseTireSwitch(Core); // tire offset 5 ft
		Pump(Core, 3.f);
		Core.SetInputRawByType(EMLInputType::Entry, false);
	}

	/** 8-ft car with a single tire hit 1 ft behind the front bumper. */
	void RunCarWithOneTire(FMicrologicSimCore& Core)
	{
		Core.SetInputRawByType(EMLInputType::Entry, true);
		Pump(Core, 1.f);
		PulseTireSwitch(Core);
		Pump(Core, 7.f);
		Core.SetInputRawByType(EMLInputType::Entry, false);
	}

	/** 8-ft vehicle whose upper-entry sensor cleared at 3 ft: cab = 3 ft. */
	void RunCabTruck(FMicrologicSimCore& Core)
	{
		Core.SetInputRawByType(EMLInputType::UpperEntry, true);
		Core.SetInputRawByType(EMLInputType::Entry, true);
		Pump(Core, 3.f);
		Core.SetInputRawByType(EMLInputType::UpperEntry, false); // cab end latched at 3 ft
		Pump(Core, 5.f);
		Core.SetInputRawByType(EMLInputType::Entry, false);
	}
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionVehicleLengthTest,
	"MicrologicSim.Function.VehicleLengthWindowBoundaries", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionVehicleLengthTest::RunTest(const FString& Parameters)
{
	// "Turn on 2 ft Before the Front, turn off 3 ft After the Rear" at a
	// device 20 ft from the eyes. 8-ft car => ON for front in [18, 31).
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f,
		2.f, EMLTurnReference::BeforeFront, 3.f, EMLTurnReference::AfterRear);

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	RunCarIn(Core, 8.f);

	PumpToFront(Core, 0, 17.f);
	TestFalse(TEXT("front=17: one pulse before turn-on -> off"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 18.f);
	TestTrue(TEXT("front=18 (D - TurnOn): relay turns on"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 30.f);
	TestTrue(TEXT("front=30 (rear=22 < D + TurnOff): still on"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 31.f);
	TestFalse(TEXT("front=31 (rear=23 = D + TurnOff): relay turns off"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionFrontRearOfVehicleTest,
	"MicrologicSim.Function.FrontOfVehicleAndRearOfVehicleWindows", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionFrontRearOfVehicleTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	// Front Of Vehicle: on at front>=20, off once front passed 4 ft beyond -> front [20,24).
	AddFunctionRelay(Config, 11, EMLFunctionType::FrontOfVehicle, 20.f,
		0.f, EMLTurnReference::BeforeFront, 4.f, EMLTurnReference::AfterFront);
	// Rear Of Vehicle: on rear>=18, off rear>=22 -> rear [18,22) -> front [26,30) for an 8-ft car.
	AddFunctionRelay(Config, 12, EMLFunctionType::RearOfVehicle, 20.f,
		2.f, EMLTurnReference::BeforeRear, 2.f, EMLTurnReference::AfterRear);

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	RunCarIn(Core, 8.f);

	PumpToFront(Core, 0, 19.f);
	TestFalse(TEXT("front=19: FrontOfVehicle off before device"), Core.GetRelayPhysical(11));
	TestFalse(TEXT("front=19: RearOfVehicle off"), Core.GetRelayPhysical(12));

	PumpToFront(Core, 0, 21.f);
	TestTrue(TEXT("front=21: FrontOfVehicle on"), Core.GetRelayPhysical(11));
	TestFalse(TEXT("front=21: RearOfVehicle still off"), Core.GetRelayPhysical(12));

	PumpToFront(Core, 0, 24.f);
	TestFalse(TEXT("front=24: FrontOfVehicle off after its 4 ft"), Core.GetRelayPhysical(11));

	PumpToFront(Core, 0, 25.f);
	TestFalse(TEXT("front=25 (rear=17): RearOfVehicle not yet on"), Core.GetRelayPhysical(12));
	PumpToFront(Core, 0, 26.f);
	TestTrue(TEXT("front=26 (rear=18): RearOfVehicle on"), Core.GetRelayPhysical(12));
	PumpToFront(Core, 0, 29.f);
	TestTrue(TEXT("front=29 (rear=21): RearOfVehicle still on"), Core.GetRelayPhysical(12));
	PumpToFront(Core, 0, 30.f);
	TestFalse(TEXT("front=30 (rear=22): RearOfVehicle off"), Core.GetRelayPhysical(12));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionHalvesTest,
	"MicrologicSim.Function.FrontHalfAndRearHalfWindows", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionHalvesTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	// 8-ft car, device at 20, 0/0 offsets:
	// FrontHalf: front>=20 && mid<20 -> front [20,24). RearHalf: mid>=20 && rear<20 -> front [24,28).
	AddFunctionRelay(Config, 13, EMLFunctionType::FrontHalf, 20.f);
	AddFunctionRelay(Config, 14, EMLFunctionType::RearHalf, 20.f);

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	RunCarIn(Core, 8.f);

	PumpToFront(Core, 0, 19.f);
	TestFalse(TEXT("front=19: FrontHalf off"), Core.GetRelayPhysical(13));
	TestFalse(TEXT("front=19: RearHalf off"), Core.GetRelayPhysical(14));

	PumpToFront(Core, 0, 20.f);
	TestTrue(TEXT("front=20: FrontHalf on"), Core.GetRelayPhysical(13));
	TestFalse(TEXT("front=20: RearHalf off"), Core.GetRelayPhysical(14));

	PumpToFront(Core, 0, 23.f);
	TestTrue(TEXT("front=23 (mid=19): FrontHalf still on"), Core.GetRelayPhysical(13));
	TestFalse(TEXT("front=23: RearHalf still off"), Core.GetRelayPhysical(14));

	PumpToFront(Core, 0, 24.f);
	TestFalse(TEXT("front=24 (mid=20): FrontHalf off"), Core.GetRelayPhysical(13));
	TestTrue(TEXT("front=24 (mid=20): RearHalf on"), Core.GetRelayPhysical(14));

	PumpToFront(Core, 0, 27.f);
	TestTrue(TEXT("front=27 (rear=19): RearHalf still on"), Core.GetRelayPhysical(14));
	PumpToFront(Core, 0, 28.f);
	TestFalse(TEXT("front=28 (rear=20): RearHalf off"), Core.GetRelayPhysical(14));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionAllTiresTest,
	"MicrologicSim.Function.AllTiresFiresPerTireCrossing", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionAllTiresTest::RunTest(const FString& Parameters)
{
	// CTA-style: device at 20, on 0 Before Front, off 1 After Front.
	// Tire at offset O is active while (front - O) is in [20, 21):
	// tire@1 -> front [21,22), tire@5 -> front [25,26).
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 11, EMLFunctionType::AllTires, 20.f,
		0.f, EMLTurnReference::BeforeFront, 1.f, EMLTurnReference::AfterFront);

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	RunCarWithTwoTires(Core);

	TestEqual(TEXT("Two tires recorded"), Core.GetCars().Num() == 1 ? Core.GetCars()[0].TireOffsetsFeet.Num() : -1, 2);

	PumpToFront(Core, 0, 20.f);
	TestFalse(TEXT("front=20: no tire at the device yet"), Core.GetRelayPhysical(11));
	PumpToFront(Core, 0, 21.f);
	TestTrue(TEXT("front=21: first tire crossing -> on"), Core.GetRelayPhysical(11));
	PumpToFront(Core, 0, 22.f);
	TestFalse(TEXT("front=22: between tires -> off"), Core.GetRelayPhysical(11));
	PumpToFront(Core, 0, 24.f);
	TestFalse(TEXT("front=24: still between tires -> off"), Core.GetRelayPhysical(11));
	PumpToFront(Core, 0, 25.f);
	TestTrue(TEXT("front=25: second tire crossing -> on"), Core.GetRelayPhysical(11));
	PumpToFront(Core, 0, 26.f);
	TestFalse(TEXT("front=26: past the last tire -> off"), Core.GetRelayPhysical(11));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionFrontRearTiresTest,
	"MicrologicSim.Function.FrontTiresFirstOnlyRearTiresLastOnly", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionFrontRearTiresTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 12, EMLFunctionType::FrontTires, 20.f,
		0.f, EMLTurnReference::BeforeFront, 1.f, EMLTurnReference::AfterFront);
	AddFunctionRelay(Config, 13, EMLFunctionType::RearTires, 20.f,
		0.f, EMLTurnReference::BeforeFront, 1.f, EMLTurnReference::AfterFront);

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	RunCarWithTwoTires(Core); // tires at 1 ft (front set) and 5 ft (rear set)

	PumpToFront(Core, 0, 21.f); // front tire at the device
	TestTrue(TEXT("front=21: FrontTires fires for the first tire"), Core.GetRelayPhysical(12));
	TestFalse(TEXT("front=21: RearTires quiet for the first tire"), Core.GetRelayPhysical(13));

	PumpToFront(Core, 0, 25.f); // rear tire at the device
	TestFalse(TEXT("front=25: FrontTires quiet for the last tire"), Core.GetRelayPhysical(12));
	TestTrue(TEXT("front=25: RearTires fires for the last tire"), Core.GetRelayPhysical(13));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionTiresRequireTwoTest,
	"MicrologicSim.Function.FrontRearTiresNeedBothSetsDefined", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionTiresRequireTwoTest::RunTest(const FString& Parameters)
{
	// Cheat sheet: "both sets of tires need to be defined by the tire switch
	// for this function to work."
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 12, EMLFunctionType::FrontTires, 20.f,
		0.f, EMLTurnReference::BeforeFront, 1.f, EMLTurnReference::AfterFront);
	AddFunctionRelay(Config, 13, EMLFunctionType::RearTires, 20.f,
		0.f, EMLTurnReference::BeforeFront, 1.f, EMLTurnReference::AfterFront);

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	RunCarWithOneTire(Core); // only one tire defined

	TestEqual(TEXT("One tire recorded"), Core.GetCars().Num() == 1 ? Core.GetCars()[0].TireOffsetsFeet.Num() : -1, 1);

	PumpToFront(Core, 0, 21.f); // the single tire is at the device
	TestFalse(TEXT("front=21: FrontTires refuses with < 2 tires"), Core.GetRelayPhysical(12));
	TestFalse(TEXT("front=21: RearTires refuses with < 2 tires"), Core.GetRelayPhysical(13));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionPickupBedTest,
	"MicrologicSim.Function.PickupBedFiresForCabOnly", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionPickupBedTest::RunTest(const FString& Parameters)
{
	// Upper entry cleared at 3 ft while the eyes were still broken: cab = 3 ft.
	// Pickup Bed (D=20, 0/0): on while front >= 20 && cab end < 20 -> front [20,23).
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 11, EMLFunctionType::PickupBed, 20.f);

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	RunCabTruck(Core);

	TestEqual(TEXT("Cab length latched from the upper-entry falling edge"),
		Core.GetCars().Num() == 1 ? Core.GetCars()[0].CabLengthFeet : -1.f, 3.f, 0.25f);

	PumpToFront(Core, 0, 19.f);
	TestFalse(TEXT("front=19: PickupBed off before the device"), Core.GetRelayPhysical(11));
	PumpToFront(Core, 0, 20.f);
	TestTrue(TEXT("front=20: PickupBed on at the cab"), Core.GetRelayPhysical(11));
	PumpToFront(Core, 0, 22.f);
	TestTrue(TEXT("front=22 (cab end=19): still on"), Core.GetRelayPhysical(11));
	PumpToFront(Core, 0, 23.f);
	TestFalse(TEXT("front=23 (cab end=20): off for the bed, well before the rear (28)"), Core.GetRelayPhysical(11));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionDefaultVsProgrammedTest,
	"MicrologicSim.Function.DefaultRelayEveryCarNonDefaultOnlyProgrammed", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionDefaultVsProgrammedTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f); // Default = YES
	AddFunctionRelay(Config, 11, EMLFunctionType::VehicleLength, 20.f,
		0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, /*bDefault=*/false);
	AddService(Config, 40, EMLServiceType::Service, { 11 }, TEXT("Add-On"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);

	// Car 1: no order.
	RunCarIn(Core, 8.f);
	PumpToFront(Core, 0, 21.f);
	TestTrue(TEXT("Car without order: default relay fires"), Core.GetRelayPhysical(10));
	TestFalse(TEXT("Car without order: non-default relay quiet"), Core.GetRelayPhysical(11));
	PumpToFront(Core, 0, 30.f); // clear car 1 out of the window

	// Car 2: ordered the add-on service.
	Core.SendOrder({ 40 });
	RunCarIn(Core, 8.f);
	PumpToFront(Core, 1, 21.f);
	TestTrue(TEXT("Programmed car: non-default relay fires"), Core.GetRelayPhysical(11));
	TestTrue(TEXT("Programmed car: default relay fires too"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionInterlockStartTest,
	"MicrologicSim.Function.InterlockStartWaitsForConveyorRunTime", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionInterlockStartTest::RunTest(const FString& Parameters)
{
	// Interlock Start 15 s: the window opens at front=10 (~10.5 s of running)
	// but the relay must stay off until the conveyor has run 15 s.
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 10.f).InterlockStartSeconds = 15.f;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);   // ~0.5 s run
	RunCarIn(Core, 8.f);   // ~8.5 s run, front=8

	PumpToFront(Core, 0, 10.f); // ~10.5 s run: window open, interlock unsatisfied
	TestFalse(TEXT("Window open at 10.5 s run but Interlock Start (15 s) holds it off"),
		Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 14.f); // ~14.5 s run
	TestFalse(TEXT("Still held off just below 15 s of running"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 16.f); // ~16.5 s run
	TestTrue(TEXT("Relay engages once the conveyor has run past 15 s (car still in window)"),
		Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 18.f); // window ends (rear=10)
	TestFalse(TEXT("Window end still turns the relay off"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionInterlockStopTest,
	"MicrologicSim.Function.InterlockStopHoldsRelayAfterConveyorStops", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionInterlockStopTest::RunTest(const FString& Parameters)
{
	// Interlock Stop 2 s: a relay on over a stopped car keeps running until
	// the conveyor has been off for 2 s.
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 10.f).InterlockStopSeconds = 2.f;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);
	RunCarIn(Core, 8.f);
	PumpToFront(Core, 0, 11.f); // car inside the [10,18) window
	TestTrue(TEXT("Precondition: relay on over the car"), Core.GetRelayPhysical(10));

	Core.ExecuteService(StopServiceNumber); // conveyor stops, car frozen in the window
	TestTrue(TEXT("Relay still on immediately after the stop"), Core.GetRelayPhysical(10));

	Pump(Core, 1.0f);
	TestTrue(TEXT("Relay still on at 1 s of conveyor stop (< 2 s interlock)"), Core.GetRelayPhysical(10));

	Pump(Core, 1.5f); // 2.5 s stopped
	TestFalse(TEXT("Relay off once the conveyor has been off 2 s"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicFunctionLookBackTest,
	"MicrologicSim.Function.LookBackHoldsForTrailingCar", MICROLOGIC_TEST_FLAGS)
bool FMicrologicFunctionLookBackTest::RunTest(const FString& Parameters)
{
	// Look Back 6 ft on a NON-default relay programmed only for car A. When
	// A's window ends, trailing car B (unprogrammed) within 6 ft of the device
	// holds the relay on; it releases when B reaches the device.
	FMLControllerConfig Config = MakeTestConfig();
	AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f,
		0.f, EMLTurnReference::BeforeFront, 0.f, EMLTurnReference::AfterRear, /*bDefault=*/false)
		.LookBackFeet = 6.f;
	AddService(Config, 40, EMLServiceType::Service, { 10 }, TEXT("Program A"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.SendOrder({ 40 }); // will bind to car A
	StartConveyor(Core);
	RunCarIn(Core, 8.f);        // car A, front=8, programmed relay 10
	Pump(Core, 2.f);            // A front=10
	RunCarIn(Core, 8.f);        // car B, front=8 (A front=18); B unprogrammed

	PumpToFront(Core, 0, 20.f); // A in window [20,28)
	TestTrue(TEXT("A front=20: relay on for programmed car A"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 27.f); // A=27 on, B=17
	TestTrue(TEXT("A front=27: still on"), Core.GetRelayPhysical(10));

	PumpToFront(Core, 0, 28.f); // A window over; B front=18, 2 ft behind the device
	TestTrue(TEXT("A window ended but B is 2 ft behind the device: Look Back holds the relay on"),
		Core.GetRelayPhysical(10));

	PumpToFront(Core, 1, 19.f); // B 1 ft behind the device
	TestTrue(TEXT("B 1 ft behind: still held"), Core.GetRelayPhysical(10));

	PumpToFront(Core, 1, 20.f); // B reaches the device: no longer "behind"
	TestFalse(TEXT("Trailing car reached the device: Look Back releases (B is unprogrammed)"),
		Core.GetRelayPhysical(10));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
