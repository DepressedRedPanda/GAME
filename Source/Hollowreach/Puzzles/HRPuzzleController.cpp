#include "Puzzles/HRPuzzleController.h"

#include "Characters/HRPlayerCharacter.h"
#include "Components/HRProgressionComponent.h"
#include "Core/HRStatics.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Narrative/HRQuestSubsystem.h"
#include "Puzzles/HRPuzzleElement.h"
#include "World/HRGate.h"

#define LOCTEXT_NAMESPACE "HRPuzzle"

AHRPuzzleController::AHRPuzzleController()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FName AHRPuzzleController::GetSolvedFlag() const
{
	return FName(*FString::Printf(TEXT("%s.Solved"), *PuzzleId.ToString()));
}

void AHRPuzzleController::BeginPlay()
{
	Super::BeginPlay();
	if (PuzzleId.IsNone())
	{
		PuzzleId = GetFName();
	}

	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	if (Narrative && Narrative->GetFlag(GetSolvedFlag()) > 0)
	{
		Solve(true);
		return;
	}
	for (AHRPuzzleElement* Element : Elements)
	{
		if (Element)
		{
			Element->OnStateChanged.AddDynamic(this, &AHRPuzzleController::HandleElementChanged);
		}
	}
}

void AHRPuzzleController::HandleElementChanged(AHRPuzzleElement* Element, int32 NewState)
{
	if (bSolved || bResetting)
	{
		return;
	}

	if (bOrderedSequence)
	{
		const int32 Index = Elements.IndexOfByKey(Element);
		if (!RequiredStates.IsValidIndex(Index) || NewState != RequiredStates[Index])
		{
			return;
		}
		if (Index != SequenceProgress)
		{
			// Wrong order: everything resets.
			bResetting = true;
			for (AHRPuzzleElement* Other : Elements)
			{
				if (Other)
				{
					Other->SetState(0);
				}
			}
			bResetting = false;
			SequenceProgress = 0;
			UHRStatics::Notify(this, LOCTEXT("Reset", "The mechanism grinds and resets."), FLinearColor(0.8f, 0.6f, 0.5f));
			return;
		}
		++SequenceProgress;
		if (SequenceProgress >= RequiredStates.Num())
		{
			Solve(false);
		}
		return;
	}

	for (int32 Index = 0; Index < RequiredStates.Num(); ++Index)
	{
		if (!Elements.IsValidIndex(Index) || !Elements[Index] || Elements[Index]->GetState() != RequiredStates[Index])
		{
			return;
		}
	}
	Solve(false);
}

void AHRPuzzleController::Solve(bool bFromSave)
{
	bSolved = true;
	for (AHRGate* Gate : GatesToOpen)
	{
		if (Gate)
		{
			Gate->Open();
		}
	}
	for (int32 Index = 0; Index < Elements.Num(); ++Index)
	{
		if (Elements[Index])
		{
			if (bFromSave && RequiredStates.IsValidIndex(Index))
			{
				Elements[Index]->SetState(RequiredStates[Index]);
			}
			Elements[Index]->SetLocked(true);
		}
	}
	if (bFromSave)
	{
		return;
	}

	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->SetFlag(GetSolvedFlag(), 1);
		Narrative->AddFlag(TEXT("Stats.Puzzles"), 1);
		if (!RewardKnowledge.IsNone())
		{
			Narrative->AddKnowledge(RewardKnowledge);
		}
	}
	if (AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(this))
	{
		Player->GetProgression()->AddEchoes(RewardEchoes, true);
	}
	if (UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this))
	{
		Quests->NotifyEvent(EHRObjectiveType::Interact, PuzzleId, 1);
	}
	UHRStatics::Notify(this, SolvedText.IsEmpty() ? LOCTEXT("Solved", "Something heavy shifts nearby.") : SolvedText, FLinearColor(0.8f, 0.9f, 1.f), true);
	OnSolved.Broadcast();
}

#undef LOCTEXT_NAMESPACE
