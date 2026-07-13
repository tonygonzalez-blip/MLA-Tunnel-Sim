# World Wiring: Connecting the Tunnel to the Controller

This guide connects the existing 3D tunnel (the machine and sensor Blueprints
already in the project) to the new C++ controller. When you finish, the
brushes, blowers, and arches run because a **relay** energized — exactly like a
real wash — and the photo eyes, tire switch, and E-stop feed real **inputs**.

Prerequisite: the C++ module builds and the tests pass
(see `docs/INTEGRATION.md`).

## How it fits together

```
  3D WORLD                          CONTROLLER (C++)                    3D WORLD
  --------                          ----------------                    --------
  PhotoEyes_BP,                                                          TopBrush_BP,
  tire switch,          input                            relay           Blower_BP,
  A/C pad, stall   -->  channels  -->  sim core logic  -->  outputs  --> sprayers, ...
  (each has a           (1-16)         (measures cars,     (1-24)       (each has a
  Micrologic Sensor                    times functions)                 Micrologic Device
  component)                                                            component)

  E-stop button  ------------------>  start/stop board (separate from inputs)
```

- A **Micrologic Sensor** component turns a world event (car blocks the eyes)
  into an input channel going ON/OFF.
- The **controller subsystem** (`UMicrologicControllerSubsystem`, always
  running during play) does everything the real controller does.
- A **Micrologic Device** component listens to one relay and tells its machine
  to start or stop.

The controller UI (dashboard, Relays page, etc.) watches the same subsystem, so
what you see on screen and what the tunnel does always agree.

## Machine-to-relay map (factory default config)

These are **suggested** relay numbers matching the factory default
configuration built into the controller. Every one of them can be changed later
from the controller UI's Relays page — that is the whole point of the trainer —
but start with this map so the default config drives the tunnel correctly.

| Machine Blueprint (Content/MyContent/Blueprints/ConfigurableMachines/) | Suggested relay | Relay name in default config |
|---|---|---|
| `TunnelConveyorRollers_BP` | 1 | Rollers |
| `SoapSprayer_BP` / `WaterSprayer_BP` (entrance arches) | 3 / 4 | Wetdown Arch / Presoak |
| `TireApplicator_BP` | 5 | Tire Applicator (CTA) |
| `BrushArms_BP` / `Brushs_BP` / `MacNeilBrushArm_BP` | 6 | Wraps / Side Brushes |
| `TopBrush_BP` / `TopBrushArm_BP` | 7 | Top Brush |
| `BrushSprayers_BP` (per instance) | 8 or 9 | High Pressure Water / Triple Foam |
| `TireBrushes_BP` / `TireBrushAir_BP` / `TireBrushSprayers_BP` | 10 | Tire Brushes |
| `Blower_BP` / `Blowers_BP` | 16 | Blowers |
| `WelcomeArchLight_BP` / `WaitGo_BP` | 17 | Wash Confirmation Light |
| `TireShine_BP` | 18 | Tire Shine |

Unassigned relays (2 Horn, 11 Ceramic Sealant, 12 Graphene Coat, 13 Rinse
Arch, 14 Spot Free Rinse, 15 Buff & Dry Cloth, 20-22 retract solenoids) can be
mapped to spare sprayer/arch instances or left unwired for now — the dashboard
still shows them firing. Relay 2 (Horn) is worth wiring early: point it at an
actor that plays a horn sound so the smoke test's pre-start horn is audible.

## Wiring one machine (editor steps)

For each machine Blueprint in the table:

1. Open the Blueprint (double-click it in the Content Browser).
2. Click **Add** (green button, Components panel) → search **Micrologic
   Device** → add it.
3. Select the new component. In the Details panel, set **Relay Number** to the
   machine's relay from the table.
4. Still in Details, scroll to the **Micrologic** events section. Click **+**
   next to **On Energized** and **+** next to **On De-Energized**. Two event
   nodes appear in the Event Graph.
5. Wire **On Energized** to whatever the machine already uses to start
   (spin the brush, start the spray particles, play the sound), and
   **On De-Energized** to whatever stops it. (There is also **On Signal
   Changed** with a boolean if the machine has a single Set-Running function.)
6. Compile, Save.

Repeat for every machine. This is repetitive point-and-click work — a good job
for the in-editor agent. With the editor open, start a local Claude Code
session in the project folder (`claude` in a terminal) and say:

> "Using the unreal-mcp skill, add a MicrologicDeviceComponent to every machine
> Blueprint per the table in docs/WORLD_WIRING.md, set each Relay Number, and
> wire OnEnergized/OnDeEnergized to each machine's existing start/stop logic."

## Wiring the sensors

Sensors use the **Micrologic Sensor** component. Add it the same way (Add
Component → Micrologic Sensor), then from the Blueprint's existing overlap or
button logic call the component's **Set Triggered** function: `true` when the
sensor is blocked/pressed, `false` when it clears.

| World actor | Component setup | Notes |
|---|---|---|
| `PhotoEyes_BP` (entry eyes) | Input Type = **Entry** (or Channel = 3) | Car blocks eyes → `SetTriggered(true)`; clears → `false`. The controller measures the car from how long the eyes stay blocked while the conveyor pulses. |
| Upper entry eyes (second, higher pair at the entrance) | Channel = **6** (Upper Entry) | Detects truck cabs/beds. If these clear while the lower eyes are still blocked, the controller records the cab length. |
| `EstopButton_BP` | **No sensor component.** | The E-stop is on the start/stop board, not the input board. On press: call **Set Stop Circuit** (Circuit 1, Closed = false) on the Micrologic Controller Subsystem. On reset/twist-release: Circuit 1, Closed = true. Stop circuits are Normally Closed — opening one kills the conveyor instantly. |
| Anti-collision pad (add a trigger volume at the tunnel end, around the 110 ft mark) | Input Type = **Anti Collision** (or Channel = 7) | Triggered while a car sits on the pad at the exit. |
| Conveyor stall demo switch (a trainer-flippable switch) | Channel = **9** (Stall) | While ON, the conveyor stops and refuses to restart — a classic teaching fault. |
| Tire switch (optional, at the CTA) | Channel = **5** (Tire Switch) | Pulse `true`/`false` as each tire passes. Drives All Tires / Front Tires / Rear Tires functions. |

The **Conveyor Interlock** input (channel 2) is wired automatically by the sim
(`bAutoConveyorInterlockWire` is on by default), so you do not need a world
actor for it. Turning that setting off is itself a training fault: the
controller will refuse to engage washes, just like a real site with a missing
conveyor interlock wire.

## Driving the visual cars

The controller is the source of truth for where every car is. Car positions
come from `GetCars()` on the subsystem: each entry has a **Car Id** and
**Front Position Feet** — feet the front bumper has traveled past the entry
eyes. The pattern:

1. **Spawn** a visual car actor when the subsystem's **On Car Entered** event
   fires. Remember the Car Id.
2. **Each tick**, call **Get Cars** and, for each car, convert
   `FrontPositionFeet` to a distance along the tunnel spline/track and move the
   matching visual car there. (Position on spline = feet × 30.48 to get
   centimeters, measured from where the spline passes the entry eyes.)
3. **Despawn** the visual car when **On Car Exited** fires with its Car Id.

`CarSpawner_BP` starts the process: it should place the car just before the
eyes and drive it forward so it **physically overlaps the `PhotoEyes_BP`
trigger** — the Entry sensor going ON/OFF is what makes the controller start
and stop measuring. Keep the spawned car overlapping the eyes for its whole
length; once the eyes clear, the controller takes over and the visual car
should follow `GetCars()` as above.

## CRITICAL: retire the old Blueprint controller

The tunnel currently has an older, Blueprint-only controller: the
`ConfigMachine_BPFL` function-library calls inside `MasterMachine_BP`, the old
`UI_ML` / `RelaysList` / `EditRelay` widgets, and the `RelayProperties_SRUCT` /
`ModifierProperties_STRUCT` structs.

**That logic must STOP driving the machines.** If both the old Blueprint path
and the new relay components command the same machine, it will double-fire:
turn on twice, fight over timing, or refuse to turn off.

1. Open `MasterMachine_BP` and disconnect the event-graph wiring that calls
   into `ConfigMachine_BPFL` / drives machine start-stop. Disconnect the wires;
   do not delete the nodes yet.
2. Stop opening the old `UI_ML` / `RelaysList` / `EditRelay` widgets from any
   level or player Blueprint.
3. **Keep the assets** for reference while the new path is proven. Delete them
   in a later cleanup pass once training runs cleanly on the new controller.

## End-to-end smoke test

Run this after wiring. It touches every layer.

1. Press **Play** (PIE).
2. Open the controller UI. Log in: username **manager**, password
   **manager01**. You land on the dashboard.
3. From the PBS or Visual Queue, send **service 2 (Better Wash)**. The order
   appears in the controller queue on the dashboard.
4. Start the conveyor (start button / service 20). You get a 1-second delay,
   a 3-second horn, then the conveyor runs.
5. Drive or spawn a car through the entry eyes. Watch the dashboard measure it,
   then watch the relays fire down the tunnel in position order: Wetdown (3),
   Presoak (4), CTA (5, at 15 ft), Tire Brushes (10, at 20 ft — fires here
   because Better Wash adds it), Wraps (6), Top Brush (7), High Pressure (8),
   Triple Foam (9, also a Better Wash add-on), then rinses, Buff & Dry,
   Blowers (16). The machines in the world should run in the same order —
   note Tire Brushes energizing right after the CTA is correct, not a wiring
   error, because its device position (20 ft) sits between the CTA and the
   Wraps.
6. Flip **relay 7's** manual board switch to **ON** (Relays board in the UI).
   The top brush now runs constantly, regardless of any car. Flip it back to
   **AUTO** and it returns to normal.
7. Hit the **E-stop**. The conveyor dies instantly and the dashboard shows the
   stop reason: **Stop Circuit / E-Stop**. Reset the E-stop and restart the
   conveyor to confirm recovery.

If every step behaves, the world is wired. Anything off — a machine that
double-fires (see the retirement step above), a relay that never energizes
(wrong Relay Number), or a car that is never measured (Entry sensor not
triggering) — is a five-minute fix for a local Claude Code session with the
editor open.
