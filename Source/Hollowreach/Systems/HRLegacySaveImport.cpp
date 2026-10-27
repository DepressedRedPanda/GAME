#include "Systems/HRSaveSubsystem.h"
#include "Systems/HRSaveGame.h"
#include "Systems/HRContentSubsystem.h"
#include "AshSave.h"
#include "Core/HRCampaignRules.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

namespace
{
    FName ConvertItem(FName Legacy)
    {
        if (Legacy == TEXT("Wood")) return TEXT("Item.Wood");
        if (Legacy == TEXT("Ore")) return TEXT("Item.IronOre");
        if (Legacy == TEXT("Herb")) return TEXT("Item.Herb");
        if (Legacy == TEXT("Relic")) return TEXT("Item.AncientAlloy");
        if (Legacy == TEXT("Cipher")) return TEXT("Item.VeilCipher");
        if (Legacy == TEXT("Knowledge")) return TEXT("Item.VeilRecord");
        if (Legacy == TEXT("Ration")) return TEXT("Item.VeilRation");
        if (Legacy == TEXT("Tonic")) return TEXT("Item.VeilTonic");
        return NAME_None;
    }

    int32 Count(const UHRSaveGame& Save, FName Id)
    {
        int32 Total = 0;
        for (const FHRItemInstance& Item : Save.Items) if (Item.ItemId == Id) Total += Item.Quantity;
        return Total;
    }

    // Conservative inventory union: retain the larger existing count, rather than double both histories.
    void EnsureItem(UHRSaveGame& Save, const UHRContentSubsystem* Content, FName Id, int32 Quantity)
    {
        const UHRItemDefinition* Definition = Content ? Content->FindItem(Id) : nullptr;
        if (!Definition) return;
        int32 Missing = FMath::Clamp(Quantity, 0, 100000) - Count(Save, Id);
        while (Missing > 0)
        {
            FHRItemInstance Instance;
            Instance.InstanceId = FGuid::NewGuid();
            Instance.ItemId = Id;
            Instance.Quantity = FMath::Min(Missing, FMath::Max(1, Definition->MaxStack));
            Save.Items.Add(Instance);
            Missing -= Instance.Quantity;
        }
    }
}

UHRSaveGame* UHRSaveSubsystem::ImportLegacySaves()
{
    const UHRSaveGame* Primary = Cast<UHRSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("Hollowreach_Slot0"), 0));
    const UAshSave* Veil = Cast<UAshSave>(UGameplayStatics::LoadGameFromSlot(TEXT("VeilOfAsh_Save1"), 0));
    if ((UGameplayStatics::DoesSaveGameExist(TEXT("Hollowreach_Slot0"), 0) && !Primary) || (UGameplayStatics::DoesSaveGameExist(TEXT("VeilOfAsh_Save1"), 0) && !Veil) || (Primary && Primary->SaveVersion > UHRSaveGame::CurrentVersion))
    {
        bUnsupportedSaveVersion = true;
        return nullptr;
    }
    if (!Primary && !Veil) return nullptr;
    UHRSaveGame* Save = Primary ? DuplicateObject<UHRSaveGame>(Primary, this) : NewObject<UHRSaveGame>(this);
    Save->SaveVersion = UHRSaveGame::CurrentVersion;
    Save->bImportedHollowreach = Primary != nullptr;
    Save->bImportedVeil = Veil != nullptr;
    const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();

    if (!Primary)
    {
        Save->LastCheckpoint = TEXT("Shrine.Veil");
        Save->SkillPoints = 2;
        Save->Traversal.Add(EHRTraversalAbility::Mantle);
        for (uint8 Index = 0; Index < static_cast<uint8>(EHRAttribute::MAX); ++Index) Save->Attributes.Add(static_cast<EHRAttribute>(Index), 10);
        EnsureItem(*Save, Content, TEXT("Item.IronBlade"), 1);
        EnsureItem(*Save, Content, TEXT("Item.LeatherCoat"), 1);
        EnsureItem(*Save, Content, TEXT("Item.EmberGourd"), 4);
        Save->QuickItem = TEXT("Item.EmberGourd");
        for (const FHRItemInstance& Item : Save->Items)
        {
            const UHRItemDefinition* Definition = Content ? Content->FindItem(Item.ItemId) : nullptr;
            if (Definition && Definition->EquipSlot != EHREquipSlot::None) Save->Equipped.Add(Definition->EquipSlot, Item.InstanceId);
        }
    }
    if (Veil)
    {
        Save->Level = HRCampaignRules::MergePersistentCount(Save->Level, Veil->Level);
        Save->SkillPoints = Primary ? HRCampaignRules::MergePersistentCount(Save->SkillPoints, Veil->SkillPoints) : FMath::Max(0, Veil->SkillPoints);
        Save->Echoes = HRCampaignRules::ImportRemainingExperience(Save->Echoes, Veil->XP);
        Save->Attributes.FindOrAdd(EHRAttribute::Strength, 10) = HRCampaignRules::ImportBranchAttribute(Save->Attributes.FindRef(EHRAttribute::Strength), Veil->Might);
        Save->Attributes.FindOrAdd(EHRAttribute::Dexterity, 10) = HRCampaignRules::ImportBranchAttribute(Save->Attributes.FindRef(EHRAttribute::Dexterity), Veil->Guile);
        Save->Attributes.FindOrAdd(EHRAttribute::Arcane, 10) = HRCampaignRules::ImportBranchAttribute(Save->Attributes.FindRef(EHRAttribute::Arcane), Veil->Insight);
        if (Save->StartingClass == EHRStartingClass::Unchosen) Save->StartingClass = static_cast<EHRStartingClass>(FMath::Clamp(Veil->HeroClass, 0, 2) + 1);
        for (const FAshStack& Stack : Veil->Items)
        {
            const FName Id = ConvertItem(Stack.Item);
            if (!Id.IsNone()) EnsureItem(*Save, Content, Id, Stack.Count);
            else Save->LegacyUnmappedItems.FindOrAdd(Stack.Item) = HRCampaignRules::MergePersistentCount(Save->LegacyUnmappedItems.FindRef(Stack.Item), Stack.Count);
        }
        for (FHRItemInstance& Item : Save->Items)
        {
            if (Item.ItemId == TEXT("Item.IronBlade"))
            {
                const UHRItemDefinition* Definition = Content ? Content->FindItem(Item.ItemId) : nullptr;
                Item.UpgradeLevel = FMath::Max(Item.UpgradeLevel, FMath::Clamp(Veil->WeaponTier - 1, 0, Definition ? Definition->Weapon.MaxUpgradeLevel : 0));
            }
        }
        Save->Reputation.Add(TEXT("Freefolk"), FMath::Clamp(static_cast<float>(Veil->FreefolkReputation), -100.f, 100.f));
        Save->Reputation.Add(TEXT("Choir"), FMath::Clamp(static_cast<float>(Veil->ChoirReputation), -100.f, 100.f));
        Save->Infection = FMath::Max(Save->Infection, FMath::Clamp(Veil->Infection, 0.f, 100.f));
        Save->TotalKills = HRCampaignRules::MergePersistentCount(Save->TotalKills, Veil->Kills);
        Save->Flags.Add(TEXT("Stats.Kills"), HRCampaignRules::MergePersistentCount(Save->Flags.FindRef(TEXT("Stats.Kills")), Save->TotalKills));
        if (!Primary)
        {
            Save->Food = FMath::Clamp(Veil->Hunger, 0.f, 100.f);
            Save->Water = FMath::Clamp(Veil->Thirst, 0.f, 100.f);
            Save->bHasVitals = true;
            Save->HealthFraction = FMath::Clamp(Veil->Health / 100.f, 0.05f, 1.f);
        }
        if (Count(*Save, TEXT("Item.VeilRecord")) > 0) Save->Knowledge.AddUnique(TEXT("Lore.VeilRunes"));
        // Old Veil saves did not store gate, rune, boss, world pickups or companion state. Never invent them.
    }

    // Preserve the historical outcome as metadata. The new combined finale requires both bosses.
    if (!Save->Ending.IsNone()) Save->LegacyHollowreachEnding = Save->Ending;
    Save->Ending = NAME_None;
    Save->Flags.Add(TEXT("Game.Ending"), 0);
    Save->Flags.Add(TEXT("Campaign.ImportedLegacy"), 1);
    for (FHRQuestProgress& Quest : Save->Quests)
    {
        if (Quest.QuestId != TEXT("Quest.Embers")) continue;
        if (Quest.State == EHRQuestState::Completed) Save->Flags.Add(TEXT("Quest.Embers.Rewarded"), 1);
        if (Quest.CurrentStage == TEXT("Warden") || Quest.CurrentStage == TEXT("Altar") || Quest.State == EHRQuestState::Completed)
        {
            Quest.State = EHRQuestState::Active;
            Quest.CurrentStage = TEXT("VeilGate");
            Quest.ObjectiveCounts = { 0 };
        }
    }
    if (!Primary && Veil)
    {
        FHRQuestProgress Main;
        Main.QuestId = TEXT("Quest.Embers");
        Main.State = EHRQuestState::Active;
        Main.CurrentStage = TEXT("VeilGate");
        Main.ObjectiveCounts = { 0 };
        Save->Quests.Add(Main);
    }
    // Imported key items replace already owned pickups so the new region cannot duplicate them on first load.
    for (int32 Index = 0; Index < FMath::Min(3, Count(*Save, TEXT("Item.VeilCipher"))); ++Index) Save->Flags.Add(FName(*FString::Printf(TEXT("Veil.Cipher%d.Collected"), Index + 1)), 1);
    for (int32 Index = 0; Index < FMath::Min(2, Count(*Save, TEXT("Item.VeilRecord"))); ++Index) Save->Flags.Add(FName(*FString::Printf(TEXT("Veil.Record%d.Collected"), Index + 1)), 1);
    return Save;
}
