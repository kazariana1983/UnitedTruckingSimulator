#include "UTS/Session/SessionManager.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace UTS
{
	namespace
	{
		bool SameRef(const FConfigVersionRef& Left, const FConfigVersionRef& Right)
		{
			return Left.ProfileId == Right.ProfileId && Left.Version == Right.Version;
		}

		std::string DescribeRef(EProfileType Type, const FConfigVersionRef& Ref)
		{
			return ToString(Type) + "[" + Ref.ProfileId.Value + ":v" + std::to_string(Ref.Version) + "]";
		}

		std::string DescribeAttemptRef(const FAttemptId& AttemptId)
		{
			return AttemptId.IsValid() ? AttemptId.Value : "<empty-attempt-id>";
		}

		bool IsExactPinnedRef(const FConfigVersionRef& Ref)
		{
			return Ref.ProfileId.IsValid() && Ref.Version != 0;
		}

		void AddMissingPinDiagnostic(
			std::vector<FMissingFieldDiagnostic>& OutDiagnostics,
			const std::string& FieldPath,
			const FConfigVersionRef& Ref)
		{
			std::string Reason = "Attempt start requires an exact nonzero configuration version pin";
			if (!Ref.ProfileId.IsValid())
			{
				Reason += "; profile id is empty";
			}
			if (Ref.Version == 0)
			{
				Reason += "; version 0 means latest and is not an exact attempt pin";
			}

			OutDiagnostics.push_back({FieldPath, Reason});
		}

		bool ResolvedContains(
			const std::vector<FConfigSnapshotEnvelope>& ResolvedProfiles,
			EProfileType Type,
			const FConfigVersionRef& Ref)
		{
			return std::any_of(
				ResolvedProfiles.begin(),
				ResolvedProfiles.end(),
				[Type, &Ref](const FConfigSnapshotEnvelope& Envelope) {
					return Envelope.ProfileType == Type && SameRef(Envelope.Provenance.VersionRef, Ref);
				});
		}

		bool IsKnownResetPolicy(EResetPolicy Policy)
		{
			switch (Policy)
			{
			case EResetPolicy::AbortAndKeepEvidence:
			case EResetPolicy::ResetInPlaceAndKeepPriorSegment:
				return true;
			}
			return false;
		}

		bool IsKnownAttemptOutcome(EAttemptOutcome Outcome)
		{
			switch (Outcome)
			{
			case EAttemptOutcome::Completed:
			case EAttemptOutcome::Aborted:
				return true;
			}
			return false;
		}

		const FFileConfigurationProvider& RequireConfigurationProvider(const FSessionManagerDependencies& Dependencies)
		{
			if (Dependencies.ConfigurationProvider == nullptr)
			{
				throw std::invalid_argument("FSessionManager requires a configuration provider");
			}
			return *Dependencies.ConfigurationProvider;
		}

		ISessionClock& RequireClock(const FSessionManagerDependencies& Dependencies)
		{
			if (Dependencies.Clock == nullptr)
			{
				throw std::invalid_argument("FSessionManager requires a clock");
			}
			return *Dependencies.Clock;
		}

		ISessionIdGenerator& RequireIdGenerator(const FSessionManagerDependencies& Dependencies)
		{
			if (Dependencies.IdGenerator == nullptr)
			{
				throw std::invalid_argument("FSessionManager requires an id generator");
			}
			return *Dependencies.IdGenerator;
		}
	}

	FSteadySessionClock::FSteadySessionClock()
		: StartTime(std::chrono::steady_clock::now())
	{
	}

	FMonotonicTimestamp FSteadySessionClock::NowSeconds() const
	{
		return std::chrono::duration<double>(std::chrono::steady_clock::now() - StartTime).count();
	}

	FSequentialSessionIdGenerator::FSequentialSessionIdGenerator(std::string InPrefix)
		: Prefix(std::move(InPrefix))
	{
		if (Prefix.empty())
		{
			Prefix = "uts-dev";
		}
	}

	FSessionId FSequentialSessionIdGenerator::CreateSessionId()
	{
		return {Prefix + ".session." + std::to_string(NextSessionOrdinal++)};
	}

	FAttemptId FSequentialSessionIdGenerator::CreateAttemptId()
	{
		return {Prefix + ".attempt." + std::to_string(NextAttemptOrdinal++)};
	}

	FSessionId FSequentialSessionIdGenerator::CreateTelemetrySessionId(const FAttemptId& AttemptId)
	{
		const std::string AttemptPart = AttemptId.IsValid() ? AttemptId.Value : "unknown-attempt";
		return {Prefix + ".telemetry." + AttemptPart + "." + std::to_string(NextTelemetryOrdinal++)};
	}

	std::string ToString(ESessionState State)
	{
		switch (State)
		{
		case ESessionState::Booting: return "Booting";
		case ESessionState::Ready: return "Ready";
		case ESessionState::Authenticated: return "Authenticated";
		case ESessionState::Configuring: return "Configuring";
		case ESessionState::Loading: return "Loading";
		case ESessionState::Active: return "Active";
		case ESessionState::Paused: return "Paused";
		case ESessionState::Completing: return "Completing";
		case ESessionState::Persisting: return "Persisting";
		case ESessionState::Results: return "Results";
		case ESessionState::Offline: return "Offline";
		case ESessionState::Recovering: return "Recovering";
		case ESessionState::UploadQueued: return "UploadQueued";
		case ESessionState::FatalError: return "FatalError";
		}
		return "UnknownSessionState";
	}

	std::string ToString(EAttemptState State)
	{
		switch (State)
		{
		case EAttemptState::Created: return "Created";
		case EAttemptState::AttemptReady: return "AttemptReady";
		case EAttemptState::Active: return "Active";
		case EAttemptState::Completed: return "Completed";
		case EAttemptState::Aborted: return "Aborted";
		case EAttemptState::SynchronizationPending: return "SynchronizationPending";
		case EAttemptState::Synchronized: return "Synchronized";
		case EAttemptState::Unknown: return "Unknown";
		case EAttemptState::Interrupted: return "Interrupted";
		}
		return "UnknownAttemptState";
	}

	std::string ToString(ESessionError Error)
	{
		switch (Error)
		{
		case ESessionError::None: return "None";
		case ESessionError::AttemptAlreadyActive: return "AttemptAlreadyActive";
		case ESessionError::ConfigurationIncomplete: return "ConfigurationIncomplete";
		case ESessionError::ConfigurationVersionMismatch: return "ConfigurationVersionMismatch";
		case ESessionError::BackendUnreachableAndOfflineNotApproved: return "BackendUnreachableAndOfflineNotApproved";
		case ESessionError::ExplicitSyntheticIdentityRequired: return "ExplicitSyntheticIdentityRequired";
		case ESessionError::AttemptNotFound: return "AttemptNotFound";
		case ESessionError::InvalidTransition: return "InvalidTransition";
		case ESessionError::DuplicateAttemptId: return "DuplicateAttemptId";
		case ESessionError::RecoveryUnavailable: return "RecoveryUnavailable";
		case ESessionError::Unknown: return "Unknown";
		}
		return "UnknownSessionError";
	}

	std::string ToString(EDomainEventType EventType)
	{
		switch (EventType)
		{
		case EDomainEventType::AttemptStarted: return "AttemptStarted";
		case EDomainEventType::AttemptPaused: return "AttemptPaused";
		case EDomainEventType::AttemptResumed: return "AttemptResumed";
		case EDomainEventType::AttemptReset: return "AttemptReset";
		case EDomainEventType::AttemptCompleted: return "AttemptCompleted";
		case EDomainEventType::AttemptAborted: return "AttemptAborted";
		case EDomainEventType::VehicleMovementStarted: return "VehicleMovementStarted";
		case EDomainEventType::VehicleMovementStopped: return "VehicleMovementStopped";
		case EDomainEventType::BoundaryEnter: return "BoundaryEnter";
		case EDomainEventType::BoundaryExit: return "BoundaryExit";
		case EDomainEventType::ConeContact: return "ConeContact";
		case EDomainEventType::PullUpCandidate: return "PullUpCandidate";
		case EDomainEventType::PullUpConfirmed: return "PullUpConfirmed";
		case EDomainEventType::CompletionCandidate: return "CompletionCandidate";
		case EDomainEventType::CompletionConfirmed: return "CompletionConfirmed";
		case EDomainEventType::DeviceDisconnected: return "DeviceDisconnected";
		case EDomainEventType::DeviceReconnected: return "DeviceReconnected";
		case EDomainEventType::BackendConnectionLost: return "BackendConnectionLost";
		case EDomainEventType::BackendConnectionRestored: return "BackendConnectionRestored";
		case EDomainEventType::TelemetryBufferStateChanged: return "TelemetryBufferStateChanged";
		}
		return "UnknownDomainEventType";
	}

	std::string ToString(EResetPolicy Policy)
	{
		switch (Policy)
		{
		case EResetPolicy::AbortAndKeepEvidence: return "AbortAndKeepEvidence";
		case EResetPolicy::ResetInPlaceAndKeepPriorSegment: return "ResetInPlaceAndKeepPriorSegment";
		}
		return "UnknownResetPolicy";
	}

	FSessionManager::FSessionManager(FSessionManagerDependencies InDependencies)
		: ConfigurationProvider(RequireConfigurationProvider(InDependencies))
		, Clock(RequireClock(InDependencies))
		, IdGenerator(RequireIdGenerator(InDependencies))
	{
	}

	ESessionState FSessionManager::GetSessionState() const
	{
		return SessionState;
	}

	bool FSessionManager::IsBackendReachable() const
	{
		return bBackendReachable;
	}

	FSessionStartResult FSessionManager::BeginSyntheticPracticeSession(
		const FSyntheticSessionStartRequest& Request)
	{
		if (ActiveAttemptId.has_value())
		{
			return RejectSessionStart(
				ESessionError::InvalidTransition,
				"Cannot begin a new synthetic session while an attempt is active");
		}

		if (SessionState != ESessionState::Booting)
		{
			return RejectSessionStart(
				ESessionError::InvalidTransition,
				"BeginSyntheticPracticeSession is only valid before a session has started; current state is " +
					ToString(SessionState));
		}

		if (!Request.StationId.IsValid() ||
			!Request.SyntheticStudentId.IsValid() ||
			Request.ClientSoftwareVersion.empty())
		{
			return RejectSessionStart(
				ESessionError::ExplicitSyntheticIdentityRequired,
				"Synthetic practice session requires station id, synthetic student id, and client software version");
		}

		const double ClockOrigin = Clock.NowSeconds();
		if (!std::isfinite(ClockOrigin) || ClockOrigin < 0.0)
			return RejectSessionStart(ESessionError::InvalidTransition, "Session clock is invalid");
		FSessionId NewSessionId = IdGenerator.CreateSessionId();
		if (!NewSessionId.IsValid())
		{
			return RejectSessionStart(
				ESessionError::Unknown,
				"Session id generator returned an empty session id");
		}

		CurrentStationId = Request.StationId;
		CurrentSyntheticStudentId = Request.SyntheticStudentId;
		CurrentSessionId = NewSessionId;
		ClientSoftwareVersion = Request.ClientSoftwareVersion;
		SessionClockOrigin = ClockOrigin;
		SessionState = ESessionState::Ready;

		FSessionStartResult Result;
		Result.bSucceeded = true;
		Result.SessionId = NewSessionId;
		return Result;
	}

	bool FSessionManager::BeginSession(const FStationId& StationId)
	{
		(void)StationId;
		return false;
	}

	FAttemptStartResult FSessionManager::TryBeginAttempt(
		const FConfigVersionRef& ExerciseVersion,
		const FConfigVersionRef& VehicleProfileVersion,
		const FConfigVersionRef& TrailerProfileVersion,
		const FConfigVersionRef& ScoringProfileVersion)
	{
		return TryBeginAttemptWithMode(
			{ExerciseVersion, VehicleProfileVersion, TrailerProfileVersion, ScoringProfileVersion},
			ESessionAttemptMode::Validated);
	}

	FAttemptStartResult FSessionManager::TryBeginSyntheticPracticeAttempt(
		const FConfigVersionRef& ExerciseVersion,
		const FConfigVersionRef& VehicleProfileVersion,
		const FConfigVersionRef& TrailerProfileVersion,
		const FConfigVersionRef& ScoringProfileVersion)
	{
		return TryBeginAttemptWithMode(
			{ExerciseVersion, VehicleProfileVersion, TrailerProfileVersion, ScoringProfileVersion},
			ESessionAttemptMode::SyntheticPracticeOnly);
	}

	FAttemptStartResult FSessionManager::TryBeginAttemptWithMode(
		const FAttemptConfigRefs& ConfigRefs,
		ESessionAttemptMode Mode)
	{
		if (!CurrentStationId.has_value() ||
			!CurrentSyntheticStudentId.has_value() ||
			!CurrentSessionId.has_value() ||
			ClientSoftwareVersion.empty())
		{
			return RejectAttemptStart(
				ESessionError::ExplicitSyntheticIdentityRequired,
				"BeginSyntheticPracticeSession must succeed before attempts can start in P2-02");
		}

		if (ActiveAttemptId.has_value())
		{
			return RejectAttemptStart(
				ESessionError::AttemptAlreadyActive,
				"Only one attempt may be active within this station session manager");
		}

		if (SessionState != ESessionState::Ready && SessionState != ESessionState::Results)
		{
			return RejectAttemptStart(
				ESessionError::InvalidTransition,
				"Attempt start is not legal from session state " + ToString(SessionState));
		}

		FAttemptStartResult ConfigResult = ValidateAttemptConfig(ConfigRefs, Mode);
		if (!ConfigResult.bSucceeded)
		{
			return ConfigResult;
		}

		FAttemptId NewAttemptId = IdGenerator.CreateAttemptId();
		if (!NewAttemptId.IsValid())
		{
			return RejectAttemptStart(
				ESessionError::Unknown,
				"Attempt id generator returned an empty attempt id");
		}

		if (HasAttemptId(NewAttemptId))
		{
			return RejectAttemptStart(
				ESessionError::DuplicateAttemptId,
				"Attempt id generator returned an id that already exists in this manager: " + NewAttemptId.Value);
		}

		FSessionId TelemetrySessionId = IdGenerator.CreateTelemetrySessionId(NewAttemptId);
		if (!TelemetrySessionId.IsValid())
		{
			return RejectAttemptStart(
				ESessionError::Unknown,
				"Telemetry session id generator returned an empty id");
		}

		FAttemptRecord Record;
		Record.Context.AttemptId = NewAttemptId;
		Record.Context.OwningSessionId = *CurrentSessionId;
		Record.Context.StationId = *CurrentStationId;
		Record.Context.StudentId = *CurrentSyntheticStudentId;
		Record.Context.bSyntheticPracticeOnly = true;
		Record.Context.ExerciseVersion = ConfigRefs.ExerciseVersion;
		Record.Context.VehicleProfileVersion = ConfigRefs.VehicleProfileVersion;
		Record.Context.TrailerProfileVersion = ConfigRefs.TrailerProfileVersion;
		Record.Context.ScoringProfileVersion = ConfigRefs.ScoringProfileVersion;
		Record.Context.TelemetrySessionId = TelemetrySessionId;
		Record.Context.ClientSoftwareVersion = ClientSoftwareVersion;
		Record.State = EAttemptState::Active;

		std::string EventError;
		std::optional<FDomainEvent> StartEvent = MakeLifecycleEvent(
			Record,
			EDomainEventType::AttemptStarted,
			"UTS.SessionManager.AttemptStarted",
			EventError);
		if (!StartEvent.has_value())
		{
			return RejectAttemptStart(ESessionError::InvalidTransition, EventError);
		}
		Record.LifecycleEvents.push_back(*StartEvent);

		Attempts.push_back(Record);
		ActiveAttemptId = NewAttemptId;
		SessionState = ESessionState::Active;

		FAttemptStartResult Result;
		Result.bSucceeded = true;
		Result.Context = Record.Context;
		return Result;
	}

	EAttemptState FSessionManager::GetAttemptState(const FAttemptId& AttemptId) const
	{
		const FAttemptRecord* Record = FindAttempt(AttemptId);
		return Record == nullptr ? EAttemptState::Unknown : Record->State;
	}

	FSessionCommandResult FSessionManager::PauseAttempt(const FAttemptId& AttemptId)
	{
		FAttemptRecord* Record = FindAttempt(AttemptId);
		if (Record == nullptr)
		{
			return RejectCommand(
				ESessionError::AttemptNotFound,
				"Cannot pause unknown attempt " + DescribeAttemptRef(AttemptId));
		}

		if (!IsCurrentActiveAttempt(AttemptId) ||
			Record->State != EAttemptState::Active ||
			SessionState != ESessionState::Active)
		{
			return RejectCommand(
				ESessionError::InvalidTransition,
				"Pause requires the current active attempt while the session is Active; attempt state is " +
					ToString(Record->State) + ", session state is " + ToString(SessionState));
		}

		std::string EventError;
		std::optional<FDomainEvent> PauseEvent = MakeLifecycleEvent(
			*Record,
			EDomainEventType::AttemptPaused,
			"UTS.SessionManager.AttemptPaused",
			EventError);
		if (!PauseEvent.has_value())
		{
			return RejectCommand(ESessionError::InvalidTransition, EventError);
		}

		Record->LifecycleEvents.push_back(*PauseEvent);
		SessionState = ESessionState::Paused;

		FSessionCommandResult Result;
		Result.bSucceeded = true;
		return Result;
	}

	FSessionCommandResult FSessionManager::ResumeAttempt(const FAttemptId& AttemptId)
	{
		FAttemptRecord* Record = FindAttempt(AttemptId);
		if (Record == nullptr)
		{
			return RejectCommand(
				ESessionError::AttemptNotFound,
				"Cannot resume unknown attempt " + DescribeAttemptRef(AttemptId));
		}

		if (!IsCurrentActiveAttempt(AttemptId) ||
			Record->State != EAttemptState::Active ||
			SessionState != ESessionState::Paused)
		{
			return RejectCommand(
				ESessionError::InvalidTransition,
				"Resume requires the current active attempt while the session is Paused; attempt state is " +
					ToString(Record->State) + ", session state is " + ToString(SessionState));
		}

		std::string EventError;
		std::optional<FDomainEvent> ResumeEvent = MakeLifecycleEvent(
			*Record,
			EDomainEventType::AttemptResumed,
			"UTS.SessionManager.AttemptResumed",
			EventError);
		if (!ResumeEvent.has_value())
		{
			return RejectCommand(ESessionError::InvalidTransition, EventError);
		}

		Record->LifecycleEvents.push_back(*ResumeEvent);
		SessionState = ESessionState::Active;

		FSessionCommandResult Result;
		Result.bSucceeded = true;
		return Result;
	}

	FSessionCommandResult FSessionManager::RequestReset(const FAttemptId& AttemptId, EResetPolicy Policy)
	{
		FAttemptRecord* Record = FindAttempt(AttemptId);
		if (Record == nullptr)
		{
			return RejectCommand(
				ESessionError::AttemptNotFound,
				"Cannot reset unknown attempt " + DescribeAttemptRef(AttemptId));
		}

		if (!IsCurrentActiveAttempt(AttemptId) ||
			Record->State != EAttemptState::Active ||
			(SessionState != ESessionState::Active && SessionState != ESessionState::Paused))
		{
			return RejectCommand(
				ESessionError::InvalidTransition,
				"Reset requires the current active attempt while the session is Active or Paused; attempt state is " +
					ToString(Record->State) + ", session state is " + ToString(SessionState));
		}

		if (!IsKnownResetPolicy(Policy))
		{
			return RejectCommand(
				ESessionError::InvalidTransition,
				"Reset rejected because the reset policy enum value is not recognized");
		}

		if (Policy == EResetPolicy::AbortAndKeepEvidence)
		{
			return RejectCommand(
				ESessionError::InvalidTransition,
				"AbortAndKeepEvidence reset is deferred until exercise/vehicle reset integration exists; call CompleteOrAbort to abort explicitly");
		}

		std::string EventError;
		std::optional<FDomainEvent> ResetEvent = MakeLifecycleEvent(
			*Record,
			EDomainEventType::AttemptReset,
			"UTS.SessionManager.AttemptReset." + ToString(Policy),
			EventError);
		if (!ResetEvent.has_value())
		{
			return RejectCommand(ESessionError::InvalidTransition, EventError);
		}

		Record->LifecycleEvents.push_back(*ResetEvent);
		Record->bHasResetPolicy = true;
		Record->LastResetPolicy = Policy;

		FSessionCommandResult Result;
		Result.bSucceeded = true;
		return Result;
	}

	FSessionCommandResult FSessionManager::CompleteOrAbort(const FAttemptId& AttemptId, EAttemptOutcome Outcome)
	{
		FAttemptRecord* Record = FindAttempt(AttemptId);
		if (Record == nullptr)
		{
			return RejectCommand(
				ESessionError::AttemptNotFound,
				"Cannot complete or abort unknown attempt " + DescribeAttemptRef(AttemptId));
		}

		if (!IsCurrentActiveAttempt(AttemptId) ||
			Record->State != EAttemptState::Active ||
			(SessionState != ESessionState::Active && SessionState != ESessionState::Paused))
		{
			return RejectCommand(
				ESessionError::InvalidTransition,
				"CompleteOrAbort requires the current active attempt while the session is Active or Paused; attempt state is " +
					ToString(Record->State) + ", session state is " + ToString(SessionState));
		}

		if (!IsKnownAttemptOutcome(Outcome))
		{
			return RejectCommand(
				ESessionError::InvalidTransition,
				"CompleteOrAbort rejected because the attempt outcome enum value is not recognized");
		}

		const bool bCompleted = Outcome == EAttemptOutcome::Completed;
		std::string EventError;
		std::optional<FDomainEvent> FinishEvent = MakeLifecycleEvent(
			*Record,
			bCompleted ? EDomainEventType::AttemptCompleted : EDomainEventType::AttemptAborted,
			bCompleted ? "UTS.SessionManager.AttemptCompleted" : "UTS.SessionManager.AttemptAborted",
			EventError);
		if (!FinishEvent.has_value())
		{
			return RejectCommand(ESessionError::InvalidTransition, EventError);
		}

		Record->LifecycleEvents.push_back(*FinishEvent);
		Record->State = bCompleted ? EAttemptState::Completed : EAttemptState::Aborted;
		ActiveAttemptId.reset();
		SessionState = ESessionState::Results;

		FSessionCommandResult Result;
		Result.bSucceeded = true;
		return Result;
	}

	FSessionCommandResult FSessionManager::OnBackendConnectivityChanged(bool bReachable)
	{
		FSessionCommandResult Result;
		Result.bSucceeded = true;
		if (bBackendReachable == bReachable) return Result;
		if (ActiveAttemptId.has_value())
		{
			FAttemptRecord* Record = FindAttempt(*ActiveAttemptId);
			if (!Record) return RejectCommand(ESessionError::AttemptNotFound, "Active attempt record is missing");
			std::string EventError;
			auto Event = MakeLifecycleEvent(*Record,
				bReachable ? EDomainEventType::BackendConnectionRestored : EDomainEventType::BackendConnectionLost,
				bReachable ? "UTS.SessionManager.BackendConnectionRestored" : "UTS.SessionManager.BackendConnectionLost",
				EventError);
			if (!Event) return RejectCommand(ESessionError::InvalidTransition, EventError);
			Record->LifecycleEvents.push_back(*Event);
		}
		bBackendReachable = bReachable;
		return Result;
	}

	bool FSessionManager::TryRecoverInterruptedAttempt(FAttemptContext& OutContext)
	{
		(void)OutContext;
		return false;
	}

	std::vector<FDomainEvent> FSessionManager::GetAttemptEvents(const FAttemptId& AttemptId) const
	{
		const FAttemptRecord* Record = FindAttempt(AttemptId);
		return Record == nullptr ? std::vector<FDomainEvent>{} : Record->LifecycleEvents;
	}

	std::optional<FAttemptRecordSnapshot> FSessionManager::GetAttemptRecord(const FAttemptId& AttemptId) const
	{
		const FAttemptRecord* Record = FindAttempt(AttemptId);
		if (Record == nullptr)
		{
			return std::nullopt;
		}
		return SnapshotAttempt(*Record);
	}

	std::size_t FSessionManager::GetAttemptCount() const
	{
		return Attempts.size();
	}

	std::optional<FAttemptId> FSessionManager::GetActiveAttemptId() const
	{
		return ActiveAttemptId;
	}

	std::optional<FSessionId> FSessionManager::GetCurrentSessionId() const
	{
		return CurrentSessionId;
	}

	FSessionStartResult FSessionManager::RejectSessionStart(
		ESessionError Error,
		const std::string& Message) const
	{
		FSessionStartResult Result;
		Result.Error = Error;
		Result.DiagnosticMessage = Message;
		return Result;
	}

	FAttemptStartResult FSessionManager::RejectAttemptStart(
		ESessionError Error,
		const std::string& Message,
		std::vector<FMissingFieldDiagnostic> MissingFields) const
	{
		FAttemptStartResult Result;
		Result.Error = Error;
		Result.DiagnosticMessage = Message;
		Result.MissingFields = std::move(MissingFields);
		return Result;
	}

	FSessionCommandResult FSessionManager::RejectCommand(
		ESessionError Error,
		const std::string& Message,
		std::vector<FMissingFieldDiagnostic> MissingFields) const
	{
		FSessionCommandResult Result;
		Result.Error = Error;
		Result.DiagnosticMessage = Message;
		Result.MissingFields = std::move(MissingFields);
		return Result;
	}

	FAttemptStartResult FSessionManager::ValidateAttemptConfig(
		const FAttemptConfigRefs& ConfigRefs,
		ESessionAttemptMode Mode) const
	{
		if (Mode != ESessionAttemptMode::Validated && Mode != ESessionAttemptMode::SyntheticPracticeOnly)
			return RejectAttemptStart(ESessionError::InvalidTransition, "Unknown attempt mode");
		std::vector<FMissingFieldDiagnostic> PinDiagnostics;
		if (!IsExactPinnedRef(ConfigRefs.ExerciseVersion))
		{
			AddMissingPinDiagnostic(PinDiagnostics, "AttemptConfig.ExerciseVersion", ConfigRefs.ExerciseVersion);
		}
		if (!IsExactPinnedRef(ConfigRefs.VehicleProfileVersion))
		{
			AddMissingPinDiagnostic(PinDiagnostics, "AttemptConfig.VehicleProfileVersion", ConfigRefs.VehicleProfileVersion);
		}
		if (!IsExactPinnedRef(ConfigRefs.TrailerProfileVersion))
		{
			AddMissingPinDiagnostic(PinDiagnostics, "AttemptConfig.TrailerProfileVersion", ConfigRefs.TrailerProfileVersion);
		}
		if (!IsExactPinnedRef(ConfigRefs.ScoringProfileVersion))
		{
			AddMissingPinDiagnostic(PinDiagnostics, "AttemptConfig.ScoringProfileVersion", ConfigRefs.ScoringProfileVersion);
		}
		if (!PinDiagnostics.empty())
		{
			return RejectAttemptStart(
				ESessionError::ConfigurationIncomplete,
				"Attempt start rejected because every config reference must be an exact version pin",
				std::move(PinDiagnostics));
		}

		const ELaunchValidationMode ConfigMode = Mode == ESessionAttemptMode::Validated
			? ELaunchValidationMode::Validated
			: ELaunchValidationMode::SyntheticPracticeOnly;
		const FConfigurationModeValidationResult ModeResult = ConfigurationProvider.ValidateForMode(
			ConfigRefs.ExerciseVersion.ProfileId,
			ConfigMode);

		if (!ModeResult.bReadyForRequestedMode)
		{
			return RejectAttemptStart(
				ESessionError::ConfigurationIncomplete,
				"Configuration provider rejected the requested attempt mode",
				ModeResult.Diagnostics);
		}

		if (!ResolvedContains(ModeResult.ResolvedProfiles, EProfileType::ExerciseDefinition, ConfigRefs.ExerciseVersion))
		{
			return RejectAttemptStart(
				ESessionError::ConfigurationVersionMismatch,
				"Resolved launch closure did not include exact requested " +
					DescribeRef(EProfileType::ExerciseDefinition, ConfigRefs.ExerciseVersion));
		}
		if (!ResolvedContains(ModeResult.ResolvedProfiles, EProfileType::VehicleProfile, ConfigRefs.VehicleProfileVersion))
		{
			return RejectAttemptStart(
				ESessionError::ConfigurationVersionMismatch,
				"Resolved launch closure did not include exact requested " +
					DescribeRef(EProfileType::VehicleProfile, ConfigRefs.VehicleProfileVersion));
		}
		if (!ResolvedContains(ModeResult.ResolvedProfiles, EProfileType::TrailerProfile, ConfigRefs.TrailerProfileVersion))
		{
			return RejectAttemptStart(
				ESessionError::ConfigurationVersionMismatch,
				"Resolved launch closure did not include exact requested " +
					DescribeRef(EProfileType::TrailerProfile, ConfigRefs.TrailerProfileVersion));
		}
		if (!ResolvedContains(ModeResult.ResolvedProfiles, EProfileType::ScoringProfile, ConfigRefs.ScoringProfileVersion))
		{
			return RejectAttemptStart(
				ESessionError::ConfigurationVersionMismatch,
				"Resolved launch closure did not include exact requested " +
					DescribeRef(EProfileType::ScoringProfile, ConfigRefs.ScoringProfileVersion));
		}

		if (Mode == ESessionAttemptMode::Validated)
		{
			return RejectAttemptStart(
				ESessionError::ExplicitSyntheticIdentityRequired,
				"Validated attempts remain deferred in P2-02; only TryBeginSyntheticPracticeAttempt is runnable");
		}

		std::vector<FMissingFieldDiagnostic> SyntheticDiagnostics;
		for (const FConfigSnapshotEnvelope& Envelope : ModeResult.ResolvedProfiles)
		{
			const FConfigVersionRef& Ref = Envelope.Provenance.VersionRef;
			std::optional<FConfigurationProfileSnapshot> Snapshot =
				ConfigurationProvider.GetProfileSnapshot(Envelope.ProfileType, Ref.ProfileId, Ref.Version);
			if (!Snapshot.has_value())
			{
				SyntheticDiagnostics.push_back({
					DescribeRef(Envelope.ProfileType, Ref),
					"Resolved profile was not retrievable from the configuration provider"
				});
				continue;
			}

			if (Snapshot->SourceAuthority != EProfileSourceAuthority::SyntheticPracticeOnly)
			{
				SyntheticDiagnostics.push_back({
					DescribeRef(Envelope.ProfileType, Ref) + ".SourceAuthority",
					"Synthetic practice session manager only admits SyntheticPracticeOnly profiles in P2-02"
				});
			}
		}

		if (!SyntheticDiagnostics.empty())
		{
			return RejectAttemptStart(
				ESessionError::ConfigurationIncomplete,
				"Synthetic practice attempt rejected because every resolved profile must be SyntheticPracticeOnly",
				std::move(SyntheticDiagnostics));
		}

		FAttemptStartResult Result;
		Result.bSucceeded = true;
		return Result;
	}

	bool FSessionManager::HasAttemptId(const FAttemptId& AttemptId) const
	{
		return FindAttempt(AttemptId) != nullptr;
	}

	FSessionManager::FAttemptRecord* FSessionManager::FindAttempt(const FAttemptId& AttemptId)
	{
		const auto Found = std::find_if(
			Attempts.begin(),
			Attempts.end(),
			[&AttemptId](const FAttemptRecord& Record) {
				return Record.Context.AttemptId == AttemptId;
			});
		return Found == Attempts.end() ? nullptr : &(*Found);
	}

	const FSessionManager::FAttemptRecord* FSessionManager::FindAttempt(const FAttemptId& AttemptId) const
	{
		const auto Found = std::find_if(
			Attempts.begin(),
			Attempts.end(),
			[&AttemptId](const FAttemptRecord& Record) {
				return Record.Context.AttemptId == AttemptId;
			});
		return Found == Attempts.end() ? nullptr : &(*Found);
	}

	FAttemptRecordSnapshot FSessionManager::SnapshotAttempt(const FAttemptRecord& Record) const
	{
		FAttemptRecordSnapshot Snapshot;
		Snapshot.Context = Record.Context;
		Snapshot.State = Record.State;
		Snapshot.LifecycleEvents = Record.LifecycleEvents;
		Snapshot.bHasResetPolicy = Record.bHasResetPolicy;
		Snapshot.LastResetPolicy = Record.LastResetPolicy;
		return Snapshot;
	}

	bool FSessionManager::IsCurrentActiveAttempt(const FAttemptId& AttemptId) const
	{
		return ActiveAttemptId.has_value() && *ActiveAttemptId == AttemptId;
	}

	std::optional<FDomainEvent> FSessionManager::MakeLifecycleEvent(
		const FAttemptRecord& Record,
		EDomainEventType EventType,
		const std::string& SourceTag,
		std::string& OutError) const
	{
		const FMonotonicTimestamp Timestamp = Clock.NowSeconds() - SessionClockOrigin;
		if (!std::isfinite(Timestamp) || Timestamp < 0.0)
		{
			OutError = "Lifecycle event rejected because the session clock returned a negative or non-finite timestamp";
			return std::nullopt;
		}

		if (!Record.LifecycleEvents.empty() && Timestamp < Record.LifecycleEvents.back().Timestamp)
		{
			OutError = "Lifecycle event rejected because the session clock moved backward";
			return std::nullopt;
		}
		if (!Attempts.empty() && !Attempts.back().LifecycleEvents.empty() &&
			Timestamp < Attempts.back().LifecycleEvents.back().Timestamp)
		{
			OutError = "Lifecycle event rejected because the session clock moved backward between attempts";
			return std::nullopt;
		}

		FDomainEvent Event;
		Event.Attempt = Record.Context.AttemptId;
		Event.Timestamp = Timestamp;
		Event.EventType = EventType;
		Event.Evidence.SourceDetectorId = SourceTag;
		return Event;
	}
}
