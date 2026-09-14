# Unreal session shell: Windows verification

Status: source integration for review. No Windows/Unreal build or screen recording has been produced in this environment. This is a development control screen, not a driving simulator or a validated training session.

## Build

Use a Windows development machine with the selected Unreal Engine installation and its compatible C++ toolchain installed. The project's `EngineAssociation` remains the provisional `5.3` baseline. Do not infer compatibility with another version from the headless Linux tests.

From the repository root, in PowerShell:

```powershell
.\simulator\scripts\build_windows_editor.ps1 -UnrealRoot 'C:\Program Files\Epic Games\UE_5.3'
```

Replace the example path with the actual engine installation. The script invokes Unreal's build tool for the Development Editor target and stops on a failed build. It does not install prerequisites, package, publish, or launch the project.

Open `simulator/UnitedTruckingSimulator.uproject`. Create/open an empty level for the development shell and use Play in Editor, not Simulate. No binary map or training-yard geometry is supplied by this change. The project default GameMode provides the development HUD/controller; remove any per-level GameMode override that prevents it from running. Click the viewport to give keyboard input focus.

## Session facade

`UUTSSessionSubsystem` is a GameInstance subsystem. It initializes after the configuration subsystem, owns one pure C++ session manager, and exposes Blueprint-callable synthetic commands and status getters. `DefaultGame.ini` supplies fake development identity and exact profile version pins. No automatic student sign-in or automatic attempt start occurs.

Configuration failures keep attempts unavailable; the wrapper displays the rejection summary and logs individual missing-field diagnostics. Completing an attempt keeps its record visible until another starts. Ending Play destroys the in-memory records. Subsystem commands are disabled in Shipping builds.

The project declares profile fixtures as loose runtime dependencies for development builds because the headless loader uses standard filesystem APIs. Packaging/cooking and loose-file discovery in a packaged Windows build still require verification.

## Evidence to capture

Default controls (click the Play viewport first):

| Key | Action | Expected result in a fresh session |
| --- | --- | --- |
| B | Begin synthetic session | Session Ready; no attempt yet. |
| Enter | Start attempt | Session/attempt Active; event count 1. |
| P | Pause/resume | Active → Paused → Active; one event per command. |
| R | Request reset | Records a reset-in-place request; retains earlier events and current lifecycle state. |
| C | Complete | Attempt Completed; session Results; final event added. |
| X | Abort | Active/Paused attempt becomes Aborted; session Results. |

For a concrete sequence, B → Enter → P → P → R → C should leave a Completed attempt with five events. Another C must be rejected with the count still five. Enter starts a new attempt with count one; X then aborts it with count two. These are expected results to verify in Unreal, not a claim that the UI was run here.

- Actual Unreal version, Windows toolchain, commit, and editor build result.
- The synthetic-only screen with session and attempt state, event count, and diagnostics visible.
- Begin/start/pause/resume/reset/complete/abort commands and resulting state changes.
- Repeated start, repeated completion, and commands before session creation rejected without replacing evidence.
- A missing or mismatched profile version in `DefaultGame.ini` blocks attempt start after restarting Play. Restore the checked-in synthetic settings after the test.
- Restarting Play returns to a fresh development session. Do not describe that as crash recovery or saved history.

No P2/G2 acceptance gate is complete until the required Unreal/Windows evidence is recorded. Real authentication, driving input, vehicle reset, persistent telemetry, scoring, hardware, and launch packaging remain separate work.

## API references used during integration

- [GameInstance subsystem lifecycle](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameInstanceSubsystem).
- [Subsystem dependency initialization](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FSubsystemCollectionBase).
- [Runtime dependencies and loose file staging](https://dev.epicgames.com/documentation/en-us/unreal-engine/integrating-third-party-libraries-into-unreal-engine).

These references guided source integration; they are not evidence that this project was built with Unreal.
