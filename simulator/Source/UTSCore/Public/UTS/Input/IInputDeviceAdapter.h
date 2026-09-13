// IInputDeviceAdapter — normalizes keyboard/wheel/pedal/clutch/shifter input
// into semantic control actions.
//
// See docs/INTERFACE_CONTRACTS.md §3. Vehicle/tractor code must never poll a
// raw device or call a vendor SDK directly (FR-INP-003) — it consumes only
// FSemanticControlFrame from this interface.

#pragma once

#include <optional>
#include "UTS/Common/UTSCommonTypes.h"

namespace UTS
{
	enum class EDeviceStatus : uint8_t
	{
		Connected,
		Disconnected,
		Faulted
	};

	enum class EDeviceKind : uint8_t
	{
		Keyboard,
		WheelPedalSet,
		Clutch,
		Shifter
	};

	// Raw per-source signal. Shape only — no calibration applied yet.
	struct FRawDeviceFrame
	{
		EDeviceKind SourceKind = EDeviceKind::Keyboard;
		// Concrete raw fields (axis counts, button bitfields, etc.) are
		// device-specific and defined by each IInputSource implementation;
		// this base shape only carries what every source has in common.
		FMonotonicTimestamp Timestamp = 0.0;
	};

	// The ONLY type tractor/vehicle code may consume (FR-INP-001).
	struct FSemanticControlFrame
	{
		float SteeringNormalized = 0.0f; // [-1, 1]
		float ThrottleNormalized = 0.0f; // [0, 1]
		float BrakeNormalized    = 0.0f; // [0, 1]
		std::optional<float> ClutchNormalized;   // [0,1], absent if no clutch configured
		bool  bShiftUp   = false;
		bool  bShiftDown = false;
		std::optional<int32_t> GearSelect;       // absent if no H-pattern/range-splitter configured
		bool  bParkingBrakeEngaged = false;
	};

	struct FCalibrationProfile
	{
		FConfigVersionRef VersionRef;
		float Center = 0.0f;
		float RangeDegreesOrUnits = 0.0f; // TBD real value; shape only
		bool  bInverted = false;
		float DeadZoneNormalized = 0.0f;
		// Response curve is represented as an opaque curve asset reference by
		// the concrete implementation; this interface only guarantees the
		// scalar calibration fields every device shares.
	};

	class IInputDeviceAdapter
	{
	public:
		virtual ~IInputDeviceAdapter() = default;

		virtual FRawDeviceFrame PollRaw(EDeviceKind Source) = 0;

		virtual FSemanticControlFrame Normalize(
			const FRawDeviceFrame& Raw, const FCalibrationProfile& Calibration) = 0;

		virtual void ApplyCalibration(EDeviceKind Source, const FCalibrationProfile& Calibration) = 0;

		// A Disconnected/Faulted transition must emit a domain event and
		// drive the session toward the approved safe response (TBD — see
		// docs/ARCHITECTURE_OPEN_DECISIONS.md item A.8), never let the last
		// known frame keep silently driving the vehicle.
		virtual EDeviceStatus GetDeviceStatus(EDeviceKind Source) const = 0;
	};
}
