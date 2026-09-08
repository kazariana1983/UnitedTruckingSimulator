# CDL Class A Simulator — MVP Software Requirements Specification

Status: Draft for human review  
Milestone: 0 — Definition  
Authority: `ground_truth/SIMULATOR_GROUND_TRUTH.md`

## 1. Purpose

This document defines the executable MVP requirements for a commercial CDL Class A trucking-school simulator focused on low-speed tractor-trailer backing practice and evaluation.

This is not an entertainment game. The MVP must provide repeatable practice, deterministic evaluation, telemetry, attempt history, and instructor review while keeping vehicle physics, official scoring, and AI coaching separated.

## 2. Authority and unknown-value policy

`ground_truth/SIMULATOR_GROUND_TRUTH.md` is the sole authority for human-approved vehicle measurements, training-yard dimensions, scoring rules, hardware facts, and validated physics observations.

- A value marked `TBD` in ground truth remains `TBD` in implementation.
- Software may expose an unknown value as a required configuration field, but must not supply a plausible or default real-world value.
- Missing required ground truth must block the affected exercise from validated/evaluation use with a clear diagnostic.
- Practice-only behavior using provisional data must be explicitly labeled unvalidated and must not be presented as official CDL/DMV scoring.
- AI services must not create, modify, infer, or override ground-truth facts, physics, or scores.

## 3. MVP scope

### 3.1 Included

- Windows PC simulator station
- Unreal Engine 5 client with C++ core logic
- Class 8 day-cab tractor with tandem rear axles
- 53-foot dry-van trailer with fifth-wheel articulation
- Keyboard developer controls
- Physical steering wheel and accelerator/brake pedal support through an input abstraction
- Optional clutch and optional shifter support when configured
- Single display initially, with architecture that does not prevent future multi-display support
- First-person cab view, left/right mirror views, and exterior instructor view
- Straight-line backing
- Offset backing left
- Offset backing right
- 90-degree alley dock
- Student session and exercise selection
- Reset, start, completion, and attempt lifecycle
- Collision, boundary, cone-contact, pull-up, stop, timeout, and completion event plumbing
- Deterministic, configurable scoring
- Telemetry recording and offline-safe upload queue
- FastAPI/PostgreSQL platform services
- React/TypeScript instructor dashboard
- Post-attempt and concise live AI coaching based only on supplied telemetry and validated rules

### 3.2 Excluded

- Open-world driving
- Highway driving
- AI traffic
- Multiplayer
- Virtual reality
- Weather
- Damage simulation
- Vehicle customization
- Cargo simulation
- Career or game-progression systems
- Force feedback in the MVP

## 4. Users and roles

| Role | Required capabilities |
| --- | --- |
| Admin | Manage school configuration, users, stations, exercises, and approved configuration records. |
| Instructor | Review station status, students, attempts, scores, telemetry summaries, and instructor notes. |
| Student | Sign in at a station, select an available exercise, perform attempts, and review permitted feedback. |
| Simulator station | Authenticate as a station, report heartbeat/status, run attempts offline when needed, and synchronize results. |

Authorization must be enforced by the backend. The dashboard must not simulate authorization only in the UI.

## 5. System context

The MVP consists of three independently deployable applications:

1. Unreal simulator client: input, vehicle/exercise runtime, event detection, deterministic local scoring, telemetry, local cache, and backend synchronization.
2. Platform backend: authentication, configuration, students, attempts, telemetry ingestion, scores, reporting data, and coaching orchestration.
3. Instructor dashboard: operational status and read/write instructor workflows through backend APIs only.

The simulator must remain usable for configured offline practice. Network or AI failure must not stop local physics, event detection, telemetry capture, or deterministic scoring.

## 6. Functional requirements

### 6.1 Session and attempt lifecycle

- **FR-SES-001** The station shall authenticate with the backend when connectivity is available.
- **FR-SES-002** The simulator shall support student sign-in and sign-out.
- **FR-SES-003** The simulator shall display only exercises whose required configuration is present.
- **FR-SES-004** An attempt shall have stable identifiers for school, station, student, exercise, vehicle configuration, trailer configuration, scoring profile, telemetry session, and software/configuration versions.
- **FR-SES-005** Attempt states shall include at least created, ready, active, completed, aborted, and synchronization-pending.
- **FR-SES-006** Reset shall create an event and restore the configured reset pose; reset behavior shall not be embedded in a vehicle class.
- **FR-SES-007** A completed or aborted attempt shall retain its captured telemetry and events.

### 6.2 Input

- **FR-INP-001** Game logic shall consume semantic actions: steering, throttle, brake, clutch, shift up, shift down, gear selection, and parking brake.
- **FR-INP-002** Keyboard developer controls and hardware adapters shall implement the same semantic interface.
- **FR-INP-003** Vendor-specific SDKs shall not be dependencies of tractor logic.
- **FR-INP-004** Profiles shall support calibration, inversion, dead zones, steering range, and sensitivity curves.
- **FR-INP-005** Raw and normalized inputs shall be observable in debug telemetry.
- **FR-INP-006** Device disconnect shall generate a visible fault and a telemetry event; the configured safe response is `TBD` pending human approval.

### 6.3 Vehicle and articulation

- **FR-VEH-001** Tractor and trailer characteristics shall be configuration-driven and traceable to an approved ground-truth revision.
- **FR-VEH-002** The runtime shall represent a tractor, semitrailer, and fifth-wheel pivot as separable components.
- **FR-VEH-003** The system shall expose speed, position, orientation, steering input/angle, throttle, brake, gear, parking brake, tractor heading, trailer heading, and articulation angle/rate.
- **FR-VEH-004** Trailer telemetry shall expose kingpin, trailer axle, and trailer rear reference positions when their required geometry is approved.
- **FR-VEH-005** No hidden steering assistance, automatic correction, or artificial jackknife prevention shall be applied.
- **FR-VEH-006** Physics parameters lacking approved values shall not be represented as validated.

### 6.4 Exercise framework

- **FR-EXR-001** Exercise rules shall be data-driven and shall not be hard-coded in maps or vehicle classes.
- **FR-EXR-002** An exercise definition shall reference starting pose, reset pose, goal region, boundaries, cones, completion conditions, optional time limit, and scoring profile.
- **FR-EXR-003** The four MVP maneuver types shall be supported as separate versioned exercise definitions.
- **FR-EXR-004** Boundary, cone, collision, pull-up, stop, reset, start, timeout, and completion observations shall produce timestamped events.
- **FR-EXR-005** Invisible detection regions and identifiers shall be available through a development/debug overlay.
- **FR-EXR-006** Exercise geometry remains unavailable for validated use until the corresponding dimensions are populated in ground truth.

### 6.5 Deterministic scoring

- **FR-SCR-001** Scoring shall be implemented in deterministic code and shall never call an LLM.
- **FR-SCR-002** A scoring profile shall be versioned and identify its mode: practice, school evaluation, jurisdiction-specific, or instructor-defined.
- **FR-SCR-003** A scoring rule shall be configuration-driven and may evaluate supported event/measurement types only when its thresholds and penalties are human-approved.
- **FR-SCR-004** Each score event shall retain timestamp, event type, severity, measured value, threshold, penalty, explanation, and evidence reference when applicable.
- **FR-SCR-005** Replaying identical versioned telemetry and events with the same scoring profile shall produce the same result.
- **FR-SCR-006** The UI/API shall not label any profile official unless that exact profile is human-validated for the named jurisdiction.
- **FR-SCR-007** Until ground truth contains approved rules, evaluation scoring shall be disabled; plumbing and zero-assumption test profiles may be used only in automated tests.

### 6.6 Telemetry and offline operation

- **FR-TEL-001** Telemetry shall contain stable attempt/session identifiers, timestamps, configuration versions, tractor state, trailer state, raw/normalized input, and exercise events.
- **FR-TEL-002** Sample frequency shall be configurable; no default frequency is approved by this SRS.
- **FR-TEL-003** Events shall be recorded independently of periodic frames.
- **FR-TEL-004** The client shall write attempts to recoverable local temporary storage before or during upload.
- **FR-TEL-005** Upload shall support retry and idempotent server handling.
- **FR-TEL-006** Loss of network access shall not stop an active attempt.
- **FR-TEL-007** Stored data shall support future deterministic replay scoring.
- **FR-TEL-008** Retention limits, sampling frequency, and acceptable data volume are `TBD` pending measurement and owner approval.

### 6.7 Backend

- **FR-API-001** The backend shall use FastAPI, PostgreSQL, SQLAlchemy, and Alembic.
- **FR-API-002** It shall expose documented, versioned APIs for authentication, users, schools, stations, configurations, exercises, attempts, telemetry upload, completion, scores, history, notes, and heartbeat/status.
- **FR-API-003** Core entities shall include School, Instructor, Student, SimulatorStation, VehicleConfiguration, TrailerConfiguration, Exercise, Attempt, TelemetrySession, Score, ScoreEvent, and InstructorNote.
- **FR-API-004** Configuration and scoring records used by an attempt shall be immutable by version or snapshotted so historical results remain reproducible.
- **FR-API-005** Telemetry and attempt upload operations shall be idempotent.
- **FR-API-006** A health endpoint, structured logs, schema migrations, validation, and automated tests shall be provided.

### 6.8 Instructor dashboard

- **FR-UI-001** The dashboard shall provide login, station status, students, attempt history, attempt detail, scores, event timeline, telemetry summary, reports, and instructor notes.
- **FR-UI-002** It shall display loading, empty, failure, offline/stale, and unauthorized states.
- **FR-UI-003** Critical current-attempt information shall be visible without decorative animation.
- **FR-UI-004** The dashboard shall be usable on desktop and tablet layouts.
- **FR-UI-005** The frontend shall consume backend APIs and shall not contain substitute backend or scoring logic.

### 6.9 AI coaching

- **FR-AI-001** LLM calls shall occur only through backend services.
- **FR-AI-002** Coaching shall use only supplied telemetry, deterministic score results, and validated training rules.
- **FR-AI-003** Coaching shall not control the vehicle or determine/alter scores.
- **FR-AI-004** When evidence is insufficient, coaching shall state that no supported conclusion can be made.
- **FR-AI-005** Live coaching shall identify at most one primary actionable correction at a time.
- **FR-AI-006** Coaching output and its input evidence references shall be retained for auditability.
- **FR-AI-007** AI coaching shall not be enabled for production validation until the deterministic foundation and human review are complete.

## 7. Non-functional requirements

- **NFR-001 Determinism:** Scoring results must be reproducible from versioned inputs.
- **NFR-002 Separation:** Physics, exercise detection, scoring, telemetry, presentation, and AI coaching must remain decoupled behind explicit interfaces.
- **NFR-003 Testability:** Configuration parsing, calibration, event detection, scoring, retry/idempotency, and authorization must be independently testable.
- **NFR-004 Observability:** Logs must be structured and correlate station, session, attempt, and upload operations without storing secrets.
- **NFR-005 Security:** Passwords must use secure one-way storage; secrets must come from environment/runtime configuration; role authorization must be server-enforced.
- **NFR-006 Resilience:** An active attempt must survive backend/AI unavailability and preserve data for later synchronization.
- **NFR-007 Extensibility:** Additional tractors, trailers, exercises, schools, stations, displays, and scoring profiles must not require rewriting core interfaces.
- **NFR-008 Performance:** Numeric frame-rate, latency, and resource budgets are `TBD`; initial implementation must instrument them before approval.
- **NFR-009 Units:** Every physical field must declare its unit and coordinate-frame convention. Unit conversion must occur at defined boundaries.
- **NFR-010 Traceability:** Every approved physical/scoring configuration must reference its ground-truth source/revision and approval state.

## 8. Configuration and validation states

Every vehicle, trailer, yard, hardware, and scoring configuration shall carry:

- stable identifier and version
- lifecycle state: draft, human-approved, retired
- source/ground-truth revision
- explicit units for physical values
- creator and approval metadata
- validation notes

The application shall reject a transition to validated/evaluation use when required fields are absent or draft. A validation error shall identify each missing ground-truth field.

## 9. Required interfaces for the executable foundation

The first Unreal implementation must define independently testable interfaces/components for:

- student session manager
- exercise manager
- telemetry subsystem
- scoring subsystem
- input device adapter
- vehicle state provider
- simulator configuration provider
- local attempt store/upload queue
- backend API client
- structured logging
- debug HUD/diagnostics

Unreal C++ owns core logic. Blueprints may configure data and presentation but shall not become the sole implementation of scoring, telemetry persistence, or session state.

## 10. Acceptance criteria by capability

### 10.1 Definition gate

- This SRS is reviewed and approved by the project owner.
- A technical architecture maps every MVP requirement to a responsible module.
- Ground-truth vehicle, yard, and practice-scoring entries needed for the first validated maneuver are supplied and approved by humans.
- No architecture or prototype contains invented physical or scoring values presented as validated.

### 10.2 Foundation executable

- A Windows development build starts and reaches a simulator shell/debug scene.
- A developer can start, reset, complete, and abort a mock/data-only attempt.
- Keyboard input flows through the same semantic abstraction intended for hardware.
- The debug HUD shows current session/attempt state and raw/normalized input.
- Telemetry frames and lifecycle events are written locally.
- The deterministic scoring interface accepts a test-only synthetic profile and produces repeatable results.
- Missing ground-truth-dependent exercise data prevents validated exercise launch with a clear diagnostic.

### 10.3 Platform slice

- Local development starts the backend and database through documented commands.
- Migrations create the required initial schema.
- A station heartbeat and one attempt summary can be uploaded idempotently.
- An instructor can sign in and view the uploaded attempt in the dashboard.
- The simulator can complete an attempt while the backend is unavailable and synchronize it later.

### 10.4 Validation gate

- Physics observations are measured and reviewed by qualified humans before being added to ground truth.
- Practice and evaluation scoring profiles are separately identified and reviewed.
- Telemetry replay proves scoring determinism.
- An independent QA review reports defects by severity.
- AI coaching is tested for evidence grounding and inability to affect scoring/physics.

## 11. Initial dependency order

1. Approve this SRS.
2. Produce and approve the technical architecture and repository structure.
3. Collect and approve the minimum ground truth for one target tractor/trailer configuration, one training-yard maneuver, the selected input hardware, and one practice scoring profile.
4. Create the Unreal foundation executable with configuration validation and test-only synthetic data.
5. Implement input calibration and hardware adapters.
6. Implement and validate tractor, fifth-wheel, and trailer behavior.
7. Implement exercise event detection and telemetry.
8. Implement deterministic scoring and replay verification.
9. Implement backend, synchronization, and dashboard vertical slice.
10. Perform independent QA and instructor validation.
11. Add AI coaching only after deterministic systems and human validations are in place.

## 12. Open decisions — human input required

The following are deliberately unresolved because the authoritative file currently marks them `TBD`:

- tractor make/model, dimensions, steering range, mass, and tandem configuration
- trailer kingpin/rear geometry, tandem position, and mass configuration
- all four exercise/yard dimensions and cone spacing
- practice, school-evaluation, and jurisdiction-specific scoring rules
- steering wheel, pedals, shifter, and display configuration
- validated physics observations and tolerances

Additional product/engineering approvals required:

- authentication/session policies
- privacy, telemetry retention, and deletion requirements
- numeric performance budgets
- offline retention capacity and synchronization policy
- first maneuver chosen for end-to-end validation
- acceptance authority and sign-off procedure

## 13. Traceability to repository backlog

This draft addresses only `Milestone 0 - Complete software requirements specification`. It does not mark that item complete; human review and approval are required. It does not satisfy architecture approval or populate any ground-truth/scoring item.
