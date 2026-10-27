#pragma once

#include "CoreMinimal.h"
#include "Core/HRTypes.h"
#include "Items/HRItemTypes.h"
#include "HRNarrativeTypes.generated.h"

// ---------------------------------------------------------------------------------------------
// Conditions & consequences: the shared vocabulary used by dialogue, quests and endings.
// ---------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EHRConditionType : uint8
{
	FlagAtLeast,
	FlagBelow,
	HasKnowledge,
	LacksKnowledge,
	ReputationAtLeast,
	ReputationBelow,
	HasItem,
	QuestActive,
	QuestCompleted,
	QuestNotStarted,
	PlayerLevelAtLeast,
	AttributeAtLeast,
	DeductionSolved,
	IsNight
};

USTRUCT(BlueprintType)
struct FHRCondition
{
	GENERATED_BODY()

	FHRCondition() {}
	FHRCondition(EHRConditionType InType, FName InKey, int32 InValue = 1) : Type(InType), Key(InKey), Value(InValue) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrative") EHRConditionType Type = EHRConditionType::FlagAtLeast;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrative") FName Key;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrative") int32 Value = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrative") EHRAttribute Attribute = EHRAttribute::Strength;
};

UENUM(BlueprintType)
enum class EHRConsequenceType : uint8
{
	SetFlag,
	AddFlag,
	AddKnowledge,
	ModifyReputation,
	StartQuest,
	SetQuestStage,
	CompleteQuest,
	FailQuest,
	GiveItem,
	TakeItem,
	GiveEchoes,
	OpenShop,
	RecruitCompanion,
	CompanionApproval,
	TriggerEnding,
	MakeSpeakerHostile,
	Notify
};

USTRUCT(BlueprintType)
struct FHRConsequence
{
	GENERATED_BODY()

	FHRConsequence() {}
	FHRConsequence(EHRConsequenceType InType, FName InKey, int32 InValue = 0, FName InSecondKey = NAME_None)
		: Type(InType), Key(InKey), SecondKey(InSecondKey), Value(InValue) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrative") EHRConsequenceType Type = EHRConsequenceType::SetFlag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrative") FName Key;
	/** Quest stage for SetQuestStage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrative") FName SecondKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrative") int32 Value = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrative") FText Text;
};

// ---------------------------------------------------------------------------------------------
// Dialogue
// ---------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FHRDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FHRCondition> Conditions;
	/** Show the choice greyed out (with its requirement) when conditions fail, instead of hiding it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") bool bShowWhenLocked = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FText LockedHint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FHRConsequence> Consequences;
	/** NAME_None ends the conversation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NextNode;

	/** Optional d20 attribute check, Baldur's Gate style. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Check") bool bHasCheck = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Check") EHRAttribute CheckAttribute = EHRAttribute::Mind;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Check") int32 CheckDC = 12;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Check") FName FailNode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue|Check") TArray<FHRConsequence> FailConsequences;
};

USTRUCT(BlueprintType)
struct FHRDialogueNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NodeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FText Speaker;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue", meta=(MultiLine=true)) FText Line;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FHRConsequence> OnEnter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FHRDialogueChoice> Choices;
};

USTRUCT(BlueprintType)
struct FHRDialogueEntryPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FHRCondition> Conditions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName NodeId;
};

USTRUCT(BlueprintType)
struct FHRDialogue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName DialogueId;
	/** Checked in order; the first entry whose conditions pass picks the opening node. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FHRDialogueEntryPoint> EntryPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName StartNode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FHRDialogueNode> Nodes;

	const FHRDialogueNode* FindNode(FName NodeId) const
	{
		return Nodes.FindByPredicate([NodeId](const FHRDialogueNode& Node) { return Node.NodeId == NodeId; });
	}
};

/** What the HUD needs to draw one dialogue option. */
USTRUCT(BlueprintType)
struct FHRChoiceView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FText Text;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") bool bAvailable = true;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FText Requirement;
};

// ---------------------------------------------------------------------------------------------
// Quests
// ---------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EHRObjectiveType : uint8
{
	Kill,
	Collect,
	Reach,
	Talk,
	Flag,
	Interact,
	Deduction
};

USTRUCT(BlueprintType)
struct FHRQuestObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") EHRObjectiveType Type = EHRObjectiveType::Kill;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") FName Target;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") int32 Count = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") bool bOptional = false;
};

USTRUCT(BlueprintType)
struct FHRQuestStage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") FName StageId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest", meta=(MultiLine=true)) FText JournalText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") TArray<FHRQuestObjective> Objectives;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") TArray<FHRConsequence> OnComplete;
	/** Next stage when all objectives are done. Branches are taken by SetQuestStage consequences instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") FName NextStage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") bool bCompletesQuest = false;
};

USTRUCT(BlueprintType)
struct FHRQuestDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") FName QuestId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") bool bMainQuest = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") TArray<FHRQuestStage> Stages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") int32 RewardEchoes = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") int32 RewardSkillPoints = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") TArray<FHRItemAmount> RewardItems;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") FName RewardFaction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest") int32 RewardReputation = 0;

	const FHRQuestStage* FindStage(FName StageId) const
	{
		return Stages.FindByPredicate([StageId](const FHRQuestStage& Stage) { return Stage.StageId == StageId; });
	}
};

UENUM(BlueprintType)
enum class EHRQuestState : uint8
{
	NotStarted,
	Active,
	Completed,
	Failed
};

USTRUCT(BlueprintType)
struct FHRQuestProgress
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest") FName QuestId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest") EHRQuestState State = EHRQuestState::NotStarted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest") FName CurrentStage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Quest") TArray<int32> ObjectiveCounts;
};

// ---------------------------------------------------------------------------------------------
// Lore, investigation, factions and endings
// ---------------------------------------------------------------------------------------------

/** Codex entry for a piece of knowledge or a clue. */
USTRUCT(BlueprintType)
struct FHRLoreEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lore") FName EntryId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lore") FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lore", meta=(MultiLine=true)) FText Body;
};

/** Combine clues into a conclusion (Sinking City / Witcher-style investigation). */
USTRUCT(BlueprintType)
struct FHRDeduction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investigation") FName DeductionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investigation") FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investigation", meta=(MultiLine=true)) FText Conclusion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investigation") TArray<FName> RequiredClues;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investigation") FName ResultKnowledge;
};

USTRUCT(BlueprintType)
struct FHRFactionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faction") FName FactionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faction") FText DisplayName;
	/** Starting reputation with the player, -100..100. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faction") float StartingReputation = 0.f;
	/** Static stance towards other factions, -100..100. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faction") TMap<FName, float> Relations;
	/** Reputation can never rise above this (monsters stay monsters). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Faction") float MaxReputation = 100.f;
};

USTRUCT(BlueprintType)
struct FHREndingDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ending") FName EndingId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ending") FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ending", meta=(MultiLine=true)) FText Epilogue;
	/** Higher priority endings are tested first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ending") int32 Priority = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ending") TArray<FHRCondition> Conditions;
};
