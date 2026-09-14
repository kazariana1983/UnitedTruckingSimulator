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
    virtual void SetupInputComponent() override;

private:
    UUTSSessionSubsystem* GetSessionSubsystem();
    void ReportCommand(const TCHAR* CommandName, bool bSucceeded, const UUTSSessionSubsystem& Session) const;

    void BeginSyntheticSession();
    void StartSyntheticAttempt();
    void TogglePauseAttempt();
    void RequestReset();
    void CompleteAttempt();
    void AbortAttempt();
};
