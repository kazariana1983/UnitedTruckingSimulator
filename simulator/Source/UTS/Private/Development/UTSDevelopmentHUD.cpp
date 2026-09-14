#include "UTS/Development/UTSDevelopmentHUD.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UTS/Session/UTSSessionSubsystem.h"

void AUTSDevelopmentHUD::DrawHUD()
{
    Super::DrawHUD();

#if !UE_BUILD_SHIPPING
    constexpr float X = 24.0f;
    float Y = 24.0f;
    constexpr float LineHeight = 18.0f;

    auto DrawStatusLine = [this, X, &Y, LineHeight](const FString& Text, const FColor& Color)
    {
        DrawText(Text, Color, X, Y, nullptr, 1.0f, false);
        Y += LineHeight;
    };

    DrawStatusLine(TEXT("SYNTHETIC DEVELOPMENT ONLY - NOT VALIDATED TRAINING"), FColor::Yellow);
    DrawStatusLine(TEXT("Records are memory-only. No driving, scoring, or validated training is connected."), FColor::White);

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UUTSSessionSubsystem* Session = GameInstance ? GameInstance->GetSubsystem<UUTSSessionSubsystem>() : nullptr;
    if (!Session)
    {
        DrawStatusLine(TEXT("UTS session subsystem unavailable."), FColor::Red);
        return;
    }

    DrawStatusLine(FString::Printf(TEXT("Session: %s"), *Session->GetSessionStateText()), FColor::Cyan);
    DrawStatusLine(FString::Printf(TEXT("Attempt: %s%s"), *Session->GetAttemptStateText(),
        Session->HasActiveAttempt() ? TEXT(" (active)") : TEXT("")), FColor::Cyan);
    DrawStatusLine(FString::Printf(TEXT("Events: %d"), Session->GetEventCount()), FColor::Cyan);
    DrawStatusLine(FString::Printf(TEXT("Last: %s"), *Session->GetLastDiagnostic()), FColor(192, 192, 192));
    DrawStatusLine(TEXT("Controls: B begin | Enter start | P pause/resume | R reset | C complete | X abort"), FColor::Green);
#endif
}
