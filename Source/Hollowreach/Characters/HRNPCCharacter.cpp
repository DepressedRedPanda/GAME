#include "Characters/HRNPCCharacter.h"

#include "AI/HRAIController.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Core/HRStatics.h"
#include "Items/HRLootLibrary.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Systems/HRContentSubsystem.h"
#include "Systems/HRFactionSubsystem.h"

#define LOCTEXT_NAMESPACE "HRNPC"

AHRNPCCharacter::AHRNPCCharacter()
{
	SharedAppearanceRole = TEXT("NPC");
	AIControllerClass = AHRAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	FactionId = TEXT("Villagers");
	BodyColor = FLinearColor(0.7f, 0.6f, 0.3f);
	BaseWalkSpeed = 300.f;
}

EHRAwareness AHRNPCCharacter::GetAwarenessTowards(const AActor* Other) const
{
	const AHRAIController* AI = Cast<AHRAIController>(GetController());
	return AI ? AI->GetAwareness() : EHRAwareness::Unaware;
}

bool AHRNPCCharacter::CanInteract(const AHRPlayerCharacter* Player) const
{
	return !bIsDead && !UHRFactionSubsystem::AreHostile(this, Player) && !DialogueId.IsNone();
}

FText AHRNPCCharacter::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	return FText::Format(LOCTEXT("Talk", "Talk to {0}"), DisplayName);
}

void AHRNPCCharacter::Interact(AHRPlayerCharacter* Player)
{
	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->StartDialogue(DialogueId, this, NpcId, Player);
	}
}

float AHRNPCCharacter::GetReputationPriceFactor() const
{
	const UHRFactionSubsystem* Factions = HRGetSubsystem<UHRFactionSubsystem>(this);
	const float Reputation = Factions ? Factions->GetReputation(FactionId) : 0.f;
	// +100 rep = 25% discount, -100 rep = 50% surcharge.
	return Reputation >= 0.f ? 1.f - 0.25f * (Reputation / 100.f) : 1.f + 0.5f * (-Reputation / 100.f);
}

int32 AHRNPCCharacter::GetBuyPrice(FName ItemId) const
{
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	const UHRItemDefinition* Definition = Content ? Content->FindItem(ItemId) : nullptr;
	if (!Definition)
	{
		return 0;
	}
	return FMath::Max(1, FMath::RoundToInt(Definition->BaseValue * PriceMultiplier * GetReputationPriceFactor()));
}

int32 AHRNPCCharacter::GetSellPrice(const FHRItemInstance& Item) const
{
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	const UHRItemDefinition* Definition = Content ? Content->FindItem(Item.ItemId) : nullptr;
	if (!Definition)
	{
		return 0;
	}
	const float Value = Definition->BaseValue * HRItems::RarityValueMultiplier(Item.Rarity) * (1.f + 0.2f * Item.UpgradeLevel);
	return FMath::Max(1, FMath::RoundToInt(Value * 0.3f / GetReputationPriceFactor()));
}

bool AHRNPCCharacter::Buy(AHRPlayerCharacter* Player, int32 StockIndex)
{
	if (!Player || !Stock.IsValidIndex(StockIndex) || Stock[StockIndex].Quantity <= 0)
	{
		return false;
	}
	const FName ItemId = Stock[StockIndex].ItemId;
	const int32 Price = GetBuyPrice(ItemId);
	if (!Player->GetProgression()->SpendEchoes(Price))
	{
		UHRStatics::Notify(this, LOCTEXT("TooPoor", "Not enough echoes."), FLinearColor::Gray);
		return false;
	}
	--Stock[StockIndex].Quantity;
	Player->GetInventory()->AddItem(ItemId, 1, true);
	return true;
}

bool AHRNPCCharacter::Sell(AHRPlayerCharacter* Player, const FGuid& ItemGuid)
{
	UHRInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	const FHRItemInstance* Item = Inventory ? Inventory->FindInstance(ItemGuid) : nullptr;
	if (!Item || Inventory->IsEquipped(ItemGuid))
	{
		return false;
	}
	const UHRItemDefinition* Definition = Inventory->GetDefinition(Item->ItemId);
	if (Definition && Definition->Category == EHRItemCategory::KeyItem)
	{
		UHRStatics::Notify(this, LOCTEXT("KeyItem", "That is too important to sell."), FLinearColor::Gray);
		return false;
	}
	const int32 Price = GetSellPrice(*Item);
	const FName ItemId = Item->ItemId;
	Inventory->RemoveInstance(ItemGuid, 1);
	Player->GetProgression()->AddEchoes(Price, true);

	// Sold goods become stock (a tiny economy).
	FHRItemAmount* Existing = Stock.FindByPredicate([ItemId](const FHRItemAmount& Amount) { return Amount.ItemId == ItemId; });
	if (Existing)
	{
		++Existing->Quantity;
	}
	else
	{
		Stock.Add(FHRItemAmount(ItemId, 1));
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
