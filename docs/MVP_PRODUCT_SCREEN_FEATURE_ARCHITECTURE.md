# United Trucking Simulator — MVP Product, Screen, and Feature Architecture

Status: Definition draft for human approval  
Repository: `kazariana1983/UnitedTruckingSimulator`  
Target: Windows PC, Unreal Engine 5  
Authority: `ground_truth/SIMULATOR_GROUND_TRUTH.md`

## 1. Purpose

United Trucking Simulator is professional CDL Class A training software for a real trucking school. The MVP teaches and evaluates low-speed tractor-trailer backing maneuvers. It is not an entertainment game and must not claim regulatory validity until its physical behavior, yard geometry, and scoring profiles are approved by qualified human reviewers.

The Sora concept video establishes a useful visual direction: a professional menu, a realistic training yard, a first-person cab, mirrors, an objective panel, exterior instructor views, maneuver feedback, and an end-of-attempt summary. It does not define simulator behavior or requirements.

## 2. MVP Boundary

### Included

- Student login and local/session identity
- Exercise selection
- Class 8 day-cab tractor with tandem rear axles
- 53-foot dry-van trailer and articulated fifth-wheel connection
- Straight-line backing
- Offset backing left
- Offset backing right
- 90-degree alley dock
- First-person cab camera
- Left and right mirror cameras
- Exterior instructor camera
- Keyboard developer controls
- Physical steering wheel and pedals through an input abstraction
- Optional clutch and optional H-pattern/range-splitter shifter support
- Device calibration
- Vehicle and exercise reset
- Boundary, cone/collision, pull-up, stop, and completion event detection
- Deterministic scoring from a human-approved configurable profile
- Telemetry recording, local offline cache, upload, and replay
- Attempt results and history
- React/TypeScript instructor dashboard
- Post-attempt AI coaching after human validation of the underlying data and rules

### Excluded from the MVP

- Open-world or highway driving
- AI traffic
- Weather
- Multiplayer
- VR
- Damage simulation
- Vehicle customization
- Cargo systems
- Career or game progression

The Sora clip's highway and rain sequences are therefore visual concept material only and must not enter the first implementation backlog.

## 3. User Roles

| Role | Primary responsibility | MVP access |
| --- | --- | --- |
| Student | Practice assigned backing exercises and review attempts | Login, exercise selection, calibration, training, results, history |
| Instructor | Observe students, review attempts, add notes, and validate training outcomes | Dashboard, live station/attempt view, history, replay, notes |
| Administrator | Maintain accounts, stations, and approved configurations | User/station configuration and approved profile assignment |
| Validator | Approve vehicle feel, yard geometry, detection, and scoring | Validation builds, diagnostic overlays, recorded evidence |

## 4. Student-Side Screen Architecture

| ID | Screen/state | Required content | Primary transition |
| --- | --- | --- | --- |
| S01 | Launch / Health Check | App version, local cache state, backend reachability, connected input devices | Continue to login; permit approved offline mode if available |
| S02 | Student Login | Student credentials/identity and station identification | Successful login opens Home |
| S03 | Home | Start Training, Attempt History, Calibration, Settings, Sign Out | Select Start Training |
| S04 | Exercise Selection | Four MVP maneuvers, availability/assignment, last attempt summary | Select an exercise |
| S05 | Pre-Drive Setup | Exercise name, vehicle/trailer profile, device status, camera check, reset/start controls | Validate prerequisites, then load yard |
| S06 | Calibration | Steering center/range, pedals, optional clutch/shifter, bindings, device profile | Save configuration and return |
| S07 | Training Yard / Cab | Cab view, mirrors, minimal objective/status panel, timer/status only if enabled by configuration | Student performs maneuver |
| S08 | Pause / Reset | Resume, reset attempt, restart exercise, exit; explicit confirmation before discarding attempt | Resume or reset |
| S09 | Attempt Complete | Recorded events, configured score/result, validation labels, attempt ID, review/retry | Retry, next exercise, or Home |
| S10 | Attempt History | Date/time, exercise, completion state, score profile/version, upload state | Open attempt detail |
| S11 | Attempt Detail | Event timeline, telemetry-derived summary, instructor notes, coaching if enabled | Return to history |
| S12 | Error / Recovery | Plain-language failure, retained-data status, safe retry path, diagnostic reference | Retry, work offline, or exit safely |

## 5. Core Training Interaction

1. Authenticate or start an approved offline session.
2. Select one of the four MVP backing exercises.
3. Confirm input-device and vehicle-profile readiness.
4. Load the configured yard layout and place the vehicle at the approved starting pose.
5. Start a uniquely identified attempt.
6. Sample controls and vehicle state; display the cab, mirrors, and minimal objective feedback.
7. Convert relevant simulator observations into timestamped domain events.
8. Persist telemetry and events locally during the attempt.
9. Detect completion, instructor termination, reset, or unrecoverable failure.
10. Replay the event stream through the deterministic scoring engine using a versioned, human-approved scoring profile.
11. Show the result and queue/upload the immutable attempt record.
12. Make the attempt available to the instructor dashboard and, later, the coaching service.

## 6. Presentation Rules Derived from the Concept Video

- The cab is the default student driving view.
- Mirrors must remain usable while driving; they are not decorative overlays.
- Exterior cameras are instructor/diagnostic tools and must not secretly assist the student during a scored attempt.
- The objective panel must be readable at normal display distance and use short messages.
- Training feedback must come from recorded simulator state or deterministic events, never from visual guesswork.
- Results must display the scoring profile and version. Until approved, the UI must label results as unvalidated practice feedback—not official CDL scoring.
- No placeholder percentages from the concept video may be copied into the product.

## 7. Runtime State Model

`Booting → Ready → Authenticated → Configuring → Loading → Active → Paused → Completing → Persisting → Results`

Exceptional states: `Offline`, `Recovering`, `UploadQueued`, and `FatalError`.

Rules:

- Only one active attempt may exist per simulator station.
- An attempt ID is created before motion/event recording begins.
- Reset closes or marks the current attempt according to an approved policy; it must not silently erase evidence.
- Loss of backend connectivity must not corrupt the active local attempt.
- A scoring result is reproducible from the stored attempt data, scoring-profile ID, and profile version.

## 8. Unreal Engine Architecture

Use C++ for core behavior and deterministic logic. Blueprints are limited to presentation, asset wiring, and approved configuration.

| Module | Responsibility | Inputs | Outputs / interfaces |
| --- | --- | --- | --- |
| App & Session | Boot, login/session lifecycle, station identity, active-attempt ownership | Auth response, station config | Session state and attempt context |
| Configuration | Load versioned vehicle, trailer, yard, exercise, input, telemetry, and scoring profiles | Approved configuration payloads | Immutable runtime config snapshot |
| Input Abstraction | Normalize keyboard, wheel, pedals, optional clutch/shifter | Raw device signals and calibration | Named normalized control actions |
| Tractor Vehicle | Low-speed tractor simulation and state exposure | Normalized controls, vehicle config | Pose, velocity, steering, axle/wheel state |
| Trailer & Fifth Wheel | Trailer motion and articulated coupling | Tractor state, coupling/trailer config | Trailer pose, articulation, coupling state |
| Camera & Mirrors | Cab, mirrors, exterior instructor and diagnostic views | Vehicle poses and camera config | Rendered views and camera state |
| Exercise Manager | Exercise lifecycle, start pose, objective state, reset/completion orchestration | Exercise definition and domain events | Exercise state and lifecycle events |
| Detection | Observe configured boundaries, cones/collisions, pull-ups, stops, and completion | Vehicle/trailer poses, collisions, yard/exercise config | Timestamped domain events |
| Telemetry | Sample, buffer, serialize, persist, and stream attempt data | Controls, poses, states, events | Versioned frames, events, local files/uploads |
| Scoring | Produce reproducible results from events and an approved profile | Attempt event stream and scoring profile | Score breakdown, result, rule trace |
| Backend Client | Auth/config sync, heartbeat, attempt upload, retry | API requests and local queue | Responses, sync state, errors |
| Debug & Validation | Physics overlays, sensor/boundary visualization, diagnostic export | Runtime state | Validator-only evidence and reports |

### Required decoupling

- Input devices must not call tractor implementation code directly.
- Vehicle physics must not know exercise or scoring rules.
- Detection creates facts/events; scoring interprets only those events under a versioned profile.
- Exercise definitions must not be embedded in yard maps or vehicle classes.
- Telemetry persistence must survive backend-client failure.
- AI coaching consumes completed attempt data; it cannot control physics, detection, or scoring.

## 9. Backend Architecture

Technology: Python FastAPI and PostgreSQL.

| Service/module | Responsibility |
| --- | --- |
| Authentication & Authorization | Identity, session/token handling, role enforcement |
| Students & Instructors | User records and school relationships |
| Simulator Stations | Station registration, configuration assignment, heartbeat, software version |
| Configuration Registry | Versioned approved vehicle, trailer, yard, exercise, device, and scoring profiles |
| Exercises | Exercise catalog and assignable definitions |
| Attempts | Immutable attempt metadata and lifecycle state |
| Telemetry Ingestion | Validate schemas, accept idempotent uploads, store object references/records |
| Events & Scores | Store event stream, scoring profile/version, rule trace, and reproducible result |
| Reporting | Student history, attempt summaries, trends, exports |
| Instructor Notes | Authored notes attached to attempts/students with audit metadata |
| Coaching | Backend-only LLM orchestration using validated structured attempt data |

Minimum API groups: `/auth`, `/students`, `/instructors`, `/stations`, `/configurations`, `/exercises`, `/attempts`, `/telemetry`, `/scores`, `/notes`, `/reports`, and `/coaching`.

Mutating requests must be authenticated, authorized, validated, logged, and safe to retry where appropriate. Attempt and telemetry upload endpoints need idempotency protection.

## 10. Instructor Dashboard

| Screen | MVP content |
| --- | --- |
| Sign In | Instructor authentication and school context |
| Overview | Connected/offline stations, active attempts, recent completions |
| Students | Searchable student list and latest activity |
| Student Detail | Attempt history, progress summaries, instructor notes |
| Live Attempt | Station health, exercise, elapsed state, event feed, instructor camera if available |
| Attempt Detail | Result, scoring trace, event timeline, synchronized telemetry/replay references, notes |
| Common Mistakes | Aggregated deterministic event categories, with transparent filters |
| Configuration Status | Active profile versions and validation/approval state; editing may be deferred |

The dashboard must distinguish live data, cached data, queued uploads, and unavailable data. It must never convert missing telemetry into a claimed event.

## 11. Core Data Entities

- School
- User
- StudentProfile
- InstructorProfile
- SimulatorStation
- DeviceProfile and CalibrationProfile
- VehicleProfile
- TrailerProfile
- YardProfile
- ExerciseDefinition and ExerciseVersion
- ScoringProfile and ScoringProfileVersion
- TrainingSession
- Attempt
- TelemetryChunk
- DomainEvent
- ScoreResult and ScoreRuleTrace
- InstructorNote
- CoachingResult
- ConfigurationApproval / ValidationRecord

Every attempt must retain the identifiers and versions of all configurations that influenced it.

## 12. Telemetry and Event Contract

The exact sampling rate and field units remain configurable/TBD. The schema must explicitly version and document units.

### Telemetry frame categories

- Attempt ID and monotonic timestamp
- Tractor pose and motion state
- Trailer pose and motion state
- Tractor-trailer articulation measurement
- Steering, throttle, brake, clutch, and shifter inputs when present
- Camera/mode state relevant to validation
- Exercise lifecycle state
- Active configuration/version identifiers
- Integrity flags such as dropped frames or clock discontinuity

### Domain event categories

- Attempt started, paused, resumed, reset, completed, aborted
- Vehicle began/stopped movement
- Boundary interaction
- Cone/collision interaction
- Pull-up candidate/confirmed event
- Completion candidate/confirmed event
- Device disconnected/reconnected
- Backend connection lost/restored
- Telemetry buffer/upload state changes

Detection semantics and thresholds are TBD until defined and approved. Event producers must attach enough source data to audit why an event was emitted.

## 13. Deterministic Scoring Architecture

The scoring engine is a pure, testable calculation:

`Attempt events + approved scoring profile/version → score result + rule trace`

Requirements:

- No LLM or random input.
- No regulatory claim unless the exact jurisdiction-specific rules are validated and approved.
- All thresholds, weights, caps, invalidation conditions, and completion criteria are versioned configuration.
- Replaying the same ordered events under the same scoring profile must return the same result.
- The result retains a rule-by-rule explanation suitable for instructor review and automated tests.
- Missing or corrupt required data produces an explicit unscorable/invalid state, not a guessed score.

## 14. AI Coaching Boundary

AI coaching is Milestone 7, after realism and scoring validation.

Allowed:

- Summarize deterministic events and approved score explanations.
- Compare a student's completed historical attempts.
- Produce short coaching language tied to supplied evidence.

Forbidden:

- Controlling the truck or physics.
- Detecting official faults from video alone.
- Creating or changing scoring rules.
- Inventing telemetry, regulations, dimensions, or causation.
- Presenting unvalidated advice as official examiner guidance.

Every coaching statement must reference available structured evidence, identify uncertainty, and be separable from the official deterministic result.

## 15. Failure and Recovery Requirements

- Backend unavailable: continue an approved local attempt, persist locally, queue upload, and show sync state.
- Input device lost: enter a safe configured response, emit an event, and prevent an invalid attempt from appearing valid.
- Telemetry write failure: notify the session manager, mark data integrity, and stop or invalidate according to approved policy.
- Configuration mismatch: refuse to start and identify the incompatible profile/version.
- Scoring failure: retain the attempt and mark scoring pending/failed; never substitute a score.
- Application crash: recover any finalized chunks and identify the attempt as interrupted.

## 16. Security and Audit Baseline

- Role-based authorization for student, instructor, administrator, and validator actions.
- Secure credential and token storage; no secrets embedded in the simulator or dashboard.
- TLS for network transport.
- Server-side validation for all submitted identifiers, configuration versions, events, and files.
- Immutable attempt audit fields and timestamps.
- Audit logs for score/configuration approval changes and instructor notes.
- Data retention and privacy policy must be defined before a school pilot.

## 17. Testing and Validation Strategy

### Automated

- Unit tests for deterministic detection and scoring logic.
- Golden replay tests: known event stream plus profile produces an exact result.
- Schema compatibility and invalid-input tests.
- Input calibration and mapping tests with simulated devices.
- API authentication, authorization, idempotency, and retry tests.
- Local cache interruption/recovery tests.
- Frontend component and role-access tests.

### Human validation

Use `docs/HUMAN_VALIDATION_CHECKLIST.md` as the minimum gate. A CDL instructor/qualified validator must review steering feel, turning behavior, 53-foot trailer response, mirror perspective, blind spots, yard geometry, event detection, transfer of technique, and absence of hidden assistance.

Unknown facts remain `TBD`; passing software tests does not convert a hypothesis into ground truth.

## 18. Definition-Phase Acceptance Criteria

Architecture approval requires:

- All screens and state transitions are reviewed.
- Each requirement is assigned to Unreal, backend, dashboard, configuration, or validation ownership.
- The four MVP exercises are supported without hard-coded yard geometry.
- Physics, detection, scoring, telemetry, and AI boundaries are explicit.
- Offline attempt preservation and retry behavior are specified.
- Each attempt is reproducible from versioned inputs and stored evidence.
- All unknown dimensions, hardware, physics observations, and scoring rules remain flagged `TBD` pending human approval.
- Excluded Sora scenes have not entered the MVP backlog.

## 19. Dependency-Ordered Implementation Plan

### Milestone 0 — Complete definition first

1. Approve this product/screen/feature architecture.
2. Expand it into traceable software requirements with requirement IDs.
3. Select the actual training tractor and input/display hardware.
4. Measure and approve vehicle/trailer configuration inputs.
5. Measure and approve each yard layout.
6. Define and approve a practice scoring profile.

### Milestone 1 — Executable vertical slice

1. Create the Unreal Engine 5 C++ project and module boundaries.
2. Implement session, configuration, input, exercise, telemetry, scoring interfaces, and debug HUD.
3. Build keyboard developer controls and a gray-box yard.
4. Use clearly labeled temporary configurable values only where necessary to exercise interfaces.
5. Run one non-authoritative straight-line-backing attempt from start through local result generation.

### Later milestones

Proceed through hardware/vehicle, all exercises, data/scoring, platform/dashboard, human validation, and only then AI coaching, matching `backlog/MVP_BACKLOG.md`.

## 20. Immediate Next Artifact

The next concrete artifact is a traceable Software Requirements Specification (SRS) with stable requirement IDs, acceptance tests, and a requirements-to-module matrix. It should resolve the first unchecked Milestone 0 backlog item while carrying every unknown physical or regulatory value as `TBD / human validation required`.
