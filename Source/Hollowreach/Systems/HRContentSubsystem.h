#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Items/HRItemTypes.h"
#include "Narrative/HRNarrativeTypes.h"
#include "Progression/HRSkillTypes.h"
#include "HRContentSubsystem.generated.h"

class UHRAbility;

/**
 * Central registry of game content: items, recipes, loot tables, skills, quests, dialogue,
 * lore, factions and endings. Ships with a built-in content set (see HRBuiltinContent.cpp) so the
 * game is playable from code alone; designers can register more from Blueprint or data assets.
 */
UCLASS()
class HOLLOWREACH_API UHRContentSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UHRContentSubsystem* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="Content") void RegisterItem(UHRItemDefinition* Definition);
	UFUNCTION(BlueprintPure, Category="Content") UHRItemDefinition* FindItem(FName ItemId) const;

	const FHRRecipe* FindRecipe(FName RecipeId) const;
	const FHRLootTable* FindLootTable(FName TableId) const;
	const FHRSkillNode* FindSkill(FName SkillId) const;
	const FHRQuestDefinition* FindQuest(FName QuestId) const;
	const FHRDialogue* FindDialogue(FName DialogueId) const;
	const FHRLoreEntry* FindLore(FName EntryId) const;
	const FHRFactionDefinition* FindFaction(FName FactionId) const;
	TSubclassOf<UHRAbility> FindAbilityClass(FName AbilityId) const;

	FText GetLoreTitle(FName EntryId) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TMap<FName, TObjectPtr<UHRItemDefinition>> Items;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TArray<FHRRecipe> Recipes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TMap<FName, FHRLootTable> LootTables;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TArray<FHRSkillNode> Skills;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TMap<FName, FHRQuestDefinition> Quests;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TMap<FName, FHRDialogue> Dialogues;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TMap<FName, FHRLoreEntry> Lore;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TArray<FHRDeduction> Deductions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TMap<FName, FHRFactionDefinition> Factions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TArray<FHREndingDefinition> Endings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content") TMap<FName, TSubclassOf<UHRAbility>> AbilityClasses;

private:
	void RegisterBuiltinContent();

	// Builders used by the built-in content set.
	UHRItemDefinition* NewItem(FName Id, const FString& Name, const FString& Description, EHRItemCategory Category, int32 Value, float Weight);
	UHRItemDefinition* NewWeapon(FName Id, const FString& Name, const FString& Description, float Damage, float Posture, EHRAttribute Scaling, int32 ComboLength, float Speed, int32 Value, float Weight);
	UHRItemDefinition* NewArmor(FName Id, const FString& Name, const FString& Description, EHREquipSlot Slot, const TArray<FHRStatModifier>& Modifiers, int32 Value, float Weight);
	UHRItemDefinition* NewConsumable(FName Id, const FString& Name, const FString& Description, const FHRConsumableData& Data, int32 Value, int32 MaxStack);
	void NewRecipe(FName Id, const TArray<FHRItemAmount>& Inputs, FHRItemAmount Output, EHRCraftStation Station, FName RequiredKnowledge = NAME_None);
	void NewLore(FName Id, const FString& Title, const FString& Body);
	FHRSkillNode& NewSkill(FName Id, FName Branch, const FString& Name, const FString& Description, int32 Cost, const TArray<FName>& Prerequisites);

	void RegisterItemsAndRecipes();
	void RegisterLootTables();
	void RegisterSkills();
	void RegisterFactions();
	void RegisterLore();
	void RegisterQuests();
	void RegisterDialogues();
	void RegisterEndings();
	void RegisterVeilCampaign();
};
