# `simulator/` — Windows UE5 Client

Status: P2-01 configuration foundation scaffold. The UE project/module files are present, but this environment does not include UnrealEditor or UnrealBuildTool, so only the headless C++ configuration tests have been run here.

## What is here now

`Source/UTSCore/Public/UTS/**` contains the existing pure abstract-class C++ interface headers for the project contracts:

| Path | Interface | Contract doc |
| --- | --- | --- |
| `Common/UTSCommonTypes.h` | Shared IDs, `FConfigVersionRef`, `EApprovalState`, `TApprovedValue<T>` | `docs/INTERFACE_CONTRACTS.md` (cross-cutting) |
| `Session/ISessionManager.h` | Session/UI state machine + attempt lifecycle | §1 |
| `Config/IConfigurationProvider.h` | Versioned, approved configuration profiles | §2 |
| `Input/IInputDeviceAdapter.h` | Semantic control normalization | §3 |
| `Vehicle/IVehicleStateProvider.h` | Read-only tractor/trailer/coupling state | §4 |
| `Exercise/IExerciseManager.h` | Data-driven exercise lifecycle | §5 |
| `Detection/IDetectionEventSink.h` | Timestamped domain-event facts | §6 |
| `Telemetry/ITelemetrySink.h` | Frame/event sampling and local persistence | §7 |
| `Scoring/IScoringEngine.h` | Deterministic, pure scoring function | §8 |
| `Backend/IBackendSyncClient.h` | Auth/config sync, heartbeat, idempotent upload | §9 |

P2-01 adds:

| Path | Purpose |
| --- | --- |
| `UnitedTruckingSimulator.uproject` | Provisional UE5 shell scaffold. `EngineAssociation` is set to `5.3` as an untested baseline because no UE installation is available in this container. |
| `Source/UnitedTruckingSimulator.Target.cs`, `Source/UnitedTruckingSimulatorEditor.Target.cs` | Game/editor target scaffolding. |
| `Source/UTSCore/UTSCore.Build.cs` | Runtime core module with C++17 and exceptions enabled for the filesystem/parser path. It has no Slate/UMG/rendering dependencies. |
| `Source/UTS/UTS.Build.cs` | Minimal primary game module depending on `UTSCore`. |
| `Source/UTSCore/Public/UTS/Config/ConfigurationStore.h` and `Source/UTSCore/Private/Config/ConfigurationStore.cpp` | Headless deterministic configuration core implementing local profile loading, immutable snapshot access, approval-state lookup, required schema validation, dependency closure validation, and fail-closed validated launch gating. |
| `Source/UTSCore/Public/UTS/Config/UTSConfigurationSubsystem.h` and `Source/UTSCore/Private/Config/UTSConfigurationSubsystem.cpp` | UE `UGameInstanceSubsystem` adapter around the headless provider. |
| `Config/ProfileSets/DraftGroundTruthPending/profiles.ini` | Draft profile set with TBD and intentionally omitted fields copied from the current ground-truth absence. It must block validated launch. |
| `Config/ProfileSets/SyntheticPracticeOnly/profiles.ini` | Synthetic practice/test profile set with round fake values. It is `Draft` plus `SyntheticPracticeOnly`, never human approved. |
| `Tests/Configuration/configuration_store_tests.cpp` and `scripts/run_headless_tests.sh` | Headless C++ tests for load, immutable snapshot metadata, conservative unknown approval behavior, validated-launch rejection, and separate synthetic-practice readiness. |

## P2-01 configuration behavior

`IConfigurationProvider::ValidateForLaunch` is treated only as the validated-attempt gate. In this increment it always fails closed with an explicit diagnostic that validated launch is unavailable until typed approved schemas and real ground-truth verification are implemented. It also reports every currently detectable missing, TBD, unapproved, synthetic, metadata, typed-value, and dependency issue.

Synthetic practice is deliberately separate through `FFileConfigurationProvider::ValidateForMode(..., ELaunchValidationMode::SyntheticPracticeOnly)`, which returns `FConfigurationModeValidationResult::bReadyForRequestedMode`. It does not set `FValidatedConfigSet::bReadyForValidatedLaunch`, and the tests cover that boundary.

Profile payload files do not declare their own required fields. The P2-01 schema lives in `GetRequiredFieldSchema(EProfileType)` plus a required launch dependency set for exercise definitions. This prevents a profile or exercise file from passing validation by omitting requirements.

The current `IConfigurationProvider::GetApprovalState` interface has no `Unknown` value. `FFileConfigurationProvider::GetApprovalState` returns `Draft` for unknown versions as a conservative fail-closed value, and `GetApprovalStateDetailed` exposes whether the profile version was actually found.

## Local verification

Run:

```bash
simulator/scripts/run_headless_tests.sh
```

Verified in this container:

```text
configuration_store_tests: PASS
pure public header syntax: PASS
```

The syntax check covers the pure public UTSCore headers with `g++ -std=c++17 -Wall -Wextra -Werror -fsyntax-only`. It excludes UE headers that require Unreal's generated-code toolchain.

## P2-02 session lifecycle

The session increment adds a deterministic in-memory manager for explicitly synthetic development sessions. It handles attempt start, pause/resume, reset requests, completion/abort, and lifecycle records. Connectivity is tracked separately from the active attempt. Exact profile versions must match the resolved configuration before an attempt starts.

See `../docs/P2_02_SESSION_CONTRACT.md` for the scope and transition requirements. Run the configuration and session suites together with `bash simulator/scripts/run_headless_tests.sh`.

Completion is a lifecycle event, not a passing score. Reset records a request and preserves evidence; vehicle movement and pose reset are not wired in this increment. In-memory records do not survive process exit. Windows/Unreal verification remains pending.

Both configuration and session headless suites pass locally. The session core is not yet wrapped by `UUTSSessionSubsystem` or connected to Blueprint/UI. That integration remains required before this component is usable in the Windows shell.

## Scope still pending

Review hardening also tests malformed reloads, duplicate profile versions, snapshot isolation, and missing directories. Failed loads discard all partial profiles; they never expose partially parsed configuration. Duplicate keys and unknown sections are rejected.

The Unreal adapter, generated headers, Windows linking, and packaged profile-file discovery have not been built or tested here. Open the `.uproject` using the chosen compatible Unreal installation, generate project files, and build the Editor target on Windows before treating this scaffold as an executable delivery. No map or packaged application is supplied by this increment.

No input adapter, exercise manager, durable telemetry store, scoring engine, vehicle movement, physics, debug HUD, backend client, maps, assets, or validated simulator behavior is implemented here. Those remain later P2+ tasks.

No value was added to `ground_truth/SIMULATOR_GROUND_TRUTH.md`. Current validated training launch remains unavailable because all human-approved physical, scoring, hardware, and yard values are still TBD.
