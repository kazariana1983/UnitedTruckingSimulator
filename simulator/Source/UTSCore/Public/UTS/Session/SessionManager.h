// P2-02 deterministic session manager implementation support.
//
// This is the headless-testable core used by the Unreal subsystem
// adapter. It deliberately admits only explicitly labeled synthetic practice
// sessions in this increment. Validated/evaluation attempts continue to fail
// closed through the configuration gate until human-approved ground truth and
// auth/offline policies exist.

#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "UTS/Config/ConfigurationStore.h"
#include "UTS/Detection/IDetectionEventSink.h"
#include "UTS/Session/ISessionManager.h"

#ifndef UTSCORE_API
#define UTSCORE_API
#endif

namespace UTS
{
	enum class ESessionAttemptMode : uint8_t
	{
		Validated,
		SyntheticPracticeOnly
	};

	struct FAttemptConfigRefs
	{
		FConfigVersionRef ExerciseVersion;
		FConfigVersionRef VehicleProfileVersion;
		FConfigVersionRef TrailerProfileVersion;
		FConfigVersionRef ScoringProfileVersion;
	};

	class UTSCORE_API ISessionClock
	{
	public:
		virtual ~ISessionClock() = default;
		virtual FMonotonicTimestamp NowSeconds() const = 0;
	};

	class UTSCORE_API ISessionIdGenerator
	{
	public:
		virtual ~ISessionIdGenerator() = default;
		virtual FSessionId CreateSessionId() = 0;
		virtual FAttemptId CreateAttemptId() = 0;
		virtual FSessionId CreateTelemetrySessionId(const FAttemptId& AttemptId) = 0;
	};

	class UTSCORE_API FSteadySessionClock final : public ISessionClock
	{
	public:
		FSteadySessionClock();
		FMonotonicTimestamp NowSeconds() const override;

	private:
		std::chrono::steady_clock::time_point StartTime;
	};

	class UTSCORE_API FSequentialSessionIdGenerator final : public ISessionIdGenerator
	{
	public:
		explicit FSequentialSessionIdGenerator(std::string Prefix = "uts-dev");

		FSessionId CreateSessionId() override;
		FAttemptId CreateAttemptId() override;
		FSessionId CreateTelemetrySessionId(const FAttemptId& AttemptId) override;

	private:
		std::string Prefix;
		uint64_t NextSessionOrdinal = 1;
		uint64_t NextAttemptOrdinal = 1;
		uint64_t NextTelemetryOrdinal = 1;
	};

	struct FSessionManagerDependencies
	{
		const FFileConfigurationProvider* ConfigurationProvider = nullptr;
		ISessionClock* Clock = nullptr;
		ISessionIdGenerator* IdGenerator = nullptr;
	};

	struct FAttemptRecordSnapshot
	{
		FAttemptContext Context;
		EAttemptState State = EAttemptState::Unknown;
		std::vector<FDomainEvent> LifecycleEvents;
		bool bHasResetPolicy = false;
		EResetPolicy LastResetPolicy = EResetPolicy::AbortAndKeepEvidence;
	};

	UTSCORE_API std::string ToString(ESessionState State);
	UTSCORE_API std::string ToString(EAttemptState State);
	UTSCORE_API std::string ToString(ESessionError Error);
	UTSCORE_API std::string ToString(EDomainEventType EventType);
	UTSCORE_API std::string ToString(EResetPolicy Policy);

	class UTSCORE_API FSessionManager final : public ISessionManager
	{
	public:
		explicit FSessionManager(FSessionManagerDependencies InDependencies);
		FSessionManager(const FSessionManager&) = delete;
		FSessionManager& operator=(const FSessionManager&) = delete;

		ESessionState GetSessionState() const override;
		bool IsBackendReachable() const override;

		FSessionStartResult BeginSyntheticPracticeSession(
			const FSyntheticSessionStartRequest& Request) override;

		// The legacy interface is intentionally not enough identity for P2-02;
		// it fails closed rather than creating an implicit synthetic user.
		bool BeginSession(const FStationId& StationId) override;

		FAttemptStartResult TryBeginAttempt(
			const FConfigVersionRef& ExerciseVersion,
			const FConfigVersionRef& VehicleProfileVersion,
			const FConfigVersionRef& TrailerProfileVersion,
			const FConfigVersionRef& ScoringProfileVersion) override;

		FAttemptStartResult TryBeginSyntheticPracticeAttempt(
			const FConfigVersionRef& ExerciseVersion,
			const FConfigVersionRef& VehicleProfileVersion,
			const FConfigVersionRef& TrailerProfileVersion,
			const FConfigVersionRef& ScoringProfileVersion);

		FAttemptStartResult TryBeginAttemptWithMode(
			const FAttemptConfigRefs& ConfigRefs,
			ESessionAttemptMode Mode);

		EAttemptState GetAttemptState(const FAttemptId& AttemptId) const override;

		FSessionCommandResult PauseAttempt(const FAttemptId& AttemptId) override;
		FSessionCommandResult ResumeAttempt(const FAttemptId& AttemptId) override;
		FSessionCommandResult RequestReset(const FAttemptId& AttemptId, EResetPolicy Policy) override;
		FSessionCommandResult CompleteOrAbort(const FAttemptId& AttemptId, EAttemptOutcome Outcome) override;

		FSessionCommandResult OnBackendConnectivityChanged(bool bReachable) override;
		bool TryRecoverInterruptedAttempt(FAttemptContext& OutContext) override;

		std::vector<FDomainEvent> GetAttemptEvents(const FAttemptId& AttemptId) const;
		std::optional<FAttemptRecordSnapshot> GetAttemptRecord(const FAttemptId& AttemptId) const;
		std::size_t GetAttemptCount() const;
		std::optional<FAttemptId> GetActiveAttemptId() const;
		std::optional<FSessionId> GetCurrentSessionId() const;

	private:
		struct FAttemptRecord
		{
			FAttemptContext Context;
			EAttemptState State = EAttemptState::Unknown;
			std::vector<FDomainEvent> LifecycleEvents;
			bool bHasResetPolicy = false;
			EResetPolicy LastResetPolicy = EResetPolicy::AbortAndKeepEvidence;
		};

		FSessionStartResult RejectSessionStart(
			ESessionError Error,
			const std::string& Message) const;

		FAttemptStartResult RejectAttemptStart(
			ESessionError Error,
			const std::string& Message,
			std::vector<FMissingFieldDiagnostic> MissingFields = {}) const;

		FSessionCommandResult RejectCommand(
			ESessionError Error,
			const std::string& Message,
			std::vector<FMissingFieldDiagnostic> MissingFields = {}) const;

		FAttemptStartResult ValidateAttemptConfig(
			const FAttemptConfigRefs& ConfigRefs,
			ESessionAttemptMode Mode) const;

		bool HasAttemptId(const FAttemptId& AttemptId) const;
		FAttemptRecord* FindAttempt(const FAttemptId& AttemptId);
		const FAttemptRecord* FindAttempt(const FAttemptId& AttemptId) const;
		FAttemptRecordSnapshot SnapshotAttempt(const FAttemptRecord& Record) const;
		bool IsCurrentActiveAttempt(const FAttemptId& AttemptId) const;

		std::optional<FDomainEvent> MakeLifecycleEvent(
			const FAttemptRecord& Record,
			EDomainEventType EventType,
			const std::string& SourceTag,
			std::string& OutError) const;

		const FFileConfigurationProvider& ConfigurationProvider;
		ISessionClock& Clock;
		ISessionIdGenerator& IdGenerator;

		ESessionState SessionState = ESessionState::Booting;
		bool bBackendReachable = true;
		std::optional<FStationId> CurrentStationId;
		std::optional<FStudentId> CurrentSyntheticStudentId;
		std::optional<FSessionId> CurrentSessionId;
		std::string ClientSoftwareVersion;
		double SessionClockOrigin = 0.0;
		std::optional<FAttemptId> ActiveAttemptId;
		std::vector<FAttemptRecord> Attempts;
	};
}
