# From Zip to Running Sim: Setting Up the New C++ Module

This guide takes you from unzipping the project to a working simulator with the
new Micrologic controller code compiled and running. Follow it top to bottom the
first time. After that, opening the project is just a double-click.

## What changed

The project now has a `Source/` folder with C++ code. This is the new
**Micrologic V2 controller simulation**: a faithful software copy of the real
tunnel controller. It measures cars with the photo eyes, tracks them down the
tunnel by conveyor pulses, fires relays at the right distances, and handles
services, washes, retracts, rollers, anti-collision, E-stops, and the
inactivity timer — the same way the real controller does. The controller UI you
use in the sim (login, Settings, Inputs, Relays, Services, Backup/Restore)
talks to this code, and the machines in the 3D tunnel are driven by its relay
outputs. Because it is real code and not Blueprint spaghetti, every behavior is
tested automatically and matches the V2 cheat sheet.

Unreal cannot open a project with C++ until that C++ is compiled **on your PC**.
That is a one-time setup. Here is how.

## Step 1: Install the prerequisites (one time)

You need two things on your Windows PC:

1. **Visual Studio 2022 Community** (free) — this is the compiler.
   - Download it from https://visualstudio.microsoft.com/downloads/
   - During install, on the "Workloads" screen, check **"Game development
     with C++"**.
   - In the panel on the right (Installation details), make sure these are
     checked (Epic's docs call these out):
     - **Windows 10 SDK** or **Windows 11 SDK** (any recent version)
     - **.NET desktop development** workload (add it from the Workloads screen)
   - Click Install. This takes a while and needs ~20 GB of disk.

2. **Unreal Engine 5.6** — from the **Epic Games Launcher**.
   - Open the launcher → Unreal Engine → Library → click **+** next to
     "Engine Versions" → pick **5.6** → Install.

If you already have both, you are done with this step.

## Step 2: First open — let it build

1. In the project folder, double-click **`MLA_Tunnel_Sim.uproject`**.
2. A dialog appears: *"The following modules are missing or built with a
   different engine version: MLA_Tunnel_Sim. Would you like to rebuild them
   now?"* — click **Yes**.
3. A small progress window shows "Building MLA_Tunnel_Sim...". This is normal.
   - First build: usually **2–10 minutes** depending on your PC.
   - You will NOT see detailed output — just the progress bar.
4. **Success looks like:** the progress window closes and the Unreal Editor
   opens to the project like it always did. That's it — the controller code is
   now live inside the editor.

After the first successful build, opening the project is instant again. You
only rebuild when the C++ changes (for example after pulling an update).

## Step 3: If the rebuild fails

Sometimes the dialog says *"MLA_Tunnel_Sim could not be compiled. Try rebuilding
from source manually."* That message hides the real error. To see it:

1. In the project folder, **right-click `MLA_Tunnel_Sim.uproject`** → choose
   **"Generate Visual Studio project files"**.
   (If you don't see that option, choose "Show more options" first on
   Windows 11.)
2. This creates **`MLA_Tunnel_Sim.sln`** in the project folder. Double-click it
   to open Visual Studio.
3. At the top of Visual Studio, set the two dropdowns to
   **Development Editor** and **Win64**.
4. Menu: **Build → Build Solution** (or press Ctrl+Shift+B).
5. Watch the **Output** window at the bottom. When it fails, the red `error`
   lines tell you exactly what is wrong.

### The fastest way to fix compile errors

You don't need to fix C++ errors yourself. Run a **local Claude Code session**
in the project folder and let it fix them:

1. Open a terminal in the project folder (in File Explorer: right-click inside
   the folder → "Open in Terminal").
2. Type `claude` and press Enter.
3. Paste the build errors from Visual Studio's Output window and say
   "fix these build errors".
4. When it's done, build again in Visual Studio (or just re-open the
   `.uproject` and answer Yes to rebuild).

Claude Code has this project's documentation (`CLAUDE.md`, `docs/`, and the
`micrologic-spec` skill) available, so it knows exactly how this module is
supposed to work.

## Step 4: Run the controller behavior tests

The controller logic ships with automated tests that prove it behaves like the
cheat sheet says. Run them after any build to confirm everything is healthy.

### In the editor (easiest)

1. Menu: **Window → Test Automation** (this opens the Session Frontend window;
   on some versions it's under Tools → Test Automation).
2. Click the **Automation** tab.
3. In the search box, type **`MicrologicSim`**.
4. Check the box next to the MicrologicSim group and click **Start Tests**.
5. Every row should go **green**. A red row means a controller behavior is
   broken — copy the message and hand it to a local Claude Code session.

### Headless (no editor window, good for scripting)

Open PowerShell and run (adjust the two paths for your machine):

```powershell
& "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "C:\Path\To\MLA-Tunnel-Sim\MLA_Tunnel_Sim.uproject" `
  -ExecCmds="Automation RunTests MicrologicSim; Quit" `
  -unattended -nullrhi -nosplash
```

This starts the engine invisibly (`-nullrhi` = no graphics), runs every test
whose name starts with `MicrologicSim`, and quits. Results are written to the
project's log file:

```
<project folder>\Saved\Logs\MLA_Tunnel_Sim.log
```

Search that file for `Test Completed` lines (each says Success or Fail) and
`Automation Test Queue Empty` near the end for the summary. Automation reports
are also written under `<project folder>\Saved\Automation\`.

## What compiling does NOT do

Building the C++ gives you the controller *brain*. Two pieces of setup live in
the Unreal content and are documented separately:

- **The controller UI widgets** (login screen, dashboard, Settings/Inputs/
  Relays/Services pages) — see **`docs/UI_PLAYBOOK.md`**.
- **Wiring the 3D world to the controller** (making the brushes, blowers, and
  photo eyes talk to relays and inputs) — see **`docs/WORLD_WIRING.md`**.

Do those next, in either order. Both are written so a local Claude Code session
can do the clicking for you.
