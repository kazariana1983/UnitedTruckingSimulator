# P2-02 session lifecycle increment

Status: synthetic development implementation; not a validated training workflow or approval of PR #17/#18. This change builds on the configuration foundation in PR #18.

## Boundary

Session management owns who is using a station, which attempt is active, and the ordered lifecycle evidence for that attempt. It does not drive the vehicle, score a maneuver, authenticate a real student, write durable telemetry, or upload an attempt.

The only runnable path in this increment is explicitly synthetic. Validated/evaluation start remains blocked by the configuration foundation. No reset or offline-access policy is approved for real training by this change.

## Required behavior

| Operation | Required behavior |
| --- | --- |
| Begin synthetic session | Require explicit development identity; do not treat a station ID as real authentication. |
| Start attempt | Resolve configuration and verify exact requested versions before admitting one active attempt. Reject a second attempt without replacing the first. |
| Pause / resume | Accept only legal transitions for the current attempt; preserve its identity and configuration. |
| Reset | Record the requested reset policy and lifecycle evidence. Keep earlier evidence. No vehicle pose mutation is implemented here. |
| Complete / abort | Close the current attempt once and retain evidence. Completion means the caller ended an attempt; it is not a passed maneuver or a score. |
| Connectivity change | Track connectivity separately from lifecycle. Losing a connection must not replace Active with Offline. |
| Invalid operation | Return a diagnostic and preserve attempt state and lifecycle evidence. |
| Unknown attempt | Report unknown explicitly rather than returning a plausible existing state. |

## Review decisions

- The original `BeginSession(StationId)` signature is insufficient to establish real authentication. A development session must be explicitly labeled synthetic; real sign-in is deferred to the backend/authentication task.
- Offline/upload state is separate from the session and attempt lifecycles. Existing architecture enum values do not authorize implicitly changing an active attempt when connectivity changes.
- Reset is a lifecycle operation only. Real vehicle reset integration must record evidence before applying pose changes, and must use a human-approved policy for validated training.
- Configuration version zero is not an exact pin. P2-02 rejects missing or mismatched pins. Where the current provider validates the latest exercise by ID, the requested exercise must match the resolved version or the request is rejected.
- One active attempt is enforced within the station's manager instance. This is not a cross-process or distributed station lock.
- Lifecycle records in memory are development evidence. They are not crash-safe telemetry, successful disk persistence, synchronization, or recovery evidence. Those requirements remain pending.

## Verification limits

Implemented transitions are Booting → Ready after explicit synthetic session creation; Ready/Results → Active on attempt start; Active ↔ Paused; Active/Paused → Results on completion or abort. The attempt record remains Active while the session is Paused. Other architecture states are reserved, not simulated as completed work.

Only `ResetInPlaceAndKeepPriorSegment` records a reset request in this increment. `AbortAndKeepEvidence` is rejected with a diagnostic; callers can explicitly abort using `CompleteOrAbort`. No reset changes a vehicle pose. Invalid enum values, negative/non-finite timestamps, and backward event time are rejected. Connectivity callbacks return a command result so recording failures are observable and retryable.

Timestamps are elapsed seconds from session creation using the injected clock. IDs from the default sequential development generator are only unique for that generator instance; production station-wide identity is deferred. Call the manager on one owning thread and keep its injected configuration provider, clock, and ID generator alive for its lifetime. It is not a distributed lock or a thread-safe shared service.

The pure manager is implemented. A following source integration provides `UUTSSessionSubsystem` and development HUD/controller wiring; Windows build/runtime verification remains pending (see `UNREAL_SESSION_SHELL_TEST.md`). `TryRecoverInterruptedAttempt` returns false without changing state because no durable store is connected. School identity, real student authentication, disk persistence, and upload status remain later integrations.

Local verification: `bash simulator/scripts/run_headless_tests.sh` passes both configuration and session suites under C++17 with `-Wall -Wextra -Werror`. Tests include legal/illegal transitions, evidence retention, exact-version rejection, unknown/duplicate IDs, synthetic labels, timestamp failures, connectivity failures, and snapshot isolation.

Headless tests exercise the actual lifecycle implementation with synthetic profiles and deterministic test inputs. Unreal compilation, Windows execution, durable crash recovery, hardware, and gameplay require later verification. No acceptance gate is marked complete by this document.
