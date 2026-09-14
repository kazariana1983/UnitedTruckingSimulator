#include "UTS/Development/UTSDevelopmentPlayerController.h"

#include "Components/InputComponent.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "UTS/Development/UTSDevelopmentTractor.h"
#include "UTS/Vehicle/SyntheticTractorMotion.h"
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

    InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AUTSDevelopmentPlayerController::ToggleCamera);
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
    bRequireNeutralInput = true;
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        ReportCommand(TEXT("Start synthetic attempt"), Session->StartSyntheticAttempt(), *Session);
    }
}

void AUTSDevelopmentPlayerController::TogglePauseAttempt()
{
    bRequireNeutralInput = true;
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        const bool bResume = Session->GetSessionStateText().Equals(TEXT("Paused"), ESearchCase::IgnoreCase);
        ReportCommand(bResume ? TEXT("Resume attempt") : TEXT("Pause attempt"),
            bResume ? Session->ResumeAttempt() : Session->PauseAttempt(), *Session);
    }
}

void AUTSDevelopmentPlayerController::RequestReset()
{
    bRequireNeutralInput = true;
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        ReportCommand(TEXT("Request reset"), Session->RequestReset(), *Session);
    }
}

void AUTSDevelopmentPlayerController::CompleteAttempt()
{
    bRequireNeutralInput = true;
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        ReportCommand(TEXT("Complete attempt"), Session->CompleteAttempt(), *Session);
    }
}

void AUTSDevelopmentPlayerController::AbortAttempt()
{
    bRequireNeutralInput = true;
    if (UUTSSessionSubsystem* Session = GetSessionSubsystem())
    {
        ReportCommand(TEXT("Abort attempt"), Session->AbortAttempt(), *Session);
    }
}

void AUTSDevelopmentPlayerController::ToggleCamera()
{
#if !UE_BUILD_SHIPPING
    if (auto* Tractor = Cast<AUTSDevelopmentTractor>(GetPawn())) Tractor->ToggleCamera();
#endif
}

void AUTSDevelopmentPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
#if !UE_BUILD_SHIPPING
    auto* Tractor = Cast<AUTSDevelopmentTractor>(GetPawn());
    if (!Tractor) return;
    auto* Session = GetSessionSubsystem();
    auto* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
    const bool bFocused = ViewportClient && ViewportClient->Viewport && ViewportClient->Viewport->HasFocus();
    const bool Forward = IsInputKeyDown(EKeys::W);
    const bool Reverse = IsInputKeyDown(EKeys::S);
    const bool Left = IsInputKeyDown(EKeys::A);
    const bool Right = IsInputKeyDown(EKeys::D);
    const bool Brake = IsInputKeyDown(EKeys::SpaceBar);
    if (!Session || !Session->IsAttemptRunning() || !bFocused)
    {
        bRequireNeutralInput = true;
        UTS::FSemanticControlFrame Stopped;
        Stopped.bParkingBrakeEngaged = true;
        Tractor->SetControlFrame(Stopped);
        // Focus loss freezes the developer attempt; resume explicitly with P.
        if (Session && Session->IsAttemptRunning() && !bFocused) Session->PauseAttempt();
        return;
    }
    if (bRequireNeutralInput)
    {
        bRequireNeutralInput = Forward || Reverse || Left || Right || Brake;
        UTS::FSemanticControlFrame Stopped;
        Stopped.bParkingBrakeEngaged = true;
        Tractor->SetControlFrame(Stopped);
        return;
    }
    Tractor->SetControlFrame(UTS::FSyntheticKeyboardAdapter::Normalize(Forward, Reverse, Left, Right, Brake));
#endif
}

void AUTSDevelopmentPlayerController::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    if (auto* Session = GetSessionSubsystem())
        Session->OnPrototypeResetRequested.AddUObject(this, &AUTSDevelopmentPlayerController::RequireNeutralInput);
#endif
}

void AUTSDevelopmentPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if !UE_BUILD_SHIPPING
    if (auto* Session = GetSessionSubsystem()) Session->OnPrototypeResetRequested.RemoveAll(this);
#endif
    Super::EndPlay(EndPlayReason);
}

void AUTSDevelopmentPlayerController::RequireNeutralInput()
{
    bRequireNeutralInput = true;
}
