# SRS ↔ Architecture Reconciliation

Status: Draft for human review — Lead Software Architect work product
Inputs reconciled:
- `docs/SOFTWARE_REQUIREMENTS_SPECIFICATION.md` as drafted in PR #1 (`codex/milestone-0-srs-draft`, open, unmerged)
- `docs/MVP_PRODUCT_SCREEN_FEATURE_ARCHITECTURE.md`
- `docs/MASTER_DELIVERY_AND_TASK_ASSIGNMENT_PLAN.md`
- `ground_truth/SIMULATOR_GROUND_TRUTH.md`

This document does not approve, merge, or edit PR #1. It does not mark any backlog item complete and does not add anything to ground truth. It records where the two definition documents already agree, where their wording differs only cosmetically (resolved below by adopting one naming convention), and where they genuinely disagree or leave a technical decision open. Genuine disagreements and open technical decisions are carried into `docs/ARCHITECTURE_OPEN_DECISIONS.md` for Project Owner / CDL Instructor sign-off; naming reconciliations are architecture calls made here and used consistently across `docs/TECHNICAL_ARCHITECTURE.md`, `docs/INTERFACE_CONTRACTS.md`, and the `simulator/` header scaffold.

## 1. Points of agreement (no action needed)

- Three independently deployable applications: Unreal simulator client, FastAPI/PostgreSQL backend, React/TypeScript dashboard.
- Four MVP exercises only; excluded-scene list (open-world, highway, AI traffic, weather, multiplayer, VR, damage, customization, cargo, career) is identical in intent across both documents.
- Physics, detection, telemetry, scoring, and AI coaching must stay decoupled behind explicit interfaces; scoring and physics never call an LLM.
- Every physical/scoring value not yet in `ground_truth/SIMULATOR_GROUND_TRUTH.md` stays `TBD` and configuration-driven; missing approved data blocks *validated* use but does not block a labeled-practice/synthetic-data shell.
- Attempt data must survive backend/AI/network unavailability; local recovery precedes upload.
- AI coaching is post-deterministic-launch (Milestone 7 / Phase 11) and cannot alter physics, detection, or scoring.

## 2. Naming/model differences resolved by this architecture

These are engineering-naming decisions within the Lead Software Architect's mandate (prompt `02_software_architect.md`), not new business or physical facts. They are applied consistently in the interface contracts and are listed here for traceability, not as items needing separate approval — though Project Owner may override any of them.

| Topic | SRS (PR #1) wording | MVP Architecture doc wording | Adopted in this architecture |
| --- | --- | --- | --- |
| Unreal module set | "student session manager, exercise manager, telemetry subsystem, scoring subsystem, input device adapter, vehicle state provider, simulator configuration provider, local attempt store/upload queue, backend API client" | Table with `App & Session`, `Configuration`, `Input Abstraction`, `Tractor Vehicle`, `Trailer & Fifth Wheel`, `Camera & Mirrors`, `Exercise Manager`, `Detection`, `Telemetry`, `Scoring`, `Backend Client`, `Debug & Validation` | Superset of the MVP doc's table (finer split of Tractor/Trailer/Fifth-Wheel/Camera is kept because prompt `02` explicitly asks for those as separate modules); SRS's "vehicle state provider" becomes the shared read-only interface those three physics modules publish through. |
| Vehicle/trailer entity naming | `VehicleConfiguration`, `TrailerConfiguration` | `VehicleProfile`, `TrailerProfile` | `VehicleProfile` / `TrailerProfile` (matches the rest of the MVP doc's `*Profile` family: `DeviceProfile`, `CalibrationProfile`, `YardProfile`, `ScoringProfile`). |
| User modeling | Separate `Instructor`, `Student` entities, no shared base | Shared `User` + `StudentProfile` / `InstructorProfile` role profiles | `User` + role profiles (MVP doc's model), because it is the superset and matches `SimulatorStation`/`Admin`/`Validator` roles the MVP doc also names that the SRS's flat model has no place for. |
| Score entity naming | `Score`, `ScoreEvent` | `ScoreResult`, `ScoreRuleTrace` | `ScoreResult` (attempt-level) containing an ordered list of `ScoreRuleTrace` entries (rule-level); SRS's "ScoreEvent" fields (timestamp, event type, severity, measured value, threshold, penalty, explanation, evidence reference — FR-SCR-004) become the required fields of `ScoreRuleTrace`. |
| Attempt vs. session | SRS only defines `Attempt` states | MVP doc has both `TrainingSession` and `Attempt` | Kept distinct: `TrainingSession` = one authenticated station occupancy (may contain zero or more attempts, matches Runtime State Model §7 of the MVP doc); `Attempt` = one scored/scorable maneuver run (matches SRS FR-SES-004/005 field-for-field). |
| Config lifecycle | Not explicit beyond "human-approved" | `ConfigurationApproval` / `ValidationRecord` entities (§11) | Kept; every `*Profile`/`*Version` entity carries a `ConfigurationApproval` reference so SRS §8's draft/human-approved/retired lifecycle and NFR-010 traceability are satisfiable without inventing an extra ad hoc "approved" boolean per table. |

## 3. Two distinct state machines, previously conflated

The MVP doc's Runtime State Model (§7: `Booting → Ready → Authenticated → Configuring → Loading → Active → Paused → Completing → Persisting → Results`, plus `Offline`/`Recovering`/`UploadQueued`/`FatalError`) and the SRS's attempt states (FR-SES-005: `created, ready, active, completed, aborted, synchronization-pending`) describe two different layers, not one machine with two names:

1. **Session/UI state** (owned by `App & Session` / `ISessionManager`): what screen/mode the station is in. One instance per running client.
2. **Attempt lifecycle state** (owned by `Exercise Manager` / attempt record): the state of one maneuver run. Zero-or-one *active* attempt may exist per station (MVP doc §7 rule), but completed/aborted/synchronization-pending attempts persist as historical records while the session itself returns to `Results` → `Ready`.

`docs/INTERFACE_CONTRACTS.md` formalizes both machines separately and defines the (session state, allowed attempt states) pairing so implementers stop guessing which document's state list is authoritative — both are, at different layers.

## 4. Genuine open conflicts / scope ambiguity (not resolved here — see Open Decisions)

- **Live coaching MVP scope.** SRS §3.1 lists "Post-attempt and concise live AI coaching" as MVP-included; the MVP architecture doc's included-feature list (§2) and the Master Delivery Plan (Phase 11, P11-04 "Implement concise live coaching") both treat live coaching as a later increment gated behind post-attempt coaching and human approval, not an MVP-parity feature. This architecture assumes **live coaching is out of the initial vertical-slice and Phase-1–7 scope**, consistent with the Master Delivery Plan's phase ordering, and treats FR-AI-005 as a Phase-11 requirement. **Requires Project Owner confirmation** — see Open Decisions.
- **Device-disconnect safe response (FR-INP-006).** Both documents agree an event must fire; neither specifies the safe response (stop input, hold last value, force pause, abort attempt). Left `TBD` in the input interface contract pending Hardware/Input Engineer + CDL Instructor input.
- **Performance/telemetry numeric budgets (NFR-008, FR-TEL-002/008).** Both documents explicitly defer these. This architecture defines the *shape* of the config (rate is a versioned float field) but supplies no default number.
- **Coordinate frame and unit convention (NFR-009).** Neither document picks one. This is an architecture-level default (see `docs/ARCHITECTURE_OPEN_DECISIONS.md` §B) rather than a physical fact, so it is proposed here for Project Owner ratification rather than left fully open.

## 5. What this reconciliation does *not* do

- It does not resolve PR #1 review comments or merge it. `P0-01` ("Review and finalize draft SRS in PR #1") remains an open Product Architect / Project Owner task.
- It does not populate any `ground_truth/SIMULATOR_GROUND_TRUTH.md` field.
- It does not approve the MVP product/screen/feature architecture (`P0-02`, Project Owner task).
