#include "UTS/Config/ConfigurationStore.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace UTS
{
	namespace
	{
		struct FParsedSection
		{
			std::string ProfileSetId;
			std::optional<FConfigurationProfileSnapshot> CurrentProfile;
		};

		std::string Trim(const std::string& Text)
		{
			auto Begin = Text.begin();
			while (Begin != Text.end() && std::isspace(static_cast<unsigned char>(*Begin)))
			{
				++Begin;
			}

			auto End = Text.end();
			while (End != Begin && std::isspace(static_cast<unsigned char>(*(End - 1))))
			{
				--End;
			}

			return std::string(Begin, End);
		}

		std::string ToLower(std::string Text)
		{
			std::transform(Text.begin(), Text.end(), Text.begin(), [](unsigned char C) {
				return static_cast<char>(std::tolower(C));
			});
			return Text;
		}

		bool StartsWith(const std::string& Text, const std::string& Prefix)
		{
			return Text.size() >= Prefix.size() && Text.compare(0, Prefix.size(), Prefix) == 0;
		}

		std::vector<std::string> Split(const std::string& Text, char Delimiter)
		{
			std::vector<std::string> Parts;
			std::stringstream Stream(Text);
			std::string Part;
			while (std::getline(Stream, Part, Delimiter))
			{
				Parts.push_back(Trim(Part));
			}
			return Parts;
		}

		bool IsMissingConfigValue(const std::string& Value)
		{
			const std::string Normalized = ToLower(Trim(Value));
			return Normalized.empty() || Normalized == "tbd" || Normalized == "todo" || Normalized == "unknown";
		}

		std::string ProfileKey(EProfileType Type, const FProfileId& ProfileId, uint32_t Version)
		{
			return ToString(Type) + ":" + ProfileId.Value + ":" + std::to_string(Version);
		}

		std::string DescribeProfile(const FConfigurationProfileSnapshot& Profile)
		{
			return ToString(Profile.Envelope.ProfileType) + "[" +
				Profile.Envelope.Provenance.VersionRef.ProfileId.Value + ":v" +
				std::to_string(Profile.Envelope.Provenance.VersionRef.Version) + "]";
		}

		std::string FieldPathFor(const FConfigurationProfileSnapshot& Profile, const std::string& Field)
		{
			return DescribeProfile(Profile) + ".field." + Field;
		}

		bool ContainsTbdOrSynthetic(const std::string& Value)
		{
			const std::string Normalized = ToLower(Value);
			return Normalized.find("tbd") != std::string::npos ||
				Normalized.find("synthetic") != std::string::npos;
		}

		bool TryParsePositiveNumber(const std::string& Text)
		{
			try
			{
				std::size_t Parsed = 0;
				const double Value = std::stod(Text, &Parsed);
				return Parsed == Trim(Text).size() && std::isfinite(Value) && Value > 0.0;
			}
			catch (const std::exception&)
			{
				return false;
			}
		}

		bool TryParsePositiveInteger(const std::string& Text)
		{
			try
			{
				std::size_t Parsed = 0;
				const unsigned long Value = std::stoul(Text, &Parsed, 10);
				return Parsed == Trim(Text).size() && Value > 0;
			}
			catch (const std::exception&)
			{
				return false;
			}
		}

		bool TryParsePositiveDimensionPair(const std::string& Text)
		{
			const std::vector<std::string> Parts = Split(Text, 'x');
			return Parts.size() == 2 && TryParsePositiveNumber(Parts[0]) && TryParsePositiveNumber(Parts[1]);
		}

		bool EndsWith(const std::string& Text, const std::string& Suffix)
		{
			return Text.size() >= Suffix.size() &&
				Text.compare(Text.size() - Suffix.size(), Suffix.size(), Suffix) == 0;
		}

		std::optional<std::string> GetTypeValidationError(const std::string& Field, const std::string& Value)
		{
			if (EndsWith(Field, "DimensionsMeters"))
			{
				return TryParsePositiveDimensionPair(Value)
					? std::nullopt
					: std::optional<std::string>("Expected positive meter dimensions formatted as <number>x<number>");
			}

			if (EndsWith(Field, "Meters") ||
				EndsWith(Field, "Degrees") ||
				EndsWith(Field, "Kilograms") ||
				EndsWith(Field, "Hz") ||
				Field == "Calibration.Center" ||
				Field == "Calibration.Range" ||
				Field == "Calibration.DeadZone")
			{
				return TryParsePositiveNumber(Value)
					? std::nullopt
					: std::optional<std::string>("Expected a positive numeric value with units implied by the field name");
			}

			if (EndsWith(Field, "SchemaVersion"))
			{
				return TryParsePositiveInteger(Value)
					? std::nullopt
					: std::optional<std::string>("Expected a positive integer schema version");
			}

			return std::nullopt;
		}

		std::optional<uint32_t> TryParseVersion(const std::string& Text)
		{
			try
			{
				std::size_t Parsed = 0;
				const unsigned long Value = std::stoul(Text, &Parsed, 10);
				if (Parsed != Text.size())
				{
					return std::nullopt;
				}
				return static_cast<uint32_t>(Value);
			}
			catch (const std::exception&)
			{
				return std::nullopt;
			}
		}

		std::optional<FProfileReference> TryParseProfileReference(EProfileType Type, const std::string& Text)
		{
			const std::vector<std::string> Parts = Split(Text, ':');
			if (Parts.size() != 2)
			{
				return std::nullopt;
			}

			std::optional<uint32_t> Version = TryParseVersion(Parts[1]);
			if (!Version.has_value())
			{
				return std::nullopt;
			}

			FProfileReference Reference;
			Reference.Type = Type;
			Reference.ProfileId.Value = Parts[0];
			Reference.Version = Version.value();
			return Reference;
		}

		std::optional<FConfigurationProfileSnapshot> TryStartProfileSection(
			const std::string& SectionName,
			const std::string& ProfileSetId,
			std::vector<std::string>& Errors,
			const std::filesystem::path& FilePath,
			std::size_t LineNumber)
		{
			const std::vector<std::string> Parts = Split(SectionName, ':');
			if (Parts.size() != 4 || Parts[0] != "Profile")
			{
				Errors.push_back(FilePath.string() + ":" + std::to_string(LineNumber) + ": malformed profile section");
				return std::nullopt;
			}

			const std::optional<EProfileType> Type = TryParseProfileType(Parts[1]);
			const std::optional<uint32_t> Version = TryParseVersion(Parts[3]);
			if (!Type.has_value() || !Version.has_value() || Parts[2].empty())
			{
				Errors.push_back(FilePath.string() + ":" + std::to_string(LineNumber) +
					": invalid profile section '" + SectionName + "'");
				return std::nullopt;
			}

			FConfigurationProfileSnapshot Snapshot;
			Snapshot.ProfileSetId = ProfileSetId;
			Snapshot.Envelope.ProfileType = Type.value();
			Snapshot.Envelope.Provenance.VersionRef.ProfileId.Value = Parts[2];
			Snapshot.Envelope.Provenance.VersionRef.Version = Version.value();
			return Snapshot;
		}

		void FinalizeProfile(
			std::optional<FConfigurationProfileSnapshot>& CurrentProfile,
			std::vector<FConfigurationProfileSnapshot>& OutProfiles)
		{
			if (!CurrentProfile.has_value())
			{
				return;
			}

			OutProfiles.push_back(CurrentProfile.value());
			CurrentProfile.reset();
		}

		void ApplyProfileKeyValue(
			FConfigurationProfileSnapshot& Profile,
			const std::string& Key,
			const std::string& Value,
			std::vector<std::string>& Errors,
			const std::filesystem::path& FilePath,
			std::size_t LineNumber)
		{
			if (Key == "display_name")
			{
				Profile.DisplayName = Value;
				return;
			}
			if (Key == "creator_id")
			{
				Profile.CreatorId = Value;
				return;
			}
			if (Key == "created_at_utc")
			{
				Profile.CreatedAtUtcIso8601 = Value;
				return;
			}
			if (Key == "validation_notes")
			{
				Profile.ValidationNotes = Value;
				return;
			}
			if (Key == "units_policy")
			{
				Profile.UnitsPolicy = Value;
				return;
			}
			if (Key == "approval_state")
			{
				const std::optional<EApprovalState> Parsed = TryParseApprovalState(Value);
				if (!Parsed.has_value())
				{
					Errors.push_back(FilePath.string() + ":" + std::to_string(LineNumber) +
						": invalid approval_state '" + Value + "'");
					return;
				}
				Profile.Envelope.Provenance.ApprovalState = Parsed.value();
				return;
			}
			if (Key == "source_authority")
			{
				const std::optional<EProfileSourceAuthority> Parsed = TryParseProfileSourceAuthority(Value);
				if (!Parsed.has_value())
				{
					Errors.push_back(FilePath.string() + ":" + std::to_string(LineNumber) +
						": invalid source_authority '" + Value + "'");
					return;
				}
				Profile.SourceAuthority = Parsed.value();
				return;
			}
			if (Key == "source_ref")
			{
				Profile.Envelope.Provenance.GroundTruthSourceRef = Value;
				return;
			}
			if (Key == "approver_id")
			{
				Profile.Envelope.Provenance.ApproverId = Value;
				return;
			}
			if (Key == "approved_at_utc")
			{
				Profile.Envelope.Provenance.ApprovedAtUtcIso8601 = Value;
				return;
			}
			if (StartsWith(Key, "field."))
			{
				Profile.Fields[Key.substr(std::string("field.").size())] = Value;
				return;
			}
			if (StartsWith(Key, "depends."))
			{
				const std::string TypeText = Key.substr(std::string("depends.").size());
				const std::optional<EProfileType> Type = TryParseProfileType(TypeText);
				if (!Type.has_value())
				{
					Errors.push_back(FilePath.string() + ":" + std::to_string(LineNumber) +
						": invalid dependency profile type '" + TypeText + "'");
					return;
				}

				const std::optional<FProfileReference> Reference = TryParseProfileReference(Type.value(), Value);
				if (!Reference.has_value())
				{
					Errors.push_back(FilePath.string() + ":" + std::to_string(LineNumber) +
						": invalid dependency reference '" + Value + "'");
					return;
				}

				Profile.Dependencies.push_back(Reference.value());
				return;
			}

			Errors.push_back(FilePath.string() + ":" + std::to_string(LineNumber) +
				": unknown key '" + Key + "'");
		}

		void ParseProfileFile(
			const std::filesystem::path& FilePath,
			std::vector<FConfigurationProfileSnapshot>& OutProfiles,
			std::vector<std::string>& Errors)
		{
			std::ifstream Input(FilePath);
			if (!Input)
			{
				Errors.push_back("Unable to open profile file: " + FilePath.string());
				return;
			}

			std::string ProfileSetId;
			std::optional<FConfigurationProfileSnapshot> CurrentProfile;
			std::string CurrentSection;
			std::string Line;
			std::size_t LineNumber = 0;
			std::set<std::string> SectionKeys;
			while (std::getline(Input, Line))
			{
				++LineNumber;
				std::string Clean = Trim(Line);
				if (Clean.empty() || StartsWith(Clean, "#") || StartsWith(Clean, ";"))
				{
					continue;
				}

				if (Clean.front() == '[' && Clean.back() == ']')
				{
					FinalizeProfile(CurrentProfile, OutProfiles);
					CurrentSection = Clean.substr(1, Clean.size() - 2);
					SectionKeys.clear();
					if (StartsWith(CurrentSection, "Profile:"))
					{
						CurrentProfile = TryStartProfileSection(CurrentSection, ProfileSetId, Errors, FilePath, LineNumber);
					}
					else if (CurrentSection != "ProfileSet")
					{
						Errors.push_back(FilePath.string() + ": unknown section '" + CurrentSection + "'");
					}
					continue;
				}

				const std::size_t Equals = Clean.find('=');
				if (Equals == std::string::npos)
				{
					Errors.push_back(FilePath.string() + ":" + std::to_string(LineNumber) +
						": expected key=value");
					continue;
				}

				const std::string Key = Trim(Clean.substr(0, Equals));
				const std::string Value = Trim(Clean.substr(Equals + 1));
				if (!SectionKeys.insert(Key).second)
				{
					Errors.push_back(FilePath.string() + ": duplicate key '" + Key + "'");
					continue;
				}

				if (CurrentSection == "ProfileSet")
				{
					if (Key == "id")
					{
						ProfileSetId = Value;
					}
					continue;
				}

				if (CurrentProfile.has_value())
				{
					ApplyProfileKeyValue(CurrentProfile.value(), Key, Value, Errors, FilePath, LineNumber);
				}
				else
				{
					Errors.push_back(FilePath.string() + ": key outside a valid section");
				}
			}

			if (Input.bad()) Errors.push_back("Failed while reading profile file: " + FilePath.string());
			FinalizeProfile(CurrentProfile, OutProfiles);
		}
	}

	std::string ToString(EProfileType Type)
	{
		switch (Type)
		{
		case EProfileType::VehicleProfile:
			return "VehicleProfile";
		case EProfileType::TrailerProfile:
			return "TrailerProfile";
		case EProfileType::YardProfile:
			return "YardProfile";
		case EProfileType::ExerciseDefinition:
			return "ExerciseDefinition";
		case EProfileType::DeviceProfile:
			return "DeviceProfile";
		case EProfileType::CalibrationProfile:
			return "CalibrationProfile";
		case EProfileType::ScoringProfile:
			return "ScoringProfile";
		case EProfileType::TelemetryProfile:
			return "TelemetryProfile";
		}
		return "UnknownProfileType";
	}

	std::string ToString(EApprovalState State)
	{
		switch (State)
		{
		case EApprovalState::Draft:
			return "Draft";
		case EApprovalState::Approved:
			return "Approved";
		case EApprovalState::Retired:
			return "Retired";
		}
		return "UnknownApprovalState";
	}

	std::string ToString(EProfileSourceAuthority Authority)
	{
		switch (Authority)
		{
		case EProfileSourceAuthority::GroundTruth:
			return "GroundTruth";
		case EProfileSourceAuthority::SyntheticPracticeOnly:
			return "SyntheticPracticeOnly";
		}
		return "UnknownProfileSourceAuthority";
	}

	std::optional<EProfileType> TryParseProfileType(const std::string& Text)
	{
		const std::string Normalized = ToLower(Trim(Text));
		if (Normalized == "vehicleprofile") return EProfileType::VehicleProfile;
		if (Normalized == "trailerprofile") return EProfileType::TrailerProfile;
		if (Normalized == "yardprofile") return EProfileType::YardProfile;
		if (Normalized == "exercisedefinition") return EProfileType::ExerciseDefinition;
		if (Normalized == "deviceprofile") return EProfileType::DeviceProfile;
		if (Normalized == "calibrationprofile") return EProfileType::CalibrationProfile;
		if (Normalized == "scoringprofile") return EProfileType::ScoringProfile;
		if (Normalized == "telemetryprofile") return EProfileType::TelemetryProfile;
		return std::nullopt;
	}

	std::optional<EApprovalState> TryParseApprovalState(const std::string& Text)
	{
		const std::string Normalized = ToLower(Trim(Text));
		if (Normalized == "draft") return EApprovalState::Draft;
		if (Normalized == "approved") return EApprovalState::Approved;
		if (Normalized == "retired") return EApprovalState::Retired;
		return std::nullopt;
	}

	std::optional<EProfileSourceAuthority> TryParseProfileSourceAuthority(const std::string& Text)
	{
		const std::string Normalized = ToLower(Trim(Text));
		if (Normalized == "groundtruth") return EProfileSourceAuthority::GroundTruth;
		if (Normalized == "syntheticpracticeonly") return EProfileSourceAuthority::SyntheticPracticeOnly;
		return std::nullopt;
	}

	const std::vector<std::string>& GetRequiredFieldSchema(EProfileType Type)
	{
		static const std::vector<std::string> VehicleFields = {
			"Vehicle.MakeModel",
			"Vehicle.WheelbaseMeters",
			"Vehicle.OverallLengthMeters",
			"Vehicle.SteeringRangeDegrees",
			"Vehicle.OperatingMassKilograms",
			"Vehicle.TandemConfiguration"
		};
		static const std::vector<std::string> TrailerFields = {
			"Trailer.Type",
			"Trailer.KingpinToRearGeometryMeters",
			"Trailer.TandemPositionMeters",
			"Trailer.MassConfigurationKilograms"
		};
		static const std::vector<std::string> YardFields = {
			"Yard.StraightLineBackingDimensionsMeters",
			"Yard.OffsetBackingDimensionsMeters",
			"Yard.AlleyDockDimensionsMeters",
			"Yard.ConeSpacingMeters"
		};
		static const std::vector<std::string> ExerciseFields = {
			"Exercise.ManeuverType",
			"Exercise.StartPoseRef",
			"Exercise.ResetPoseRef",
			"Exercise.GoalRegionRef",
			"Exercise.BoundarySetRef",
			"Exercise.ScoringProfileRef"
		};
		static const std::vector<std::string> DeviceFields = {
			"Hardware.SteeringWheel",
			"Hardware.Pedals",
			"Hardware.Shifter",
			"Hardware.DisplayConfiguration"
		};
		static const std::vector<std::string> CalibrationFields = {
			"Calibration.Center",
			"Calibration.Range",
			"Calibration.DeadZone",
			"Calibration.ResponseCurve"
		};
		static const std::vector<std::string> ScoringFields = {
			"Scoring.PracticeRules",
			"Scoring.SchoolEvaluationRules",
			"Scoring.JurisdictionSpecificRules"
		};
		static const std::vector<std::string> TelemetryFields = {
			"Telemetry.SampleRateHz",
			"Telemetry.RetentionPolicy",
			"Telemetry.SchemaVersion"
		};

		switch (Type)
		{
		case EProfileType::VehicleProfile:
			return VehicleFields;
		case EProfileType::TrailerProfile:
			return TrailerFields;
		case EProfileType::YardProfile:
			return YardFields;
		case EProfileType::ExerciseDefinition:
			return ExerciseFields;
		case EProfileType::DeviceProfile:
			return DeviceFields;
		case EProfileType::CalibrationProfile:
			return CalibrationFields;
		case EProfileType::ScoringProfile:
			return ScoringFields;
		case EProfileType::TelemetryProfile:
			return TelemetryFields;
		}
		return VehicleFields;
	}

	FFileConfigurationProvider::FFileConfigurationProvider(std::filesystem::path ProfileRoot)
	{
		LoadFromDirectory(ProfileRoot);
	}

	FProfileSetLoadResult FFileConfigurationProvider::LoadFromDirectory(const std::filesystem::path& ProfileRoot)
	{
		Clear();
		LastLoadedRoot = ProfileRoot;

		FProfileSetLoadResult Result;
		std::vector<std::filesystem::path> Files;
		try
		{
			if (!std::filesystem::exists(ProfileRoot))
			{
				Result.Errors.push_back("Profile root does not exist: " + ProfileRoot.string());
				return Result;
			}

			for (const auto& Entry : std::filesystem::recursive_directory_iterator(ProfileRoot))
			{
				if (Entry.is_regular_file() && Entry.path().extension() == ".ini")
				{
					Files.push_back(Entry.path());
				}
			}
		}
		catch (const std::exception& Error)
		{
			Result.Errors.push_back(std::string("Failed to enumerate profile root: ") + Error.what());
			return Result;
		}

		std::sort(Files.begin(), Files.end());
		for (const std::filesystem::path& File : Files)
		{
			ParseProfileFile(File, Profiles, Result.Errors);
			Result.LoadedFiles.push_back(File);
		}

		std::set<std::string> Identities;
		for (const auto& Profile : Profiles)
		{
			const auto& Ref = Profile.Envelope.Provenance.VersionRef;
			if (!Identities.insert(ProfileKey(Profile.Envelope.ProfileType, Ref.ProfileId, Ref.Version)).second)
				Result.Errors.push_back("Duplicate profile version: " + DescribeProfile(Profile));
		}
		if (Profiles.empty() && Result.Errors.empty()) Result.Errors.push_back("No profiles found");
		// Never expose a partially parsed configuration after a failed reload.
		if (!Result.Errors.empty()) Profiles.clear();
		Result.ProfileCount = Profiles.size();
		Result.bSucceeded = Result.Errors.empty() && !Profiles.empty();
		return Result;
	}

	void FFileConfigurationProvider::Clear()
	{
		Profiles.clear();
	}

	FConfigLoadResult FFileConfigurationProvider::LoadProfile(
		EProfileType Type,
		const FProfileId& ProfileId,
		uint32_t RequestedVersion)
	{
		FConfigLoadResult Result;
		const FConfigurationProfileSnapshot* Profile = nullptr;
		if (RequestedVersion == 0)
		{
			Profile = FindLatestValidatedApproved(Type, ProfileId);
		}
		else
		{
			Profile = FindProfile(Type, ProfileId, RequestedVersion);
		}

		if (Profile == nullptr)
		{
			Result.Error = EConfigError::VersionNotFound;
			return Result;
		}

		Result.bSucceeded = true;
		Result.Envelope = Profile->Envelope;
		Result.Error = Profile->Envelope.Provenance.ApprovalState == EApprovalState::Retired
			? EConfigError::Retired
			: EConfigError::None;
		return Result;
	}

	FValidatedConfigSet FFileConfigurationProvider::ValidateForLaunch(const FProfileId& ExerciseId)
	{
		const FConfigurationModeValidationResult ModeResult = ValidateForMode(ExerciseId, ELaunchValidationMode::Validated);
		FValidatedConfigSet Result;
		Result.bReadyForValidatedLaunch = false;
		Result.ResolvedProfiles = ModeResult.ResolvedProfiles;
		Result.MissingFields = ModeResult.Diagnostics;
		return Result;
	}

	FConfigurationModeValidationResult FFileConfigurationProvider::ValidateForMode(
		const FProfileId& ExerciseId,
		ELaunchValidationMode Mode) const
	{
		FConfigurationModeValidationResult Result;
		Result.Mode = Mode;

		const FConfigurationProfileSnapshot* Exercise = FindLatestAny(EProfileType::ExerciseDefinition, ExerciseId);
		if (Exercise == nullptr)
		{
			Result.Diagnostics.push_back({
				"ExerciseDefinition[" + ExerciseId.Value + "]",
				"No exercise definition profile version exists"
			});
			return Result;
		}

		if (Mode == ELaunchValidationMode::Validated)
		{
			Result.Diagnostics.push_back({
				"Configuration.ValidatedLaunch",
				"Validated launch remains unavailable in P2-01 until typed approved schemas and real ground-truth verification are implemented"
			});
		}

		ValidateRequiredExerciseDependencies(*Exercise, Result.Diagnostics);

		std::set<std::string> SeenKeys;
		AddProfileClosure(*Exercise, Mode, SeenKeys, Result.ResolvedProfiles, Result.Diagnostics);
		Result.bReadyForRequestedMode = Result.Diagnostics.empty();
		return Result;
	}

	EApprovalState FFileConfigurationProvider::GetApprovalState(
		const FProfileId& ProfileId,
		uint32_t Version) const
	{
		const FApprovalStateQueryResult Detailed = GetApprovalStateDetailed(ProfileId, Version);
		// The published interface has no Unknown state. Fail closed by returning
		// Draft for unknown profile versions so callers never mistake missing data
		// for approval. Call GetApprovalStateDetailed when the distinction matters.
		return Detailed.bFound ? Detailed.ApprovalState : EApprovalState::Draft;
	}

	FApprovalStateQueryResult FFileConfigurationProvider::GetApprovalStateDetailed(
		const FProfileId& ProfileId,
		uint32_t Version) const
	{
		for (const FConfigurationProfileSnapshot& Profile : Profiles)
		{
			const FConfigVersionRef& Ref = Profile.Envelope.Provenance.VersionRef;
			if (Ref.ProfileId == ProfileId && Ref.Version == Version)
			{
				FApprovalStateQueryResult Result;
				Result.bFound = true;
				Result.ApprovalState = Profile.Envelope.Provenance.ApprovalState;
				Result.SourceAuthority = Profile.SourceAuthority;
				return Result;
			}
		}

		return {};
	}

	std::optional<FConfigurationProfileSnapshot> FFileConfigurationProvider::GetProfileSnapshot(
		EProfileType Type,
		const FProfileId& ProfileId,
		uint32_t Version) const
	{
		const FConfigurationProfileSnapshot* Profile = FindProfile(Type, ProfileId, Version);
		if (Profile == nullptr)
		{
			return std::nullopt;
		}
		return *Profile;
	}

	std::vector<FConfigurationProfileSnapshot> FFileConfigurationProvider::GetAllSnapshots() const
	{
		return Profiles;
	}

	std::size_t FFileConfigurationProvider::GetProfileCount() const
	{
		return Profiles.size();
	}

	const FConfigurationProfileSnapshot* FFileConfigurationProvider::FindProfile(
		EProfileType Type,
		const FProfileId& ProfileId,
		uint32_t Version) const
	{
		for (const FConfigurationProfileSnapshot& Profile : Profiles)
		{
			const FConfigVersionRef& Ref = Profile.Envelope.Provenance.VersionRef;
			if (Profile.Envelope.ProfileType == Type && Ref.ProfileId == ProfileId && Ref.Version == Version)
			{
				return &Profile;
			}
		}

		return nullptr;
	}

	const FConfigurationProfileSnapshot* FFileConfigurationProvider::FindLatestAny(
		EProfileType Type,
		const FProfileId& ProfileId) const
	{
		const FConfigurationProfileSnapshot* Latest = nullptr;
		for (const FConfigurationProfileSnapshot& Profile : Profiles)
		{
			const FConfigVersionRef& Ref = Profile.Envelope.Provenance.VersionRef;
			if (Profile.Envelope.ProfileType != Type || Ref.ProfileId != ProfileId)
			{
				continue;
			}

			if (Latest == nullptr || Ref.Version > Latest->Envelope.Provenance.VersionRef.Version)
			{
				Latest = &Profile;
			}
		}

		return Latest;
	}

	const FConfigurationProfileSnapshot* FFileConfigurationProvider::FindLatestValidatedApproved(
		EProfileType Type,
		const FProfileId& ProfileId) const
	{
		const FConfigurationProfileSnapshot* Latest = nullptr;
		for (const FConfigurationProfileSnapshot& Profile : Profiles)
		{
			const FConfigVersionRef& Ref = Profile.Envelope.Provenance.VersionRef;
			if (Profile.Envelope.ProfileType != Type || Ref.ProfileId != ProfileId)
			{
				continue;
			}
			if (Profile.Envelope.Provenance.ApprovalState != EApprovalState::Approved)
			{
				continue;
			}
			if (Profile.SourceAuthority != EProfileSourceAuthority::GroundTruth)
			{
				continue;
			}
			if (ContainsTbdOrSynthetic(Profile.Envelope.Provenance.GroundTruthSourceRef))
			{
				continue;
			}

			if (Latest == nullptr || Ref.Version > Latest->Envelope.Provenance.VersionRef.Version)
			{
				Latest = &Profile;
			}
		}

		return Latest;
	}

	void FFileConfigurationProvider::ValidateProfileForMode(
		const FConfigurationProfileSnapshot& Profile,
		ELaunchValidationMode Mode,
		std::vector<FMissingFieldDiagnostic>& OutDiagnostics) const
	{
		const std::string ProfilePath = DescribeProfile(Profile);

		if (Profile.CreatorId.empty())
		{
			OutDiagnostics.push_back({
				ProfilePath + ".CreatorId",
				"Configuration profiles must carry creator metadata"
			});
		}
		if (Profile.CreatedAtUtcIso8601.empty())
		{
			OutDiagnostics.push_back({
				ProfilePath + ".CreatedAtUtc",
				"Configuration profiles must carry creation timestamp metadata"
			});
		}
		if (Profile.ValidationNotes.empty())
		{
			OutDiagnostics.push_back({
				ProfilePath + ".ValidationNotes",
				"Configuration profiles must carry validation notes, even when the note is that validation is pending"
			});
		}
		if (Profile.UnitsPolicy.empty())
		{
			OutDiagnostics.push_back({
				ProfilePath + ".UnitsPolicy",
				"Configuration profiles must state their units policy"
			});
		}

		if (Profile.Envelope.Provenance.ApprovalState == EApprovalState::Retired)
		{
			OutDiagnostics.push_back({
				ProfilePath + ".ApprovalState",
				"Retired profiles cannot back a new launch"
			});
		}

		if (Mode == ELaunchValidationMode::Validated)
		{
			if (Profile.Envelope.Provenance.ApprovalState != EApprovalState::Approved)
			{
				OutDiagnostics.push_back({
					ProfilePath + ".ApprovalState",
					"Validated launch requires a human-approved profile; current state is " +
						ToString(Profile.Envelope.Provenance.ApprovalState)
				});
			}

			if (Profile.SourceAuthority != EProfileSourceAuthority::GroundTruth)
			{
				OutDiagnostics.push_back({
					ProfilePath + ".SourceAuthority",
					"Synthetic practice profile cannot back a validated launch"
				});
			}

			if (ContainsTbdOrSynthetic(Profile.Envelope.Provenance.GroundTruthSourceRef))
			{
				OutDiagnostics.push_back({
					ProfilePath + ".GroundTruthSourceRef",
					"Validated launch requires a concrete human-approved ground-truth revision"
				});
			}

			if (Profile.Envelope.Provenance.ApproverId.empty() ||
				Profile.Envelope.Provenance.ApprovedAtUtcIso8601.empty())
			{
				OutDiagnostics.push_back({
					ProfilePath + ".ApprovalMetadata",
					"Validated launch requires approver identity and approval timestamp"
				});
			}
		}
		else if (Mode == ELaunchValidationMode::SyntheticPracticeOnly)
		{
			if (Profile.SourceAuthority != EProfileSourceAuthority::SyntheticPracticeOnly &&
				Profile.Envelope.Provenance.ApprovalState != EApprovalState::Approved)
			{
				OutDiagnostics.push_back({
					ProfilePath + ".SourceAuthority",
					"Synthetic practice launch requires synthetic-practice data or human-approved ground-truth data"
				});
			}
		}

		for (const std::string& RequiredField : GetRequiredFieldSchema(Profile.Envelope.ProfileType))
		{
			const auto Field = Profile.Fields.find(RequiredField);
			if (Field == Profile.Fields.end() || IsMissingConfigValue(Field->second))
			{
				OutDiagnostics.push_back({
					FieldPathFor(Profile, RequiredField),
					"Required field is missing or still TBD; no default or guessed value is allowed"
				});
				continue;
			}

			const std::optional<std::string> TypeError = GetTypeValidationError(RequiredField, Field->second);
			if (TypeError.has_value())
			{
				OutDiagnostics.push_back({
					FieldPathFor(Profile, RequiredField),
					TypeError.value()
				});
			}
		}
	}

	void FFileConfigurationProvider::AddProfileClosure(
		const FConfigurationProfileSnapshot& Profile,
		ELaunchValidationMode Mode,
		std::set<std::string>& SeenKeys,
		std::vector<FConfigSnapshotEnvelope>& OutResolvedProfiles,
		std::vector<FMissingFieldDiagnostic>& OutDiagnostics) const
	{
		const FConfigVersionRef& Ref = Profile.Envelope.Provenance.VersionRef;
		const std::string Key = ProfileKey(Profile.Envelope.ProfileType, Ref.ProfileId, Ref.Version);
		if (!SeenKeys.insert(Key).second)
		{
			return;
		}

		ValidateProfileForMode(Profile, Mode, OutDiagnostics);
		OutResolvedProfiles.push_back(Profile.Envelope);

		for (const FProfileReference& Dependency : Profile.Dependencies)
		{
			const FConfigurationProfileSnapshot* DependencyProfile = FindProfile(
				Dependency.Type,
				Dependency.ProfileId,
				Dependency.Version);
			if (DependencyProfile == nullptr)
			{
				OutDiagnostics.push_back({
					DescribeProfile(Profile) + ".depends." + ToString(Dependency.Type),
					"Referenced profile " + Dependency.ProfileId.Value + ":v" +
						std::to_string(Dependency.Version) + " was not found"
				});
				continue;
			}

			AddProfileClosure(*DependencyProfile, Mode, SeenKeys, OutResolvedProfiles, OutDiagnostics);
		}
	}

	void FFileConfigurationProvider::ValidateRequiredExerciseDependencies(
		const FConfigurationProfileSnapshot& Exercise,
		std::vector<FMissingFieldDiagnostic>& OutDiagnostics) const
	{
		static const std::vector<EProfileType> RequiredDependencies = {
			EProfileType::VehicleProfile,
			EProfileType::TrailerProfile,
			EProfileType::YardProfile,
			EProfileType::DeviceProfile,
			EProfileType::CalibrationProfile,
			EProfileType::ScoringProfile,
			EProfileType::TelemetryProfile
		};

		for (const EProfileType RequiredType : RequiredDependencies)
		{
			const bool bFound = std::any_of(
				Exercise.Dependencies.begin(),
				Exercise.Dependencies.end(),
				[RequiredType](const FProfileReference& Reference) {
					return Reference.Type == RequiredType && Reference.Version != 0 && Reference.ProfileId.IsValid();
				});

			if (!bFound)
			{
				OutDiagnostics.push_back({
					DescribeProfile(Exercise) + ".depends." + ToString(RequiredType),
					"Exercise definitions must explicitly reference this required launch profile type"
				});
			}
		}
	}
}
