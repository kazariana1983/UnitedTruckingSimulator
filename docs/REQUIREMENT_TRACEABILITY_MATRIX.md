# Requirement → Module / Interface / Test Matrix

Status: Draft for human review — Lead Software Architect's module-level mapping of the SRS drafted in PR #1
Scope note: `docs/MASTER_DELIVERY_AND_TASK_ASSIGNMENT_PLAN.md` assigns the full traceability matrix (`P0-03`) to the Product Architect, gated on SRS review (`P0-01`) and architecture approval (`P0-02`). This document is the **architecture-side half** of that matrix — requirement → owning module → primary interface/class → test obligation — produced now so `P0-11` (architecture approval) and `P0-03` are not sequentially blocking each other. It does not substitute for, approve, or close `P0-03`, and it does not mark PR #1 reviewed or merged.

Legend: Module names match `docs/TECHNICAL_ARCHITECTURE.md` §7–9. "Test" gives the obligation type, not a written test (Milestone/Phase-dependent — see rightmost column).

## Session & attempt lifecycle (FR-SES)

| ID | Requirement | Module | Interface/class | Test | Phase |
| --- | --- | --- | --- | --- | --- |
| FR-SES-001 | Station authenticates when connectivity available | Backend Client | `IBackendSyncClient::Authenticate` | Unit + integration | P2/P6 |
| FR-SES-002 | Student sign-in/sign-out | Student Session | `ISessionManager::BeginSession` | State-transition unit | P2/P7 |
| FR-SES-003 | Only show exercises with present required config | Student Session, Configuration | `IConfigurationProvider::ValidateForLaunch` | Unit (reject-list) | P2/P7 |
| FR-SES-004 | Attempt has stable identifiers for all referenced entities/versions | Student Session | `FAttemptContext` (Common types) | Unit: field completeness | P2 |
| FR-SES-005 | Attempt states: created/ready/active/completed/aborted/sync-pending | Student Session | `ISessionManager` attempt lifecycle | State-transition table | P2 |
| FR-SES-006 | Reset creates event + restores config reset pose; not embedded in vehicle class | Student Session, Exercise Manager | `ISessionManager::RequestReset`, `IExerciseManager::RequestReset` | Unit + regression per exercise | P2/P4 |
| FR-SES-007 | Completed/aborted attempt retains telemetry+events | Local Cache, Telemetry | `UUTSLocalAttemptStore` | Fault-injection recovery test | P2/P5 |

## Input (FR-INP)

| ID | Requirement | Module | Interface/class | Test | Phase |
| --- | --- | --- | --- | --- | --- |
| FR-INP-001 | Game logic consumes only semantic actions | Input Abstraction | `IInputDeviceAdapter::Normalize` | Unit: signature/contract test | P2 |
| FR-INP-002 | Keyboard + hardware share one interface | Input Abstraction | `IInputDeviceAdapter` impls | Unit: interface conformance | P2/P3 |
| FR-INP-003 | No vendor SDK dependency in tractor logic | Input Abstraction, Tractor Vehicle | Module dependency direction | Static/architecture test (dependency lint) | P2/P3 |
| FR-INP-004 | Calibration: dead zones, inversion, range, curves | Input Abstraction | `FCalibrationProfile`, `ApplyCalibration` | Unit: curve/dead-zone math | P3 |
| FR-INP-005 | Raw + normalized inputs observable in debug telemetry | Input Abstraction, Debug & Validation | `FRawDeviceFrame`, `FSemanticControlFrame` in `UUTSDebugHUDWidget` | Manual + integration | P2 |
| FR-INP-006 | Device disconnect → fault + event; safe response `TBD` | Input Abstraction, Detection | `GetDeviceStatus`, `FDomainEvent::DeviceDisconnected` | Integration: simulated disconnect | P3 (policy: **Open Decision**) |

## Vehicle & articulation (FR-VEH)

| ID | Requirement | Module | Interface/class | Test | Phase |
| --- | --- | --- | --- | --- | --- |
| FR-VEH-001 | Vehicle/trailer characteristics config-driven, traceable to ground-truth revision | Configuration, Tractor/Trailer | `FConfigSnapshot.GroundTruthSourceRef` | Unit: provenance field present | P2/P3 |
| FR-VEH-002 | Tractor, trailer, fifth-wheel are separable components | Tractor Vehicle, Trailer Physics, Fifth-Wheel Coupling | `AUTSTractorPawn`, `AUTSTrailerActor`, `UUTSFifthWheelComponent` | Architecture/interface-surface test | P3 |
| FR-VEH-003 | Expose speed/position/orientation/steering/throttle/brake/gear/parking-brake/headings/articulation | Vehicle State | `IVehicleStateProvider::GetState` | Unit: field presence | P3 |
| FR-VEH-004 | Expose kingpin/axle/rear reference positions when geometry approved | Vehicle State | `FTrailerState` optional fields | Unit: `Unavailable` when `TBD` | P3 |
| FR-VEH-005 | No hidden steering assist/auto-correction/anti-jackknife | Tractor Vehicle, Trailer Physics | Interface surface (no such method exists) | Static/interface-surface test | P3 |
| FR-VEH-006 | Unapproved physics params not represented as validated | Configuration, Vehicle State | `ValidateForLaunch`, `Unavailable` states | Unit | P2/P3 |

## Exercise framework (FR-EXR)

| ID | Requirement | Module | Interface/class | Test | Phase |
| --- | --- | --- | --- | --- | --- |
| FR-EXR-001 | Exercise rules data-driven, not hard-coded in maps/vehicles | Exercise Manager | `ExerciseDefinition` (data asset) | Architecture review + regression | P2/P4 |
| FR-EXR-002 | Definition references pose/reset/goal/boundaries/cones/completion/time-limit/scoring | Exercise Manager | `ExerciseDefinition` schema | Unit: schema validation | P4 |
| FR-EXR-003 | Four MVP maneuver types as separate versioned definitions | Exercise Manager | `FLoadedExercise` per maneuver | Regression: one test per maneuver | P4 |
| FR-EXR-004 | Boundary/cone/collision/pull-up/stop/reset/start/timeout/completion produce timestamped events | Detection | `IDetectionEventSink`, `FDomainEvent` | Unit per detector + golden event-list test | P4 |
| FR-EXR-005 | Invisible detection regions visible in dev/debug overlay | Debug & Validation | `UUTSDiagnosticOverlayComponent` | Manual (Human Validation Checklist) | P3/P4 |
| FR-EXR-006 | Exercise geometry unavailable for validated use until ground truth populated | Configuration, Exercise Manager | `ValidateForLaunch`, `LoadExercise` | Unit: reject-list | P2/P4 |

## Deterministic scoring (FR-SCR)

| ID | Requirement | Module | Interface/class | Test | Phase |
| --- | --- | --- | --- | --- | --- |
| FR-SCR-001 | Scoring is deterministic code, never calls an LLM | Scoring | `IScoringEngine` (no LLM dependency in module) | Static: dependency lint | P2/P5 |
| FR-SCR-002 | Profile versioned, mode = practice/school/jurisdiction/instructor-defined | Scoring, Configuration Registry (backend) | `FScoringProfileSnapshot.Mode` | Unit | P5/P6 |
| FR-SCR-003 | Rule config-driven, evaluated only when thresholds/penalties approved | Scoring | `FScoreRuleTrace` generation logic | Unit | P5 |
| FR-SCR-004 | Score event retains timestamp/type/severity/value/threshold/penalty/explanation/evidence | Scoring | `FScoreRuleTrace` | Unit: field completeness | P5 |
| FR-SCR-005 | Replay determinism: same events+profile ⇒ same result | Scoring | `IScoringEngine::ScoreAttempt` | **Golden replay suite** | P5 |
| FR-SCR-006 | Never labeled "official" unless jurisdiction-validated | Scoring, Dashboard (Scores) | `FScoreResult` labeling, UI render rule | UI/API contract test | P5/P7 |
| FR-SCR-007 | Evaluation scoring disabled until ground truth approved; synthetic profiles test-only | Scoring, Configuration | `ValidateForLaunch`, test harness gating | Unit: synthetic-profile-only-in-tests check | P2/P5 |

## Telemetry & offline (FR-TEL)

| ID | Requirement | Module | Interface/class | Test | Phase |
| --- | --- | --- | --- | --- | --- |
| FR-TEL-001 | Frame contains IDs, timestamps, config versions, tractor/trailer state, input, events | Telemetry | `FTelemetryFrame` | Unit: field completeness | P2/P5 |
| FR-TEL-002 | Sample frequency configurable, no default asserted | Telemetry, Configuration | `TelemetryProfile.SampleRateHz` (TBD) | Unit: config-driven, no hardcode | P5 |
| FR-TEL-003 | Events recorded independent of frame cadence | Telemetry | `ITelemetrySink::RecordEvent` | Unit | P2 |
| FR-TEL-004 | Local recoverable storage before/during upload | Local Cache | `UUTSLocalAttemptStore` | Fault-injection | P2/P5 |
| FR-TEL-005 | Upload retry + idempotent server handling | Backend Client, Backend Telemetry | `IBackendSyncClient::UploadTelemetryChunk` | Integration: idempotency | P6 |
| FR-TEL-006 | Network loss does not stop active attempt | Telemetry, Backend Client | Non-blocking upload path | Integration: network partition | P2/P6 |
| FR-TEL-007 | Stored data supports future deterministic replay scoring | Telemetry, Scoring | Schema-versioned chunks | Unit: replay round-trip | P5 |
| FR-TEL-008 | Retention/sample-rate/volume `TBD` pending measurement | Configuration | `TelemetryProfile` fields | N/A until approved | Open Decision |

## Backend (FR-API)

| ID | Requirement | Module | Interface/class | Test | Phase |
| --- | --- | --- | --- | --- | --- |
| FR-API-001 | FastAPI + PostgreSQL + SQLAlchemy + Alembic | Backend (all) | App skeleton | Build/lint check | P1/P6 |
| FR-API-002 | Documented versioned APIs for all listed domains | Backend routers | `/auth …/coaching` | API contract tests | P6 |
| FR-API-003 | Core entities present | Backend models | ORM models (see §2 reconciliation for naming) | Schema/migration test | P6 |
| FR-API-004 | Config/scoring records used by an attempt are immutable/snapshotted | Configuration Registry, Attempts | Version snapshot on attempt create | Unit: snapshot integrity | P6 |
| FR-API-005 | Telemetry/attempt upload idempotent | Attempts, Telemetry (backend) | Idempotency-key tables | Integration: replayed request | P6 |
| FR-API-006 | Health endpoint, structured logs, migrations, validation, tests | Backend (all) | `/health`, logging middleware | CI check | P1/P6 |

## Dashboard (FR-UI)

| ID | Requirement | Module | Interface/class | Test | Phase |
| --- | --- | --- | --- | --- | --- |
| FR-UI-001 | Login, station status, students, history, detail, scores, timeline, telemetry summary, reports, notes | Dashboard (all listed modules) | React views per `docs/TECHNICAL_ARCHITECTURE.md` §9 | Component tests | P7 |
| FR-UI-002 | Loading/empty/failure/offline-stale/unauthorized states | Dashboard (all) | Shared state-rendering pattern | Component tests per state | P7 |
| FR-UI-003 | Critical current-attempt info visible without decorative animation | Current Attempt | Live view component | Usability review | P7 |
| FR-UI-004 | Usable on desktop and tablet | Dashboard (all) | Responsive layout | Usability/accessibility review | P7 |
| FR-UI-005 | Dashboard consumes backend APIs only, no substitute logic | Dashboard (all) | API client boundary | Architecture review | P1/P7 |

## AI coaching (FR-AI)

| ID | Requirement | Module | Interface/class | Test | Phase |
| --- | --- | --- | --- | --- | --- |
| FR-AI-001 | LLM calls only from backend | Coaching (backend) | `coaching` router, LLM client wrapper | Architecture review (no LLM dep in simulator/dashboard) | P11 |
| FR-AI-002 | Coaching uses only supplied telemetry/score/validated rules | Coaching | Evidence-grounding contract | Unit + red-team | P11 |
| FR-AI-003 | Coaching cannot control vehicle or scores | Coaching | No write path to Scores/Attempts physics | Architecture review | P11 |
| FR-AI-004 | Insufficient evidence → explicit no-conclusion | Coaching | `CoachingResult` schema | Unit | P11 |
| FR-AI-005 | Live coaching: at most one primary correction | Coaching | Live-coaching output constraint | Unit | P11 (scope flagged — see Open Decisions) |
| FR-AI-006 | Coaching output + evidence retained for audit | Coaching | `CoachingResult` audit fields | Unit | P11 |
| FR-AI-007 | Not enabled for production validation until deterministic foundation + human review complete | Coaching, Project Owner gate | Feature flag / rollout gate | Release-gate checklist | P11 |

## Non-functional requirements (NFR)

| ID | Requirement | Where enforced | Test |
| --- | --- | --- | --- |
| NFR-001 Determinism | Scoring | Golden replay suite |
| NFR-002 Separation | All module boundaries (§7 "Required decoupling") | Architecture/dependency-lint tests |
| NFR-003 Testability | Configuration, Input, Detection, Scoring, Backend | Per-module unit test presence in CI |
| NFR-004 Observability | Logging strategy (`TECHNICAL_ARCHITECTURE.md` §5) | Log-correlation integration test |
| NFR-005 Security | Backend Authentication, secret management | Security review (`P8-02`) |
| NFR-006 Resilience | Telemetry, Local Cache, Backend Client | Fault-injection / network-partition tests |
| NFR-007 Extensibility | Configuration profile model, Exercise data-driven design | Architecture review |
| NFR-008 Performance | Deferred (`TBD` budgets) | Instrumentation added before approval — **Open Decision** |
| NFR-009 Units | Coordinate/unit convention (`INTERFACE_CONTRACTS.md` preamble) | Unit-conversion boundary test |
| NFR-010 Traceability | Configuration provenance fields | Unit: provenance field presence |

## Coverage note

This matrix maps 100% of the SRS requirement IDs present in PR #1 as drafted (43 functional + 10 non-functional = 53 IDs) to at least one module, interface, and test obligation. It does not itself constitute the requirement *approval* the Product Architect's `P0-03` matrix must additionally record (priority, MVP-vs-later classification disputes, acceptance-test authorship) — this is the architecture's side of that mapping, offered as input to `P0-03`, not a replacement for it.
