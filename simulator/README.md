# `simulator/` — Windows UE5 Client

Status: Architecture-phase scaffold. No `.uproject` yet — see `docs/UE5_WINDOWS_SHELL_HANDOFF.md` for the Unreal Foundation Engineer's task sequence to create one around the interfaces below.

## What is here now

`Source/UTSCore/Public/UTS/**` — pure abstract-class C++ interface headers for the nine contracts this project's architecture package defines, with no `.cpp`, no physics, no scoring rule, and no measurement:

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

## What is not here yet

- `UTSCore.Build.cs`, `.uproject`, the `UTS` game module, any concrete implementation class, any level/map, any physics.
- Any value from `ground_truth/SIMULATOR_GROUND_TRUTH.md` — every field there is `TBD`; nothing in this folder invents one.

## Where to go next

- Building the first Windows shell: `docs/UE5_WINDOWS_SHELL_HANDOFF.md`.
- Why these interfaces look the way they do: `docs/INTERFACE_CONTRACTS.md` and `docs/TECHNICAL_ARCHITECTURE.md` §7.
- Requirement coverage: `docs/REQUIREMENT_TRACEABILITY_MATRIX.md`.
