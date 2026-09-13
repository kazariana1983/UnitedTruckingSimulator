#include "UTS/Config/UTSConfigurationSubsystem.h"

#include "Misc/Paths.h"

#include <filesystem>

DEFINE_LOG_CATEGORY_STATIC(LogUTSConfig, Log, All);

namespace
{
    UTS::FProfileId ToProfileId(const FString& Value)
    {
        UTS::FProfileId Result;
        Result.Value = TCHAR_TO_UTF8(*Value);
        return Result;
    }
}

void UUTSConfigurationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadConfigurationProfiles();
}

bool UUTSConfigurationSubsystem::ReloadConfigurationProfiles()
{
    LastProfileRoot = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("ProfileSets"));
#if PLATFORM_WINDOWS
    const std::filesystem::path NativeProfileRoot(*LastProfileRoot);
#else
    const std::filesystem::path NativeProfileRoot(std::filesystem::u8path(TCHAR_TO_UTF8(*LastProfileRoot)));
#endif
    UTS::FProfileSetLoadResult Result = Provider.LoadFromDirectory(NativeProfileRoot);

    if (!Result.bSucceeded)
    {
        for (const std::string& Error : Result.Errors)
        {
            UE_LOG(LogUTSConfig, Error, TEXT("Configuration profile load error: %s"), UTF8_TO_TCHAR(Error.c_str()));
        }
        return false;
    }

    UE_LOG(LogUTSConfig, Display, TEXT("Loaded %d configuration profiles from %s"), static_cast<int32>(Result.ProfileCount), *LastProfileRoot);
    return true;
}

bool UUTSConfigurationSubsystem::IsValidatedLaunchAvailableForExercise(const FString& ExerciseProfileId)
{
    UTS::FValidatedConfigSet Result = Provider.ValidateForLaunch(ToProfileId(ExerciseProfileId));
    return Result.bReadyForValidatedLaunch;
}

UTS::FConfigLoadResult UUTSConfigurationSubsystem::LoadProfile(
    UTS::EProfileType Type,
    const UTS::FProfileId& ProfileId,
    uint32_t RequestedVersion)
{
    return Provider.LoadProfile(Type, ProfileId, RequestedVersion);
}

UTS::FValidatedConfigSet UUTSConfigurationSubsystem::ValidateForLaunch(const UTS::FProfileId& ExerciseId)
{
    return Provider.ValidateForLaunch(ExerciseId);
}

UTS::EApprovalState UUTSConfigurationSubsystem::GetApprovalState(
    const UTS::FProfileId& ProfileId,
    uint32_t Version) const
{
    return Provider.GetApprovalState(ProfileId, Version);
}
