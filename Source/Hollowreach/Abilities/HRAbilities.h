#pragma once

#include "CoreMinimal.h"
#include "Abilities/HRAbility.h"
#include "HRAbilities.generated.h"

/** Hurls a bolt of fire. Lights braziers and detonates barrels. */
UCLASS()
class HOLLOWREACH_API UHRAbility_Firebolt : public UHRAbility
{
	GENERATED_BODY()
public:
	UHRAbility_Firebolt();
	virtual void Activate(AHRCharacterBase* Owner) override;
};

/** Telekinetic shove: launches props, staggers enemies, slides puzzle blocks. */
UCLASS()
class HOLLOWREACH_API UHRAbility_KineticPush : public UHRAbility
{
	GENERATED_BODY()
public:
	UHRAbility_KineticPush();
	virtual void Activate(AHRCharacterBase* Owner) override;

	UPROPERTY(EditDefaultsOnly, Category="Ability") float Range = 700.f;
	UPROPERTY(EditDefaultsOnly, Category="Ability") float Force = 1400.f;
};

/** Short-range teleport through enemies (exploration + combat). */
UCLASS()
class HOLLOWREACH_API UHRAbility_Blink : public UHRAbility
{
	GENERATED_BODY()
public:
	UHRAbility_Blink();
	virtual void Activate(AHRCharacterBase* Owner) override;

	UPROPERTY(EditDefaultsOnly, Category="Ability") float Distance = 650.f;
};

/** Temporary arcane armor. */
UCLASS()
class HOLLOWREACH_API UHRAbility_Ward : public UHRAbility
{
	GENERATED_BODY()
public:
	UHRAbility_Ward();
	virtual void Activate(AHRCharacterBase* Owner) override;
};

/** Become nearly invisible for a few seconds. */
UCLASS()
class HOLLOWREACH_API UHRAbility_ShadowVeil : public UHRAbility
{
	GENERATED_BODY()
public:
	UHRAbility_ShadowVeil();
	virtual void Activate(AHRCharacterBase* Owner) override;
};

/** Toggle: reveals hidden collectibles, clues and enemies through walls (detective senses). */
UCLASS()
class HOLLOWREACH_API UHRAbility_SpiritSight : public UHRAbility
{
	GENERATED_BODY()
public:
	UHRAbility_SpiritSight();
	virtual void Activate(AHRCharacterBase* Owner) override;
	virtual void Deactivate(AHRCharacterBase* Owner) override;
};

/** Toggle transformation: a hulking beast with more power and poise but no items or stealth. */
UCLASS()
class HOLLOWREACH_API UHRAbility_BeastForm : public UHRAbility
{
	GENERATED_BODY()
public:
	UHRAbility_BeastForm();
	virtual void Activate(AHRCharacterBase* Owner) override;
	virtual void Deactivate(AHRCharacterBase* Owner) override;
};
