#pragma once

#include "CoreMinimal.h"
#include "Characters/HRCharacterBase.h"
#include "Items/HRItemTypes.h"
#include "HREnemyCharacter.generated.h"

class AHRAIController;

/**
 * AI combatant. Behaviour lives in AHRAIController; this class holds the tuning: how skilled it
 * is at blocking/deflecting, how aggressive, what it drops, and whether it returns when the
 * player rests (Souls-style respawning).
 */
UCLASS()
class HOLLOWREACH_API AHREnemyCharacter : public AHRCharacterBase
{
	GENERATED_BODY()

public:
	AHREnemyCharacter();

	virtual void BeginPlay() override;
	virtual EHRAwareness GetAwarenessTowards(const AActor* Other) const override;
	virtual void Die(AActor* Killer) override;

	/** Called by the AI controller when awareness changes (bosses close their arena on alert). */
	virtual void OnAwarenessChanged(EHRAwareness OldAwareness, EHRAwareness NewAwareness) {}

	/** Souls-style: restore to spawn state (called when the player rests or dies). */
	virtual void ResetEnemy();
	/** Hide and disable after death until the next reset. */
	void Dormant();
	/** Applies a level multiplier to health/damage/posture (dungeon depth, blood moon). */
	void ApplyDifficultyScale(float Scale);

	UFUNCTION(BlueprintPure, Category="Enemy") AHRAIController* GetHRController() const;
	UFUNCTION(BlueprintPure, Category="Enemy") bool IsDormant() const { return bDormant; }

	/** Ranged enemies shoot instead of closing to melee. */
	void FireProjectileAt(AActor* Target);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") FName EnemyTypeId = TEXT("Enemy.Hollow");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") int32 EchoReward = 60;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") FName LootTableId = TEXT("Loot.Hollow");
	/** 0..1: chance and timing quality of blocks, deflects and dodges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") float SkillLevel = 0.3f;
	/** 0..1: how often it presses the attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") float Aggression = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") float PerilousChance = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") float AttackRange = 190.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") int32 MaxComboHits = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") bool bRanged = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") float PreferredRange = 1100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") float SightRange = 2200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") bool bStationary = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI") float WanderRadius = 700.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|AI", meta=(MakeEditWidget=true)) TArray<FVector> PatrolPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") bool bRespawnsOnRest = true;
	/** Temporary spawns (blood moon hordes) are destroyed instead of going dormant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") bool bTemporary = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy") FHRAttackStep PerilousAttack;

	FTransform GetSpawnTransform() const { return SpawnTransform; }
	FVector GetHomeLocation() const { return SpawnTransform.GetLocation(); }

protected:
	virtual void DropLoot(AActor* Killer);

	FTransform SpawnTransform;
	bool bDormant = false;
	bool bRegistered = false;
	FTimerHandle DormantTimer;
};
