# United Trucking Simulator — Standalone Mobile MVP

Status: Proposed implementation specification and task plan; no mobile code or device validation completed.
Scope authorized: a separate mobile backing-practice track alongside the Windows simulator.
Authority: ../ground_truth/SIMULATOR_GROUND_TRUTH.md

## Product and release boundary

Build an offline-capable phone/tablet application for practicing the relationship between steering input, tractor movement, and trailer position. The first release candidate has one training yard, one tractor-trailer configuration, and straight-line backing. Offset left, offset right, and alley docking follow after that complete loop is validated.

This is a conceptual backing-practice tool. It must not claim to qualify a driver, replace real-truck instruction, or reproduce steering/pedal/mirror skills. An overhead view is an explicit learning aid. Results from assisted mobile practice are not equivalent to a Windows station evaluation.

The Windows UE5 client, backend, instructor dashboard, existing work packages, and original four-exercise scope remain a separate track. This mobile plan does not declare the Windows architecture approved or change its ground truth.

## Initial scope

| Include in first release candidate | Defer |
| --- | --- |
| Landscape overhead view; simple readable vehicle shapes | Realistic cab interiors, rendered mirrors, optional 3D cab |
| Touch steering, accelerator, brake, forward/reverse selector | Physical wheel/pedal support and manual shifting |
| One tractor-trailer and one straight-line exercise | More vehicles and remaining three maneuvers |
| Start, pause, reset, end attempt, retry | Accounts, cloud synchronization, instructor dashboard |
| Observed-event feedback and local attempt summaries | Weighted scores until approved; AI coaching |
| Offline operation after installation | Traffic, roads, weather, multiplayer, ads, purchases |

No service account or server should be needed to complete the first mobile exercise. There is no monetization requirement in this plan.

## Screens and controls

1. Home: Start Practice, local history, controls/help.
2. Exercise briefing: objective, controls, configuration validation status, Start.
3. Practice: overhead yard with clear tractor/trailer orientation, visible boundaries, steering position indicator, touch controls, direction selector, pause.
4. Pause: Resume, Reset, End Attempt, Home. Reset ends the previous attempt as reset/aborted rather than silently deleting it.
5. Attempt summary: outcome (ended/completed/reset/interrupted), recorded observations, validation state, Retry.
6. History: local attempts, supported event counts, configuration versions, delete-local-history action.
7. Help: explain overhead-view assistance and the limits of the model.

Proposed control behavior for implementation tests:
- Permit simultaneous steering and accelerator/brake touches; handle each touch identity independently.
- Steering stays at the selected setting until changed or centered with an explicit control; show its position continuously.
- Releasing accelerator/brake clears the corresponding input. Do not automatically recenter steering invisibly.
- Brake takes precedence over accelerator in the input layer.
- Accept direction changes only while the model is stopped; document the configurable numerical tolerance as an engineering parameter, not a CDL rule.
- Loss of app focus, screen lock, or interrupted touches pauses the attempt and clears held inputs. Resume only after a deliberate tap.
- Do not derive road-wheel angles from screen position without an explicit, versioned steering mapping.

Control decisions above are proposed mobile UI behavior, not claims about truck mechanics. Review with users during the device prototype.

## Architecture and reuse

Keep the simulation state separate from rendering and touch input.

| Component | Responsibility | Reuse boundary |
| --- | --- | --- |
| Touch adapter | Convert touches into steering/throttle/brake/direction actions | Separate mobile code; align semantic action names with Windows |
| Simulation model | Compute low-speed tractor/trailer pose from config and controls | Mobile implementation requires its own validation |
| Exercise definitions | Starting pose, boundaries, target region and completion definition | Share versioned data where semantics match |
| Event detector | Emit timestamped boundary/contact/completion observations | Reuse contract and test cases; do not assume binary code reuse |
| Attempt recorder | Store IDs, events, versions, outcome and integrity flags locally | Align schema with future backend without depending on it |
| Feedback presenter | Explain supported observations | Never invent results or convert unavailable data to zero |
| Renderer | Draw yard, tractor, trailer and aids | Mobile-only presentation |
| Local storage | Versioned persistence, migration, deletion, recovery | Device-local; no automatic upload |
| Validation gate | Block validated practice when required config is missing | Same authority and approval concepts as Windows |

Use a consistent simulation timestep independent of display rendering; record model version, configuration versions and input/event ordering. Matching data schemas does not prove that the mobile and Windows physical behavior matches. Cross-platform replay tests apply to equivalent deterministic rules, not an assumption of identical physics engines.

The first model may use a simplified planar articulation approach if the feasibility test supports it. Treat this as a modeling proposal; document omissions and obtain instructor review. Do not invent tire, mass, steering, geometry, acceleration, friction or training-rule values.

## Ground truth and feedback

Required for representative straight-line practice:
- Identified tractor/trailer configuration and geometric reference points.
- Approved steering mapping and documented model assumptions.
- Yard/start/target geometry.
- Reviewed boundary/contact semantics and completion conditions.
- Human observations and tolerances suitable for validating the simplified model.

Unknown physical values stay unknown. Synthetic fixtures may exercise controls, storage, detection and automated tests in a clearly labeled developer demo. They must not silently become release defaults.

A recorded contact count is an observation under a detector definition, not an official penalty. Completion must not be inferred from an arbitrary target overlap. If completion criteria are missing, allow the developer to end the attempt manually; show "Ended", not "Passed". Do not display a percentage, pass/fail result, or official CDL claim without the appropriate approved rule profile.

Community submissions from issue #14 are unverified evidence until reviewed. Different trucks and tandem positions must remain separate configurations.

## Platform decision and feasibility test

The product direction is standalone mobile, including phone and tablet layouts. First OS, reference devices, minimum OS versions, and engine/framework remain explicit decisions to resolve before production implementation.

Do not automatically port UE5 or select a new engine solely because it supports mobile export. MOB-02 must compare a lightweight 2D approach with a mobile-capable game engine using the same small scene. Verify current official platform/toolchain documentation then and record:
- Installation and signed-build path for the candidate OS.
- Multitouch behavior and app-background recovery.
- Render and simulation timings on reference physical devices.
- Text/control readability, memory use and sustained thermal behavior.
- Local storage behavior across restart/update.
- Licensing/build-service costs, required development hardware, and maintainer familiarity.
- Engineering effort and reusable contracts.

Select one OS for the first complete test build; sequence the second after the first passes its gate. An on-device benchmark must precede numeric performance promises. Browser-only previews do not count as a standalone-device acceptance test.

## Delivery tasks

Owners below are accountable specialist roles, not hired people or already running agents. These tasks are planned, not executed.

| ID | Owner | Task and output | Dependency | Acceptance evidence |
| --- | --- | --- | --- | --- |
| MOB-01 | Product Architect + Owner | Freeze one-maneuver scope, choose first OS/reference phone/tablet, acceptance authority | None | Decision record; current Windows scope preserved |
| MOB-02 | Mobile Engineer | Run framework/device feasibility test and select implementation stack | MOB-01 | Installable sample on actual reference device; comparison and build instructions |
| MOB-03 | Architect | Specify modules, schemas, units, coordinates, input semantics and repository layout | MOB-02 | Reviewed contracts and traceability matrix |
| MOB-04 | Owner + CDL Instructor | Supply/review initial truck, trailer, yard, model and completion inputs | None; required before validated release | Versioned evidence and approved applicable values |
| MOB-05 | Mobile Engineer | Scaffold mobile application, repeatable build and CI | MOB-03 | Build artifact; installation test; checks pass |
| MOB-06 | Mobile UI Engineer | Home, briefing, practice screen and accessible multitouch controls | MOB-05 | Simultaneous touches and focus-loss cases verified on device |
| MOB-07 | Simulation Engineer | Implement simplified tractor/trailer model with diagnostic view | MOB-03, MOB-05; MOB-04 for representative validation | Units/config tests, model limitations, stable state updates |
| MOB-08 | Exercise Engineer | Load yard/start/target data and implement event detectors | MOB-07 | Boundary/contact edge-case tests; invalid geometry rejected |
| MOB-09 | Mobile Engineer | Implement attempt states, pause, reset, direction switching and recovery | MOB-06, MOB-07 | Repeated reset, screen lock, touch cancellation and resume verified |
| MOB-10 | Telemetry Engineer | Local attempt storage, schema versioning, summary/history and deletion | MOB-09 | Restart/migration/write-failure tests; no silent loss |
| MOB-11 | Exercise Engineer + Instructor | Finish straight-line exercise and evidence-based feedback | MOB-04, MOB-08, MOB-10 | Full attempt completed; approved definitions reflected accurately |
| MOB-12 | Independent QA | Review code and exercise loop | MOB-11 | Traceable tests, defects, replay checks for deterministic rules |
| MOB-13 | Instructor + UX Reviewer | Validate model limits, controls and comprehension | MOB-12 | Written findings on actual devices; no misleading skill-transfer claims |
| MOB-14 | Mobile Engineer | Resolve performance/readability/lifecycle defects | MOB-13 | Sustained on-device test against measured/approved budgets |
| MOB-15 | Release Engineer + Owner | Prepare distribution, privacy disclosures, assets, signing and rollback/support plan | MOB-14 | Current platform checklist verified; no production release implied |
| MOB-16 | Owner + Instructor + QA | Limited mobile pilot and corrective changes | MOB-15 | Observed sessions; documented feedback; blockers fixed and retested |
| MOB-17 | Owner + Release Engineer | Approve and submit/publish first release | MOB-16 | Actual signed release; submission tracked; acceptance not assumed |
| MOB-18 | Exercise Engineer + Instructor | Add offset left and right | MOB-17 plus approved layouts/rules | Independent tests and validation for each maneuver |
| MOB-19 | Exercise Engineer + Instructor | Add alley docking | MOB-18 plus approved layout/rules | Same completion and evidence standard |
| MOB-20 | Mobile Engineer + QA | Add second OS/device coverage | First OS loop validated; schedule after MOB-17 by default | Native device tests and distribution verification |
| MOB-21 | Product Architect | Evaluate optional cab view, accounts, cloud sync and dashboard integration | Core mobile release evidence | Separately scoped follow-up; no automatic scope expansion |

MOB-04 can proceed while technical planning and the non-authoritative developer sample proceed. No human input is needed merely to test a mock storage/input interface. Missing validated facts block representative practice and release claims, not all engineering work.

## First release acceptance checklist

- [ ] Installs and opens on supported reference devices.
- [ ] Runs the complete exercise without network access.
- [ ] Controls are readable, reachable and usable simultaneously.
- [ ] Pause/focus loss clears held inputs and prevents unintended resumed movement.
- [ ] Truck/trailer remain numerically stable for approved configurations.
- [ ] Input/render timing does not silently change deterministic event interpretation.
- [ ] Boundary, contact and completion feedback follows reviewed definitions.
- [ ] No invented scores or validated-physics claims.
- [ ] History survives restart; reset/interruption outcomes remain distinguishable.
- [ ] Storage faults produce an honest message and integrity state.
- [ ] Privacy/distribution disclosures match actual app behavior and dependencies.
- [ ] Instructor and small user pilot findings are resolved.
- [ ] Required signing, device tests, release artifacts and support plan exist.
- [ ] Owner approves release; store submission is distinct from store acceptance.

## Next coding handoff

Start with MOB-01 through MOB-03, followed by MOB-05/MOB-06: an installable shell with a landscape overhead scene, multitouch input indicators, pause/resume behavior and a test attempt. Use synthetic fixtures only, without presenting a valid vehicle or training result.

Deliver a branch/PR, reproducible build instructions, tests, actual device evidence where available, and an explicit list of untested assumptions. If no physical device/build toolchain is available, report that limit rather than marking the task done.

No release date or cost is committed. Estimate after MOB-02 using measured implementation effort, available devices, validation access, and the selected distribution path.
