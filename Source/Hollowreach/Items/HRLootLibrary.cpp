#include "Items/HRLootLibrary.h"

#include "Systems/HRContentSubsystem.h"

void UHRLootLibrary::RollLoot(const UObject* WorldContextObject, FName TableId, int32 Seed, float Luck, TArray<FHRItemInstance>& OutItems, int32& OutEchoes)
{
	OutEchoes = 0;
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(WorldContextObject);
	const FHRLootTable* Table = Content ? Content->FindLootTable(TableId) : nullptr;
	if (!Table)
	{
		return;
	}

	FRandomStream Stream(Seed);

	for (const FHRItemAmount& Guaranteed : Table->Guaranteed)
	{
		OutItems.Add(MakeInstance(WorldContextObject, Guaranteed.ItemId, Guaranteed.Quantity, Stream, Luck));
	}

	float TotalWeight = 0.f;
	for (const FHRLootEntry& Entry : Table->Entries)
	{
		TotalWeight += FMath::Max(0.f, Entry.Weight);
	}

	// Luck grants a chance at one bonus roll.
	int32 Rolls = Stream.RandRange(Table->MinRolls, FMath::Max(Table->MinRolls, Table->MaxRolls));
	if (Stream.FRand() < FMath::Clamp(Luck * 0.5f, 0.f, 0.5f))
	{
		++Rolls;
	}

	for (int32 Roll = 0; Roll < Rolls && TotalWeight > 0.f; ++Roll)
	{
		if (Stream.FRand() < Table->EmptyChance)
		{
			continue;
		}

		float Pick = Stream.FRandRange(0.f, TotalWeight);
		for (const FHRLootEntry& Entry : Table->Entries)
		{
			Pick -= FMath::Max(0.f, Entry.Weight);
			if (Pick <= 0.f)
			{
				const int32 Quantity = Stream.RandRange(Entry.MinQuantity, FMath::Max(Entry.MinQuantity, Entry.MaxQuantity));
				OutItems.Add(MakeInstance(WorldContextObject, Entry.ItemId, Quantity, Stream, Luck, Entry.MinRarity));
				break;
			}
		}
	}

	if (Table->MaxEchoes > 0)
	{
		OutEchoes = Stream.RandRange(Table->MinEchoes, Table->MaxEchoes);
	}
}

FHRItemInstance UHRLootLibrary::MakeInstance(const UObject* WorldContextObject, FName ItemId, int32 Quantity, FRandomStream& Stream, float Luck, EHRItemRarity MinRarity)
{
	FHRItemInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.ItemId = ItemId;
	Instance.Quantity = FMath::Max(1, Quantity);

	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(WorldContextObject);
	const UHRItemDefinition* Definition = Content ? Content->FindItem(ItemId) : nullptr;
	if (Definition && Definition->bCanRollAffixes)
	{
		Instance.Quantity = 1;
		Instance.Rarity = RollRarity(Stream, Luck, MinRarity);
		RollAffixes(Definition, Instance.Rarity, Stream, Instance.Affixes);
	}
	return Instance;
}

EHRItemRarity UHRLootLibrary::RollRarity(FRandomStream& Stream, float Luck, EHRItemRarity MinRarity)
{
	// Base weights for Common..Legendary; luck shifts weight towards rarer tiers.
	float Weights[5] = { 60.f, 25.f, 10.f, 4.f, 1.f };
	const float Shift = FMath::Clamp(Luck, 0.f, 2.f);
	for (int32 Tier = 1; Tier < 5; ++Tier)
	{
		Weights[Tier] *= 1.f + Shift * Tier;
	}

	float Total = 0.f;
	for (int32 Tier = static_cast<int32>(MinRarity); Tier < 5; ++Tier)
	{
		Total += Weights[Tier];
	}

	float Pick = Stream.FRandRange(0.f, Total);
	for (int32 Tier = static_cast<int32>(MinRarity); Tier < 5; ++Tier)
	{
		Pick -= Weights[Tier];
		if (Pick <= 0.f)
		{
			return static_cast<EHRItemRarity>(Tier);
		}
	}
	return MinRarity;
}

void UHRLootLibrary::RollAffixes(const UHRItemDefinition* Definition, EHRItemRarity Rarity, FRandomStream& Stream, TArray<FHRStatModifier>& OutAffixes)
{
	if (!Definition)
	{
		return;
	}

	struct FAffixTemplate
	{
		EHRStat Stat;
		float Min;
		float Max;
	};

	TArray<FAffixTemplate> Pool;
	switch (Definition->Category)
	{
	case EHRItemCategory::Weapon:
		Pool = {
			{ EHRStat::PhysicalPower, 2.f, 6.f },
			{ EHRStat::CritChance, 0.02f, 0.05f },
			{ EHRStat::PostureDamage, 0.04f, 0.12f },
			{ EHRStat::ArcanePower, 2.f, 6.f },
			{ EHRStat::StaminaRegen, 2.f, 5.f },
		};
		break;
	case EHRItemCategory::Armor:
		Pool = {
			{ EHRStat::Defense, 2.f, 6.f },
			{ EHRStat::MaxHealth, 8.f, 25.f },
			{ EHRStat::FireResist, 0.03f, 0.08f },
			{ EHRStat::FrostResist, 0.03f, 0.08f },
			{ EHRStat::Insulation, 3.f, 8.f },
			{ EHRStat::MaxStamina, 4.f, 12.f },
		};
		break;
	default:
		Pool = {
			{ EHRStat::ArcanePower, 2.f, 5.f },
			{ EHRStat::FocusRegen, 0.3f, 0.8f },
			{ EHRStat::Stealth, 0.03f, 0.08f },
			{ EHRStat::LootLuck, 0.05f, 0.15f },
			{ EHRStat::MaxFocus, 5.f, 15.f },
		};
		break;
	}

	const int32 NumAffixes = FMath::Min(static_cast<int32>(Rarity), Pool.Num());
	const float TierScale = 1.f + 0.25f * static_cast<float>(Rarity);
	for (int32 Index = 0; Index < NumAffixes; ++Index)
	{
		const int32 PoolIndex = Stream.RandRange(0, Pool.Num() - 1);
		const FAffixTemplate Template = Pool[PoolIndex];
		Pool.RemoveAtSwap(PoolIndex);
		OutAffixes.Add(FHRStatModifier(Template.Stat, Stream.FRandRange(Template.Min, Template.Max) * TierScale));
	}
}

FText UHRLootLibrary::GetItemDisplayName(const UObject* WorldContextObject, const FHRItemInstance& Item)
{
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(WorldContextObject);
	const UHRItemDefinition* Definition = Content ? Content->FindItem(Item.ItemId) : nullptr;
	FString Name = Definition ? Definition->DisplayName.ToString() : Item.ItemId.ToString();

	if (Definition && Definition->bCanRollAffixes && Item.Rarity != EHRItemRarity::Common)
	{
		Name = UEnum::GetDisplayValueAsText(Item.Rarity).ToString() + TEXT(" ") + Name;
	}
	if (Item.UpgradeLevel > 0)
	{
		Name += FString::Printf(TEXT(" +%d"), Item.UpgradeLevel);
	}
	if (Item.Quantity > 1)
	{
		Name += FString::Printf(TEXT(" x%d"), Item.Quantity);
	}
	return FText::FromString(Name);
}

FText UHRLootLibrary::DescribeModifier(const FHRStatModifier& Modifier)
{
	const FString StatName = UEnum::GetDisplayValueAsText(Modifier.Stat).ToString();
	FString Result;
	if (!FMath::IsNearlyZero(Modifier.Additive))
	{
		const bool bFraction = FMath::Abs(Modifier.Additive) < 1.f;
		Result = bFraction
			? FString::Printf(TEXT("%+.0f%% %s"), Modifier.Additive * 100.f, *StatName)
			: FString::Printf(TEXT("%+.0f %s"), Modifier.Additive, *StatName);
	}
	if (!FMath::IsNearlyEqual(Modifier.Multiplier, 1.f))
	{
		if (!Result.IsEmpty())
		{
			Result += TEXT(", ");
		}
		Result += FString::Printf(TEXT("x%.2f %s"), Modifier.Multiplier, *StatName);
	}
	return FText::FromString(Result);
}
