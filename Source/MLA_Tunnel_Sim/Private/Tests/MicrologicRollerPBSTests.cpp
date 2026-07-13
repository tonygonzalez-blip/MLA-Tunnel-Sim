// Copyright Micrologic Associates. All Rights Reserved.
//
// Roller sequencing and Push Button Station command tests. Roller defaults:
// Up 4 ft, Down 4 ft, Up Again 8 ft — at 1 pulse/ft the roller relay is ON
// for pulses [0,4), OFF for [4,8), ON for [8,16), then off.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.h"
#include "MicrologicTestHelpers.h"

using namespace MicrologicTest;

namespace
{
	constexpr int32 RollerRelay = 16;
	constexpr int32 LightRelay = 17;

	FMLControllerConfig MakeRollerConfig()
	{
		FMLControllerConfig Config = MakeTestConfig(); // AutomaticRear, 4/4/8
		AddRelay(Config, RollerRelay, EMLRelayType::Roller, false, TEXT("Rollers"));
		AddRelay(Config, LightRelay, EMLRelayType::Normal, false, TEXT("Wash Confirmation Light"))
			.Function.Type = EMLFunctionType::Light;
		return Config;
	}
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicRollerSequenceTest,
	"MicrologicSim.Roller.AutomaticRearUpDownUpAgainSequence", MICROLOGIC_TEST_FLAGS)
bool FMicrologicRollerSequenceTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeRollerConfig(), true);
	StartConveyor(Core);

	TestTrue(TEXT("Roller request accepted in Automatic/Rear mode"), Core.RequestRoller());
	TestTrue(TEXT("Sequence active"), Core.IsRollerSequenceActive());
	TestTrue(TEXT("0 ft: leading set up"), Core.GetRelayPhysical(RollerRelay));

	Pump(Core, 3.f); // 3 ft traveled
	TestTrue(TEXT("3 ft: still up (Up = 4 ft)"), Core.GetRelayPhysical(RollerRelay));
	Pump(Core, 1.f); // 4 ft
	TestFalse(TEXT("4 ft: down phase begins"), Core.GetRelayPhysical(RollerRelay));
	Pump(Core, 3.f); // 7 ft
	TestFalse(TEXT("7 ft: still down (Down = 4 ft)"), Core.GetRelayPhysical(RollerRelay));
	Pump(Core, 1.f); // 8 ft
	TestTrue(TEXT("8 ft: up-again phase begins"), Core.GetRelayPhysical(RollerRelay));
	Pump(Core, 7.f); // 15 ft
	TestTrue(TEXT("15 ft: still up (Up Again = 8 ft)"), Core.GetRelayPhysical(RollerRelay));
	Pump(Core, 1.f); // 16 ft
	TestFalse(TEXT("16 ft: sequence complete, relay off"), Core.GetRelayPhysical(RollerRelay));
	TestFalse(TEXT("Sequence no longer active"), Core.IsRollerSequenceActive());
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicRollerAbortTest,
	"MicrologicSim.Roller.AbortMidSequenceTurnsOff", MICROLOGIC_TEST_FLAGS)
bool FMicrologicRollerAbortTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeRollerConfig(), true);
	StartConveyor(Core);

	Core.RequestRoller();
	Pump(Core, 1.f); // 1 ft into the Up phase
	TestTrue(TEXT("Precondition: roller up mid-sequence"), Core.GetRelayPhysical(RollerRelay));

	Core.AbortRoller();
	TestFalse(TEXT("Abort kills the sequence"), Core.IsRollerSequenceActive());
	TestFalse(TEXT("Roller relay drops immediately"), Core.GetRelayPhysical(RollerRelay));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicRollerNeedsCarQueuedTest,
	"MicrologicSim.Roller.NeedsCarQueuedGatesRequests", MICROLOGIC_TEST_FLAGS)
bool FMicrologicRollerNeedsCarQueuedTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeRollerConfig();
	Config.RollerDefaults.bNeedsCarQueuedForRollerRequest = true;
	AddService(Config, 40, EMLServiceType::Wash, TArray<int32>(), TEXT("Wash A"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	StartConveyor(Core);

	TestFalse(TEXT("Empty queue: roller request denied"), Core.RequestRoller());
	TestFalse(TEXT("No sequence started"), Core.IsRollerSequenceActive());

	Core.SendOrder({ 40 });
	TestTrue(TEXT("Order queued: roller request allowed"), Core.RequestRoller());
	TestTrue(TEXT("Sequence started"), Core.IsRollerSequenceActive());
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicRollerManualFrontTest,
	"MicrologicSim.Roller.ManualFrontRequiresTireSwitchInput", MICROLOGIC_TEST_FLAGS)
bool FMicrologicRollerManualFrontTest::RunTest(const FString& Parameters)
{
	// Manual/Front listens to the tire switch: with no TireSwitch input
	// configured the controller will not fire a series of rollers.
	FMLControllerConfig NoTireConfig = MakeRollerConfig();
	NoTireConfig.RollerDefaults.RollerMode = EMLRollerMode::ManualFront;
	NoTireConfig.Inputs.RemoveAll([](const FMLInputConfig& Input)
	{
		return Input.Type == EMLInputType::TireSwitch;
	});

	FMicrologicSimCore CoreNoTire;
	CoreNoTire.SetConfig(NoTireConfig, true);
	TestFalse(TEXT("Manual/Front without a tire switch input: request denied"), CoreNoTire.RequestRoller());

	FMLControllerConfig WithTireConfig = MakeRollerConfig(); // ch5 TireSwitch present
	WithTireConfig.RollerDefaults.RollerMode = EMLRollerMode::ManualFront;

	FMicrologicSimCore CoreWithTire;
	CoreWithTire.SetConfig(WithTireConfig, true);
	TestTrue(TEXT("Manual/Front with a tire switch input: request allowed"), CoreWithTire.RequestRoller());
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicRollerPositionCountTest,
	"MicrologicSim.Roller.RollerPositionInputCountsRisingEdges", MICROLOGIC_TEST_FLAGS)
bool FMicrologicRollerPositionCountTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeRollerConfig();
	AddInput(Config, 10, EMLInputType::RollerPosition, TEXT("Roller Position Switch"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);
	TestEqual(TEXT("Count starts at 0"), Core.GetRollerCount(), 0);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		Core.SetInputRaw(10, true);
		Core.SetInputRaw(10, false);
	}
	TestEqual(TEXT("Three rising edges counted"), Core.GetRollerCount(), 3);

	Core.SetInputRaw(10, false); // repeated low: no edge
	TestEqual(TEXT("Falling/steady states do not count"), Core.GetRollerCount(), 3);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicRollerLightMirrorsTest,
	"MicrologicSim.Roller.LightFunctionMirrorsRollerUpPhases", MICROLOGIC_TEST_FLAGS)
bool FMicrologicRollerLightMirrorsTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeRollerConfig(), true);
	StartConveyor(Core);

	Core.RequestRoller();
	TestTrue(TEXT("Up phase: light on with the roller"), Core.GetRelayPhysical(LightRelay));

	Pump(Core, 4.f); // Down phase
	TestFalse(TEXT("Down phase: light off with the roller"), Core.GetRelayPhysical(LightRelay));

	Pump(Core, 4.f); // Up Again phase
	TestTrue(TEXT("Up Again phase: light back on"), Core.GetRelayPhysical(LightRelay));

	Pump(Core, 8.f); // sequence done
	TestFalse(TEXT("Sequence over: light off"), Core.GetRelayPhysical(LightRelay));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicRollerQueueCommandsTest,
	"MicrologicSim.Roller.QueueCommandsPopAndClear", MICROLOGIC_TEST_FLAGS)
bool FMicrologicRollerQueueCommandsTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeRollerConfig();
	Config.RollerDefaults.QueueMode = EMLQueueMode::Sequential;
	AddService(Config, 40, EMLServiceType::Wash, TArray<int32>(), TEXT("Wash A"));
	AddService(Config, 41, EMLServiceType::Wash, TArray<int32>(), TEXT("Wash B"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	// Cancel Order removes the newest order.
	Core.SendOrder({ 40 });
	Core.SendOrder({ 41 });
	TestEqual(TEXT("Two orders queued"), Core.GetQueue().Num(), 2);
	TestTrue(TEXT("CancelOrder command accepted"), Core.ExecuteCommand(EMLCommand::CancelOrder));
	TestEqual(TEXT("CancelOrder popped one order"), Core.GetQueue().Num(), 1);
	if (Core.GetQueue().Num() == 1)
	{
		TestTrue(TEXT("The newest order was the one cancelled"),
			Core.GetQueue()[0].ServiceNumbers.Contains(40));
	}

	// Remove Last also removes the newest order.
	Core.SendOrder({ 41 });
	TestTrue(TEXT("RemoveLast command accepted"), Core.ExecuteCommand(EMLCommand::RemoveLast));
	TestEqual(TEXT("RemoveLast popped one order"), Core.GetQueue().Num(), 1);
	if (Core.GetQueue().Num() == 1)
	{
		TestTrue(TEXT("The oldest order remains"), Core.GetQueue()[0].ServiceNumbers.Contains(40));
	}

	// Remove All clears the queue.
	Core.SendOrder({ 41 });
	TestTrue(TEXT("RemoveAll command accepted"), Core.ExecuteCommand(EMLCommand::RemoveAll));
	TestEqual(TEXT("RemoveAll cleared the queue"), Core.GetQueue().Num(), 0);
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicRollerAbortCommandTest,
	"MicrologicSim.Roller.RollerAbortCommandCancelsSequence", MICROLOGIC_TEST_FLAGS)
bool FMicrologicRollerAbortCommandTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeRollerConfig(), true);
	StartConveyor(Core);

	TestTrue(TEXT("RollerRequest command starts a sequence"),
		Core.ExecuteCommand(EMLCommand::RollerRequest));
	Pump(Core, 1.f);
	TestTrue(TEXT("Sequence running"), Core.IsRollerSequenceActive());

	TestTrue(TEXT("RollerAbort command accepted"), Core.ExecuteCommand(EMLCommand::RollerAbort));
	TestFalse(TEXT("RollerAbort cancelled the sequence"), Core.IsRollerSequenceActive());
	TestFalse(TEXT("Roller relay off"), Core.GetRelayPhysical(RollerRelay));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicRollerLightIdleTest,
	"MicrologicSim.Roller.LightRelayStaysOffWithoutRollerActivity", MICROLOGIC_TEST_FLAGS)
bool FMicrologicRollerLightIdleTest::RunTest(const FString& Parameters)
{
	FMicrologicSimCore Core;
	Core.SetConfig(MakeRollerConfig(), true);
	StartConveyor(Core);

	TestFalse(TEXT("Light off with the conveyor running and no roller"), Core.GetRelayPhysical(LightRelay));

	RunCarIn(Core, 8.f); // a car passing does not drive a Light function
	Pump(Core, 5.f);
	TestFalse(TEXT("Light stays off while a car rides through with no roller request"),
		Core.GetRelayPhysical(LightRelay));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
