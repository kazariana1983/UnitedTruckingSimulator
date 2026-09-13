// ISessionManager — App/UI runtime state machine + attempt-lifecycle ownership.
//
// See docs/INTERFACE_CONTRACTS.md §1 for the full narrative contract,
// including the session-state vs. attempt-state distinction
// (docs/SRS_ARCHITECTURE_RECONCILIATION.md §3).
//
// Architecture-phase artifact: pure interface, no implementation. The
// concrete implementation (UUTSSessionSubsystem) is Master Delivery Plan
// task P2-02, specified in docs/UE5_WINDOWS_SHELL_HANDOFF.md.

#pragma once

#include <vector>
#include "UTS/Common/UTSCommonTypes.h"

namespace UTS
{
	// Session/UI runtime state (MVP architecture doc §7). Distinct from the
	// attempt lifecycle below — one client has exactly one session state at
	// a time, independent of how many past attempts it has recorded.
	enum class ESessionState : uint8_t
	{
		Booting,
		Ready,
		Authenticated,
		Configuring,
		Loading,
		Active,
		Paused,
		Completing,
		Persisting,
		Results,
		// Exceptional states — orthogonal to the happy-path sequence above.
		Offline,
		Recovering,
		UploadQueued,
		FatalError
	};

	// Attempt-lifecycle state (FR-SES-005). One record per maneuver run;
	// many may exist historically while the session itself is in Results/Ready.
	enum class EAttemptState : uint8_t
	{
		Created,
		AttemptReady,
		Active,
		Completed,
		Aborted,
		SynchronizationPending,
		Synchronized
	};

	enum class ESessionError : uint8_t
	{
		None,
		AttemptAlreadyActive,     // single-active-attempt-per-station rule
		ConfigurationIncomplete,  // see accompanying FMissingFieldDiagnostic list
		BackendUnreachableAndOfflineNotApproved,
		Unknown
	};

	enum class EResetPolicy : uint8_t
	{
		// Exact semantics of "reset" against an evidence-bearing attempt are
		// TBD — see docs/ARCHITECTURE_OPEN_DECISIONS.md item A.9. Both members
		// are declared so the interface is ready either way; only an approved
		// policy selects which one the session manager is allowed to use for
		// a *validated* attempt.
		AbortAndKeepEvidence,
		ResetInPlaceAndKeepPriorSegment
	};

	enum class EAttemptOutcome : uint8_t
	{
		Completed,
		Aborted
	};

	// Every attempt's stable identifying context (FR-SES-004).
	struct FAttemptContext
	{
		FAttemptId        AttemptId;
		FStationId        StationId;
		FStudentId        StudentId;
		FConfigVersionRef ExerciseVersion;
		FConfigVersionRef VehicleProfileVersion;
		FConfigVersionRef TrailerProfileVersion;
		FConfigVersionRef ScoringProfileVersion;
		FSessionId        TelemetrySessionId;
		std::string       ClientSoftwareVersion;
	};

	struct FAttemptStartResult
	{
		bool                             bSucceeded = false;
		FAttemptContext                  Context;   // valid only if bSucceeded
		ESessionError                    Error = ESessionError::None;
		std::vector<FMissingFieldDiagnostic> MissingFields; // populated only for ConfigurationIncomplete
	};

	// Pure interface. Implementations must not call physics/exercise/scoring
	// code directly from here beyond invoking their own published interfaces
	// (NFR-002 "Required decoupling").
	class ISessionManager
	{
	public:
		virtual ~ISessionManager() = default;

		virtual ESessionState GetSessionState() const = 0;

		// Fails into Offline (not FatalError) when the backend is unreachable
		// but an approved offline policy exists.
		virtual bool BeginSession(const FStationId& StationId) = 0;

		// Enforces single-active-attempt-per-station (MVP doc §7 rule) and the
		// Configuration gate (IConfigurationProvider::ValidateForLaunch).
		virtual FAttemptStartResult TryBeginAttempt(
			const FConfigVersionRef& ExerciseVersion,
			const FConfigVersionRef& VehicleProfileVersion,
			const FConfigVersionRef& TrailerProfileVersion,
			const FConfigVersionRef& ScoringProfileVersion) = 0;

		virtual EAttemptState GetAttemptState(const FAttemptId& AttemptId) const = 0;

		// Always emits a domain event before mutating pose; never silently
		// discards telemetry already written (FR-SES-006).
		virtual void RequestReset(const FAttemptId& AttemptId, EResetPolicy Policy) = 0;

		virtual void CompleteOrAbort(const FAttemptId& AttemptId, EAttemptOutcome Outcome) = 0;

		// Never forces a transition away from Active; only affects
		// Offline/UploadQueued labeling.
		virtual void OnBackendConnectivityChanged(bool bReachable) = 0;

		// On relaunch after a crash: query Local Cache for an unfinalized
		// Active attempt and surface it as Interrupted rather than silently
		// resuming as if nothing happened.
		virtual bool TryRecoverInterruptedAttempt(FAttemptContext& OutContext) = 0;
	};
}
