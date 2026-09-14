#include "UTS/Development/UTSDevelopmentGameMode.h"

#include "UTS/Development/UTSDevelopmentHUD.h"
#include "UTS/Development/UTSDevelopmentTractor.h"
#include "UTS/Development/UTSDevelopmentYard.h"
#include "Engine/World.h"
#include "UTS/Development/UTSDevelopmentPlayerController.h"

AUTSDevelopmentGameMode::AUTSDevelopmentGameMode()
{
#if !UE_BUILD_SHIPPING
    DefaultPawnClass = AUTSDevelopmentTractor::StaticClass();
    HUDClass = AUTSDevelopmentHUD::StaticClass();
    PlayerControllerClass = AUTSDevelopmentPlayerController::StaticClass();
#endif
}

void AUTSDevelopmentGameMode::StartPlay()
{
#if !UE_BUILD_SHIPPING
    // Elevated test platform isolates this prototype from the template landscape.
    // This is an arbitrary developer origin, not a surveyed training location.
    if (GetWorld())
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        GetWorld()->SpawnActor<AUTSDevelopmentYard>(FVector(0, 0, 100000), FRotator::ZeroRotator, Parameters);
    }
#endif
    Super::StartPlay();
}

void AUTSDevelopmentGameMode::RestartPlayer(AController* NewPlayer)
{
#if !UE_BUILD_SHIPPING
    // No PlayerStart or hand-built map is required for the development yard.
    if (NewPlayer) RestartPlayerAtTransform(NewPlayer, FTransform(FRotator::ZeroRotator, FVector(0, 0, 100000)));
#else
    Super::RestartPlayer(NewPlayer);
#endif
}
