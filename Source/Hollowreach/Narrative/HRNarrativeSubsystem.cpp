#include "Narrative/HRNarrativeSubsystem.h"
#include "Core/HRCampaignRules.h"
#include "Systems/HRSaveSubsystem.h"

#include "AI/HRAIController.h"
#include "Characters/HRCharacterBase.h"
#include "Characters/HRCompanionCharacter.h"
#include "Characters/HRNPCCharacter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Core/HRStatics.h"
#include "EngineUtils.h"
#include "Game/HRPlayerController.h"
#include "Narrative/HRQuestSubsystem.h"
#include "Systems/HRContentSubsystem.h"
#include "Systems/HRFactionSubsystem.h"
#include "Systems/HRTimeWeatherSubsystem.h"

#define LOCTEXT_NAMESPACE "HRNarrative"

void UHRNarrativeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UHRContentSubsystem>();
	Super::Initialize(Collection);
}

AHRPlayerCharacter* UHRNarrativeSubsystem::GetPlayer() const
{
	return DialoguePlayer.IsValid() ? DialoguePlayer.Get() : UHRStatics::GetHRPlayer(GetGameInstance());
}

// --- Flags -------------------------------------------------------------------------------------

int32 UHRNarrativeSubsystem::GetFlag(FName Flag) const
{
	const int32* Value = Flags.Find(Flag);
	return Value ? *Value : 0;
}

void UHRNarrativeSubsystem::SetFlag(FName Flag, int32 Value)
{
	Flags.Add(Flag, Value);
	if (UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>())
	{
		Quests->NotifyFlag(Flag, Value);
	}
}

void UHRNarrativeSubsystem::AddFlag(FName Flag, int32 Delta)
{
	SetFlag(Flag, GetFlag(Flag) + Delta);
}

// --- Knowledge ---------------------------------------------------------------------------------

void UHRNarrativeSubsystem::AddKnowledge(FName KnowledgeId, bool bNotify)
{
	if (KnowledgeId.IsNone() || Knowledge.Contains(KnowledgeId))
	{
		return;
	}
	Knowledge.Add(KnowledgeId);
	if (bNotify)
	{
		const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
		UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("Learned", "Knowledge gained: {0}"), Content ? Content->GetLoreTitle(KnowledgeId) : FText::FromName(KnowledgeId)),
			FLinearColor(0.75f, 0.85f, 1.f));
	}
	OnKnowledgeGained.Broadcast(KnowledgeId);
	if (UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>())
	{
		Quests->NotifyFlag(KnowledgeId, 1);
	}
}

void UHRNarrativeSubsystem::AddClue(FName ClueId)
{
	if (ClueId.IsNone() || Clues.Contains(ClueId))
	{
		return;
	}
	Clues.Add(ClueId);
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("Clue", "Clue noted: {0}"), Content ? Content->GetLoreTitle(ClueId) : FText::FromName(ClueId)),
		FLinearColor(0.5f, 0.9f, 0.9f));
	if (UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>())
	{
		Quests->NotifyEvent(EHRObjectiveType::Interact, ClueId, 1);
	}
	TryResolveDeductions();
}

void UHRNarrativeSubsystem::TryResolveDeductions()
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	if (!Content)
	{
		return;
	}
	for (const FHRDeduction& Deduction : Content->Deductions)
	{
		if (SolvedDeductions.Contains(Deduction.DeductionId))
		{
			continue;
		}
		bool bComplete = true;
		for (const FName& Clue : Deduction.RequiredClues)
		{
			bComplete &= Clues.Contains(Clue);
		}
		if (!bComplete)
		{
			continue;
		}
		SolvedDeductions.Add(Deduction.DeductionId);
		UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("Deduction", "Deduction: {0}"), Deduction.Title), FLinearColor(0.4f, 1.f, 1.f), true);
		UHRStatics::Notify(GetGameInstance(), Deduction.Conclusion, FLinearColor(0.7f, 0.95f, 0.95f));
		AddKnowledge(Deduction.ResultKnowledge, false);
		if (UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>())
		{
			Quests->NotifyEvent(EHRObjectiveType::Deduction, Deduction.DeductionId, 1);
		}
	}
}

// --- Conditions --------------------------------------------------------------------------------

bool UHRNarrativeSubsystem::EvaluateCondition(const FHRCondition& Condition, const AHRPlayerCharacter* Player) const
{
	const UHRFactionSubsystem* Factions = GetGameInstance()->GetSubsystem<UHRFactionSubsystem>();
	const UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>();

	switch (Condition.Type)
	{
	case EHRConditionType::FlagAtLeast: return GetFlag(Condition.Key) >= Condition.Value;
	case EHRConditionType::FlagBelow: return GetFlag(Condition.Key) < Condition.Value;
	case EHRConditionType::HasKnowledge: return HasKnowledge(Condition.Key);
	case EHRConditionType::LacksKnowledge: return !HasKnowledge(Condition.Key);
	case EHRConditionType::ReputationAtLeast: return Factions && Factions->GetReputation(Condition.Key) >= Condition.Value;
	case EHRConditionType::ReputationBelow: return Factions && Factions->GetReputation(Condition.Key) < Condition.Value;
	case EHRConditionType::HasItem: return Player && Player->GetInventory()->HasItem(Condition.Key, FMath::Max(1, Condition.Value));
	case EHRConditionType::QuestActive: return Quests && Quests->GetQuestState(Condition.Key) == EHRQuestState::Active;
	case EHRConditionType::QuestCompleted: return Quests && Quests->GetQuestState(Condition.Key) == EHRQuestState::Completed;
	case EHRConditionType::QuestNotStarted: return Quests && Quests->GetQuestState(Condition.Key) == EHRQuestState::NotStarted;
	case EHRConditionType::PlayerLevelAtLeast: return Player && Player->GetProgression()->GetLevel() >= Condition.Value;
	case EHRConditionType::AttributeAtLeast: return Player && Player->GetProgression()->GetAttribute(Condition.Attribute) >= Condition.Value;
	case EHRConditionType::DeductionSolved: return IsDeductionSolved(Condition.Key);
	case EHRConditionType::IsNight:
	{
		const UWorld* World = GetGameInstance()->GetWorld();
		const UHRTimeWeatherSubsystem* TimeWeather = World ? World->GetSubsystem<UHRTimeWeatherSubsystem>() : nullptr;
		return TimeWeather && TimeWeather->IsNight() == (Condition.Value != 0);
	}
	}
	return false;
}

bool UHRNarrativeSubsystem::EvaluateConditions(const TArray<FHRCondition>& Conditions, const AHRPlayerCharacter* Player) const
{
	for (const FHRCondition& Condition : Conditions)
	{
		if (!EvaluateCondition(Condition, Player))
		{
			return false;
		}
	}
	return true;
}

FText UHRNarrativeSubsystem::DescribeCondition(const FHRCondition& Condition) const
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	switch (Condition.Type)
	{
	case EHRConditionType::HasKnowledge:
		return FText::Format(LOCTEXT("ReqKnowledge", "Requires knowledge: {0}"), Content ? Content->GetLoreTitle(Condition.Key) : FText::FromName(Condition.Key));
	case EHRConditionType::ReputationAtLeast:
	{
		const FHRFactionDefinition* Faction = Content ? Content->FindFaction(Condition.Key) : nullptr;
		return FText::Format(LOCTEXT("ReqRep", "Requires standing with {0}"), Faction ? Faction->DisplayName : FText::FromName(Condition.Key));
	}
	case EHRConditionType::HasItem:
	{
		const UHRItemDefinition* Item = Content ? Content->FindItem(Condition.Key) : nullptr;
		return FText::Format(LOCTEXT("ReqItem", "Requires {0}"), Item ? Item->DisplayName : FText::FromName(Condition.Key));
	}
	case EHRConditionType::AttributeAtLeast:
		return FText::Format(LOCTEXT("ReqAttr", "Requires {0} {1}"), UEnum::GetDisplayValueAsText(Condition.Attribute), Condition.Value);
	case EHRConditionType::PlayerLevelAtLeast:
		return FText::Format(LOCTEXT("ReqLevel", "Requires level {0}"), Condition.Value);
	case EHRConditionType::DeductionSolved:
		return LOCTEXT("ReqDeduction", "Requires solving the mystery");
	default:
		return LOCTEXT("ReqLocked", "Unavailable");
	}
}

// --- Consequences ------------------------------------------------------------------------------

void UHRNarrativeSubsystem::ApplyConsequences(const TArray<FHRConsequence>& Consequences, AHRPlayerCharacter* Player, AActor* Source)
{
	for (const FHRConsequence& Consequence : Consequences)
	{
		ApplyConsequence(Consequence, Player, Source);
	}
}

void UHRNarrativeSubsystem::ApplyConsequence(const FHRConsequence& Consequence, AHRPlayerCharacter* Player, AActor* Source)
{
	UHRFactionSubsystem* Factions = GetGameInstance()->GetSubsystem<UHRFactionSubsystem>();
	UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>();

	switch (Consequence.Type)
	{
	case EHRConsequenceType::SetFlag: SetFlag(Consequence.Key, Consequence.Value); break;
	case EHRConsequenceType::AddFlag: AddFlag(Consequence.Key, Consequence.Value); break;
	case EHRConsequenceType::AddKnowledge: AddKnowledge(Consequence.Key); break;
	case EHRConsequenceType::ModifyReputation:
		if (Factions)
		{
			Factions->ModifyReputation(Consequence.Key, static_cast<float>(Consequence.Value));
		}
		break;
	case EHRConsequenceType::StartQuest: if (Quests) { Quests->StartQuest(Consequence.Key); } break;
	case EHRConsequenceType::SetQuestStage: if (Quests) { Quests->SetStage(Consequence.Key, Consequence.SecondKey); } break;
	case EHRConsequenceType::CompleteQuest: if (Quests) { Quests->CompleteQuest(Consequence.Key); } break;
	case EHRConsequenceType::FailQuest: if (Quests) { Quests->FailQuest(Consequence.Key); } break;
	case EHRConsequenceType::GiveItem:
		if (Player)
		{
			Player->GetInventory()->AddItem(Consequence.Key, FMath::Max(1, Consequence.Value), true);
		}
		break;
	case EHRConsequenceType::TakeItem:
		if (Player)
		{
			Player->GetInventory()->RemoveItem(Consequence.Key, FMath::Max(1, Consequence.Value));
		}
		break;
	case EHRConsequenceType::GiveEchoes:
		if (Player)
		{
			Consequence.Value >= 0 ? Player->GetProgression()->AddEchoes(Consequence.Value, true) : (void)Player->GetProgression()->SpendEchoes(-Consequence.Value);
		}
		break;
	case EHRConsequenceType::OpenShop:
		bShopRequested = true;
		bEndRequested = true;
		break;
	case EHRConsequenceType::RecruitCompanion:
		if (AHRCompanionCharacter* Companion = Cast<AHRCompanionCharacter>(Source))
		{
			Companion->Recruit(Player);
			UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("Joined", "{0} joins you."), Companion->GetDisplayName()), FLinearColor(0.5f, 1.f, 0.6f), true);
		}
		break;
	case EHRConsequenceType::CompanionApproval:
	{
		bool bApplied = false;
		for (TActorIterator<AHRCompanionCharacter> It(GetGameInstance()->GetWorld()); It; ++It)
		{
			if (It->CompanionId == Consequence.Key)
			{
				It->ModifyApproval(Consequence.Value);
				bApplied = true;
			}
		}
		if (!bApplied)
		{
			AddFlag(FName(*FString::Printf(TEXT("%s.Approval"), *Consequence.Key.ToString())), Consequence.Value);
		}
		break;
	}
	case EHRConsequenceType::TriggerEnding:
		bEndRequested = true;
		TriggerEnding(Consequence.Key);
		break;
	case EHRConsequenceType::MakeSpeakerHostile:
		if (AHRCharacterBase* Speaker = Cast<AHRCharacterBase>(Source))
		{
			if (Factions)
			{
				Factions->ModifyReputation(Speaker->FactionId, -100.f);
			}
			if (AHRAIController* AI = Cast<AHRAIController>(Speaker->GetController()))
			{
				AI->AlertTo(Player);
			}
		}
		bEndRequested = true;
		break;
	case EHRConsequenceType::Notify:
		UHRStatics::Notify(GetGameInstance(), Consequence.Text, FLinearColor(0.9f, 0.9f, 0.75f));
		break;
	}
}

// --- Dialogue ----------------------------------------------------------------------------------

bool UHRNarrativeSubsystem::StartDialogue(FName DialogueId, AActor* Speaker, FName SpeakerId, AHRPlayerCharacter* Player)
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRDialogue* Dialogue = Content ? Content->FindDialogue(DialogueId) : nullptr;
	if (!Dialogue || !Player)
	{
		return false;
	}

	CurrentDialogueId = DialogueId;
	DialogueSpeaker = Speaker;
	DialoguePlayer = Player;
	LastCheckResult = FText::GetEmpty();
	bEndRequested = false;
	bShopRequested = false;

	FName StartNode = Dialogue->StartNode;
	for (const FHRDialogueEntryPoint& Entry : Dialogue->EntryPoints)
	{
		if (EvaluateConditions(Entry.Conditions, Player))
		{
			StartNode = Entry.NodeId;
			break;
		}
	}

	if (AHRPlayerController* PC = Cast<AHRPlayerController>(Player->GetController()))
	{
		PC->SetUIMode(EHRUIMode::Dialogue);
	}
	if (UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>())
	{
		Quests->NotifyEvent(EHRObjectiveType::Talk, SpeakerId, 1);
	}
	EnterNode(StartNode);
	return true;
}

void UHRNarrativeSubsystem::EnterNode(FName NodeId)
{
	CurrentNodeId = NodeId;
	const FHRDialogueNode* Node = GetCurrentNode();
	if (!Node)
	{
		EndDialogue();
		return;
	}
	ApplyConsequences(Node->OnEnter, DialoguePlayer.Get(), DialogueSpeaker.Get());
	OnDialogueChanged.Broadcast();
	if (bEndRequested)
	{
		EndDialogue();
	}
}

void UHRNarrativeSubsystem::EndDialogue()
{
	AHRPlayerCharacter* Player = DialoguePlayer.Get();
	AActor* Speaker = DialogueSpeaker.Get();
	const bool bOpenShop = bShopRequested;

	CurrentDialogueId = NAME_None;
	CurrentNodeId = NAME_None;
	bEndRequested = false;
	bShopRequested = false;
	OnDialogueChanged.Broadcast();

	AHRPlayerController* PC = Player ? Cast<AHRPlayerController>(Player->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}
	if (!ActiveEnding.IsNone() && PC->GetUIMode() == EHRUIMode::Ending)
	{
		return;
	}
	if (bOpenShop)
	{
		PC->OpenShop(Cast<AHRNPCCharacter>(Speaker));
	}
	else if (PC->GetUIMode() == EHRUIMode::Dialogue)
	{
		PC->SetUIMode(EHRUIMode::Gameplay);
	}
}

const FHRDialogueNode* UHRNarrativeSubsystem::GetCurrentNode() const
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRDialogue* Dialogue = Content ? Content->FindDialogue(CurrentDialogueId) : nullptr;
	return Dialogue ? Dialogue->FindNode(CurrentNodeId) : nullptr;
}

void UHRNarrativeSubsystem::BuildVisibleChoices(TArray<int32>& OutIndices, TArray<FHRChoiceView>* OutViews) const
{
	const FHRDialogueNode* Node = GetCurrentNode();
	if (!Node)
	{
		return;
	}
	const AHRPlayerCharacter* Player = DialoguePlayer.Get();
	for (int32 Index = 0; Index < Node->Choices.Num(); ++Index)
	{
		const FHRDialogueChoice& Choice = Node->Choices[Index];
		const bool bAvailable = EvaluateConditions(Choice.Conditions, Player);
		if (!bAvailable && !Choice.bShowWhenLocked)
		{
			continue;
		}
		OutIndices.Add(Index);
		if (OutViews)
		{
			FHRChoiceView View;
			View.bAvailable = bAvailable;
			View.Text = Choice.Text;
			if (Choice.bHasCheck)
			{
				View.Text = FText::Format(LOCTEXT("CheckChoice", "[{0} DC {1}] {2}"), UEnum::GetDisplayValueAsText(Choice.CheckAttribute), Choice.CheckDC, Choice.Text);
			}
			if (!bAvailable)
			{
				View.Requirement = !Choice.LockedHint.IsEmpty() ? Choice.LockedHint
					: (Choice.Conditions.Num() > 0 ? DescribeCondition(Choice.Conditions[0]) : FText::GetEmpty());
			}
			OutViews->Add(View);
		}
	}
	if (OutIndices.Num() == 0 && OutViews)
	{
		// Every node can always be left.
		FHRChoiceView View;
		View.Text = LOCTEXT("Leave", "[Leave]");
		OutViews->Add(View);
	}
}

void UHRNarrativeSubsystem::GetCurrentChoices(TArray<FHRChoiceView>& OutChoices) const
{
	TArray<int32> Indices;
	BuildVisibleChoices(Indices, &OutChoices);
}

bool UHRNarrativeSubsystem::SelectChoice(int32 VisibleIndex)
{
	const FHRDialogueNode* Node = GetCurrentNode();
	if (!Node)
	{
		return false;
	}
	TArray<int32> Indices;
	BuildVisibleChoices(Indices, nullptr);
	if (Indices.Num() == 0)
	{
		EndDialogue();
		return true;
	}
	if (!Indices.IsValidIndex(VisibleIndex))
	{
		return false;
	}

	// Copy: consequences may end the dialogue and invalidate the node.
	const FHRDialogueChoice Choice = Node->Choices[Indices[VisibleIndex]];
	AHRPlayerCharacter* Player = DialoguePlayer.Get();
	if (!EvaluateConditions(Choice.Conditions, Player))
	{
		return false;
	}

	FName Next = Choice.NextNode;
	if (Choice.bHasCheck && Player)
	{
		const int32 Roll = FMath::RandRange(1, 20);
		const int32 Modifier = (Player->GetProgression()->GetAttribute(Choice.CheckAttribute) - 10) / 2;
		const bool bSuccess = Roll == 20 || (Roll != 1 && Roll + Modifier >= Choice.CheckDC);
		LastCheckResult = FText::Format(LOCTEXT("CheckResult", "{0} check: rolled {1} {2} {3} vs DC {4} - {5}"),
			UEnum::GetDisplayValueAsText(Choice.CheckAttribute), Roll, FText::FromString(Modifier >= 0 ? TEXT("+") : TEXT("-")), FMath::Abs(Modifier), Choice.CheckDC,
			bSuccess ? LOCTEXT("Success", "SUCCESS") : LOCTEXT("Failure", "FAILURE"));
		UHRStatics::Notify(GetGameInstance(), LastCheckResult, bSuccess ? FLinearColor(0.5f, 1.f, 0.5f) : FLinearColor(1.f, 0.5f, 0.4f));
		if (bSuccess)
		{
			ApplyConsequences(Choice.Consequences, Player, DialogueSpeaker.Get());
		}
		else
		{
			ApplyConsequences(Choice.FailConsequences, Player, DialogueSpeaker.Get());
			Next = Choice.FailNode;
		}
	}
	else
	{
		ApplyConsequences(Choice.Consequences, Player, DialogueSpeaker.Get());
	}

	if (bEndRequested || Next.IsNone() || !IsInDialogue())
	{
		if (IsInDialogue())
		{
			EndDialogue();
		}
		return true;
	}
	EnterNode(Next);
	return true;
}

// --- Endings -----------------------------------------------------------------------------------

const FHREndingDefinition* UHRNarrativeSubsystem::FindEnding(FName EndingId) const
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	return Content ? Content->Endings.FindByPredicate([EndingId](const FHREndingDefinition& Ending) { return Ending.EndingId == EndingId; }) : nullptr;
}

FName UHRNarrativeSubsystem::ResolveEnding() const
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	if (!Content)
	{
		return NAME_None;
	}
	TArray<FHREndingDefinition> Sorted = Content->Endings;
	Sorted.Sort([](const FHREndingDefinition& A, const FHREndingDefinition& B) { return A.Priority > B.Priority; });
	for (const FHREndingDefinition& Ending : Sorted)
	{
		if (EvaluateConditions(Ending.Conditions, GetPlayer()))
		{
			return Ending.EndingId;
		}
	}
	return NAME_None;
}

void UHRNarrativeSubsystem::TriggerEnding(FName ForcedEndingId)
{
	if (!ActiveEnding.IsNone() || !HRCampaignRules::CanResolveCampaign(GetFlag(TEXT("Boss.PaleKeeper.Defeated")) > 0, GetFlag(TEXT("Boss.Warden.Defeated")) > 0)) return;
	if (!ForcedEndingId.IsNone())
	{
		const FHREndingDefinition* Ending = FindEnding(ForcedEndingId);
		if (!Ending || !EvaluateConditions(Ending->Conditions, GetPlayer())) return;
	}
	ActiveEnding = ForcedEndingId.IsNone() ? ResolveEnding() : ForcedEndingId;
	if (ActiveEnding.IsNone())
	{
		return;
	}
	SetFlag(TEXT("Game.Ending"), 1);
	if (UHRSaveSubsystem* Saves = GetGameInstance()->GetSubsystem<UHRSaveSubsystem>()) Saves->SaveProgress(GetPlayer());
	OnEndingTriggered.Broadcast(ActiveEnding);
	if (AHRPlayerController* PC = UHRStatics::GetHRPlayerController(GetGameInstance()))
	{
		PC->ShowEnding(ActiveEnding);
	}
}

void UHRNarrativeSubsystem::RestoreState(const TMap<FName, int32>& InFlags, const TArray<FName>& InKnowledge, const TArray<FName>& InClues, const TArray<FName>& InDeductions, FName InEnding)
{
	Flags = InFlags;
	Knowledge = TSet<FName>(InKnowledge);
	Clues = TSet<FName>(InClues);
	SolvedDeductions = TSet<FName>(InDeductions);
	ActiveEnding = InEnding;
}

#undef LOCTEXT_NAMESPACE
