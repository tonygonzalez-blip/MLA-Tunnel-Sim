# MLA-Tunnel-Sim

Unreal Engine **5.6** training simulator for Micrologic's V2 car-wash tunnel
controller. Trainees run a virtual tunnel — configure the controller (inputs,
relays, services, settings), send washes, and watch the 3D machines respond —
exactly as they would on a real LogicWash V2 site. Primary user: Micrologic
trainers (not programmers); treat docs and UI text accordingly.

## Architecture

One runtime C++ module, `Source/MLA_Tunnel_Sim/`, in three layers:

1. **`FMicrologicSimCore`** (`MicrologicSimCore.h/.cpp`) — the controller
   brain. Pure C++: no UObjects, no world, no tick. Fed time via `Advance()`
   and wire states via `SetInputRaw()`; emits relay outputs and events.
   Deterministic and headless-testable.
2. **`UMicrologicControllerSubsystem`** (GameInstance subsystem) — engine
   wrapper. Ticks the core, exposes the full Blueprint API and dynamic events,
   owns staged-vs-live config with Commit / Commit+Reload / Reset / Backup /
   Restore (XML via `MicrologicXml`, persistence via `MicrologicSaveGame`).
   `MakeFactoryDefaultConfig()` at the bottom of the subsystem .cpp is the
   canonical default wiring/relay/service table — keep docs in sync with it.
3. **World components + UI** — `UMicrologicDeviceComponent` (machine actor ←
   relay), `UMicrologicSensorComponent` (sensor actor → input channel), and
   Blueprint UMG widgets for the controller UI (built per
   `docs/UI_PLAYBOOK.md`).

Supporting types (all enums/config/runtime structs) live in
`MicrologicTypes.h`.

Blueprint content lives in `Content/MyContent/Blueprints/` (machines under
`ConfigurableMachines/`). The legacy Blueprint controller
(`ConfigMachine_BPFL`, `MasterMachine_BP` wiring, old `UI_ML` widgets,
`RelayProperties_SRUCT`) is being retired — see `docs/WORLD_WIRING.md`; do not
extend it.

## Key rule

**ALL controller behavior belongs in `FMicrologicSimCore`.** Never implement
or duplicate controller logic (timing, measurement, relay windows, queueing,
safety interlocks) in Blueprints or widgets — they only display state and
forward user actions to the subsystem. If a Blueprint needs the controller to
do something new, add it to the core (with a test) and expose it through the
subsystem.

## Build & test

- Windows only. Requires Visual Studio 2022 ("Game development with C++"
  workload + Windows SDK) and UE 5.6 from the Epic Launcher.
- Build: open `MLA_Tunnel_Sim.uproject` and accept the rebuild prompt; or
  right-click the .uproject → Generate Visual Studio project files → build
  `Development Editor | Win64` in the .sln.
- This cloud/CLI environment has **no engine installed** — code cannot be
  compiled here. Keep changes conservative and verifiably correct by reading;
  the user compiles on their Windows PC. From a running editor, prefer Live
  Coding via the unreal-mcp skill.
- Tests: Automation tests under the `MicrologicSim` pretty-name prefix.
  In-editor: Window → Test Automation → filter `MicrologicSim` → run.
  Headless:
  `UnrealEditor-Cmd.exe <path>\MLA_Tunnel_Sim.uproject -ExecCmds="Automation RunTests MicrologicSim; Quit" -unattended -nullrhi -nosplash`
  (results in `Saved/Logs/MLA_Tunnel_Sim.log`).

## Docs & skills

- `docs/INTEGRATION.md` — zip → compiled module → tests running (trainer-facing).
- `docs/WORLD_WIRING.md` — connecting world Blueprints to the subsystem:
  machine→relay map, sensor wiring, car driving pattern, legacy retirement,
  smoke test.
- `docs/UI_PLAYBOOK.md` — building the controller UI widgets.
- `.claude/skills/micrologic-spec` — the condensed V2 controller domain spec
  (relays/inputs/services/functions/modifiers semantics + code map). Load it
  for any controller-behavior work.
- `.claude/skills/unreal-mcp` — live-editor MCP workflow for in-editor changes
  (Blueprints, widgets, levels, Live Coding) when the user has the editor open.
