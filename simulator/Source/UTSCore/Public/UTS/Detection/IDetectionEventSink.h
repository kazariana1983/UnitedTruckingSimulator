// IDetectionEventSink — produces timestamped domain-event FACTS only.
//
// See docs/INTERFACE_CONTRACTS.md §6. Detection never computes a score,
// penalty, or pass/fail: FDomainEvent below has no severity/points field at
// all. That separation is structural (NFR-002), not a convention.
//
// Detection thresholds are TBD (ground_truth §Training Yard / §Scoring).
// Until approved, concrete detectors emit *Candidate events for audit/tuning
// but must withhold the corresponding *Confirmed event.

#pragma once

#include <string>
#include <vector>
#include "UTS/Common/UTSCommonTypes.h"

namespace UTS
{
	enum class EDomainEventType : uint8_t
	{
		AttemptStarted, AttemptPaused, AttemptResumed, AttemptReset, AttemptCompleted, AttemptAborted,
		VehicleMovementStarted, VehicleMovementStopped,
		BoundaryEnter, BoundaryExit,
		ConeContact,
		PullUpCandidate, PullUpConfirmed,
		CompletionCandidate, CompletionConfirmed,
		DeviceDisconnected, DeviceReconnected,
		BackendConnectionLost, BackendConnectionRestored,
		TelemetryBufferStateChanged
	};

	// Enough raw data to audit *why* an event fired (pose snapshot reference,
	// threshold used, source volume/detector id) — FR-EXR-004's audit requirement.
	struct FEventEvidence
	{
		std::string SourceDetectorId;
		std::string ThresholdConfigRef; // points at the config field used, so "why" is traceable even before it's Approved
		std::string PoseSnapshotRef;    // opaque reference to the pose data at trigger time
	};

	struct FDomainEvent
	{
		FAttemptId Attempt;
		FMonotonicTimestamp Timestamp = 0.0;
		EDomainEventType EventType = EDomainEventType::AttemptStarted;
		FEventEvidence Evidence;
	};

	// One concrete class per detector kind implements this on the producer
	// side (UUTSBoundaryVolume, UUTSConeVolume, UUTSPullUpDetector,
	// UUTSCompletionDetector); IDetectionEventSink is the consumer-facing
	// registration point.
	class IDetector
	{
	public:
		virtual ~IDetector() = default;
		virtual std::string GetDetectorId() const = 0;
	};

	class IDetectionEventSink
	{
	public:
		virtual ~IDetectionEventSink() = default;

		virtual void RegisterDetector(IDetector& Detector) = 0;

		// Pull model for consumers that don't want a push callback; concrete
		// implementations may additionally expose a push/delegate mechanism
		// in the game module without changing this contract.
		virtual std::vector<FDomainEvent> DrainPendingEvents() = 0;
	};
}
