// The built-in content set: everything needed to play Hollowreach from code alone.
// Designers can add more at runtime through UHRContentSubsystem's public maps, or replace
// these definitions with data assets.

#include "Systems/HRContentSubsystem.h"

#include "Abilities/HRAbilities.h"

namespace
{
	FText Txt(const TCHAR* String)
	{
		return FText::FromString(String);
	}

	FHRDialogueNode MakeNode(FName Id, const TCHAR* Speaker, const TCHAR* Line)
	{
		FHRDialogueNode Node;
		Node.NodeId = Id;
		Node.Speaker = Txt(Speaker);
		Node.Line = Txt(Line);
		return Node;
	}

	FHRDialogueChoice MakeChoice(const TCHAR* Text, FName Next, const TArray<FHRConsequence>& Consequences = {}, const TArray<FHRCondition>& Conditions = {})
	{
		FHRDialogueChoice Choice;
		Choice.Text = Txt(Text);
		Choice.NextNode = Next;
		Choice.Consequences = Consequences;
		Choice.Conditions = Conditions;
		return Choice;
	}

	FHRDialogueEntryPoint MakeEntry(FName Node, const TArray<FHRCondition>& Conditions)
	{
		FHRDialogueEntryPoint Entry;
		Entry.NodeId = Node;
		Entry.Conditions = Conditions;
		return Entry;
	}

	FHRQuestObjective MakeObjective(const TCHAR* Description, EHRObjectiveType Type, FName Target, int32 Count = 1, bool bOptional = false)
	{
		FHRQuestObjective Objective;
		Objective.Description = Txt(Description);
		Objective.Type = Type;
		Objective.Target = Target;
		Objective.Count = Count;
		Objective.bOptional = bOptional;
		return Objective;
	}

	FHRQuestStage MakeStage(FName Id, const TCHAR* Journal, const TArray<FHRQuestObjective>& Objectives, FName Next, bool bCompletes = false)
	{
		FHRQuestStage Stage;
		Stage.StageId = Id;
		Stage.JournalText = Txt(Journal);
		Stage.Objectives = Objectives;
		Stage.NextStage = Next;
		Stage.bCompletesQuest = bCompletes;
		return Stage;
	}

	FHRLootEntry MakeLoot(FName Item, float Weight, int32 Min = 1, int32 Max = 1, EHRItemRarity MinRarity = EHRItemRarity::Common)
	{
		FHRLootEntry Entry;
		Entry.ItemId = Item;
		Entry.Weight = Weight;
		Entry.MinQuantity = Min;
		Entry.MaxQuantity = Max;
		Entry.MinRarity = MinRarity;
		return Entry;
	}

	using C = EHRConsequenceType;
	using K = EHRConditionType;
}

void UHRContentSubsystem::RegisterBuiltinContent()
{
	RegisterItemsAndRecipes();
	RegisterLootTables();
	RegisterSkills();
	RegisterFactions();
	RegisterLore();
	RegisterQuests();
	RegisterDialogues();
	RegisterEndings();
	RegisterVeilCampaign();

	AbilityClasses.Add(TEXT("Ability.Firebolt"), UHRAbility_Firebolt::StaticClass());
	AbilityClasses.Add(TEXT("Ability.KineticPush"), UHRAbility_KineticPush::StaticClass());
	AbilityClasses.Add(TEXT("Ability.Blink"), UHRAbility_Blink::StaticClass());
	AbilityClasses.Add(TEXT("Ability.Ward"), UHRAbility_Ward::StaticClass());
	AbilityClasses.Add(TEXT("Ability.ShadowVeil"), UHRAbility_ShadowVeil::StaticClass());
	AbilityClasses.Add(TEXT("Ability.SpiritSight"), UHRAbility_SpiritSight::StaticClass());
	AbilityClasses.Add(TEXT("Ability.BeastForm"), UHRAbility_BeastForm::StaticClass());
}

// --- Items -------------------------------------------------------------------------------------

void UHRContentSubsystem::RegisterItemsAndRecipes()
{
	// Weapons: (damage, posture, scaling, combo length, speed, value, weight)
	UHRItemDefinition* IronBlade = NewWeapon(TEXT("Item.IronBlade"), TEXT("Iron Blade"), TEXT("A dependable sword. Rewards a steady rhythm."), 20.f, 13.f, EHRAttribute::Strength, 3, 1.f, 120, 3.f);
	IronBlade->Weapon.UpgradeMaterial = TEXT("Item.IronIngot");

	UHRItemDefinition* Fangs = NewWeapon(TEXT("Item.TwinFangs"), TEXT("Twin Fangs"), TEXT("Paired daggers. Five-strike chains for those who keep the beat."), 12.f, 8.f, EHRAttribute::Dexterity, 5, 1.45f, 320, 1.5f);
	for (FHRAttackStep& Step : Fangs->Weapon.LightCombo)
	{
		Step.Reach = 160.f;
		Step.LungeSpeed = 520.f;
	}

	UHRItemDefinition* Greatsword = NewWeapon(TEXT("Item.WardenGreatsword"), TEXT("Warden Greatsword"), TEXT("The Warden's own blade. Slow, but it shatters guards."), 34.f, 27.f, EHRAttribute::Strength, 2, 0.7f, 900, 9.f);
	Greatsword->Weapon.HeavyAttack.Reach = 270.f;
	Greatsword->Weapon.HeavyAttack.Radius = 95.f;
	for (FHRAttackStep& Step : Greatsword->Weapon.LightCombo)
	{
		Step.Reach = 240.f;
		Step.Radius = 85.f;
	}
	Greatsword->Weapon.VisualLength = 170.f;

	UHRItemDefinition* Staff = NewWeapon(TEXT("Item.EmberStaff"), TEXT("Ember Staff"), TEXT("Strikes burn. Scales with Arcane; lights braziers on contact."), 14.f, 10.f, EHRAttribute::Arcane, 3, 0.95f, 600, 2.f);
	Staff->Weapon.DamageType = EHRDamageType::Fire;
	Staff->Weapon.ScalingFactor = 1.5f;
	Staff->Modifiers = { FHRStatModifier(EHRStat::ArcanePower, 5.f) };

	NewWeapon(TEXT("Item.AshbornSaber"), TEXT("Ashborn Saber"), TEXT("A curved miner's blade, reforged for war."), 17.f, 11.f, EHRAttribute::Dexterity, 4, 1.15f, 350, 2.5f);

	// Armor and trinkets.
	NewArmor(TEXT("Item.LeatherCoat"), TEXT("Leather Coat"), TEXT("Worn, patched, warm enough."), EHREquipSlot::Chest, { FHRStatModifier(EHRStat::Defense, 8.f), FHRStatModifier(EHRStat::Insulation, 4.f) }, 60, 4.f);
	NewArmor(TEXT("Item.LeatherHood"), TEXT("Leather Hood"), TEXT("Keeps your face in shadow."), EHREquipSlot::Head, { FHRStatModifier(EHRStat::Defense, 4.f), FHRStatModifier(EHRStat::Stealth, 0.05f) }, 70, 1.f);
	NewArmor(TEXT("Item.FurCloak"), TEXT("Fur Cloak"), TEXT("Thick fur against the peak's cold."), EHREquipSlot::Chest, { FHRStatModifier(EHRStat::Defense, 6.f), FHRStatModifier(EHRStat::Insulation, 15.f), FHRStatModifier(EHRStat::FrostResist, 0.15f) }, 180, 5.f);
	NewArmor(TEXT("Item.WardenPlate"), TEXT("Warden Plate"), TEXT("Heavy oath-bound plate. You will not be moved, nor move quickly."), EHREquipSlot::Chest,
		{ FHRStatModifier(EHRStat::Defense, 30.f), FHRStatModifier(EHRStat::MaxPosture, 30.f), FHRStatModifier(EHRStat::MoveSpeed, 0.f, 0.92f) }, 900, 14.f);
	NewArmor(TEXT("Item.RangerGloves"), TEXT("Ranger Gloves"), TEXT("Sure grip, sure strike."), EHREquipSlot::Hands, { FHRStatModifier(EHRStat::Defense, 3.f), FHRStatModifier(EHRStat::CritChance, 0.03f) }, 90, 0.5f);
	NewArmor(TEXT("Item.TravelerBoots"), TEXT("Traveler Boots"), TEXT("Light boots for long roads."), EHREquipSlot::Legs, { FHRStatModifier(EHRStat::Defense, 3.f), FHRStatModifier(EHRStat::MoveSpeed, 0.f, 1.05f) }, 90, 1.f);
	NewArmor(TEXT("Item.EmberRing"), TEXT("Ember Ring"), TEXT("Warm to the touch."), EHREquipSlot::Trinket, { FHRStatModifier(EHRStat::ArcanePower, 8.f), FHRStatModifier(EHRStat::FireResist, 0.1f) }, 400, 0.1f);
	NewArmor(TEXT("Item.OwlCharm"), TEXT("Owl Charm"), TEXT("Silent wings, silent steps."), EHREquipSlot::Trinket, { FHRStatModifier(EHRStat::Stealth, 0.12f) }, 400, 0.1f);
	NewArmor(TEXT("Item.LuckyCoin"), TEXT("Lucky Coin"), TEXT("Heads, you find something better."), EHREquipSlot::Trinket, { FHRStatModifier(EHRStat::LootLuck, 0.4f) }, 400, 0.1f);
	UHRItemDefinition* Sigil = NewArmor(TEXT("Item.WardenSigil"), TEXT("Warden's Sigil"), TEXT("Proof the Warden fell. It still hums with his oath."), EHREquipSlot::Trinket,
		{ FHRStatModifier(EHRStat::MaxHealth, 60.f), FHRStatModifier(EHRStat::PostureRegen, 0.f, 1.2f) }, 1, 0.1f);
	Sigil->bCanRollAffixes = false;

	// Consumables.
	auto Consumable = [this](FName Id, const TCHAR* Name, const TCHAR* Description, int32 Value, TFunctionRef<void(FHRConsumableData&)> Fill)
	{
		FHRConsumableData Data;
		Fill(Data);
		return NewConsumable(Id, Name, Description, Data, Value, 99);
	};
	Consumable(TEXT("Item.EmberGourd"), TEXT("Ember Gourd"), TEXT("A draught of living ember. Refills when you rest at a shrine."), 1, [](FHRConsumableData& D) { D.Heal = 110.f; D.UseTime = 0.75f; });
	Consumable(TEXT("Item.Herb"), TEXT("Bitterleaf"), TEXT("Chew it. Closes small wounds."), 6, [](FHRConsumableData& D) { D.Heal = 20.f; D.Food = 3.f; D.UseTime = 0.5f; });
	Consumable(TEXT("Item.Poultice"), TEXT("Herbal Poultice"), TEXT("Packed herbs. Heals and steadies breath."), 25, [](FHRConsumableData& D) { D.Heal = 60.f; D.RestoreStamina = 30.f; D.UseTime = 0.9f; });
	Consumable(TEXT("Item.Bandage"), TEXT("Bandage"), TEXT("Binds wounds and broken bones. Cures injuries."), 20, [](FHRConsumableData& D) { D.Heal = 10.f; D.bCuresInjuries = true; D.UseTime = 1.2f; });
	Consumable(TEXT("Item.Splint"), TEXT("Splint"), TEXT("A proper splint. Cures injuries and heals a little."), 30, [](FHRConsumableData& D) { D.Heal = 25.f; D.bCuresInjuries = true; D.UseTime = 1.5f; });
	Consumable(TEXT("Item.Bread"), TEXT("Hard Bread"), TEXT("Food that keeps."), 5, [](FHRConsumableData& D) { D.Food = 30.f; });
	Consumable(TEXT("Item.RawMeat"), TEXT("Raw Meat"), TEXT("Better cooked."), 4, [](FHRConsumableData& D) { D.Food = 12.f; });
	Consumable(TEXT("Item.CookedMeat"), TEXT("Roast Meat"), TEXT("Hot, filling, restorative."), 15, [](FHRConsumableData& D) { D.Food = 45.f; D.Heal = 15.f; });
	Consumable(TEXT("Item.Mushroom"), TEXT("Glowcap"), TEXT("Faintly luminous. Sharpens the mind."), 6, [](FHRConsumableData& D) { D.Food = 8.f; D.RestoreFocus = 10.f; });
	Consumable(TEXT("Item.Waterskin"), TEXT("Waterskin"), TEXT("Clean water."), 4, [](FHRConsumableData& D) { D.Water = 45.f; });
	Consumable(TEXT("Item.HotTea"), TEXT("Pine Tea"), TEXT("Warms you to the bone."), 12, [](FHRConsumableData& D) { D.Water = 20.f; D.Warmth = 35.f; D.Rest = 10.f; });
	Consumable(TEXT("Item.FocusTonic"), TEXT("Focus Tonic"), TEXT("Restores focus for spells."), 40, [](FHRConsumableData& D) { D.RestoreFocus = 50.f; });
	Consumable(TEXT("Item.StaminaDraught"), TEXT("Stamina Draught"), TEXT("For 90 seconds, you recover breath faster."), 45, [](FHRConsumableData& D)
	{
		D.Buff = { FHRStatModifier(EHRStat::StaminaRegen, 0.f, 1.5f), FHRStatModifier(EHRStat::MaxStamina, 20.f) };
		D.BuffDuration = 90.f;
	});
	Consumable(TEXT("Item.Firebomb"), TEXT("Firebomb"), TEXT("Throw it. Burns enemies, lights braziers, detonates barrels."), 30, [](FHRConsumableData& D)
	{
		D.bThrown = true;
		D.ThrownElement = EHRDamageType::Fire;
		D.ThrownDamage = 55.f;
		D.UseTime = 0.5f;
	});

	// Materials.
	NewItem(TEXT("Item.IronOre"), TEXT("Iron Ore"), TEXT("Smelt two at a forge for an ingot."), EHRItemCategory::Material, 5, 1.f);
	NewItem(TEXT("Item.IronIngot"), TEXT("Iron Ingot"), TEXT("Reinforces iron weapons."), EHRItemCategory::Material, 20, 1.f);
	NewItem(TEXT("Item.Wood"), TEXT("Deadwood"), TEXT("Dry and useful."), EHRItemCategory::Material, 3, 1.f);
	NewItem(TEXT("Item.Cloth"), TEXT("Cloth Scraps"), TEXT("For bandages and bombs."), EHRItemCategory::Material, 5, 0.2f);
	NewItem(TEXT("Item.Oil"), TEXT("Lamp Oil"), TEXT("Flammable."), EHRItemCategory::Material, 8, 0.5f);
	NewItem(TEXT("Item.Leather"), TEXT("Hide"), TEXT("Tanned hide."), EHRItemCategory::Material, 8, 0.5f);
	NewItem(TEXT("Item.HollowAsh"), TEXT("Hollow Ash"), TEXT("What remains of the Hollow. Strengthens weapons."), EHRItemCategory::Material, 15, 0.1f);
	NewItem(TEXT("Item.Moonstone"), TEXT("Moonstone"), TEXT("Rare. Needed to push a weapon beyond +5."), EHRItemCategory::Material, 150, 0.2f);

	// Key and lore items.
	UHRItemDefinition* Key = NewItem(TEXT("Item.RuinsKey"), TEXT("Ruins Key"), TEXT("Taken from Captain Varn. Opens a vault in the Sunken Ruins."), EHRItemCategory::KeyItem, 0, 0.f);
	Key->MaxStack = 1;
	UHRItemDefinition* Journal = NewItem(TEXT("Item.WardenJournal"), TEXT("Warden's Journal"), TEXT("\"I swore to keep the flame. I did not swear to let it keep me.\""), EHRItemCategory::Lore, 0, 0.1f);
	Journal->GrantsKnowledge = TEXT("Lore.WardenOath");
	Journal->MaxStack = 1;

	// Recipes.
	NewRecipe(TEXT("Recipe.Poultice"), { FHRItemAmount(TEXT("Item.Herb"), 2) }, FHRItemAmount(TEXT("Item.Poultice"), 1), EHRCraftStation::None);
	NewRecipe(TEXT("Recipe.Bandage"), { FHRItemAmount(TEXT("Item.Cloth"), 2) }, FHRItemAmount(TEXT("Item.Bandage"), 1), EHRCraftStation::None);
	NewRecipe(TEXT("Recipe.Splint"), { FHRItemAmount(TEXT("Item.Wood"), 1), FHRItemAmount(TEXT("Item.Cloth"), 1) }, FHRItemAmount(TEXT("Item.Splint"), 1), EHRCraftStation::None);
	NewRecipe(TEXT("Recipe.Firebomb"), { FHRItemAmount(TEXT("Item.Oil"), 1), FHRItemAmount(TEXT("Item.Cloth"), 1) }, FHRItemAmount(TEXT("Item.Firebomb"), 2), EHRCraftStation::None);
	NewRecipe(TEXT("Recipe.CookedMeat"), { FHRItemAmount(TEXT("Item.RawMeat"), 1) }, FHRItemAmount(TEXT("Item.CookedMeat"), 1), EHRCraftStation::Campfire);
	NewRecipe(TEXT("Recipe.HotTea"), { FHRItemAmount(TEXT("Item.Herb"), 1), FHRItemAmount(TEXT("Item.Waterskin"), 1) }, FHRItemAmount(TEXT("Item.HotTea"), 1), EHRCraftStation::Campfire);
	NewRecipe(TEXT("Recipe.IronIngot"), { FHRItemAmount(TEXT("Item.IronOre"), 2) }, FHRItemAmount(TEXT("Item.IronIngot"), 1), EHRCraftStation::Forge);
	NewRecipe(TEXT("Recipe.FurCloak"), { FHRItemAmount(TEXT("Item.Leather"), 3), FHRItemAmount(TEXT("Item.Cloth"), 2) }, FHRItemAmount(TEXT("Item.FurCloak"), 1), EHRCraftStation::Forge);
	NewRecipe(TEXT("Recipe.FocusTonic"), { FHRItemAmount(TEXT("Item.Mushroom"), 2), FHRItemAmount(TEXT("Item.Herb"), 1) }, FHRItemAmount(TEXT("Item.FocusTonic"), 1), EHRCraftStation::Campfire, TEXT("Lore.Alchemy"));
	NewRecipe(TEXT("Recipe.StaminaDraught"), { FHRItemAmount(TEXT("Item.Mushroom"), 2), FHRItemAmount(TEXT("Item.Waterskin"), 1) }, FHRItemAmount(TEXT("Item.StaminaDraught"), 1), EHRCraftStation::Campfire, TEXT("Lore.Alchemy"));
}

// --- Loot --------------------------------------------------------------------------------------

void UHRContentSubsystem::RegisterLootTables()
{
	auto Table = [this](FName Id, int32 MinRolls, int32 MaxRolls, float Empty, int32 MinEchoes, int32 MaxEchoes) -> FHRLootTable&
	{
		FHRLootTable& LootTable = LootTables.Add(Id);
		LootTable.TableId = Id;
		LootTable.MinRolls = MinRolls;
		LootTable.MaxRolls = MaxRolls;
		LootTable.EmptyChance = Empty;
		LootTable.MinEchoes = MinEchoes;
		LootTable.MaxEchoes = MaxEchoes;
		return LootTable;
	};

	FHRLootTable& Hollow = Table(TEXT("Loot.Hollow"), 1, 2, 0.35f, 5, 25);
	Hollow.Entries = { MakeLoot(TEXT("Item.HollowAsh"), 5.f, 1, 2), MakeLoot(TEXT("Item.Herb"), 2.f), MakeLoot(TEXT("Item.Cloth"), 2.f), MakeLoot(TEXT("Item.RawMeat"), 1.5f), MakeLoot(TEXT("Item.Oil"), 1.f), MakeLoot(TEXT("Item.Leather"), 1.f) };

	FHRLootTable& Ashborn = Table(TEXT("Loot.Ashborn"), 1, 2, 0.2f, 20, 50);
	Ashborn.Entries = { MakeLoot(TEXT("Item.Cloth"), 3.f, 1, 2), MakeLoot(TEXT("Item.Oil"), 2.f), MakeLoot(TEXT("Item.Firebomb"), 1.f), MakeLoot(TEXT("Item.Bread"), 2.f), MakeLoot(TEXT("Item.IronOre"), 2.f, 1, 2),
		MakeLoot(TEXT("Item.AshbornSaber"), 0.3f), MakeLoot(TEXT("Item.LeatherHood"), 0.4f) };

	FHRLootTable& Captain = Table(TEXT("Loot.Captain"), 1, 1, 0.f, 200, 300);
	Captain.Guaranteed = { FHRItemAmount(TEXT("Item.RuinsKey"), 1) };
	Captain.Entries = { MakeLoot(TEXT("Item.AshbornSaber"), 1.f, 1, 1, EHRItemRarity::Uncommon) };

	FHRLootTable& Common = Table(TEXT("Loot.Chest.Common"), 2, 3, 0.f, 30, 80);
	Common.Entries = { MakeLoot(TEXT("Item.Herb"), 3.f, 1, 3), MakeLoot(TEXT("Item.Bandage"), 2.f), MakeLoot(TEXT("Item.Bread"), 2.f), MakeLoot(TEXT("Item.Waterskin"), 2.f),
		MakeLoot(TEXT("Item.IronOre"), 2.f, 1, 3), MakeLoot(TEXT("Item.Leather"), 1.5f), MakeLoot(TEXT("Item.HollowAsh"), 2.f, 1, 3),
		MakeLoot(TEXT("Item.LeatherHood"), 0.6f), MakeLoot(TEXT("Item.TravelerBoots"), 0.6f), MakeLoot(TEXT("Item.RangerGloves"), 0.6f), MakeLoot(TEXT("Item.Firebomb"), 1.f, 1, 2) };

	FHRLootTable& Rare = Table(TEXT("Loot.Chest.Rare"), 2, 3, 0.f, 100, 250);
	Rare.Entries = { MakeLoot(TEXT("Item.TwinFangs"), 1.f, 1, 1, EHRItemRarity::Uncommon), MakeLoot(TEXT("Item.EmberStaff"), 1.f, 1, 1, EHRItemRarity::Uncommon),
		MakeLoot(TEXT("Item.EmberRing"), 1.f, 1, 1, EHRItemRarity::Uncommon), MakeLoot(TEXT("Item.OwlCharm"), 1.f, 1, 1, EHRItemRarity::Uncommon),
		MakeLoot(TEXT("Item.LuckyCoin"), 0.8f, 1, 1, EHRItemRarity::Uncommon), MakeLoot(TEXT("Item.FurCloak"), 1.f, 1, 1, EHRItemRarity::Uncommon),
		MakeLoot(TEXT("Item.Moonstone"), 2.f), MakeLoot(TEXT("Item.HollowAsh"), 2.f, 2, 4), MakeLoot(TEXT("Item.StaminaDraught"), 1.f) };

	FHRLootTable& Boss = Table(TEXT("Loot.Boss"), 1, 2, 0.f, 0, 0);
	Boss.Guaranteed = { FHRItemAmount(TEXT("Item.Moonstone"), 2), FHRItemAmount(TEXT("Item.WardenJournal"), 1) };
	Boss.Entries = { MakeLoot(TEXT("Item.WardenPlate"), 1.f, 1, 1, EHRItemRarity::Rare), MakeLoot(TEXT("Item.WardenGreatsword"), 1.f, 1, 1, EHRItemRarity::Rare) };
}

// --- Skills ------------------------------------------------------------------------------------

void UHRContentSubsystem::RegisterSkills()
{
	// Blade
	NewSkill(TEXT("Skill.DeflectMastery"), TEXT("Blade"), TEXT("Deflect Mastery"), TEXT("Wider deflect window."), 1, {}).Modifiers = { FHRStatModifier(EHRStat::ParryWindow, 0.06f) };
	NewSkill(TEXT("Skill.Riposte"), TEXT("Blade"), TEXT("Riposte"), TEXT("Your blows break posture 15% faster."), 1, { TEXT("Skill.DeflectMastery") }).Modifiers = { FHRStatModifier(EHRStat::PostureDamage, 0.15f) };
	NewSkill(TEXT("Skill.Bladedance"), TEXT("Blade"), TEXT("Bladedance"), TEXT("More power and faster stamina recovery."), 2, { TEXT("Skill.Riposte") }).Modifiers = { FHRStatModifier(EHRStat::PhysicalPower, 6.f), FHRStatModifier(EHRStat::StaminaRegen, 6.f) };
	NewSkill(TEXT("Skill.IronWill"), TEXT("Blade"), TEXT("Iron Will"), TEXT("Much harder to break your posture."), 1, {}).Modifiers = { FHRStatModifier(EHRStat::MaxPosture, 40.f) };

	// Shadow
	NewSkill(TEXT("Skill.SilentStep"), TEXT("Shadow"), TEXT("Silent Step"), TEXT("Harder to see and hear."), 1, {}).Modifiers = { FHRStatModifier(EHRStat::Stealth, 0.15f) };
	FHRSkillNode& WallRun = NewSkill(TEXT("Skill.WallRun"), TEXT("Shadow"), TEXT("Wall Run"), TEXT("Sprint along walls; jump off them."), 1, { TEXT("Skill.SilentStep") });
	WallRun.bGrantsTraversal = true;
	WallRun.Traversal = EHRTraversalAbility::WallRun;
	NewSkill(TEXT("Skill.ShadowVeil"), TEXT("Shadow"), TEXT("Shadow Veil"), TEXT("Ability: fade from sight for 8 seconds."), 1, { TEXT("Skill.SilentStep") }).GrantAbility = TEXT("Ability.ShadowVeil");
	NewSkill(TEXT("Skill.Blink"), TEXT("Shadow"), TEXT("Blink"), TEXT("Ability: short-range teleport through enemies."), 2, { TEXT("Skill.WallRun") }).GrantAbility = TEXT("Ability.Blink");
	FHRSkillNode& Grapple = NewSkill(TEXT("Skill.Grapple"), TEXT("Shadow"), TEXT("Grappling Hook"), TEXT("Jump in mid-air at a glowing anchor to pull yourself to it."), 2, { TEXT("Skill.WallRun") });
	Grapple.bGrantsTraversal = true;
	Grapple.Traversal = EHRTraversalAbility::Grapple;

	// Arcane
	NewSkill(TEXT("Skill.Firebolt"), TEXT("Arcane"), TEXT("Firebolt"), TEXT("Ability: hurl fire. Lights braziers, detonates barrels."), 1, {}).GrantAbility = TEXT("Ability.Firebolt");
	NewSkill(TEXT("Skill.KineticPush"), TEXT("Arcane"), TEXT("Kinetic Push"), TEXT("Ability: a wave of force that throws objects and staggers foes."), 1, {}).GrantAbility = TEXT("Ability.KineticPush");
	NewSkill(TEXT("Skill.Ward"), TEXT("Arcane"), TEXT("Ember Ward"), TEXT("Ability: a shield of embers."), 1, { TEXT("Skill.Firebolt") }).GrantAbility = TEXT("Ability.Ward");
	FHRSkillNode& Sight = NewSkill(TEXT("Skill.SpiritSight"), TEXT("Arcane"), TEXT("Spirit Sight"), TEXT("Ability: see secrets, clues and enemies through walls. Must first learn the Old Tongue."), 1, {});
	Sight.GrantAbility = TEXT("Ability.SpiritSight");
	Sight.RequiredKnowledge = TEXT("Lore.OldTongue");
	FHRSkillNode& Beast = NewSkill(TEXT("Skill.BeastForm"), TEXT("Arcane"), TEXT("Beast Form"), TEXT("Ability: transform into a hulking beast."), 3, { TEXT("Skill.Ward"), TEXT("Skill.KineticPush") });
	Beast.GrantAbility = TEXT("Ability.BeastForm");
	Beast.RequiredLevel = 8;

	// Survival
	NewSkill(TEXT("Skill.ThickHide"), TEXT("Survival"), TEXT("Thick Hide"), TEXT("Resist cold."), 1, {}).Modifiers = { FHRStatModifier(EHRStat::Insulation, 12.f), FHRStatModifier(EHRStat::FrostResist, 0.1f) };
	NewSkill(TEXT("Skill.PackMule"), TEXT("Survival"), TEXT("Pack Mule"), TEXT("Carry 30 more weight."), 1, {}).Modifiers = { FHRStatModifier(EHRStat::CarryCapacity, 30.f) };
	NewSkill(TEXT("Skill.Scavenger"), TEXT("Survival"), TEXT("Scavenger"), TEXT("Better loot from every source."), 1, { TEXT("Skill.PackMule") }).Modifiers = { FHRStatModifier(EHRStat::LootLuck, 0.3f) };
	NewSkill(TEXT("Skill.SecondWind"), TEXT("Survival"), TEXT("Second Wind"), TEXT("A second resurrection charge."), 2, { TEXT("Skill.ThickHide") });
}

// --- Factions ----------------------------------------------------------------------------------

void UHRContentSubsystem::RegisterFactions()
{
	auto Faction = [this](FName Id, const TCHAR* Name, float Start, const TMap<FName, float>& Relations, float Max = 100.f)
	{
		FHRFactionDefinition Definition;
		Definition.FactionId = Id;
		Definition.DisplayName = Txt(Name);
		Definition.StartingReputation = Start;
		Definition.Relations = Relations;
		Definition.MaxReputation = Max;
		Factions.Add(Id, Definition);
	};
	Faction(TEXT("Villagers"), TEXT("Emberfall Villagers"), 10.f, { { TEXT("Hollow"), -100.f }, { TEXT("Ashborn"), -40.f }, { TEXT("Wardens"), 60.f } });
	Faction(TEXT("Wardens"), TEXT("Order of the Flame"), 0.f, { { TEXT("Hollow"), -100.f }, { TEXT("Ashborn"), -80.f }, { TEXT("Villagers"), 60.f } });
	Faction(TEXT("Ashborn"), TEXT("The Ashborn"), -40.f, { { TEXT("Hollow"), -100.f }, { TEXT("Wardens"), -80.f }, { TEXT("Villagers"), -40.f } });
	Faction(TEXT("Hollow"), TEXT("The Hollow"), -100.f, { { TEXT("Villagers"), -100.f }, { TEXT("Wardens"), -100.f }, { TEXT("Ashborn"), -100.f } }, -100.f);
}

// --- Lore --------------------------------------------------------------------------------------

void UHRContentSubsystem::RegisterLore()
{
	NewLore(TEXT("Lore.Emberfall"), TEXT("Emberfall"), TEXT("The last village under the peak. Its warding bell kept the Hollow at bay, until it fell silent."));
	NewLore(TEXT("Lore.Whispers"), TEXT("The Whispering Wood"), TEXT("The Hollow wander here, mindless. Tall grass hides the careful; the careless are heard."));
	NewLore(TEXT("Lore.OldTongue"), TEXT("The Old Tongue"), TEXT("A primer for the Wardens' script. Glyphs across the land can now be read, and the spirit-sight of the old order can be learned."));
	NewLore(TEXT("Lore.Alchemy"), TEXT("Herbalist's Notes"), TEXT("Glowcaps steeped with bitterleaf focus the mind. New campfire recipes learned."));
	NewLore(TEXT("Lore.FirstFlame"), TEXT("The First Flame"), TEXT("Before the Wardens, the flame had a keeper who knew its name. The name was lost; the flame began to starve."));
	NewLore(TEXT("Lore.WardenOath"), TEXT("The Warden's Oath"), TEXT("The Warden held the flame alone for a century. The oath hollowed him out from the inside."));
	NewLore(TEXT("Lore.TrueName"), TEXT("A Name in the Bell"), TEXT("Scratched into the bell frame by the first keeper: the flame's true name. Speak it at the altar."));
	NewLore(TEXT("Lore.AshbornCause"), TEXT("The Ashborn"), TEXT("Miners abandoned by the Order when the ore ran out. They want the flame's warmth for everyone, not just the faithful."));
	NewLore(TEXT("Lore.Trial"), TEXT("The Trial of Echoes"), TEXT("A shifting labyrinth that remembers every challenger. It is never the same twice; it only grows deeper."));
	NewLore(TEXT("Lore.BellRinger"), TEXT("Who Silenced the Bell"), TEXT("Brother Aldous paid the Ashborn to cut the bell down, then sold the ash of the Hollow it let in."));

	NewLore(TEXT("Clue.BellRope"), TEXT("Cut Bell Rope"), TEXT("Found in the wood. Cut cleanly with a blade: the bell didn't fall, someone brought it down."));
	NewLore(TEXT("Clue.BellLedger"), TEXT("Waterlogged Ledger"), TEXT("Brother Aldous's ledger. Payments to 'the miners' the week the bell fell, and sales of Hollow Ash every week since."));
	NewLore(TEXT("Clue.BellClapper"), TEXT("The Bell's Clapper"), TEXT("Wrapped in Ashborn cloth to muffle it, then hidden at the top of the old tower."));

	FHRDeduction Deduction;
	Deduction.DeductionId = TEXT("Deduction.BellRinger");
	Deduction.Title = Txt(TEXT("The Silenced Bell"));
	Deduction.Conclusion = Txt(TEXT("The Ashborn cut the bell down, but Brother Aldous paid them, and profits from every Hollow that wanders in."));
	Deduction.RequiredClues = { TEXT("Clue.BellRope"), TEXT("Clue.BellLedger"), TEXT("Clue.BellClapper") };
	Deduction.ResultKnowledge = TEXT("Lore.BellRinger");
	Deductions.Add(Deduction);
}

// --- Quests ------------------------------------------------------------------------------------

void UHRContentSubsystem::RegisterQuests()
{
	using O = EHRObjectiveType;

	{
		FHRQuestDefinition Quest;
		Quest.QuestId = TEXT("Quest.Embers");
		Quest.Title = Txt(TEXT("The Dying Flame"));
		Quest.bMainQuest = true;
		Quest.RewardEchoes = 2000;
		Quest.Stages = {
			MakeStage(TEXT("Kindle"), TEXT("Kindle the ember shrine in Emberfall's square."), { MakeObjective(TEXT("Kindle the Ember Hearth"), O::Interact, TEXT("Shrine.Hearth")) }, TEXT("Elder")),
			MakeStage(TEXT("Elder"), TEXT("Speak with Elder Maren in the village."), { MakeObjective(TEXT("Talk to Elder Maren"), O::Talk, TEXT("NPC.Elder")) }, TEXT("Warden")),
			MakeStage(TEXT("Warden"), TEXT("The Hollow Warden guards Ashen Peak, far to the south. Climb, and end him."), { MakeObjective(TEXT("Defeat the Hollow Warden"), O::Kill, TEXT("Boss.Warden")) }, TEXT("Altar")),
			MakeStage(TEXT("Altar"), TEXT("The First Flame waits beyond the Warden's arena. Decide its fate."), { MakeObjective(TEXT("Choose the flame's fate"), O::Flag, TEXT("Game.Ending")) }, NAME_None, true),
		};
		Quests.Add(Quest.QuestId, Quest);
	}
	{
		FHRQuestDefinition Quest;
		Quest.QuestId = TEXT("Quest.DrownedBell");
		Quest.Title = Txt(TEXT("The Drowned Bell"));
		Quest.RewardEchoes = 600;
		Quest.RewardSkillPoints = 1;
		Quest.RewardFaction = TEXT("Villagers");
		Quest.RewardReputation = 10;
		Quest.Stages = {
			MakeStage(TEXT("Investigate"), TEXT("Someone silenced Emberfall's warding bell. Search the wood, the ruins and the old tower for what remains of it."), {
				MakeObjective(TEXT("Find the bell rope"), O::Interact, TEXT("Clue.BellRope")),
				MakeObjective(TEXT("Find the ledger"), O::Interact, TEXT("Clue.BellLedger")),
				MakeObjective(TEXT("Find the clapper"), O::Interact, TEXT("Clue.BellClapper")),
				MakeObjective(TEXT("Work out who did it"), O::Deduction, TEXT("Deduction.BellRinger")),
			}, TEXT("Confront")),
			MakeStage(TEXT("Confront"), TEXT("You know who silenced the bell. Tell Elder Maren... or don't."), {}, NAME_None),
		};
		Quests.Add(Quest.QuestId, Quest);
	}
	{
		FHRQuestDefinition Quest;
		Quest.QuestId = TEXT("Quest.HollowCull");
		Quest.Title = Txt(TEXT("Thin the Hollow"));
		Quest.RewardEchoes = 400;
		Quest.RewardFaction = TEXT("Villagers");
		Quest.RewardReputation = 10;
		Quest.RewardItems = { FHRItemAmount(TEXT("Item.Firebomb"), 3) };
		Quest.Stages = {
			MakeStage(TEXT("Hunt"), TEXT("Elder Maren asked you to cull the Hollow near the village."), { MakeObjective(TEXT("Slay Hollow"), O::Kill, TEXT("Enemy.Hollow"), 6) }, NAME_None, true),
		};
		Quests.Add(Quest.QuestId, Quest);
	}
	{
		FHRQuestDefinition Quest;
		Quest.QuestId = TEXT("Quest.AshbornPact");
		Quest.Title = Txt(TEXT("Embers for All"));
		Quest.RewardEchoes = 400;
		Quest.Stages = {
			MakeStage(TEXT("Choice"), TEXT("Sable of the Ashborn wants your help. Or your answer."), {}, NAME_None),
			MakeStage(TEXT("Supplies"), TEXT("Gather iron ore for the Ashborn forges."), { MakeObjective(TEXT("Gather iron ore"), O::Collect, TEXT("Item.IronOre"), 5) }, TEXT("Deliver")),
			MakeStage(TEXT("Deliver"), TEXT("Bring the ore to Sable in Emberfall."), {}, NAME_None),
			MakeStage(TEXT("Raid"), TEXT("Break the Ashborn: defeat Captain Varn in their eastern camp."), { MakeObjective(TEXT("Defeat Captain Varn"), O::Kill, TEXT("Enemy.AshbornCaptain")) }, NAME_None, true),
		};
		Quest.Stages[3].OnComplete = { FHRConsequence(C::ModifyReputation, TEXT("Villagers"), 15), FHRConsequence(C::ModifyReputation, TEXT("Wardens"), 15) };
		Quests.Add(Quest.QuestId, Quest);
	}
	{
		FHRQuestDefinition Quest;
		Quest.QuestId = TEXT("Quest.KestrelRuins");
		Quest.Title = Txt(TEXT("What the Ruins Keep"));
		Quest.RewardEchoes = 400;
		Quest.Stages = {
			MakeStage(TEXT("Travel"), TEXT("Kestrel wants to see the Sunken Ruins in the west, together."), { MakeObjective(TEXT("Reach the Sunken Ruins"), O::Reach, TEXT("Landmark.Ruins")) }, TEXT("Vault")),
			MakeStage(TEXT("Vault"), TEXT("Kestrel thinks the dial vault holds something about the First Flame."), { MakeObjective(TEXT("Open the dial vault"), O::Interact, TEXT("Puzzle.RuinsDials")) }, NAME_None, true),
		};
		Quest.Stages[1].OnComplete = { FHRConsequence(C::CompanionApproval, TEXT("Companion.Kestrel"), 20) };
		Quests.Add(Quest.QuestId, Quest);
	}
	{
		FHRQuestDefinition Quest;
		Quest.QuestId = TEXT("Quest.Trial");
		Quest.Title = Txt(TEXT("Trial of Echoes"));
		Quest.RewardEchoes = 800;
		Quest.RewardSkillPoints = 1;
		Quest.Stages = {
			MakeStage(TEXT("Descend"), TEXT("East of the village, a labyrinth reshapes itself. Defeat its guardian."), { MakeObjective(TEXT("Defeat a Trial Guardian"), O::Kill, TEXT("Enemy.TrialGuardian")) }, NAME_None, true),
		};
		Quests.Add(Quest.QuestId, Quest);
	}
}

// --- Dialogue ----------------------------------------------------------------------------------

void UHRContentSubsystem::RegisterDialogues()
{
	const FName End = NAME_None;

	// Elder Maren: quest hub with a branching accusation.
	{
		FHRDialogue Dialogue;
		Dialogue.DialogueId = TEXT("Dialogue.Elder");
		Dialogue.StartNode = TEXT("Greet");
		Dialogue.EntryPoints = {
			MakeEntry(TEXT("Confront"), { FHRCondition(K::QuestActive, TEXT("Quest.DrownedBell")), FHRCondition(K::DeductionSolved, TEXT("Deduction.BellRinger")) }),
			MakeEntry(TEXT("Accuse"), { FHRCondition(K::QuestActive, TEXT("Quest.DrownedBell")) }),
		};

		FHRDialogueNode Greet = MakeNode(TEXT("Greet"), TEXT("Elder Maren"),
			TEXT("Another wanderer, drawn by the dying flame. Our warding bell has gone silent, and the Hollow grow bolder every night."));
		Greet.Choices = {
			MakeChoice(TEXT("What happened to the bell?"), TEXT("Bell"), {}, { FHRCondition(K::QuestNotStarted, TEXT("Quest.DrownedBell")) }),
			MakeChoice(TEXT("Tell me about the Warden."), TEXT("Warden")),
			MakeChoice(TEXT("I'll thin out the Hollow for you."), TEXT("Thanks"), { FHRConsequence(C::StartQuest, TEXT("Quest.HollowCull")) }, { FHRCondition(K::QuestNotStarted, TEXT("Quest.HollowCull")) }),
			MakeChoice(TEXT("Farewell."), End),
		};
		FHRDialogueNode Bell = MakeNode(TEXT("Bell"), TEXT("Elder Maren"),
			TEXT("Someone brought it down in the night. Find out who. Look in the wood, the drowned ruins, the old tower. I'll see you rewarded."));
		Bell.Choices = {
			MakeChoice(TEXT("I'll find out."), End, { FHRConsequence(C::StartQuest, TEXT("Quest.DrownedBell")) }),
			MakeChoice(TEXT("Not my concern."), End),
		};
		FHRDialogueNode Warden = MakeNode(TEXT("Warden"), TEXT("Elder Maren"),
			TEXT("He kept the First Flame for a hundred years. Now he is hollow, and he guards it from everyone. Climb Ashen Peak to the south. Bring furs: the cold up there kills slower than he does, but it kills."));
		Warden.Choices = { MakeChoice(TEXT("Then I'll climb."), TEXT("Greet")), MakeChoice(TEXT("Farewell."), End) };
		FHRDialogueNode Thanks = MakeNode(TEXT("Thanks"), TEXT("Elder Maren"), TEXT("Six of them would make a difference. Go carefully."));
		Thanks.Choices = { MakeChoice(TEXT("[Leave]"), End) };

		FHRDialogueNode Accuse = MakeNode(TEXT("Accuse"), TEXT("Elder Maren"), TEXT("Have you learned who silenced the bell?"));
		FHRDialogueChoice Miller = MakeChoice(TEXT("It was the miller. I'm certain."), TEXT("Wrong"),
			{ FHRConsequence(C::FailQuest, TEXT("Quest.DrownedBell")), FHRConsequence(C::ModifyReputation, TEXT("Villagers"), -15) });
		Accuse.Choices = { Miller, MakeChoice(TEXT("Not yet."), End) };
		FHRDialogueNode Wrong = MakeNode(TEXT("Wrong"), TEXT("Elder Maren"),
			TEXT("The miller was at his daughter's bedside all week. You've shamed a grieving man. Leave it be, wanderer."));
		Wrong.Choices = { MakeChoice(TEXT("[Leave]"), End) };

		FHRDialogueNode Confront = MakeNode(TEXT("Confront"), TEXT("Elder Maren"), TEXT("You have the look of someone who knows something."));
		FHRDialogueChoice Truth = MakeChoice(TEXT("Brother Aldous paid the Ashborn to cut the bell down. He sells the ash of every Hollow it let in."), TEXT("TruthTold"), {
			FHRConsequence(C::CompleteQuest, TEXT("Quest.DrownedBell")), FHRConsequence(C::SetFlag, TEXT("Bell.Truth"), 1),
			FHRConsequence(C::ModifyReputation, TEXT("Wardens"), -20), FHRConsequence(C::ModifyReputation, TEXT("Villagers"), 15),
			FHRConsequence(C::CompanionApproval, TEXT("Companion.Kestrel"), 10) });
		FHRDialogueChoice Blackmail = MakeChoice(TEXT("Some things are worth more kept quiet. I'll be paying Aldous a visit instead."), TEXT("Blackmail"), {
			FHRConsequence(C::GiveEchoes, NAME_None, 700), FHRConsequence(C::SetFlag, TEXT("Bell.Blackmail"), 1), FHRConsequence(C::CompleteQuest, TEXT("Quest.DrownedBell")),
			FHRConsequence(C::CompanionApproval, TEXT("Companion.Kestrel"), -15) });
		Blackmail.bHasCheck = true;
		Blackmail.CheckAttribute = EHRAttribute::Mind;
		Blackmail.CheckDC = 14;
		Blackmail.FailNode = TEXT("BlackmailFailed");
		Blackmail.FailConsequences = { FHRConsequence(C::FailQuest, TEXT("Quest.DrownedBell")), FHRConsequence(C::ModifyReputation, TEXT("Villagers"), -20) };
		FHRDialogueChoice BlameAshborn = MakeChoice(TEXT("The Ashborn did it. Nobody else."), TEXT("Blamed"), {
			FHRConsequence(C::CompleteQuest, TEXT("Quest.DrownedBell")), FHRConsequence(C::SetFlag, TEXT("Bell.BlamedAshborn"), 1),
			FHRConsequence(C::ModifyReputation, TEXT("Ashborn"), -25), FHRConsequence(C::ModifyReputation, TEXT("Villagers"), 5) });
		Confront.Choices = { Truth, Blackmail, BlameAshborn };

		FHRDialogueNode TruthTold = MakeNode(TEXT("TruthTold"), TEXT("Elder Maren"), TEXT("A brother of the Order... I'll see the bell rehung and Aldous answer for it. Thank you."));
		TruthTold.Choices = { MakeChoice(TEXT("[Leave]"), End) };
		FHRDialogueNode BlackmailNode = MakeNode(TEXT("Blackmail"), TEXT("Brother Aldous (later)"), TEXT("...Take it. Take it and never speak of this again."));
		BlackmailNode.Choices = { MakeChoice(TEXT("[Leave]"), End) };
		FHRDialogueNode BlackmailFailed = MakeNode(TEXT("BlackmailFailed"), TEXT("Elder Maren"), TEXT("You think I can't read a schemer's face? Get out of my sight."));
		BlackmailFailed.Choices = { MakeChoice(TEXT("[Leave]"), End) };
		FHRDialogueNode Blamed = MakeNode(TEXT("Blamed"), TEXT("Elder Maren"), TEXT("Of course it was. The village will remember who its enemies are."));
		Blamed.Choices = { MakeChoice(TEXT("[Leave]"), End) };

		Dialogue.Nodes = { Greet, Bell, Warden, Thanks, Accuse, Wrong, Confront, TruthTold, BlackmailNode, BlackmailFailed, Blamed };
		Dialogues.Add(Dialogue.DialogueId, Dialogue);
	}

	// Odo the merchant.
	{
		FHRDialogue Dialogue;
		Dialogue.DialogueId = TEXT("Dialogue.Merchant");
		Dialogue.StartNode = TEXT("Greet");
		FHRDialogueNode Greet = MakeNode(TEXT("Greet"), TEXT("Odo the Peddler"), TEXT("Echoes for goods, goods for echoes. Prices depend on how well the village likes you."));
		Greet.Choices = {
			MakeChoice(TEXT("Show me your wares."), End, { FHRConsequence(C::OpenShop, NAME_None) }),
			MakeChoice(TEXT("Heard any rumors?"), TEXT("Rumors")),
			MakeChoice(TEXT("Goodbye."), End),
		};
		FHRDialogueNode Rumors = MakeNode(TEXT("Rumors"), TEXT("Odo the Peddler"),
			TEXT("The Ashborn captain carries a key to the old vaults in the west. And they say the peak's tower ledges were built for climbing, not decoration."));
		Rumors.Choices = { MakeChoice(TEXT("Show me your wares."), End, { FHRConsequence(C::OpenShop, NAME_None) }), MakeChoice(TEXT("Goodbye."), End) };
		Dialogue.Nodes = { Greet, Rumors };
		Dialogues.Add(Dialogue.DialogueId, Dialogue);
	}

	// Sable, envoy of the Ashborn: picks a side in the faction conflict.
	{
		FHRDialogue Dialogue;
		Dialogue.DialogueId = TEXT("Dialogue.Envoy");
		Dialogue.StartNode = TEXT("Greet");
		Dialogue.EntryPoints = {
			MakeEntry(TEXT("Deliver"), { FHRCondition(K::QuestActive, TEXT("Quest.AshbornPact")), FHRCondition(K::HasItem, TEXT("Item.IronOre"), 5) }),
			MakeEntry(TEXT("Waiting"), { FHRCondition(K::QuestActive, TEXT("Quest.AshbornPact")) }),
		};
		FHRDialogueNode Greet = MakeNode(TEXT("Greet"), TEXT("Sable"),
			TEXT("We aren't bandits. We're the miners the Order abandoned when the ore ran out. The flame should warm everyone. Help us, or stay out of the way."));
		FHRDialogueChoice Help = MakeChoice(TEXT("What do you need?"), TEXT("Help"), {
			FHRConsequence(C::StartQuest, TEXT("Quest.AshbornPact")), FHRConsequence(C::SetQuestStage, TEXT("Quest.AshbornPact"), 0, TEXT("Supplies")),
			FHRConsequence(C::ModifyReputation, TEXT("Ashborn"), 15), FHRConsequence(C::AddKnowledge, TEXT("Lore.AshbornCause")) },
			{ FHRCondition(K::QuestNotStarted, TEXT("Quest.AshbornPact")) });
		FHRDialogueChoice Oppose = MakeChoice(TEXT("Your people are raiders. I'll put an end to your camp."), End, {
			FHRConsequence(C::StartQuest, TEXT("Quest.AshbornPact")), FHRConsequence(C::SetQuestStage, TEXT("Quest.AshbornPact"), 0, TEXT("Raid")),
			FHRConsequence(C::ModifyReputation, TEXT("Ashborn"), -20) },
			{ FHRCondition(K::QuestNotStarted, TEXT("Quest.AshbornPact")) });
		FHRDialogueChoice Intimidate = MakeChoice(TEXT("Tell your captain to keep his raiders off the roads, or I'll do it for him."), TEXT("Respect"), {
			FHRConsequence(C::ModifyReputation, TEXT("Ashborn"), 10), FHRConsequence(C::ModifyReputation, TEXT("Villagers"), 5) },
			{ FHRCondition(K::FlagBelow, TEXT("Envoy.Intimidated"), 1) });
		Intimidate.bHasCheck = true;
		Intimidate.CheckAttribute = EHRAttribute::Strength;
		Intimidate.CheckDC = 13;
		Intimidate.FailNode = TEXT("Laugh");
		Intimidate.Consequences.Add(FHRConsequence(C::SetFlag, TEXT("Envoy.Intimidated"), 1));
		Intimidate.FailConsequences = { FHRConsequence(C::SetFlag, TEXT("Envoy.Intimidated"), 1), FHRConsequence(C::ModifyReputation, TEXT("Ashborn"), -10) };
		Greet.Choices = { Help, Oppose, Intimidate, MakeChoice(TEXT("Leave."), End) };

		FHRDialogueNode HelpNode = MakeNode(TEXT("Help"), TEXT("Sable"), TEXT("Iron. Five loads of ore for the forges. The peak's slopes are full of it, if you can stand the cold."));
		HelpNode.Choices = { MakeChoice(TEXT("[Leave]"), End) };
		FHRDialogueNode Respect = MakeNode(TEXT("Respect"), TEXT("Sable"), TEXT("...You mean it. Fine. I'll pass it on."));
		Respect.Choices = { MakeChoice(TEXT("[Leave]"), End) };
		FHRDialogueNode Laugh = MakeNode(TEXT("Laugh"), TEXT("Sable"), TEXT("Ha! Varn would eat you alive. Run along."));
		Laugh.Choices = { MakeChoice(TEXT("[Leave]"), End) };
		FHRDialogueNode Waiting = MakeNode(TEXT("Waiting"), TEXT("Sable"), TEXT("Well? The forges are cold."));
		Waiting.Choices = { MakeChoice(TEXT("Still working on it."), End) };
		FHRDialogueNode Deliver = MakeNode(TEXT("Deliver"), TEXT("Sable"), TEXT("Is that ore I see?"));
		Deliver.Choices = {
			MakeChoice(TEXT("Five loads, as promised."), TEXT("Delivered"), {
				FHRConsequence(C::TakeItem, TEXT("Item.IronOre"), 5), FHRConsequence(C::ModifyReputation, TEXT("Ashborn"), 40),
				FHRConsequence(C::CompleteQuest, TEXT("Quest.AshbornPact")), FHRConsequence(C::SetFlag, TEXT("Ashborn.Allied"), 1) },
				{ FHRCondition(K::QuestActive, TEXT("Quest.AshbornPact")) }),
			MakeChoice(TEXT("Not yet."), End),
		};
		FHRDialogueNode Delivered = MakeNode(TEXT("Delivered"), TEXT("Sable"), TEXT("The Ashborn won't forget this. Our camp is open to you, friend."));
		Delivered.Choices = { MakeChoice(TEXT("[Leave]"), End) };
		Dialogue.Nodes = { Greet, HelpNode, Respect, Laugh, Waiting, Deliver, Delivered };
		Dialogues.Add(Dialogue.DialogueId, Dialogue);
	}

	// Kestrel, the companion.
	{
		FHRDialogue Dialogue;
		Dialogue.DialogueId = TEXT("Dialogue.Kestrel");
		Dialogue.StartNode = TEXT("Meet");
		Dialogue.EntryPoints = { MakeEntry(TEXT("Party"), { FHRCondition(K::FlagAtLeast, TEXT("Companion.Kestrel.Recruited"), 1) }) };
		FHRDialogueNode Meet = MakeNode(TEXT("Meet"), TEXT("Kestrel"),
			TEXT("Kestrel. Hunter. I've watched the Hollow come down from the peak for weeks. If you're going up there, you'll want someone watching your back."));
		Meet.Choices = {
			MakeChoice(TEXT("Travel with me."), TEXT("Joined"), { FHRConsequence(C::RecruitCompanion, NAME_None), FHRConsequence(C::StartQuest, TEXT("Quest.KestrelRuins")) }),
			MakeChoice(TEXT("Not now."), End),
		};
		FHRDialogueNode Joined = MakeNode(TEXT("Joined"), TEXT("Kestrel"), TEXT("Good. Press G to tell me how to fight. And if you fall, I'll drag you back up. Once."));
		Joined.Choices = { MakeChoice(TEXT("[Leave]"), End) };
		FHRDialogueNode Party = MakeNode(TEXT("Party"), TEXT("Kestrel"), TEXT("Something on your mind?"));
		Party.Choices = {
			MakeChoice(TEXT("What do you make of the Ashborn?"), TEXT("Ashborn")),
			MakeChoice(TEXT("What drives you, Kestrel?"), TEXT("Drive"), { FHRConsequence(C::CompanionApproval, TEXT("Companion.Kestrel"), 5) }, { FHRCondition(K::FlagBelow, TEXT("Kestrel.Asked"), 1) }),
			MakeChoice(TEXT("Nothing. Let's move."), End),
		};
		Party.Choices[1].Consequences.Add(FHRConsequence(C::SetFlag, TEXT("Kestrel.Asked"), 1));
		FHRDialogueNode Ashborn = MakeNode(TEXT("Ashborn"), TEXT("Kestrel"), TEXT("Desperate people. Desperate people do ugly things. Doesn't make them wrong."));
		Ashborn.Choices = { MakeChoice(TEXT("[Leave]"), End) };
		FHRDialogueNode Drive = MakeNode(TEXT("Drive"), TEXT("Kestrel"), TEXT("My brother went up the peak a year ago. If he's Hollow now, I want to be the one who lays him down."));
		Drive.Choices = { MakeChoice(TEXT("We'll find him."), End), MakeChoice(TEXT("[Leave]"), End) };
		Dialogue.Nodes = { Meet, Joined, Party, Ashborn, Drive };
		Dialogues.Add(Dialogue.DialogueId, Dialogue);
	}

	// The First Flame: the ending choice.
	{
		FHRDialogue Dialogue;
		Dialogue.DialogueId = TEXT("Dialogue.Altar");
		Dialogue.StartNode = TEXT("Flame");
		FHRDialogueNode Flame = MakeNode(TEXT("Flame"), TEXT("The First Flame"),
			TEXT("It gutters, small as a candle, warm as a hearth. It waits for a keeper, or a thief, or an end."));
		FHRDialogueChoice Rekindle = MakeChoice(TEXT("Rekindle the flame and take up the Warden's oath."), End, { FHRConsequence(C::SetFlag, TEXT("Ending.Choice"), 1), FHRConsequence(C::TriggerEnding, NAME_None) });
		FHRDialogueChoice Extinguish = MakeChoice(TEXT("Let it die. Let the world find its own way in the dark."), End, { FHRConsequence(C::SetFlag, TEXT("Ending.Choice"), 2), FHRConsequence(C::TriggerEnding, NAME_None) });
		FHRDialogueChoice Seize = MakeChoice(TEXT("Seize the flame. Its warmth belongs to whoever is strong enough to hold it."), End, { FHRConsequence(C::SetFlag, TEXT("Ending.Choice"), 3), FHRConsequence(C::TriggerEnding, NAME_None) });
		FHRDialogueChoice Name = MakeChoice(TEXT("Speak the flame's true name."), End, {
			FHRConsequence(C::SetFlag, TEXT("Ending.Choice"), 1), FHRConsequence(C::SetFlag, TEXT("Ending.NameSpoken"), 1), FHRConsequence(C::TriggerEnding, NAME_None) },
			{ FHRCondition(K::HasKnowledge, TEXT("Lore.TrueName")) });
		Name.bShowWhenLocked = true;
		Name.LockedHint = Txt(TEXT("You do not know its name. Somewhere, someone wrote it down."));
		Flame.Choices = { Rekindle, Extinguish, Seize, Name, MakeChoice(TEXT("Not yet."), End) };
		Dialogue.Nodes = { Flame };
		Dialogues.Add(Dialogue.DialogueId, Dialogue);
	}
}

// --- Endings -----------------------------------------------------------------------------------

void UHRContentSubsystem::RegisterEndings()
{
	auto Ending = [this](FName Id, const TCHAR* Title, const TCHAR* Epilogue, int32 Priority, const TArray<FHRCondition>& Conditions)
	{
		FHREndingDefinition Definition;
		Definition.EndingId = Id;
		Definition.Title = Txt(Title);
		Definition.Epilogue = Txt(Epilogue);
		Definition.Priority = Priority;
		Definition.Conditions = Conditions;
		Endings.Add(Definition);
	};

	Ending(TEXT("Ending.Keeper"), TEXT("THE KEEPER REMEMBERED"),
		TEXT("You speak the name and the flame answers. It does not need a jailer; it needed to be known. Emberfall's bell rings again, and no one ever has to hollow themselves out to keep the light."),
		40, { FHRCondition(K::FlagAtLeast, TEXT("Ending.NameSpoken"), 1), FHRCondition(K::HasKnowledge, TEXT("Lore.FirstFlame")) });
	Ending(TEXT("Ending.Usurper"), TEXT("THE ASHEN THRONE"),
		TEXT("With the Ashborn at your back you carry the flame down the mountain. Its warmth reaches every hearth, and every hearth owes you. History will argue about what you became."),
		30, { FHRCondition(K::FlagAtLeast, TEXT("Ending.Choice"), 3), FHRCondition(K::ReputationAtLeast, TEXT("Ashborn"), 25) });
	Ending(TEXT("Ending.Hollow"), TEXT("ANOTHER HOLLOW KING"),
		TEXT("No one stands with you. The flame burns through your hands, then your heart. The next wanderer to climb Ashen Peak finds a new Warden waiting."),
		25, { FHRCondition(K::FlagAtLeast, TEXT("Ending.Choice"), 3) });
	Ending(TEXT("Ending.Ashen"), TEXT("THE NEW WARDEN"),
		TEXT("You take up the oath. The flame steadies, the Hollow retreat, and the years begin, slowly, to empty you out. Somewhere below, a hunter keeps your name."),
		20, { FHRCondition(K::FlagAtLeast, TEXT("Ending.Choice"), 1), FHRCondition(K::FlagBelow, TEXT("Ending.Choice"), 2) });
	Ending(TEXT("Ending.Dark"), TEXT("AGE OF EMBERS' END"),
		TEXT("The flame dies with a sigh. The Hollow fall still, finally at rest. The world is colder and darker, and for the first time in a century, entirely its own."),
		10, { FHRCondition(K::FlagAtLeast, TEXT("Ending.Choice"), 2), FHRCondition(K::FlagBelow, TEXT("Ending.Choice"), 3) });
}
