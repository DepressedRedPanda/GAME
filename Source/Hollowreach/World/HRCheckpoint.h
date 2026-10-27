#pragma once

#include "CoreMinimal.h"
#include "World/HRInteractableActor.h"
#include "HRCheckpoint.generated.h"

class UPointLightComponent;

/**
 * Ember shrine (bonfire / Site of Grace). Kindle it to discover it, rest to heal, refill flasks,
 * save, level up, craft, upgrade and fast-travel. Resting makes the world's enemies return.
 * Shrines are also heat sources for the survival system.
 */
UCLASS()
class HOLLOWREACH_API AHRCheckpoint : public AHRInteractableActor
{
	GENERATED_BODY()

public:
	AHRCheckpoint();

	virtual void BeginPlay() override;
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	UFUNCTION(BlueprintPure, Category="Checkpoint") bool IsDiscovered() const;
	UFUNCTION(BlueprintPure, Category="Checkpoint") FTransform GetRespawnTransform() const;
	void Discover(bool bNotify);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Checkpoint") bool bHasForge = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Checkpoint") bool bStartsDiscovered = false;

protected:
	void RefreshVisuals();

	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Flame;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UPointLightComponent> Light;
};
