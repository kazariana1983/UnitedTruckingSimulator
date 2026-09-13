# United Trucking Simulator — Technical Architecture (Windows PC / UE5)

Status: Draft technical architecture for Project Owner approval (`P0-11` candidate deliverable)
Owner role: Lead Software Architect (`prompts/02_software_architect.md`)
Reconciled against: `docs/SRS_ARCHITECTURE_RECONCILIATION.md`
Authority for physical/business facts: `ground_truth/SIMULATOR_GROUND_TRUTH.md` (unchanged by this document)

This document does not approve itself. Per the Master Delivery Plan, `P0-11` approval belongs to the Project Owner, and `P1-01` (creating this structure for real) is gated on that approval. What follows is the proposal plus the parts of it that are safe to scaffold now as non-authoritative, empty-of-logic structure (folders, READMEs, and pure interface headers with no physics, no scoring rule, and no measurement in them) — matching the Master Delivery Plan's explicit permission to "begin Phase 1 repository scaffolding in parallel, using only synthetic test configurations clearly labeled non-authoritative."

## 1. System context

Three independently deployable applications, none of which may embed another's core logic:

```
┌─────────────────────────┐        HTTPS/TLS         ┌──────────────────────────┐
│  Windows Simulator       │ ───────────────────────▶ │  Backend (FastAPI +      │
│  Client (UE5, C++ core)  │ ◀─────────────────────── │  PostgreSQL)             │
│  - runs offline-capable  │   auth / config / attempt │  - source of truth for   │
│  - never blocks on       │   / telemetry / scores    │    accounts, config,     │
│    backend or LLM        │                           │    attempts, scores      │
└─────────────────────────┘                           └───────────┬──────────────┘
                                                                    │ HTTPS/TLS
                                                                    ▼
                                                        ┌──────────────────────────┐
                                                        │  Instructor Dashboard    │
                                                        │  (React + TypeScript)    │
                                                        │  - reads/writes only     │
                                                        │    through backend APIs │
                                                        └──────────────────────────┘
```

The simulator must remain usable for configured offline practice; loss of the backend or the coaching LLM must never stop local physics, event detection, telemetry capture, or deterministic local scoring (SRS §5, MVP doc §15).

## 2. Proposed repository structure

Monorepo, matching Master Delivery Plan `P1-01`:

```
UnitedTruckingSimulator/
├── ground_truth/                  # unchanged, human-approved facts only
├── docs/                          # this package + existing product/process docs
├── backlog/
├── prompts/
├── simulator/                     # UE5 C++ client (this package's primary deliverable)
│   ├── Source/
│   │   └── UTSCore/               # C++ module: interfaces + deterministic logic
│   │       └── Public/UTS/
│   │           ├── Common/        # shared value types, IDs, versioning
│   │           ├── Session/       # ISessionManager, session/attempt state
│   │           ├── Config/        # IConfigurationProvider, profile snapshot types
│   │           ├── Input/         # IInputDeviceAdapter, semantic actions
│   │           ├── Vehicle/       # IVehicleStateProvider (tractor/trailer/coupling read model)
│   │           ├── Exercise/      # IExerciseManager, exercise definition types
│   │           ├── Detection/     # IDetectionEventSink, domain event types
│   │           ├── Telemetry/     # ITelemetrySink, frame/event schema types
│   │           ├── Scoring/       # IScoringEngine, score result/rule-trace types
│   │           └── Backend/       # IBackendSyncClient, sync state types
│   └── README.md                  # module map + link to UE5_WINDOWS_SHELL_HANDOFF.md
├── contracts/                     # versioned cross-app schema (telemetry, events, config, API DTOs)
│   └── README.md
├── backend/                       # FastAPI + PostgreSQL platform (Backend Engineer owns implementation)
│   └── README.md
├── dashboard/                     # React + TypeScript instructor dashboard (Dashboard Engineer owns implementation)
│   └── README.md
└── infra/                         # deployment, CI, environment config (Phase 8 owns implementation)
    └── README.md
```

Rationale for a monorepo over polyrepo: `contracts/` must be shared and versioned atomically with the three apps that consume it (telemetry schema, event schema, config schema, REST DTOs). A monorepo lets a single PR change a contract and all three consumers together, which the Master Delivery Plan's PR-per-subsystem review model and `NFR-010` (traceable versioned config) both depend on. Polyrepo would require a separate contract-versioning/release process this project has not scoped.

`simulator/`, `backend/`, `dashboard/`, `contracts/`, and `infra/` are created in this PR as directory + `README.md` scaffolding only — no FastAPI, no React, no build files — so their owning engineers (`Backend Engineer`, `Dashboard Engineer`, later Unreal work) start from an agreed layout instead of an empty repository, without this document overstepping into their subsystems. The `simulator/Source/UTSCore/Public/UTS/*` interface headers are the one exception: they are pure abstract-class contracts (no `.cpp`, no physics, no scoring rule, no measurement), written out fully so "interfaces for sessions, configuration, input devices, vehicle state, exercises, detection, telemetry, scoring, and backend sync" (this task's explicit ask) exist as real, reviewable C++ rather than prose describing C++. See `docs/INTERFACE_CONTRACTS.md` for the narrative form of the same contracts and `docs/UE5_WINDOWS_SHELL_HANDOFF.md` for what the Unreal Foundation Engineer builds around them.

## 3. Branching, review, and coding standards (proposal for `P1-02`)

- **Branch model:** `main` protected; one branch per task ID from the Master Delivery Plan (e.g. `feature/p2-02-session-manager`); PRs required, no direct pushes to `main`.
- **Definition of done:** the Master Delivery Plan §6 checklist (linked task/requirement IDs, scope + exclusions, tests, build/test evidence, config/migration notes, failure cases, screenshots for UI, telemetry/log evidence, human-validation flag, docs, reviewer approval) applies verbatim to every PR touching `simulator/`, `backend/`, or `dashboard/`.
- **C++ standard:** Unreal's Epic coding standard (naming: `I`-prefixed interfaces, `F`-prefixed value structs, `U`-prefixed `UObject`s, `A`-prefixed actors), C++17 subset as constrained by the engine's toolchain. Core logic in C++; Blueprints restricted to presentation/asset wiring/config exposure per `prompts/02` and `prompts/03`.
- **Python standard:** PEP 8, type hints required on public functions, `ruff`/`black` for lint/format (to be finalized by Backend Engineer at `P1-06`).
- **TypeScript standard:** strict mode, ESLint + Prettier (to be finalized by Dashboard Engineer at `P1-08`).
- **No subsystem may hard-code another's rules:** enforced structurally (below), not just by convention.

## 4. Configuration strategy

Every vehicle, trailer, yard, hardware, and scoring configuration is a **versioned, immutable-once-approved snapshot**, per SRS §8 and MVP doc §11's `ConfigurationApproval`/`ValidationRecord` entities:

- Identity: `{ProfileId, Version}` — never mutated after `Approved`; a change is a new version.
- Lifecycle: `Draft → Approved → Retired` (SRS §8). Only `Approved` profiles may back a *validated* attempt. `Draft` profiles may back a *labeled-practice/synthetic* attempt only, and the UI/API must render an explicit "unvalidated practice" label whenever a `Draft` profile is in use (MVP doc §6).
- Provenance: every profile version records its `ground_truth` source revision (or "synthetic — not for validated use" when it has none) and approver identity/timestamp — this is `NFR-010`.
- Attempts snapshot the exact profile *version* IDs they ran under (`FR-API-004`), not a live reference, so a result stays reproducible even if a profile is later retired or superseded.
- Startup validation: the Configuration module refuses to start a *validated* exercise when a required field is `TBD`/`Draft` and reports exactly which field is missing (SRS §8, FR-EXR-006). It does not refuse a *synthetic/practice-labeled* run using clearly-marked test data — this is how Milestone 1's non-authoritative vertical slice stays possible before ground truth exists.

## 5. Logging strategy

- Structured logs (key-value/JSON), never free-text-only, correlating `StationId`, `SessionId`, `AttemptId`, and `UploadOperationId` end to end (`NFR-004`).
- No secrets, tokens, or PII payloads in logs.
- Unreal side: a dedicated log category per module (`LogUTSSession`, `LogUTSInput`, `LogUTSVehicle`, `LogUTSExercise`, `LogUTSTelemetry`, `LogUTSScoring`, `LogUTSBackend`) so a station's log can be filtered per subsystem during CDL-instructor/physics-diagnostic review without exposing internals to the student build.
- Backend side: request-scoped structured logging correlated by the same `AttemptId`/`StationId` fields so a backend log line and a simulator log line about the same attempt can be joined.
- Debug HUD (Milestone 1) surfaces the same correlation IDs plus live session/attempt state, raw+normalized input, config version, and telemetry health — it is a *read* view onto the same structured log/telemetry data, not a second source of truth.

## 6. Build & deployment overview

Full detail is Phase 8 (`P8-03` through `P8-08`) and out of this document's scope; the following constraints shape the architecture now so later phases are not blocked:

- **Simulator:** Windows development + packaged (Shipping/Development) UE5 builds; packaging/installer work is `P8-05` and requires the module boundaries below to exist first.
- **Backend:** FastAPI app behind TLS, PostgreSQL with Alembic migrations, 12-factor-style environment configuration (no secrets in source or in the simulator/dashboard binaries), consistent with `FR-API-001/006` and `NFR-005`.
- **Dashboard:** static production build served behind TLS, calling backend APIs only — never embedding scoring or backend logic (`FR-UI-005`).
- **CI foundation (`P1-03`):** backend lint/type/test, dashboard lint/type/test/build, contract-schema validation, and (as available in this environment) Unreal C++ compile checks, run per PR.

## 7. Required Unreal modules

Per `prompts/02_software_architect.md`'s explicit module list, reconciled with the MVP doc's module table (`docs/SRS_ARCHITECTURE_RECONCILIATION.md` §2). Each module's interface is fully specified in `docs/INTERFACE_CONTRACTS.md`; header stubs live under `simulator/Source/UTSCore/Public/UTS/`.

| Module | Responsibility | Inputs | Outputs / interface | Depends on | Primary classes | Failure cases | Tests |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Student Session** | Boot→ready→authenticated→configuring→loading→active→paused→completing→persisting→results state machine; station/attempt ownership (one active attempt/station) | Auth response, station config, UI transitions | Session state changes, attempt-context handle | Configuration, Backend Client (optional) | `UUTSSessionSubsystem : ISessionManager` | Backend unreachable at login → `Offline` state, cached/local identity if approved; two attempt-start requests → second rejected | Unit: state-transition table; integration: offline login path |
| **Configuration** | Load/validate versioned vehicle, trailer, yard, exercise, input, telemetry, scoring profiles; enforce Draft/Approved/Retired gate | Profile payloads (local file or backend) | Immutable `FConfigSnapshot` per profile type | None (leaf module) | `UUTSConfigurationSubsystem : IConfigurationProvider` | Missing/`TBD` required field for a validated launch → blocked with named-field diagnostic, never a guessed default | Unit: parse/validate per profile type; golden-file: reject-list of intentionally incomplete profiles |
| **Input Abstraction** | Normalize keyboard, wheel, pedals, optional clutch/shifter into semantic actions; calibration | Raw device signals, `FCalibrationProfile` | `FSemanticControlFrame` (steering/throttle/brake/clutch/shift/gear/parking-brake) | Configuration | `UUTSInputSubsystem : IInputDeviceAdapter`, per-device `IInputSource` implementations (keyboard, wheel/pedal, shifter) | Device disconnect mid-attempt → fault event + configured safe response (`TBD`, see Open Decisions) | Unit: calibration curve/dead-zone math; integration: simulated-device disconnect |
| **Tractor Vehicle** | Low-speed tractor simulation (position, heading, steering, axle/wheel state) | Semantic controls, `VehicleProfile` | Tractor pose/motion state (published through `IVehicleStateProvider`) | Input Abstraction, Configuration | `AUTSTractorPawn` | Config missing approved mass/steering-range → tractor refuses validated spawn | Unit: none invented until physics exist; interface/mock tests only in this phase |
| **Trailer Physics** | 53 ft trailer dynamics | Tractor state, `TrailerProfile` | Trailer pose/motion state | Fifth-Wheel Coupling, Configuration | `AUTSTrailerActor` | Same as above | Same as above |
| **Fifth-Wheel Coupling** | Articulated connection between tractor and trailer | Tractor/trailer state, coupling config | Articulation angle/rate, coupling-health state | Tractor Vehicle, Trailer Physics | `UUTSFifthWheelComponent` | Coupling geometry `TBD` → articulation reported as `Unavailable`, never estimated | Unit: kinematic articulation math once geometry exists |
| **Camera & Mirrors** | Cab, mirror, exterior instructor views | Vehicle/trailer poses, camera config | Rendered views, active-camera state | Tractor Vehicle, Trailer Physics | `AUTSCabCameraActor`, `AUTSMirrorCaptureComponent`, `AUTSInstructorCameraActor` | Mirror render target failure → visible fallback, never a silently frozen/stale mirror | Manual/visual (see Human Validation Checklist); automated: camera-state transition tests |
| **Exercise Manager** | Exercise lifecycle: load definition, start pose, reset, objective/lifecycle state | `ExerciseDefinition` (versioned, data-only), domain events | Exercise lifecycle events, current objective state | Configuration, Detection | `UUTSExerciseManagerSubsystem : IExerciseManager` | Exercise definition references missing yard geometry → load blocked with diagnostic | Unit: lifecycle state machine per exercise type; regression: one test per MVP maneuver |
| **Boundary/Collision Detection** | Observe boundaries, cones/collisions, pull-ups, stops, completion candidates | Vehicle/trailer poses, collision events, yard/exercise config | Timestamped `FDomainEvent`s (facts only — no scoring) | Trailer Physics, Configuration | `UUTSDetectionSubsystem : IDetectionEventSink` producers (`UUTSBoundaryVolume`, `UUTSConeVolume`, `UUTSPullUpDetector`, `UUTSCompletionDetector`) | Detection thresholds `TBD` → detector runs in "observe only, do not certify completion" mode until approved | Unit: one test per detector type with synthetic pose sequences; golden: recorded pose sequence → exact expected event list |
| **Telemetry** | Sample/buffer/serialize/persist/stream attempt data | Controls, poses, states, domain events | Versioned frames + events to local store and (optionally) backend | Input Abstraction, Vehicle modules, Detection | `UUTSTelemetrySubsystem : ITelemetrySink` | Local write failure → integrity flag set, session manager notified, never silent data loss | Unit: frame/event serialization round-trip; fault-injection: disk-full/crash-recovery |
| **Scoring** | Deterministic score from event stream + approved profile | Attempt event stream, `ScoringProfile` version | `ScoreResult` + ordered `ScoreRuleTrace` | Detection (via stored events), Configuration | `UUTSScoringSubsystem : IScoringEngine` (pure C++, no `UObject` side effects beyond result data) | Missing/corrupt required event → explicit `Unscorable` result, never a guessed score | Unit: rule-by-rule; **golden replay**: fixed event stream + profile ⇒ byte-identical result, run on every scoring-code change |
| **Local Cache** | Recoverable local storage for attempts pending upload | Telemetry frames/events, attempt lifecycle | Durable local attempt store, integrity state | Telemetry | `UUTSLocalAttemptStore` | Process crash mid-attempt → finalized chunks recovered, attempt marked `Interrupted` on restart | Fault-injection: kill process mid-write, verify recovery |
| **Backend Client** | Auth/config sync, heartbeat, attempt upload, retry | API requests, local upload queue | Sync responses/state, errors | Local Cache | `UUTSBackendClientSubsystem : IBackendSyncClient` | Network loss → queued, retried with backoff, never blocks the active attempt | Unit: idempotency-key generation, retry/backoff schedule; integration: simulated network partition |
| **Debug & Validation** | Physics overlays, boundary/sensor visualization, diagnostic export | Runtime state from all modules above | Validator-only evidence/reports | All (read-only) | `UUTSDebugHUDWidget`, `UUTSDiagnosticOverlayComponent` | N/A (diagnostic-only; must never affect scored behavior) | Manual: Human Validation Checklist |

**Required decoupling (SRS NFR-002, MVP doc §8 "Required decoupling," Agent Operating Rule #14):**

- Input devices never call tractor implementation code directly — only through `IInputDeviceAdapter`'s semantic actions.
- Vehicle physics (Tractor/Trailer/Coupling) never reference exercise or scoring types.
- Detection produces facts (`FDomainEvent`) only; it never computes a score or penalty.
- Scoring consumes only the recorded event stream + profile; it never reads live vehicle/exercise state directly (this is what makes replay determinism possible).
- Exercise definitions are data (`ExerciseDefinition` assets/JSON), never embedded in a map's level blueprint or a vehicle C++ class.
- Telemetry persistence must keep working with `IBackendSyncClient` entirely absent (e.g., unit-tested with a null backend client).
- AI coaching (Phase 11, out of this package's scope) consumes only completed, stored `Attempt` + `ScoreResult` data — it has no reference to any live subsystem above.

## 8. Backend modules

| Module | Responsibility | Inputs | Outputs / interface | Depends on | Primary components | Failure cases | Tests |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Authentication** | Identity, session/token issuance, role enforcement | Credentials, tokens | AuthN/AuthZ decisions | Students, Instructors (for role lookup) | `auth` router, password hashing service, token issuer | Bad credentials, expired token, role mismatch → typed 401/403, never a silent allow | Unit: hashing/verification, token expiry; integration: role-matrix per endpoint |
| **Students** | Student records, school relationship | Admin/API writes | `StudentProfile` records | Authentication | `students` router + repository | Duplicate/conflicting student identity → 409 with reason | Unit + API contract tests |
| **Instructors** | Instructor records, school relationship, notes authorship | Admin/API writes | `InstructorProfile` records | Authentication | `instructors` router + repository | Same pattern as Students | Unit + API contract tests |
| **Simulator Stations** | Station registration, config assignment, heartbeat, software version tracking | Station auth, heartbeat pings | Station status, assigned config versions | Authentication, Configuration Registry | `stations` router, heartbeat table | Missed heartbeat → station marked stale, dashboard reflects it, never silently "online" | Unit: staleness threshold; integration: heartbeat gap simulation |
| **Configuration Registry** | Versioned vehicle/trailer/yard/exercise/device/scoring profile storage + approval lifecycle | Draft submissions, approval actions | `Draft/Approved/Retired` profile versions | Authentication (approver role) | `configurations` router, approval audit table | Attempt requests a retired/unknown version → 409/404, never silently substitutes latest | Unit: lifecycle transitions; audit-log completeness test |
| **Exercises** | Exercise catalog, assignable definitions | Configuration Registry entries | Exercise definitions by station/assignment | Configuration Registry | `exercises` router | Requested exercise unassigned to station → 403 | Unit + API contract tests |
| **Attempts** | Immutable attempt metadata + lifecycle state | Attempt create/update from simulator | Attempt records, idempotent create | Students, Stations, Exercises, Configuration Registry | `attempts` router, idempotency-key table | Duplicate create with same idempotency key → returns original, no duplicate record | Unit: idempotency; integration: replayed request |
| **Telemetry** | Schema-validated, idempotent telemetry ingestion | Telemetry chunks from simulator | Stored frames/events, integrity reports | Attempts | `telemetry` router, chunk store | Schema mismatch/corrupt chunk → rejected with diagnostic, attempt marked integrity-flagged, never silently accepted | Unit: schema validation; fault-injection: corrupt/truncated chunk |
| **Scores** | Store/serve event stream, scoring profile/version, rule trace, reproducible result | Scoring Engine output (may run in simulator and/or backend replay) | `ScoreResult`/`ScoreRuleTrace` records | Telemetry, Configuration Registry, Attempts | `scores` router | Missing required events → stored as `Unscorable`, not a guessed score | Golden replay tests shared with simulator's Scoring module |
| **Coaching** | Backend-only LLM orchestration on validated structured attempt data | Completed `Attempt` + `ScoreResult` | Auditable `CoachingResult` | Attempts, Scores | `coaching` router, LLM client wrapper | Insufficient evidence → explicit "no supported conclusion," never invented advice | Unit: evidence-grounding contract; red-team tests (Phase 11) |
| **Reporting** | History, summaries, trends, exports for dashboard | Attempts, Scores, Telemetry | Aggregated read views | Attempts, Scores | `reports` router | N/A (read-only aggregation) | Unit: aggregation correctness on fixture data |

Minimum API groups match SRS `FR-API-002` / MVP doc §9 verbatim: `/auth`, `/students`, `/instructors`, `/stations`, `/configurations`, `/exercises`, `/attempts`, `/telemetry`, `/scores`, `/notes`, `/reports`, `/coaching`. Mutating endpoints are authenticated, authorized, validated, logged, and idempotent where retried (`/attempts`, `/telemetry` in particular — `FR-API-005`).

## 9. Dashboard modules

| Module | Responsibility | Data source | Interface | Depends on | Failure/empty states | Tests |
| --- | --- | --- | --- | --- | --- | --- |
| **Student List** | Searchable roster, latest activity | `/students`, `/attempts` | Read-only list + filter | Backend Attempts/Students | Loading, empty, unauthorized | Component tests, role-access tests |
| **Simulator Station Status** | Connected/offline stations, active attempts | `/stations` | Live/cached status view | Backend Stations | Must distinguish live vs. cached vs. stale (MVP doc §10) | Component tests for each state |
| **Current Attempt** | Live station health, exercise, elapsed state, event feed | `/stations`, `/attempts`, event stream | Live view | Backend Attempts/Stations | Must never convert missing telemetry into a claimed event (MVP doc §10) | Component + contract test on event feed shape |
| **Attempt History** | Date/time, exercise, completion, score-profile/version, upload state | `/attempts` | Paginated list → detail | Backend Attempts | Empty/offline/unauthorized states | Component + pagination tests |
| **Scores** | Result, scoring trace, event timeline, replay references | `/scores`, `/attempts` | Read-only detail view | Backend Scores | Unscorable/pending states rendered explicitly, not hidden | Component tests per result state |
| **Common Mistakes** | Aggregated deterministic event categories, transparent filters | `/reports` | Aggregated read view | Backend Reporting | No inferred missing events (MVP doc §10) | Aggregation-contract tests |
| **Instructor Notes** | Authored notes on attempts/students, audit metadata | `/notes` | Read/write (instructor role) | Backend Instructors/Attempts | Write failure → visible retry, not silent loss | Component + API contract tests |

The dashboard consumes only backend APIs (`FR-UI-005`); it contains no substitute scoring or backend logic and no direct database or simulator access.

## 10. Dependency-ordered implementation backlog (technical detail under Master Delivery Plan Phases 1–2)

1. `P1-01` Land this repository structure (this PR delivers the scaffold; Project Owner approval formally closes the gate).
2. `P1-04` Freeze `contracts/` v0: `FAttemptId`/`FConfigVersionRef` shape, telemetry frame field list (units TBD-but-typed), domain event taxonomy, config-profile envelope. (See `docs/INTERFACE_CONTRACTS.md`.)
3. `P1-05` Unreal Foundation Engineer creates the actual `.uproject`/module build files around the `UTSCore` header contracts (see `docs/UE5_WINDOWS_SHELL_HANDOFF.md`).
4. `P1-06`–`P1-08` Backend and dashboard skeletons, in parallel with (3).
5. `P2-01`…`P2-08` Implement Configuration → Session → Exercise Manager (interface only) → Input Abstraction → Telemetry → Scoring (interface + synthetic profile) → Debug HUD, in that dependency order, against `UTSCore` interfaces — no physics yet.
6. `P2-09` Data-only vertical slice: start → reset → complete → abort → store → rescore a mock attempt end to end, entirely on synthetic, explicitly-non-authoritative configuration.

Phases 3+ (real physics, real exercises, real scoring rules, backend platform, UX, security/ops, validation, launch, AI coaching) proceed exactly as sequenced in `docs/MASTER_DELIVERY_AND_TASK_ASSIGNMENT_PLAN.md` and are unchanged by this document.

## 11. Cross-references

- `docs/SRS_ARCHITECTURE_RECONCILIATION.md` — how this architecture reconciles PR #1's SRS with the MVP product/screen/feature architecture.
- `docs/INTERFACE_CONTRACTS.md` — full data/method contracts for the nine interfaces this task asked for by name.
- `docs/REQUIREMENT_TRACEABILITY_MATRIX.md` — every SRS requirement ID mapped to module, primary class/interface, and test obligation.
- `docs/UE5_WINDOWS_SHELL_HANDOFF.md` — the literal task list for the first UE5 C++ Windows shell.
- `docs/ARCHITECTURE_OPEN_DECISIONS.md` — everything here that still needs Project Owner and/or CDL Instructor sign-off.
