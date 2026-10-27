#pragma once

#include "CoreMinimal.h"
#include "Characters/HRCharacterBase.h"
#include "Core/HRInteractable.h"
#include "Items/HRItemTypes.h"
#include "HRNPCCharacter.generated.h"

class AHRPlayerCharacter;

/**
 * Talkable villager, quest giver or merchant. Uses the same AI as enemies, so if your
 * reputation with their faction sinks low enough, they will fight you.
 */
UCLASS()
class HOLLOWREACH_API AHRNPCCharacter : public AHRCharacterBase, public IHRInteractable
{
	GENERATED_BODY()

public:
	AHRNPCCharacter();

	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override;
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;
	virtual EHRAwareness GetAwarenessTowards(const AActor* Other) const override;

	/** Price for the player, after reputation discounts or surcharges. */
	int32 GetBuyPrice(FName ItemId) const;
	int32 GetSellPrice(const FHRItemInstance& Item) const;
	bool Buy(AHRPlayerCharacter* Player, int32 StockIndex);
	bool Sell(AHRPlayerCharacter* Player, const FGuid& ItemGuid);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName NpcId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") FName DialogueId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") bool bIsMerchant = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") TArray<FHRItemAmount> Stock;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC") float PriceMultiplier = 1.f;

private:
	float GetReputationPriceFactor() const;
};
