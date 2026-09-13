# Interface Contracts

Status: Draft for human review — accompanies `docs/TECHNICAL_ARCHITECTURE.md`
Matching header stubs: `simulator/Source/UTSCore/Public/UTS/**`

Conventions used everywhere below:

- Every physical/measured field is typed and unit-labeled but carries no default real-world value; it is populated only from an `Approved` configuration profile sourced from `ground_truth/SIMULATOR_GROUND_TRUTH.md` (`NFR-009`, `NFR-010`).
- Internal simulation math runs in Unreal's native convention (centimeters, left-handed, Z-up, per Epic's default). Every boundary-crossing type (telemetry, backend API, scoring input) converts to SI units (meters, radians, seconds) at the point it leaves `UTSCore`. This split is an architecture default proposed for ratification — see `docs/ARCHITECTURE_OPEN_DECISIONS.md` §B.
- `FAttemptId`, `FStationId`, `FStudentId`, `FConfigVersionRef` are opaque, stable, serializable identifiers shared by every module and by the backend schema — defined once in `Common/UTSCommonTypes.h`.
- "TBD" in a field comment means: the *field* is part of the contract now; the *value* is not supplied until a human approves it in ground truth.

## 1. Session

**Interface:** `ISessionManager` (`Session/ISessionManager.h`)
**Owns:** the App/UI runtime state machine (MVP doc §7) and single-active-attempt-per-station enforcement (`FR-SES-005`, MVP doc §7 rule).

States (session layer — distinct from attempt layer, see `docs/SRS_ARCHITECTURE_RECONCILIATION.md` §3):
`Booting → Ready → Authenticated → Configuring → Loading → Active → Paused → Completing → Persisting → Results`, plus exceptional `Offline`, `Recovering`, `UploadQueued`, `FatalError`.

Contract:
- `BeginSession(StationId, AuthResult) → SessionHandle` — fails into `Offline` (not `FatalError`) when the backend is unreachable but an approved offline policy exists.
- `TryBeginAttempt(ExerciseId, VehicleProfileVersion, TrailerProfileVersion, ScoringProfileVersion) → Result<AttemptHandle, ESessionError>` — rejects a second concurrent attempt on the same station with `ESessionError::AttemptAlreadyActive`; rejects when required config is `Draft`/missing for a validated request with `ESessionError::ConfigurationIncomplete(FieldPath)`.
- `RequestReset(AttemptHandle, EResetPolicy) → void` — always emits a `FDomainEvent` before mutating pose; never silently discards telemetry already written (`FR-SES-006`, MVP doc §7).
- `CompleteOrAbort(AttemptHandle, EAttemptOutcome) → void` — transitions session to `Completing → Persisting → Results`; the attempt record itself moves to the Attempt-lifecycle states below.
- `OnBackendConnectivityChanged(bool bReachable)` — never forces a state transition away from `Active`; only affects `Offline`/`UploadQueued` labeling.

Attempt lifecycle (data layer, `FR-SES-004/005`): `Created → Ready → Active → {Completed | Aborted} → SynchronizationPending → Synchronized`. Every attempt carries stable identifiers for school, station, student, exercise, vehicle/trailer/scoring config versions, telemetry session, and client software version (`FR-SES-004`) — see `FAttemptContext` in the header.

Failure cases: two `TryBeginAttempt` calls race → second fails, first proceeds; app crash mid-`Active` → on relaunch, Session queries Local Cache for an `Active`/unfinalized attempt and surfaces it as `Interrupted`, never silently resumed as if nothing happened.

Tests: state-transition table (every legal/illegal transition), single-active-attempt enforcement, offline-login path, crash-recovery-on-relaunch path.

## 2. Configuration

**Interface:** `IConfigurationProvider` (`Config/IConfigurationProvider.h`)

Contract:
- `LoadProfile<T>(ProfileId, RequestedVersion?) → Result<FConfigSnapshot<T>, EConfigError>` — `T` ∈ {`VehicleProfile`, `TrailerProfile`, `YardProfile`, `ExerciseDefinition`, `DeviceProfile`, `CalibrationProfile`, `ScoringProfile`}. Returns the exact requested version, or latest `Approved` if none requested.
- `ValidateForLaunch(ExerciseId) → Result<FValidatedConfigSet, TArray<FMissingFieldDiagnostic>>` — the single gate exercise start must pass before a *validated* attempt; returns every missing/`Draft` field, not just the first (`FR-EXR-006`, SRS §8).
- `GetApprovalState(ProfileId, Version) → EApprovalState` (`Draft | Approved | Retired`).
- Every returned `FConfigSnapshot` is immutable for the caller's lifetime and carries `{ProfileId, Version, ApprovalState, GroundTruthSourceRef, ApproverId, ApprovedAtUtc}`.

Failure cases: requested a `Retired` version explicitly → returned with a loud "retired, not for new attempts" flag rather than silently substituting latest; local profile file missing/corrupt → `EConfigError::LoadFailed`, never a synthesized default.

Tests: per-profile-type parse/validate; golden reject-list of deliberately incomplete profiles; approval-state transition unit tests.

## 3. Input Devices

**Interface:** `IInputDeviceAdapter` (`Input/IInputDeviceAdapter.h`)

Contract:
- `PollRaw() → FRawDeviceFrame` (per-source: keyboard, wheel/pedal set, optional clutch, optional shifter) — never called directly by vehicle code (`FR-INP-003`).
- `Normalize(FRawDeviceFrame, FCalibrationProfile) → FSemanticControlFrame { Steering[-1,1], Throttle[0,1], Brake[0,1], Clutch[0,1]?, ShiftUp:bool, ShiftDown:bool, GearSelect:int?, ParkingBrake:bool }` — this is the *only* type vehicle/tractor code consumes (`FR-INP-001`).
- `ApplyCalibration(FCalibrationProfile { Center, Range, Inversion, DeadZone, ResponseCurve })`.
- `GetDeviceStatus() → EDeviceStatus (Connected | Disconnected | Faulted)`; a `Disconnected`/`Faulted` transition always emits an `FDomainEvent` and drives `ISessionManager` toward the approved safe response (`TBD` — see Open Decisions) rather than letting the last-known frame silently keep driving the vehicle.
- Keyboard and hardware adapters implement the exact same `IInputDeviceAdapter` surface (`FR-INP-002`); the vehicle layer cannot tell which one is active.

Tests: calibration curve/dead-zone math (pure functions, easy to unit test exhaustively); simulated disconnect mid-frame; raw+normalized frame observability in debug telemetry (`FR-INP-005`).

## 4. Vehicle State

**Interface:** `IVehicleStateProvider` (`Vehicle/IVehicleStateProvider.h`)
**Published by:** Tractor Vehicle + Trailer Physics + Fifth-Wheel Coupling modules; **consumed by:** Camera, Detection, Telemetry. This is the SRS's "vehicle state provider" (§9) unified across the MVP doc's three separate physics modules (`docs/SRS_ARCHITECTURE_RECONCILIATION.md` §2).

Contract (read-only snapshot per tick, `FVehicleState`):
```
FTractorState  { Pose, VelocityMps, SteeringAngleRad, ThrottleNorm, BrakeNorm, GearState, HeadingRad }
FTrailerState  { Pose, HeadingRad, KingpinPos?, AxlePos?, RearRefPos? }   // ? = only when geometry Approved (FR-VEH-004)
FCouplingState { ArticulationAngleRad | Unavailable, ArticulationRateRadPerSec | Unavailable, CouplingHealth }
```
- `GetState() → FVehicleState` — read-only; no method on this interface may mutate exercise or scoring state (`NFR-002`).
- No hidden-assist contract: the interface has no "auto-correct" or "auto-straighten" method and none may be added without an explicit `FR-VEH-005` exception approved by the Project Owner.
- `TBD`-geometry fields resolve to `Unavailable` (a real discriminated state, not `0`/`null` treated as a value) until ground truth supplies them — this is enforced by the type, not by a caller convention (`FR-VEH-004/006`).

Failure cases: `VehicleProfile`/`TrailerProfile` missing approved mass/steering-range/kingpin geometry → the corresponding pawn refuses to spawn into a *validated* exercise (it may still spawn into a labeled-synthetic debug scene).

Tests: none invented for physics behavior in this phase (Agent Operating Rule #12/#5 — no LLM-authored physics facts); this phase tests only that the interface correctly reports `Unavailable` for missing geometry and that no hidden-assist path exists (a static/interface-surface test, not a physics test).

## 5. Exercises

**Interface:** `IExerciseManager` (`Exercise/IExerciseManager.h`)

Contract:
- `LoadExercise(ExerciseDefinitionRef) → Result<FLoadedExercise, TArray<FMissingFieldDiagnostic>>` — an `ExerciseDefinition` is pure data: `{StartPose, ResetPose, GoalRegion, Boundaries[], Cones[], CompletionConditions[], OptionalTimeLimit, ScoringProfileRef}` (`FR-EXR-002`). It is never embedded in a map's level Blueprint or in a vehicle C++ class (`FR-EXR-001`, MVP doc §8).
- `OnDomainEvent(FDomainEvent)` — the Exercise Manager subscribes to Detection's event stream; it does not itself decide pass/fail (that is Scoring's job) but does drive lifecycle transitions (e.g., a `CompletionConfirmed` event moves the exercise to `Completing`).
- `RequestReset(EResetPolicy)`, `RequestStart()`, `GetObjectiveState() → FObjectiveState` (for the minimal HUD panel, MVP doc §6).
- The same `IExerciseManager` implementation and `ExerciseDefinition` schema serve all four MVP maneuvers (`FR-EXR-003`) — no maneuver-specific subclassing of the manager itself; maneuver differences live entirely in the data.

Failure cases: `ExerciseDefinition` references yard geometry not yet `Approved` → `LoadExercise` fails with the missing field(s) named, exercise unavailable in Exercise Selection (S04) rather than loading with guessed geometry (`FR-EXR-006`).

Tests: one lifecycle-state-machine test per exercise type using synthetic `ExerciseDefinition` fixtures; regression test per MVP maneuver once real geometry lands.

## 6. Detection

**Interface:** `IDetectionEventSink` (`Detection/IDetectionEventSink.h`)
**Produces:** `FDomainEvent` facts only. Never computes a score, penalty, or pass/fail — that separation is structural (`NFR-002`, MVP doc §8): Detection's output type has no severity/points field at all; only `ScoreRuleTrace` (Scoring's output) does.

```
FDomainEvent {
  AttemptId, MonotonicTimestamp,
  EventType: EDomainEventType,   // BoundaryEnter/Exit, ConeContact, PullUpCandidate, PullUpConfirmed,
                                  // CompletionCandidate, CompletionConfirmed, VehicleMovementStarted/Stopped,
                                  // DeviceDisconnected/Reconnected, BackendConnectionLost/Restored,
                                  // TelemetryBufferStateChanged, AttemptStarted/Paused/Resumed/Reset/Completed/Aborted
  SourceEvidence: FEventEvidence // enough raw data (pose snapshot, threshold, volume id) to audit *why* this fired
}
```
- `RegisterDetector(IDetector)` — one concrete class per detector kind (`UUTSBoundaryVolume`, `UUTSConeVolume`, `UUTSPullUpDetector`, `UUTSCompletionDetector`), each independently unit-testable against synthetic pose sequences.
- Detection semantics/thresholds are `TBD` (`FR-EXR-006`, `ground_truth` §Training Yard/Scoring both `TBD`); until approved, detectors run in "observe and record only" mode — they still emit `*Candidate` events for audit/tuning but the corresponding `*Confirmed` event (which Exercise Manager and Scoring treat as authoritative) is withheld until a human-approved threshold exists.

Tests: one test per detector type with synthetic pose sequences; golden test — a recorded pose sequence plus config produces an exact, order-stable expected event list (this is the deterministic contract Scoring's replay tests build on).

## 7. Telemetry

**Interface:** `ITelemetrySink` (`Telemetry/ITelemetrySink.h`)

Contract:
```
FTelemetryFrame {
  AttemptId, MonotonicTimestamp,
  TractorState, TrailerState, CouplingState,      // SI units at this boundary
  RawInputFrame?, SemanticControlFrame,
  CameraModeState, ExerciseLifecycleState,
  ActiveConfigVersions: {Vehicle, Trailer, Yard, Exercise, Scoring, Input, Telemetry},
  IntegrityFlags: { DroppedFrame:bool, ClockDiscontinuity:bool }
}
```
- `RecordFrame(FTelemetryFrame)` — sampling rate is a versioned config field (`FR-TEL-002`); no default rate is asserted by this contract.
- `RecordEvent(FDomainEvent)` — independent of the frame cadence (`FR-TEL-003`).
- `Flush() → Result<void, ETelemetryError>` and `GetIntegrityState() → FIntegrityState` — a local write failure sets an integrity flag and notifies `ISessionManager`; it never silently drops data (`FR-TEL-004`, MVP doc §15).
- Schema version travels with every persisted chunk so `contracts/` can evolve without breaking replay of older attempts (`FR-TEL-007`, `NFR-010`).
- Retention limits and sample rate remain `TBD` (`FR-TEL-008`) — the config field exists; no number is asserted here.

Failure cases: disk full mid-attempt → `Flush` fails, integrity flag set, Session Manager decides continue-with-flag vs. stop per approved policy (policy itself is `TBD`, MVP doc §15).

Tests: frame/event (de)serialization round-trip per schema version; fault-injection (disk-full, mid-write crash) recovery test.

## 8. Scoring

**Interface:** `IScoringEngine` (`Scoring/IScoringEngine.h`)
**Purity constraint:** pure function of its inputs — `NFR-001`, `FR-SCR-001/005`. No LLM call, no random input, no live-subsystem read.

```
ScoreAttempt(TArray<FDomainEvent> OrderedEvents, FScoringProfileSnapshot Profile) → FScoreResult
FScoreResult { AttemptId, ProfileId, ProfileVersion, Outcome: (Scored | Unscorable), RuleTrace: TArray<FScoreRuleTrace> }
FScoreRuleTrace { Timestamp, EventType, Severity, MeasuredValue, Threshold, Penalty, Explanation, EvidenceRef }
```
- Same ordered `Events` + same `Profile` version ⇒ byte-identical `FScoreResult`, always (`FR-SCR-005`) — this is the golden-replay contract.
- `Profile.Mode ∈ {Practice, SchoolEvaluation, JurisdictionSpecific, InstructorDefined}` (`FR-SCR-002`); only a profile whose mode+jurisdiction has been explicitly human-validated may be labeled "official" anywhere in UI/API (`FR-SCR-006`).
- Missing/corrupt required event data ⇒ `Outcome = Unscorable` with an explanation, never a guessed `Scored` result (`FR-EXR`/`FR-SCR-004`, MVP doc §13).
- Until `ground_truth/SIMULATOR_GROUND_TRUTH.md` §Scoring is populated and approved, only test-only synthetic profiles may be used, and only in automated tests — never presented to a student as a real result (`FR-SCR-007`).

Tests: rule-by-rule unit tests once rules are approved; **golden replay suite** (fixed event stream + profile ⇒ exact result) run on every scoring-code change, from day one using synthetic profiles.

## 9. Backend Sync

**Interface:** `IBackendSyncClient` (`Backend/IBackendSyncClient.h`)

Contract:
- `Authenticate(StationCredentials) → Result<FAuthResult, EBackendError>`.
- `SyncConfigurations() → TArray<FConfigSnapshot>` (pull-only; the simulator never writes configuration to the backend).
- `Heartbeat(FStationStatus)`.
- `UploadAttempt(FAttemptId, IdempotencyKey) → Result<FUploadReceipt, EBackendError>` and `UploadTelemetryChunk(FAttemptId, ChunkSeq, IdempotencyKey, bytes)` — both idempotent; a retried call with the same key never creates a duplicate record server-side (`FR-TEL-005`, `FR-API-005`).
- `GetSyncState() → ESyncState (Idle | Queued | Uploading | Retrying | Failed | Complete)` — surfaced by the dashboard and by S10/S12 (attempt history / error screens) so "unavailable" is never rendered as "confirmed."
- Every method is non-blocking with respect to the active attempt: `Local Cache` and `Telemetry` continue writing regardless of this interface's state (`FR-TEL-006`, MVP doc §15).

Failure cases: network partition mid-upload → queued with backoff, session stays `Active`/`UploadQueued`, never corrupts the local attempt (`NFR-006`); backend rejects a config version the station doesn't recognize → surfaced as a configuration-mismatch fault (SRS §15), attempt does not start.

Tests: idempotency-key correctness; retry/backoff schedule; simulated network-partition integration test; offline/sync fault suite (`P6-10`, owned by Independent QA Reviewer — this contract is what that suite tests against).

## Cross-cutting: `Common/UTSCommonTypes.h`

Shared, dependency-free types every module above references: `FAttemptId`, `FStationId`, `FStudentId`, `FInstructorId`, `FConfigVersionRef { ProfileId, Version }`, `EApprovalState`, `FMissingFieldDiagnostic { FieldPath, Reason }`, monotonic timestamp type. Defined once so no module pair develops incompatible ad hoc ID types.
