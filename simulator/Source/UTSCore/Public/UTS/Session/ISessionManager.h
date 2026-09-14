// ISessionManager — App/UI runtime state machine + attempt-lifecycle ownership.
//
// See docs/INTERFACE_CONTRACTS.md §1 for the full narrative contract,
// including the session-state vs. attempt-state distinction
// (docs/SRS_ARCHITECTURE_RECONCILIATION.md §3).
//
// P2-02 provides FSessionManager as a pure C++ implementation. The Unreal
// UUTSSessionSubsystem adapter is source-integrated; Windows verification is pending.

#pragma once

#include <string>
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
		Created = 0,
		AttemptReady = 1,
		Active = 2,
		Completed = 3,
		Aborted = 4,
		SynchronizationPending = 5,
		Synchronized = 6,
		Unknown = 7,
		Interrupted = 8
	};

	enum class ESessionError : uint8_t
	{
		None,
		AttemptAlreadyActive,     // single-active-attempt-per-station rule
		ConfigurationIncomplete,  // see accompanying FMissingFieldDiagnostic list
		ConfigurationVersionMismatch,
		BackendUnreachableAndOfflineNotApproved,
		ExplicitSyntheticIdentityRequired,
		AttemptNotFound,
		InvalidTransition,
		DuplicateAttemptId,
		RecoveryUnavailable,
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

	// P2-02 only admits development/synthetic identity. Real station/student
	// authentication remains a backend/auth task and must not be inferred from
	// a station id alone.
	struct FSyntheticSessionStartRequest
	{
		FStationId StationId;
		FStudentId SyntheticStudentId;
		std::string ClientSoftwareVersion;
	};

	struct FSessionStartResult
	{
		bool bSucceeded = false;
		FSessionId SessionId;
		ESessionError Error = ESessionError::None;
		std::string DiagnosticMessage;
	};

	struct FSessionCommandResult
	{
		bool bSucceeded = false;
		ESessionError Error = ESessionError::None;
		std::string DiagnosticMessage;
		std::vector<FMissingFieldDiagnostic> MissingFields;
	};

	// Every attempt's stable identifying context (FR-SES-004).
	struct FAttemptContext
	{
		FAttemptId        AttemptId;
		FSessionId        OwningSessionId;
		FStationId        StationId;
		FStudentId        StudentId;
		bool              bSyntheticPracticeOnly = false;
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
		std::string                      DiagnosticMessage;
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

		virtual bool IsBackendReachable() const = 0;

		virtual FSessionStartResult BeginSyntheticPracticeSession(
			const FSyntheticSessionStartRequest& Request) = 0;

		// Legacy draft signature retained for compatibility only. P2-02 has no
		// real auth/offline policy and therefore fails closed instead of
		// creating an implicit synthetic identity.
		virtual bool BeginSession(const FStationId& StationId) = 0;

		// Enforces single-active-attempt-per-station (MVP doc §7 rule) and the
		// Configuration gate (IConfigurationProvider::ValidateForLaunch).
		virtual FAttemptStartResult TryBeginAttempt(
			const FConfigVersionRef& ExerciseVersion,
			const FConfigVersionRef& VehicleProfileVersion,
			const FConfigVersionRef& TrailerProfileVersion,
			const FConfigVersionRef& ScoringProfileVersion) = 0;

		virtual EAttemptState GetAttemptState(const FAttemptId& AttemptId) const = 0;

		virtual FSessionCommandResult PauseAttempt(const FAttemptId& AttemptId) = 0;

		virtual FSessionCommandResult ResumeAttempt(const FAttemptId& AttemptId) = 0;

		// Always emits a domain event before mutating pose; never silently
		// discards telemetry already written (FR-SES-006).
		virtual FSessionCommandResult RequestReset(const FAttemptId& AttemptId, EResetPolicy Policy) = 0;

		virtual FSessionCommandResult CompleteOrAbort(const FAttemptId& AttemptId, EAttemptOutcome Outcome) = 0;

		// Never forces a transition away from Active; only affects
		// Offline/UploadQueued labeling.
		virtual FSessionCommandResult OnBackendConnectivityChanged(bool bReachable) = 0;

		// On relaunch after a crash: query Local Cache for an unfinalized
		// Active attempt and surface it as Interrupted rather than silently
		// resuming as if nothing happened.
		virtual bool TryRecoverInterruptedAttempt(FAttemptContext& OutContext) = 0;
	};
}
