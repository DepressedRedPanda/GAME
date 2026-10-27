#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HRAbility.generated.h"

class AHRCharacterBase;

/**
 * Base class for spells, supernatural powers and transformations. Instant abilities fire once;
 * toggled abilities (Spirit Sight, Beast Form) stay active and drain Focus every second.
 * Subclass in C++ or Blueprint.
 */
UCLASS(Abstract, Blueprintable)
class HOLLOWREACH_API UHRAbility : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability") FName AbilityId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability") FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability", meta=(MultiLine=true)) FText Description;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability") float FocusCost = 15.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability") float Cooldown = 2.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability") float CastTime = 0.35f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability") bool bIsToggle = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability") float FocusPerSecond = 0.f;
	/** While active, no other ability can be used (transformations). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability") bool bBlocksOtherAbilities = false;

	virtual bool CanActivate(const AHRCharacterBase* Owner, FText& OutReason) const;
	virtual void Activate(AHRCharacterBase* Owner);
	virtual void Deactivate(AHRCharacterBase* Owner);
	virtual void TickActive(AHRCharacterBase* Owner, float DeltaTime) {}

	bool IsActive() const { return bActive; }

	UFUNCTION(BlueprintImplementableEvent, Category="Ability", meta=(DisplayName="On Activated"))
	void K2_OnActivated(AHRCharacterBase* Owner);

	UFUNCTION(BlueprintImplementableEvent, Category="Ability", meta=(DisplayName="On Deactivated"))
	void K2_OnDeactivated(AHRCharacterBase* Owner);

	virtual UWorld* GetWorld() const override;

protected:
	bool bActive = false;
};
