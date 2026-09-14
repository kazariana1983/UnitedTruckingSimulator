#include "UTS/Development/UTSDevelopmentPlayerController.h"

#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "UTS/Session/UTSSessionSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTSDevelopmentControls, Log, All);

void AUTSDevelopmentPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

#if !UE_BUILD_SHIPPING
    if (!InputComponent)
    {
        return;
    }

    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AUTSDevelopmentPlayerController::BeginSyntheticSession);
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AUTSDevelopmentPlayerController::StartSyntheticAttempt);
    InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AUTSDevelopmentPlayerController::TogglePauseAttempt);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AUTSDevelopmentPlayerController::RequestReset);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AUTSDevelopmentPlayerController::CompleteAttempt);
    InputComponent->BindKey(EKeys::X, IE_Pressed, this, &AUTSDevelopmentPlayerController::AbortAttempt);
#endif
}

UUTSSessionSubsystem* AUTSDevelopmentPlayerController::GetSessionSubsystem()
{
#if !UE_BUILD_SHIPPING
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UUTSSessionSubsystem* Session = GameInstance ? GameInstance->GetSubsystem<UUTSSessionSubsystem>() : nullptr;
    if (!Session)
    {
        UE_LOG(LogUTSDevelopmentControls, Warning, TEXT("UTS session subsystem unavailable."));
    }
    return Session;
#else
    return nullptr;
#endif
}

void AUTSDevelopmentPlayerController::ReportCommand(
    const TCHAR* CommandName,
    bool bSucceeded,
    const UUTSSessionSubsystem& Session) const
{
#if !UE_BUILD_SHIPPING
    UE_LOG(LogUTSDevelopmentControls, Display, TEXT("%s %s: %s"), CommandName,
        bSucceeded ? TEXT("accepted") : TEXT("rejected"), *Session.GetLastDiagnostic());
#endif
}

void AUTSDevelopmentPlayerController::BeginSyntheticSession()
{
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        ReportCommand(TEXT("Begin synthetic session"), Session->BeginSyntheticSession(), *Session);
    }
}

void AUTSDevelopmentPlayerController::StartSyntheticAttempt()
{
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        ReportCommand(TEXT("Start synthetic attempt"), Session->StartSyntheticAttempt(), *Session);
    }
}

void AUTSDevelopmentPlayerController::TogglePauseAttempt()
{
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        const bool bResume = Session->GetSessionStateText().Equals(TEXT("Paused"), ESearchCase::IgnoreCase);
        ReportCommand(bResume ? TEXT("Resume attempt") : TEXT("Pause attempt"),
            bResume ? Session->ResumeAttempt() : Session->PauseAttempt(), *Session);
    }
}

void AUTSDevelopmentPlayerController::RequestReset()
{
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        ReportCommand(TEXT("Request reset"), Session->RequestReset(), *Session);
    }
}

void AUTSDevelopmentPlayerController::CompleteAttempt()
{
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        ReportCommand(TEXT("Complete attempt"), Session->CompleteAttempt(), *Session);
    }
}

void AUTSDevelopmentPlayerController::AbortAttempt()
{
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        ReportCommand(TEXT("Abort attempt"), Session->AbortAttempt(), *Session);
    }
}
