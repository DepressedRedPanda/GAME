#pragma once

#include "CoreMinimal.h"
#include "World/HRInteractableActor.h"
#include "HRVeilNode.generated.h"

class AHRGate;

UENUM()
enum class EHRVeilNodeKind : uint8 { Cipher, Record, Rune, Gate, FreefolkEnvoy, ChoirEnvoy, Vanguard, Seer, Shade };

/** Veil chapter interactions on the same Hollowreach player, inventory and narrative flags. */
UCLASS()
class HOLLOWREACH_API AHRVeilNode : public AHRInteractableActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual bool CanInteract(const AHRPlayerCharacter* Player) const override;
    virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
    virtual void Interact(AHRPlayerCharacter* Player) override;

    UPROPERTY(EditAnywhere) EHRVeilNodeKind Kind = EHRVeilNodeKind::Cipher;
    UPROPERTY(EditAnywhere) int32 RuneNumber = 1;
    UPROPERTY() TObjectPtr<AHRGate> LinkedGate;
};
