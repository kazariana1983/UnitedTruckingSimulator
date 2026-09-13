// IScoringEngine — deterministic, pure function of (events, profile).
//
// See docs/INTERFACE_CONTRACTS.md §8. Purity constraint (NFR-001,
// FR-SCR-001/005): no LLM call, no random input, no live-subsystem read.
// This header/module must have zero dependency on any LLM/network library —
// that absence is itself part of the contract, enforced by a dependency-lint
// test, not just this comment (docs/REQUIREMENT_TRACEABILITY_MATRIX.md FR-SCR-001).
//
// Until ground_truth/SIMULATOR_GROUND_TRUTH.md §Scoring is populated and
// approved, only test-only synthetic profiles may be used, and only in
// automated tests (FR-SCR-007) — never presented to a student as a real result.

#pragma once

#include <string>
#include <vector>
#include "UTS/Common/UTSCommonTypes.h"
#include "UTS/Detection/IDetectionEventSink.h"

namespace UTS
{
	enum class EScoringProfileMode : uint8_t
	{
		Practice,
		SchoolEvaluation,
		JurisdictionSpecific,
		InstructorDefined
	};

	// Opaque rule payload is left to the concrete implementation; this
	// interface guarantees the mode + provenance envelope every profile
	// carries. Only a profile whose mode+jurisdiction has been explicitly
	// human-validated may be labeled "official" anywhere in UI/API (FR-SCR-006).
	struct FScoringProfileSnapshot
	{
		FConfigVersionRef   VersionRef;
		EScoringProfileMode Mode = EScoringProfileMode::Practice;
		FConfigProvenance   Provenance;
	};

	struct FScoreRuleTrace
	{
		FMonotonicTimestamp Timestamp = 0.0;
		EDomainEventType EventType = EDomainEventType::AttemptStarted;
		std::string Severity;
		double MeasuredValue = 0.0;
		double Threshold = 0.0;
		double Penalty = 0.0;
		std::string Explanation;
		std::string EvidenceRef; // ties back to FEventEvidence on the source FDomainEvent
	};

	enum class EScoreOutcome : uint8_t
	{
		Scored,
		Unscorable   // missing/corrupt required event data — never a guessed score (FR-SCR-004, MVP doc §13)
	};

	struct FScoreResult
	{
		FAttemptId Attempt;
		FProfileId ProfileId;
		uint32_t   ProfileVersion = 0;
		EScoreOutcome Outcome = EScoreOutcome::Unscorable;
		std::string UnscorableReason; // populated only when Outcome == Unscorable
		std::vector<FScoreRuleTrace> RuleTrace; // ordered, suitable for instructor review and automated golden-replay tests
	};

	class IScoringEngine
	{
	public:
		virtual ~IScoringEngine() = default;

		// Same ordered Events + same Profile version MUST produce a
		// byte-identical FScoreResult, always (FR-SCR-005). This is the
		// golden-replay contract every scoring-code change is tested against.
		virtual FScoreResult ScoreAttempt(
			const std::vector<FDomainEvent>& OrderedEvents,
			const FScoringProfileSnapshot& Profile) const = 0;
	};
}
