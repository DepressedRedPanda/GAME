#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HRAbilityComponent.generated.h"

class UHRAbility;
class AHRCharacterBase;

/** Owns granted abilities, four quick slots, cooldowns and toggled-ability upkeep. */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRAbilityComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	static constexpr int32 NumSlots = 4;

	UFUNCTION(BlueprintCallable, Category="Abilities") UHRAbility* GrantAbility(TSubclassOf<UHRAbility> AbilityClass);
	UFUNCTION(BlueprintCallable, Category="Abilities") UHRAbility* GrantAbilityById(FName AbilityId);
	UFUNCTION(BlueprintPure, Category="Abilities") bool HasAbility(FName AbilityId) const;
	UFUNCTION(BlueprintPure, Category="Abilities") UHRAbility* FindAbility(FName AbilityId) const;
	UFUNCTION(BlueprintPure, Category="Abilities") UHRAbility* GetSlotAbility(int32 Slot) const;

	UFUNCTION(BlueprintCallable, Category="Abilities") bool ActivateSlot(int32 Slot);
	UFUNCTION(BlueprintCallable, Category="Abilities") bool ActivateAbility(FName AbilityId);
	UFUNCTION(BlueprintCallable, Category="Abilities") void DeactivateAll();
	UFUNCTION(BlueprintPure, Category="Abilities") float GetCooldownRemaining(FName AbilityId) const;
	UFUNCTION(BlueprintPure, Category="Abilities") bool IsTransformed() const;

	TArray<FName> GetGrantedIds() const;
	TArray<FName> GetSlotIds() const { return SlotIds; }
	void RestoreSlots(const TArray<FName>& InSlots);

protected:
	UPROPERTY(VisibleInstanceOnly, Category="Abilities") TArray<TObjectPtr<UHRAbility>> Abilities;
	UPROPERTY(VisibleInstanceOnly, Category="Abilities") TArray<FName> SlotIds;

private:
	AHRCharacterBase* GetOwnerCharacter() const;
	TMap<FName, double> CooldownEnds;
};
