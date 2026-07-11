// Copyright Micrologic Associates. All Rights Reserved.
//
// Modifier (retract) tests. All scenarios: Normal default relay, Vehicle
// Length function, device at 20 ft, 0 ft Before Front / 0 ft After Rear, so
// the unmodified window is front in [20, 20 + car length). "S" below is feet
// of travel since the window opened (= front - 20).

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.h"
#include "MicrologicTestHelpers.h"

using namespace MicrologicTest;

namespace
{
	FMLControllerConfig MakeModifierConfig(EMLModifierType Type, float StartFeet, float LengthFeet)
	{
		FMLControllerConfig Config = MakeTestConfig();
		FMLRelayConfig& Relay = AddFunctionRelay(Config, 10, EMLFunctionType::VehicleLength, 20.f);
		FMLModifierConfig Mod;
		Mod.Type = Type;
		Mod.StartFeet = StartFeet;
		Mod.LengthFeet = LengthFeet;
		Relay.Function.Modifiers.Add(Mod);
		return Config;
	}
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicModifierBumpTest,
	"MicrologicSim.Modifier.BumpEngagesRetractsReengages", MICROLOGIC_TEST_FLAGS)
bool FMicrologicModifierBumpTest::RunTest(const FString& Parameters)
{
	// Cheat sheet common config: down for 7 ft, retract for 11 ft, back on.
	// 20-ft car: window front [20,40); on [20,27), off [27,38), on [38,40).
	FMicrologicSimCore Core;
	Core.SetConfig(MakeModifierConfig(EMLModifierType::Bump, 7.f, 11.f), true);
	StartConveyor(Core);
	RunCarIn(Core, 20.f);

	PumpToFront(Core, 0, 21.f);
	TestTrue(TEXT("S=1: engaged during the first 7 ft"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 26.f);
	TestTrue(TEXT("S=6: still engaged"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 27.f);
	TestFalse(TEXT("S=7: retract begins"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 37.f);
	TestFalse(TEXT("S=17: still retracted (7 + 11 = 18)"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 38.f);
	TestTrue(TEXT("S=18: re-engages after the 11 ft retract"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 39.f);
	TestTrue(TEXT("S=19: on until the window ends"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 40.f);
	TestFalse(TEXT("front=40: window over (rear passed the device)"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicModifierMirrorBumpTest,
	"MicrologicSim.Modifier.MirrorBumpRetractsAroundMirror", MICROLOGIC_TEST_FLAGS)
bool FMicrologicModifierMirrorBumpTest::RunTest(const FString& Parameters)
{
	// Cheat sheet common config: retract 2 ft 6 in after the front, back in
	// after 1 ft 9 in (Start 2.5, Length 1.75) -> suppressed S in [2.5, 4.25).
	FMicrologicSimCore Core;
	Core.SetConfig(MakeModifierConfig(EMLModifierType::MirrorBump, 2.5f, 1.75f), true);
	StartConveyor(Core);
	RunCarIn(Core, 8.f); // window front [20,28)

	PumpToFront(Core, 0, 22.f);
	TestTrue(TEXT("S=2 (< 2.5): wrap still in"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 23.f);
	TestFalse(TEXT("S=3: retracted around the mirror"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 24.f);
	TestFalse(TEXT("S=4 (< 4.25): still retracted"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 25.f);
	TestTrue(TEXT("S=5 (>= 4.25): back in"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicModifierFrontOfCarTest,
	"MicrologicSim.Modifier.FrontOfCarSuppressesFirstFeet", MICROLOGIC_TEST_FLAGS)
bool FMicrologicModifierFrontOfCarTest::RunTest(const FString& Parameters)
{
	// Grill retract: wait 2 ft after the front of the vehicle to turn on.
	FMicrologicSimCore Core;
	Core.SetConfig(MakeModifierConfig(EMLModifierType::FrontOfCar, 0.f, 2.f), true);
	StartConveyor(Core);
	RunCarIn(Core, 8.f); // window front [20,28)

	PumpToFront(Core, 0, 20.f);
	TestFalse(TEXT("S=0: suppressed at the front bumper"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 21.f);
	TestFalse(TEXT("S=1: still suppressed inside the first 2 ft"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 22.f);
	TestTrue(TEXT("S=2: turns on after 2 ft"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 27.f);
	TestTrue(TEXT("On through the rest of the vehicle"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicModifierRearOfCarTest,
	"MicrologicSim.Modifier.RearOfCarSuppressesLastFeet", MICROLOGIC_TEST_FLAGS)
bool FMicrologicModifierRearOfCarTest::RunTest(const FString& Parameters)
{
	// Hitch retract: off for the last 3 ft — suppressed once the car's rear
	// is within 3 ft of the device (rear >= 17 -> front >= 25 for an 8-ft car).
	FMicrologicSimCore Core;
	Core.SetConfig(MakeModifierConfig(EMLModifierType::RearOfCar, 0.f, 3.f), true);
	StartConveyor(Core);
	RunCarIn(Core, 8.f); // window front [20,28)

	PumpToFront(Core, 0, 24.f);
	TestTrue(TEXT("front=24 (rear=16): still engaged"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 25.f);
	TestFalse(TEXT("front=25 (rear=17, within 3 ft of device): retracted"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 27.f);
	TestFalse(TEXT("front=27: retracted through the rest of the vehicle"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicModifierFrontAndRearOnlyTest,
	"MicrologicSim.Modifier.FrontAndRearOnlySkipsMiddle", MICROLOGIC_TEST_FLAGS)
bool FMicrologicModifierFrontAndRearOnlyTest::RunTest(const FString& Parameters)
{
	// Keep the first 2 ft (Start) and the last 3 ft (Length); skip the middle.
	// 8-ft car: on front [20,22), off [22,25), on [25,28).
	FMicrologicSimCore Core;
	Core.SetConfig(MakeModifierConfig(EMLModifierType::FrontAndRearOnly, 2.f, 3.f), true);
	StartConveyor(Core);
	RunCarIn(Core, 8.f);

	PumpToFront(Core, 0, 21.f);
	TestTrue(TEXT("S=1: front section on"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 22.f);
	TestFalse(TEXT("S=2: middle skipped"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 24.f);
	TestFalse(TEXT("front=24 (rear=16): still skipped"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 25.f);
	TestTrue(TEXT("front=25 (rear=17, last 3 ft): rear section on"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 27.f);
	TestTrue(TEXT("Rear section on until the window ends"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicModifierOpenPickupBedTest,
	"MicrologicSim.Modifier.OpenPickupBedRetractsAtCabEnd", MICROLOGIC_TEST_FLAGS)
bool FMicrologicModifierOpenPickupBedTest::RunTest(const FString& Parameters)
{
	// Flagged open-bed truck (8 ft, cab 3 ft): the brush retracts once the cab
	// end passes the device -> on front [20,23), off [23,28).
	// A truck with a latched cab that is NOT flagged open-bed is unaffected.
	FMicrologicSimCore Core;
	Core.SetConfig(MakeModifierConfig(EMLModifierType::OpenPickupBed, 0.f, 0.f), true);
	StartConveyor(Core);

	auto RunTruck = [&Core](bool bFlagOpenBed)
	{
		Core.SetInputRawByType(EMLInputType::UpperEntry, true);
		Core.SetInputRawByType(EMLInputType::Entry, true);
		Pump(Core, 3.f);
		Core.SetInputRawByType(EMLInputType::UpperEntry, false); // cab end at 3 ft
		if (bFlagOpenBed)
		{
			Core.FlagMeasuringCarOpenBed(true);
		}
		Pump(Core, 5.f);
		Core.SetInputRawByType(EMLInputType::Entry, false);
	};

	// Truck 1: flagged open bed.
	RunTruck(true);
	PumpToFront(Core, 0, 22.f);
	TestTrue(TEXT("Flagged truck, front=22 (cab end=19): brush down over the cab"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 23.f);
	TestFalse(TEXT("Flagged truck, front=23 (cab end=20): retracted over the open bed"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 27.f);
	TestFalse(TEXT("Flagged truck: retracted for the rest of the bed"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 0, 40.f); // clear truck 1 well past the device

	// Truck 2: cab latched but NOT flagged open bed -> modifier must not fire.
	RunTruck(false);
	PumpToFront(Core, 1, 23.f);
	TestTrue(TEXT("Unflagged truck, front=23: unaffected by the modifier"), Core.GetRelayPhysical(10));
	PumpToFront(Core, 1, 27.f);
	TestTrue(TEXT("Unflagged truck: on for the full vehicle"), Core.GetRelayPhysical(10));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
