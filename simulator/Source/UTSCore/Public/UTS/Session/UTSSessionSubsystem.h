#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UTS/Session/SessionManager.h"
#include "UTSSessionSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FUTSPrototypeResetRequested);

// Development facade only. Synthetic identity and version pins are configured
// in DefaultGame.ini; this is not a sign-in or validated training interface.
UCLASS(Config=Game, DefaultConfig)
class UTSCORE_API UUTSSessionSubsystem final : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    FUTSPrototypeResetRequested OnPrototypeResetRequested;

    UFUNCTION(BlueprintPure, Category="UTS|Development")
    bool IsAttemptRunning() const;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="UTS|Development")
    bool BeginSyntheticSession();
    UFUNCTION(BlueprintCallable, Category="UTS|Development")
    bool StartSyntheticAttempt();
    UFUNCTION(BlueprintCallable, Category="UTS|Development")
    bool PauseAttempt();
    UFUNCTION(BlueprintCallable, Category="UTS|Development")
    bool ResumeAttempt();
    UFUNCTION(BlueprintCallable, Category="UTS|Development")
    bool RequestReset();
    UFUNCTION(BlueprintCallable, Category="UTS|Development")
    bool CompleteAttempt();
    UFUNCTION(BlueprintCallable, Category="UTS|Development")
    bool AbortAttempt();

    UFUNCTION(BlueprintPure, Category="UTS|Development")
    FString GetSessionStateText() const;
    UFUNCTION(BlueprintPure, Category="UTS|Development")
    FString GetAttemptStateText() const;
    UFUNCTION(BlueprintPure, Category="UTS|Development")
    FString GetLastDiagnostic() const;
    UFUNCTION(BlueprintPure, Category="UTS|Development")
    int32 GetEventCount() const;
    UFUNCTION(BlueprintPure, Category="UTS|Development")
    bool HasActiveAttempt() const;

private:
    // No plausible physical defaults. All supplied identifiers point to the
    // explicitly synthetic profile set checked into this development project.
    UPROPERTY(Config) FString StationId;
    UPROPERTY(Config) FString SyntheticStudentId;
    UPROPERTY(Config) FString ClientVersion;
    UPROPERTY(Config) FString ExerciseProfileId;
    UPROPERTY(Config) int32 ExerciseVersion = 0;
    UPROPERTY(Config) FString VehicleProfileId;
    UPROPERTY(Config) int32 VehicleVersion = 0;
    UPROPERTY(Config) FString TrailerProfileId;
    UPROPERTY(Config) int32 TrailerVersion = 0;
    UPROPERTY(Config) FString ScoringProfileId;
    UPROPERTY(Config) int32 ScoringVersion = 0;

    UTS::FSteadySessionClock Clock;
    TUniquePtr<UTS::FSequentialSessionIdGenerator> Ids;
    TUniquePtr<UTS::FSessionManager> Manager;
    UTS::FAttemptId DisplayAttemptId;
    FString LastDiagnostic;

    bool RequireManager();
    bool RecordResult(const UTS::FSessionCommandResult& Result, const TCHAR* Success);
};
