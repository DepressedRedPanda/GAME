#pragma once

#include "CoreMinimal.h"
#include "World/HRInteractableActor.h"
#include "HREndingAltar.generated.h"

/** The final choice. Only reachable once the Warden falls; what you can choose depends on everything you did. */
UCLASS()
class HOLLOWREACH_API AHREndingAltar : public AHRInteractableActor
{
	GENERATED_BODY()

public:
	AHREndingAltar();

	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override;
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ending") FName RequiredFlag = TEXT("Boss.Warden.Defeated");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ending") FName DialogueId = TEXT("Dialogue.Altar");
};
