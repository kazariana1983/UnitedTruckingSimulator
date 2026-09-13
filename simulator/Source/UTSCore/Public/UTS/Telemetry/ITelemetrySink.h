// ITelemetrySink — samples/buffers/serializes/persists attempt data.
//
// See docs/INTERFACE_CONTRACTS.md §7. A local write failure must set an
// integrity flag and notify ISessionManager; it must never silently drop
// data (FR-TEL-004). Sampling rate and retention are TBD config fields
// (FR-TEL-002/008) — no default number is asserted anywhere in this header.

#pragma once

#include <optional>
#include <vector>
#include "UTS/Common/UTSCommonTypes.h"
#include "UTS/Vehicle/IVehicleStateProvider.h"
#include "UTS/Input/IInputDeviceAdapter.h"
#include "UTS/Detection/IDetectionEventSink.h"
#include "UTS/Exercise/IExerciseManager.h"

namespace UTS
{
	struct FActiveConfigVersions
	{
		FConfigVersionRef Vehicle;
		FConfigVersionRef Trailer;
		FConfigVersionRef Yard;
		FConfigVersionRef Exercise;
		FConfigVersionRef Scoring;
		FConfigVersionRef Input;
		FConfigVersionRef Telemetry;
	};

	struct FIntegrityFlags
	{
		bool bDroppedFrame = false;
		bool bClockDiscontinuity = false;
	};

	// SI units at this boundary regardless of the engine's internal
	// convention (docs/INTERFACE_CONTRACTS.md preamble).
	struct FTelemetryFrame
	{
		FAttemptId Attempt;
		FMonotonicTimestamp Timestamp = 0.0;
		FTractorState  TractorState;
		FTrailerState  TrailerState;
		FCouplingState CouplingState;
		std::optional<FRawDeviceFrame> RawInput;
		FSemanticControlFrame SemanticControl;
		std::string CameraModeState;
		EExerciseLifecycleState ExerciseLifecycleState = EExerciseLifecycleState::Unloaded;
		FActiveConfigVersions ActiveConfigVersions;
		FIntegrityFlags IntegrityFlags;
		uint32_t SchemaVersion = 1; // travels with every persisted chunk (FR-TEL-007, NFR-010)
	};

	enum class ETelemetryError : uint8_t
	{
		None,
		LocalWriteFailed,
		DiskFull,
		SerializationFailed
	};

	struct FIntegrityState
	{
		bool bAnyIntegrityFlagSet = false;
		std::vector<ETelemetryError> RecentErrors;
	};

	class ITelemetrySink
	{
	public:
		virtual ~ITelemetrySink() = default;

		virtual void RecordFrame(const FTelemetryFrame& Frame) = 0;

		// Independent of frame cadence (FR-TEL-003).
		virtual void RecordEvent(const FDomainEvent& Event) = 0;

		virtual ETelemetryError Flush() = 0;

		virtual FIntegrityState GetIntegrityState() const = 0;
	};
}
