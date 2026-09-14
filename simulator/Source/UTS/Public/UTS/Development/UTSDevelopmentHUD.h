#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UTSDevelopmentHUD.generated.h"

UCLASS()
class UTS_API AUTSDevelopmentHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
