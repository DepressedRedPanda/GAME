#include "Components/HRInventoryComponent.h"
#include "Characters/HRCharacterBase.h"

#include "Characters/HRPlayerCharacter.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/HRSurvivalComponent.h"
#include "Core/HRStatics.h"
#include "Items/HRLootLibrary.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Narrative/HRQuestSubsystem.h"
#include "Systems/HRContentSubsystem.h"
#include "World/HRProjectile.h"

UHRInventoryComponent::UHRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHRInventoryComponent::InitializeStartingLoadout()
{
	for (const FHRItemAmount& Amount : StartingItems)
	{
		AddItem(Amount.ItemId, Amount.Quantity, false);
	}
	for (const FName& ItemId : StartingEquipment)
	{
		const FGuid Guid = AddItem(ItemId, 1, false);
		Equip(Guid);
	}
	if (QuickItemId.IsNone())
	{
		QuickItemId = FlaskItemId;
	}
	RefillFlasks();
}

const UHRItemDefinition* UHRInventoryComponent::GetDefinition(FName ItemId) const
{
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	return Content ? Content->FindItem(ItemId) : nullptr;
}

FGuid UHRInventoryComponent::AddItem(FName ItemId, int32 Quantity, bool bNotify)
{
	FRandomStream Stream(FMath::Rand());
	FHRItemInstance Instance = UHRLootLibrary::MakeInstance(this, ItemId, Quantity, Stream, 0.f);
	Instance.Rarity = EHRItemRarity::Common;
	Instance.Affixes.Reset();
	return AddItemInstance(Instance, bNotify);
}

FGuid UHRInventoryComponent::AddItemInstance(const FHRItemInstance& Instance, bool bNotify)
{
	const UHRItemDefinition* Definition = GetDefinition(Instance.ItemId);
	if (!Definition || Instance.Quantity <= 0)
	{
		return FGuid();
	}

	FGuid Result;
	int32 Remaining = Instance.Quantity;
	if (Definition->IsStackable())
	{
		for (FHRItemInstance& Existing : Items)
		{
			if (Existing.ItemId == Instance.ItemId && Existing.Quantity < Definition->MaxStack)
			{
				const int32 Added = FMath::Min(Remaining, Definition->MaxStack - Existing.Quantity);
				Existing.Quantity += Added;
				Remaining -= Added;
				Result = Existing.InstanceId;
				if (Remaining <= 0)
				{
					break;
				}
			}
		}
	}

	while (Remaining > 0)
	{
		FHRItemInstance NewInstance = Instance;
		NewInstance.InstanceId = FGuid::NewGuid();
		NewInstance.Quantity = Definition->IsStackable() ? FMath::Min(Remaining, Definition->MaxStack) : 1;
		Remaining -= NewInstance.Quantity;
		Items.Add(NewInstance);
		Result = NewInstance.InstanceId;
	}

	if (bNotify)
	{
		const FText Name = UHRLootLibrary::GetItemDisplayName(this, Instance);
		UHRStatics::Notify(this, FText::Format(NSLOCTEXT("HR", "Acquired", "Acquired: {0}"), Name), HRItems::RarityColor(Instance.Rarity));
	}

	OnItemAdded.Broadcast(Instance.ItemId, Instance.Quantity);
	NotifyChanged(Instance.ItemId);
	return Result;
}

bool UHRInventoryComponent::RemoveItem(FName ItemId, int32 Quantity)
{
	if (CountItem(ItemId) < Quantity)
	{
		return false;
	}
	int32 Remaining = Quantity;
	for (int32 Index = Items.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FHRItemInstance& Item = Items[Index];
		if (Item.ItemId != ItemId || IsEquipped(Item.InstanceId))
		{
			continue;
		}
		const int32 Taken = FMath::Min(Remaining, Item.Quantity);
		Item.Quantity -= Taken;
		Remaining -= Taken;
		if (Item.Quantity <= 0)
		{
			Items.RemoveAt(Index);
		}
	}
	NotifyChanged(ItemId);
	return Remaining <= 0;
}

bool UHRInventoryComponent::RemoveInstance(const FGuid& InstanceId, int32 Quantity)
{
	const int32 Index = Items.IndexOfByPredicate([&InstanceId](const FHRItemInstance& Item) { return Item.InstanceId == InstanceId; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	const FName ItemId = Items[Index].ItemId;
	Items[Index].Quantity -= Quantity;
	if (Items[Index].Quantity <= 0)
	{
		for (TPair<EHREquipSlot, FGuid>& Pair : Equipped)
		{
			if (Pair.Value == InstanceId)
			{
				Unequip(Pair.Key);
				break;
			}
		}
		Items.RemoveAt(Index);
	}
	NotifyChanged(ItemId);
	return true;
}

int32 UHRInventoryComponent::CountItem(FName ItemId) const
{
	int32 Count = 0;
	for (const FHRItemInstance& Item : Items)
	{
		if (Item.ItemId == ItemId)
		{
			Count += Item.Quantity;
		}
	}
	return Count;
}

const FHRItemInstance* UHRInventoryComponent::FindInstance(const FGuid& InstanceId) const
{
	return Items.FindByPredicate([&InstanceId](const FHRItemInstance& Item) { return Item.InstanceId == InstanceId; });
}

FHRItemInstance* UHRInventoryComponent::FindInstanceMutable(const FGuid& InstanceId)
{
	return Items.FindByPredicate([&InstanceId](const FHRItemInstance& Item) { return Item.InstanceId == InstanceId; });
}

float UHRInventoryComponent::GetCurrentWeight() const
{
	float Weight = 0.f;
	for (const FHRItemInstance& Item : Items)
	{
		if (const UHRItemDefinition* Definition = GetDefinition(Item.ItemId))
		{
			Weight += Definition->Weight * Item.Quantity;
		}
	}
	return Weight;
}

float UHRInventoryComponent::GetCarryCapacity() const
{
	const UHRAttributeComponent* Attributes = GetOwner() ? GetOwner()->FindComponentByClass<UHRAttributeComponent>() : nullptr;
	return Attributes ? Attributes->GetStat(EHRStat::CarryCapacity) : 60.f;
}

float UHRInventoryComponent::GetEncumbrance() const
{
	return GetCurrentWeight() / FMath::Max(1.f, GetCarryCapacity());
}

// --- Equipment ---------------------------------------------------------------------------------

FName UHRInventoryComponent::SlotSourceName(EHREquipSlot Slot)
{
	return FName(*FString::Printf(TEXT("Equip.%s"), *UEnum::GetValueAsString(Slot)));
}

bool UHRInventoryComponent::Equip(const FGuid& InstanceId)
{
	const FHRItemInstance* Instance = FindInstance(InstanceId);
	const UHRItemDefinition* Definition = Instance ? GetDefinition(Instance->ItemId) : nullptr;
	if (!Definition || !Definition->IsEquippable())
	{
		return false;
	}
	Equipped.Add(Definition->EquipSlot, InstanceId);
	ApplyEquipmentEffects(Definition->EquipSlot);
	NotifyChanged(Instance->ItemId);
	return true;
}

bool UHRInventoryComponent::Unequip(EHREquipSlot Slot)
{
	if (Equipped.Remove(Slot) == 0)
	{
		return false;
	}
	ApplyEquipmentEffects(Slot);
	NotifyChanged(NAME_None);
	return true;
}

bool UHRInventoryComponent::IsEquipped(const FGuid& InstanceId) const
{
	for (const TPair<EHREquipSlot, FGuid>& Pair : Equipped)
	{
		if (Pair.Value == InstanceId)
		{
			return true;
		}
	}
	return false;
}

FHRItemInstance UHRInventoryComponent::GetEquipped(EHREquipSlot Slot) const
{
	const FGuid* Guid = Equipped.Find(Slot);
	const FHRItemInstance* Instance = Guid ? FindInstance(*Guid) : nullptr;
	return Instance ? *Instance : FHRItemInstance();
}

void UHRInventoryComponent::ApplyEquipmentEffects(EHREquipSlot Slot)
{
	AActor* Owner = GetOwner();
	UHRAttributeComponent* Attributes = Owner ? Owner->FindComponentByClass<UHRAttributeComponent>() : nullptr;
	UHRCombatComponent* Combat = Owner ? Owner->FindComponentByClass<UHRCombatComponent>() : nullptr;

	const FGuid* Guid = Equipped.Find(Slot);
	const FHRItemInstance* Instance = Guid ? FindInstance(*Guid) : nullptr;
	const UHRItemDefinition* Definition = Instance ? GetDefinition(Instance->ItemId) : nullptr;

	if (Attributes)
	{
		if (Definition)
		{
			TArray<FHRStatModifier> Modifiers = Definition->Modifiers;
			Modifiers.Append(Instance->Affixes);
			Attributes->SetModifierSource(SlotSourceName(Slot), Modifiers);
		}
		else
		{
			Attributes->RemoveModifierSource(SlotSourceName(Slot));
		}
	}

	if (Slot == EHREquipSlot::MainHand && Combat)
	{
		if (Definition)
		{
			Combat->SetWeapon(Definition->Weapon, Instance->UpgradeLevel);
		}
		else
		{
			Combat->ResetWeaponToDefault();
		}
		if (AHRCharacterBase* Character = Cast<AHRCharacterBase>(Owner)) Character->RefreshWeaponAppearance(Definition ? Definition->ItemId : NAME_None);
	}
}

void UHRInventoryComponent::RefreshEquipmentEffects()
{
	for (uint8 SlotIndex = 1; SlotIndex <= static_cast<uint8>(EHREquipSlot::Trinket); ++SlotIndex)
	{
		ApplyEquipmentEffects(static_cast<EHREquipSlot>(SlotIndex));
	}
}

// --- Use ---------------------------------------------------------------------------------------

bool UHRInventoryComponent::UseItem(const FGuid& InstanceId)
{
	const FHRItemInstance* Instance = FindInstance(InstanceId);
	const UHRItemDefinition* Definition = Instance ? GetDefinition(Instance->ItemId) : nullptr;
	if (!Definition)
	{
		return false;
	}

	if (Definition->Category == EHRItemCategory::Lore)
	{
		if (!Definition->GrantsKnowledge.IsNone())
		{
			if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
			{
				Narrative->AddKnowledge(Definition->GrantsKnowledge);
			}
		}
		UHRStatics::Notify(this, Definition->Description, FLinearColor(0.85f, 0.8f, 0.6f));
		return true;
	}

	if (Definition->IsEquippable())
	{
		return IsEquipped(InstanceId) ? Unequip(Definition->EquipSlot) : Equip(InstanceId);
	}

	if (Definition->Category != EHRItemCategory::Consumable)
	{
		return false;
	}

	// Using an item is a commitment: it locks you out of combat actions briefly.
	if (UHRCombatComponent* Combat = GetOwner()->FindComponentByClass<UHRCombatComponent>())
	{
		if (!Combat->BeginBusy(Definition->Consumable.UseTime))
		{
			return false;
		}
	}

	ApplyConsumable(*Definition);
	RemoveInstance(InstanceId, 1);
	return true;
}

void UHRInventoryComponent::ApplyConsumable(const UHRItemDefinition& Definition)
{
	AActor* Owner = GetOwner();
	const FHRConsumableData& Data = Definition.Consumable;

	if (Data.bThrown)
	{
		if (APawn* Pawn = Cast<APawn>(Owner))
		{
			const FRotator Aim = Pawn->GetBaseAimRotation() + FRotator(8.f, 0.f, 0.f);
			FHRDamageInfo Damage;
			Damage.Amount = Data.ThrownDamage;
			Damage.PostureDamage = Data.ThrownDamage * 0.6f;
			Damage.Type = Data.ThrownElement;
			Damage.Source = Owner;
			AHRProjectile::Launch(Owner, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 60.f + FVector(0.f, 0.f, 60.f), Aim.Vector(), Damage, 1800.f, 0.6f);
		}
		return;
	}

	if (UHRAttributeComponent* Attributes = Owner->FindComponentByClass<UHRAttributeComponent>())
	{
		Attributes->Heal(Data.Heal);
		Attributes->RestoreStamina(Data.RestoreStamina);
		Attributes->RestoreFocus(Data.RestoreFocus);
		if (Data.bCuresInjuries)
		{
			Attributes->CureAllInjuries();
		}
		if (Data.Buff.Num() > 0 && Data.BuffDuration > 0.f)
		{
			Attributes->AddTimedModifiers(Definition.ItemId, Data.Buff, Data.BuffDuration);
		}
	}
	if (UHRSurvivalComponent* Survival = Owner->FindComponentByClass<UHRSurvivalComponent>())
	{
		Survival->Eat(Data.Food);
		Survival->Drink(Data.Water);
		Survival->AddWarmth(Data.Warmth);
		Survival->AddRest(Data.Rest);
		Survival->TreatInfection(Data.InfectionTreatment);
	}
}

bool UHRInventoryComponent::UseQuickItem()
{
	for (const FHRItemInstance& Item : Items)
	{
		if (Item.ItemId == QuickItemId)
		{
			return UseItem(Item.InstanceId);
		}
	}
	UHRStatics::Notify(this, NSLOCTEXT("HR", "QuickEmpty", "Nothing left to use."), FLinearColor::Gray);
	return false;
}

void UHRInventoryComponent::CycleQuickItem()
{
	TArray<FName> Consumables;
	for (const FHRItemInstance& Item : Items)
	{
		const UHRItemDefinition* Definition = GetDefinition(Item.ItemId);
		if (Definition && Definition->Category == EHRItemCategory::Consumable)
		{
			Consumables.AddUnique(Item.ItemId);
		}
	}
	if (Consumables.Num() == 0)
	{
		return;
	}
	const int32 Current = Consumables.IndexOfByKey(QuickItemId);
	QuickItemId = Consumables[(Current + 1) % Consumables.Num()];
	NotifyChanged(NAME_None);
}

void UHRInventoryComponent::RefillFlasks()
{
	const int32 Current = CountItem(FlaskItemId);
	if (Current < MaxFlaskCharges)
	{
		AddItem(FlaskItemId, MaxFlaskCharges - Current, false);
	}
}

void UHRInventoryComponent::RestoreState(const TArray<FHRItemInstance>& InItems, const TMap<EHREquipSlot, FGuid>& InEquipped, FName InQuickItem, int32 InMaxFlasks)
{
	Items = InItems;
	Equipped = InEquipped;
	QuickItemId = InQuickItem.IsNone() ? FlaskItemId : InQuickItem;
	MaxFlaskCharges = FMath::Max(1, InMaxFlasks);
	RefreshEquipmentEffects();
	NotifyChanged(NAME_None);
}

void UHRInventoryComponent::NotifyChanged(FName ItemId)
{
	OnInventoryChanged.Broadcast();
	if (!ItemId.IsNone())
	{
		if (UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this))
		{
			Quests->NotifyItemCount(ItemId, CountItem(ItemId));
		}
	}
}
