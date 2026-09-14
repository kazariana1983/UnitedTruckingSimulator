#include "UTS/Config/ConfigurationStore.h"

#include <cstdlib>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
	int FailureCount = 0;

	#define CHECK(Condition) \
		do \
		{ \
			if (!(Condition)) \
			{ \
				std::cerr << "CHECK failed at " << __FILE__ << ":" << __LINE__ << ": " #Condition << std::endl; \
				++FailureCount; \
			} \
		} while (false)

	UTS::FProfileId Id(const std::string& Value)
	{
		UTS::FProfileId Result;
		Result.Value = Value;
		return Result;
	}

	bool DiagnosticsContain(const std::vector<UTS::FMissingFieldDiagnostic>& Diagnostics, const std::string& Needle)
	{
		for (const UTS::FMissingFieldDiagnostic& Diagnostic : Diagnostics)
		{
			if (Diagnostic.FieldPath.find(Needle) != std::string::npos ||
				Diagnostic.Reason.find(Needle) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	std::string Lower(std::string Text)
	{
		for (char& C : Text)
		{
			if (C >= 'A' && C <= 'Z')
			{
				C = static_cast<char>(C - 'A' + 'a');
			}
		}
		return Text;
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "usage: configuration_store_tests <profile-root>" << std::endl;
		return EXIT_FAILURE;
	}

	UTS::FFileConfigurationProvider Provider;
	const UTS::FProfileSetLoadResult LoadResult = Provider.LoadFromDirectory(argv[1]);
	CHECK(LoadResult.bSucceeded);
	CHECK(LoadResult.LoadedFiles.size() == 2);
	CHECK(LoadResult.ProfileCount == 16);
	CHECK(Provider.GetProfileCount() == 16);

	const UTS::FConfigLoadResult SyntheticVehicle = Provider.LoadProfile(
		UTS::EProfileType::VehicleProfile,
		Id("uts.synthetic.practice.vehicle"),
		1);
	CHECK(SyntheticVehicle.bSucceeded);
	CHECK(SyntheticVehicle.Error == UTS::EConfigError::None);
	CHECK(SyntheticVehicle.Envelope.Provenance.ApprovalState == UTS::EApprovalState::Draft);
	CHECK(SyntheticVehicle.Envelope.Provenance.GroundTruthSourceRef.find("SYNTHETIC TEST DATA") != std::string::npos);

	const UTS::FConfigLoadResult LatestSyntheticVehicle = Provider.LoadProfile(
		UTS::EProfileType::VehicleProfile,
		Id("uts.synthetic.practice.vehicle"),
		0);
	CHECK(!LatestSyntheticVehicle.bSucceeded);
	CHECK(LatestSyntheticVehicle.Error == UTS::EConfigError::VersionNotFound);

	const UTS::FApprovalStateQueryResult UnknownApproval = Provider.GetApprovalStateDetailed(Id("uts.unknown.profile"), 1000);
	CHECK(!UnknownApproval.bFound);
	CHECK(Provider.GetApprovalState(Id("uts.unknown.profile"), 1000) == UTS::EApprovalState::Draft);

	const UTS::FValidatedConfigSet DraftLaunch = Provider.ValidateForLaunch(Id("uts.gt_pending.exercise.straight_backing"));
	CHECK(!DraftLaunch.bReadyForValidatedLaunch);
	CHECK(DiagnosticsContain(DraftLaunch.MissingFields, "Vehicle.WheelbaseMeters"));
	CHECK(DiagnosticsContain(DraftLaunch.MissingFields, "Vehicle.SteeringRangeDegrees"));
	CHECK(DiagnosticsContain(DraftLaunch.MissingFields, "Exercise.ResetPoseRef"));
	CHECK(DiagnosticsContain(DraftLaunch.MissingFields, "Scoring.PracticeRules"));
	CHECK(DiagnosticsContain(DraftLaunch.MissingFields, "Telemetry.SampleRateHz"));

	const UTS::FValidatedConfigSet SyntheticValidatedLaunch = Provider.ValidateForLaunch(Id("uts.synthetic.practice.exercise.straight_backing"));
	CHECK(!SyntheticValidatedLaunch.bReadyForValidatedLaunch);
	CHECK(DiagnosticsContain(SyntheticValidatedLaunch.MissingFields, "Synthetic practice profile cannot back a validated launch"));
	CHECK(DiagnosticsContain(SyntheticValidatedLaunch.MissingFields, "ApprovalState"));
	CHECK(DiagnosticsContain(SyntheticValidatedLaunch.MissingFields, "Validated launch remains unavailable in P2-01"));

	const UTS::FConfigurationModeValidationResult SyntheticPracticeLaunch = Provider.ValidateForMode(
		Id("uts.synthetic.practice.exercise.straight_backing"),
		UTS::ELaunchValidationMode::SyntheticPracticeOnly);
	CHECK(SyntheticPracticeLaunch.bReadyForRequestedMode);
	CHECK(SyntheticPracticeLaunch.Diagnostics.empty());
	CHECK(SyntheticPracticeLaunch.ResolvedProfiles.size() == 8);

	const std::vector<UTS::FConfigurationProfileSnapshot> Snapshots = Provider.GetAllSnapshots();
	for (const UTS::FConfigurationProfileSnapshot& Snapshot : Snapshots)
	{
		if (Snapshot.SourceAuthority != UTS::EProfileSourceAuthority::SyntheticPracticeOnly)
		{
			continue;
		}

		CHECK(Snapshot.Envelope.Provenance.ApprovalState != UTS::EApprovalState::Approved);
		CHECK(Lower(Snapshot.DisplayName).find("approved") == std::string::npos);
		CHECK(Lower(Snapshot.Envelope.Provenance.GroundTruthSourceRef).find("approved") == std::string::npos);
	}

	// A broken reload must never leave valid-looking partial profiles exposed.
	const auto TempRoot = std::filesystem::temp_directory_path() /
		("uts-config-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	CHECK(std::filesystem::create_directory(TempRoot));
	std::ifstream Fixture(std::filesystem::path(argv[1]) / "SyntheticPracticeOnly/profiles.ini");
	std::ostringstream FixtureText;
	FixtureText << Fixture.rdbuf();
	const std::string GoodText = FixtureText.str();
	CHECK(!GoodText.empty());
	auto CheckRejectedReload = [&](const std::string& BadText) {
		{ std::ofstream File(TempRoot / "profiles.ini"); File << BadText; }
		const auto BadLoad = Provider.LoadFromDirectory(TempRoot);
		CHECK(!BadLoad.bSucceeded);
		CHECK(!BadLoad.Errors.empty());
		CHECK(Provider.GetProfileCount() == 0);
		CHECK(!Provider.LoadProfile(UTS::EProfileType::VehicleProfile, Id("uts.synthetic.practice.vehicle"), 1).bSucceeded);
		CHECK(!Provider.ValidateForMode(Id("uts.synthetic.practice.exercise.straight_backing"),
			UTS::ELaunchValidationMode::SyntheticPracticeOnly).bReadyForRequestedMode);
	};
	CheckRejectedReload(GoodText + "\nnot-key-value\n");
	CheckRejectedReload(GoodText + "\n[Profile:broken]\n");
	CheckRejectedReload(GoodText + "\n[UnexpectedSection]\n");
	CheckRejectedReload(GoodText + "\n" + GoodText); // duplicate immutable versions
	CHECK(Provider.LoadFromDirectory(argv[1]).bSucceeded);
	auto Snapshot = Provider.GetProfileSnapshot(UTS::EProfileType::VehicleProfile, Id("uts.synthetic.practice.vehicle"), 1);
	CHECK(Snapshot.has_value());
	if (Snapshot)
	{
		Snapshot->Fields["Vehicle.WheelbaseMeters"] = "changed";
		CHECK(Provider.GetProfileSnapshot(UTS::EProfileType::VehicleProfile,
			Id("uts.synthetic.practice.vehicle"), 1)->Fields.at("Vehicle.WheelbaseMeters") == "1000");
	}
	CHECK(!Provider.LoadFromDirectory(TempRoot / "missing").bSucceeded);
	CHECK(Provider.GetProfileCount() == 0);
	std::filesystem::remove_all(TempRoot);

	if (FailureCount != 0)
	{
		std::cerr << FailureCount << " configuration test check(s) failed" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "configuration_store_tests: PASS" << std::endl;
	return EXIT_SUCCESS;
}
