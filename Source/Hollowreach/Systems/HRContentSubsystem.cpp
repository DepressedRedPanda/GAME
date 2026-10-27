#include "Systems/HRContentSubsystem.h"

#include "Abilities/HRAbility.h"
#include "Core/HRStatics.h"

void UHRContentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisterBuiltinContent();
}

UHRContentSubsystem* UHRContentSubsystem::Get(const UObject* WorldContextObject)
{
	return HRGetSubsystem<UHRContentSubsystem>(WorldContextObject);
}

void UHRContentSubsystem::RegisterItem(UHRItemDefinition* Definition)
{
	if (Definition && !Definition->ItemId.IsNone())
	{
		Items.Add(Definition->ItemId, Definition);
	}
}

UHRItemDefinition* UHRContentSubsystem::FindItem(FName ItemId) const
{
	const TObjectPtr<UHRItemDefinition>* Found = Items.Find(ItemId);
	return Found ? Found->Get() : nullptr;
}

const FHRRecipe* UHRContentSubsystem::FindRecipe(FName RecipeId) const
{
	return Recipes.FindByPredicate([RecipeId](const FHRRecipe& Recipe) { return Recipe.RecipeId == RecipeId; });
}

const FHRLootTable* UHRContentSubsystem::FindLootTable(FName TableId) const
{
	return LootTables.Find(TableId);
}

const FHRSkillNode* UHRContentSubsystem::FindSkill(FName SkillId) const
{
	return Skills.FindByPredicate([SkillId](const FHRSkillNode& Node) { return Node.SkillId == SkillId; });
}

const FHRQuestDefinition* UHRContentSubsystem::FindQuest(FName QuestId) const
{
	return Quests.Find(QuestId);
}

const FHRDialogue* UHRContentSubsystem::FindDialogue(FName DialogueId) const
{
	return Dialogues.Find(DialogueId);
}

const FHRLoreEntry* UHRContentSubsystem::FindLore(FName EntryId) const
{
	return Lore.Find(EntryId);
}

const FHRFactionDefinition* UHRContentSubsystem::FindFaction(FName FactionId) const
{
	return Factions.Find(FactionId);
}

TSubclassOf<UHRAbility> UHRContentSubsystem::FindAbilityClass(FName AbilityId) const
{
	const TSubclassOf<UHRAbility>* Found = AbilityClasses.Find(AbilityId);
	return Found ? *Found : nullptr;
}

FText UHRContentSubsystem::GetLoreTitle(FName EntryId) const
{
	const FHRLoreEntry* Entry = FindLore(EntryId);
	return Entry ? Entry->Title : FText::FromName(EntryId);
}

// --- Builders ---------------------------------------------------------------------------------

UHRItemDefinition* UHRContentSubsystem::NewItem(FName Id, const FString& Name, const FString& Description, EHRItemCategory Category, int32 Value, float Weight)
{
	// Object names may not contain '.', item ids do ("Item.IronBlade").
	const FName ObjectName(*Id.ToString().Replace(TEXT("."), TEXT("_")));
	UHRItemDefinition* Item = NewObject<UHRItemDefinition>(this, MakeUniqueObjectName(this, UHRItemDefinition::StaticClass(), ObjectName));
	Item->ItemId = Id;
	Item->DisplayName = FText::FromString(Name);
	Item->Description = FText::FromString(Description);
	Item->Category = Category;
	Item->BaseValue = Value;
	Item->Weight = Weight;
	Item->MaxStack = (Category == EHRItemCategory::Material || Category == EHRItemCategory::Consumable) ? 99 : 1;
	RegisterItem(Item);
	return Item;
}

UHRItemDefinition* UHRContentSubsystem::NewWeapon(FName Id, const FString& Name, const FString& Description, float Damage, float Posture, EHRAttribute Scaling, int32 ComboLength, float Speed, int32 Value, float Weight)
{
	UHRItemDefinition* Item = NewItem(Id, Name, Description, EHRItemCategory::Weapon, Value, Weight);
	Item->EquipSlot = EHREquipSlot::MainHand;
	Item->bCanRollAffixes = true;
	Item->Weapon.BaseDamage = Damage;
	Item->Weapon.BasePosture = Posture;
	Item->Weapon.ScalingAttribute = Scaling;
	Item->Weapon.ScalingFactor = 1.f;

	// Combos ramp up: the last hit of a chain hits harder and breaks more posture.
	const float TimeScale = 1.f / FMath::Max(0.25f, Speed);
	for (int32 Index = 0; Index < ComboLength; ++Index)
	{
		const bool bFinisher = Index == ComboLength - 1;
		FHRAttackStep Step;
		Step.DamageMultiplier = bFinisher ? 1.35f : 1.f + 0.05f * Index;
		Step.PostureMultiplier = bFinisher ? 1.5f : 1.f;
		Step.StaminaCost = 12.f * TimeScale;
		Step.Windup = (bFinisher ? 0.3f : 0.2f) * TimeScale;
		Step.ActiveTime = 0.13f * TimeScale;
		Step.Recovery = (bFinisher ? 0.45f : 0.3f) * TimeScale;
		Item->Weapon.LightCombo.Add(Step);
	}

	FHRAttackStep& Heavy = Item->Weapon.HeavyAttack;
	Heavy.DamageMultiplier = 1.9f;
	Heavy.PostureMultiplier = 2.2f;
	Heavy.StaminaCost = 26.f * TimeScale;
	Heavy.Windup = 0.55f * TimeScale;
	Heavy.ActiveTime = 0.18f * TimeScale;
	Heavy.Recovery = 0.55f * TimeScale;
	Heavy.Reach = 220.f;
	Heavy.Radius = 80.f;
	Heavy.LungeSpeed = 650.f;
	return Item;
}

UHRItemDefinition* UHRContentSubsystem::NewArmor(FName Id, const FString& Name, const FString& Description, EHREquipSlot Slot, const TArray<FHRStatModifier>& Modifiers, int32 Value, float Weight)
{
	const EHRItemCategory Category = Slot == EHREquipSlot::Trinket ? EHRItemCategory::Trinket : EHRItemCategory::Armor;
	UHRItemDefinition* Item = NewItem(Id, Name, Description, Category, Value, Weight);
	Item->EquipSlot = Slot;
	Item->Modifiers = Modifiers;
	Item->bCanRollAffixes = true;
	return Item;
}

UHRItemDefinition* UHRContentSubsystem::NewConsumable(FName Id, const FString& Name, const FString& Description, const FHRConsumableData& Data, int32 Value, int32 MaxStack)
{
	UHRItemDefinition* Item = NewItem(Id, Name, Description, EHRItemCategory::Consumable, Value, 0.3f);
	Item->Consumable = Data;
	Item->MaxStack = MaxStack;
	return Item;
}

void UHRContentSubsystem::NewRecipe(FName Id, const TArray<FHRItemAmount>& Inputs, FHRItemAmount Output, EHRCraftStation Station, FName RequiredKnowledge)
{
	FHRRecipe Recipe;
	Recipe.RecipeId = Id;
	Recipe.Inputs = Inputs;
	Recipe.Output = Output;
	Recipe.Station = Station;
	Recipe.RequiredKnowledge = RequiredKnowledge;
	Recipes.Add(Recipe);
}

void UHRContentSubsystem::NewLore(FName Id, const FString& Title, const FString& Body)
{
	FHRLoreEntry Entry;
	Entry.EntryId = Id;
	Entry.Title = FText::FromString(Title);
	Entry.Body = FText::FromString(Body);
	Lore.Add(Id, Entry);
}

FHRSkillNode& UHRContentSubsystem::NewSkill(FName Id, FName Branch, const FString& Name, const FString& Description, int32 Cost, const TArray<FName>& Prerequisites)
{
	FHRSkillNode& Node = Skills.AddDefaulted_GetRef();
	Node.SkillId = Id;
	Node.Branch = Branch;
	Node.DisplayName = FText::FromString(Name);
	Node.Description = FText::FromString(Description);
	Node.Cost = Cost;
	Node.Prerequisites = Prerequisites;
	return Node;
}
