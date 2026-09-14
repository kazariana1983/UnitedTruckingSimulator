#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UTSDevelopmentPlayerController.generated.h"

class UUTSSessionSubsystem;

UCLASS()
class UTS_API AUTSDevelopmentPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;

private:
    bool bRequireNeutralInput = true;
    void RequireNeutralInput();
    void ToggleCamera();
    UUTSSessionSubsystem* GetSessionSubsystem();
    void ReportCommand(const TCHAR* CommandName, bool bSucceeded, const UUTSSessionSubsystem& Session) const;

    void BeginSyntheticSession();
    void StartSyntheticAttempt();
    void TogglePauseAttempt();
    void RequestReset();
    void CompleteAttempt();
    void AbortAttempt();
};
