#pragma once

#include "CoreMinimal.h"
#include "HRTypes.generated.h"

/** Every derived stat in the game. Final value = (Base + sum(Additive)) * product(Multiplier). */
UENUM(BlueprintType)
enum class EHRStat : uint8
{
	MaxHealth,
	MaxStamina,
	MaxPosture,
	MaxFocus,
	StaminaRegen,
	PostureRegen,
	FocusRegen,
	PhysicalPower,
	ArcanePower,
	Defense,
	FireResist,
	FrostResist,
	BlightResist,
	Insulation,
	MoveSpeed,
	CarryCapacity,
	Stealth,
	ParryWindow,
	CritChance,
	PostureDamage,
	LootLuck,
	MAX UMETA(Hidden)
};

/** Level-up attributes (Souls-style). Each point feeds several stats. */
UENUM(BlueprintType)
enum class EHRStartingClass : uint8 { Unchosen, Vanguard, Seer, Shade };

UENUM(BlueprintType)
enum class EHRAttribute : uint8
{
	Vigor,
	Endurance,
	Strength,
	Dexterity,
	Arcane,
	Mind,
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EHRDamageType : uint8
{
	Physical,
	Fire,
	Frost,
	Lightning,
	Arcane,
	Blight,
	Fall
};

UENUM(BlueprintType)
enum class EHRHitResponse : uint8
{
	Ignored,
	Hit,
	Blocked,
	GuardBroken,
	Deflected,
	Evaded,
	Deathblow,
	Killed
};

UENUM(BlueprintType)
enum class EHRCombatState : uint8
{
	Idle,
	Attacking,
	Blocking,
	Dodging,
	Staggered,
	PostureBroken,
	Busy,
	Downed,
	Dead
};

UENUM(BlueprintType)
enum class EHRAwareness : uint8
{
	Unaware,
	Suspicious,
	Searching,
	Alerted
};

UENUM(BlueprintType)
enum class EHRInjury : uint8
{
	None,
	DeepWound,
	BrokenArm,
	SprainedAnkle,
	Concussion,
	Frostbite,
	Burn
};

UENUM(BlueprintType)
enum class EHRWeather : uint8
{
	Clear,
	Overcast,
	Rain,
	Storm,
	Fog,
	Snow,
	Ashfall,
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EHRTraversalAbility : uint8
{
	Mantle,
	DoubleJump,
	WallRun,
	Grapple,
	Glide
};

UENUM(BlueprintType)
enum class EHRAttitude : uint8
{
	Hostile,
	Neutral,
	Friendly
};

UENUM(BlueprintType)
enum class EHRCraftStation : uint8
{
	None,
	Campfire,
	Forge
};

UENUM(BlueprintType)
enum class EHRUIMode : uint8
{
	Gameplay,
	Rest,
	Dialogue,
	Shop,
	Journal,
	Dead,
	Ending
};

USTRUCT(BlueprintType)
struct FHRStatModifier
{
	GENERATED_BODY()

	FHRStatModifier() {}
	FHRStatModifier(EHRStat InStat, float InAdditive, float InMultiplier = 1.f)
		: Stat(InStat), Additive(InAdditive), Multiplier(InMultiplier) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	EHRStat Stat = EHRStat::MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Additive = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Multiplier = 1.f;
};

USTRUCT(BlueprintType)
struct FHRDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	float Amount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	float PostureDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	EHRDamageType Type = EHRDamageType::Physical;

	/** Perilous attack: cannot be blocked or deflected, only evaded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bUnblockable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bHeavy = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bCritical = false;

	/** Execution: stealth takedown or a strike against a broken posture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bDeathblow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	FVector HitLocation = FVector::ZeroVector;
};
