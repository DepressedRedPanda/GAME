#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Items/HRItemTypes.h"
#include "HRLootLibrary.generated.h"

/** Randomized, seedable loot: weighted tables, rarity rolls shifted by luck, and Diablo-style affixes. */
UCLASS()
class HOLLOWREACH_API UHRLootLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Loot", meta=(WorldContext="WorldContextObject"))
	static void RollLoot(const UObject* WorldContextObject, FName TableId, int32 Seed, float Luck, TArray<FHRItemInstance>& OutItems, int32& OutEchoes);

	/** Creates an instance, rolling rarity/affixes if the definition allows it. */
	static FHRItemInstance MakeInstance(const UObject* WorldContextObject, FName ItemId, int32 Quantity, FRandomStream& Stream, float Luck, EHRItemRarity MinRarity = EHRItemRarity::Common);

	static EHRItemRarity RollRarity(FRandomStream& Stream, float Luck, EHRItemRarity MinRarity);
	static void RollAffixes(const class UHRItemDefinition* Definition, EHRItemRarity Rarity, FRandomStream& Stream, TArray<FHRStatModifier>& OutAffixes);

	UFUNCTION(BlueprintPure, Category="Loot", meta=(WorldContext="WorldContextObject"))
	static FText GetItemDisplayName(const UObject* WorldContextObject, const FHRItemInstance& Item);

	UFUNCTION(BlueprintPure, Category="Loot")
	static FText DescribeModifier(const FHRStatModifier& Modifier);
};
