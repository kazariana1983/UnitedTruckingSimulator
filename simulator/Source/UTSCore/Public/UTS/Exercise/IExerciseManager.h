// IExerciseManager — exercise lifecycle over a purely data-driven definition.
//
// See docs/INTERFACE_CONTRACTS.md §5. An ExerciseDefinition is data; it must
// never be embedded in a map's level Blueprint or in a vehicle C++ class
// (FR-EXR-001). The same manager implementation and schema serve all four
// MVP maneuvers (FR-EXR-003) — maneuver differences live entirely in data.

#pragma once

#include <optional>
#include <vector>
#include "UTS/Common/UTSCommonTypes.h"
#include "UTS/Detection/IDetectionEventSink.h"

namespace UTS
{
	struct FPose3D; // fwd decl (Vehicle/IVehicleStateProvider.h)

	struct FBoundaryRegionRef { std::string RegionId; };
	struct FConeRef { std::string ConeId; };
	struct FCompletionConditionRef { std::string ConditionId; };

	// Pure data. Populated only from an Approved YardProfile/ExerciseDefinition
	// config snapshot — never authored by this module.
	struct FExerciseDefinition
	{
		FConfigVersionRef VersionRef;
		std::string StartPoseRef;
		std::string ResetPoseRef;
		std::string GoalRegionRef;
		std::vector<FBoundaryRegionRef> Boundaries;
		std::vector<FConeRef> Cones;
		std::vector<FCompletionConditionRef> CompletionConditions;
		std::optional<double> OptionalTimeLimitSeconds;
		FConfigVersionRef ScoringProfileRef;
	};

	enum class EExerciseLifecycleState : uint8_t
	{
		Unloaded,
		Loaded,
		AwaitingStart,
		InProgress,
		Completing,
		Completed,
		Aborted
	};

	struct FObjectiveState
	{
		EExerciseLifecycleState LifecycleState = EExerciseLifecycleState::Unloaded;
		std::string ShortStatusMessage; // for the minimal HUD panel, MVP doc §6 — no timer/score leakage unless configured
	};

	struct FLoadExerciseResult
	{
		bool bSucceeded = false;
		FExerciseDefinition Definition;                 // valid only if bSucceeded
		std::vector<FMissingFieldDiagnostic> MissingFields; // when a referenced yard/geometry field isn't Approved
	};

	enum class EResetPolicy : uint8_t; // see Session/ISessionManager.h — same enum, exercise manager only executes it

	class IExerciseManager
	{
	public:
		virtual ~IExerciseManager() = default;

		// Fails with the missing field(s) named when the definition references
		// yard geometry not yet Approved (FR-EXR-006) — exercise stays
		// unavailable in Exercise Selection (S04) rather than loading guessed geometry.
		virtual FLoadExerciseResult LoadExercise(const FProfileId& ExerciseDefinitionId) = 0;

		// Subscribed to Detection's event stream; drives lifecycle transitions
		// (e.g. CompletionConfirmed -> Completing) but never itself decides
		// pass/fail — that is Scoring's job (NFR-002).
		virtual void OnDomainEvent(const FDomainEvent& Event) = 0;

		virtual void RequestReset(EResetPolicy Policy) = 0;
		virtual void RequestStart() = 0;

		virtual FObjectiveState GetObjectiveState() const = 0;
	};
}
