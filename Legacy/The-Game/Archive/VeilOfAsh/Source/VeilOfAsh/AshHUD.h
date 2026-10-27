#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AshHUD.generated.h"
UCLASS()
class VEILOFASH_API AAshHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
