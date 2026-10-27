#include "Systems/HRContentSubsystem.h"
#include "Abilities/HRVeilAbilities.h"

void UHRContentSubsystem::RegisterVeilCampaign()
{
    NewItem(TEXT("Item.VeilCipher"), TEXT("Cipher Seal"), TEXT("A seal from the memory gate beneath the Pale Keeper."), EHRItemCategory::KeyItem, 0, 0.f)->MaxStack = 99;
    NewItem(TEXT("Item.VeilRecord"), TEXT("Lost Record"), TEXT("SECOND, FIRST, THIRD. The order survives even when its authors do not."), EHRItemCategory::KeyItem, 0, 0.f)->MaxStack = 99;
    NewItem(TEXT("Item.AncientAlloy"), TEXT("Ancient Alloy"), TEXT("Metal recovered from the Choir Engine's ruins."), EHRItemCategory::Material, 30, 0.2f)->MaxStack = 99;
    FHRConsumableData Ration;
    Ration.Food = 35.f;
    Ration.Heal = 10.f;
    NewConsumable(TEXT("Item.VeilRation"), TEXT("Trail Ration"), TEXT("A meal prepared from herbs and scavenged supplies."), Ration, 12, 20);
    FHRConsumableData Tonic;
    Tonic.Heal = 25.f;
    Tonic.RestoreFocus = 20.f;
    Tonic.InfectionTreatment = 35.f;
    NewConsumable(TEXT("Item.VeilTonic"), TEXT("Ash Tonic"), TEXT("Restores focus and eases ash infection."), Tonic, 25, 20);
    NewRecipe(TEXT("Recipe.VeilRation"), { FHRItemAmount(TEXT("Item.Herb"), 2) }, FHRItemAmount(TEXT("Item.VeilRation"), 1), EHRCraftStation::None);
    NewRecipe(TEXT("Recipe.VeilTonic"), { FHRItemAmount(TEXT("Item.Herb"), 2), FHRItemAmount(TEXT("Item.AncientAlloy"), 1) }, FHRItemAmount(TEXT("Item.VeilTonic"), 1), EHRCraftStation::None);
    NewLore(TEXT("Lore.VeilRunes"), TEXT("The Ninth Memory"), TEXT("The lost records describe the same force the Wardens call the First Flame. The Choir Engine holds its memories apart. Open its circuit in the order SECOND, FIRST, THIRD."));
    NewLore(TEXT("Lore.Freefolk"), TEXT("The Freefolk"), TEXT("They want the Engine's memories and the First Flame shared among living communities, without another eternal keeper."));
    NewLore(TEXT("Lore.Choir"), TEXT("The Choir"), TEXT("They believe the Engine and Flame must be reunited under one immortal will, even if that will consumes its keeper."));

    for (const FName Id : { FName(TEXT("Freefolk")), FName(TEXT("Choir")) })
    {
        FHRFactionDefinition Faction;
        Faction.FactionId = Id;
        Faction.DisplayName = FText::FromName(Id);
        Faction.Relations.Add(Id == TEXT("Freefolk") ? FName(TEXT("Choir")) : FName(TEXT("Freefolk")), -60.f);
        Faction.Relations.Add(TEXT("Hollow"), -100.f);
        Factions.Add(Id, Faction);
    }

    for (int32 Tier = 1; Tier <= 3; ++Tier)
    {
        const TArray<FName> MightParents = Tier == 1 ? TArray<FName>() : TArray<FName>{ FName(*FString::Printf(TEXT("Skill.Veil.Might%d"), Tier - 1)) };
        const TArray<FName> GuileParents = Tier == 1 ? TArray<FName>() : TArray<FName>{ FName(*FString::Printf(TEXT("Skill.Veil.Guile%d"), Tier - 1)) };
        const TArray<FName> InsightParents = Tier == 1 ? TArray<FName>() : TArray<FName>{ FName(*FString::Printf(TEXT("Skill.Veil.Insight%d"), Tier - 1)) };
        NewSkill(FName(*FString::Printf(TEXT("Skill.Veil.Might%d"), Tier)), TEXT("Blade"), FString::Printf(TEXT("Might %d"), Tier), TEXT("More physical power; uses the shared skill-point pool."), 1, MightParents).Modifiers = { FHRStatModifier(EHRStat::PhysicalPower, 3.f) };
        NewSkill(FName(*FString::Printf(TEXT("Skill.Veil.Guile%d"), Tier)), TEXT("Shadow"), FString::Printf(TEXT("Guile %d"), Tier), TEXT("Reduced detection; uses the shared skill-point pool."), 1, GuileParents).Modifiers = { FHRStatModifier(EHRStat::Stealth, 0.08f) };
        NewSkill(FName(*FString::Printf(TEXT("Skill.Veil.Insight%d"), Tier)), TEXT("Arcane"), FString::Printf(TEXT("Insight %d"), Tier), TEXT("More arcane power; uses the shared skill-point pool."), 1, InsightParents).Modifiers = { FHRStatModifier(EHRStat::ArcanePower, 3.f) };
    }
    AbilityClasses.Add(TEXT("Ability.VoidPulse"), UHRAbility_VoidPulse::StaticClass());
    AbilityClasses.Add(TEXT("Ability.SpectralForm"), UHRAbility_SpectralForm::StaticClass());

    // This replaces the original main-quest route, rather than registering another campaign.
    if (FHRQuestDefinition* Main = Quests.Find(TEXT("Quest.Embers")))
    {
        Main->Title = NSLOCTEXT("Veil", "CampaignTitle", "Veil of Ash: The Dying Flame");
        if (FHRQuestStage* Elder = Main->Stages.FindByPredicate([](const FHRQuestStage& Stage) { return Stage.StageId == TEXT("Elder"); })) Elder->NextStage = TEXT("VeilGate");
        FHRQuestStage GateStage;
        GateStage.StageId = TEXT("VeilGate");
        GateStage.JournalText = NSLOCTEXT("Veil", "GateJournal", "Travel northeast to the Memory Reliquary. Open its gate with ciphers and lost records, or decode its runes.");
        FHRQuestObjective GateObjective;
        GateObjective.Description = NSLOCTEXT("Veil", "GateObjective", "Open the memory gate");
        GateObjective.Type = EHRObjectiveType::Flag;
        GateObjective.Target = TEXT("Veil.GateOpen");
        GateStage.Objectives.Add(GateObjective);
        GateStage.NextStage = TEXT("PaleKeeper");
        FHRQuestStage KeeperStage;
        KeeperStage.StageId = TEXT("PaleKeeper");
        KeeperStage.JournalText = NSLOCTEXT("Veil", "KeeperJournal", "Defeat the Pale Keeper and recover the Choir Engine's memory. Its death will sever the Warden's ward on Ashen Peak.");
        FHRQuestObjective KeeperObjective;
        KeeperObjective.Description = NSLOCTEXT("Veil", "KeeperObjective", "Defeat the Pale Keeper");
        KeeperObjective.Type = EHRObjectiveType::Kill;
        KeeperObjective.Target = TEXT("Boss.PaleKeeper");
        KeeperStage.Objectives.Add(KeeperObjective);
        KeeperStage.NextStage = TEXT("Warden");
        Main->Stages.Insert(GateStage, 2);
        Main->Stages.Insert(KeeperStage, 3);
    }
    if (FHRDialogue* ElderDialogue = Dialogues.Find(TEXT("Dialogue.Elder")))
    {
        if (FHRDialogueNode* Node = ElderDialogue->Nodes.FindByPredicate([](const FHRDialogueNode& Entry) { return Entry.NodeId == TEXT("Warden"); }))
        {
            Node->Line = NSLOCTEXT("Veil", "ElderRoute", "The First Flame and the Choir Engine were once one. The Pale Keeper guards its lost memory in the reliquary northeast of Emberfall. Break that ward first. Only then can the Warden on Ashen Peak be freed from his oath.");
        }
    }

    if (FHRDialogue* Altar = Dialogues.Find(TEXT("Dialogue.Altar")))
    {
        if (FHRDialogueNode* Flame = Altar->Nodes.FindByPredicate([](const FHRDialogueNode& Node) { return Node.NodeId == TEXT("Flame"); }))
        {
            auto AddChoice = [Flame](const TCHAR* Text, FName Ending, FName Faction)
            {
                FHRDialogueChoice Choice;
                Choice.Text = FText::FromString(Text);
                Choice.Consequences = { FHRConsequence(EHRConsequenceType::SetFlag, FName(*(Ending.ToString() + TEXT(".Chosen"))), 1), FHRConsequence(EHRConsequenceType::TriggerEnding, Ending) };
                if (!Faction.IsNone()) Choice.Conditions = { FHRCondition(EHRConditionType::ReputationAtLeast, Faction, 1) };
                Choice.bShowWhenLocked = true;
                Choice.LockedHint = NSLOCTEXT("Veil", "EndingFaction", "Earn this faction's trust at the Memory Reliquary.");
                Flame->Choices.Insert(Choice, Flame->Choices.Num() - 1);
            };
            AddChoice(TEXT("Share the Engine's memories and the Flame with the Freefolk."), TEXT("Ending.Freefolk"), TEXT("Freefolk"));
            AddChoice(TEXT("Bind the Engine and Flame to the Choir."), TEXT("Ending.Choir"), TEXT("Choir"));
            AddChoice(TEXT("Break the Engine and release the Flame from every keeper."), TEXT("Ending.Unbound"), NAME_None);
        }
    }
    auto AddEnding = [this](FName Id, const TCHAR* Title, const TCHAR* Epilogue, FName Faction)
    {
        FHREndingDefinition Ending;
        Ending.EndingId = Id;
        Ending.Title = FText::FromString(Title);
        Ending.Epilogue = FText::FromString(Epilogue);
        // Explicit choices only; never silently take precedence over the original five outcomes.
        Ending.Priority = -100;
        Ending.Conditions = { FHRCondition(EHRConditionType::FlagAtLeast, TEXT("Boss.PaleKeeper.Defeated")), FHRCondition(EHRConditionType::FlagAtLeast, TEXT("Boss.Warden.Defeated")), FHRCondition(EHRConditionType::FlagAtLeast, FName(*(Id.ToString() + TEXT(".Chosen")))) };
        if (!Faction.IsNone()) Ending.Conditions.Add(FHRCondition(EHRConditionType::ReputationAtLeast, Faction, 1));
        Endings.Add(Ending);
    };
    AddEnding(TEXT("Ending.Freefolk"), TEXT("MEMORY BELONGS TO THE LIVING"), TEXT("The Freefolk carry the Engine's memories to the villages. The Flame warms many hearths. Neither a Pale Keeper nor a hollow Warden will stand between the living and their own history again."), TEXT("Freefolk"));
    AddEnding(TEXT("Ending.Choir"), TEXT("THE EVERLASTING CHOIR"), TEXT("You bind memory and Flame together. The Choir sings through every ruin, and your will answers through it. The dying land endures; whether its people remain free is another question."), TEXT("Choir"));
    AddEnding(TEXT("Ending.Unbound"), TEXT("THE LAST WARD IS BROKEN"), TEXT("The Engine breaks. The Flame scatters into the night. The dead keep their memories, the living keep their choices, and the land begins a history no immortal keeper can dictate."), NAME_None);
}
