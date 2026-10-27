#pragma once

#include "CoreMinimal.h"
#include "Characters/HREnemyCharacter.h"
#include "HRBossCharacter.generated.h"

class AHRGate;

USTRUCT(BlueprintType)
struct FHRBossPhase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") FText PhaseLine;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") TArray<FHRStatModifier> Modifiers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") float SkillLevel = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") float Aggression = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") float PerilousChance = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") FLinearColor Color = FLinearColor(0.35f, 0.1f, 0.45f);
};

/**
 * Multi-phase boss (Sekiro: one deathblow per phase). Locks its arena with fog gates while the
 * fight is on, resets if the player dies, and stays dead once beaten. Every death to a boss
 * reveals another hint about its weaknesses: learning through failure, made explicit.
 */
UCLASS()
class HOLLOWREACH_API AHRBossCharacter : public AHREnemyCharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign") FName RequiredCampaignFlag;
	bool IsCampaignUnlocked() const;
	AHRBossCharacter();
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;
	virtual void Die(AActor* Killer) override;
	virtual void ResetEnemy() override;
	virtual void OnAwarenessChanged(EHRAwareness OldAwareness, EHRAwareness NewAwareness) override;

	UFUNCTION(BlueprintPure, Category="Boss") bool IsFightActive() const { return bFightActive; }
	UFUNCTION(BlueprintPure, Category="Boss") int32 GetPhaseIndex() const { return PhaseIndex; }
	UFUNCTION(BlueprintPure, Category="Boss") int32 GetPhaseCount() const { return FMath::Max(1, Phases.Num()); }
	UFUNCTION(BlueprintPure, Category="Boss") bool IsDefeated() const;

	/** Called by the game mode when the player dies during this fight. */
	void OnPlayerDefeated();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") FName BossId = TEXT("Boss.Warden");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") TArray<FHRBossPhase> Phases;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") TArray<TObjectPtr<AHRGate>> ArenaGates;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(MultiLine=true)) TArray<FText> Hints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") int32 SkillPointReward = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") FName RewardItem;

protected:
	virtual void HandleHealthDepleted(UHRAttributeComponent* InAttributes, AActor* Killer) override;

	void EnterPhase(int32 NewPhase);
	void SetArenaSealed(bool bSealed);
	FName GetDefeatedFlag() const;

	int32 PhaseIndex = 0;
	bool bFightActive = false;
	bool bTransitioning = false;
};
