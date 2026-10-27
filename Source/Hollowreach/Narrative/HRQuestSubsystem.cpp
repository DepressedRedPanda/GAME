#include "Narrative/HRQuestSubsystem.h"

#include "Characters/HRPlayerCharacter.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Core/HRStatics.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Systems/HRContentSubsystem.h"
#include "Systems/HRFactionSubsystem.h"

#define LOCTEXT_NAMESPACE "HRQuests"

void UHRQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UHRContentSubsystem>();
	Super::Initialize(Collection);
}

bool UHRQuestSubsystem::StartQuest(FName QuestId)
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRQuestDefinition* Definition = Content ? Content->FindQuest(QuestId) : nullptr;
	if (!Definition || Definition->Stages.Num() == 0 || GetQuestState(QuestId) != EHRQuestState::NotStarted)
	{
		return false;
	}
	FHRQuestProgress& QuestProgress = Progress.Add(QuestId);
	QuestProgress.QuestId = QuestId;
	QuestProgress.State = EHRQuestState::Active;
	UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("Started", "New quest: {0}"), Definition->Title), FLinearColor(1.f, 0.9f, 0.5f), true);
	EnterStage(QuestProgress, Definition->Stages[0].StageId);
	return true;
}

void UHRQuestSubsystem::SetStage(FName QuestId, FName StageId)
{
	FHRQuestProgress* QuestProgress = Progress.Find(QuestId);
	if (!QuestProgress)
	{
		if (!StartQuest(QuestId))
		{
			return;
		}
		QuestProgress = Progress.Find(QuestId);
	}
	if (QuestProgress->State != EHRQuestState::Active)
	{
		return;
	}
	EnterStage(*QuestProgress, StageId);
}

void UHRQuestSubsystem::EnterStage(FHRQuestProgress& QuestProgress, FName StageId)
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRQuestDefinition* Definition = Content ? Content->FindQuest(QuestProgress.QuestId) : nullptr;
	const FHRQuestStage* Stage = Definition ? Definition->FindStage(StageId) : nullptr;
	if (!Stage)
	{
		return;
	}
	QuestProgress.CurrentStage = StageId;
	QuestProgress.ObjectiveCounts.Init(0, Stage->Objectives.Num());
	SyncAbsoluteObjectives(QuestProgress);
	if (!Stage->JournalText.IsEmpty())
	{
		UHRStatics::Notify(GetGameInstance(), Stage->JournalText, FLinearColor(0.95f, 0.9f, 0.7f));
	}
	OnQuestUpdated.Broadcast(QuestProgress.QuestId);
	CheckStage(QuestProgress.QuestId);
}

void UHRQuestSubsystem::SyncAbsoluteObjectives(FHRQuestProgress& QuestProgress)
{
	// Objectives about things you may already have (items, flags, knowledge) start pre-filled.
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRQuestDefinition* Definition = Content ? Content->FindQuest(QuestProgress.QuestId) : nullptr;
	const FHRQuestStage* Stage = Definition ? Definition->FindStage(QuestProgress.CurrentStage) : nullptr;
	const UHRNarrativeSubsystem* Narrative = GetGameInstance()->GetSubsystem<UHRNarrativeSubsystem>();
	const AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(GetGameInstance());
	if (!Stage)
	{
		return;
	}
	for (int32 Index = 0; Index < Stage->Objectives.Num(); ++Index)
	{
		const FHRQuestObjective& Objective = Stage->Objectives[Index];
		if (Objective.Type == EHRObjectiveType::Collect && Player)
		{
			QuestProgress.ObjectiveCounts[Index] = Player->GetInventory()->CountItem(Objective.Target);
		}
		else if (Objective.Type == EHRObjectiveType::Flag && Narrative)
		{
			QuestProgress.ObjectiveCounts[Index] = FMath::Max(Narrative->GetFlag(Objective.Target), Narrative->HasKnowledge(Objective.Target) ? 1 : 0);
		}
		else if (Objective.Type == EHRObjectiveType::Deduction && Narrative)
		{
			QuestProgress.ObjectiveCounts[Index] = Narrative->IsDeductionSolved(Objective.Target) ? 1 : 0;
		}
		else if (Objective.Type == EHRObjectiveType::Interact && Narrative)
		{
			const FString Target = Objective.Target.ToString();
			const bool bDone = Narrative->HasClue(Objective.Target)
				|| Narrative->GetFlag(FName(*(Target + TEXT(".Solved")))) > 0
				|| Narrative->GetFlag(FName(*(Target + TEXT(".Discovered")))) > 0
				|| Narrative->GetFlag(FName(*(Target + TEXT(".Collected")))) > 0
				|| Narrative->GetFlag(FName(*(Target + TEXT(".Opened")))) > 0;
			QuestProgress.ObjectiveCounts[Index] = bDone ? 1 : 0;
		}
		else if (Objective.Type == EHRObjectiveType::Reach && Narrative)
		{
			QuestProgress.ObjectiveCounts[Index] = Narrative->GetFlag(FName(*(Objective.Target.ToString() + TEXT(".Discovered")))) > 0 ? 1 : 0;
		}
		else if (Objective.Type == EHRObjectiveType::Kill && Narrative && Narrative->GetFlag(FName(*(Objective.Target.ToString() + TEXT(".Defeated")))) > 0)
		{
			QuestProgress.ObjectiveCounts[Index] = Objective.Count;
		}
	}
}

void UHRQuestSubsystem::NotifyEvent(EHRObjectiveType Type, FName Target, int32 Amount, int32 AbsoluteCount)
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	if (!Content || Target.IsNone())
	{
		return;
	}

	TArray<FName> Touched;
	for (TPair<FName, FHRQuestProgress>& Pair : Progress)
	{
		FHRQuestProgress& QuestProgress = Pair.Value;
		if (QuestProgress.State != EHRQuestState::Active)
		{
			continue;
		}
		const FHRQuestDefinition* Definition = Content->FindQuest(QuestProgress.QuestId);
		const FHRQuestStage* Stage = Definition ? Definition->FindStage(QuestProgress.CurrentStage) : nullptr;
		if (!Stage)
		{
			continue;
		}
		for (int32 Index = 0; Index < Stage->Objectives.Num() && Index < QuestProgress.ObjectiveCounts.Num(); ++Index)
		{
			const FHRQuestObjective& Objective = Stage->Objectives[Index];
			if (Objective.Type != Type || Objective.Target != Target)
			{
				continue;
			}
			const int32 Before = QuestProgress.ObjectiveCounts[Index];
			QuestProgress.ObjectiveCounts[Index] = AbsoluteCount >= 0 ? AbsoluteCount : Before + Amount;
			if (QuestProgress.ObjectiveCounts[Index] != Before)
			{
				Touched.AddUnique(QuestProgress.QuestId);
				if (Before < Objective.Count && QuestProgress.ObjectiveCounts[Index] >= Objective.Count)
				{
					UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("ObjectiveDone", "Done: {0}"), Objective.Description), FLinearColor(0.8f, 1.f, 0.6f));
				}
			}
		}
	}

	for (const FName& QuestId : Touched)
	{
		OnQuestUpdated.Broadcast(QuestId);
		CheckStage(QuestId);
	}
}

void UHRQuestSubsystem::CheckStage(FName QuestId)
{
	if (bProcessing)
	{
		return;
	}
	FHRQuestProgress* QuestProgress = Progress.Find(QuestId);
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRQuestDefinition* Definition = Content ? Content->FindQuest(QuestId) : nullptr;
	if (!QuestProgress || !Definition || QuestProgress->State != EHRQuestState::Active)
	{
		return;
	}
	const FHRQuestStage* Stage = Definition->FindStage(QuestProgress->CurrentStage);
	if (!Stage)
	{
		return;
	}

	for (int32 Index = 0; Index < Stage->Objectives.Num(); ++Index)
	{
		const FHRQuestObjective& Objective = Stage->Objectives[Index];
		if (!Objective.bOptional && (!QuestProgress->ObjectiveCounts.IsValidIndex(Index) || QuestProgress->ObjectiveCounts[Index] < Objective.Count))
		{
			return;
		}
	}
	if (Stage->Objectives.Num() == 0 && Stage->NextStage.IsNone() && !Stage->bCompletesQuest)
	{
		return; // A waiting stage advanced only by dialogue.
	}

	// Stage complete.
	const FHRQuestStage StageCopy = *Stage;
	bProcessing = true;
	if (UHRNarrativeSubsystem* Narrative = GetGameInstance()->GetSubsystem<UHRNarrativeSubsystem>())
	{
		Narrative->ApplyConsequences(StageCopy.OnComplete, UHRStatics::GetHRPlayer(GetGameInstance()), nullptr);
	}
	bProcessing = false;

	QuestProgress = Progress.Find(QuestId);
	if (!QuestProgress || QuestProgress->State != EHRQuestState::Active || QuestProgress->CurrentStage != StageCopy.StageId)
	{
		return; // A consequence already moved the quest on.
	}
	if (StageCopy.bCompletesQuest)
	{
		CompleteQuest(QuestId);
	}
	else if (!StageCopy.NextStage.IsNone())
	{
		EnterStage(*QuestProgress, StageCopy.NextStage);
	}
}

void UHRQuestSubsystem::CompleteQuest(FName QuestId)
{
	FHRQuestProgress* QuestProgress = Progress.Find(QuestId);
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRQuestDefinition* Definition = Content ? Content->FindQuest(QuestId) : nullptr;
	if (!QuestProgress || !Definition || QuestProgress->State != EHRQuestState::Active)
	{
		return;
	}
	QuestProgress->State = EHRQuestState::Completed;
	UHRNarrativeSubsystem* RewardNarrative = GetGameInstance()->GetSubsystem<UHRNarrativeSubsystem>();
	const FName RewardFlag(*FString::Printf(TEXT("%s.Rewarded"), *QuestId.ToString()));
	if (RewardNarrative && RewardNarrative->GetFlag(RewardFlag) > 0)
	{
		RewardNarrative->SetFlag(FName(*FString::Printf(TEXT("%s.Completed"), *QuestId.ToString())), 1);
		OnQuestCompleted.Broadcast(QuestId);
		return;
	}
	UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("Completed", "Quest complete: {0}"), Definition->Title), FLinearColor(1.f, 0.85f, 0.3f), true);

	if (AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(GetGameInstance()))
	{
		Player->GetProgression()->AddEchoes(Definition->RewardEchoes, true);
		if (Definition->RewardSkillPoints > 0)
		{
			Player->GetProgression()->GrantSkillPoints(Definition->RewardSkillPoints);
			UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("SkillPoints", "+{0} skill point(s)"), Definition->RewardSkillPoints), FLinearColor(0.6f, 0.8f, 1.f));
		}
		for (const FHRItemAmount& Item : Definition->RewardItems)
		{
			Player->GetInventory()->AddItem(Item.ItemId, Item.Quantity, true);
		}
	}
	if (!Definition->RewardFaction.IsNone())
	{
		if (UHRFactionSubsystem* Factions = GetGameInstance()->GetSubsystem<UHRFactionSubsystem>())
		{
			Factions->ModifyReputation(Definition->RewardFaction, static_cast<float>(Definition->RewardReputation));
		}
	}
	if (UHRNarrativeSubsystem* Narrative = GetGameInstance()->GetSubsystem<UHRNarrativeSubsystem>())
	{
		Narrative->SetFlag(FName(*FString::Printf(TEXT("%s.Completed"), *QuestId.ToString())), 1);
		Narrative->SetFlag(RewardFlag, 1);
	}
	OnQuestCompleted.Broadcast(QuestId);
}

void UHRQuestSubsystem::FailQuest(FName QuestId)
{
	FHRQuestProgress* QuestProgress = Progress.Find(QuestId);
	if (!QuestProgress || QuestProgress->State != EHRQuestState::Active)
	{
		return;
	}
	QuestProgress->State = EHRQuestState::Failed;
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	if (const FHRQuestDefinition* Definition = Content ? Content->FindQuest(QuestId) : nullptr)
	{
		UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("Failed", "Quest failed: {0}"), Definition->Title), FLinearColor(1.f, 0.4f, 0.4f), true);
	}
	OnQuestUpdated.Broadcast(QuestId);
}

EHRQuestState UHRQuestSubsystem::GetQuestState(FName QuestId) const
{
	const FHRQuestProgress* QuestProgress = Progress.Find(QuestId);
	return QuestProgress ? QuestProgress->State : EHRQuestState::NotStarted;
}

TArray<FHRQuestProgress> UHRQuestSubsystem::GetQuestsInState(EHRQuestState State) const
{
	TArray<FHRQuestProgress> Result;
	for (const TPair<FName, FHRQuestProgress>& Pair : Progress)
	{
		if (Pair.Value.State == State)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

FText UHRQuestSubsystem::GetObjectiveText(const FHRQuestProgress& QuestProgress, int32 ObjectiveIndex) const
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRQuestDefinition* Definition = Content ? Content->FindQuest(QuestProgress.QuestId) : nullptr;
	const FHRQuestStage* Stage = Definition ? Definition->FindStage(QuestProgress.CurrentStage) : nullptr;
	if (!Stage || !Stage->Objectives.IsValidIndex(ObjectiveIndex))
	{
		return FText::GetEmpty();
	}
	const FHRQuestObjective& Objective = Stage->Objectives[ObjectiveIndex];
	const int32 Count = QuestProgress.ObjectiveCounts.IsValidIndex(ObjectiveIndex) ? QuestProgress.ObjectiveCounts[ObjectiveIndex] : 0;
	const FString Check = Count >= Objective.Count ? TEXT("[x]") : TEXT("[ ]");
	const FString Optional = Objective.bOptional ? TEXT(" (optional)") : TEXT("");
	if (Objective.Count > 1)
	{
		return FText::FromString(FString::Printf(TEXT("%s %s (%d/%d)%s"), *Check, *Objective.Description.ToString(), FMath::Min(Count, Objective.Count), Objective.Count, *Optional));
	}
	return FText::FromString(FString::Printf(TEXT("%s %s%s"), *Check, *Objective.Description.ToString(), *Optional));
}

FText UHRQuestSubsystem::GetStageText(const FHRQuestProgress& QuestProgress) const
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRQuestDefinition* Definition = Content ? Content->FindQuest(QuestProgress.QuestId) : nullptr;
	const FHRQuestStage* Stage = Definition ? Definition->FindStage(QuestProgress.CurrentStage) : nullptr;
	return Stage ? Stage->JournalText : FText::GetEmpty();
}

FName UHRQuestSubsystem::GetTrackedQuest() const
{
	// Main quest first, then the most recently started side quest.
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	FName Fallback;
	for (const TPair<FName, FHRQuestProgress>& Pair : Progress)
	{
		if (Pair.Value.State != EHRQuestState::Active)
		{
			continue;
		}
		const FHRQuestDefinition* Definition = Content ? Content->FindQuest(Pair.Key) : nullptr;
		if (Definition && Definition->bMainQuest)
		{
			return Pair.Key;
		}
		Fallback = Pair.Key;
	}
	return Fallback;
}

TArray<FHRQuestProgress> UHRQuestSubsystem::GetAllProgress() const
{
	TArray<FHRQuestProgress> Result;
	Progress.GenerateValueArray(Result);
	return Result;
}

void UHRQuestSubsystem::RestoreState(const TArray<FHRQuestProgress>& InProgress)
{
	Progress.Reset();
	for (const FHRQuestProgress& QuestProgress : InProgress)
	{
		Progress.Add(QuestProgress.QuestId, QuestProgress);
	}
}

#undef LOCTEXT_NAMESPACE
