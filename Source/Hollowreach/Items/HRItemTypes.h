#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/HRTypes.h"
#include "HRItemTypes.generated.h"

class UAnimMontage;
class UStaticMesh;

UENUM(BlueprintType)
enum class EHRItemCategory : uint8
{
	Weapon,
	Armor,
	Trinket,
	Consumable,
	Material,
	KeyItem,
	Lore
};

UENUM(BlueprintType)
enum class EHRItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

UENUM(BlueprintType)
enum class EHREquipSlot : uint8
{
	None,
	MainHand,
	Head,
	Chest,
	Hands,
	Legs,
	Trinket
};

/** One swing of a combo. Timings drive gameplay; the optional montage is purely visual. */
USTRUCT(BlueprintType)
struct FHRAttackStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") float DamageMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") float PostureMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") float StaminaCost = 14.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") float Windup = 0.22f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") float ActiveTime = 0.14f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") float Recovery = 0.32f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") float Reach = 190.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") float Radius = 65.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") float LungeSpeed = 450.f;
	/** Perilous: must be dodged, cannot be blocked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") bool bUnblockable = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack") TObjectPtr<UAnimMontage> Montage = nullptr;

	float GetTotalTime() const { return Windup + ActiveTime + Recovery; }
};

USTRUCT(BlueprintType)
struct FHRWeaponData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") float BaseDamage = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") float BasePosture = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") EHRDamageType DamageType = EHRDamageType::Physical;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") EHRAttribute ScalingAttribute = EHRAttribute::Strength;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") float ScalingFactor = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") TArray<FHRAttackStep> LightCombo;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") FHRAttackStep HeavyAttack;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") int32 MaxUpgradeLevel = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") FName UpgradeMaterial = TEXT("Item.HollowAsh");
	/** Visual length of the greybox blade, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon") float VisualLength = 110.f;
};

USTRUCT(BlueprintType)
struct FHRConsumableData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float Heal = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float RestoreStamina = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float RestoreFocus = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float Food = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float Water = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float Warmth = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float Rest = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float InfectionTreatment = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") bool bCuresInjuries = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") TArray<FHRStatModifier> Buff;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float BuffDuration = 0.f;
	/** Thrown instead of drunk (firebombs). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") bool bThrown = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") EHRDamageType ThrownElement = EHRDamageType::Fire;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float ThrownDamage = 0.f;
	/** Time the user is locked in the use animation. Healing is a commitment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Consumable") float UseTime = 0.6f;
};

/** Static definition of an item. Built-in items are created in code; designers can add data assets. */
UCLASS(BlueprintType)
class HOLLOWREACH_API UHRItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") FName ItemId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(MultiLine=true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") EHRItemCategory Category = EHRItemCategory::Material;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") EHREquipSlot EquipSlot = EHREquipSlot::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") int32 MaxStack = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") float Weight = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") int32 BaseValue = 10;
	/** Gear rolls random rarity + affixes when dropped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") bool bCanRollAffixes = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") TArray<FHRStatModifier> Modifiers;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") FHRWeaponData Weapon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") FHRConsumableData Consumable;
	/** Reading a lore item grants this knowledge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") FName GrantsKnowledge;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item") TObjectPtr<UStaticMesh> WorldMesh;

	bool IsEquippable() const { return EquipSlot != EHREquipSlot::None; }
	bool IsStackable() const { return MaxStack > 1; }

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("HRItem"), ItemId);
	}
};

/** A concrete item the player owns. Saved by value, so only ids are referenced. */
USTRUCT(BlueprintType)
struct FHRItemInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FGuid InstanceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FName ItemId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") int32 Quantity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") int32 UpgradeLevel = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") EHRItemRarity Rarity = EHRItemRarity::Common;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") TArray<FHRStatModifier> Affixes;

	bool IsValid() const { return !ItemId.IsNone() && Quantity > 0; }
};

USTRUCT(BlueprintType)
struct FHRItemAmount
{
	GENERATED_BODY()

	FHRItemAmount() {}
	FHRItemAmount(FName InItemId, int32 InQuantity) : ItemId(InItemId), Quantity(InQuantity) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FName ItemId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FHRRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting") FName RecipeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting") TArray<FHRItemAmount> Inputs;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting") FHRItemAmount Output;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting") EHRCraftStation Station = EHRCraftStation::None;
	/** Recipe is hidden until the player has learned this (knowledge-based progression). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crafting") FName RequiredKnowledge;
};

USTRUCT(BlueprintType)
struct FHRLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") FName ItemId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") float Weight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") int32 MinQuantity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") int32 MaxQuantity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") EHRItemRarity MinRarity = EHRItemRarity::Common;
};

USTRUCT(BlueprintType)
struct FHRLootTable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") FName TableId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") TArray<FHRLootEntry> Entries;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") TArray<FHRItemAmount> Guaranteed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") int32 MinRolls = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") int32 MaxRolls = 2;
	/** Chance that any single roll produces nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") float EmptyChance = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") int32 MinEchoes = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot") int32 MaxEchoes = 0;
};

namespace HRItems
{
	inline FLinearColor RarityColor(EHRItemRarity Rarity)
	{
		switch (Rarity)
		{
		case EHRItemRarity::Uncommon: return FLinearColor(0.35f, 0.9f, 0.35f);
		case EHRItemRarity::Rare: return FLinearColor(0.3f, 0.55f, 1.f);
		case EHRItemRarity::Epic: return FLinearColor(0.75f, 0.35f, 1.f);
		case EHRItemRarity::Legendary: return FLinearColor(1.f, 0.65f, 0.15f);
		default: return FLinearColor(0.85f, 0.85f, 0.85f);
		}
	}

	inline float RarityValueMultiplier(EHRItemRarity Rarity)
	{
		return 1.f + 0.75f * static_cast<float>(Rarity);
	}
}
