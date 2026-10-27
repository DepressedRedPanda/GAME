#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HRInteractionComponent.generated.h"

class AHRPlayerCharacter;

/** Finds the best interactable in front of the player and exposes its prompt to the HUD. */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="Interaction") bool Interact();
	UFUNCTION(BlueprintPure, Category="Interaction") AActor* GetFocusedActor() const { return Focused.Get(); }
	UFUNCTION(BlueprintPure, Category="Interaction") FText GetPrompt() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") float Radius = 220.f;

private:
	void FindFocus();
	AHRPlayerCharacter* GetPlayer() const;

	TWeakObjectPtr<AActor> Focused;
};
