# Micrologic Controller UI — Widget Blueprint Playbook

How to build the Widget Blueprints over the C++ UMG bases in
`Source/MLA_Tunnel_Sim/Public/UI/`. All logic lives in C++; the Widget
Blueprints only lay out named subwidgets and set styling. Every binding below
is `BindWidgetOptional` — a partial layout still runs — but a widget that is
not placed (or is misnamed/mistyped) simply does nothing, so treat these
tables as the contract. **Names and types must match exactly.**

Conventions used by all screens:

- LED-style `UBorder` widgets are recolored by C++: on = `(0.1, 0.8, 0.2)`
  green, off = `(0.15, 0.15, 0.15)` dim gray, fault/stop red =
  `(0.9, 0.15, 0.1)`.
- ComboBoxes are populated by C++ in enum order — leave their option lists
  empty in the designer.
- Rows (`*Row` classes) are created by C++ via `CreateWidget` into a
  ScrollBox; the screen's `*RowClass` property (Class Defaults) must be set
  to your row Widget Blueprint.
- Screens subscribe to controller events and unsubscribe on destruct; no
  Blueprint tick logic is needed anywhere.

---

## UMLControllerScreenBase

Shared base (no bindings). Gives every screen `GetController()`
(BlueprintPure) and the `RefreshFromController()` hook, called once from
`NativeConstruct`.

## UMLLoginScreen

| Binding | Type | Purpose |
|---|---|---|
| `EditableTextBox_Username` | EditableTextBox | Username entry (default `manager`) |
| `EditableTextBox_Password` | EditableTextBox | Password entry (set IsPassword; default `manager01`) |
| `Button_Login` | Button | Validates via `ValidateLogin` |
| `TextBlock_Error` | TextBlock | Shows "Invalid username or password." on failure |

Event: `OnLoginSucceeded` (BlueprintAssignable) — the shell binds this in C++.

## UMLControllerShellWidget

| Binding | Type | Purpose |
|---|---|---|
| `TextBlock_AddressBar` | TextBlock | Set to `http://10.0.1.90/` by C++ |
| `Widget_Login` | UMLLoginScreen (your WBP) | Login page inside root switcher index 0 |
| `WidgetSwitcher_Root` | WidgetSwitcher | 0 = login page, 1 = main app |
| `WidgetSwitcher_Screens` | WidgetSwitcher | 0 Dashboard, 1 Settings, 2 Inputs, 3 Relays, 4 Services, 5 Hardware Boards |
| `Button_NavDashboard` | Button | Switch to screen 0 |
| `Button_NavSettings` | Button | Switch to screen 1 |
| `Button_NavInputs` | Button | Switch to screen 2 |
| `Button_NavRelays` | Button | Switch to screen 3 |
| `Button_NavServices` | Button | Switch to screen 4 |
| `Button_NavBoards` | Button | Switch to screen 5 |
| `Border_LockOverlay` | Border | Full-screen VNC-style lock overlay (topmost in an Overlay panel) |
| `EditableTextBox_UnlockPassword` | EditableTextBox | Unlock password entry (IsPassword) |
| `Button_Unlock` | Button | Validates via `ValidateUiPassword` |

Behavior baked into C++: nav clicks reset the idle timer; after 300 s idle
while logged in the UI auto-locks; `LockUi()` is BlueprintCallable (bind it
to an "L" key press or a lock button if desired).

## UMLDashboardScreen

| Binding | Type | Purpose |
|---|---|---|
| `TextBlock_ConveyorState` | TextBlock | Stopped / Horn Delay / Horn / Running / Slowing Down (green when running, red otherwise) |
| `TextBlock_StopReason` | TextBlock | Last stop reason (blank while running) |
| `TextBlock_QueueCount` | TextBlock | Number of queued orders (number only — add your own label) |
| `Button_ConveyorStart` | Button | Executes the Conveyor tab's On Activation service |
| `Button_ConveyorStop` | Button | Executes the Shut Off service |
| `UniformGridPanel_Relays` | UniformGridPanel | Relay LEDs, built by C++, 8 per row, tooltip = description |
| `UniformGridPanel_Inputs` | UniformGridPanel | Input-channel LEDs, same pattern |
| `ScrollBox_Cars` | ScrollBox | Text rows: `Car 3 — 17.2 ft @ 42.5 ft — services: 1,9` (refreshed 4×/s) |
| `ScrollBox_Queue` | ScrollBox | Text rows: `Order 12 — services: 1,9` |
| `Border_PulseLed` | Border | Blinks green on each conveyor pulse |

## UMLInputsScreen

Class default: `InputRowClass` = your `WBP_MLInputRow`.

| Binding | Type | Purpose |
|---|---|---|
| `ScrollBox_Inputs` | ScrollBox | One `UMLInputRow` per staged input |
| `Button_Save` | Button | `Commit()` — writes staged edits to the live controller |

## UMLInputRow

| Binding | Type | Purpose |
|---|---|---|
| `TextBlock_Channel` | TextBlock | Channel number |
| `EditableTextBox_Description` | EditableTextBox | Description; commits on Enter/focus-loss |
| `ComboBoxString_Type` | ComboBoxString | None / Trigger / Conveyor / Roller Position / Tire Switch / Upper Entry / Anti Collision / Stall / Entry / Exit Door |
| `CheckBox_Inverted` | CheckBox | Inverted flag |
| `SpinBox_Debounce` | SpinBox | Debounce seconds (0 = default debounce) |
| `SpinBox_TriggerService` | SpinBox | Service fired when a Trigger-type input activates |
| `Border_Led` | Border | Live committed input state |

Every edit immediately updates the STAGED config (`UpsertStagedInput`);
nothing hits the live controller until Save/Commit.

## UMLRelaysScreen

Class default: `RelayRowClass` = your `WBP_MLRelayRow`.

| Binding | Type | Purpose |
|---|---|---|
| `ScrollBox_Relays` | ScrollBox | One `UMLRelayRow` per staged relay |
| `Widget_EditPanel` | UMLRelayEditPanel (your WBP) | Shared edit form; hidden until a row's Edit is clicked |
| `Button_AddRelay` | Button | Opens the edit panel with the next free relay number |

## UMLRelayRow

| Binding | Type | Purpose |
|---|---|---|
| `TextBlock_Number` | TextBlock | Relay number |
| `TextBlock_Description` | TextBlock | Description |
| `TextBlock_Type` | TextBlock | Normal / Roller / Horn / Conveyor |
| `TextBlock_Default` | TextBlock | "Yes" / "No" |
| `Border_Led` | Border | Live PHYSICAL relay state |
| `Button_Edit` | Button | Opens the screen's edit panel for this relay |
| `Button_SwitchAuto` | Button | Manual switch → AUTO (active position is disabled/highlighted) |
| `Button_SwitchOn` | Button | Manual switch → ON |
| `Button_SwitchOff` | Button | Manual switch → OFF |
| `EditableTextBox_Pin` | EditableTextBox | PIN sent with `SetRelayOverride` when Security requires one |

A rejected PIN silently leaves the switch in its actual position (the row
re-reads `GetRelayOverride`).

## UMLRelayEditPanel

Class default: `ModifierRowClass` = your `WBP_MLModifierRow`.

| Binding | Type | Purpose |
|---|---|---|
| `TextBlock_RelayNumber` | TextBlock | Relay number being edited (not editable) |
| `CheckBox_Active` | CheckBox | Active |
| `EditableTextBox_Description` | EditableTextBox | Description |
| `CheckBox_Default` | CheckBox | Default (fires every wash) |
| `ComboBoxString_Type` | ComboBoxString | Normal / Roller / Horn / Conveyor |
| `CheckBox_InactivityCheck` | CheckBox | Inactivity Check |
| `SpinBox_InterlockStart` | SpinBox | Interlock Start seconds |
| `SpinBox_InterlockStop` | SpinBox | Interlock Stop seconds |
| `SpinBox_LookBack` | SpinBox | Look Back feet |
| `ComboBoxString_FunctionType` | ComboBoxString | None / Vehicle Length / Front Of Vehicle / ... / Light |
| `SpinBox_DevicePosition` | SpinBox | Device Position feet |
| `SpinBox_TurnOnFeet` | SpinBox | Turn On Length feet |
| `ComboBoxString_TurnOnRef` | ComboBoxString | Before/After Front/Rear Of Vehicle |
| `SpinBox_TurnOffFeet` | SpinBox | Turn Off Length feet |
| `ComboBoxString_TurnOffRef` | ComboBoxString | Before/After Front/Rear Of Vehicle |
| `ScrollBox_Modifiers` | ScrollBox | One `UMLModifierRow` per modifier |
| `Button_AddModifier` | Button | Appends a default modifier |
| `Button_Save` | Button | `UpsertStagedRelay` + notifies the screen (OnSaved) |
| `Button_Cancel` | Button | Discards the working copy |

Public API: `OpenForRelay(Config)` / `OpenForNew(SuggestedNumber)` — both
BlueprintCallable; the panel shows/hides itself.

## UMLModifierRow

| Binding | Type | Purpose |
|---|---|---|
| `ComboBoxString_Type` | ComboBoxString | Front & Rear Only / Bump / Mirror Bump / Open Pickup Bed / Rear Of Car / Front Of Car |
| `SpinBox_Start` | SpinBox | Start feet |
| `SpinBox_Length` | SpinBox | Length feet |
| `Button_Remove` | Button | Removes this modifier from the panel's working copy |

## UMLServicesScreen

Class default: `ServiceRowClass` = your `WBP_MLServiceRow`.

| Binding | Type | Purpose |
|---|---|---|
| `ScrollBox_Services` | ScrollBox | One `UMLServiceRow` per staged service |
| `Widget_EditPanel` | UMLServiceEditPanel (your WBP) | Shared edit form |
| `Button_AddService` | Button | Opens the edit panel with the next free service number |

## UMLServiceRow

| Binding | Type | Purpose |
|---|---|---|
| `TextBlock_Number` | TextBlock | Service number |
| `TextBlock_Description` | TextBlock | Description |
| `TextBlock_Type` | TextBlock | Wash / Service / Macro / De-programmable / ... / Command |
| `Button_Edit` | Button | Opens the screen's edit panel |
| `Button_Send` | Button | `ExecuteService` on the LIVE controller — trainees fire services from here |

## UMLServiceEditPanel

| Binding | Type | Purpose |
|---|---|---|
| `SpinBox_ServiceNumber` | SpinBox | Service number |
| `EditableTextBox_Description` | EditableTextBox | Description |
| `ComboBoxString_Type` | ComboBoxString | The 11 service types |
| `EditableTextBox_Relays` | EditableTextBox | CSV relay list, e.g. `9,10,18` (non-numeric entries ignored) |
| `SpinBox_Time` | SpinBox | Momentary Time seconds |
| `SpinBox_Delay` | SpinBox | Momentary Delay seconds |
| `EditableTextBox_MacroServices` | EditableTextBox | CSV service list for Macro type |
| `ComboBoxString_Command` | ComboBoxString | None / Accept Order / Cancel Order / Remove Last / Remove All / Roller Abort / Roller Request |
| `Button_Save` | Button | `UpsertStagedService` |
| `Button_Cancel` | Button | Discard |
| `Button_Delete` | Button | `RemoveStagedService` |

Public API: `OpenForService(Config)` / `OpenForNew(SuggestedNumber)`.

## UMLSettingsScreen

| Binding | Type | Purpose |
|---|---|---|
| `WidgetSwitcher_Tabs` | WidgetSwitcher | 0 Communications, 1 Conveyor, 2 Anti-Collision, 3 Roller/Defaults, 4 Security, 5 Backup/Restore, 6 Sonar |
| `Button_TabCommunications` | Button | Tab 0 |
| `Button_TabConveyor` | Button | Tab 1 |
| `Button_TabAntiCollision` | Button | Tab 2 |
| `Button_TabRollerDefaults` | Button | Tab 3 |
| `Button_TabSecurity` | Button | Tab 4 |
| `Button_TabBackupRestore` | Button | Tab 5 |
| `Button_TabSonar` | Button | Tab 6 |

Each tab is its own Widget Blueprint (parents below) placed as the matching
switcher child. Every tab's `Button_Save` stages that tab's section and
commits (matching the real UI's per-tab Save at the bottom right). Tabs
reload from the staged config on `OnConfigChanged`.

## UMLSettingsCommunicationsTab

| Binding | Type | Purpose |
|---|---|---|
| `SpinBox_NumInputBoards` | SpinBox | 16 channels per board |
| `SpinBox_NumRelayBoards` | SpinBox | 8 relays per board |
| `EditableTextBox_InputBoardPort` | EditableTextBox | e.g. COM1 |
| `EditableTextBox_OutputBoardPort` | EditableTextBox | e.g. COM2 |
| `EditableTextBox_KeypadPort` | EditableTextBox | e.g. COM3 |
| `CheckBox_ExitDoor` | CheckBox | Exit Door feature enable |
| `Button_Save` | Button | Stage + Commit |

## UMLSettingsConveyorTab

| Binding | Type | Purpose |
|---|---|---|
| `SpinBox_IPS` | SpinBox | Inches Per Second |
| `SpinBox_IPP` | SpinBox | Inches Per Pulse |
| `SpinBox_OnActivationService` | SpinBox | Conveyor-start service (0 = disabled) |
| `SpinBox_ShutOffService` | SpinBox | Conveyor-stop service (0 = disabled) |
| `SpinBox_InactivityTimeout` | SpinBox | Seconds (0 = disabled) |
| `SpinBox_HornTime` | SpinBox | Seconds (0 = disabled) |
| `SpinBox_HornDelay` | SpinBox | Seconds (0 = disabled) |
| `Button_Save` | Button | Stage + Commit |

## UMLSettingsAntiCollisionTab

| Binding | Type | Purpose |
|---|---|---|
| `SpinBox_Relay` | SpinBox | Ghost relay number |
| `SpinBox_AfterClearsService` | SpinBox | Conveyor-restart service after A/C clears |
| `SpinBox_SlowDownService` | SpinBox | Slow-down service |
| `SpinBox_SlowDownTime` | SpinBox | Seconds before stop (0 = immediate) |
| `SpinBox_SlowDownHornService` | SpinBox | Horn service during slow-down |
| `CheckBox_ConveyorStall` | CheckBox | Conveyor Stall input enabled (YES = stall stops the conveyor and blocks starts) |
| `Button_Save` | Button | Stage + Commit |

## UMLSettingsRollerDefaultsTab

| Binding | Type | Purpose |
|---|---|---|
| `SpinBox_MinCarLength` | SpinBox | Micrologic preset (6 ft) — editable but leave alone |
| `SpinBox_MaxCarLength` | SpinBox | Preset (25 ft) |
| `SpinBox_AverageCarLength` | SpinBox | Preset (15 ft) |
| `ComboBoxString_RollerMode` | ComboBoxString | Manual/Front, Automatic/Rear |
| `SpinBox_Up` | SpinBox | Up feet |
| `SpinBox_Down` | SpinBox | Down feet |
| `SpinBox_UpAgain` | SpinBox | Up Again feet |
| `CheckBox_NeedsCarQueued` | CheckBox | Deny roller requests with empty queue |
| `SpinBox_DefaultDebounce` | SpinBox | Default Input Debounce seconds |
| `ComboBoxString_QueueMode` | ComboBoxString | None / Random / Sequential |
| `SpinBox_ServiceOnQueued` | SpinBox | Service fired per queued order |
| `SpinBox_DefaultWash` | SpinBox | Default Wash if None Programmed |
| `Button_Save` | Button | Stage + Commit |

## UMLSettingsSecurityTab

| Binding | Type | Purpose |
|---|---|---|
| `CheckBox_RequirePin` | CheckBox | Require PIN for relay overrides |
| `EditableTextBox_Pin` | EditableTextBox | The PIN code |
| `EditableTextBox_UiPassword` | EditableTextBox | Native (VNC) UI unlock password |
| `Button_Save` | Button | Stage + Commit (web login credentials untouched) |

## UMLSettingsBackupRestoreTab

| Binding | Type | Purpose |
|---|---|---|
| `EditableTextBox_BackupName` | EditableTextBox | Name for the next backup file |
| `Button_Backup` | Button | Writes live config XML; status shows the path |
| `ScrollBox_Backups` | ScrollBox | Backup files as clickable rows (built by C++; click selects + highlights blue) |
| `Button_Restore` | Button | Loads the selected file into the STAGED config |
| `Button_Reset` | Button | Factory defaults (staged + live) |
| `Button_Commit` | Button | Apply staged changes without reload |
| `Button_CommitReload` | Button | Apply + reload (queue kept) |
| `TextBlock_Status` | TextBlock | Feedback: "Backup written to ...", "Restored — press Commit + Reload to apply", errors |

## UMLSettingsSonarTab

| Binding | Type | Purpose |
|---|---|---|
| `EditableTextBox_SonarAddress` | EditableTextBox | Sonar IP (typically 10.0.1.95) |
| `Button_Save` | Button | Stage + Commit (rest of Communications untouched) |
| `TextBlock_Note` | TextBlock | C++ sets: "If this location does not utilize Micrologic sonar, this section can be ignored." |

## UMLPushButtonStation

| Binding | Type | Purpose |
|---|---|---|
| `Button_1` … `Button_5` | Button | Wash buttons; locked out (disabled) while the key is OFF |
| `Button_7` … `Button_11` | Button | Retract/add-on services |
| `Button_13` … `Button_17` | Button | Retract/add-on services |
| `Button_19` … `Button_22` | Button | Conveyor Start/Stop, Wetdown, Horn... (the physical panel skips 6/12/18) |
| `Button_Key` | Button | Toggles the key switch |
| `Button_Enter` | Button | Sends the assembled order (`SendOrder`) |
| `TextBlock_Display` | TextBlock | Pending order, e.g. `2 + 9 + 13` |
| `Border_KeyLed` | Border | Key state LED |

Button N fires service N from the LIVE config. Wash-type press sets the
pending wash; Momentarily On / Command types execute immediately; everything
else toggles into the pending extras. Unmapped buttons are disabled.

## UMLHardwareBoardsScreen

Class default: `RelaySwitchRowClass` = your `WBP_MLRelaySwitchRow`.

| Binding | Type | Purpose |
|---|---|---|
| `UniformGridPanel_InputLeds` | UniformGridPanel | 16 channel LEDs built by C++ (tooltip = description) |
| `Border_PulseLed` | Border | Lit while `GetTimeSinceLastPulse() < 0.15 s` |
| `ScrollBox_RelaySwitches` | ScrollBox | One `UMLRelaySwitchRow` per LIVE relay |
| `EditableTextBox_Pin` | EditableTextBox | Shared override PIN for all switch rows |
| `CheckBox_Stop1` … `CheckBox_Stop5` | CheckBox | Stop circuits — checked = closed (NC); uncheck one and watch the conveyor die |
| `Button_Start` | Button | `PressStartCircuit(1)` (momentary) |
| `Border_StarterLed` | Border | Lit while the conveyor is running |
| `Border_HornLed` | Border | Lit during the Horn phase |

## UMLRelaySwitchRow

| Binding | Type | Purpose |
|---|---|---|
| `TextBlock_Relay` | TextBlock | `7 — Top Brush` |
| `Button_Auto` / `Button_On` / `Button_Off` | Button | 3-position manual switch (active position disabled) |
| `Border_Led` | Border | Physical relay state |

---

# Building the Widget Blueprints (Claude Code + unreal-mcp)

For a local session with the `unreal-mcp` skill and the editor open on
`MLA_Tunnel_Sim.uproject`:

1. **Compile first.** These classes are new C++ — build the module (or Live
   Coding) before creating assets so the parent classes exist.
2. **Create leaf widgets first**, under `/Game/UI/Micrologic/`:
   `WBP_MLInputRow`, `WBP_MLRelayRow`, `WBP_MLModifierRow`,
   `WBP_MLRelayEditPanel`, `WBP_MLServiceRow`, `WBP_MLServiceEditPanel`,
   `WBP_MLRelaySwitchRow`, the 7 settings tabs, `WBP_MLLoginScreen`. Set each
   Widget Blueprint's **parent class** to the matching `UML*` C++ class, then
   add child widgets named EXACTLY per the tables above (the binding is
   by name + type).
3. **Then screens**: `WBP_MLDashboardScreen`, `WBP_MLInputsScreen`,
   `WBP_MLRelaysScreen`, `WBP_MLServicesScreen`, `WBP_MLSettingsScreen`
   (placing the 7 tab WBPs inside `WidgetSwitcher_Tabs` in the documented
   order), `WBP_MLHardwareBoardsScreen`, `WBP_MLPushButtonStation`.
4. **Set the row classes** in each screen's Class Defaults:
   `InputRowClass`, `RelayRowClass`, `ModifierRowClass` (on the relay edit
   panel), `ServiceRowClass`, `RelaySwitchRowClass`.
5. **Finally the shell**: `WBP_MLControllerShell` (parent
   `UMLControllerShellWidget`). Root layout: an Overlay containing
   (a) a vertical box with the address bar strip (`TextBlock_AddressBar`),
   the nav button strip, and `WidgetSwitcher_Root` — child 0 =
   `Widget_Login` (an instance of `WBP_MLLoginScreen`), child 1 = the main
   app (nav strip + `WidgetSwitcher_Screens` holding the six screens in
   order Dashboard, Settings, Inputs, Relays, Services, Hardware Boards) —
   and (b) `Border_LockOverlay` as the topmost Overlay child (semi-opaque,
   contains `EditableTextBox_UnlockPassword` + `Button_Unlock`), visibility
   Collapsed by default.
6. **Add to viewport**: the simplest wiring is a `Create Widget`
   (WBP_MLControllerShell) + `Add to Viewport` in the existing player
   controller's BeginPlay (or a minimal HUD/level Blueprint), plus
   `Set Input Mode Game and UI`. The Push Button Station is a separate
   widget — either a second viewport widget toggled by a key, or a
   WidgetComponent on the physical station mesh in the tunnel.
7. **Verify bindings**: after each WBP, confirm no "BindWidget" warnings on
   compile and that names match the tables; a silent no-op usually means a
   name/type mismatch.

## Styling

Match the real Micrologic V2 web UI (reference videos: shared Drive,
**"Controller UI/Software"** folder):

- **White page background**, near-black text.
- **Micrologic blue `#2B5DD7`** for primary buttons, nav highlights, the
  selected backup row, and links/accents.
- State colors must match the C++ constants: on-green `(0.1, 0.8, 0.2)`,
  off-gray `(0.15, 0.15, 0.15)`, fault-red `(0.9, 0.15, 0.1)` — do not
  restyle LED Borders' brush color in the designer (C++ overwrites it).
- Browser chrome on the shell: light-gray toolbar strip, rounded address-bar
  box showing `http://10.0.1.90/`.
- The Push Button Station is NOT a web page — style it like the physical
  box: dark panel, chunky square buttons with the number engraved, red
  7-segment-style display font for `TextBlock_Display`.
- Keep tables/rows dense (the real UI is a compact data grid): 4-8 px cell
  padding, 1 px light-gray row separators.
