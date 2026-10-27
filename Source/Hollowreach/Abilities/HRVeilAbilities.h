#pragma once

#include "CoreMinimal.h"
#include "Abilities/HRAbility.h"
#include "HRVeilAbilities.generated.h"

UCLASS()
class HOLLOWREACH_API UHRAbility_VoidPulse : public UHRAbility
{
    GENERATED_BODY()
public:
    UHRAbility_VoidPulse();
    virtual void Activate(AHRCharacterBase* Owner) override;
};

UCLASS()
class HOLLOWREACH_API UHRAbility_SpectralForm : public UHRAbility
{
    GENERATED_BODY()
public:
    UHRAbility_SpectralForm();
    virtual void Activate(AHRCharacterBase* Owner) override;
    virtual void Deactivate(AHRCharacterBase* Owner) override;
};
