# Architecture Decisions Requiring Owner / CDL-Instructor Approval

Status: Draft for human review. Nothing in this document is self-approving; it exists so `docs/TECHNICAL_ARCHITECTURE.md`, `docs/INTERFACE_CONTRACTS.md`, and `docs/UE5_WINDOWS_SHELL_HANDOFF.md` can proceed with clearly-flagged assumptions instead of either stalling or silently guessing.

## A. Physical, business, and regulatory facts (owner: Project Owner + CDL Instructor)

These restate and consolidate SRS §12's open-decision list (PR #1) plus the MVP architecture doc's `TBD` items — nothing new is invented here, this is the architecture's confirmation that none of it has been resolved by writing this package:

1. Tractor make/model, wheelbase, overall length, steering range, curb/operating mass, tandem configuration (`ground_truth` §Tractor).
2. Trailer kingpin-to-rear geometry, tandem position, mass configuration (`ground_truth` §Trailer).
3. Straight-line, offset, and alley-dock yard dimensions and cone spacing (`ground_truth` §Training Yard).
4. Practice, school-evaluation, and jurisdiction-specific scoring rules, including pull-up and completion definitions and their edge cases (`ground_truth` §Scoring).
5. Steering wheel, pedal, shifter, and display hardware selection (`ground_truth` §Hardware).
6. Validated physics observations (`ground_truth` §Validated Physics Observations) — currently none.
7. First maneuver selected for end-to-end validation (Master Delivery Plan `P0-04`; SRS §12 recommends straight-line backing, not yet confirmed).
8. Device-disconnect safe response (`FR-INP-006`): stop input in place, hold last value, force pause, or abort the attempt? Affects `IInputDeviceAdapter`/`ISessionManager` behavior directly (`docs/INTERFACE_CONTRACTS.md` §3).
9. Reset policy on an active, evidence-bearing attempt (`FR-SES-006`): does a mid-attempt reset abort-and-keep or reset-in-place-and-keep-prior-telemetry-segment? Affects `ISessionManager::RequestReset`.
10. Telemetry sampling rate, retention window, and acceptable local storage volume (`FR-TEL-002/008`, `NFR-008`).
11. Live-coaching MVP scope (see `docs/SRS_ARCHITECTURE_RECONCILIATION.md` §4): this architecture assumes live coaching is Phase 11 only, not part of the Phase 1–7 vertical slice, despite SRS §3.1 listing it as MVP-included. **Needs explicit Project Owner confirmation** because it changes whether `FR-AI-005` is tracked against an earlier milestone.
12. Privacy/telemetry-retention/deletion policy (Master Delivery Plan `P8-01`) — required before any real student data flows through the pipeline this architecture defines.
13. Numeric performance budgets (frame rate, input latency, network latency) — `NFR-008`.

None of these are resolved, invented, or defaulted anywhere in this PR's documents or code, per Agent Operating Rules #4/#5/#7 and Master Delivery Plan §3 rule 1.

## B. Architecture-level defaults proposed in this package (owner: Project Owner ratifies; Lead Software Architect proposes under `prompts/02_software_architect.md` mandate)

These are engineering conventions, not physical or regulatory facts. They are already applied in `docs/INTERFACE_CONTRACTS.md` and the header scaffold so the team has something concrete to build against; flagging them here so the Project Owner can override any of them before Phase 2 work relies on them further.

1. **Unit/coordinate convention (`NFR-009`).** Proposal: simulate internally in Unreal's native units (centimeters, left-handed, Z-up); convert to SI (meters, radians, seconds) at every module boundary that crosses into Telemetry, Scoring, or the Backend API. Alternative considered: simulate in SI throughout and let only the render layer convert — rejected because it would require overriding Unreal's physics/collision defaults everywhere, increasing risk of subtle unit bugs in exactly the tractor/trailer physics code that most needs to be trustworthy.
2. **Monorepo over polyrepo.** Proposal: single repository with `simulator/`, `backend/`, `dashboard/`, `contracts/`, `infra/` (`docs/TECHNICAL_ARCHITECTURE.md` §2). Rationale: atomic cross-app contract changes; existing repo is already structured this way informally (`docs/`, `backlog/`, `prompts/` at root). Alternative (polyrepo with a published contracts package) would need a release/versioning process not yet scoped.
3. **Engine version pin.** `docs/UE5_WINDOWS_SHELL_HANDOFF.md` recommends "UE5.3 or later" but does not pin an exact version or an Epic Games source-build vs. launcher-build choice — both have licensing/tooling implications (source access, Perforce vs. Git-friendly workflow) that are a Project Owner call, not purely technical.
4. **Session-state vs. attempt-state separation.** Proposal in `docs/SRS_ARCHITECTURE_RECONCILIATION.md` §3 to treat these as two machines rather than reconciling them into one. Low-risk, but flagged because it means two documents' state lists are both "correct" at different layers rather than one superseding the other — worth an explicit nod from whoever reviews `P0-02`.
5. **Entity-naming unification** (`VehicleProfile`/`TrailerProfile`/`ScoreResult`/`ScoreRuleTrace`/unified `User` model) — `docs/SRS_ARCHITECTURE_RECONCILIATION.md` §2. Purely a naming/schema-shape decision; flagged so the Product Architect's eventual `P0-03` matrix and the Backend Engineer's `P6-01` schema use the same names this architecture used, rather than re-deriving a third naming scheme independently.
6. **`UTSCore` has zero rendering/Slate/HTTP dependencies.** Proposed so Configuration/Detection/Scoring stay headless-testable. This constrains later Unreal Foundation Engineer choices (e.g., cannot put a scoring rule behind a Blueprint-only node) and is worth an explicit "yes, keep this constraint" before Phase 2 code accumulates against it.

## What this document does not do

It does not approve `docs/TECHNICAL_ARCHITECTURE.md`, does not close Master Delivery Plan `P0-11`, does not resolve PR #1, and does not add or modify anything in `ground_truth/SIMULATOR_GROUND_TRUTH.md`. Every item in Section A remains `TBD` until a named human approves it; every item in Section B is a reversible engineering default, not a fact, and is called out precisely so it can be reversed cheaply if the Project Owner disagrees.
