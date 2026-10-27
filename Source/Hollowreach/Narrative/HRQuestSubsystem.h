#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Narrative/HRNarrativeTypes.h"
#include "HRQuestSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHRQuestSignature, FName, QuestId);

/**
 * Staged quests with objectives (kill, collect, reach, talk, flag, interact, deduction).
 * Stages advance linearly by default; dialogue choices can jump to other stages to branch.
 * There are no waypoint markers: the journal describes where to go, the world does the rest.
 */
UCLASS()
class HOLLOWREACH_API UHRQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="Quests") bool StartQuest(FName QuestId);
	UFUNCTION(BlueprintCallable, Category="Quests") void SetStage(FName QuestId, FName StageId);
	UFUNCTION(BlueprintCallable, Category="Quests") void CompleteQuest(FName QuestId);
	UFUNCTION(BlueprintCallable, Category="Quests") void FailQuest(FName QuestId);
	UFUNCTION(BlueprintPure, Category="Quests") EHRQuestState GetQuestState(FName QuestId) const;

	/** Generic event. AbsoluteCount >= 0 sets progress directly (used for item counts and flags). */
	void NotifyEvent(EHRObjectiveType Type, FName Target, int32 Amount = 1, int32 AbsoluteCount = -1);
	void NotifyKill(FName EnemyType) { NotifyEvent(EHRObjectiveType::Kill, EnemyType, 1); }
	void NotifyItemCount(FName ItemId, int32 Count) { NotifyEvent(EHRObjectiveType::Collect, ItemId, 0, Count); }
	void NotifyReach(FName LocationId) { NotifyEvent(EHRObjectiveType::Reach, LocationId, 1); }
	void NotifyFlag(FName Flag, int32 Value) { NotifyEvent(EHRObjectiveType::Flag, Flag, 0, Value); }

	const FHRQuestProgress* GetProgress(FName QuestId) const { return Progress.Find(QuestId); }
	TArray<FHRQuestProgress> GetQuestsInState(EHRQuestState State) const;
	FText GetObjectiveText(const FHRQuestProgress& QuestProgress, int32 ObjectiveIndex) const;
	FText GetStageText(const FHRQuestProgress& QuestProgress) const;

	FName GetTrackedQuest() const;

	TArray<FHRQuestProgress> GetAllProgress() const;
	void RestoreState(const TArray<FHRQuestProgress>& InProgress);

	UPROPERTY(BlueprintAssignable, Category="Quests") FHRQuestSignature OnQuestUpdated;
	UPROPERTY(BlueprintAssignable, Category="Quests") FHRQuestSignature OnQuestCompleted;

private:
	void EnterStage(FHRQuestProgress& QuestProgress, FName StageId);
	void CheckStage(FName QuestId);
	void SyncAbsoluteObjectives(FHRQuestProgress& QuestProgress);

	UPROPERTY() TMap<FName, FHRQuestProgress> Progress;
	bool bProcessing = false;
};
