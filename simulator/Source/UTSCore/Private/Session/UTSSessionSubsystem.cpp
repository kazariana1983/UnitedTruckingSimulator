#include "UTS/Session/UTSSessionSubsystem.h"

#include "Misc/Guid.h"
#include "Subsystems/SubsystemCollection.h"
#include "UTS/Config/UTSConfigurationSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTSSession, Log, All);

namespace
{
    UTS::FConfigVersionRef ProfileRef(const FString& Id, int32 Version)
    {
        return {UTS::FProfileId{TCHAR_TO_UTF8(*Id)}, Version > 0 ? static_cast<uint32_t>(Version) : 0};
    }
}

void UUTSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
#if UE_BUILD_SHIPPING
    LastDiagnostic = TEXT("Development session controls are disabled in Shipping builds.");
#else
    auto* Configuration = Collection.InitializeDependency<UUTSConfigurationSubsystem>();
    if (!Configuration)
    {
        LastDiagnostic = TEXT("Configuration subsystem is unavailable.");
        return;
    }
    // Distinct development generator per game instance, including multi-PIE.
    const FString Prefix = TEXT("synthetic-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Ids = MakeUnique<UTS::FSequentialSessionIdGenerator>(TCHAR_TO_UTF8(*Prefix));
    Manager = MakeUnique<UTS::FSessionManager>(UTS::FSessionManagerDependencies{
        &Configuration->GetConfigurationProvider(), &Clock, Ids.Get()});
    LastDiagnostic = TEXT("SYNTHETIC DEVELOPMENT ONLY. Begin a session; records stay in memory.");
#endif
}

void UUTSSessionSubsystem::Deinitialize()
{
    // No false persistence or recovery claim when this game instance ends.
    Manager.Reset();
    Ids.Reset();
    DisplayAttemptId = {};
    Super::Deinitialize();
}

bool UUTSSessionSubsystem::RequireManager()
{
    if (Manager) return true;
    LastDiagnostic = TEXT("Development session manager is unavailable; see initialization log/settings.");
    return false;
}

bool UUTSSessionSubsystem::RecordResult(const UTS::FSessionCommandResult& Result, const TCHAR* Success)
{
    LastDiagnostic = Result.bSucceeded ? FString(Success) : FString(UTF8_TO_TCHAR(Result.DiagnosticMessage.c_str()));
    UE_LOG(LogUTSSession, Display, TEXT("Synthetic session command: %s"), *LastDiagnostic);
    return Result.bSucceeded;
}

bool UUTSSessionSubsystem::BeginSyntheticSession()
{
    if (!RequireManager()) return false;
    const auto Result = Manager->BeginSyntheticPracticeSession({
        UTS::FStationId{TCHAR_TO_UTF8(*StationId)}, UTS::FStudentId{TCHAR_TO_UTF8(*SyntheticStudentId)},
        TCHAR_TO_UTF8(*ClientVersion)});
    LastDiagnostic = Result.bSucceeded ? TEXT("Synthetic session ready; no real authentication performed.") :
        FString(UTF8_TO_TCHAR(Result.DiagnosticMessage.c_str()));
    return Result.bSucceeded;
}

bool UUTSSessionSubsystem::StartSyntheticAttempt()
{
    if (!RequireManager()) return false;
    const auto Result = Manager->TryBeginSyntheticPracticeAttempt(
        ProfileRef(ExerciseProfileId, ExerciseVersion), ProfileRef(VehicleProfileId, VehicleVersion),
        ProfileRef(TrailerProfileId, TrailerVersion), ProfileRef(ScoringProfileId, ScoringVersion));
    LastDiagnostic = Result.bSucceeded ? TEXT("Synthetic attempt active; no driving or scoring is connected.") :
        FString(UTF8_TO_TCHAR(Result.DiagnosticMessage.c_str()));
    for (const auto& Missing : Result.MissingFields)
    {
        UE_LOG(LogUTSSession, Warning, TEXT("%s: %s"), UTF8_TO_TCHAR(Missing.FieldPath.c_str()), UTF8_TO_TCHAR(Missing.Reason.c_str()));
    }
    if (Result.bSucceeded) DisplayAttemptId = Result.Context.AttemptId;
    return Result.bSucceeded;
}

bool UUTSSessionSubsystem::PauseAttempt()
{
    return RequireManager() && RecordResult(Manager->PauseAttempt(DisplayAttemptId), TEXT("Attempt paused."));
}
bool UUTSSessionSubsystem::ResumeAttempt()
{
    return RequireManager() && RecordResult(Manager->ResumeAttempt(DisplayAttemptId), TEXT("Attempt resumed."));
}
bool UUTSSessionSubsystem::RequestReset()
{
    return RequireManager() && RecordResult(Manager->RequestReset(DisplayAttemptId,
        UTS::EResetPolicy::ResetInPlaceAndKeepPriorSegment), TEXT("Reset request recorded; no vehicle pose reset is connected."));
}
bool UUTSSessionSubsystem::CompleteAttempt()
{
    return RequireManager() && RecordResult(Manager->CompleteOrAbort(DisplayAttemptId,
        UTS::EAttemptOutcome::Completed), TEXT("Attempt ended. No score, pass/fail, or disk save is implied."));
}
bool UUTSSessionSubsystem::AbortAttempt()
{
    return RequireManager() && RecordResult(Manager->CompleteOrAbort(DisplayAttemptId,
        UTS::EAttemptOutcome::Aborted), TEXT("Attempt aborted; earlier events remain in memory."));
}
FString UUTSSessionSubsystem::GetSessionStateText() const
{
    return Manager ? FString(UTF8_TO_TCHAR(UTS::ToString(Manager->GetSessionState()).c_str())) : TEXT("Unavailable");
}
FString UUTSSessionSubsystem::GetAttemptStateText() const
{
    return Manager ? FString(UTF8_TO_TCHAR(UTS::ToString(Manager->GetAttemptState(DisplayAttemptId)).c_str())) : TEXT("Unavailable");
}
FString UUTSSessionSubsystem::GetLastDiagnostic() const { return LastDiagnostic; }
int32 UUTSSessionSubsystem::GetEventCount() const
{
    return Manager ? static_cast<int32>(Manager->GetAttemptEvents(DisplayAttemptId).size()) : 0;
}
bool UUTSSessionSubsystem::HasActiveAttempt() const
{
    return Manager && Manager->GetActiveAttemptId().has_value();
}
