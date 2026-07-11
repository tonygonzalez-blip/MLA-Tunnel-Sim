// Copyright Micrologic Associates. All Rights Reserved.
//
// Hardware-layer tests: relay board manual switches (ON/OFF/AUTO), input
// inversion, debounce, trigger inputs, and what survives a Commit + Reload.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicControllerSubsystem.h"
#include "MicrologicTestHelpers.h"

using namespace MicrologicTest;

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicHardwareForcedOnTest,
	"MicrologicSim.Hardware.ForcedOnSwitchOverridesLogic", MICROLOGIC_TEST_FLAGS)
bool FMicrologicHardwareForcedOnTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 10, EMLRelayType::Normal, false, TEXT("Test Device"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	TestFalse(TEXT("No logic driving the relay"), Core.GetRelayLogical(10));
	TestFalse(TEXT("Physical off in Auto"), Core.GetRelayPhysical(10));

	Core.SetRelayOverride(10, EMLRelayOverride::ForcedOn);
	TestTrue(TEXT("Forced ON: physical on with the window off"), Core.GetRelayPhysical(10));
	TestFalse(TEXT("Logical state unaffected by the switch"), Core.GetRelayLogical(10));

	Core.SetRelayOverride(10, EMLRelayOverride::Auto);
	TestFalse(TEXT("Back to Auto: physical follows logic (off)"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicHardwareForcedOffTest,
	"MicrologicSim.Hardware.ForcedOffPinsRelayAgainstLogic", MICROLOGIC_TEST_FLAGS)
bool FMicrologicHardwareForcedOffTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 10, EMLRelayType::Normal, false, TEXT("Test Device"));
	AddService(Config, 47, EMLServiceType::Toggler, { 10 }, TEXT("Hold On"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.ExecuteService(47); // logic drives the relay on
	TestTrue(TEXT("Precondition: logically on"), Core.GetRelayLogical(10));
	TestTrue(TEXT("Precondition: physically on in Auto"), Core.GetRelayPhysical(10));

	Core.SetRelayOverride(10, EMLRelayOverride::ForcedOff);
	TestFalse(TEXT("Forced OFF pins the output even though logic says on"), Core.GetRelayPhysical(10));
	TestTrue(TEXT("Logic still on underneath"), Core.GetRelayLogical(10));

	Core.SetRelayOverride(10, EMLRelayOverride::Auto);
	TestTrue(TEXT("Auto returns the output to logic (on)"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicHardwareInvertedInputTest,
	"MicrologicSim.Hardware.InvertedInputFlipsCommittedState", MICROLOGIC_TEST_FLAGS)
bool FMicrologicHardwareInvertedInputTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddInput(Config, 10, EMLInputType::None, TEXT("Inverted Aux")).bInverted = true;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	TestFalse(TEXT("Raw wire is off"), Core.GetInputRaw(10));
	TestTrue(TEXT("Inverted: raw OFF commits as ON"), Core.GetInputCommitted(10));

	Core.SetInputRaw(10, true);
	TestTrue(TEXT("Raw wire now on"), Core.GetInputRaw(10));
	TestFalse(TEXT("Inverted: raw ON commits as OFF"), Core.GetInputCommitted(10));

	Core.SetInputRaw(10, false);
	TestTrue(TEXT("Raw back off commits as ON again"), Core.GetInputCommitted(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicHardwareDebounceTest,
	"MicrologicSim.Hardware.DebounceDelaysCommitAndDropsBlips", MICROLOGIC_TEST_FLAGS)
bool FMicrologicHardwareDebounceTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddInput(Config, 11, EMLInputType::None, TEXT("Debounced Aux")).DebounceSeconds = 1.f;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	// A blip shorter than the debounce never commits.
	Core.SetInputRaw(11, true);
	TestFalse(TEXT("Change not committed instantly"), Core.GetInputCommitted(11));
	Pump(Core, 0.4f);
	TestFalse(TEXT("Still pending inside the debounce window"), Core.GetInputCommitted(11));
	Core.SetInputRaw(11, false); // revert before the debounce elapses
	Pump(Core, 2.0f);
	TestFalse(TEXT("Reverted blip cancelled the pending change"), Core.GetInputCommitted(11));

	// A held change commits after the debounce time.
	Core.SetInputRaw(11, true);
	Pump(Core, 0.4f);
	TestFalse(TEXT("Held change still pending at 0.4 s"), Core.GetInputCommitted(11));
	Pump(Core, 0.8f); // 1.2 s held
	TestTrue(TEXT("Held change committed after ~1 s"), Core.GetInputCommitted(11));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicHardwareDefaultDebounceTest,
	"MicrologicSim.Hardware.DefaultInputDebounceUsedWhenUnset", MICROLOGIC_TEST_FLAGS)
bool FMicrologicHardwareDefaultDebounceTest::RunTest(const FString& Parameters)
{
	// The input's own Debounce is 0 -> the Roller/Defaults tab's Default
	// Input Debounce applies.
	FMLControllerConfig Config = MakeTestConfig();
	Config.RollerDefaults.DefaultInputDebounceSeconds = 1.f;
	AddInput(Config, 12, EMLInputType::None, TEXT("Aux, no own debounce")); // DebounceSeconds = 0

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.SetInputRaw(12, true);
	TestFalse(TEXT("Not committed instantly"), Core.GetInputCommitted(12));
	Pump(Core, 0.5f);
	TestFalse(TEXT("Still pending at 0.5 s (default debounce = 1 s)"), Core.GetInputCommitted(12));
	Pump(Core, 0.7f); // 1.2 s held
	TestTrue(TEXT("Committed after the default debounce elapses"), Core.GetInputCommitted(12));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicHardwareTriggerInputTest,
	"MicrologicSim.Hardware.TriggerInputExecutesServiceOnRisingEdge", MICROLOGIC_TEST_FLAGS)
bool FMicrologicHardwareTriggerInputTest::RunTest(const FString& Parameters)
{
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 10, EMLRelayType::Normal, false, TEXT("Prep Gun"));
	AddService(Config, 47, EMLServiceType::Toggler, { 10 }, TEXT("Prep Gun Toggle"));
	AddInput(Config, 13, EMLInputType::Trigger, TEXT("Prep Gun Button")).TriggerServiceNumber = 47;

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	TestFalse(TEXT("Relay starts off"), Core.GetRelayPhysical(10));

	Core.SetInputRaw(13, true); // rising edge fires the trigger service
	TestTrue(TEXT("Rising edge executed the trigger service (toggled on)"), Core.GetRelayPhysical(10));

	Core.SetInputRaw(13, false); // falling edge does nothing
	TestTrue(TEXT("Falling edge did not re-fire the service"), Core.GetRelayPhysical(10));

	Core.SetInputRaw(13, true); // next rising edge fires again
	TestFalse(TEXT("Second rising edge toggled back off"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicHardwareOverridesSurviveReloadTest,
	"MicrologicSim.Hardware.ManualSwitchesSurviveCommitReload", MICROLOGIC_TEST_FLAGS)
bool FMicrologicHardwareOverridesSurviveReloadTest::RunTest(const FString& Parameters)
{
	// Rewiring/reloading a controller does not move the physical board
	// switches.
	FMLControllerConfig Config = MakeTestConfig();
	AddRelay(Config, 10, EMLRelayType::Normal, false, TEXT("Test Device"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.SetRelayOverride(10, EMLRelayOverride::ForcedOn);
	TestTrue(TEXT("Precondition: forced on"), Core.GetRelayPhysical(10));

	const FMLControllerConfig ConfigCopy = Core.GetConfig();
	Core.SetConfig(ConfigCopy, /*bReload=*/true); // Commit + Reload

	TestTrue(TEXT("Manual switch position survives the reload"),
		Core.GetRelayOverride(10) == EMLRelayOverride::ForcedOn);
	TestTrue(TEXT("Relay still physically on after the reload"), Core.GetRelayPhysical(10));
	return true;
}

// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMicrologicHardwareQueueSurvivesReloadTest,
	"MicrologicSim.Hardware.QueueSurvivesCommitReload", MICROLOGIC_TEST_FLAGS)
bool FMicrologicHardwareQueueSurvivesReloadTest::RunTest(const FString& Parameters)
{
	// Cheat sheet, Backup/Restore: Commit + Reload "will not reboot it or
	// remove the queue."
	FMLControllerConfig Config = MakeTestConfig();
	AddService(Config, 40, EMLServiceType::Wash, TArray<int32>(), TEXT("Wash A"));

	FMicrologicSimCore Core;
	Core.SetConfig(Config, true);

	Core.SendOrder({ 40 });
	TestEqual(TEXT("Precondition: one order queued"), Core.GetQueue().Num(), 1);

	const FMLControllerConfig ConfigCopy = Core.GetConfig();
	Core.SetConfig(ConfigCopy, /*bReload=*/true); // Commit + Reload

	TestEqual(TEXT("Queue kept across Commit + Reload"), Core.GetQueue().Num(), 1);
	if (Core.GetQueue().Num() == 1)
	{
		TestTrue(TEXT("Order contents intact"), Core.GetQueue()[0].ServiceNumbers.Contains(40));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
