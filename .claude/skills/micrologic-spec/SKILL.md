---
name: micrologic-spec
description: "Use this skill for any work involving the Micrologic V2 tunnel-controller simulation in this repo: the domain semantics of relays, inputs, services, functions, and modifiers; questions about how the tunnel controller behaves (measuring cars, conveyor start/stop, anti-collision, rollers, inactivity, queue modes, backup/restore); changes to the C++ module under Source/MLA_Tunnel_Sim; building or reviewing controller UI widgets or world wiring against spec. Concrete triggers: 'relay', 'input channel', 'service/wash/deprogrammable', 'function type', 'modifier/bump/mirror bump', 'roller', 'anti-collision', 'photo eyes', 'tire switch', 'conveyor IPS/IPP/pulse', 'PBS/push button station', 'visual queue', 'E-stop/stop circuit', 'FMicrologicSimCore', 'MicrologicControllerSubsystem', editing any Micrologic*.h/.cpp, or checking sim behavior against the cheat sheet. SKIP for: pure Unreal editor mechanics with no controller semantics (use unreal-mcp), generic UE C++ questions unrelated to this module, or unrelated uses of 'relay'/'service' (e.g. web services, Unreal replication relays)."
---

# Micrologic V2 Controller Spec

This is the condensed domain spec for the Micrologic V2 tunnel controller,
distilled from the official V2 Micrologic Controller Cheat Sheet (Tony
Gonzalez). The C++ module in `Source/MLA_Tunnel_Sim/` implements it exactly.
**Rule: all controller behavior lives in `FMicrologicSimCore` — never in
Blueprints.** If a behavior question isn't answered here, read
`Source/MLA_Tunnel_Sim/Private/MicrologicSimCore.cpp`; it is the executable
spec, commented against the cheat sheet.

## Controller basics

- Real controller: web UI at IP **10.0.1.90** (Micrologic IP scheme 10.0.1.x),
  login **manager / manager01**. Native (VNC) UI unlock password: manager01.
  Sonar address typically **10.0.1.95**.
- Coordinate system: distances in **feet** along the tunnel, origin at the
  **entry photo eyes**, increasing toward the exit.
- Position comes from conveyor **pulses**: while the conveyor runs, travel
  accumulates at IPS inches/second and one pulse is emitted every IPP inches.
  The controller advances every tracked car by the pulse distance per pulse.

## Settings tabs

### Communications
- **Number of Input Boards**: each board = 16 input channels (typically 1).
- **Number of Relay Boards**: each board = 8 relays (96 relays → 12 boards).
- **Input/Output/Keypad Board Port**: COM ports on the controller PC
  (sim defaults COM1/COM2/COM3 — informational).
- **Exit Door**: when enabled and wired, the conveyor only runs while the Exit
  Door input is ON. Only works if the Shut Off Button is defined in the
  Conveyor tab.

### Conveyor
- **Inches Per Second (IPS)** — measured: time a roller over 20 ft, 3 trials,
  take the median; IPS = 240 / median seconds.
  Example: (17.1 + 17.3 + 17.25)/3 = 17.21 → 240/17.21 = **13.94**.
- **Inches Per Pulse (IPP)** — count pulse-LED blinks during that same time,
  3 trials, median; IPP = 240 / median pulses.
  Example: (19 + 18 + 19)/3 = 18.66 → 240/18.66 = **12.86**.
- **On Activation Button**: the Conveyor Start service; lets the controller
  auto-restart the tunnel after an inactivity stop (None = disabled).
- **Shut Off Button**: the Conveyor Stop service; lets the controller stop the
  tunnel after inactivity (None = disabled).
- **Conveyor Inactivity Timeout**: seconds the conveyor keeps running with no
  wash in the tunnel before auto-stop (0 = disabled).
- **Horn Time**: seconds the horn relay fires on conveyor activation (0 = off).
- **Horn Delay**: seconds to delay the horn before sounding (0 = off).
- Start sequence: Stopped → Horn Delay → Horn → Running.

### Anti-Collision
- **Relay**: the relay configured as the anti-collision **ghost relay** — a
  Normal, Default-YES relay with **no wires connected**, Function Type Vehicle
  Length, Device Position = where the trailing car should stop (typically
  6-10 ft before the A/C pad), Turn On 0' Before Front, Turn Off 0' After Rear,
  no modifiers.
- **Input**: the A/C pad or detection loop, wired to input 7 per the diagram.
  ON while a car sits at the tunnel end.
- Behavior: when a trailing car enters the ghost relay's window while the pad
  is occupied and the conveyor is running → fire **Slow Down** service (if set)
  and **Slow Down Horn** service (typically the horn), count down **Slow Down
  Time**, then stop the conveyor (Slow Down Time 0 = immediate stop). If the
  pad clears during the countdown, resume running. After the lead car clears
  the pad, **After Anti-Collision Clears Activate Button** (a conveyor-start
  service) restarts the conveyor automatically.

### Roller/Defaults
- Car lengths — **preset by Micrologic, do NOT change**:
  **Minimum Car Length 6 ft** (eyes must see at least this before a wash
  engages; shorter = discarded), **Maximum Car Length 25 ft** (measurement
  force-completes at this length), **Average 15 ft**.
- **Roller Mode**: **Manual/Front** — listens to the tire switch; without
  front/rear tires defined (tire switch input present), the controller will
  not fire rollers. **Automatic/Rear** — blind trigger; rollers fire on any
  request.
- **Up / Down / Up Again** (feet): leading roller set fires for Up ft; second
  set held Down ft; second set fires Up Again ft (follows the car out).
  Defaults 4 / 4 / 8.
- **Needs Car Queued For Roller Request**: YES = roller requests denied until
  an order is in the queue; NO = requests allowed at will from VQ/PBS.
- **Default Input Debounce**: seconds an input change must hold before the
  controller reacts (per-input Debounce of 0 falls back to this).
- **Queue Mode**:
  - **None** — one order max before the eyes; a second order **replaces** it.
  - **Random** — multiple orders stack in send order; typically non-auto-roller
    sites.
  - **Sequential** — multiple orders; typically non-VQ sites (kiosk straight to
    controller).
- **Service to Activate when Car is Queued**: fires every time a new order
  arrives (not the roller function).
- **Default Wash if None Programmed**: the service fired automatically if a
  car breaks the eyes with no order queued.

### Security
- **Require PIN Code to Override Relays**: YES = user creates a PIN, then the
  PIN is required to operate the native controller UI / flip relay overrides.

### Backup/Restore
- **Backup**: writes an XML file of the controller's current (live) config.
- **Restore**: loads a chosen backup file into the *staged* config; the user
  must **Commit + Reload** for it to be written to the running controller.
- **Reset**: erases the configuration back to factory defaults.
- **Commit**: saves changes to the live controller *without* reloading — used
  to test functionality before fully writing changes.
- **Commit + Reload**: saves changes and reloads the controller — does **not**
  reboot it and does **not** clear the queue.

## Input types (Inputs page)

Each input row: Channel, Type, Description, Inverted (flips default state),
Debounce (seconds; 0 = use default), and for Trigger a service number.

1. **Trigger** — fires a defined service when the input activates.
2. **Conveyor** — tells the controller the conveyor is on. **Without this
   input the controller will not engage a wash.**
3. **Roller Position** — wired to a roller position switch; counts rollers
   coming out. Not required for rollers to fire.
4. **Tire Switch** — pulses per tire; gives tire spacing. Used by tire
   functions (CTA etc.).
5. **Upper Entry** — truck-bed detection without sonar. If the entry eyes are
   still engaged when the upper entry clears, that latches the cab end (top
   brush retract applies if configured).
6. **Anti-Collision** — the A/C pad/loop: ON while a car is stopped at the
   tunnel end.
7. **Stall** — while engaged, the conveyor stops and cannot restart until
   cleared.
8. **Entry** — AKA the photo eyes; measures the vehicle.
9. **Exit Door** — with the Exit Door feature enabled, conveyor stops when
   this goes OFF and won't restart until it's ON.

### Factory default input wiring (channels 1-16, one board)

| Ch | Type | Description |
|---|---|---|
| 1 | None | Aux #1 |
| 2 | Conveyor | Conveyor Interlock |
| 3 | Entry | Lower Entry Sensor (Photo Eyes) |
| 4 | None | Aux #4 |
| 5 | Tire Switch | Tire Switch |
| 6 | Upper Entry | Upper Entry Sensor |
| 7 | Anti-Collision | Anti-Collision |
| 8 | Exit Door | Exit Door |
| 9 | Stall | Conveyor Stall |
| 10-16 | None | Aux #10-#16 |

## Relays page

Relay row fields:
- **Relay**: number on the output boards (board N covers relays (N-1)*8+1..N*8).
- **Active**: relay enabled or not.
- **Description**: function name ("Top Brush", "Triple Foam"...).
- **Default**: YES = fires for every wash (typically only basic-wash
  functions).
- **Type** (4 types):
  - **Normal** — no special configuration.
  - **Roller** — the roller relay only (fires during roller Up / Up Again
    phases).
  - **Horn** — the horn relay (fires during the conveyor Horn phase).
  - **Conveyor** — relay is on for the duration of the conveyor running. NOT
    for conveyor start/stop relays (those are Normal relays pulsed by
    momentary services).
- **Inactivity Check**: YES = the controller checks this relay is off before
  auto-stopping the conveyor for inactivity; NO = stops regardless.
- **Interlock Start**: seconds the conveyor must run before the function may
  turn on.
- **Interlock Stop**: seconds the conveyor must be off before the function
  turns off.
- **Look Back**: keeps the function on if a following vehicle is within the
  given distance (feet) behind the device. 0 = off.

### Function configuration (per relay)

- **Function Type**: how the relay times against the vehicle (below).
- **Device Position**: distance (feet) from the photo eyes to the device.
  Mis-measuring this mistimes the equipment.
- **Turn On Length** / **Turn Off Length**: read as sentences — "Turn on 8 ft
  Before the Front of the vehicle", "Turn off 20 ft After the Rear of the
  vehicle". References: Before/After × Front/Rear.

### Function types (all 10)

1. **Vehicle Length** — on for the entire length of the vehicle.
2. **Front Of Vehicle** — on only for the front of the vehicle.
3. **Rear Of Vehicle** — on only for the rear of the vehicle.
4. **Front Half Of Vehicle** — on for the front half only.
5. **Rear Half Of Vehicle** — on for the rear half only.
6. **All Tires** — engages only with the tire switch input (e.g. CTAs).
7. **Front Tires** — front tires only; **both** tire sets must be defined by
   the tire switch or it won't fire.
8. **Rear Tires** — rear tires only; same both-sets requirement.
9. **Pickup Bed** — listens to the upper entry input; if only a cab is
   detected, fires the relay only for the duration of the cab.
10. **Light** — on for the duration of the roller function; used for wash
    confirmation lights.

### Modifiers (retract/suppression windows inside a function)

Safety-critical: these control retracts that prevent vehicle damage. Each has
Start (feet) and Length (feet) values.

1. **Front & Rear Only** — function on for the front and rear of the vehicle
   only; skips the middle.
2. **Bump** — skip part of the vehicle. Common config: brush comes down for
   **7 ft**, then retracts for **11 ft**, then re-engages
   (Start 7, Length 11).
3. **Mirror Bump** — moves a wrap/side brush around the mirror. Common config:
   retract **2 ft 6 in** after the front of the vehicle, come back in after
   **1 ft 9 in** (Start 2.5, Length 1.75).
4. **Open Pickup Bed** — listen to the sonar for bump values; retract at the
   back of the cab for every open-bed truck. (Sim: cab end from
   `FlagMeasuringCarOpenBed` + upper-entry cab latch.)
5. **Rear Of Car** — turn off for the rear of the vehicle only; typical hitch
   retract. Common config: retract for **3 ft** starting at the rear.
6. **Front Of Car** — turn off for the front only; typical grill retract.
   Common config: wait **2 ft** after the front of the vehicle to turn on.

## Services page

Service types (all 11):

1. **Wash** — a wash package. Contains only the package's non-default relays;
   Default relays fire for every wash automatically (green check mark in the
   UI — don't add them to washes).
2. **Service** — a-la-carte add-ons and retract add-ons; adds relays to a
   wash.
3. **De-programmable** — removes a relay no matter the wash package. Used for
   non-default relays that must not ride along on lower-tier washes.
4. **Turn OFF** — turns off the selected relay **and everything after it** for
   the duration of the wash. Typically not used.
5. **Turn ON** — turns on the selected relay **and everything after it** for
   the duration of the wash. Typically not used.
6. **Momentarily On** — relay ON for a specified Time (optional Delay). Used
   for conveyor start/stop and wetdowns.
7. **Momentarily Off** — forces a function OFF for a specified time.
8. **Override** — forces relays on for the **entire vehicle length**; PBS
   rescue when a car entered without a required retract/function.
9. **Toggler** — light-switch toggle of relays; typically prep guns. (Real
   controller: appsetting `bstallcontroller` must be True.)
10. **Macro** — bundles several services as one button (e.g. conveyor start +
    roller), executed in order.
11. **Command** — issues a controller command (see PBS commands).

## Push Button Station (PBS)

- **Key lockout**: the PBS has a key switch. With the key off/removed the
  buttons are locked out — no services or commands can be sent. Turn the key
  on to unlock the station.
- **Commands glossary**:
  - **Accept Order** — sends the assembled order (selected services) to the
    controller queue.
  - **Cancel Order** — cancels the order currently sent.
  - **Remove Last** — removes the last order sent.
  - **Remove All** — clears the controller queue.
  - **Roller Abort** — cancels the roller request.
  - **Roller Request** — calls a roller up.

### Factory default button ↔ service map

| Service # | Name | Type | Relays |
|---|---|---|---|
| 1 | Basic Wash | Wash | (defaults only) |
| 2 | Better Wash | Wash | 9, 10 |
| 3 | Ceramic Wash | Wash | 9, 10, 11, 18 |
| 4 | Graphene + Ceramic Wash | Wash | 9, 10, 11, 12, 18 |
| 7 | Hitch Retract | Service | 20 |
| 8 | Grill Retract | Service | 21 |
| 9 | Tire Shine Retract | De-programmable | 18 |
| 10 | Tire Brush Retract | De-programmable | 10 |
| 11 | Full Top Retract | De-programmable | 7 |
| 12 | Buff & Dry Retract | De-programmable | 15 |
| 13 | Wrap Retract | De-programmable | 6 |
| 14 | Open Bed Retract | Service | 22 |
| 20 | Conveyor Start | Momentarily On | 23 (2 s) |
| 21 | Conveyor Stop | Momentarily On | 24 (2 s) |
| 22 | Wetdown | Momentarily On | 3 (30 s) |
| 23 | Dry Wetdown (LED Only) | Momentarily On | 17 (30 s) |
| 24 | Horn Blast | Momentarily On | 2 (2 s) |
| 25 | Roller Up | Command | Roller Request |

Conveyor tab designations in the default config: On Activation Button =
service 20, Shut Off Button = service 21, inactivity timeout 120 s, horn 3 s,
horn delay 1 s. Anti-collision: ghost relay 19, after-clears service 20,
slow-down time 3 s, slow-down horn service 24. Default Wash if None
Programmed = service 1.

### Factory default relay lineup (24 relays / 3 boards)

1 Rollers (Roller) · 2 Horn (Horn) · 3 Wetdown Arch (def, pos 5) ·
4 Presoak (def, pos 12) · 5 Tire Applicator CTA (def, All Tires, pos 15) ·
6 Wraps/Side Brushes (def, pos 25, Mirror Bump 2.5/1.75, interlock start 2 s) ·
7 Top Brush (def, pos 32, interlock start 2 s) · 8 High Pressure Water (def,
pos 40) · 9 Triple Foam (pos 48) · 10 Tire Brushes (pos 20) · 11 Ceramic
Sealant (pos 55) · 12 Graphene Coat (pos 58) · 13 Rinse Arch (def, pos 65) ·
14 Spot Free Rinse (def, pos 70) · 15 Buff & Dry Cloth (def, pos 78, interlock
start 2 s) · 16 Blowers (def, pos 90, on 4' before front / off 6' after rear,
inactivity check YES, interlock start 3 s, look back 10) · 17 Wash
Confirmation Light (Light fn) · 18 Tire Shine (All Tires, pos 74) ·
19 Anti Collision ghost (def, pos 100) · 20 Hitch Retract Solenoid (Rear Of
Vehicle, pos 78) · 21 Grill Retract Solenoid (Front Of Vehicle, pos 25) ·
22 Open Bed Retract Solenoid (Rear Half, pos 32) · 23 Conveyor Start ·
24 Conveyor Stop.

## Start/Stop board

Separate from the input boards. Five **STOP** circuits and five **START**
circuits:

- **STOP circuits are Normally Closed (NC)**: opening any circuit (E-stop
  pressed, wire cut) kills the conveyor immediately and blocks restart until
  re-closed. `SetStopCircuit(Circuit, bClosed)` — `bClosed=false` = opened.
- **START circuits are Normally Open (NO), momentary**: a press requests the
  conveyor start sequence (horn delay → horn → run).
  `PressStartCircuit(Circuit)`.

## Where things live in code

All under `Source/MLA_Tunnel_Sim/`:

| Concept | Code |
|---|---|
| Every enum/config/runtime struct (input types, relay types, function types, modifiers, service types, queue modes, `FMLControllerConfig`, `FMLCarState`...) | `Public/MicrologicTypes.h` |
| **All controller behavior** — pulses & car advancement (`EmitPulse`, `AdvanceCarsOnePulse`), measurement (`BeginCarMeasurement`/`FinishCarMeasurement`), order→car programming (`ApplyOrderToCar`, `ApplyServiceToCar`), relay window math (`EvaluateRelayWindow`, `IsSuppressedByModifiers`, `EvaluateRelays`), conveyor state machine (`RequestConveyorStart/Stop`, `UpdateConveyor`), anti-collision (`UpdateAntiCollision`), inactivity (`UpdateInactivity`), rollers (`RequestRoller`, `UpdateRollerSequenceOnPulse`), debounce (`UpdateInputsDebounce`), momentaries, togglers, commands, stop circuits | `Public/MicrologicSimCore.h` + `Private/MicrologicSimCore.cpp` (`FMicrologicSimCore` — pure C++, no UObjects, unit-testable via `Advance()` + `SetInputRaw()`) |
| Engine wrapper: GameInstance subsystem that ticks the core, staged-vs-live config, Commit / Commit+Reload / Reset / Backup / Restore, login validation, Blueprint API + dynamic events | `Public/MicrologicControllerSubsystem.h` + `Private/MicrologicControllerSubsystem.cpp` (`UMicrologicControllerSubsystem`) |
| **Factory default config** (input wiring, 24 relays, service table) | `MakeFactoryDefaultConfig()` at the bottom of `Private/MicrologicControllerSubsystem.cpp` — keep docs and UI in sync with it |
| Machine actor → relay binding (`RelayNumber`, `OnEnergized`/`OnDeEnergized`/`OnSignalChanged`, mirrors PHYSICAL relay incl. manual switches) | `Public/MicrologicDeviceComponent.h` |
| Sensor actor → input channel binding (`Channel` or `InputType`, `SetTriggered`) | `Public/MicrologicSensorComponent.h` |
| Config persistence between sessions (SaveGame slot `MicrologicControllerConfig`) | `Public/MicrologicSaveGame.h` |
| Backup/Restore XML serialization | `Public/MicrologicXml.h` + `Private/MicrologicXml.cpp` |

Sim-only knobs (`FMLSimSettings`, not on the real controller): tunnel length
120 ft, A/C pad position 110 ft, and `bAutoConveyorInterlockWire` (default
true — the sim wires input ch 2 to the conveyor's running state; set false to
train the missing-conveyor-interlock fault where the controller refuses to
engage a wash).

Behavior notes the code enforces that trip people up:

- Relay **manual switch** (ON/OFF/AUTO, `EMLRelayOverride`) sits between
  logical and physical relay state; `UMicrologicDeviceComponent` and
  `GetRelayState` report **physical**. Overrides and raw input wire states
  survive config reloads.
- Commit + Reload **keeps the queue** (`ResetRuntime` intentionally doesn't
  clear it); Restore only fills the staged config until Commit + Reload.
- Queue Mode None replaces the pending order; a car popping the eyes **binds
  the queue-head order immediately at eyes-break** (so near-entrance devices
  fire while measuring), else falls back to Default Wash, else a defaults-only
  ride. De-programmable removals apply after all additive services in the
  order, regardless of listed order (macros expanded first).
- Cars shorter than Minimum Car Length are discarded at measurement end and
  their consumed order returns to the queue head (unless Queue Mode None
  already holds a replacement); at Maximum Car Length the order completes
  mid-eyes.
- Directly-executed car-programming services (Wash/Service/Deprogrammable/
  TurnOn/TurnOff/Override via `ExecuteService`) apply to the **newest car in
  the tunnel** and are rejected when the tunnel is empty; momentary/toggler/
  macro/command types act globally.
- Turn ON/OFF cascade by relay **number** (>= the lowest selected relay).
