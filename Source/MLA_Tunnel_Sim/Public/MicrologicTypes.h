// Copyright Micrologic Associates. All Rights Reserved.
//
// Data model for the Micrologic V2 tunnel-controller simulation.
// Field names and semantics mirror the V2 Micrologic Controller Cheat Sheet
// so that what technicians configure here matches the real controller UI.

#pragma once

#include "CoreMinimal.h"
#include "MicrologicTypes.generated.h"

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

/** Inputs page: the role of a physical input-board channel. */
UENUM(BlueprintType)
enum class EMLInputType : uint8
{
	None            UMETA(DisplayName = "None"),
	Trigger         UMETA(DisplayName = "Trigger"),
	Conveyor        UMETA(DisplayName = "Conveyor"),
	RollerPosition  UMETA(DisplayName = "Roller Position"),
	TireSwitch      UMETA(DisplayName = "Tire Switch"),
	UpperEntry      UMETA(DisplayName = "Upper Entry"),
	AntiCollision   UMETA(DisplayName = "Anti Collision"),
	Stall           UMETA(DisplayName = "Stall"),
	Entry           UMETA(DisplayName = "Entry"),
	ExitDoor        UMETA(DisplayName = "Exit Door")
};

/** Relays page: relay Type field. */
UENUM(BlueprintType)
enum class EMLRelayType : uint8
{
	Normal    UMETA(DisplayName = "Normal"),
	Roller    UMETA(DisplayName = "Roller"),
	Horn      UMETA(DisplayName = "Horn"),
	Conveyor  UMETA(DisplayName = "Conveyor")
};

/** Relays page: Function Type — how the relay times against the vehicle. */
UENUM(BlueprintType)
enum class EMLFunctionType : uint8
{
	None            UMETA(DisplayName = "None"),
	VehicleLength   UMETA(DisplayName = "Vehicle Length"),
	FrontOfVehicle  UMETA(DisplayName = "Front Of Vehicle"),
	RearOfVehicle   UMETA(DisplayName = "Rear Of Vehicle"),
	FrontHalf       UMETA(DisplayName = "Front Half Of Vehicle"),
	RearHalf        UMETA(DisplayName = "Rear Half Of Vehicle"),
	AllTires        UMETA(DisplayName = "All Tires"),
	FrontTires      UMETA(DisplayName = "Front Tires"),
	RearTires       UMETA(DisplayName = "Rear Tires"),
	PickupBed       UMETA(DisplayName = "Pickup Bed"),
	Light           UMETA(DisplayName = "Light")
};

/** Function modifiers — retract/suppression windows within a function. */
UENUM(BlueprintType)
enum class EMLModifierType : uint8
{
	FrontAndRearOnly  UMETA(DisplayName = "Front & Rear Only"),
	Bump              UMETA(DisplayName = "Bump"),
	MirrorBump        UMETA(DisplayName = "Mirror Bump"),
	OpenPickupBed     UMETA(DisplayName = "Open Pickup Bed"),
	RearOfCar         UMETA(DisplayName = "Rear Of Car"),
	FrontOfCar        UMETA(DisplayName = "Front Of Car")
};

/** Services page: service Type field. */
UENUM(BlueprintType)
enum class EMLServiceType : uint8
{
	Wash            UMETA(DisplayName = "Wash"),
	Service         UMETA(DisplayName = "Service"),
	Macro           UMETA(DisplayName = "Macro"),
	Deprogrammable  UMETA(DisplayName = "De-programmable"),
	TurnOff         UMETA(DisplayName = "Turn OFF"),
	TurnOn          UMETA(DisplayName = "Turn ON"),
	MomentarilyOn   UMETA(DisplayName = "Momentarily On"),
	MomentarilyOff  UMETA(DisplayName = "Momentarily Off"),
	Override        UMETA(DisplayName = "Override"),
	Toggler         UMETA(DisplayName = "Toggler"),
	Command         UMETA(DisplayName = "Command")
};

/** Roller/Defaults tab: controller queue mode. */
UENUM(BlueprintType)
enum class EMLQueueMode : uint8
{
	None        UMETA(DisplayName = "None"),
	Random      UMETA(DisplayName = "Random"),
	Sequential  UMETA(DisplayName = "Sequential")
};

/** Roller/Defaults tab: roller mode. */
UENUM(BlueprintType)
enum class EMLRollerMode : uint8
{
	ManualFront    UMETA(DisplayName = "Manual/Front"),
	AutomaticRear  UMETA(DisplayName = "Automatic/Rear")
};

/**
 * Turn On/Off Length reference. Read as a sentence, e.g.
 * "Turn on 8 ft Before the Front of the vehicle (reaches the device)".
 */
UENUM(BlueprintType)
enum class EMLTurnReference : uint8
{
	BeforeFront  UMETA(DisplayName = "Before Front Of Vehicle"),
	AfterFront   UMETA(DisplayName = "After Front Of Vehicle"),
	BeforeRear   UMETA(DisplayName = "Before Rear Of Vehicle"),
	AfterRear    UMETA(DisplayName = "After Rear Of Vehicle")
};

/** The 3-position manual switch on the physical relay (output) board. */
UENUM(BlueprintType)
enum class EMLRelayOverride : uint8
{
	Auto       UMETA(DisplayName = "Auto"),
	ForcedOn   UMETA(DisplayName = "On"),
	ForcedOff  UMETA(DisplayName = "Off")
};

/** Conveyor state machine. */
UENUM(BlueprintType)
enum class EMLConveyorState : uint8
{
	Stopped      UMETA(DisplayName = "Stopped"),
	HornDelay    UMETA(DisplayName = "Horn Delay"),
	Horn         UMETA(DisplayName = "Horn"),
	Running      UMETA(DisplayName = "Running"),
	SlowingDown  UMETA(DisplayName = "Slowing Down")
};

/** Push Button Station / keypad commands. */
UENUM(BlueprintType)
enum class EMLCommand : uint8
{
	None           UMETA(DisplayName = "None"),
	AcceptOrder    UMETA(DisplayName = "Accept Order"),
	CancelOrder    UMETA(DisplayName = "Cancel Order"),
	RemoveLast     UMETA(DisplayName = "Remove Last"),
	RemoveAll      UMETA(DisplayName = "Remove All"),
	RollerAbort    UMETA(DisplayName = "Roller Abort"),
	RollerRequest  UMETA(DisplayName = "Roller Request")
};

/** Why the conveyor last stopped — surfaced on the dashboard for training. */
UENUM(BlueprintType)
enum class EMLStopReason : uint8
{
	None           UMETA(DisplayName = "None"),
	Manual         UMETA(DisplayName = "Manual Stop"),
	StopCircuit    UMETA(DisplayName = "Stop Circuit / E-Stop"),
	Stall          UMETA(DisplayName = "Conveyor Stall"),
	ExitDoor       UMETA(DisplayName = "Exit Door"),
	AntiCollision  UMETA(DisplayName = "Anti-Collision"),
	Inactivity     UMETA(DisplayName = "Inactivity Timeout")
};

// ---------------------------------------------------------------------------
// Configuration structs (what Backup/Restore serializes)
// ---------------------------------------------------------------------------

/** One modifier row on a relay function. */
USTRUCT(BlueprintType)
struct FMLModifierConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLModifierType Type = EMLModifierType::Bump;

	/**
	 * Distance (ft) into the function window before the modifier engages.
	 * Bump/MirrorBump: the engaged stretch before the retract (Bump example: 7).
	 * FrontAndRearOnly: the kept front-section length.
	 * FrontOfCar/RearOfCar/OpenPickupBed ignore it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float StartFeet = 0.f;

	/**
	 * Length (ft) of the suppressed/retracted stretch.
	 * Bump example: 11 → retracted for 11 ft, then re-engages.
	 * FrontOfCar: feet to wait after the front before turning on.
	 * RearOfCar: feet suppressed at the rear of the vehicle.
	 * FrontAndRearOnly: the kept rear-section length.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float LengthFeet = 0.f;
};

/** Function timing configuration on a relay. */
USTRUCT(BlueprintType)
struct FMLFunctionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLFunctionType Type = EMLFunctionType::None;

	/** Distance (ft) from the entry photo eyes to the device. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float DevicePositionFeet = 0.f;

	/** Turn On Length (ft) + reference. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float TurnOnFeet = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLTurnReference TurnOnReference = EMLTurnReference::BeforeFront;

	/** Turn Off Length (ft) + reference. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float TurnOffFeet = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLTurnReference TurnOffReference = EMLTurnReference::AfterRear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	TArray<FMLModifierConfig> Modifiers;
};

/** One relay row (Relays page). */
USTRUCT(BlueprintType)
struct FMLRelayConfig
{
	GENERATED_BODY()

	/** Relay number on the output boards (1-based; board N covers (N-1)*8+1..N*8). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 RelayNumber = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	bool bActive = true;

	/** Name of the function, e.g. "Top Brush", "Triple Foam". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString Description;

	/** Default = relay fires for every wash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	bool bDefault = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLRelayType Type = EMLRelayType::Normal;

	/** YES = while this relay is on, the inactivity timer will not stop the conveyor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	bool bInactivityCheck = false;

	/** Seconds the conveyor must run before the function may turn on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float InterlockStartSeconds = 0.f;

	/** Seconds the conveyor must be off before the function turns off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float InterlockStopSeconds = 0.f;

	/** Keep the function on if a following vehicle is within this distance (ft). 0 = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float LookBackFeet = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FMLFunctionConfig Function;
};

/** One input row (Inputs page). */
USTRUCT(BlueprintType)
struct FMLInputConfig
{
	GENERATED_BODY()

	/** Channel number on the input boards (1-based; board N covers (N-1)*16+1..N*16). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 Channel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLInputType Type = EMLInputType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString Description;

	/** Inverted flips the input's default state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	bool bInverted = false;

	/** Seconds a change must hold before the controller reacts. 0 = use Default Input Debounce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float DebounceSeconds = 0.f;

	/** For Type == Trigger: the service fired when this input activates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 TriggerServiceNumber = 0;
};

/** One service row (Services page). */
USTRUCT(BlueprintType)
struct FMLServiceConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 ServiceNumber = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLServiceType Type = EMLServiceType::Service;

	/**
	 * Relays this service programs/deprograms/targets.
	 * Wash: the package's non-default relays. Service: add-on relays.
	 * De-programmable: relays removed. Turn ON/OFF: first relay of the cascade.
	 * Momentarily On/Off: relays pulsed. Override: relays forced full-length.
	 * Toggler: relays toggled.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	TArray<int32> RelayNumbers;

	/** Momentarily On/Off: how long (seconds) the relays hold the momentary state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float TimeSeconds = 0.f;

	/** Momentarily On/Off: delay (seconds) before the momentary state engages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float DelaySeconds = 0.f;

	/** Macro: the services this macro bundles (executed in order). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	TArray<int32> MacroServiceNumbers;

	/** Command: the controller command this service issues. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLCommand Command = EMLCommand::None;
};

/** Settings → Communications tab. */
USTRUCT(BlueprintType)
struct FMLCommunicationsSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 NumInputBoards = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 NumRelayBoards = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString InputBoardPort = TEXT("COM1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString OutputBoardPort = TEXT("COM2");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString KeypadPort = TEXT("COM3");

	/** Exit Door feature: conveyor only runs while the Exit Door input is ON. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	bool bExitDoorEnabled = false;

	/** Settings → Sonar tab (informational unless a site uses Micrologic sonar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString SonarAddress = TEXT("10.0.1.95");
};

/** Settings → Conveyor tab. */
USTRUCT(BlueprintType)
struct FMLConveyorSettings
{
	GENERATED_BODY()

	/** Inches the conveyor travels each second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float InchesPerSecond = 13.94f;

	/** Inches of travel per pulse-switch pulse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float InchesPerPulse = 12.86f;

	/** On Activation Button: the conveyor-start service. 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 OnActivationService = 0;

	/** Shut Off Button: the conveyor-stop service. 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 ShutOffService = 0;

	/** Seconds with no wash in the tunnel before the conveyor auto-stops. 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float InactivityTimeoutSeconds = 0.f;

	/** Seconds the horn relay fires on conveyor activation. 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float HornTimeSeconds = 0.f;

	/** Seconds the controller delays the horn before sounding. 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float HornDelaySeconds = 0.f;
};

/** Settings → Anti-Collision tab. */
USTRUCT(BlueprintType)
struct FMLAntiCollisionSettings
{
	GENERATED_BODY()

	/** The relay configured as the anti-collision (ghost) relay. 0 = not configured. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 RelayNumber = 0;

	/** After Anti-Collision Clears Activate Button: conveyor-start service. 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 AfterClearsActivateService = 0;

	/** Slow Down: service engaged when the slow-down begins. 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 SlowDownService = 0;

	/** Seconds of slow-down before the conveyor stops. 0 = immediate shut off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float SlowDownTimeSeconds = 0.f;

	/** Slow Down Horn: service fired when the slow-down engages (typically the horn). 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 SlowDownHornService = 0;

	/**
	 * Conveyor Stall: if YES the stall input is enabled — the conveyor stops
	 * on it and will not start until it clears.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	bool bConveyorStallEnabled = true;
};

/** Settings → Roller/Defaults tab. */
USTRUCT(BlueprintType)
struct FMLRollerDefaults
{
	GENERATED_BODY()

	/** Minimum length (ft) the photo eyes must see before a wash engages. Preset by Micrologic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float MinCarLengthFeet = 6.f;

	/** Maximum length (ft) measured before the controller completes the order. Preset by Micrologic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float MaxCarLengthFeet = 25.f;

	/** Average car length (ft). Preset by Micrologic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float AverageCarLengthFeet = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLRollerMode RollerMode = EMLRollerMode::AutomaticRear;

	/** Distance (ft) the leading set of rollers fires for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float UpFeet = 4.f;

	/** Distance (ft) the next set of rollers is held for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float DownFeet = 4.f;

	/** Distance (ft) the second set fires for (follows the car off the track). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float UpAgainFeet = 8.f;

	/** YES = roller requests are denied until an order is in the controller queue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	bool bNeedsCarQueuedForRollerRequest = false;

	/** Default Input Debounce (seconds) for inputs that don't specify their own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float DefaultInputDebounceSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	EMLQueueMode QueueMode = EMLQueueMode::None;

	/** Service fired every time an order is sent to the controller. 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 ServiceOnQueued = 0;

	/** Default Wash if None Programmed: fires if a car breaks the eyes with no order. 0 = disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	int32 DefaultWashService = 0;
};

/** Settings → Security tab (+ the VNC-style native UI lock). */
USTRUCT(BlueprintType)
struct FMLSecuritySettings
{
	GENERATED_BODY()

	/** Require a PIN code to override relays from the native UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	bool bRequirePinForRelayOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString PinCode;

	/** Password for unlocking the native (VNC) UI with the 'L' key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString UiPassword = TEXT("manager01");

	/** Web UI login. Real controller default: manager / manager01. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString Username = TEXT("manager");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FString Password = TEXT("manager01");
};

/** Simulation-only knobs (not part of the real controller UI). */
USTRUCT(BlueprintType)
struct FMLSimSettings
{
	GENERATED_BODY()

	/** Length (ft) of the tunnel from the entry eyes to car despawn/exit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float TunnelLengthFeet = 120.f;

	/**
	 * When true, the sim "wires" the Conveyor-type input to the running state
	 * automatically. Set false to train the missing-conveyor-interlock fault
	 * (controller will not engage a wash).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	bool bAutoConveyorInterlockWire = true;

	/** Feet behind the entry eyes at which the anti-collision pad sits (visual aid only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	float AntiCollisionPadPositionFeet = 110.f;
};

/** The complete controller configuration — what Backup/Restore serializes. */
USTRUCT(BlueprintType)
struct FMLControllerConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FMLCommunicationsSettings Communications;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FMLConveyorSettings Conveyor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FMLAntiCollisionSettings AntiCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FMLRollerDefaults RollerDefaults;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FMLSecuritySettings Security;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	FMLSimSettings Sim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	TArray<FMLInputConfig> Inputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	TArray<FMLRelayConfig> Relays;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Micrologic")
	TArray<FMLServiceConfig> Services;
};

// ---------------------------------------------------------------------------
// Runtime state structs (read-only views for UI / world)
// ---------------------------------------------------------------------------

/** An order waiting in the controller queue. */
USTRUCT(BlueprintType)
struct FMLOrder
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	int32 OrderId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	TArray<int32> ServiceNumbers;
};

/** A vehicle tracked in the tunnel. Positions are feet past the entry eyes. */
USTRUCT(BlueprintType)
struct FMLCarState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	int32 CarId = 0;

	/** Distance (ft) the front bumper has traveled past the entry eyes. */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	float FrontPositionFeet = 0.f;

	/** Measured length (ft). Grows while measuring. */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	float LengthFeet = 0.f;

	/** True while the entry eyes are still measuring this vehicle. */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	bool bMeasuring = false;

	/** Cab length (ft) if the upper-entry sensor detected a bed/open area behind the cab. 0 = none. */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	float CabLengthFeet = 0.f;

	/** True if flagged as an open pickup bed (sonar simulation). */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	bool bOpenPickupBed = false;

	/** Tire positions as distance (ft) behind the front bumper, from the tire switch. */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	TArray<float> TireOffsetsFeet;

	/** Services applied to this vehicle. */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	TArray<int32> ServiceNumbers;

	/** Relays programmed to fire for this vehicle (after defaults/washes/deprograms). */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	TArray<int32> ProgrammedRelays;

	/** Relays forced full-length by an Override service. */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	TArray<int32> OverrideRelays;

	float RearPositionFeet() const { return FrontPositionFeet - LengthFeet; }
};

/** Live state of one relay, combining controller logic and the manual board switch. */
USTRUCT(BlueprintType)
struct FMLRelayRuntime
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	int32 RelayNumber = 0;

	/** What the controller logic commands. */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	bool bLogicalOn = false;

	/** The physical output after the manual ON/OFF/AUTO switch. */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	bool bPhysicalOn = false;

	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	EMLRelayOverride Override = EMLRelayOverride::Auto;
};

/** Live state of one input channel. */
USTRUCT(BlueprintType)
struct FMLInputRuntime
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	int32 Channel = 0;

	/** Raw wire state as set by the world (before inversion/debounce). */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	bool bRaw = false;

	/** State the controller acts on (after inversion + debounce). */
	UPROPERTY(BlueprintReadOnly, Category = "Micrologic")
	bool bCommitted = false;
};
