#include "UTS/Development/UTSDevelopmentGameMode.h"

#include "UTS/Development/UTSDevelopmentHUD.h"
#include "UTS/Development/UTSDevelopmentPlayerController.h"

AUTSDevelopmentGameMode::AUTSDevelopmentGameMode()
{
#if !UE_BUILD_SHIPPING
    DefaultPawnClass = nullptr; // This screen has no driving pawn yet.
    HUDClass = AUTSDevelopmentHUD::StaticClass();
    PlayerControllerClass = AUTSDevelopmentPlayerController::StaticClass();
#endif
}
