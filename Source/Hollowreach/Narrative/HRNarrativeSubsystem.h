#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Narrative/HRNarrativeTypes.h"
#include "HRNarrativeSubsystem.generated.h"

class AHRPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHRKnowledgeSignature, FName, KnowledgeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHREndingSignature, FName, EndingId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHRDialogueEventSignature);

/**
 * World memory and storytelling:
 *  - Flags: integer world state (choices made, doors opened, bosses beaten, approval...).
 *  - Knowledge: things the *player* has learned. Gates skills, recipes, dialogue and puzzles
 *    (TUNIC / Outer Wilds-style knowledge progression).
 *  - Clues & deductions for investigations.
 *  - Branching dialogue with conditions, consequences and d20 attribute checks.
 *  - Ending resolution from accumulated state.
 */
UCLASS()
class HOLLOWREACH_API UHRNarrativeSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- Flags ---
	UFUNCTION(BlueprintPure, Category="Narrative") int32 GetFlag(FName Flag) const;
	UFUNCTION(BlueprintCallable, Category="Narrative") void SetFlag(FName Flag, int32 Value);
	UFUNCTION(BlueprintCallable, Category="Narrative") void AddFlag(FName Flag, int32 Delta = 1);

	// --- Knowledge & investigation ---
	UFUNCTION(BlueprintCallable, Category="Narrative") void AddKnowledge(FName KnowledgeId, bool bNotify = true);
	UFUNCTION(BlueprintPure, Category="Narrative") bool HasKnowledge(FName KnowledgeId) const { return Knowledge.Contains(KnowledgeId); }
	UFUNCTION(BlueprintCallable, Category="Narrative") void AddClue(FName ClueId);
	UFUNCTION(BlueprintPure, Category="Narrative") bool HasClue(FName ClueId) const { return Clues.Contains(ClueId); }
	UFUNCTION(BlueprintPure, Category="Narrative") bool IsDeductionSolved(FName DeductionId) const { return SolvedDeductions.Contains(DeductionId); }
	TArray<FName> GetKnowledgeList() const { return Knowledge.Array(); }
	TArray<FName> GetClueList() const { return Clues.Array(); }
	TArray<FName> GetSolvedDeductions() const { return SolvedDeductions.Array(); }

	// --- Conditions & consequences ---
	bool EvaluateCondition(const FHRCondition& Condition, const AHRPlayerCharacter* Player) const;
	bool EvaluateConditions(const TArray<FHRCondition>& Conditions, const AHRPlayerCharacter* Player) const;
	FText DescribeCondition(const FHRCondition& Condition) const;
	void ApplyConsequences(const TArray<FHRConsequence>& Consequences, AHRPlayerCharacter* Player, AActor* Source);

	// --- Dialogue ---
	bool StartDialogue(FName DialogueId, AActor* Speaker, FName SpeakerId, AHRPlayerCharacter* Player);
	void EndDialogue();
	bool IsInDialogue() const { return !CurrentDialogueId.IsNone(); }
	const FHRDialogueNode* GetCurrentNode() const;
	void GetCurrentChoices(TArray<FHRChoiceView>& OutChoices) const;
	bool SelectChoice(int32 VisibleIndex);
	FText GetLastCheckResult() const { return LastCheckResult; }
	AActor* GetDialogueSpeaker() const { return DialogueSpeaker.Get(); }

	// --- Endings ---
	void TriggerEnding(FName ForcedEndingId);
	FName ResolveEnding() const;
	FName GetActiveEnding() const { return ActiveEnding; }
	const FHREndingDefinition* FindEnding(FName EndingId) const;

	// --- Save support ---
	const TMap<FName, int32>& GetAllFlags() const { return Flags; }
	void RestoreState(const TMap<FName, int32>& InFlags, const TArray<FName>& InKnowledge, const TArray<FName>& InClues, const TArray<FName>& InDeductions, FName InEnding);

	UPROPERTY(BlueprintAssignable, Category="Narrative") FHRKnowledgeSignature OnKnowledgeGained;
	UPROPERTY(BlueprintAssignable, Category="Narrative") FHREndingSignature OnEndingTriggered;
	UPROPERTY(BlueprintAssignable, Category="Narrative") FHRDialogueEventSignature OnDialogueChanged;

private:
	void EnterNode(FName NodeId);
	void TryResolveDeductions();
	void ApplyConsequence(const FHRConsequence& Consequence, AHRPlayerCharacter* Player, AActor* Source);
	void BuildVisibleChoices(TArray<int32>& OutIndices, TArray<FHRChoiceView>* OutViews) const;
	AHRPlayerCharacter* GetPlayer() const;

	UPROPERTY() TMap<FName, int32> Flags;
	UPROPERTY() TSet<FName> Knowledge;
	UPROPERTY() TSet<FName> Clues;
	UPROPERTY() TSet<FName> SolvedDeductions;
	FName ActiveEnding;

	FName CurrentDialogueId;
	FName CurrentNodeId;
	TWeakObjectPtr<AActor> DialogueSpeaker;
	TWeakObjectPtr<AHRPlayerCharacter> DialoguePlayer;
	FText LastCheckResult;
	bool bEndRequested = false;
	bool bShopRequested = false;
};
