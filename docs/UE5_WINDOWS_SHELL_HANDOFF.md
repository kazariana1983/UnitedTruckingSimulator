# Implementation Handoff — First UE5 C++ Windows Shell

Status: Draft handoff from Lead Software Architect to Unreal Foundation Engineer
Matches: `prompts/03_unreal_foundation.md`, Master Delivery Plan `P1-05` and Phase 2 (`P2-01`…`P2-09`), SRS §10.2 acceptance criteria
Precondition this handoff assumes: Project Owner has approved (or provisionally authorized scaffolding of) `docs/TECHNICAL_ARCHITECTURE.md`. It does **not** assume any ground-truth value has been approved — every task below is buildable with `TBD`/synthetic data only.

## 0. What already exists when you start

- `simulator/Source/UTSCore/Public/UTS/**` — pure abstract-class interface headers for all nine contracts (Session, Configuration, Input, Vehicle State, Exercise, Detection, Telemetry, Scoring, Backend Sync) plus `Common/UTSCommonTypes.h`. No `.cpp`, no `.uproject`, no physics, no scoring rule, no measurement.
- `docs/INTERFACE_CONTRACTS.md` — the narrative form of every header above; read it alongside the headers, it explains the *why* the headers don't.
- `docs/TECHNICAL_ARCHITECTURE.md` §7 — module table with primary class names already chosen (`UUTSSessionSubsystem`, `UUTSConfigurationSubsystem`, etc.) so this handoff and the architecture stay in sync.

## 1. What you are building (and not building) in this pass

**Building:** a Windows development build that reaches a debug shell/scene, exercises every interface above through synthetic/mock data, and produces the SRS §10.2 acceptance evidence. This is Master Delivery Plan Gate G2's data-only vertical slice (`P2-09`), reached through `P2-01`…`P2-08`.

**Not building in this pass:** real tractor/trailer physics, real fifth-wheel dynamics, real camera/mirror optics tuning, real exercise geometry, real scoring rules, real hardware adapters beyond keyboard. Those are Phase 3+ and require approved ground truth or dedicated engineering passes this handoff does not authorize. Where a module needs a placeholder (e.g., a box-collider "tractor" that just moves under keyboard input so Detection/Telemetry/Scoring have something to observe), it must be **visibly and textually labeled** in-editor and in logs as a non-authoritative gray-box stand-in — never presented as tuned vehicle behavior.

## 2. Engine setup

1. Create the `.uproject` at repository root of `simulator/` (i.e., `simulator/UnitedTruckingSimulator.uproject`), C++ project, Windows target, targeting the latest stable UE5 release available to you (5.3 or later; exact version pin and Epic account/build-source choice is an **Open Decision** — see `docs/ARCHITECTURE_OPEN_DECISIONS.md` §B — record whatever version you actually use in the PR).
2. Primary game module: `UTS` (`simulator/Source/UTS/`) — Blueprint-exposable actors, pawns, the debug HUD widget, and the level(s). Depends on `UTSCore`.
3. Core module: `UTSCore` (already scaffolded, header-only so far) — add matching `.cpp` files and a `UTSCore.Build.cs` with **no dependency on `UMG`/Slate/rendering modules**, so the deterministic interfaces (Configuration, Detection, Scoring especially) stay unit-testable without booting a full editor/game world. `UTS`(game module) depends on `UTSCore`; the reverse must never be true.
4. Enable UE5's automation/functional testing framework for both modules from the start (`P2-01` onward needs it).

## 3. Task sequence (each task = one PR, one Master Delivery Plan ID)

1. **`P2-01` Configuration** — Implement `UUTSConfigurationSubsystem : IConfigurationProvider` as a `UGameInstanceSubsystem`. Loads profiles from local JSON/`.ini` (no backend dependency yet). Ship exactly two profile sets: (a) a `Draft` set with several fields intentionally missing, to prove `ValidateForLaunch` blocks it; (b) a `synthetic-only`, clearly-named `Approved`-for-testing-purposes set with made-up round numbers that are obviously not real measurements (e.g., document in-file "SYNTHETIC TEST DATA — NOT GROUND TRUTH" and pick values like exactly `1000` where a real one would never be that round). Never copy a number from `ground_truth/SIMULATOR_GROUND_TRUTH.md` — every field there is `TBD`, so there is nothing to copy; that is the point.
2. **`P2-02` Student Session** — `UUTSSessionSubsystem : ISessionManager`. Implement the full session state machine and the single-active-attempt rule against the synthetic config. Unit-test every transition in the table from `docs/INTERFACE_CONTRACTS.md` §1.
3. **`P2-04`/`P2-05` Input Abstraction + keyboard** — `UUTSInputSubsystem : IInputDeviceAdapter` with one `IInputSource` implementation: keyboard. Wheel/pedal adapters are explicitly out of scope here (`P3-01`).
4. **`P2-03` Exercise Manager (interface only)** — `UUTSExerciseManagerSubsystem : IExerciseManager` loading a single synthetic `ExerciseDefinition` (a gray-box rectangle yard with placeholder boundaries) sufficient to exercise lifecycle transitions and feed Detection something to react to. No real maneuver geometry.
5. **Gray-box vehicle stand-in** — a minimal `AUTSTractorPawn` that moves under `IInputDeviceAdapter` semantic actions (no suspension/tire model, just enough kinematics to move and turn) so downstream modules have a real `IVehicleStateProvider` to read. Log and HUD-label it "SYNTHETIC MOVEMENT MODEL — NOT VALIDATED TRACTOR PHYSICS." No trailer/fifth-wheel stand-in is required for this pass (Detection/Telemetry/Scoring can exercise their contracts against tractor-only state); add one only if it materially simplifies wiring, under the same synthetic-labeling rule.
6. **Detection (minimal)** — one `UUTSBoundaryVolume` and one `UUTSCompletionDetector` against the gray-box yard, emitting real `FDomainEvent`s from real (if simplistic) pose data — this is what makes the later golden-event and golden-replay tests meaningful rather than fully mocked.
7. **`P2-06` Telemetry** — `UUTSTelemetrySubsystem : ITelemetrySink` writing versioned frames/events to local disk. Prove crash/restart recovery with a manual disk-full or forced-kill test; capture the evidence for the PR per Master Delivery Plan §6.
8. **`P2-07` Scoring (interface + synthetic profile)** — `UUTSScoringSubsystem : IScoringEngine` with a test-only synthetic `ScoringProfile` (again obviously fake numbers, clearly labeled). Ship the golden-replay test harness now, even with only one or two trivial rules — this harness is what every later real rule must pass through.
9. **`P2-08` Debug HUD + structured logging** — `UUTSDebugHUDWidget` (in `UTS`, not `UTSCore`) surfacing session/attempt state, raw+normalized input, active config versions, telemetry integrity, and any fault state. Wire the `LogUTSSession`/`LogUTSInput`/`LogUTSVehicle`/`LogUTSExercise`/`LogUTSTelemetry`/`LogUTSScoring`/`LogUTSBackend` categories from `docs/TECHNICAL_ARCHITECTURE.md` §5.
10. **`P2-09` Vertical slice** — wire all of the above into one playable path: boot → (mock) login → select the one synthetic exercise → start attempt → drive the gray-box tractor with keyboard → trigger a boundary/completion event → complete attempt → local store write → synthetic-profile rescore → result shown in debug HUD. No backend client is required to be functional for this pass (`IBackendSyncClient` may be a no-op stub) — `FR-TEL-006`/`NFR-006` just require that its *absence* doesn't break anything, which a no-op stub demonstrates trivially; a real backend integration is Phase 6.

## 4. Acceptance evidence to include in the PR (SRS §10.2, verbatim)

- Windows development build starts and reaches the simulator shell/debug scene.
- Start/reset/complete/abort of a mock/data-only attempt, demonstrated (screen recording or step-by-step screenshots per Master Delivery Plan §6).
- Keyboard input flows through the same semantic abstraction intended for hardware (show the debug HUD's raw+normalized panel).
- Debug HUD shows current session/attempt state and raw/normalized input.
- Telemetry frames and lifecycle events written locally (show the output file/log).
- Deterministic scoring interface accepts the test-only synthetic profile and produces repeatable results (show the golden-replay test passing, twice, with identical output).
- Missing-ground-truth-dependent exercise data prevents validated exercise launch with a clear diagnostic (show the `Draft` profile set from step 1 being rejected by name).

## 5. Things this handoff explicitly forbids

- Do not add a value to `ground_truth/SIMULATOR_GROUND_TRUTH.md`.
- Do not name any synthetic profile, number, or behavior "validated," "approved," or omit the synthetic label anywhere a student/instructor-facing string could later leak it.
- Do not implement a hidden steering-assist/auto-centering path "to make the demo look good" — `FR-VEH-005` applies even to the gray-box stand-in.
- Do not let `UTSCore` depend on `UTS`, on Slate/UMG, or on any backend/HTTP library — it must stay unit-testable headless.
- Do not touch `mobile/`, `docs/MOBILE_MVP_PLAN.md`, or anything under the mobile track.

## 6. Definition of done

Master Delivery Plan §6 checklist in full, plus: every interface in `docs/INTERFACE_CONTRACTS.md` has at least one concrete implementation and at least one passing automated test; the golden-replay scoring test and the golden-event-list detection test both exist and pass; `docs/REQUIREMENT_TRACEABILITY_MATRIX.md` rows tagged Phase P2 are all demonstrably exercised by this build.
