#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/HRTypes.h"
#include "HRProgressionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHRLevelUpSignature, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHREchoesChangedSignature, int32, NewEchoes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHRSkillUnlockedSignature, FName, SkillId);

/**
 * Souls-style progression: Echoes are both experience and currency. Spend them at a shrine to
 * raise an attribute (one level). Levels also grant skill points for the skill tree. Traversal
 * powers (double jump, wall run, grapple, glide) are unlocked here and gate exploration.
 */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRProgressionComponent();

	virtual void BeginPlay() override;

	// --- Echoes (XP + currency) ---
	UFUNCTION(BlueprintCallable, Category="Progression") void AddEchoes(int32 Amount, bool bNotify = true);
	UFUNCTION(BlueprintCallable, Category="Progression") bool SpendEchoes(int32 Amount);
	UFUNCTION(BlueprintPure, Category="Progression") int32 GetEchoes() const { return Echoes; }
	UFUNCTION(BlueprintCallable, Category="Progression") void SetEchoes(int32 Amount);

	// --- Levels & attributes ---
	UFUNCTION(BlueprintPure, Category="Progression") int32 GetLevel() const { return Level; }
	UFUNCTION(BlueprintPure, Category="Progression") EHRStartingClass GetStartingClass() const { return StartingClass; }
	UFUNCTION(BlueprintCallable, Category="Progression") bool ChooseStartingClass(EHRStartingClass InClass);
	void RestoreStartingClass(EHRStartingClass InClass);
	UFUNCTION(BlueprintPure, Category="Progression") int32 GetLevelUpCost() const;
	UFUNCTION(BlueprintPure, Category="Progression") bool CanLevelUp() const { return Echoes >= GetLevelUpCost(); }
	UFUNCTION(BlueprintCallable, Category="Progression") bool LevelUpAttribute(EHRAttribute Attribute);
	UFUNCTION(BlueprintPure, Category="Progression") int32 GetAttribute(EHRAttribute Attribute) const;
	/** Bonus damage fraction a weapon gets from the given attribute. */
	UFUNCTION(BlueprintPure, Category="Progression") float GetAttributeScaling(EHRAttribute Attribute) const;

	// --- Skills ---
	UFUNCTION(BlueprintPure, Category="Progression") int32 GetSkillPoints() const { return SkillPoints; }
	UFUNCTION(BlueprintCallable, Category="Progression") void GrantSkillPoints(int32 Amount);
	UFUNCTION(BlueprintPure, Category="Progression") bool HasSkill(FName SkillId) const { return UnlockedSkills.Contains(SkillId); }
	UFUNCTION(BlueprintPure, Category="Progression") bool CanUnlockSkill(FName SkillId, FText& OutReason) const;
	UFUNCTION(BlueprintCallable, Category="Progression") bool UnlockSkill(FName SkillId);
	UFUNCTION(BlueprintPure, Category="Progression") TArray<FName> GetUnlockedSkills() const { return UnlockedSkills; }

	// --- Traversal ---
	UFUNCTION(BlueprintPure, Category="Progression") bool HasTraversal(EHRTraversalAbility Ability) const;
	UFUNCTION(BlueprintCallable, Category="Progression") void GrantTraversal(EHRTraversalAbility Ability, bool bNotify = true);
	TArray<EHRTraversalAbility> GetTraversal() const { return Traversal; }

	/** Re-applies attribute & skill stat modifiers and re-grants abilities. */
	void ApplyAllEffects();

	void RestoreState(int32 InLevel, int32 InEchoes, int32 InSkillPoints, const TMap<EHRAttribute, int32>& InAttributes,
		const TArray<FName>& InSkills, const TArray<EHRTraversalAbility>& InTraversal);
	const TMap<EHRAttribute, int32>& GetAttributeMap() const { return Attributes; }

	UPROPERTY(BlueprintAssignable, Category="Progression") FHRLevelUpSignature OnLevelUp;
	UPROPERTY(BlueprintAssignable, Category="Progression") FHREchoesChangedSignature OnEchoesChanged;
	UPROPERTY(BlueprintAssignable, Category="Progression") FHRSkillUnlockedSignature OnSkillUnlocked;

protected:
	UPROPERTY(EditAnywhere, Category="Progression") int32 Level = 1;
	UPROPERTY(VisibleInstanceOnly, Category="Progression") EHRStartingClass StartingClass = EHRStartingClass::Unchosen;
	UPROPERTY(EditAnywhere, Category="Progression") int32 Echoes = 0;
	UPROPERTY(EditAnywhere, Category="Progression") int32 SkillPoints = 2;
	UPROPERTY(EditAnywhere, Category="Progression") TMap<EHRAttribute, int32> Attributes;
	UPROPERTY(EditAnywhere, Category="Progression") TArray<FName> UnlockedSkills;
	UPROPERTY(EditAnywhere, Category="Progression") TArray<EHRTraversalAbility> Traversal;

private:
	void ApplyAttributeModifiers();
	void ApplySkillModifiers();
	void ApplyClassModifiers();
};
