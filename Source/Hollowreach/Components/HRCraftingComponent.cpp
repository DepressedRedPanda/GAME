#include "Components/HRCraftingComponent.h"

#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Core/HRStatics.h"
#include "Items/HRLootLibrary.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Systems/HRContentSubsystem.h"

#define LOCTEXT_NAMESPACE "HRCrafting"

UHRCraftingComponent::UHRCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHRCraftingComponent::SetStation(EHRCraftStation InStation, bool bInHasForge)
{
	Station = InStation;
	bHasForge = bInHasForge;
}

UHRInventoryComponent* UHRCraftingComponent::GetInventory() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UHRInventoryComponent>() : nullptr;
}

TArray<FHRRecipe> UHRCraftingComponent::GetKnownRecipes() const
{
	TArray<FHRRecipe> Known;
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	if (!Content)
	{
		return Known;
	}
	for (const FHRRecipe& Recipe : Content->Recipes)
	{
		if (Recipe.RequiredKnowledge.IsNone() || (Narrative && Narrative->HasKnowledge(Recipe.RequiredKnowledge)))
		{
			Known.Add(Recipe);
		}
	}
	return Known;
}

bool UHRCraftingComponent::CanCraft(const FHRRecipe& Recipe, FText& OutReason) const
{
	const UHRInventoryComponent* Inventory = GetInventory();
	if (!Inventory)
	{
		return false;
	}
	if (Recipe.Station == EHRCraftStation::Forge && !bHasForge)
	{
		OutReason = LOCTEXT("NeedForge", "Requires a forge");
		return false;
	}
	if (Recipe.Station == EHRCraftStation::Campfire && Station == EHRCraftStation::None)
	{
		OutReason = LOCTEXT("NeedFire", "Requires a fire");
		return false;
	}
	for (const FHRItemAmount& Input : Recipe.Inputs)
	{
		if (!Inventory->HasItem(Input.ItemId, Input.Quantity))
		{
			OutReason = LOCTEXT("MissingMaterials", "Missing materials");
			return false;
		}
	}
	OutReason = FText::GetEmpty();
	return true;
}

bool UHRCraftingComponent::Craft(FName RecipeId)
{
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	const FHRRecipe* Recipe = Content ? Content->FindRecipe(RecipeId) : nullptr;
	UHRInventoryComponent* Inventory = GetInventory();
	FText Reason;
	if (!Recipe || !Inventory || !CanCraft(*Recipe, Reason))
	{
		UHRStatics::Notify(this, Reason, FLinearColor::Gray);
		return false;
	}
	for (const FHRItemAmount& Input : Recipe->Inputs)
	{
		Inventory->RemoveItem(Input.ItemId, Input.Quantity);
	}
	Inventory->AddItem(Recipe->Output.ItemId, Recipe->Output.Quantity, true);
	return true;
}

bool UHRCraftingComponent::GetUpgradeCost(const FGuid& ItemGuid, FName& OutMaterial, int32& OutMaterialCount, int32& OutEchoes) const
{
	const UHRInventoryComponent* Inventory = GetInventory();
	const FHRItemInstance* Instance = Inventory ? Inventory->FindInstance(ItemGuid) : nullptr;
	const UHRItemDefinition* Definition = Instance ? Inventory->GetDefinition(Instance->ItemId) : nullptr;
	if (!Definition || Definition->Category != EHRItemCategory::Weapon)
	{
		return false;
	}
	// Higher tiers need the rarer material.
	const int32 NextLevel = Instance->UpgradeLevel + 1;
	OutMaterial = NextLevel > 5 ? FName(TEXT("Item.Moonstone")) : Definition->Weapon.UpgradeMaterial;
	OutMaterialCount = NextLevel > 5 ? NextLevel - 5 : NextLevel;
	OutEchoes = 150 * NextLevel;
	return true;
}

bool UHRCraftingComponent::CanUpgrade(const FGuid& ItemGuid, FText& OutReason) const
{
	const UHRInventoryComponent* Inventory = GetInventory();
	const FHRItemInstance* Instance = Inventory ? Inventory->FindInstance(ItemGuid) : nullptr;
	const UHRItemDefinition* Definition = Instance ? Inventory->GetDefinition(Instance->ItemId) : nullptr;
	FName Material;
	int32 MaterialCount = 0;
	int32 EchoCost = 0;
	if (!Definition || !GetUpgradeCost(ItemGuid, Material, MaterialCount, EchoCost))
	{
		OutReason = LOCTEXT("NotUpgradable", "Cannot be upgraded");
		return false;
	}
	if (!bHasForge)
	{
		OutReason = LOCTEXT("UpgradeNeedForge", "Requires a forge");
		return false;
	}
	if (Instance->UpgradeLevel >= Definition->Weapon.MaxUpgradeLevel)
	{
		OutReason = LOCTEXT("MaxLevel", "Fully upgraded");
		return false;
	}
	const UHRProgressionComponent* Progression = GetOwner()->FindComponentByClass<UHRProgressionComponent>();
	if (!Inventory->HasItem(Material, MaterialCount) || !Progression || Progression->GetEchoes() < EchoCost)
	{
		OutReason = LOCTEXT("UpgradeMissing", "Missing materials or echoes");
		return false;
	}
	return true;
}

bool UHRCraftingComponent::UpgradeItem(const FGuid& ItemGuid)
{
	FText Reason;
	if (!CanUpgrade(ItemGuid, Reason))
	{
		UHRStatics::Notify(this, Reason, FLinearColor::Gray);
		return false;
	}
	UHRInventoryComponent* Inventory = GetInventory();
	UHRProgressionComponent* Progression = GetOwner()->FindComponentByClass<UHRProgressionComponent>();
	FName Material;
	int32 MaterialCount = 0;
	int32 EchoCost = 0;
	GetUpgradeCost(ItemGuid, Material, MaterialCount, EchoCost);

	Inventory->RemoveItem(Material, MaterialCount);
	Progression->SpendEchoes(EchoCost);
	if (FHRItemInstance* Instance = Inventory->FindInstanceMutable(ItemGuid))
	{
		++Instance->UpgradeLevel;
		Inventory->RefreshEquipmentEffects();
		UHRStatics::Notify(this, FText::Format(LOCTEXT("Upgraded", "Reinforced: {0}"), UHRLootLibrary::GetItemDisplayName(this, *Instance)), FLinearColor(1.f, 0.7f, 0.3f));
	}
	return true;
}

FText UHRCraftingComponent::DescribeRecipe(const UObject* WorldContextObject, const FHRRecipe& Recipe)
{
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(WorldContextObject);
	auto ItemName = [Content](FName ItemId)
	{
		const UHRItemDefinition* Definition = Content ? Content->FindItem(ItemId) : nullptr;
		return Definition ? Definition->DisplayName.ToString() : ItemId.ToString();
	};

	FString Inputs;
	for (const FHRItemAmount& Input : Recipe.Inputs)
	{
		if (!Inputs.IsEmpty())
		{
			Inputs += TEXT(", ");
		}
		Inputs += FString::Printf(TEXT("%d %s"), Input.Quantity, *ItemName(Input.ItemId));
	}
	return FText::FromString(FString::Printf(TEXT("%s x%d  <-  %s"), *ItemName(Recipe.Output.ItemId), Recipe.Output.Quantity, *Inputs));
}

#undef LOCTEXT_NAMESPACE
