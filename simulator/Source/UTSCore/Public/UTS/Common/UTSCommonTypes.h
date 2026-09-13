// United Trucking Simulator — UTSCore shared types.
//
// Architecture-phase artifact. See docs/INTERFACE_CONTRACTS.md and
// docs/TECHNICAL_ARCHITECTURE.md for the narrative contract this header
// implements in code form.
//
// Rules enforced by this file's design (do not violate when extending it):
//  - No field here carries a plausible real-world default. Every physical
//    or business value flows in from an approved configuration profile;
//    this header defines *shape*, never *value*.
//  - No dependency on rendering, Slate/UMG, or networking modules: UTSCore
//    must stay headless-testable (see docs/ARCHITECTURE_OPEN_DECISIONS.md B6).
//
// TBD values (per ground_truth/SIMULATOR_GROUND_TRUTH.md) are represented
// by real discriminated "unavailable" states below, never by 0/-1/null
// silently treated as a value.

#pragma once

#include <cstdint>
#include <string>

namespace UTS
{
	// Opaque, stable, serializable identifiers shared by every module and by
	// the backend schema. Deliberately not typedef'd to a bare int/string so
	// a StationId can never be passed where an AttemptId is expected.
#define UTS_DECLARE_OPAQUE_ID(Name)                                          \
	struct Name                                                              \
	{                                                                        \
		std::string Value;                                                   \
		bool operator==(const Name& Other) const { return Value == Other.Value; } \
		bool operator!=(const Name& Other) const { return !(*this == Other); }    \
		bool IsValid() const { return !Value.empty(); }                      \
	};

	UTS_DECLARE_OPAQUE_ID(FAttemptId)
	UTS_DECLARE_OPAQUE_ID(FStationId)
	UTS_DECLARE_OPAQUE_ID(FStudentId)
	UTS_DECLARE_OPAQUE_ID(FInstructorId)
	UTS_DECLARE_OPAQUE_ID(FSessionId)
	UTS_DECLARE_OPAQUE_ID(FProfileId)

#undef UTS_DECLARE_OPAQUE_ID

	// Monotonic timestamp used across telemetry/events/scoring so ordering is
	// well-defined independent of wall-clock discontinuities. Units: seconds
	// since session start (double precision). Wall-clock correlation, if
	// needed, travels alongside this field, never in place of it.
	using FMonotonicTimestamp = double;

	// A configuration profile version reference. Identity is {ProfileId,
	// Version} and is immutable once ApprovalState == Approved: a change is
	// always a new Version, never a mutation in place (docs/TECHNICAL_ARCHITECTURE.md §4).
	struct FConfigVersionRef
	{
		FProfileId ProfileId;
		uint32_t   Version = 0;
	};

	// Lifecycle of every vehicle/trailer/yard/exercise/device/scoring profile.
	// Only Approved profiles may back a *validated* attempt. Draft profiles
	// may back a labeled-practice/synthetic attempt only.
	enum class EApprovalState : uint8_t
	{
		Draft,
		Approved,
		Retired
	};

	// Returned by IConfigurationProvider::ValidateForLaunch and similar
	// validation calls: names exactly which field is missing/unapproved
	// rather than failing generically. Never silently substituted with a
	// guessed value (Agent Operating Rules #4/#5).
	struct FMissingFieldDiagnostic
	{
		std::string FieldPath;   // e.g. "TrailerProfile.KingpinToRearGeometry"
		std::string Reason;      // e.g. "TBD in ground truth; no Approved version exists"
	};

	// Full provenance a config snapshot must carry (NFR-010).
	struct FConfigProvenance
	{
		FConfigVersionRef VersionRef;
		EApprovalState    ApprovalState = EApprovalState::Draft;
		std::string       GroundTruthSourceRef; // ground_truth revision, or
		                                         // "synthetic — not for validated use"
		std::string       ApproverId;           // empty when ApprovalState != Approved
		std::string       ApprovedAtUtcIso8601; // empty when ApprovalState != Approved
	};

	// A physical/measured value that may not yet be approved. Consumers must
	// branch on HasValue() rather than reading Value when false — this is
	// what makes "Unavailable" a real state instead of a convention.
	template <typename T>
	struct TApprovedValue
	{
		bool HasValue() const { return bHasValue; }
		const T& Get() const { return Value; } // caller must check HasValue() first

		static TApprovedValue Unavailable() { return TApprovedValue{}; }
		static TApprovedValue Of(const T& InValue)
		{
			TApprovedValue Result;
			Result.bHasValue = true;
			Result.Value = InValue;
			return Result;
		}

	private:
		bool bHasValue = false;
		T Value{};
	};
}
