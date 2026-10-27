#pragma once

#include "CoreMinimal.h"
#include "Core/HRTypes.h"
#include "HRSkillTypes.generated.h"

/** A node in the skill tree. Nodes can grant stats, abilities or traversal powers. */
USTRUCT(BlueprintType)
struct FHRSkillNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") FName SkillId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill", meta=(MultiLine=true)) FText Description;
	/** Blade, Shadow, Arcane, Survival... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") FName Branch;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") int32 Cost = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") int32 RequiredLevel = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") TArray<FName> Prerequisites;
	/** Knowledge-gated skills: you must have *learned* something in the world first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") FName RequiredKnowledge;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") TArray<FHRStatModifier> Modifiers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") FName GrantAbility;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") bool bGrantsTraversal = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill") EHRTraversalAbility Traversal = EHRTraversalAbility::Mantle;
};
