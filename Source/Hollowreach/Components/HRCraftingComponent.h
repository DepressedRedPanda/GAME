#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/HRItemTypes.h"
#include "HRCraftingComponent.generated.h"

class UHRInventoryComponent;

/**
 * Crafting from gathered and scavenged materials, plus weapon upgrades at a forge.
 * The available station is set by whatever the player is resting at.
 */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRCraftingComponent();

	UFUNCTION(BlueprintCallable, Category="Crafting") void SetStation(EHRCraftStation InStation, bool bInHasForge);
	UFUNCTION(BlueprintPure, Category="Crafting") EHRCraftStation GetStation() const { return Station; }

	/** Recipes whose required knowledge the player has. */
	UFUNCTION(BlueprintPure, Category="Crafting") TArray<FHRRecipe> GetKnownRecipes() const;
	UFUNCTION(BlueprintPure, Category="Crafting") bool CanCraft(const FHRRecipe& Recipe, FText& OutReason) const;
	UFUNCTION(BlueprintCallable, Category="Crafting") bool Craft(FName RecipeId);

	UFUNCTION(BlueprintPure, Category="Crafting") bool GetUpgradeCost(const FGuid& ItemGuid, FName& OutMaterial, int32& OutMaterialCount, int32& OutEchoes) const;
	UFUNCTION(BlueprintPure, Category="Crafting") bool CanUpgrade(const FGuid& ItemGuid, FText& OutReason) const;
	UFUNCTION(BlueprintCallable, Category="Crafting") bool UpgradeItem(const FGuid& ItemGuid);

	static FText DescribeRecipe(const UObject* WorldContextObject, const FHRRecipe& Recipe);

private:
	UHRInventoryComponent* GetInventory() const;

	EHRCraftStation Station = EHRCraftStation::None;
	bool bHasForge = false;
};
