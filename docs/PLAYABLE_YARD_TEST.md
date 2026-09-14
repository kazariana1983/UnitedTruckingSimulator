# Synthetic playable yard: first Windows check

This increment adds a code-generated yard and tractor to the development session shell. It is a toy movement prototype, not validated commercial-driver training. The previous shell opened in Unreal on the user's machine; this new increment has not yet been compiled or played in Windows here.

## Install and play

1. Download the `codex/p2-03-playable-yard` branch ZIP and extract it to a new folder. Keep the working project folder as a backup; do not overwrite a project while Unreal is open.
2. Open `simulator/UnitedTruckingSimulator.uproject` with Unreal 5.8.2. Accept rebuilding the project modules if prompted. If compilation fails, provide the final compiler errors from the log.
3. Open your existing basic level, or create a Basic level. The project sets `UTSDevelopmentGameMode` as its default. In World Settings, clear a conflicting GameMode Override or select this development mode.
4. Choose **Play**, not **Simulate**, with one local player. Click the game viewport. No hand-placed truck, PlayerStart, imported asset pack, or binary map is required.
5. The expected view is a simple tractor on a square paved platform, with cones/lane markings and a yellow synthetic-development HUD. The platform is generated only during Play; it will not appear in the normal editing viewport. It sits at an arbitrary elevated developer origin to keep the default landscape out of the way.
6. Press **B**, then **Enter**, release movement keys, and drive with the controls below.

| Key | Action |
| --- | --- |
| B | Begin synthetic session |
| Enter | Start an attempt and return tractor to start |
| W / S | Forward / reverse; changing direction first slows to a stop |
| A / D | Steer left / right |
| Space | Brake |
| V | Switch overhead / cab camera |
| P | Pause / resume attempt |
| R | Reset tractor pose after session accepts reset; keep prior events |
| C / X | Complete / abort attempt and disable movement |

Release movement keys after starting, resetting, or resuming. Losing viewport focus pauses the development attempt; click back into the viewport and press P to resume. Escape ends Play in Editor.

## Acceptance checks

- Before B + Enter, W does not move the tractor.
- After B + Enter, W/S/A/D work and the HUD speed changes. Space stops it. W + S together brakes instead of selecting a direction.
- V switches between a simple open cab view and an overhead view. These are prototype cameras, not realistic mirrors or an instrumented cab.
- P stops movement; holding W while paused does nothing. Resume, release controls, and press W again to drive.
- R returns to the original pose with zero speed and adds a reset event. A rejected reset (before an attempt or after it ends) must not change the pose.
- C or X stops movement. Enter can begin the next attempt, with a fresh starting pose.
- Alt-tab away while driving: return to a paused attempt without a stuck throttle.
- Stop Play and start again: one new yard and tractor appear, with a fresh session. Previous session events were memory-only.

## Scope and limits

The tractor uses primitive engine meshes and a deterministic kinematic toy model. There is no articulated trailer, tire/suspension/air-brake simulation, calibrated wheel input, collision response, scoring, telemetry frame persistence, or launch package in this increment. Cones and markings are visual only. A synthetic position bound contains the tractor; reaching it does not generate an official boundary violation. Reverse or reset away from the boundary if blocked.

Round developer settings live in `FSyntheticTractorSettings` in `SyntheticTractorMotion.h`. They do not consume the deliberately fake dimensions in the synthetic profile fixture and do not establish approved vehicle specifications. Ground-truth files and scoring rules are unchanged. Existing attempt profile pins are retained for lifecycle testing; they do not mean trailer geometry or physics is present. Shipping builds disable this development shell.

Configuration, session, and toy motion suites run with:

```bash
bash simulator/scripts/run_headless_tests.sh
```

These tests do not compile UHT/Unreal modules or establish visual quality, packaged-asset availability, performance, or Windows compatibility. The Windows build and acceptance checks above remain necessary.

API references used during source review: [Epic FViewport](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FViewport), [Epic Restart Player at Transform](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Game/RestartPlayeratTransform).
