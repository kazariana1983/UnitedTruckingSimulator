// P2-01 configuration implementation support.
//
// This file is the deterministic, headless-testable configuration core used by
// the Unreal UUTSConfigurationSubsystem adapter. It intentionally keeps
// synthetic practice data separate from human approval: synthetic profiles may
// be loaded for explicitly labeled practice/test flows, but they are never
// accepted by IConfigurationProvider::ValidateForLaunch, which is the gate for
// validated attempts.

#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "UTS/Config/IConfigurationProvider.h"

#ifndef UTSCORE_API
#define UTSCORE_API
#endif

namespace UTS
{
	enum class EProfileSourceAuthority : uint8_t
	{
		GroundTruth,
		SyntheticPracticeOnly
	};

	enum class ELaunchValidationMode : uint8_t
	{
		Validated,
		SyntheticPracticeOnly
	};

	struct FProfileReference
	{
		EProfileType Type = EProfileType::VehicleProfile;
		FProfileId ProfileId;
		uint32_t Version = 0;
	};

	struct FConfigurationProfileSnapshot
	{
		FConfigSnapshotEnvelope Envelope;
		std::string ProfileSetId;
		std::string DisplayName;
		std::string CreatorId;
		std::string CreatedAtUtcIso8601;
		std::string ValidationNotes;
		std::string UnitsPolicy;
		EProfileSourceAuthority SourceAuthority = EProfileSourceAuthority::GroundTruth;
		std::map<std::string, std::string> Fields;
		std::vector<FProfileReference> Dependencies;
	};

	struct FProfileSetLoadResult
	{
		bool bSucceeded = false;
		std::vector<std::filesystem::path> LoadedFiles;
		std::vector<std::string> Errors;
		std::size_t ProfileCount = 0;
	};

	struct FApprovalStateQueryResult
	{
		bool bFound = false;
		EApprovalState ApprovalState = EApprovalState::Draft;
		EProfileSourceAuthority SourceAuthority = EProfileSourceAuthority::GroundTruth;
	};

	struct FConfigurationModeValidationResult
	{
		ELaunchValidationMode Mode = ELaunchValidationMode::Validated;
		bool bReadyForRequestedMode = false;
		std::vector<FConfigSnapshotEnvelope> ResolvedProfiles;
		std::vector<FMissingFieldDiagnostic> Diagnostics;
	};

	UTSCORE_API std::string ToString(EProfileType Type);
	UTSCORE_API std::string ToString(EApprovalState State);
	UTSCORE_API std::string ToString(EProfileSourceAuthority Authority);
	UTSCORE_API std::optional<EProfileType> TryParseProfileType(const std::string& Text);
	UTSCORE_API std::optional<EApprovalState> TryParseApprovalState(const std::string& Text);
	UTSCORE_API std::optional<EProfileSourceAuthority> TryParseProfileSourceAuthority(const std::string& Text);

	// Conservative schema owned by code, not by profile payloads. This prevents a
	// profile file from declaring itself complete by omitting required fields.
	UTSCORE_API const std::vector<std::string>& GetRequiredFieldSchema(EProfileType Type);

	class UTSCORE_API FFileConfigurationProvider final : public IConfigurationProvider
	{
	public:
		FFileConfigurationProvider() = default;
		explicit FFileConfigurationProvider(std::filesystem::path ProfileRoot);

		FProfileSetLoadResult LoadFromDirectory(const std::filesystem::path& ProfileRoot);
		void Clear();

		FConfigLoadResult LoadProfile(
			EProfileType Type,
			const FProfileId& ProfileId,
			uint32_t RequestedVersion) override;

		FValidatedConfigSet ValidateForLaunch(const FProfileId& ExerciseId) override;

		FConfigurationModeValidationResult ValidateForMode(
			const FProfileId& ExerciseId,
			ELaunchValidationMode Mode) const;

		EApprovalState GetApprovalState(
			const FProfileId& ProfileId,
			uint32_t Version) const override;

		FApprovalStateQueryResult GetApprovalStateDetailed(
			const FProfileId& ProfileId,
			uint32_t Version) const;

		std::optional<FConfigurationProfileSnapshot> GetProfileSnapshot(
			EProfileType Type,
			const FProfileId& ProfileId,
			uint32_t Version) const;

		std::vector<FConfigurationProfileSnapshot> GetAllSnapshots() const;
		std::size_t GetProfileCount() const;

	private:
		std::vector<FConfigurationProfileSnapshot> Profiles;
		std::filesystem::path LastLoadedRoot;

		const FConfigurationProfileSnapshot* FindProfile(
			EProfileType Type,
			const FProfileId& ProfileId,
			uint32_t Version) const;

		const FConfigurationProfileSnapshot* FindLatestAny(
			EProfileType Type,
			const FProfileId& ProfileId) const;

		const FConfigurationProfileSnapshot* FindLatestValidatedApproved(
			EProfileType Type,
			const FProfileId& ProfileId) const;

		void ValidateProfileForMode(
			const FConfigurationProfileSnapshot& Profile,
			ELaunchValidationMode Mode,
			std::vector<FMissingFieldDiagnostic>& OutDiagnostics) const;

		void AddProfileClosure(
			const FConfigurationProfileSnapshot& Profile,
			ELaunchValidationMode Mode,
			std::set<std::string>& SeenKeys,
			std::vector<FConfigSnapshotEnvelope>& OutResolvedProfiles,
			std::vector<FMissingFieldDiagnostic>& OutDiagnostics) const;

		void ValidateRequiredExerciseDependencies(
			const FConfigurationProfileSnapshot& Exercise,
			std::vector<FMissingFieldDiagnostic>& OutDiagnostics) const;
	};
}
