// Copyright Micrologic Associates. All Rights Reserved.
//
// Anti-collision tests. Per the factory pattern: ghost relay 19 (Normal,
// Default YES, Vehicle Length, 0' Before Front / 0' After Rear) at device
// position 100 ft; A/C input on channel 7; slow-down time 3 s; After
// Anti-Collision Clears Activate Button = the conveyor-start service.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.h"
#include "MicrologicTestHelpers.h"

using namespace MicrologicTest;

namespace
{
	constexpr int32 SlowDownHornService = 51;
	constexpr int32 SlowDownHornRelay = 15;

	FMLControllerConfig MakeACConfig()
	{
		FMLControllerConfig Config = MakeTestConfig();
		AddFunctionRelay(Config, 19, EMLFunctionType::VehicleLength, 100.f); // ghost relay, Default YES
		AddRelay(Config, SlowDownHornRelay, EMLRelayType::Normal, false, TEXT("A/C Horn Beacon"));
		AddService(Config, SlowDownHornService, EMLServiceType::Toggler, { SlowDownHornRelay }, TEXT("Slow Down Horn"));

		Config.AntiCollision.RelayNumber = 19;
		Config.AntiCollision.AfterClearsActivateService = StartServiceNumber;
		Config.AntiCollision.SlowDownService = 0;
		Config.AntiCollision.SlowDownTimeSeconds = 3.f;
		Config.AntiCollision.SlowDownHornService = SlowDownHornService;
		return Config;
	}

	/**
	 * Start the conveyor, run an 8-ft car in, occupy the A/C pad, and drive
	 * the car's front bumper to the ghost relay's device position (100 ft),
	 * which trips the anti-collision logic.
	 */
	void DriveCarToACDevice(FMicrologicSimCore& Core)
	{
		StartConveyor(Core);
		RunCarIn(Core, 8.f); // front = 8
		Core.SetInputRawByType(EMLInputType::AntiCollision, true);
		Pump(Core, 92.f);    // front = 100 = ghost relay device position
	}
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicACTripSlowsDownTest,
	"MicrologicSim.AntiCollision.TripEntersSlowingDown", MICROLOGIC_TEST_FLAGS)
bool FMicrologicACTripSlowsDownTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeACConfig(), true);
	DriveCarToACDevice(Core);

	TestTrue(TEXT("Car at the ghost relay with the pad occupied: SlowingDown"),
		Core.GetConveyorState() == EMLConveyorState::SlowingDown);
	TestTrue(TEXT("Conveyor still moving during the slow-down"), Core.IsConveyorRunning());
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicACSlowDownHornTest,
	"MicrologicSim.AntiCollision.SlowDownHornServiceFiredOnTrip", MICROLOGIC_TEST_FLAGS)
bool FMicrologicACSlowDownHornTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeACConfig(), true);

	TestFalse(TEXT("Horn beacon off before the trip"), Core.IsRelayToggledOn(SlowDownHornRelay));
	DriveCarToACDevice(Core);
	TestTrue(TEXT("Slow Down Horn service executed exactly when the trip began"),
		Core.IsRelayToggledOn(SlowDownHornRelay));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicACStopsAfterSlowDownTest,
	"MicrologicSim.AntiCollision.StopsAfterSlowDownTime", MICROLOGIC_TEST_FLAGS)
bool FMicrologicACStopsAfterSlowDownTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeACConfig(), true);
	DriveCarToACDevice(Core);

	Pump(Core, 2.0f); // 2 s into the 3 s slow-down
	TestTrue(TEXT("Still slowing down before the 3 s elapse"),
		Core.GetConveyorState() == EMLConveyorState::SlowingDown);

	Pump(Core, 1.5f); // 3.5 s total
	TestTrue(TEXT("Stopped once the slow-down time elapses"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);
	TestTrue(TEXT("Stop reason is AntiCollision"), Core.GetLastStopReason() == EMLStopReason::AntiCollision);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicACAutoRestartTest,
	"MicrologicSim.AntiCollision.RestartsWhenInputClears", MICROLOGIC_TEST_FLAGS)
bool FMicrologicACAutoRestartTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeACConfig(), true);
	DriveCarToACDevice(Core);
	Pump(Core, 3.5f);
	TestTrue(TEXT("Precondition: stopped by anti-collision"),
		Core.GetConveyorState() == EMLConveyorState::Stopped &&
		Core.GetLastStopReason() == EMLStopReason::AntiCollision);

	Core.SetInputRawByType(EMLInputType::AntiCollision, false); // lead car exits
	Pump(Core, 0.5f);
	TestTrue(TEXT("After Anti-Collision Clears Activate: conveyor restarts automatically"),
		Core.GetConveyorState() == EMLConveyorState::Running);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicACZeroSlowDownTest,
	"MicrologicSim.AntiCollision.ZeroSlowDownTimeStopsImmediately", MICROLOGIC_TEST_FLAGS)
bool FMicrologicACZeroSlowDownTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeACConfig();
	Config.AntiCollision.SlowDownTimeSeconds = 0.f;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	DriveCarToACDevice(Core);

	TestTrue(TEXT("Slow Down Time 0: immediate shut off at the trip"),
		Core.GetConveyorState() == EMLConveyorState::Stopped);
	TestTrue(TEXT("Stop reason is AntiCollision"), Core.GetLastStopReason() == EMLStopReason::AntiCollision);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicACClearDuringSlowDownTest,
	"MicrologicSim.AntiCollision.PadClearingDuringSlowDownResumesRunning", MICROLOGIC_TEST_FLAGS)
bool FMicrologicACClearDuringSlowDownTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeACConfig(), true);
	DriveCarToACDevice(Core);
	Pump(Core, 1.0f); // 1 s into the 3 s slow-down
	TestTrue(TEXT("Precondition: slowing down"), Core.GetConveyorState() == EMLConveyorState::SlowingDown);

	Core.SetInputRawByType(EMLInputType::AntiCollision, false); // threat gone
	Pump(Core, 0.5f);
	TestTrue(TEXT("Pad cleared mid-countdown: back to Running"),
		Core.GetConveyorState() == EMLConveyorState::Running);
	TestTrue(TEXT("The conveyor never actually stopped"), Core.GetLastStopReason() == EMLStopReason::None);

	Pump(Core, 3.0f);
	TestTrue(TEXT("No delayed stop after the recovery"), Core.GetConveyorState() == EMLConveyorState::Running);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicACBlocksStartTest,
	"MicrologicSim.AntiCollision.StartBlockedWhilePadOccupied", MICROLOGIC_TEST_FLAGS)
bool FMicrologicACBlocksStartTest::RunTest(const FString& Parameters)
{
	// With anti-collision configured, the conveyor refuses to start while the
	// A/C input is on.
	FMicrologicSimCore CoreConfigured;
	CoreConfigured.SetConfig(MakeACConfig(), true);
	CoreConfigured.SetInputRawByType(EMLInputType::AntiCollision, true);
	CoreConfigured.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("A/C configured + pad occupied: start refused"),
		CoreConfigured.GetConveyorState() == EMLConveyorState::Stopped);

	CoreConfigured.SetInputRawByType(EMLInputType::AntiCollision, false);
	CoreConfigured.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("Pad cleared: start allowed"),
		CoreConfigured.GetConveyorState() == EMLConveyorState::Running);

	// Without an A/C relay configured, the input does not gate the start.
	FMicrologicSimCore CoreUnconfigured;
	CoreUnconfigured.SetConfig(MakeTestConfig(), true); // AntiCollision.RelayNumber = 0
	CoreUnconfigured.SetInputRawByType(EMLInputType::AntiCollision, true);
	CoreUnconfigured.ExecuteService(StartServiceNumber);
	TestTrue(TEXT("A/C not configured: input does not block the start"),
		CoreUnconfigured.GetConveyorState() == EMLConveyorState::Running);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
