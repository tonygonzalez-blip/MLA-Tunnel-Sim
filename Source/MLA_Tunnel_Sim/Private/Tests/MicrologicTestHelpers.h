// Copyright Micrologic Associates. All Rights Reserved.
//
// Shared helpers for the MicrologicSim automation test suite.
//
// The canonical test configuration uses IPS = 12 in/s and IPP = 12 in/pulse so
// the conveyor moves exactly 1 foot per second and emits exactly 1 pulse per
// foot: seconds == feet == pulses, which keeps window math readable.
//
// Pulse-phase discipline: StartConveyor() leaves the pulse accumulator ~0.5 ft
// into the current pulse interval. From then on, pumping WHOLE seconds
// advances the tunnel by exactly that many pulses, so car front positions are
// exact integer feet and assertions never sit on a float boundary.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "MicrologicSimCore.h"
#include "MicrologicTypes.h"

// UE 5.5+ turned EAutomationTestFlags into an enum class and moved the
// application-context mask to the EAutomationTestFlags_ApplicationContextMask
// #define. This is the 5.6 spelling of "ApplicationContextMask | ProductFilter".
#define MICROLOGIC_TEST_FLAGS (EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

namespace MicrologicTest
{
	// Numbers reserved by MakeTestConfig(). Tests add their own scenario
	// relays in the 10..19 range and services in the 30..59 range.
	constexpr int32 StartServiceNumber = 20; // MomentarilyOn -> relay 23, designated On Activation Button
	constexpr int32 StopServiceNumber  = 21; // MomentarilyOn -> relay 24, designated Shut Off Button
	constexpr int32 StartRelayNumber   = 23;
	constexpr int32 StopRelayNumber    = 24;

	inline FMLInputConfig& AddInput(FMLControllerConfig& Config, int32 Channel, EMLInputType Type, const TCHAR* Description = TEXT(""))
	{
		FMLInputConfig Input;
		Input.Channel = Channel;
		Input.Type = Type;
		Input.Description = Description;
		return Config.Inputs[Config.Inputs.Add(Input)];
	}

	inline FMLRelayConfig& AddRelay(FMLControllerConfig& Config, int32 RelayNumber, EMLRelayType Type = EMLRelayType::Normal, bool bDefault = false, const TCHAR* Description = TEXT(""))
	{
		FMLRelayConfig Relay;
		Relay.RelayNumber = RelayNumber;
		Relay.Type = Type;
		Relay.bDefault = bDefault;
		Relay.Description = Description;
		return Config.Relays[Config.Relays.Add(Relay)];
	}

	/** Relay with a function window. Defaults: VehicleLength-style 0 ft Before Front / 0 ft After Rear, Default=YES so every car programs it. */
	inline FMLRelayConfig& AddFunctionRelay(FMLControllerConfig& Config, int32 RelayNumber, EMLFunctionType FnType, float DevicePositionFeet,
		float TurnOnFeet = 0.f, EMLTurnReference TurnOnRef = EMLTurnReference::BeforeFront,
		float TurnOffFeet = 0.f, EMLTurnReference TurnOffRef = EMLTurnReference::AfterRear,
		bool bDefault = true)
	{
		FMLRelayConfig& Relay = AddRelay(Config, RelayNumber, EMLRelayType::Normal, bDefault);
		Relay.Function.Type = FnType;
		Relay.Function.DevicePositionFeet = DevicePositionFeet;
		Relay.Function.TurnOnFeet = TurnOnFeet;
		Relay.Function.TurnOnReference = TurnOnRef;
		Relay.Function.TurnOffFeet = TurnOffFeet;
		Relay.Function.TurnOffReference = TurnOffRef;
		return Relay;
	}

	inline FMLServiceConfig& AddService(FMLControllerConfig& Config, int32 ServiceNumber, EMLServiceType Type, const TArray<int32>& RelayNumbers = TArray<int32>(), const TCHAR* Description = TEXT(""))
	{
		FMLServiceConfig Service;
		Service.ServiceNumber = ServiceNumber;
		Service.Type = Type;
		Service.RelayNumbers = RelayNumbers;
		Service.Description = Description;
		return Config.Services[Config.Services.Add(Service)];
	}

	/**
	 * Minimal deterministic configuration (NOT the factory one):
	 *  - IPS = 12 in/s, IPP = 12 in/pulse -> 1 ft/s, 1 pulse/ft.
	 *  - No horn delay/time, no inactivity timeout, no anti-collision.
	 *  - Inputs: ch2 Conveyor, ch3 Entry, ch5 TireSwitch, ch6 UpperEntry,
	 *    ch7 AntiCollision, ch8 ExitDoor, ch9 Stall.
	 *  - Services 20/21 (MomentarilyOn -> relays 23/24) designated as the
	 *    On Activation / Shut Off buttons.
	 */
	inline FMLControllerConfig MakeTestConfig()
	{
		FMLControllerConfig Config;

		Config.Communications.NumInputBoards = 1;  // 16 input channels
		Config.Communications.NumRelayBoards = 3;  // 24 relays
		Config.Communications.bExitDoorEnabled = false;

		Config.Conveyor.InchesPerSecond = 12.f;
		Config.Conveyor.InchesPerPulse = 12.f;
		Config.Conveyor.OnActivationService = StartServiceNumber;
		Config.Conveyor.ShutOffService = StopServiceNumber;
		Config.Conveyor.InactivityTimeoutSeconds = 0.f;
		Config.Conveyor.HornTimeSeconds = 0.f;
		Config.Conveyor.HornDelaySeconds = 0.f;

		Config.AntiCollision = FMLAntiCollisionSettings();
		Config.AntiCollision.RelayNumber = 0;

		Config.RollerDefaults.MinCarLengthFeet = 6.f;
		Config.RollerDefaults.MaxCarLengthFeet = 25.f;
		Config.RollerDefaults.AverageCarLengthFeet = 15.f;
		Config.RollerDefaults.RollerMode = EMLRollerMode::AutomaticRear;
		Config.RollerDefaults.UpFeet = 4.f;
		Config.RollerDefaults.DownFeet = 4.f;
		Config.RollerDefaults.UpAgainFeet = 8.f;
		Config.RollerDefaults.bNeedsCarQueuedForRollerRequest = false;
		Config.RollerDefaults.DefaultInputDebounceSeconds = 0.f;
		Config.RollerDefaults.QueueMode = EMLQueueMode::None;
		Config.RollerDefaults.ServiceOnQueued = 0;
		Config.RollerDefaults.DefaultWashService = 0;

		Config.Sim.TunnelLengthFeet = 200.f;
		Config.Sim.bAutoConveyorInterlockWire = true;

		AddInput(Config, 2, EMLInputType::Conveyor, TEXT("Conveyor Interlock"));
		AddInput(Config, 3, EMLInputType::Entry, TEXT("Photo Eyes"));
		AddInput(Config, 5, EMLInputType::TireSwitch, TEXT("Tire Switch"));
		AddInput(Config, 6, EMLInputType::UpperEntry, TEXT("Upper Entry"));
		AddInput(Config, 7, EMLInputType::AntiCollision, TEXT("Anti-Collision"));
		AddInput(Config, 8, EMLInputType::ExitDoor, TEXT("Exit Door"));
		AddInput(Config, 9, EMLInputType::Stall, TEXT("Stall"));

		AddRelay(Config, StartRelayNumber, EMLRelayType::Normal, false, TEXT("Conveyor Start"));
		AddRelay(Config, StopRelayNumber, EMLRelayType::Normal, false, TEXT("Conveyor Stop"));

		FMLServiceConfig& Start = AddService(Config, StartServiceNumber, EMLServiceType::MomentarilyOn, { StartRelayNumber }, TEXT("Conveyor Start"));
		Start.TimeSeconds = 1.f;
		FMLServiceConfig& Stop = AddService(Config, StopServiceNumber, EMLServiceType::MomentarilyOn, { StopRelayNumber }, TEXT("Conveyor Stop"));
		Stop.TimeSeconds = 1.f;

		return Config;
	}

	/** Advance the core Seconds of wall time in small deterministic steps. */
	inline void Pump(FMicrologicSimCore& Core, float Seconds, float Step = 0.05f)
	{
		float Remaining = Seconds;
		while (Remaining > KINDA_SMALL_NUMBER)
		{
			const float Dt = FMath::Min(Step, Remaining);
			Core.Advance(Dt);
			Remaining -= Dt;
		}
	}

	/**
	 * Start the conveyor via the On Activation service (20) and pump past any
	 * configured horn delay/horn phases plus half a second, leaving the pulse
	 * accumulator half way to the next pulse boundary.
	 */
	inline void StartConveyor(FMicrologicSimCore& Core)
	{
		Core.ExecuteService(StartServiceNumber);
		const FMLConveyorSettings& Conveyor = Core.GetConfig().Conveyor;
		Pump(Core, Conveyor.HornDelaySeconds + Conveyor.HornTimeSeconds + 0.5f);
	}

	/**
	 * Drive a car of LengthFeet (whole feet) past the entry eyes: break the
	 * eyes, pump LengthFeet seconds (= LengthFeet pulses = LengthFeet feet),
	 * clear the eyes. On return the car's front bumper sits at LengthFeet.
	 */
	inline void RunCarIn(FMicrologicSimCore& Core, float LengthFeet)
	{
		Core.SetInputRawByType(EMLInputType::Entry, true);
		Pump(Core, LengthFeet);
		Core.SetInputRawByType(EMLInputType::Entry, false);
	}

	/** Pulse (rise + fall) the tire switch; during measurement this records a tire. */
	inline void PulseTireSwitch(FMicrologicSimCore& Core)
	{
		Core.SetInputRawByType(EMLInputType::TireSwitch, true);
		Core.SetInputRawByType(EMLInputType::TireSwitch, false);
	}

	/** Pump until car CarIndex's front bumper reaches TargetFront (whole feet, conveyor running). */
	inline void PumpToFront(FMicrologicSimCore& Core, int32 CarIndex, float TargetFront)
	{
		const TArray<FMLCarState>& Cars = Core.GetCars();
		if (Cars.IsValidIndex(CarIndex))
		{
			const float Delta = TargetFront - Cars[CarIndex].FrontPositionFeet;
			if (Delta > 0.f)
			{
				Pump(Core, Delta);
			}
		}
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
