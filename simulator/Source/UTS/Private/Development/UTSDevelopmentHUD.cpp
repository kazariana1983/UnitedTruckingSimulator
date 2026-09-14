#include "UTS/Development/UTSDevelopmentHUD.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UTS/Development/UTSDevelopmentTractor.h"
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

    DrawRect(FLinearColor(0.01f, 0.02f, 0.03f, 0.85f), 16, 16, 880, 210);
    DrawStatusLine(TEXT("SYNTHETIC DEVELOPMENT ONLY - NOT VALIDATED TRAINING"), FColor::Yellow);
    DrawStatusLine(TEXT("Toy tractor + yard. No trailer, collision scoring, or validated vehicle physics. Records stay in memory."), FColor::White);

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
    if (PlayerOwner)
    {
        if (auto* Tractor = Cast<AUTSDevelopmentTractor>(PlayerOwner->GetPawn()))
            DrawStatusLine(FString::Printf(TEXT("Prototype speed: %.1f m/s"), Tractor->GetPrototypeSpeedMps()), FColor::White);
    }
    DrawStatusLine(TEXT("Drive: W forward | S reverse | A/D steer | Space brake | V cab/overhead"), FColor::Green);
    DrawStatusLine(TEXT("Controls: B begin | Enter start | P pause/resume | R reset | C complete | X abort"), FColor::Green);
#endif
}
