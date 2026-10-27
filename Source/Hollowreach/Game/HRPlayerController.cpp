#include "Game/HRPlayerController.h"
#include "Kismet/GameplayStatics.h"

#include "Abilities/HRAbility.h"
#include "Characters/HRNPCCharacter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRAbilityComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCraftingComponent.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Components/HRTraversalComponent.h"
#include "Core/HRInputSet.h"
#include "Core/HRStatics.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Game/HRGameMode.h"
#include "Items/HRLootLibrary.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Narrative/HRQuestSubsystem.h"
#include "Systems/HRContentSubsystem.h"
#include "Systems/HRFactionSubsystem.h"
#include "Systems/HRSaveSubsystem.h"
#include "Systems/HRTimeWeatherSubsystem.h"
#include "World/HRCheckpoint.h"

#define LOCTEXT_NAMESPACE "HRMenus"

namespace HRMenuActions
{
	const FName LevelUp(TEXT("LevelUp"));
	const FName Skill(TEXT("Skill"));
	const FName Craft(TEXT("Craft"));
	const FName Upgrade(TEXT("Upgrade"));
	const FName Travel(TEXT("Travel"));
	const FName UseItem(TEXT("UseItem"));
	const FName Buy(TEXT("Buy"));
	const FName Sell(TEXT("Sell"));
	const FName Choice(TEXT("Choice"));
	const FName Close(TEXT("Close"));
}

AHRPlayerController::AHRPlayerController()
{
}

UHRInputSet* AHRPlayerController::GetInputSet()
{
	if (!InputSet)
	{
		InputSet = NewObject<UHRInputSet>(this, TEXT("HRInputSet"));
		InputSet->Build();
	}
	return InputSet;
}

void AHRPlayerController::EnsureInputMapping()
{
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!Subsystem->HasMappingContext(GetInputSet()->Context))
			{
				Subsystem->AddMappingContext(GetInputSet()->Context, 0);
			}
		}
	}
}

void AHRPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (AHRPlayerCharacter* PossessedCharacter = Cast<AHRPlayerCharacter>(InPawn))
	{
		PlayerCharacter = PossessedCharacter;
	}
	EnsureInputMapping();
}

// --- UI state ----------------------------------------------------------------------------------

void AHRPlayerController::SetUIMode(EHRUIMode NewMode)
{
	if (UIMode == EHRUIMode::Rest && NewMode != EHRUIMode::Rest)
	{
		if (AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get())
		{
			PlayerChar->EndRest();
		}
		RestCheckpoint = nullptr;
	}
	UIMode = NewMode;
	MenuPage = 0;
}

void AHRPlayerController::OpenRestMenu(AHRCheckpoint* Checkpoint)
{
	RestCheckpoint = Checkpoint;
	RestTab = EHRRestTab::LevelUp;
	SetUIMode(EHRUIMode::Rest);
}

void AHRPlayerController::OpenShop(AHRNPCCharacter* Merchant)
{
	if (!Merchant || !Merchant->bIsMerchant)
	{
		SetUIMode(EHRUIMode::Gameplay);
		return;
	}
	ShopNPC = Merchant;
	bShopSellMode = false;
	SetUIMode(EHRUIMode::Shop);
}

void AHRPlayerController::ToggleJournal()
{
	if (UIMode == EHRUIMode::Journal)
	{
		SetUIMode(EHRUIMode::Gameplay);
	}
	else if (UIMode == EHRUIMode::Gameplay)
	{
		SetUIMode(EHRUIMode::Journal);
	}
}

void AHRPlayerController::ShowEnding(FName EndingId)
{
	ShownEnding = EndingId;
	SetUIMode(EHRUIMode::Ending);
}

void AHRPlayerController::CloseMenu()
{
	switch (UIMode)
	{
	case EHRUIMode::Dialogue:
		if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
		{
			Narrative->EndDialogue();
		}
		SetUIMode(EHRUIMode::Gameplay);
		break;
	case EHRUIMode::Dead:
		break;
	default:
		SetUIMode(EHRUIMode::Gameplay);
		break;
	}
}

void AHRPlayerController::NextMenuTab()
{
	MenuPage = 0;
	switch (UIMode)
	{
	case EHRUIMode::Rest:
		RestTab = static_cast<EHRRestTab>((static_cast<uint8>(RestTab) + 1) % static_cast<uint8>(EHRRestTab::MAX));
		break;
	case EHRUIMode::Journal:
		JournalTab = static_cast<EHRJournalTab>((static_cast<uint8>(JournalTab) + 1) % static_cast<uint8>(EHRJournalTab::MAX));
		break;
	case EHRUIMode::Shop:
		bShopSellMode = !bShopSellMode;
		break;
	default:
		break;
	}
}

void AHRPlayerController::HandleMenuHotkey(int32 Index)
{
	TArray<FHRMenuEntry> Entries;
	FText Footer;
	GetAllEntries(Entries, Footer);

	const int32 PageCount = FMath::Max(1, FMath::DivideAndRoundUp(Entries.Num(), EntriesPerPage));
	if (Index == EntriesPerPage)
	{
		MenuPage = (MenuPage + 1) % PageCount;
		return;
	}
	const int32 EntryIndex = MenuPage * EntriesPerPage + Index;
	if (Entries.IsValidIndex(EntryIndex) && Entries[EntryIndex].bEnabled && !Entries[EntryIndex].Action.IsNone())
	{
		ExecuteEntry(Entries[EntryIndex]);
	}
}

// --- Menu content ------------------------------------------------------------------------------

void AHRPlayerController::GetAllEntries(TArray<FHRMenuEntry>& Out, FText& OutFooter) const
{
	switch (UIMode)
	{
	case EHRUIMode::Rest: BuildRestEntries(Out); break;
	case EHRUIMode::Journal: BuildJournalEntries(Out); break;
	case EHRUIMode::Shop: BuildShopEntries(Out); break;
	case EHRUIMode::Dialogue: BuildDialogueEntries(Out, OutFooter); break;
	case EHRUIMode::Ending:
	{
		FHRMenuEntry Entry;
		Entry.Label = LOCTEXT("KeepPlaying", "Continue wandering the world");
		Entry.Action = HRMenuActions::Close;
		Out.Add(Entry);
		break;
	}
	default: break;
	}
}

void AHRPlayerController::BuildMenu(FText& OutTitle, TArray<FText>& OutTabs, int32& OutActiveTab, TArray<FHRMenuEntry>& OutEntries, FText& OutFooter) const
{
	OutActiveTab = 0;
	GetAllEntries(OutEntries, OutFooter);

	switch (UIMode)
	{
	case EHRUIMode::Rest:
		OutTitle = RestCheckpoint.IsValid() ? RestCheckpoint->DisplayName : LOCTEXT("Shrine", "Shrine");
		OutTabs = { LOCTEXT("TabLevel", "Level Up"), LOCTEXT("TabSkills", "Skills"), LOCTEXT("TabCraft", "Craft"), LOCTEXT("TabUpgrade", "Upgrade"), LOCTEXT("TabTravel", "Travel") };
		OutActiveTab = static_cast<int32>(RestTab);
		if (const AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get())
		{
			OutFooter = FText::Format(LOCTEXT("RestFooter", "Echoes: {0}   Skill points: {1}   Level {2}   [Tab] switch  [Backspace] leave"),
				PlayerChar->GetProgression()->GetEchoes(), PlayerChar->GetProgression()->GetSkillPoints(), PlayerChar->GetProgression()->GetLevel());
		}
		break;
	case EHRUIMode::Journal:
		OutTitle = LOCTEXT("Journal", "Journal");
		OutTabs = { LOCTEXT("TabQuests", "Quests"), LOCTEXT("TabInventory", "Inventory"), LOCTEXT("TabCodex", "Codex"), LOCTEXT("TabCharacter", "Character") };
		OutActiveTab = static_cast<int32>(JournalTab);
		OutFooter = LOCTEXT("JournalFooter", "[1-8] use/equip  [9] next page  [Tab] switch  [J] close");
		break;
	case EHRUIMode::Shop:
		OutTitle = ShopNPC.IsValid() ? ShopNPC->GetDisplayName() : LOCTEXT("Merchant", "Merchant");
		OutTabs = { LOCTEXT("TabBuy", "Buy"), LOCTEXT("TabSell", "Sell") };
		OutActiveTab = bShopSellMode ? 1 : 0;
		if (const AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get())
		{
			OutFooter = FText::Format(LOCTEXT("ShopFooter", "Echoes: {0}   [Tab] buy/sell  [Backspace] leave"), PlayerChar->GetProgression()->GetEchoes());
		}
		break;
	case EHRUIMode::Dialogue:
		if (const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
		{
			if (const FHRDialogueNode* Node = Narrative->GetCurrentNode())
			{
				OutTitle = Node->Speaker;
			}
		}
		break;
	case EHRUIMode::Ending:
		if (const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
		{
			if (const FHREndingDefinition* Ending = Narrative->FindEnding(ShownEnding))
			{
				OutTitle = Ending->Title;
				const AHRGameMode* GameMode = GetWorld()->GetAuthGameMode<AHRGameMode>();
				OutFooter = FText::Format(LOCTEXT("EndingFooter", "{0}\n\nDeaths: {1}   Landmarks found: {2}   Secrets: {3}   Puzzles solved: {4}"),
					Ending->Epilogue, GameMode ? GameMode->GetDeathCount() : 0, Narrative->GetFlag(TEXT("Stats.Landmarks")),
					Narrative->GetFlag(TEXT("Stats.Collectibles")), Narrative->GetFlag(TEXT("Stats.Puzzles")));
			}
		}
		break;
	default:
		break;
	}
}

void AHRPlayerController::BuildRestEntries(TArray<FHRMenuEntry>& Out) const
{
	const AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get();
	if (!PlayerChar)
	{
		return;
	}
	const UHRProgressionComponent* Progression = PlayerChar->GetProgression();
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);

	switch (RestTab)
	{
	case EHRRestTab::LevelUp:
	{
		static const TCHAR* Effects[] = {
			TEXT("Health"), TEXT("Stamina, posture, carry weight"), TEXT("Physical power, posture damage"),
			TEXT("Power, crits, parry window"), TEXT("Arcane power, resistances"), TEXT("Focus pool and regeneration")
		};
		for (uint8 Index = 0; Index < static_cast<uint8>(EHRAttribute::MAX); ++Index)
		{
			const EHRAttribute Attribute = static_cast<EHRAttribute>(Index);
			FHRMenuEntry Entry;
			Entry.Label = FText::Format(LOCTEXT("AttrLine", "{0}  {1} -> {2}"), UEnum::GetDisplayValueAsText(Attribute), Progression->GetAttribute(Attribute), Progression->GetAttribute(Attribute) + 1);
			Entry.Detail = FText::Format(LOCTEXT("AttrDetail", "{0}. Cost {1} echoes"), FText::FromString(Effects[Index]), Progression->GetLevelUpCost());
			Entry.bEnabled = Progression->CanLevelUp();
			Entry.Action = HRMenuActions::LevelUp;
			Entry.Payload = Index;
			Out.Add(Entry);
		}
		break;
	}
	case EHRRestTab::Skills:
		if (Content)
		{
			for (const FHRSkillNode& Node : Content->Skills)
			{
				FText Reason;
				const bool bAvailable = Progression->CanUnlockSkill(Node.SkillId, Reason);
				FHRMenuEntry Entry;
				Entry.Label = FText::Format(LOCTEXT("SkillLine", "[{0}] {1} ({2} pt) - {3}"), FText::FromName(Node.Branch), Node.DisplayName, Node.Cost, Reason);
				Entry.Detail = Node.Description;
				Entry.bEnabled = bAvailable;
				Entry.Action = HRMenuActions::Skill;
				Entry.Id = Node.SkillId;
				Out.Add(Entry);
			}
		}
		break;
	case EHRRestTab::Craft:
		for (const FHRRecipe& Recipe : PlayerChar->GetCrafting()->GetKnownRecipes())
		{
			FText Reason;
			FHRMenuEntry Entry;
			Entry.Label = UHRCraftingComponent::DescribeRecipe(this, Recipe);
			Entry.bEnabled = PlayerChar->GetCrafting()->CanCraft(Recipe, Reason);
			Entry.Detail = Reason;
			Entry.Action = HRMenuActions::Craft;
			Entry.Id = Recipe.RecipeId;
			Out.Add(Entry);
		}
		break;
	case EHRRestTab::Upgrade:
		for (const FHRItemInstance& Item : PlayerChar->GetInventory()->GetItems())
		{
			FName Material;
			int32 MaterialCount = 0;
			int32 EchoCost = 0;
			if (!PlayerChar->GetCrafting()->GetUpgradeCost(Item.InstanceId, Material, MaterialCount, EchoCost))
			{
				continue;
			}
			FText Reason;
			const UHRItemDefinition* MaterialDef = Content ? Content->FindItem(Material) : nullptr;
			FHRMenuEntry Entry;
			Entry.Label = UHRLootLibrary::GetItemDisplayName(this, Item);
			Entry.bEnabled = PlayerChar->GetCrafting()->CanUpgrade(Item.InstanceId, Reason);
			Entry.Detail = FText::Format(LOCTEXT("UpgradeCost", "Needs {0} {1} + {2} echoes. {3}"), MaterialCount,
				MaterialDef ? MaterialDef->DisplayName : FText::FromName(Material), EchoCost, Reason);
			Entry.Action = HRMenuActions::Upgrade;
			Entry.Guid = Item.InstanceId;
			Out.Add(Entry);
		}
		break;
	case EHRRestTab::Travel:
		if (const AHRGameMode* GameMode = GetWorld()->GetAuthGameMode<AHRGameMode>())
		{
			for (const AHRCheckpoint* Checkpoint : GameMode->GetDiscoveredCheckpoints())
			{
				FHRMenuEntry Entry;
				Entry.Label = Checkpoint->DisplayName;
				Entry.bEnabled = Checkpoint != RestCheckpoint.Get();
				Entry.Action = HRMenuActions::Travel;
				Entry.Id = Checkpoint->PersistentId;
				Out.Add(Entry);
			}
		}
		break;
	default:
		break;
	}
}

void AHRPlayerController::BuildJournalEntries(TArray<FHRMenuEntry>& Out) const
{
	const AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get();
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	const UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this);
	if (!PlayerChar || !Content)
	{
		return;
	}

	switch (JournalTab)
	{
	case EHRJournalTab::Quests:
		if (Quests)
		{
			for (const EHRQuestState State : { EHRQuestState::Active, EHRQuestState::Completed, EHRQuestState::Failed })
			{
				for (const FHRQuestProgress& Progress : Quests->GetQuestsInState(State))
				{
					const FHRQuestDefinition* Definition = Content->FindQuest(Progress.QuestId);
					if (!Definition)
					{
						continue;
					}
					FHRMenuEntry Entry;
					Entry.Label = FText::Format(LOCTEXT("QuestLine", "{0} [{1}]"), Definition->Title, UEnum::GetDisplayValueAsText(State));
					if (State == EHRQuestState::Active)
					{
						FString Detail = Quests->GetStageText(Progress).ToString();
						const FHRQuestStage* Stage = Definition->FindStage(Progress.CurrentStage);
						for (int32 Index = 0; Stage && Index < Stage->Objectives.Num(); ++Index)
						{
							Detail += TEXT("\n   ") + Quests->GetObjectiveText(Progress, Index).ToString();
						}
						Entry.Detail = FText::FromString(Detail);
					}
					Out.Add(Entry);
				}
			}
		}
		break;
	case EHRJournalTab::Inventory:
	{
		const UHRInventoryComponent* Inventory = PlayerChar->GetInventory();
		for (const FHRItemInstance& Item : Inventory->GetItems())
		{
			const UHRItemDefinition* Definition = Inventory->GetDefinition(Item.ItemId);
			if (!Definition)
			{
				continue;
			}
			FHRMenuEntry Entry;
			const bool bEquipped = Inventory->IsEquipped(Item.InstanceId);
			Entry.Label = FText::Format(LOCTEXT("ItemLine", "{0}{1}"), UHRLootLibrary::GetItemDisplayName(this, Item), bEquipped ? LOCTEXT("Equipped", "  [equipped]") : FText::GetEmpty());
			FString Detail = Definition->Description.ToString();
			TArray<FHRStatModifier> Modifiers = Definition->Modifiers;
			Modifiers.Append(Item.Affixes);
			for (const FHRStatModifier& Modifier : Modifiers)
			{
				Detail += TEXT("  ") + UHRLootLibrary::DescribeModifier(Modifier).ToString();
			}
			if (Definition->Category == EHRItemCategory::Weapon)
			{
				Detail += FString::Printf(TEXT("  Dmg %.0f  Posture %.0f  Scales: %s"), Definition->Weapon.BaseDamage, Definition->Weapon.BasePosture,
					*UEnum::GetDisplayValueAsText(Definition->Weapon.ScalingAttribute).ToString());
			}
			Entry.Detail = FText::FromString(Detail);
			const bool bUsable = Definition->IsEquippable() || Definition->Category == EHRItemCategory::Consumable || Definition->Category == EHRItemCategory::Lore;
			Entry.bEnabled = bUsable;
			Entry.Action = bUsable ? HRMenuActions::UseItem : NAME_None;
			Entry.Guid = Item.InstanceId;
			Out.Add(Entry);
		}
		break;
	}
	case EHRJournalTab::Codex:
		if (Narrative)
		{
			for (const FName& Id : Narrative->GetKnowledgeList())
			{
				const FHRLoreEntry* Lore = Content->FindLore(Id);
				FHRMenuEntry Entry;
				Entry.Label = Lore ? Lore->Title : FText::FromName(Id);
				Entry.Detail = Lore ? Lore->Body : FText::GetEmpty();
				Out.Add(Entry);
			}
			for (const FName& Id : Narrative->GetClueList())
			{
				const FHRLoreEntry* Lore = Content->FindLore(Id);
				FHRMenuEntry Entry;
				Entry.Label = FText::Format(LOCTEXT("ClueLine", "Clue: {0}"), Lore ? Lore->Title : FText::FromName(Id));
				Entry.Detail = Lore ? Lore->Body : FText::GetEmpty();
				Out.Add(Entry);
			}
		}
		break;
	case EHRJournalTab::Character:
	{
		const UHRProgressionComponent* Progression = PlayerChar->GetProgression();
		const UHRAttributeComponent* Attributes = PlayerChar->GetAttributes();
		FString Stats = FString::Printf(TEXT("Level %d   Echoes %d   Skill points %d   Weight %.0f/%.0f"),
			Progression->GetLevel(), Progression->GetEchoes(), Progression->GetSkillPoints(),
			PlayerChar->GetInventory()->GetCurrentWeight(), PlayerChar->GetInventory()->GetCarryCapacity());
		FHRMenuEntry Header;
		Header.Label = FText::FromString(Stats);
		FString AttributeLine;
		for (uint8 Index = 0; Index < static_cast<uint8>(EHRAttribute::MAX); ++Index)
		{
			const EHRAttribute Attribute = static_cast<EHRAttribute>(Index);
			AttributeLine += FString::Printf(TEXT("%s %d   "), *UEnum::GetDisplayValueAsText(Attribute).ToString(), Progression->GetAttribute(Attribute));
		}
		Header.Detail = FText::FromString(AttributeLine);
		Out.Add(Header);

		FHRMenuEntry Derived;
		Derived.Label = LOCTEXT("Derived", "Derived stats");
		Derived.Detail = FText::FromString(FString::Printf(TEXT("HP %.0f  Stamina %.0f  Posture %.0f  Focus %.0f  Phys %.0f  Arcane %.0f  Def %.0f  Crit %.0f%%  Parry %.2fs  Stealth %.0f%%"),
			Attributes->GetMaxHealth(), Attributes->GetMaxStamina(), Attributes->GetMaxPosture(), Attributes->GetMaxFocus(),
			Attributes->GetStat(EHRStat::PhysicalPower), Attributes->GetStat(EHRStat::ArcanePower), Attributes->GetStat(EHRStat::Defense),
			Attributes->GetStat(EHRStat::CritChance) * 100.f, Attributes->GetStat(EHRStat::ParryWindow), Attributes->GetStat(EHRStat::Stealth) * 100.f));
		Out.Add(Derived);

		FHRMenuEntry Movement;
		Movement.Label = LOCTEXT("Movement", "Movement");
		FString MovementLine;
		for (const EHRTraversalAbility Ability : Progression->GetTraversal())
		{
			MovementLine += UEnum::GetDisplayValueAsText(Ability).ToString() + TEXT("  ");
		}
		Movement.Detail = FText::FromString(MovementLine);
		Out.Add(Movement);

		if (const UHRFactionSubsystem* Factions = HRGetSubsystem<UHRFactionSubsystem>(this))
		{
			for (const TPair<FName, FHRFactionDefinition>& Pair : Content->Factions)
			{
				FHRMenuEntry Entry;
				const float Reputation = Factions->GetReputation(Pair.Key);
				Entry.Label = FText::Format(LOCTEXT("FactionLine", "{0}: {1} ({2})"), Pair.Value.DisplayName, UHRFactionSubsystem::GetRankText(Reputation), FMath::RoundToInt(Reputation));
				Out.Add(Entry);
			}
		}
		break;
	}
	default:
		break;
	}
}

void AHRPlayerController::BuildShopEntries(TArray<FHRMenuEntry>& Out) const
{
	const AHRNPCCharacter* Merchant = ShopNPC.Get();
	const AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get();
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	if (!Merchant || !PlayerChar || !Content)
	{
		return;
	}
	if (!bShopSellMode)
	{
		for (int32 Index = 0; Index < Merchant->Stock.Num(); ++Index)
		{
			const FHRItemAmount& Stock = Merchant->Stock[Index];
			const UHRItemDefinition* Definition = Content->FindItem(Stock.ItemId);
			if (!Definition || Stock.Quantity <= 0)
			{
				continue;
			}
			const int32 Price = Merchant->GetBuyPrice(Stock.ItemId);
			FHRMenuEntry Entry;
			Entry.Label = FText::Format(LOCTEXT("BuyLine", "{0} (x{1}) - {2} echoes"), Definition->DisplayName, Stock.Quantity, Price);
			Entry.Detail = Definition->Description;
			Entry.bEnabled = PlayerChar->GetProgression()->GetEchoes() >= Price;
			Entry.Action = HRMenuActions::Buy;
			Entry.Payload = Index;
			Out.Add(Entry);
		}
		return;
	}
	for (const FHRItemInstance& Item : PlayerChar->GetInventory()->GetItems())
	{
		if (PlayerChar->GetInventory()->IsEquipped(Item.InstanceId))
		{
			continue;
		}
		FHRMenuEntry Entry;
		Entry.Label = FText::Format(LOCTEXT("SellLine", "{0} - {1} echoes"), UHRLootLibrary::GetItemDisplayName(this, Item), Merchant->GetSellPrice(Item));
		Entry.Action = HRMenuActions::Sell;
		Entry.Guid = Item.InstanceId;
		Out.Add(Entry);
	}
}

void AHRPlayerController::BuildDialogueEntries(TArray<FHRMenuEntry>& Out, FText& OutFooter) const
{
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	if (!Narrative)
	{
		return;
	}
	if (const FHRDialogueNode* Node = Narrative->GetCurrentNode())
	{
		OutFooter = Node->Line;
	}
	TArray<FHRChoiceView> Choices;
	Narrative->GetCurrentChoices(Choices);
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		FHRMenuEntry Entry;
		Entry.Label = Choices[Index].Text;
		Entry.Detail = Choices[Index].Requirement;
		Entry.bEnabled = Choices[Index].bAvailable;
		Entry.Action = HRMenuActions::Choice;
		Entry.Payload = Index;
		Out.Add(Entry);
	}
}

void AHRPlayerController::ExecuteEntry(const FHRMenuEntry& Entry)
{
	AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get();
	if (!PlayerChar)
	{
		return;
	}

	if (Entry.Action == HRMenuActions::LevelUp)
	{
		PlayerChar->GetProgression()->LevelUpAttribute(static_cast<EHRAttribute>(Entry.Payload));
		PlayerChar->GetAttributes()->RefillAll();
	}
	else if (Entry.Action == HRMenuActions::Skill)
	{
		PlayerChar->GetProgression()->UnlockSkill(Entry.Id);
		PlayerChar->GetTraversal()->RefreshUnlocks();
	}
	else if (Entry.Action == HRMenuActions::Craft)
	{
		PlayerChar->GetCrafting()->Craft(Entry.Id);
	}
	else if (Entry.Action == HRMenuActions::Upgrade)
	{
		PlayerChar->GetCrafting()->UpgradeItem(Entry.Guid);
	}
	else if (Entry.Action == HRMenuActions::Travel)
	{
		if (AHRGameMode* GameMode = GetWorld()->GetAuthGameMode<AHRGameMode>())
		{
			GameMode->FastTravel(PlayerChar, GameMode->FindCheckpoint(Entry.Id));
		}
	}
	else if (Entry.Action == HRMenuActions::UseItem)
	{
		PlayerChar->GetInventory()->UseItem(Entry.Guid);
	}
	else if (Entry.Action == HRMenuActions::Buy)
	{
		if (AHRNPCCharacter* Merchant = ShopNPC.Get())
		{
			Merchant->Buy(PlayerChar, Entry.Payload);
		}
	}
	else if (Entry.Action == HRMenuActions::Sell)
	{
		if (AHRNPCCharacter* Merchant = ShopNPC.Get())
		{
			Merchant->Sell(PlayerChar, Entry.Guid);
		}
	}
	else if (Entry.Action == HRMenuActions::Choice)
	{
		if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
		{
			Narrative->SelectChoice(Entry.Payload);
		}
	}
	else if (Entry.Action == HRMenuActions::Close)
	{
		SetUIMode(EHRUIMode::Gameplay);
	}
}

// --- Notifications -----------------------------------------------------------------------------

void AHRPlayerController::AddNotification(const FText& Text, const FLinearColor& Color, float Duration, bool bLarge)
{
	if (Text.IsEmpty())
	{
		return;
	}
	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
	Notifications.RemoveAll([Now](const FHRNotification& Notification) { return Now - Notification.Time > Notification.Duration; });

	FHRNotification Notification;
	Notification.Text = Text;
	Notification.Color = Color;
	Notification.Time = Now;
	Notification.Duration = Duration;
	Notification.bLarge = bLarge;
	Notifications.Add(Notification);
	if (Notifications.Num() > 12)
	{
		Notifications.RemoveAt(0);
	}
}

// --- Cheats ------------------------------------------------------------------------------------

void AHRPlayerController::HRGive(FName ItemId, int32 Quantity)
{
	if (AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get())
	{
		PlayerChar->GetInventory()->AddItem(ItemId, Quantity, true);
	}
}

void AHRPlayerController::HREchoes(int32 Amount)
{
	if (AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get())
	{
		PlayerChar->GetProgression()->AddEchoes(Amount, true);
	}
}

void AHRPlayerController::HRTime(float Hour)
{
	if (UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>())
	{
		TimeWeather->SetTimeOfDay(Hour);
	}
}

void AHRPlayerController::HRWeather(int32 WeatherIndex)
{
	if (UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>())
	{
		TimeWeather->SetWeather(static_cast<EHRWeather>(FMath::Clamp(WeatherIndex, 0, static_cast<int32>(EHRWeather::MAX) - 1)));
	}
}

void AHRPlayerController::HRSkillPoints(int32 Amount)
{
	if (AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get())
	{
		PlayerChar->GetProgression()->GrantSkillPoints(Amount);
	}
}

void AHRPlayerController::HRUnlockAll()
{
	AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get();
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	if (!PlayerChar || !Content)
	{
		return;
	}
	if (Narrative)
	{
		for (const TPair<FName, FHRLoreEntry>& Pair : Content->Lore)
		{
			Narrative->AddKnowledge(Pair.Key, false);
		}
	}
	PlayerChar->GetProgression()->GrantSkillPoints(100);
	for (const FHRSkillNode& Node : Content->Skills)
	{
		PlayerChar->GetProgression()->UnlockSkill(Node.SkillId);
	}
	for (uint8 Index = 0; Index <= static_cast<uint8>(EHRTraversalAbility::Glide); ++Index)
	{
		PlayerChar->GetProgression()->GrantTraversal(static_cast<EHRTraversalAbility>(Index), false);
	}
	PlayerChar->GetTraversal()->RefreshUnlocks();
}

void AHRPlayerController::HRKnow(FName KnowledgeId)
{
	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->AddKnowledge(KnowledgeId);
	}
}

void AHRPlayerController::HRGod()
{
	bGodMode = !bGodMode;
	if (AHRPlayerCharacter* PlayerChar = PlayerCharacter.Get())
	{
		PlayerChar->GetAttributes()->bInvulnerable = bGodMode;
	}
	AddNotification(FText::FromString(bGodMode ? TEXT("God mode ON") : TEXT("God mode OFF")), FLinearColor::Yellow, 3.f, false);
}

void AHRPlayerController::HRSave()
{
	if (UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(this))
	{
		Saves->SaveProgress(PlayerCharacter.Get());
	}
}

void AHRPlayerController::HRNewGame()
{
	if (UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(this))
	{
		if (!Saves->DeleteSave())
		{
			AddNotification(NSLOCTEXT("HR", "ResetFailed", "Could not create the new-game save. Existing progress was kept."), FLinearColor(1.f, 0.3f, 0.3f), 5.f, true);
			return;
		}
	}
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

#undef LOCTEXT_NAMESPACE
