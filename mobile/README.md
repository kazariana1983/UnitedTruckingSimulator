# Backing Lab — mobile developer prototype

Executable 2D browser prototype for issue #15. This implements a subset of MOB-05–MOB-10 using synthetic fixtures. It is **not an APK, IPA, production mobile app, approved vehicle model, or CDL test**. No task requiring a physical device is marked complete.

Verification in the coding environment: six Node core tests pass; static build and JavaScript syntax checks pass; the local server serves the assets. UI smoke execution was blocked because no Chromium binary was installed and its download timed out/returned 502. No visual QA, actual multitouch test, native compilation, or physical-device test is claimed. The included browser test is pending execution.

## Run and build

Requires Node 22 or later. No npm dependencies are required for the core app.

```sh
cd mobile
npm test
npm run build
npm start
```

Open http://localhost:4173. Serve over localhost or HTTPS so secure-context browser APIs work. The self-contained build is in `dist/`; it makes no API, font, image, analytics or other network calls. Native offline packaging is not yet verified. Browser offline installation/service-worker support is not included.

## What works

- Landscape overhead yard and portrait fallback layout.
- Start, steer, accelerate, brake, forward/reverse, center steering, pause/resume, reset and end.
- Semantic input separated from toy articulated motion and canvas rendering.
- Brake precedence and direction changes while stopped only.
- Fixed simulation timestep; long frame gaps pause rather than fast-forward.
- Synthetic boundary-entry observations, no score or pass/fail.
- Input changes, periodic frames, configuration/model versions and final state recorded.
- Last ten ended attempts in local history; ID-based deduplication.
- Once-per-simulated-second recovery checkpoints, plus pause/focus-loss checkpoint.
- Restart recovery marks an interrupted attempt; up to the last checkpoint interval may be missing after a crash.
- Storage errors displayed rather than reported as saved; damaged history is not silently overwritten.
- Explicit destructive confirmation for history deletion and reset confirmation.
- Three-minute developer recording cap bounds memory. This is not an exercise time limit. End/reset when it is reached.

Keyboard: Tab between controls; range slider uses arrow keys; hold Space or Enter on a focused pedal. Touch: hold a pedal with one finger while using steering with another. Steering stays selected when released.

## Source map

| File | Responsibility |
| --- | --- |
| core.mjs | Synthetic fixture, motion, detector, session and local store |
| app.mjs | Touch/UI adaptation, drawing, lifecycle, persistence |
| index.html / style.css | Responsive screen and dialogs |
| test/core.test.mjs | Determinism, stop/brake, pause, events, persistence, failures |
| test/ui-smoke.cjs | Playwright browser interaction/recovery/layout checks |
| tools.mjs | Local server and repeatable static asset build |
| capacitor.config.json | Provisional native packaging configuration only |

Run the optional UI smoke test with a local server running, a Playwright installation and its Chromium binary available: `node test/ui-smoke.cjs`. If Playwright is installed outside this project, set `NODE_PATH` to its node_modules directory. A headless test is not an on-device acceptance test.

## Modeling and validation boundaries

Every number in FIXTURE is a synthetic developer parameter in arbitrary scene units. The fixture is deliberately not a 53-foot trailer specification. The toy model uses a rear-reference tractor heading and planar articulated follower. It omits tire dynamics, mass/load, coupling offsets, air brakes, gears/clutch, real steering ratios, collision response and vehicle-specific behavior. Drawn bodies are schematic; diagnostic boundary intersections refer to these shapes, not a validated vehicle swept path.

Do not promote this fixture into release defaults. Approved measurements and definitions must come from `ground_truth/SIMULATOR_GROUND_TRUTH.md` with owner approval. A reviewed model and detector must replace or validate this toy before representative training use. End is manual because completion criteria remain unknown.

No secret driver assistance, automatic path correction or LLM scoring is implemented. Synthetic boundary entries are rising-edge observations (counted again after leaving and reentering); their definitions are not official rules. Recorded inputs support deterministic toy-model regression tests, not proof of real-world fidelity.

## Stack decision: provisional experiment

| Option | Fit for this overhead prototype | Remaining evidence |
| --- | --- | --- |
| Canvas/JavaScript + potential Capacitor wrapper | Small dependency-free core; immediate browser testing; data contracts can align with Windows/backend | Native multitouch/lifecycle/storage and sustained device benchmark; SDK dependencies and costs |
| Godot 2D | Dedicated scene/editor/export workflow worth evaluating for richer simulation | Equivalent same-scene device test, team/toolchain fit, Android export environment |

Canvas is selected for this reversible developer spike only. The full MOB-02 engine comparison and production-platform choice remain open; no Godot benchmark has been performed. Android is the proposed first packaging target because it can be developed from multiple desktop platforms; actual supported phone/tablet selection is still required. iPhone/iPad build testing remains a separate gate.

Official references consulted:
- [Capacitor installation and platform creation](https://capacitorjs.com/docs/getting-started)
- [Capacitor environment setup](https://capacitorjs.com/docs/getting-started/environment-setup)
- [Godot Android export requirements](https://docs.godotengine.org/en/stable/tutorials/export/exporting_for_android.html)

The current environment has Node and Java but no configured Android SDK, adb or connected device. The configuration file does not create a native app by itself. Once the required build environment is available:

1. Select compatible pinned Capacitor core, CLI and platform versions using current official documentation; commit the dependency lockfile.
2. Install core/CLI and Android platform dependencies, run the web build, then `npx cap add android` and `npx cap sync android`.
3. Review the generated project and app lifecycle integration, replace the provisional app ID if needed, then build with the documented Android toolchain.
4. Test on actual hardware: multiple touches, interruptions, safe areas, storage limits, thermal behavior and offline launch.
5. Use the documented Apple toolchain/signing path for iOS separately. No signing identities or developer-account access are assumed.

Generated native projects are ignored until dependency and toolchain selection is reviewed. No store submission, publishing, signing or device approval has occurred.

## Remaining risks / next handoff

- Native lifecycle events must be integrated and verified; browser visibility handling alone is insufficient proof.
- Synchronous localStorage is suitable only for this bounded demo; measure and choose durable native storage before release.
- History capped at ten; it is not a production telemetry retention policy.
- There is no camera following or bounded-world recovery besides Reset; you can drive off screen.
- No completion detection, collision forces, production configuration loader, or approved scoring.
- Native builds, physical-device tests and the full comparative feasibility gate remain incomplete.
- Expand tests for multitouch cancellation, cap behavior, migration and supported real-device layouts.

The Windows project and ground truth are unchanged. Review this prototype as a testable first coding increment, then finish MOB-01/MOB-02 with actual device access and human review.
