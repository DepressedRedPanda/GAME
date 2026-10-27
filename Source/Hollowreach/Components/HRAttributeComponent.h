#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/HRTypes.h"
#include "HRAttributeComponent.generated.h"

class UHRAttributeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHRVitalChangedSignature, UHRAttributeComponent*, Attributes, float, Current, float, Max);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRDamagedSignature, UHRAttributeComponent*, Attributes, const FHRDamageInfo&, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRHealthDepletedSignature, UHRAttributeComponent*, Attributes, AActor*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHRPostureBrokenSignature, UHRAttributeComponent*, Attributes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRInjurySignature, UHRAttributeComponent*, Attributes, EHRInjury, Injury);

USTRUCT()
struct FHRModifierSet
{
	GENERATED_BODY()

	UPROPERTY() TArray<FHRStatModifier> Modifiers;
	/** World time at which the set expires; negative = permanent. */
	UPROPERTY() double ExpireTime = -1.0;
};

/**
 * Stats and vitals for any combatant.
 *  - Health, Stamina (actions), Posture (Sekiro: fills up, breaks at max), Focus (magic).
 *  - Named modifier sources (equipment, skills, buffs, survival, injuries, transformations).
 *  - Injuries: lasting debuffs from heavy hits, falls, fire and frost, cured by splints or rest.
 */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRAttributeComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Stats ---
	UFUNCTION(BlueprintPure, Category="Attributes") float GetStat(EHRStat Stat) const;
	UFUNCTION(BlueprintPure, Category="Attributes") float GetBaseStat(EHRStat Stat) const;
	UFUNCTION(BlueprintCallable, Category="Attributes") void SetBaseStat(EHRStat Stat, float Value);
	UFUNCTION(BlueprintCallable, Category="Attributes") void ScaleBaseStat(EHRStat Stat, float Factor);
	UFUNCTION(BlueprintCallable, Category="Attributes") void SetModifierSource(FName Source, const TArray<FHRStatModifier>& Modifiers);
	UFUNCTION(BlueprintCallable, Category="Attributes") void AddTimedModifiers(FName Source, const TArray<FHRStatModifier>& Modifiers, float Duration);
	UFUNCTION(BlueprintCallable, Category="Attributes") void RemoveModifierSource(FName Source);
	UFUNCTION(BlueprintPure, Category="Attributes") bool HasModifierSource(FName Source) const;

	// --- Vitals ---
	UFUNCTION(BlueprintPure, Category="Attributes") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category="Attributes") float GetStamina() const { return Stamina; }
	UFUNCTION(BlueprintPure, Category="Attributes") float GetPosture() const { return Posture; }
	UFUNCTION(BlueprintPure, Category="Attributes") float GetFocus() const { return Focus; }
	UFUNCTION(BlueprintPure, Category="Attributes") float GetMaxHealth() const { return GetStat(EHRStat::MaxHealth); }
	UFUNCTION(BlueprintPure, Category="Attributes") float GetMaxStamina() const { return GetStat(EHRStat::MaxStamina); }
	UFUNCTION(BlueprintPure, Category="Attributes") float GetMaxPosture() const { return GetStat(EHRStat::MaxPosture); }
	UFUNCTION(BlueprintPure, Category="Attributes") float GetMaxFocus() const { return GetStat(EHRStat::MaxFocus); }
	UFUNCTION(BlueprintPure, Category="Attributes") float GetHealthPercent() const;
	UFUNCTION(BlueprintPure, Category="Attributes") bool IsAlive() const { return Health > 0.f; }
	UFUNCTION(BlueprintPure, Category="Attributes") bool IsExhausted() const { return Stamina <= 0.f; }

	/** Applies resistances and returns the health damage actually dealt. */
	UFUNCTION(BlueprintCallable, Category="Attributes") float ApplyDamage(const FHRDamageInfo& Damage);
	UFUNCTION(BlueprintCallable, Category="Attributes") void ApplyPostureDamage(float Amount);
	UFUNCTION(BlueprintCallable, Category="Attributes") void Heal(float Amount);
	/** Souls rule: any action is allowed while stamina > 0, even if it overdraws. */
	UFUNCTION(BlueprintCallable, Category="Attributes") bool ConsumeStamina(float Amount);
	UFUNCTION(BlueprintCallable, Category="Attributes") void RestoreStamina(float Amount);
	UFUNCTION(BlueprintCallable, Category="Attributes") bool ConsumeFocus(float Amount);
	UFUNCTION(BlueprintCallable, Category="Attributes") void RestoreFocus(float Amount);
	UFUNCTION(BlueprintCallable, Category="Attributes") void ResetPosture(float Fraction = 0.f);
	UFUNCTION(BlueprintCallable, Category="Attributes") void RefillAll();
	UFUNCTION(BlueprintCallable, Category="Attributes") void SetHealthFraction(float Fraction);
	UFUNCTION(BlueprintCallable, Category="Attributes") void Kill(AActor* Killer);

	// --- Injuries ---
	UFUNCTION(BlueprintCallable, Category="Attributes|Injury") void AddInjury(EHRInjury Injury);
	UFUNCTION(BlueprintCallable, Category="Attributes|Injury") void CureAllInjuries();
	UFUNCTION(BlueprintPure, Category="Attributes|Injury") TArray<EHRInjury> GetInjuries() const { return Injuries; }
	UFUNCTION(BlueprintCallable, Category="Attributes|Injury") void SetInjuries(const TArray<EHRInjury>& InInjuries);
	UFUNCTION(BlueprintPure, Category="Attributes|Injury") static FText GetInjuryName(EHRInjury Injury);

	/** Guarding doubles posture recovery, like holding block in Sekiro. */
	void SetGuarding(bool bInGuarding) { bGuarding = bInGuarding; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attributes") bool bInvulnerable = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attributes") float DamageTakenMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attributes") bool bCanBeInjured = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attributes") float StaminaRegenDelay = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attributes") float PostureRegenDelay = 1.5f;

	UPROPERTY(BlueprintAssignable, Category="Attributes") FHRVitalChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category="Attributes") FHRDamagedSignature OnDamaged;
	UPROPERTY(BlueprintAssignable, Category="Attributes") FHRHealthDepletedSignature OnHealthDepleted;
	UPROPERTY(BlueprintAssignable, Category="Attributes") FHRPostureBrokenSignature OnPostureBroken;
	UPROPERTY(BlueprintAssignable, Category="Attributes") FHRInjurySignature OnInjured;

protected:
	UPROPERTY(EditAnywhere, Category="Attributes") TMap<EHRStat, float> BaseStats;

	UPROPERTY(VisibleInstanceOnly, Category="Attributes") float Health = 100.f;
	UPROPERTY(VisibleInstanceOnly, Category="Attributes") float Stamina = 100.f;
	UPROPERTY(VisibleInstanceOnly, Category="Attributes") float Posture = 0.f;
	UPROPERTY(VisibleInstanceOnly, Category="Attributes") float Focus = 50.f;
	UPROPERTY(VisibleInstanceOnly, Category="Attributes") TArray<EHRInjury> Injuries;

	UPROPERTY(Transient) TMap<FName, FHRModifierSet> ModifierSources;

private:
	void MarkStatsDirty();
	void RecalculateStats() const;
	void ClampVitals();
	void RefreshInjuryModifiers();
	static void GetInjuryModifiers(EHRInjury Injury, TArray<FHRStatModifier>& OutModifiers);

	mutable TArray<float> CachedStats;
	mutable bool bStatsDirty = true;

	float StaminaDelayRemaining = 0.f;
	float PostureDelayRemaining = 0.f;
	bool bGuarding = false;
};
