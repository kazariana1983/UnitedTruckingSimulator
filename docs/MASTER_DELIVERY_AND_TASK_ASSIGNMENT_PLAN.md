# United Trucking Simulator — Master Delivery and Task Assignment Plan

Status: Execution plan  
Target release: Production-ready MVP for a trucking-school pilot on Windows PC  
Repository: `kazariana1983/UnitedTruckingSimulator`  
Authority: `ground_truth/SIMULATOR_GROUND_TRUTH.md`

## 1. Launch definition

The MVP is launch-ready only when a student can sign in at a Windows simulator station, calibrate approved controls, complete all four backing exercises, receive reproducible practice results, preserve attempts through network interruption, and have the attempts reviewed in the instructor dashboard.

Launch does not mean public app-store distribution. The simulator is a Windows training-station application plus a hosted backend and web dashboard. The initial launch target is a controlled trucking-school pilot.

The MVP excludes open-world/highway driving, traffic, weather, multiplayer, VR, damage, cargo, customization, and career/game systems.

## 2. Assignment model

| Owner | Responsibility |
| --- | --- |
| Project Owner | Scope, budget, approvals, equipment purchase, privacy decisions, and launch sign-off |
| Product Architect | SRS, traceability, acceptance criteria, and scope control |
| Lead Software Architect | Technical architecture, repository structure, integration contracts, and release coordination |
| Unreal Foundation Engineer | UE5 project, core runtime, configuration, sessions, build pipeline, and debug tooling |
| Hardware/Input Engineer | Keyboard controls, device abstraction, calibration, wheel/pedal/shifter adapters |
| Tractor Physics Engineer | Tractor low-speed behavior and instrumented tuning |
| Trailer Physics Engineer | Trailer dynamics and fifth-wheel articulation |
| Exercise Framework Engineer | Exercise definitions, lifecycle, detection volumes, maneuver implementations |
| Telemetry Engineer | Frame/event contracts, local persistence, upload queue, and replay |
| Scoring Engineer | Deterministic scoring profiles, rule trace, and replay determinism tests |
| Backend Engineer | FastAPI, PostgreSQL, authentication, APIs, station sync, deployment |
| Dashboard Engineer | React/TypeScript instructor workflows and production web build |
| AI Instructor Engineer | Evidence-grounded coaching after deterministic systems are approved |
| Independent QA Reviewer | Test plans, code review, defect triage, regression and release verification |
| Physics Diagnostic Reviewer | Measurements, overlays, behavior comparisons, and tuning evidence |
| CDL Instructor / Human Validator | Yard rules, scoring, vehicle realism, technique transfer, pilot approval |

An “owner” is accountable for the deliverable. Supporting roles may contribute, but one owner must close each task. AI roles may draft specifications and code; only the Project Owner and qualified human validators can approve physical facts, training rules, and release readiness.

## 3. Non-negotiable execution rules

1. Never invent dimensions, scoring rules, physics facts, or CDL regulations.
2. Keep unknown values `TBD` and configuration-driven.
3. Do not start a dependent task until its gate is satisfied.
4. Physics, detection, telemetry, scoring, UI, and AI remain decoupled.
5. LLMs never control physics or official/deterministic scoring.
6. Every deterministic subsystem receives automated tests.
7. Every implementation task ends with build/test evidence and documentation.
8. Every pull request is reviewed before merge; protected behavior is not replaced casually.
9. Only human-approved data enters `ground_truth/SIMULATOR_GROUND_TRUTH.md`.
10. “Looks realistic” is not validation; measured evidence and instructor review are required.

## 4. Delivery sequence

### Phase 0 — Definition and blocking human inputs

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P0-01 | Review and finalize draft SRS in PR #1 | Product Architect | None | Conflicts resolved; requirements reviewed; PR approved and merged |
| P0-02 | Approve product/screen/feature architecture | Project Owner | P0-01 | Approval recorded; incompatible SRS details corrected |
| P0-03 | Create requirement traceability matrix | Product Architect | P0-01, P0-02 | Every SRS requirement maps to module, test, and milestone |
| P0-04 | Select first validation maneuver | Project Owner + CDL Instructor | P0-02 | One of the four MVP exercises selected; recommendation: straight-line backing |
| P0-05 | Select actual tractor/trailer configuration | Project Owner + CDL Instructor | None | Specific training vehicle identified for measurement |
| P0-06 | Select input/display hardware | Project Owner + Hardware/Input Engineer | None | Wheel, pedals, optional shifter/clutch, and display configuration recorded |
| P0-07 | Measure vehicle/trailer facts | Physics Diagnostic Reviewer | P0-05 | Measurement worksheet reviewed and approved by human validator |
| P0-08 | Measure all MVP yard layouts | CDL Instructor + Physics Diagnostic Reviewer | P0-04 | Starting poses, boundaries, goal regions, cone positions, and units approved |
| P0-09 | Define practice scoring profile | CDL Instructor + Scoring Engineer | P0-08 | Versioned human-approved rules with event definitions and edge cases |
| P0-10 | Update ground truth | Project Owner | P0-06 through P0-09 | Only approved facts committed; remaining unknowns stay TBD |
| P0-11 | Approve technical architecture and repo structure | Lead Software Architect | P0-01 through P0-03 | Module boundaries, contracts, build/test strategy approved |

Gate G0: SRS and architecture approved; first maneuver, hardware, required geometry, and practice-scoring inputs are human-approved. Code scaffolding can begin before every physical value is approved, but validated exercise/physics claims cannot.

### Phase 1 — Repository and engineering foundation

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P1-01 | Establish monorepo structure | Lead Software Architect | P0-11 | `simulator/`, `backend/`, `dashboard/`, `contracts/`, `infra/`, `docs/`, test conventions |
| P1-02 | Add contribution and branching rules | Lead Software Architect | P1-01 | PR template, definition of done, review rules, protected main strategy |
| P1-03 | Add CI foundation | Lead Software Architect | P1-01 | Automated checks for backend, dashboard, schemas, and available Unreal checks |
| P1-04 | Define shared IDs, units, coordinates, schemas | Lead Software Architect + Telemetry Engineer | P0-03, P1-01 | Versioned contract package; unit and coordinate conventions documented |
| P1-05 | Create Unreal Engine 5 C++ project | Unreal Foundation Engineer | P1-01 | Windows development build opens a shell/debug scene |
| P1-06 | Create backend skeleton | Backend Engineer | P1-01 | FastAPI app, health endpoint, settings, tests, lint/type checks |
| P1-07 | Create database foundation | Backend Engineer | P1-06 | PostgreSQL local environment, SQLAlchemy models base, Alembic migration path |
| P1-08 | Create dashboard skeleton | Dashboard Engineer | P1-01 | React/TypeScript app, routing, API client boundary, tests, production build |
| P1-09 | Document one-command local setup | Lead Software Architect | P1-05 through P1-08 | Fresh-machine setup and verification instructions |

Gate G1: all three applications build; CI runs; the Windows simulator reaches a debug shell; backend health and dashboard shell work locally.

### Phase 2 — Simulator runtime vertical slice

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P2-01 | Implement versioned configuration system | Unreal Foundation Engineer | P1-04, P1-05 | Loads/validates profiles; missing approved values block validated mode |
| P2-02 | Implement session manager | Unreal Foundation Engineer | P2-01 | Created/ready/active/completed/aborted/sync-pending states tested |
| P2-03 | Implement exercise manager interface | Exercise Framework Engineer | P2-01, P2-02 | Data-driven exercise lifecycle independent of map/vehicle classes |
| P2-04 | Implement input abstraction | Hardware/Input Engineer | P1-04, P1-05 | Semantic controls exposed independent of device vendor |
| P2-05 | Implement keyboard developer controls | Hardware/Input Engineer | P2-04 | Keyboard drives semantic controls and appears in diagnostics |
| P2-06 | Implement telemetry interface and local writer | Telemetry Engineer | P1-04, P2-02 | Frames/events persisted under stable attempt IDs |
| P2-07 | Implement scoring interface | Scoring Engineer | P1-04, P2-03 | Test-only synthetic profile produces reproducible output |
| P2-08 | Implement debug HUD and structured logging | Unreal Foundation Engineer | P2-02 through P2-07 | Session, inputs, config versions, telemetry health, and faults visible |
| P2-09 | Complete data-only attempt vertical slice | Lead Software Architect | P2-01 through P2-08 | Start, reset, complete, abort, store, and rescore a mock attempt |

Gate G2: a non-authoritative exercise shell completes end to end using synthetic test data without implying validated physics or scoring.

### Phase 3 — Hardware, tractor, trailer, cameras

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P3-01 | Implement generic wheel/pedal adapter | Hardware/Input Engineer | P0-06, P2-04 | Approved devices connect through abstraction; disconnect events emitted |
| P3-02 | Implement calibration UI and profiles | Hardware/Input Engineer | P3-01 | Center/range, inversion, dead zones, and curves save/load per device |
| P3-03 | Implement optional clutch/shifter adapters | Hardware/Input Engineer | P0-06, P3-01 | Configured hardware works without affecting keyboard path |
| P3-04 | Implement tractor low-speed physics | Tractor Physics Engineer | P0-07, P2-01, P2-05 | Instrumented, configuration-driven tractor behavior with no hidden assist |
| P3-05 | Implement fifth-wheel coupling | Trailer Physics Engineer | P0-07, P3-04 | Articulated connection reports auditable state and failure diagnostics |
| P3-06 | Implement 53-foot trailer physics | Trailer Physics Engineer | P3-05 | Trailer behavior instrumented and configuration-driven |
| P3-07 | Implement cab camera | Unreal Foundation Engineer | P3-04 | Stable first-person view tied to approved driver reference |
| P3-08 | Implement left/right mirrors | Unreal Foundation Engineer | P3-04, P3-06 | Usable mirror views with validator-adjustable configuration |
| P3-09 | Implement exterior instructor camera | Unreal Foundation Engineer | P3-04, P3-06 | Instructor/diagnostic view; unavailable as hidden scored-attempt aid |
| P3-10 | Implement physics diagnostic overlays | Physics Diagnostic Reviewer | P3-04 through P3-09 | Poses, pivots, headings, articulation, contacts, and reference points visible |
| P3-11 | Conduct first realism tuning review | CDL Instructor + Physics Diagnostic Reviewer | P3-10 | Written findings; approved observations added to ground truth by owner |

Gate G3: approved hardware operates; tractor/trailer/coupling/cameras are reviewable; human validation findings are recorded. Gate does not pass on developer opinion alone.

### Phase 4 — Exercises and event detection

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P4-01 | Implement yard/exercise data schema | Exercise Framework Engineer | P0-08, P1-04, P2-03 | Versioned poses, boundaries, cones, goals, and completion conditions |
| P4-02 | Implement boundary detection | Exercise Framework Engineer | P4-01, P3-06 | Auditable timestamped events with tests |
| P4-03 | Implement cone/collision detection | Exercise Framework Engineer | P4-01, P3-06 | Auditable timestamped events with tests |
| P4-04 | Implement movement/stop detection | Exercise Framework Engineer | P4-01, P3-06 | Configurable human-approved semantics with tests |
| P4-05 | Implement pull-up detection | Exercise Framework Engineer | P0-09, P4-04 | Events match human-approved definition and edge cases |
| P4-06 | Implement completion detection | Exercise Framework Engineer | P0-08, P0-09, P4-01 | Configured completion logic with test evidence |
| P4-07 | Build straight-line backing | Exercise Framework Engineer | P4-01 through P4-06 | Full start/reset/complete flow with approved geometry |
| P4-08 | Build offset backing left | Exercise Framework Engineer | P4-07 | Approved exercise definition and regression tests |
| P4-09 | Build offset backing right | Exercise Framework Engineer | P4-07 | Approved exercise definition and regression tests |
| P4-10 | Build 90-degree alley dock | Exercise Framework Engineer | P4-07 | Approved exercise definition and regression tests |
| P4-11 | Add exercise/debug visualization | Exercise Framework Engineer | P4-01 through P4-10 | Validator can inspect every invisible region and event trigger |
| P4-12 | Instructor detection review | CDL Instructor | P4-11 | Boundary, cone, pull-up, stop, and completion findings signed off or returned |

Gate G4: all four exercises operate with approved geometry; event detection passes automated and human review.

### Phase 5 — Telemetry, scoring, and replay

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P5-01 | Finalize telemetry frame schema | Telemetry Engineer | P1-04, P3-11, P4-12 | Versioned fields, units, rate config, integrity flags |
| P5-02 | Finalize domain event schema | Telemetry Engineer | P4-12 | Versioned events and evidence fields |
| P5-03 | Harden local attempt store | Telemetry Engineer | P2-06, P5-01, P5-02 | Crash/restart recovery, integrity state, retention configuration |
| P5-04 | Implement upload queue | Telemetry Engineer | P5-03 | Idempotency keys, retries, backoff, visible queue state |
| P5-05 | Implement deterministic scoring engine | Scoring Engineer | P0-09, P5-02 | Versioned rules, unscorable state, rule-by-rule trace |
| P5-06 | Implement replay scorer | Scoring Engineer + Telemetry Engineer | P5-03, P5-05 | Stored attempt can be rescored under an explicitly chosen profile |
| P5-07 | Create golden replay test suite | Scoring Engineer | P5-06 | Same inputs/profile always produce exact same result |
| P5-08 | Validate scoring with instructors | CDL Instructor + Scoring Engineer | P5-07 | Practice profile accepted; evaluation profiles remain separately controlled |

Gate G5: every result is reproducible and explainable; corrupt/missing evidence never becomes a guessed score.

### Phase 6 — Backend platform

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P6-01 | Implement users, schools, roles, authentication | Backend Engineer | P1-06, P1-07 | Secure auth, server-side authorization, tests |
| P6-02 | Implement station registration/heartbeat | Backend Engineer | P6-01 | Station status and software/config versions tracked |
| P6-03 | Implement versioned configuration registry | Backend Engineer | P1-04, P6-01 | Draft/approved/retired lifecycle and approval audit fields |
| P6-04 | Implement exercise APIs | Backend Engineer | P4-01, P6-03 | Approved definitions retrievable by assigned station |
| P6-05 | Implement attempt lifecycle APIs | Backend Engineer | P2-02, P6-01 | Idempotent creation, completion, abort, and status transitions |
| P6-06 | Implement telemetry ingestion | Backend Engineer | P5-01 through P5-04, P6-05 | Chunk validation, idempotent upload, integrity reporting |
| P6-07 | Implement scores and rule-trace APIs | Backend Engineer | P5-05, P6-05 | Immutable versioned results accessible by role |
| P6-08 | Implement student history and notes APIs | Backend Engineer | P6-01, P6-05 | Paginated history and audited instructor notes |
| P6-09 | Integrate simulator backend client | Unreal Foundation Engineer + Backend Engineer | P6-02 through P6-08 | Auth/config sync, heartbeat, attempts, offline queue upload |
| P6-10 | Complete offline/sync fault testing | Independent QA Reviewer | P6-09 | Network loss, duplication, partial upload, restart cases verified |

Gate G6: a real simulator attempt survives offline operation, synchronizes exactly once, and is retrievable with correct authorization.

### Phase 7 — Student UX and instructor dashboard

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P7-01 | Implement launch/health/login screens | Unreal Foundation Engineer | P6-01, P6-02, P6-09 | Connected/offline/device/config state displayed accurately |
| P7-02 | Implement home and exercise selection | Unreal Foundation Engineer | P4-07 through P4-10, P6-04 | Only available/configured exercises shown |
| P7-03 | Implement pre-drive, training HUD, pause/reset | Unreal Foundation Engineer | P2-02, P3-08, P4-12 | Readable minimal UI; no hidden assistance |
| P7-04 | Implement results/history/detail screens | Unreal Foundation Engineer | P5-08, P6-07, P6-08 | Profile/version and validation status shown |
| P7-05 | Implement dashboard authentication/layout | Dashboard Engineer | P1-08, P6-01 | Role-safe desktop/tablet shell |
| P7-06 | Implement station overview/live attempt | Dashboard Engineer | P6-02, P6-05 | Fresh/stale/offline distinctions and event state |
| P7-07 | Implement students/history/detail/notes | Dashboard Engineer | P6-07, P6-08 | Instructor can review and annotate attempts |
| P7-08 | Implement common-mistakes reporting | Dashboard Engineer + Backend Engineer | P5-02, P6-08 | Transparent deterministic aggregation; no inferred missing events |
| P7-09 | Accessibility and usability review | Independent QA Reviewer + CDL Instructor | P7-01 through P7-08 | Blocking usability issues resolved for station and tablet workflows |

Gate G7: the complete student and instructor workflows work without debug tools and correctly communicate unvalidated/offline/error states.

### Phase 8 — Security, operations, and release engineering

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P8-01 | Approve privacy/retention/deletion policy | Project Owner | P5-01, P6-01 | Written policy before real student data |
| P8-02 | Threat model and security review | Independent QA Reviewer + Backend Engineer | P6, P7 | Risks, controls, and unresolved launch blockers documented |
| P8-03 | Production backend/database deployment | Backend Engineer | P8-01, P8-02 | TLS, secrets, backups, migrations, monitoring, rollback tested |
| P8-04 | Production dashboard deployment | Dashboard Engineer | P8-03 | TLS, environment config, error monitoring, rollback tested |
| P8-05 | Windows packaging/installer | Unreal Foundation Engineer | P7-04 | Versioned signed build/installer and clean-machine install test |
| P8-06 | Station provisioning/runbook | Lead Software Architect | P8-03 through P8-05 | Install, configure, calibrate, diagnose, update, rollback procedures |
| P8-07 | Backup/restore and disaster test | Backend Engineer + Independent QA Reviewer | P8-03 | Successful documented restore and data-integrity check |
| P8-08 | Release telemetry and support process | Lead Software Architect | P8-03 through P8-06 | Crash/log collection, severity definitions, owner/escalation path |

Gate G8: deploy, install, monitor, back up, restore, update, and roll back are proven—not assumed.

### Phase 9 — Independent QA, instructor validation, pilot

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P9-01 | Full requirements traceability audit | Independent QA Reviewer | G7, G8 | Every in-scope requirement has passing evidence or blocker |
| P9-02 | Regression/performance/stability testing | Independent QA Reviewer | P9-01 | Approved test duration/load criteria met; defects triaged |
| P9-03 | Final physics realism review | CDL Instructor + Physics Diagnostic Reviewer | G4, G5, P9-02 | Human checklist completed for target vehicle/configuration |
| P9-04 | Final scoring validation | CDL Instructor + Scoring Engineer | P9-03 | Practice scoring/profile version approved with evidence |
| P9-05 | Controlled student pilot | Project Owner + CDL Instructor | P9-04 | Consent/privacy in place; observed sessions and feedback recorded |
| P9-06 | Fix pilot blockers and rerun regression | Responsible engineers + QA | P9-05 | No open launch-blocking defects |
| P9-07 | Go/no-go review | Project Owner | P9-06 | Written decision against launch checklist |

Gate G9: qualified instructor approval, successful student pilot, no open launch blockers, operations ready.

### Phase 10 — MVP launch and stabilization

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P10-01 | Deploy tagged production release | Lead Software Architect | G9 | Immutable release tag, release notes, artifacts and config versions |
| P10-02 | Provision first school station | Unreal Foundation + Hardware/Input Engineers | P10-01 | Install, calibrate, connectivity and offline test passed |
| P10-03 | Train instructors/admins | Project Owner + CDL Instructor | P10-02 | Runbook walkthrough and support contacts acknowledged |
| P10-04 | Monitor stabilization period | QA + Backend + Unreal owners | P10-02 | Incidents triaged; fixes use controlled patch releases |
| P10-05 | Launch retrospective and backlog reset | Project Owner + Product Architect | P10-04 | Outcomes, metrics, defects, and next-release priorities recorded |

Gate G10: production MVP is operating at the pilot school with monitored support and controlled updates.

### Phase 11 — AI coaching after deterministic launch gates

| ID | Task | Primary owner | Depends on | Deliverable / done condition |
| --- | --- | --- | --- | --- |
| P11-01 | Define evidence-grounding contract | AI Instructor + Scoring + Telemetry Engineers | G5 | Coaching input contains only validated structured evidence |
| P11-02 | Implement post-attempt coaching service | AI Instructor Engineer | P11-01, P6 | Backend-only LLM integration with audit records |
| P11-03 | Implement historical comparison | AI Instructor Engineer | P11-02 | Claims trace to real attempts and comparable profiles |
| P11-04 | Implement concise live coaching | AI Instructor Engineer | P11-02, human approval | At most one supported actionable correction; kill switch present |
| P11-05 | Red-team invented claims and rule leakage | Independent QA Reviewer | P11-02 through P11-04 | Unsafe/unsupported outputs blocked or clearly declined |
| P11-06 | Human coaching validation and rollout | CDL Instructor + Project Owner | P11-05 | Controlled enablement; AI cannot affect physics/scoring |

AI coaching is not a dependency for initial deterministic MVP launch and may be delayed without blocking the core product.

## 5. Critical path

`SRS/architecture approval → measured ground truth → UE5 foundation → vehicle/trailer validation → exercise detection → telemetry/scoring replay → backend sync → user interfaces → operations/security → instructor validation → student pilot → production release`

Parallel work is allowed only where dependencies permit. Backend/dashboard shells can be built while physical measurement work proceeds. Validated physics, exercise behavior, or scoring cannot bypass human inputs.

## 6. Pull-request definition of done

Every code task must include:

- Linked task/requirement IDs
- Scope statement and explicit exclusions
- Tests for deterministic behavior
- Build/test commands and results
- Configuration and migration notes
- Failure cases and recovery behavior
- Screenshots/video for UI or visual changes
- Telemetry/log evidence for runtime changes
- Human-validation flag for any physical/training claim
- Documentation update
- Reviewer approval

## 7. Launch checklist

- [ ] SRS merged and approved
- [ ] Architecture approved
- [ ] Required ground truth approved
- [ ] Four exercises validated
- [ ] Hardware calibrated and disconnect-safe
- [ ] Telemetry integrity and replay verified
- [ ] Practice scoring approved and deterministic
- [ ] Offline completion and synchronization verified
- [ ] Backend/dashboard deployed with TLS, backup, restore, monitoring, and rollback
- [ ] Windows clean-machine install verified
- [ ] Privacy/retention policy approved
- [ ] Security review completed
- [ ] Instructor validation checklist signed
- [ ] Controlled student pilot completed
- [ ] No open launch-blocking defects
- [ ] Release tag, notes, artifacts, runbook, and support ownership ready

## 8. Current first actions

1. Resolve and merge draft SRS PR #1.
2. Record Project Owner approval or requested edits for the architecture document.
3. Select the actual training tractor, hardware, display, and first maneuver.
4. Schedule vehicle/trailer and yard measurements with a CDL instructor.
5. Begin Phase 1 repository scaffolding in parallel, using only synthetic test configurations clearly labeled non-authoritative.

The first executable coding ticket should be P1-01/P1-05 only after the architecture/repository structure is approved. The first validated maneuver remains blocked until human-approved ground truth is supplied.
