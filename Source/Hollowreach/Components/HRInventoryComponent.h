#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/HRItemTypes.h"
#include "HRInventoryComponent.generated.h"

class UHRItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHRInventoryChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRItemAddedSignature, FName, ItemId, int32, Quantity);

/**
 * Weight-limited inventory with stacking, equipment slots, a quick-use item and healing flasks.
 * Equipping pushes weapon data into the combat component and gear stats into attributes.
 */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRInventoryComponent();

	/** Gives starting items and equipment. Called by the owner once its other components are ready. */
	void InitializeStartingLoadout();

	UFUNCTION(BlueprintCallable, Category="Inventory") FGuid AddItem(FName ItemId, int32 Quantity = 1, bool bNotify = true);
	UFUNCTION(BlueprintCallable, Category="Inventory") FGuid AddItemInstance(const FHRItemInstance& Instance, bool bNotify = true);
	UFUNCTION(BlueprintCallable, Category="Inventory") bool RemoveItem(FName ItemId, int32 Quantity = 1);
	UFUNCTION(BlueprintCallable, Category="Inventory") bool RemoveInstance(const FGuid& InstanceId, int32 Quantity = 1);
	UFUNCTION(BlueprintPure, Category="Inventory") int32 CountItem(FName ItemId) const;
	UFUNCTION(BlueprintPure, Category="Inventory") bool HasItem(FName ItemId, int32 Quantity = 1) const { return CountItem(ItemId) >= Quantity; }
	UFUNCTION(BlueprintPure, Category="Inventory") TArray<FHRItemInstance> GetItems() const { return Items; }

	const FHRItemInstance* FindInstance(const FGuid& InstanceId) const;
	FHRItemInstance* FindInstanceMutable(const FGuid& InstanceId);
	const UHRItemDefinition* GetDefinition(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="Inventory") float GetCurrentWeight() const;
	UFUNCTION(BlueprintPure, Category="Inventory") float GetCarryCapacity() const;
	UFUNCTION(BlueprintPure, Category="Inventory") float GetEncumbrance() const;

	// --- Equipment ---
	UFUNCTION(BlueprintCallable, Category="Inventory") bool Equip(const FGuid& InstanceId);
	UFUNCTION(BlueprintCallable, Category="Inventory") bool Unequip(EHREquipSlot Slot);
	UFUNCTION(BlueprintPure, Category="Inventory") bool IsEquipped(const FGuid& InstanceId) const;
	UFUNCTION(BlueprintPure, Category="Inventory") FHRItemInstance GetEquipped(EHREquipSlot Slot) const;
	/** Re-applies weapon and gear effects (after upgrades or loading). */
	void RefreshEquipmentEffects();

	// --- Use ---
	UFUNCTION(BlueprintCallable, Category="Inventory") bool UseItem(const FGuid& InstanceId);
	UFUNCTION(BlueprintCallable, Category="Inventory") bool UseQuickItem();
	UFUNCTION(BlueprintCallable, Category="Inventory") void CycleQuickItem();
	UFUNCTION(BlueprintPure, Category="Inventory") FName GetQuickItemId() const { return QuickItemId; }
	UFUNCTION(BlueprintCallable, Category="Inventory") void RefillFlasks();

	// --- Save support ---
	void RestoreState(const TArray<FHRItemInstance>& InItems, const TMap<EHREquipSlot, FGuid>& InEquipped, FName InQuickItem, int32 InMaxFlasks);
	const TMap<EHREquipSlot, FGuid>& GetEquippedMap() const { return Equipped; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") TArray<FHRItemAmount> StartingItems;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") TArray<FName> StartingEquipment;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") FName FlaskItemId = TEXT("Item.EmberGourd");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory") int32 MaxFlaskCharges = 4;

	UPROPERTY(BlueprintAssignable, Category="Inventory") FHRInventoryChangedSignature OnInventoryChanged;
	UPROPERTY(BlueprintAssignable, Category="Inventory") FHRItemAddedSignature OnItemAdded;

protected:
	UPROPERTY(VisibleInstanceOnly, Category="Inventory") TArray<FHRItemInstance> Items;
	UPROPERTY(VisibleInstanceOnly, Category="Inventory") TMap<EHREquipSlot, FGuid> Equipped;
	UPROPERTY(VisibleInstanceOnly, Category="Inventory") FName QuickItemId;

private:
	void ApplyEquipmentEffects(EHREquipSlot Slot);
	void ApplyConsumable(const UHRItemDefinition& Definition);
	static FName SlotSourceName(EHREquipSlot Slot);
	void NotifyChanged(FName ItemId);
};
