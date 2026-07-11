// Copyright Micrologic Associates. All Rights Reserved.
//
// FMicrologicSimCore — the deterministic, engine-independent brain of the
// Micrologic V2 tunnel-controller simulation. No UObjects, no world, no tick
// dependencies: callers feed it wall time via Advance() and wire states via
// SetInputRaw(); it produces relay outputs and events. This makes every
// controller behavior unit-testable headlessly.
//
// Coordinate system: distances are FEET along the tunnel, origin at the entry
// photo eyes, increasing toward the exit. While the conveyor runs, travel
// accumulates at InchesPerSecond and a pulse is emitted every InchesPerPulse
// inches — exactly how the real controller derives car position.

#pragma once

#include "CoreMinimal.h"
#include "MicrologicTypes.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FMLOnRelayChanged, int32 /*RelayNumber*/, bool /*bPhysicalOn*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMLOnInputChanged, int32 /*Channel*/, bool /*bCommitted*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FMLOnConveyorStateChanged, EMLConveyorState);
DECLARE_MULTICAST_DELEGATE(FMLOnPulse);
DECLARE_MULTICAST_DELEGATE(FMLOnQueueChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FMLOnCarEvent, const FMLCarState&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMLOnServiceExecuted, int32 /*ServiceNumber*/, bool /*bAccepted*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FMLOnConveyorStopped, EMLStopReason);

/**
 * The controller simulation. One instance = one tunnel controller.
 *
 * External interface groups:
 *  - Configuration: SetConfig / GetConfig
 *  - Time:          Advance(DeltaSeconds)
 *  - Field wiring:  SetInputRaw (photo eyes, tire switch, A/C pad, stall, exit door...)
 *  - Start/Stop board: PressStartCircuit, SetStopCircuit
 *  - Orders:        SendOrder, CancelOrder, RemoveLastOrder, RemoveAllOrders
 *  - Services:      ExecuteService (what the PBS/VQ/kiosk sends)
 *  - Rollers:       RequestRoller, AbortRoller
 *  - Board switches: SetRelayOverride
 *  - Queries:       GetRelays, GetInputs, GetCars, GetQueue, GetConveyorState...
 */
class MLA_TUNNEL_SIM_API FMicrologicSimCore
{
public:
	FMicrologicSimCore();

	// ---- Configuration ----------------------------------------------------

	/** Replace the live configuration. bReload additionally reinitializes runtime state (keeps queue). */
	void SetConfig(const FMLControllerConfig& InConfig, bool bReload);
	const FMLControllerConfig& GetConfig() const { return Config; }

	/** Reset all runtime state: cars, queue, timers, relays, momentaries, toggles. */
	void ResetRuntime();

	// ---- Time -------------------------------------------------------------

	/** Advance the simulation by DeltaSeconds of wall time. */
	void Advance(float DeltaSeconds);

	// ---- Field wiring (world -> controller) --------------------------------

	/** Set the raw wire state of an input channel (1-based). Inversion/debounce applied internally. */
	void SetInputRaw(int32 Channel, bool bState);

	/** Convenience: raw state of the first channel configured with the given type. */
	void SetInputRawByType(EMLInputType Type, bool bState);

	/** Mark the vehicle currently being measured as an open pickup bed (sonar simulation). */
	void FlagMeasuringCarOpenBed(bool bOpenBed);

	// ---- Start/Stop board ---------------------------------------------------

	/** Momentarily close a START circuit (1-5): requests the horn/start sequence. */
	void PressStartCircuit(int32 Circuit);

	/** Set a STOP circuit (1-5). Circuits are Normally Closed: bClosed=false opens the circuit and kills the conveyor. */
	void SetStopCircuit(int32 Circuit, bool bClosed);
	bool IsStopCircuitClosed(int32 Circuit) const;

	// ---- Orders / queue ------------------------------------------------------

	/** Send an order (set of services) to the controller queue, honoring the Queue Mode. Returns the order id (0 = rejected). */
	int32 SendOrder(const TArray<int32>& ServiceNumbers);
	void CancelOrder();      // Cancel the order currently sent (newest)
	void RemoveLastOrder();  // Remove the last order sent
	void RemoveAllOrders();  // Clear the queue

	// ---- Services ------------------------------------------------------------

	/**
	 * Execute a service immediately (PBS/VQ/native UI path). Car-programming
	 * service types (Wash/Service/Deprogrammable/TurnOn/TurnOff/Override) only
	 * make sense inside an order and are ignored here unless a car is in the
	 * tunnel to apply them to (Override etc. applies to the newest tracked car).
	 * Momentary/Toggler/Macro/Command types act globally.
	 */
	bool ExecuteService(int32 ServiceNumber);

	/** Execute a keypad command directly. */
	bool ExecuteCommand(EMLCommand Command);

	// ---- Rollers ---------------------------------------------------------------

	bool RequestRoller();
	void AbortRoller();
	bool IsRollerSequenceActive() const { return RollerSequence.bActive; }
	int32 GetRollerCount() const { return RollerCount; }

	// ---- Relay board manual switches --------------------------------------------

	void SetRelayOverride(int32 RelayNumber, EMLRelayOverride Override);
	EMLRelayOverride GetRelayOverride(int32 RelayNumber) const;

	// ---- Queries -------------------------------------------------------------------

	EMLConveyorState GetConveyorState() const { return ConveyorState; }
	EMLStopReason GetLastStopReason() const { return LastStopReason; }
	bool IsConveyorRunning() const { return ConveyorState == EMLConveyorState::Running || ConveyorState == EMLConveyorState::SlowingDown; }

	bool GetRelayPhysical(int32 RelayNumber) const;
	bool GetRelayLogical(int32 RelayNumber) const;
	bool GetInputCommitted(int32 Channel) const;
	bool GetInputRaw(int32 Channel) const;

	int32 GetNumRelays() const;
	int32 GetNumInputs() const;

	TArray<FMLRelayRuntime> GetRelayStates() const;
	TArray<FMLInputRuntime> GetInputStates() const;
	const TArray<FMLCarState>& GetCars() const { return Cars; }
	const TArray<FMLOrder>& GetQueue() const { return Queue; }

	/** Toggler service state for a relay. */
	bool IsRelayToggledOn(int32 RelayNumber) const { return ToggledRelays.Contains(RelayNumber); }

	/** Seconds since the last pulse (drives the input-board pulse LED). */
	float GetTimeSinceLastPulse() const { return TimeSinceLastPulse; }

	/** Total pulses emitted since reset (diagnostics). */
	int64 GetTotalPulses() const { return TotalPulses; }

	/** Seconds the tunnel has been empty (inactivity timer). */
	float GetInactivitySeconds() const { return InactivitySeconds; }

	// ---- Events --------------------------------------------------------------------

	FMLOnRelayChanged OnRelayChanged;
	FMLOnInputChanged OnInputChanged;
	FMLOnConveyorStateChanged OnConveyorStateChanged;
	FMLOnConveyorStopped OnConveyorStopped;
	FMLOnPulse OnPulse;
	FMLOnQueueChanged OnQueueChanged;
	FMLOnCarEvent OnCarEntered;
	FMLOnCarEvent OnCarMeasured;
	FMLOnCarEvent OnCarExited;
	FMLOnServiceExecuted OnServiceExecuted;

private:
	// ---- Internal runtime types ----

	struct FInputRuntime
	{
		bool bRaw = false;        // wire state as set by the world
		bool bEffective = false;  // raw after inversion
		bool bCommitted = false;  // after debounce — what logic sees
		float PendingTimer = -1.f; // >=0: counting toward committing bEffective
		bool bPendingState = false;
	};

	struct FRelayRuntime
	{
		bool bLogicalOn = false;
		bool bPhysicalOn = false;
		EMLRelayOverride Override = EMLRelayOverride::Auto;
		// Interlock bookkeeping: desired-by-window state before interlocks
		bool bWindowActive = false;
		// While conveyor stopped, a relay that was on holds until InterlockStop elapses
		float OffPendingTimer = -1.f;
	};

	struct FMomentaryAction
	{
		int32 RelayNumber = 0;
		bool bTurnOn = true;      // true: force on for Duration; false: force off
		float DelayRemaining = 0.f;
		float DurationRemaining = 0.f;
		bool bStarted = false;
	};

	struct FCarProgram
	{
		TSet<int32> ProgrammedRelays;     // relays that fire for this car (incl. defaults unless deprogrammed)
		TSet<int32> OverrideRelays;       // forced full-length
		int32 CascadeOffFromRelay = 0;    // TurnOff service: suppress >= this number (0 = none)
		int32 CascadeOnFromRelay = 0;     // TurnOn service: force >= this number (0 = none)
	};

	struct FCar
	{
		FMLCarState State;
		FCarProgram Program;
		bool bUpperEntrySeenOff = false;  // cab end latched
		FMLOrder BoundOrder;              // order consumed from the queue at eyes-break
		bool bHasBoundOrder = false;      // returned to the queue if the "car" was a blip
	};

	struct FRollerSequence
	{
		bool bActive = false;
		int32 Phase = 0;          // 0=Up, 1=Down(hold), 2=UpAgain, 3=done
		float DistanceRemaining = 0.f;
	};

	// ---- Config + state ----

	FMLControllerConfig Config;

	TArray<FInputRuntime> Inputs;   // index = channel-1
	TArray<FRelayRuntime> Relays;   // index = relay-1
	TArray<FCar> Cars_Internal;
	TArray<FMLCarState> Cars;       // mirrored public view, rebuilt after changes
	TArray<FMLOrder> Queue;
	TArray<FMomentaryAction> Momentaries;
	TSet<int32> ToggledRelays;

	bool StopCircuits[5] = { true, true, true, true, true }; // NC: true = closed = OK

	EMLConveyorState ConveyorState = EMLConveyorState::Stopped;
	EMLStopReason LastStopReason = EMLStopReason::None;
	EMLStopReason PendingStopReasonOverride = EMLStopReason::None; // reattributes shut-off-service stops
	float ConveyorPhaseTimer = 0.f;     // horn delay / horn countdown
	float ConveyorRunSeconds = 0.f;     // continuous run time (interlock start)
	float ConveyorStopSeconds = 0.f;    // continuous stop time (interlock stop)
	float SlowDownRemaining = 0.f;      // anti-collision slow-down countdown
	bool bAntiCollisionHold = false;    // stopped by A/C; restart when input clears
	bool bACTripped = false;            // slow-down already fired for this approach

	float PulseAccumulatorFeet = 0.f;
	float TimeSinceLastPulse = 0.f;
	int64 TotalPulses = 0;
	float InactivitySeconds = 0.f;

	FRollerSequence RollerSequence;
	int32 RollerCount = 0;              // from the Roller Position input
	float LastTireSwitchLength = -1.f;  // measured length at last tire-switch pulse

	int32 NextOrderId = 1;
	int32 NextCarId = 1;
	int32 MeasuringCarIndex = INDEX_NONE;

	// Guard against runaway Macro recursion
	int32 MacroDepth = 0;

	// ---- Internals ----

	void EmitPulse();
	void AdvanceCarsOnePulse(float PulseFeet);
	void BeginCarMeasurement();
	void FinishCarMeasurement(bool bValid);
	void ApplyOrderToCar(FCar& Car, const FMLOrder& Order);
	void ApplyServiceToCar(FCar& Car, const FMLServiceConfig& Service);
	void CollectOrderServices(const TArray<int32>& ServiceNumbers, int32 Depth,
	                          TArray<const FMLServiceConfig*>& OutServices) const;
	void RebuildPublicCarStates();

	void EvaluateRelays();
	bool EvaluateRelayWindow(const FMLRelayConfig& RelayCfg, const FCar& Car) const;
	bool IsSuppressedByModifiers(const FMLFunctionConfig& Fn, const FCar& Car) const;
	static float ThresholdFor(EMLTurnReference Ref, float OffsetFeet, float DevicePos);
	float ReferencePosition(EMLTurnReference Ref, const FCar& Car) const;

	void RequestConveyorStart();
	void RequestConveyorStop(EMLStopReason Reason);
	void SetConveyorState(EMLConveyorState NewState);
	void UpdateConveyor(float Dt);
	void UpdateAntiCollision(float Dt);
	void UpdateInputsDebounce(float Dt);
	void UpdateMomentaries(float Dt);
	void UpdateInactivity(float Dt);
	void UpdateRollerSequenceOnPulse(float PulseFeet);

	void OnInputCommitted(int32 Channel, bool bNewState);
	void SyncConveyorInterlockWire();
	bool HasCommittedInput(EMLInputType Type) const;
	int32 FindChannelByType(EMLInputType Type) const;
	const FMLInputConfig* FindInputConfig(int32 Channel) const;
	const FMLRelayConfig* FindRelayConfig(int32 RelayNumber) const;
	const FMLServiceConfig* FindServiceConfig(int32 ServiceNumber) const;
	float EffectiveDebounce(const FMLInputConfig& InputCfg) const;

	void ApplyPhysicalRelayStates();

	float PulseFeetInterval() const;
	bool CanEngageWash() const;
};
