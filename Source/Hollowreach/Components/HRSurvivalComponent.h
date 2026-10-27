#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HRSurvivalComponent.generated.h"

/**
 * Hunger, thirst, warmth and fatigue. Rates are per in-game hour so they follow the day cycle.
 * Weather, night, altitude and nearby fires change warmth. Neglect has real stat penalties
 * and, at the extremes, health loss and frostbite.
 */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRSurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRSurvivalComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="Survival") void Eat(float Amount);
	UFUNCTION(BlueprintCallable, Category="Survival") void Drink(float Amount);
	UFUNCTION(BlueprintCallable, Category="Survival") void AddWarmth(float Amount);
	UFUNCTION(BlueprintCallable, Category="Survival") void AddRest(float Amount);
	UFUNCTION(BlueprintCallable, Category="Survival") void RestoreAll();
	UFUNCTION(BlueprintCallable, Category="Survival") void SetValues(float InFood, float InWater, float InWarmth, float InFatigue);

	UFUNCTION(BlueprintPure, Category="Survival") float GetFood() const { return Food; }
	UFUNCTION(BlueprintPure, Category="Survival") float GetWater() const { return Water; }
	UFUNCTION(BlueprintPure, Category="Survival") float GetWarmth() const { return Warmth; }
	UFUNCTION(BlueprintPure, Category="Survival") float GetFatigue() const { return Fatigue; }
	UFUNCTION(BlueprintPure, Category="Survival") float GetInfection() const { return Infection; }
	UFUNCTION(BlueprintCallable, Category="Survival") void SetInfection(float Value) { Infection = FMath::Clamp(Value, 0.f, 100.f); }
	UFUNCTION(BlueprintCallable, Category="Survival") void TreatInfection(float Amount) { SetInfection(Infection - FMath::Max(0.f, Amount)); }
	UFUNCTION(BlueprintPure, Category="Survival") float GetAmbientTemperature() const { return AmbientTemperature; }
	UFUNCTION(BlueprintPure, Category="Survival") bool IsNearHeat() const { return bNearHeat; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") float FoodPerHour = 3.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") float WaterPerHour = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") float FatiguePerHour = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") float SprintFatigueMultiplier = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Survival") float HeatSourceRadius = 700.f;

protected:
	UPROPERTY(VisibleInstanceOnly, Category="Survival") float Food = 100.f;
	UPROPERTY(VisibleInstanceOnly, Category="Survival") float Water = 100.f;
	UPROPERTY(VisibleInstanceOnly, Category="Survival") float Warmth = 80.f;
	UPROPERTY(VisibleInstanceOnly, Category="Survival") float Fatigue = 0.f;
	UPROPERTY(VisibleInstanceOnly, Category="Survival") float Infection = 0.f;

private:
	void UpdateHeatSources();
	void ApplyEffects(float DeltaTime);

	float AmbientTemperature = 15.f;
	bool bNearHeat = false;
	float HeatCheckTimer = 0.f;
	float EffectTimer = 0.f;
};
