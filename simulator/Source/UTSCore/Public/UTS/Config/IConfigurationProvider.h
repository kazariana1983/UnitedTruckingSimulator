// IConfigurationProvider — versioned, human-approved configuration profiles.
//
// See docs/INTERFACE_CONTRACTS.md §2. Leaf module: depends on nothing else
// in UTSCore. Every other module reads configuration only through this
// interface — none may read a profile file directly (docs/TECHNICAL_ARCHITECTURE.md §4).

#pragma once

#include <string>
#include <vector>
#include "UTS/Common/UTSCommonTypes.h"

namespace UTS
{
	enum class EProfileType : uint8_t
	{
		VehicleProfile,
		TrailerProfile,
		YardProfile,
		ExerciseDefinition,
		DeviceProfile,
		CalibrationProfile,
		ScoringProfile,
		TelemetryProfile
	};

	enum class EConfigError : uint8_t
	{
		None,
		LoadFailed,         // file missing/corrupt — never synthesized as a default
		VersionNotFound,
		Retired             // explicitly requested a retired version
	};

	// Immutable for the caller's lifetime. Opaque payload pointer/blob is left
	// to the concrete implementation (UUTSConfigurationSubsystem); this
	// interface only guarantees the provenance envelope every profile type
	// must carry.
	struct FConfigSnapshotEnvelope
	{
		EProfileType      ProfileType = EProfileType::VehicleProfile;
		FConfigProvenance Provenance;
	};

	struct FConfigLoadResult
	{
		bool                    bSucceeded = false;
		FConfigSnapshotEnvelope Envelope;   // valid only if bSucceeded
		EConfigError            Error = EConfigError::None;
	};

	// Everything a validated exercise launch needs, or the full list of what
	// is missing — never a partial silent success (FR-EXR-006, SRS §8).
	struct FValidatedConfigSet
	{
		bool                                  bReadyForValidatedLaunch = false;
		std::vector<FConfigSnapshotEnvelope>  ResolvedProfiles;   // when ready
		std::vector<FMissingFieldDiagnostic>  MissingFields;      // when not ready
	};

	class IConfigurationProvider
	{
	public:
		virtual ~IConfigurationProvider() = default;

		// RequestedVersion == 0 means "latest Approved".
		virtual FConfigLoadResult LoadProfile(
			EProfileType Type,
			const FProfileId& ProfileId,
			uint32_t RequestedVersion) = 0;

		// The single gate an exercise start must pass before a *validated*
		// (non-synthetic) attempt may begin.
		virtual FValidatedConfigSet ValidateForLaunch(const FProfileId& ExerciseId) = 0;

		virtual EApprovalState GetApprovalState(
			const FProfileId& ProfileId, uint32_t Version) const = 0;
	};
}
