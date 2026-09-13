# PR #17 review and first implementation boundary

Reviewed architecture head: `6b09e9c38c0ad5df032b56fff9b296e6e9a9375a`.
Scope: suitability for the first configuration/foundation change, not approval of the complete architecture or training product.

## Findings

1. **Synthetic approval conflict (blocks implementing the handoff literally).** `UE5_WINDOWS_SHELL_HANDOFF.md` section 3.1 requests an Approved-for-testing synthetic set, while section 5 forbids calling synthetic data approved. Configuration must distinguish synthetic use from human approval and reject synthetic data for validated launch. No shipped fixture is human approved. Implement this boundary in P2-01.
2. **Configuration payload and lookup gaps (must be explicit in P2-01).** `FConfigLoadResult` carries only a provenance envelope; no payload or typed snapshot can be consumed through the interface. `GetApprovalState` cannot distinguish an unknown version from a known Draft. Introduce an explicit immutable payload/snapshot access path and document any conservative unknown-state behavior. Do not silently use zeros or latest versions when an exact version was requested.
3. **Unavailable value is not enforced (resolve before physics use).** `TApprovedValue::Get()` returns a default-initialized value even when `HasValue()` is false. The documentation claims stronger enforcement than this implementation provides. Use checked access or an explicit optional/result before vehicle implementations consume it. This is not a blocker to a configuration-only change that does not consume this type.
4. **Session contract needs another pass before P2-02.** The prose accepts an authentication result but the header's `BeginSession` accepts only StationId. Interrupted recovery has no corresponding attempt enum member; no explicit pause/resume command exists despite declared states. Offline/upload status is called orthogonal but is encoded in the same enum as Active. Define the transition table and independent connectivity status before implementing the session state machine. Do not invent authentication or recovery policies in P2-01.
5. **Scoring snapshot is incomplete for pure replay (before P2-07).** `FScoringProfileSnapshot` contains provenance/mode but no rules payload. A scorer could not evaluate arbitrary versioned rules solely from these inputs. Add an immutable rule representation and canonical serialization before claiming deterministic replay. Byte equality must refer to serialized output, not raw C++ object memory.
6. **Requirement count in the PR description is inaccurate (documentation only).** Comparing unique `FR-*` and `NFR-*` identifiers in SRS PR #1 at `a7824937725491cd8cd3cadeaf4bd7f0c9331a57` with the traceability matrix finds 68 in each, with no missing or extra IDs. The PR description says 53. Identifier coverage is present; this does not establish that the mapped interfaces satisfy the requirements.

## Verification

All ten original public interface headers compiled independently with the local C++ compiler using `-std=c++17 -Wall -Wextra -Werror -fsyntax-only`. This checks header syntax and include completeness only. It does not verify Unreal Header Tool, Unreal Build Tool, Windows linking, runtime behavior, or architectural requirements.

## Authorized next increment

The owner's request to continue authorizes reversible scaffolding from this draft architecture. Implement P2-01 configuration and the project/module files needed to host it. Follow the handoff's one-task-per-PR boundary. Keep later session/input/exercise/telemetry/scoring work pending. Stack the implementation PR on PR #17 so its additions can be reviewed independently; merge neither PR as part of this work.

Ground-truth measurements, scoring rules, hardware choices, and all human validation gates remain unresolved. The next implementation must be useful with missing data and must report that validated training is unavailable.
