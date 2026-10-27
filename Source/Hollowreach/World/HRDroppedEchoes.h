#pragma once

#include "CoreMinimal.h"
#include "World/HRInteractableActor.h"
#include "HRDroppedEchoes.generated.h"

/** The echoes you carried when you died. Reach them before you die again or they are gone. */
UCLASS()
class HOLLOWREACH_API AHRDroppedEchoes : public AHRInteractableActor
{
	GENERATED_BODY()

public:
	AHRDroppedEchoes();

	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Echoes") int32 Amount = 0;
};
