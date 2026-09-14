#include "UTS/Session/SessionManager.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <utility>

namespace
{
	int FailureCount = 0;

	#define CHECK(Condition) \
		do \
		{ \
			if (!(Condition)) \
			{ \
				std::cerr << "CHECK failed at " << __FILE__ << ":" << __LINE__ << ": " #Condition << std::endl; \
				++FailureCount; \
			} \
		} while (false)

	UTS::FProfileId ProfileId(const std::string& Value)
	{
		UTS::FProfileId Result;
		Result.Value = Value;
		return Result;
	}

	UTS::FStationId StationId(const std::string& Value)
	{
		UTS::FStationId Result;
		Result.Value = Value;
		return Result;
	}

	UTS::FStudentId StudentId(const std::string& Value)
	{
		UTS::FStudentId Result;
		Result.Value = Value;
		return Result;
	}

	UTS::FAttemptId AttemptId(const std::string& Value)
	{
		UTS::FAttemptId Result;
		Result.Value = Value;
		return Result;
	}

	UTS::FConfigVersionRef Ref(const std::string& Profile, uint32_t Version)
	{
		UTS::FConfigVersionRef Result;
		Result.ProfileId = ProfileId(Profile);
		Result.Version = Version;
		return Result;
	}

	bool SameRef(const UTS::FConfigVersionRef& Left, const UTS::FConfigVersionRef& Right)
	{
		return Left.ProfileId == Right.ProfileId && Left.Version == Right.Version;
	}

	bool DiagnosticsContain(const std::vector<UTS::FMissingFieldDiagnostic>& Diagnostics, const std::string& Needle)
	{
		for (const UTS::FMissingFieldDiagnostic& Diagnostic : Diagnostics)
		{
			if (Diagnostic.FieldPath.find(Needle) != std::string::npos ||
				Diagnostic.Reason.find(Needle) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	bool EventsAreFiniteAndNondecreasing(const std::vector<UTS::FDomainEvent>& Events)
	{
		double Previous = -1.0;
		for (const UTS::FDomainEvent& Event : Events)
		{
			if (!std::isfinite(Event.Timestamp) || Event.Timestamp < Previous)
			{
				return false;
			}
			Previous = Event.Timestamp;
		}
		return true;
	}

	bool EventsHaveStableSessionSourceOnly(const std::vector<UTS::FDomainEvent>& Events)
	{
		for (const UTS::FDomainEvent& Event : Events)
		{
			if (Event.Evidence.SourceDetectorId.find("UTS.SessionManager") != 0 ||
				!Event.Evidence.ThresholdConfigRef.empty() ||
				!Event.Evidence.PoseSnapshotRef.empty())
			{
				return false;
			}
		}
		return true;
	}

	class FManualClock final : public UTS::ISessionClock
	{
	public:
		UTS::FMonotonicTimestamp NowSeconds() const override
		{
			return Now;
		}

		void Advance(double DeltaSeconds)
		{
			Now += DeltaSeconds;
		}

		void Set(double NewNow)
		{
			Now = NewNow;
		}

	private:
		double Now = 0.0;
	};

	class FScriptedIdGenerator final : public UTS::ISessionIdGenerator
	{
	public:
		FScriptedIdGenerator(
			std::vector<std::string> InSessionIds,
			std::vector<std::string> InAttemptIds,
			std::vector<std::string> InTelemetryIds)
			: SessionIds(std::move(InSessionIds))
			, AttemptIds(std::move(InAttemptIds))
			, TelemetryIds(std::move(InTelemetryIds))
		{
		}

		UTS::FSessionId CreateSessionId() override
		{
			UTS::FSessionId Result;
			Result.Value = Next(SessionIds, NextSessionIndex, "session-auto-");
			return Result;
		}

		UTS::FAttemptId CreateAttemptId() override
		{
			UTS::FAttemptId Result;
			Result.Value = Next(AttemptIds, NextAttemptIndex, "attempt-auto-");
			return Result;
		}

		UTS::FSessionId CreateTelemetrySessionId(const UTS::FAttemptId& AttemptId) override
		{
			UTS::FSessionId Result;
			Result.Value = Next(TelemetryIds, NextTelemetryIndex, "telemetry-auto-" + AttemptId.Value + "-");
			return Result;
		}

	private:
		static std::string Next(
			const std::vector<std::string>& Values,
			std::size_t& Index,
			const std::string& FallbackPrefix)
		{
			if (Index < Values.size())
			{
				return Values[Index++];
			}
			return FallbackPrefix + std::to_string(++Index);
		}

		std::vector<std::string> SessionIds;
		std::vector<std::string> AttemptIds;
		std::vector<std::string> TelemetryIds;
		std::size_t NextSessionIndex = 0;
		std::size_t NextAttemptIndex = 0;
		std::size_t NextTelemetryIndex = 0;
	};

	UTS::FSyntheticSessionStartRequest SyntheticSessionRequest()
	{
		UTS::FSyntheticSessionStartRequest Request;
		Request.StationId = StationId("station.synthetic.dev");
		Request.SyntheticStudentId = StudentId("student.synthetic.dev");
		Request.ClientSoftwareVersion = "session-manager-tests";
		return Request;
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "usage: session_manager_tests <profile-root>" << std::endl;
		return EXIT_FAILURE;
	}

	UTS::FFileConfigurationProvider Provider;
	const UTS::FProfileSetLoadResult LoadResult = Provider.LoadFromDirectory(argv[1]);
	CHECK(LoadResult.bSucceeded);

	const UTS::FConfigVersionRef SyntheticExercise = Ref("uts.synthetic.practice.exercise.straight_backing", 1);
	const UTS::FConfigVersionRef SyntheticVehicle = Ref("uts.synthetic.practice.vehicle", 1);
	const UTS::FConfigVersionRef SyntheticTrailer = Ref("uts.synthetic.practice.trailer", 1);
	const UTS::FConfigVersionRef SyntheticScoring = Ref("uts.synthetic.practice.scoring", 1);

	const UTS::FConfigVersionRef GroundTruthExercise = Ref("uts.gt_pending.exercise.straight_backing", 1);
	const UTS::FConfigVersionRef GroundTruthVehicle = Ref("uts.gt_pending.vehicle", 1);
	const UTS::FConfigVersionRef GroundTruthTrailer = Ref("uts.gt_pending.trailer", 1);
	const UTS::FConfigVersionRef GroundTruthScoring = Ref("uts.gt_pending.scoring", 1);

	{
		FManualClock Clock;
		FScriptedIdGenerator Ids(
			{"session-1"},
			{"attempt-1", "attempt-2"},
			{"telemetry-1", "telemetry-2"});
		UTS::FSessionManager Manager({&Provider, &Clock, &Ids});

		CHECK(Manager.GetSessionState() == UTS::ESessionState::Booting);
		CHECK(Manager.GetAttemptState(AttemptId("missing")) == UTS::EAttemptState::Unknown);
		CHECK(!Manager.BeginSession(StationId("legacy-station-only")));
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Booting);

		const UTS::FAttemptStartResult StartBeforeSession = Manager.TryBeginSyntheticPracticeAttempt(
			SyntheticExercise,
			SyntheticVehicle,
			SyntheticTrailer,
			SyntheticScoring);
		CHECK(!StartBeforeSession.bSucceeded);
		CHECK(StartBeforeSession.Error == UTS::ESessionError::ExplicitSyntheticIdentityRequired);
		CHECK(Manager.GetAttemptCount() == 0);

		const UTS::FSessionStartResult SessionStart = Manager.BeginSyntheticPracticeSession(SyntheticSessionRequest());
		CHECK(SessionStart.bSucceeded);
		CHECK(SessionStart.SessionId.Value == "session-1");
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Ready);
		CHECK(Manager.IsBackendReachable());

		const UTS::FAttemptStartResult ValidatedStart = Manager.TryBeginAttempt(
			GroundTruthExercise,
			GroundTruthVehicle,
			GroundTruthTrailer,
			GroundTruthScoring);
		CHECK(!ValidatedStart.bSucceeded);
		CHECK(ValidatedStart.Error == UTS::ESessionError::ConfigurationIncomplete);
		CHECK(DiagnosticsContain(ValidatedStart.MissingFields, "Validated launch remains unavailable"));
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Ready);
		CHECK(Manager.GetAttemptCount() == 0);

		const UTS::FAttemptStartResult Start = Manager.TryBeginSyntheticPracticeAttempt(
			SyntheticExercise,
			SyntheticVehicle,
			SyntheticTrailer,
			SyntheticScoring);
		CHECK(Start.bSucceeded);
		CHECK(Start.Context.AttemptId.Value == "attempt-1");
		CHECK(Start.Context.OwningSessionId.Value == "session-1");
		CHECK(Start.Context.TelemetrySessionId.Value == "telemetry-1");
		CHECK(Start.Context.bSyntheticPracticeOnly);
		CHECK(SameRef(Start.Context.ExerciseVersion, SyntheticExercise));
		CHECK(SameRef(Start.Context.VehicleProfileVersion, SyntheticVehicle));
		CHECK(SameRef(Start.Context.TrailerProfileVersion, SyntheticTrailer));
		CHECK(SameRef(Start.Context.ScoringProfileVersion, SyntheticScoring));
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Active);
		CHECK(Manager.GetAttemptState(Start.Context.AttemptId) == UTS::EAttemptState::Active);
		CHECK(Manager.GetAttemptCount() == 1);
		CHECK(Manager.GetActiveAttemptId().has_value());
		CHECK(Manager.GetActiveAttemptId()->Value == "attempt-1");

		std::vector<UTS::FDomainEvent> Events = Manager.GetAttemptEvents(Start.Context.AttemptId);
		CHECK(Events.size() == 1);
		CHECK(Events[0].EventType == UTS::EDomainEventType::AttemptStarted);
		CHECK(Events[0].Timestamp == 0.0);
		CHECK(EventsHaveStableSessionSourceOnly(Events));

		const UTS::FAttemptStartResult DuplicateStart = Manager.TryBeginSyntheticPracticeAttempt(
			SyntheticExercise,
			SyntheticVehicle,
			SyntheticTrailer,
			SyntheticScoring);
		CHECK(!DuplicateStart.bSucceeded);
		CHECK(DuplicateStart.Error == UTS::ESessionError::AttemptAlreadyActive);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == Events.size());

		const UTS::FSessionCommandResult WrongPause = Manager.PauseAttempt(AttemptId("attempt-missing"));
		CHECK(!WrongPause.bSucceeded);
		CHECK(WrongPause.Error == UTS::ESessionError::AttemptNotFound);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == Events.size());

		Clock.Advance(1.0);
		const UTS::FSessionCommandResult Pause = Manager.PauseAttempt(Start.Context.AttemptId);
		CHECK(Pause.bSucceeded);
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Paused);
		CHECK(Manager.GetAttemptState(Start.Context.AttemptId) == UTS::EAttemptState::Active);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 2);

		const UTS::FSessionCommandResult PauseAgain = Manager.PauseAttempt(Start.Context.AttemptId);
		CHECK(!PauseAgain.bSucceeded);
		CHECK(PauseAgain.Error == UTS::ESessionError::InvalidTransition);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 2);

		Clock.Advance(0.5);
		const UTS::FSessionCommandResult Resume = Manager.ResumeAttempt(Start.Context.AttemptId);
		CHECK(Resume.bSucceeded);
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Active);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 3);

		Clock.Advance(0.5);
		Manager.OnBackendConnectivityChanged(false);
		CHECK(!Manager.IsBackendReachable());
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Active);
		CHECK(Manager.GetAttemptState(Start.Context.AttemptId) == UTS::EAttemptState::Active);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 4);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).back().EventType == UTS::EDomainEventType::BackendConnectionLost);

		Manager.OnBackendConnectivityChanged(false);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 4);

		Clock.Advance(0.5);
		const UTS::FSessionCommandResult Reset = Manager.RequestReset(
			Start.Context.AttemptId,
			UTS::EResetPolicy::AbortAndKeepEvidence);
		CHECK(!Reset.bSucceeded);
		CHECK(Reset.Error == UTS::ESessionError::InvalidTransition);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 4);

		const UTS::FSessionCommandResult SupportedReset = Manager.RequestReset(
			Start.Context.AttemptId,
			UTS::EResetPolicy::ResetInPlaceAndKeepPriorSegment);
		CHECK(SupportedReset.bSucceeded);
		std::optional<UTS::FAttemptRecordSnapshot> AfterReset = Manager.GetAttemptRecord(Start.Context.AttemptId);
		CHECK(AfterReset.has_value());
		CHECK(AfterReset->bHasResetPolicy);
		CHECK(AfterReset->LifecycleEvents.size() == 5);
		CHECK(AfterReset->LifecycleEvents.back().EventType == UTS::EDomainEventType::AttemptReset);
		CHECK(AfterReset->LifecycleEvents.back().Evidence.SourceDetectorId.find("ResetInPlaceAndKeepPriorSegment") != std::string::npos);

		Clock.Advance(0.5);
		Manager.OnBackendConnectivityChanged(true);
		CHECK(Manager.IsBackendReachable());
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Active);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 6);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).back().EventType == UTS::EDomainEventType::BackendConnectionRestored);

		const UTS::FSessionCommandResult InvalidOutcome = Manager.CompleteOrAbort(
			Start.Context.AttemptId,
			static_cast<UTS::EAttemptOutcome>(255));
		CHECK(!InvalidOutcome.bSucceeded);
		CHECK(InvalidOutcome.Error == UTS::ESessionError::InvalidTransition);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 6);

		Clock.Advance(0.5);
		const UTS::FSessionCommandResult Complete = Manager.CompleteOrAbort(
			Start.Context.AttemptId,
			UTS::EAttemptOutcome::Completed);
		CHECK(Complete.bSucceeded);
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Results);
		CHECK(Manager.GetAttemptState(Start.Context.AttemptId) == UTS::EAttemptState::Completed);
		CHECK(!Manager.GetActiveAttemptId().has_value());
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 7);

		const UTS::FSessionCommandResult CompleteAgain = Manager.CompleteOrAbort(
			Start.Context.AttemptId,
			UTS::EAttemptOutcome::Completed);
		CHECK(!CompleteAgain.bSucceeded);
		CHECK(CompleteAgain.Error == UTS::ESessionError::InvalidTransition);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 7);

		const UTS::FSessionCommandResult ResetCompleted = Manager.RequestReset(
			Start.Context.AttemptId,
			UTS::EResetPolicy::ResetInPlaceAndKeepPriorSegment);
		CHECK(!ResetCompleted.bSucceeded);
		CHECK(ResetCompleted.Error == UTS::ESessionError::InvalidTransition);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 7);

		const std::vector<UTS::FDomainEvent> CompletedEvents = Manager.GetAttemptEvents(Start.Context.AttemptId);
		CHECK(EventsAreFiniteAndNondecreasing(CompletedEvents));
		CHECK(EventsHaveStableSessionSourceOnly(CompletedEvents));

		const UTS::FAttemptStartResult SecondStart = Manager.TryBeginSyntheticPracticeAttempt(
			SyntheticExercise,
			SyntheticVehicle,
			SyntheticTrailer,
			SyntheticScoring);
		CHECK(SecondStart.bSucceeded);
		CHECK(SecondStart.Context.AttemptId.Value == "attempt-2");
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Active);

		Clock.Advance(0.5);
		const UTS::FSessionCommandResult Abort = Manager.CompleteOrAbort(
			SecondStart.Context.AttemptId,
			UTS::EAttemptOutcome::Aborted);
		CHECK(Abort.bSucceeded);
		CHECK(Manager.GetAttemptState(SecondStart.Context.AttemptId) == UTS::EAttemptState::Aborted);
		CHECK(Manager.GetAttemptEvents(SecondStart.Context.AttemptId).back().EventType == UTS::EDomainEventType::AttemptAborted);

		UTS::FAttemptContext RecoveredContext;
		CHECK(!Manager.TryRecoverInterruptedAttempt(RecoveredContext));
	}

	{
		FManualClock Clock;
		FScriptedIdGenerator Ids({"session-mismatch"}, {"attempt-mismatch"}, {"telemetry-mismatch"});
		UTS::FSessionManager Manager({&Provider, &Clock, &Ids});
		CHECK(Manager.BeginSyntheticPracticeSession(SyntheticSessionRequest()).bSucceeded);

		const UTS::FAttemptStartResult MismatchStart = Manager.TryBeginSyntheticPracticeAttempt(
			SyntheticExercise,
			GroundTruthVehicle,
			SyntheticTrailer,
			SyntheticScoring);
		CHECK(!MismatchStart.bSucceeded);
		CHECK(MismatchStart.Error == UTS::ESessionError::ConfigurationVersionMismatch);
		CHECK(Manager.GetAttemptCount() == 0);

		const UTS::FAttemptStartResult ZeroVersionStart = Manager.TryBeginSyntheticPracticeAttempt(
			SyntheticExercise,
			Ref("uts.synthetic.practice.vehicle", 0),
			SyntheticTrailer,
			SyntheticScoring);
		CHECK(!ZeroVersionStart.bSucceeded);
		CHECK(ZeroVersionStart.Error == UTS::ESessionError::ConfigurationIncomplete);
		CHECK(DiagnosticsContain(ZeroVersionStart.MissingFields, "version 0"));
		CHECK(Manager.GetAttemptCount() == 0);
	}

	{
		FManualClock Clock;
		FScriptedIdGenerator Ids({"session-clock"}, {"attempt-clock"}, {"telemetry-clock"});
		UTS::FSessionManager Manager({&Provider, &Clock, &Ids});
		CHECK(Manager.BeginSyntheticPracticeSession(SyntheticSessionRequest()).bSucceeded);

		Clock.Set(10.0);
		const UTS::FAttemptStartResult Start = Manager.TryBeginSyntheticPracticeAttempt(
			SyntheticExercise,
			SyntheticVehicle,
			SyntheticTrailer,
			SyntheticScoring);
		CHECK(Start.bSucceeded);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 1);

		Clock.Set(9.0);
		const UTS::FSessionCommandResult BackwardClockPause = Manager.PauseAttempt(Start.Context.AttemptId);
		CHECK(!BackwardClockPause.bSucceeded);
		CHECK(BackwardClockPause.Error == UTS::ESessionError::InvalidTransition);
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Active);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 1);

		Clock.Set(std::numeric_limits<double>::quiet_NaN());
		const UTS::FSessionCommandResult NanClockPause = Manager.PauseAttempt(Start.Context.AttemptId);
		CHECK(!NanClockPause.bSucceeded);
		CHECK(NanClockPause.Error == UTS::ESessionError::InvalidTransition);
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Active);
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 1);
		const auto FailedConnectivity = Manager.OnBackendConnectivityChanged(false);
		CHECK(!FailedConnectivity.bSucceeded);
		CHECK(!FailedConnectivity.DiagnosticMessage.empty());
		CHECK(Manager.IsBackendReachable());
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 1);
		Clock.Set(-1.0);
		CHECK(!Manager.CompleteOrAbort(Start.Context.AttemptId, UTS::EAttemptOutcome::Completed).bSucceeded);
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Active);
		Clock.Set(11.0);
		CHECK(Manager.OnBackendConnectivityChanged(false).bSucceeded);
		CHECK(!Manager.IsBackendReachable());
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Active);
		auto Copy = Manager.GetAttemptRecord(Start.Context.AttemptId);
		CHECK(Copy.has_value());
		if (Copy) Copy->LifecycleEvents.clear();
		CHECK(Manager.GetAttemptEvents(Start.Context.AttemptId).size() == 2);
	}

	{
		FManualClock Clock;
		FScriptedIdGenerator Ids({"session-invalid-mode"}, {"attempt-invalid-mode"}, {"telemetry-invalid-mode"});
		UTS::FSessionManager Manager({&Provider, &Clock, &Ids});
		CHECK(Manager.BeginSyntheticPracticeSession(SyntheticSessionRequest()).bSucceeded);
		const auto InvalidMode = Manager.TryBeginAttemptWithMode(
			{SyntheticExercise, SyntheticVehicle, SyntheticTrailer, SyntheticScoring},
			static_cast<UTS::ESessionAttemptMode>(255));
		CHECK(!InvalidMode.bSucceeded);
		CHECK(Manager.GetAttemptCount() == 0);
		CHECK(Manager.GetSessionState() == UTS::ESessionState::Ready);
	}

	{
		FManualClock Clock;
		FScriptedIdGenerator Ids(
			{"session-duplicate-id"},
			{"attempt-duplicate", "attempt-duplicate"},
			{"telemetry-duplicate-1", "telemetry-duplicate-2"});
		UTS::FSessionManager Manager({&Provider, &Clock, &Ids});
		CHECK(Manager.BeginSyntheticPracticeSession(SyntheticSessionRequest()).bSucceeded);

		const UTS::FAttemptStartResult FirstStart = Manager.TryBeginSyntheticPracticeAttempt(
			SyntheticExercise,
			SyntheticVehicle,
			SyntheticTrailer,
			SyntheticScoring);
		CHECK(FirstStart.bSucceeded);
		CHECK(Manager.CompleteOrAbort(FirstStart.Context.AttemptId, UTS::EAttemptOutcome::Completed).bSucceeded);
		const std::size_t CompletedEventCount = Manager.GetAttemptEvents(FirstStart.Context.AttemptId).size();

		const UTS::FAttemptStartResult DuplicateIdStart = Manager.TryBeginSyntheticPracticeAttempt(
			SyntheticExercise,
			SyntheticVehicle,
			SyntheticTrailer,
			SyntheticScoring);
		CHECK(!DuplicateIdStart.bSucceeded);
		CHECK(DuplicateIdStart.Error == UTS::ESessionError::DuplicateAttemptId);
		CHECK(Manager.GetAttemptCount() == 1);
		CHECK(Manager.GetAttemptState(FirstStart.Context.AttemptId) == UTS::EAttemptState::Completed);
		CHECK(Manager.GetAttemptEvents(FirstStart.Context.AttemptId).size() == CompletedEventCount);
	}

	if (FailureCount != 0)
	{
		std::cerr << FailureCount << " session manager test check(s) failed" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "session_manager_tests: PASS" << std::endl;
	return EXIT_SUCCESS;
}
