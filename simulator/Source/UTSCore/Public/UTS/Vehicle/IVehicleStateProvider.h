// IVehicleStateProvider — read-only tractor/trailer/coupling state.
//
// Published by the Tractor Vehicle, Trailer Physics, and Fifth-Wheel
// Coupling modules; consumed by Camera, Detection, and Telemetry. This is
// the SRS's "vehicle state provider" (§9) unifying those three physics
// modules — see docs/SRS_ARCHITECTURE_RECONCILIATION.md §2.
//
// No method on this interface may mutate exercise or scoring state (NFR-002).
// There is deliberately no "auto-correct"/"auto-straighten" method: adding
// one requires an explicit, approved FR-VEH-005 exception, not a quiet patch
// to this header.

#pragma once

#include "UTS/Common/UTSCommonTypes.h"

namespace UTS
{
	// A pose in the module's chosen convention (see docs/INTERFACE_CONTRACTS.md
	// preamble: native engine units internally, SI at every module boundary
	// that leaves UTSCore). This header defines shape only.
	struct FPose3D
	{
		double X = 0.0, Y = 0.0, Z = 0.0;
		double HeadingRad = 0.0;
	};

	struct FTractorState
	{
		FPose3D Pose;
		double  VelocityMetersPerSecond = 0.0;
		double  SteeringAngleRad = 0.0;
		float   ThrottleNormalized = 0.0f;
		float   BrakeNormalized = 0.0f;
		int32_t GearState = 0;
	};

	struct FTrailerState
	{
		FPose3D Pose;
		// TBD until trailer geometry is Approved (FR-VEH-004) — a real
		// "unavailable" state, not a zeroed struct silently treated as valid.
		TApprovedValue<FPose3D> KingpinPosition   = TApprovedValue<FPose3D>::Unavailable();
		TApprovedValue<FPose3D> AxlePosition      = TApprovedValue<FPose3D>::Unavailable();
		TApprovedValue<FPose3D> RearReferencePosition = TApprovedValue<FPose3D>::Unavailable();
	};

	enum class ECouplingHealth : uint8_t
	{
		Coupled,
		Uncoupled,
		Faulted
	};

	struct FCouplingState
	{
		TApprovedValue<double> ArticulationAngleRad = TApprovedValue<double>::Unavailable();
		TApprovedValue<double> ArticulationRateRadPerSec = TApprovedValue<double>::Unavailable();
		ECouplingHealth Health = ECouplingHealth::Uncoupled;
	};

	struct FVehicleState
	{
		FMonotonicTimestamp Timestamp = 0.0;
		FTractorState  Tractor;
		FTrailerState  Trailer;
		FCouplingState Coupling;
	};

	class IVehicleStateProvider
	{
	public:
		virtual ~IVehicleStateProvider() = default;

		// Read-only snapshot per tick. Implementations must refuse to spawn
		// into a *validated* exercise when required VehicleProfile/TrailerProfile
		// geometry (mass, steering range, kingpin geometry) is not Approved —
		// they may still spawn into a labeled-synthetic debug scene.
		virtual FVehicleState GetState() const = 0;
	};
}
