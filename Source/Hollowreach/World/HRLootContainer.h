#pragma once

#include "CoreMinimal.h"
#include "World/HRInteractableActor.h"
#include "Items/HRItemTypes.h"
#include "HRLootContainer.generated.h"

class AHREnemyCharacter;

/**
 * Chests, corpses and loot bags. Contents roll from a loot table the first time they're opened
 * (seeded, so the same world gives the same loot). Some chests are locked behind keys, and a
 * few are not chests at all.
 */
UCLASS()
class HOLLOWREACH_API AHRLootContainer : public AHRInteractableActor
{
	GENERATED_BODY()

public:
	AHRLootContainer();

	virtual void BeginPlay() override;
	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override;
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	/** Drops a bag with pre-rolled contents (enemy loot). */
	static AHRLootContainer* SpawnLootBag(UWorld* World, const FVector& Location, const TArray<FHRItemInstance>& Items, int32 Echoes);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") FName LootTableId = TEXT("Loot.Chest.Common");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") FName RequiredKeyItem;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") bool bConsumeKey = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") float MimicChance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") bool bIsLootBag = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") TArray<FHRItemInstance> PrerolledItems;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") int32 PrerolledEchoes = 0;

protected:
	bool IsOpened() const;
	void SetOpenedVisuals();
	void SpawnMimic(AHRPlayerCharacter* Player);

	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Lid;
};
