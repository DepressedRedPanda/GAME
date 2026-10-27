#pragma once

#include "CoreMinimal.h"
#include "World/HRInteractableActor.h"
#include "HRResourceNode.generated.h"

/** Herbs, ore veins, timber, water. Depletes when gathered and regrows after in-game hours. */
UCLASS()
class HOLLOWREACH_API AHRResourceNode : public AHRInteractableActor
{
	GENERATED_BODY()

public:
	AHRResourceNode();
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override { return !bDepleted; }
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource") FName ItemId = TEXT("Item.Herb");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource") int32 MinQuantity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource") int32 MaxQuantity = 3;
	/** 0 = never depletes (springs, wells). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource") float RespawnHours = 24.f;
	/** Bonus item with a small chance (rare finds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource") FName BonusItemId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resource") float BonusChance = 0.1f;

private:
	void SetDepleted(bool bInDepleted);

	bool bDepleted = false;
	float DepletedAtHours = 0.f;
};
