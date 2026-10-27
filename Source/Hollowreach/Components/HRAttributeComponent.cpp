#include "Components/HRAttributeComponent.h"

#include "Engine/World.h"

namespace
{
	const FName InjurySource(TEXT("Injuries"));
}

UHRAttributeComponent::UHRAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BaseStats.Add(EHRStat::MaxHealth, 100.f);
	BaseStats.Add(EHRStat::MaxStamina, 100.f);
	BaseStats.Add(EHRStat::MaxPosture, 100.f);
	BaseStats.Add(EHRStat::MaxFocus, 50.f);
	BaseStats.Add(EHRStat::StaminaRegen, 32.f);
	BaseStats.Add(EHRStat::PostureRegen, 14.f);
	BaseStats.Add(EHRStat::FocusRegen, 1.f);
	BaseStats.Add(EHRStat::PhysicalPower, 0.f);
	BaseStats.Add(EHRStat::ArcanePower, 0.f);
	BaseStats.Add(EHRStat::Defense, 10.f);
	BaseStats.Add(EHRStat::MoveSpeed, 1.f);
	BaseStats.Add(EHRStat::CarryCapacity, 60.f);
	BaseStats.Add(EHRStat::Stealth, 0.f);
	BaseStats.Add(EHRStat::ParryWindow, 0.18f);
	BaseStats.Add(EHRStat::CritChance, 0.05f);
	BaseStats.Add(EHRStat::PostureDamage, 1.f);
	BaseStats.Add(EHRStat::LootLuck, 0.f);
}

void UHRAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	MarkStatsDirty();
	RefillAll();
}

void UHRAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsAlive())
	{
		return;
	}

	// Expire timed modifiers (buffs, wards, veils).
	const double Now = GetWorld()->GetTimeSeconds();
	TArray<FName> Expired;
	for (const TPair<FName, FHRModifierSet>& Pair : ModifierSources)
	{
		if (Pair.Value.ExpireTime >= 0.0 && Now >= Pair.Value.ExpireTime)
		{
			Expired.Add(Pair.Key);
		}
	}
	for (const FName& Source : Expired)
	{
		RemoveModifierSource(Source);
	}

	// Stamina
	StaminaDelayRemaining = FMath::Max(0.f, StaminaDelayRemaining - DeltaTime);
	if (StaminaDelayRemaining <= 0.f)
	{
		const float Rate = GetStat(EHRStat::StaminaRegen) * (bGuarding ? 0.5f : 1.f);
		Stamina = FMath::Min(GetMaxStamina(), Stamina + Rate * DeltaTime);
	}

	// Posture recovers faster at high health and while guarding.
	PostureDelayRemaining = FMath::Max(0.f, PostureDelayRemaining - DeltaTime);
	if (PostureDelayRemaining <= 0.f && Posture > 0.f)
	{
		const float HealthFactor = 0.25f + 0.75f * GetHealthPercent();
		const float Rate = GetStat(EHRStat::PostureRegen) * HealthFactor * (bGuarding ? 2.f : 1.f);
		Posture = FMath::Max(0.f, Posture - Rate * DeltaTime);
	}

	Focus = FMath::Min(GetMaxFocus(), Focus + GetStat(EHRStat::FocusRegen) * DeltaTime);
}

float UHRAttributeComponent::GetStat(EHRStat Stat) const
{
	if (bStatsDirty)
	{
		RecalculateStats();
	}
	const int32 Index = static_cast<int32>(Stat);
	return CachedStats.IsValidIndex(Index) ? CachedStats[Index] : 0.f;
}

float UHRAttributeComponent::GetBaseStat(EHRStat Stat) const
{
	const float* Value = BaseStats.Find(Stat);
	return Value ? *Value : 0.f;
}

void UHRAttributeComponent::SetBaseStat(EHRStat Stat, float Value)
{
	BaseStats.Add(Stat, Value);
	MarkStatsDirty();
}

void UHRAttributeComponent::ScaleBaseStat(EHRStat Stat, float Factor)
{
	SetBaseStat(Stat, GetBaseStat(Stat) * Factor);
}

void UHRAttributeComponent::SetModifierSource(FName Source, const TArray<FHRStatModifier>& Modifiers)
{
	FHRModifierSet& Set = ModifierSources.FindOrAdd(Source);
	Set.Modifiers = Modifiers;
	Set.ExpireTime = -1.0;
	MarkStatsDirty();
}

void UHRAttributeComponent::AddTimedModifiers(FName Source, const TArray<FHRStatModifier>& Modifiers, float Duration)
{
	FHRModifierSet& Set = ModifierSources.FindOrAdd(Source);
	Set.Modifiers = Modifiers;
	Set.ExpireTime = GetWorld() ? GetWorld()->GetTimeSeconds() + Duration : -1.0;
	MarkStatsDirty();
}

void UHRAttributeComponent::RemoveModifierSource(FName Source)
{
	if (ModifierSources.Remove(Source) > 0)
	{
		MarkStatsDirty();
	}
}

bool UHRAttributeComponent::HasModifierSource(FName Source) const
{
	return ModifierSources.Contains(Source);
}

float UHRAttributeComponent::GetHealthPercent() const
{
	const float Max = GetMaxHealth();
	return Max > 0.f ? Health / Max : 0.f;
}

float UHRAttributeComponent::ApplyDamage(const FHRDamageInfo& Damage)
{
	if (!IsAlive() || (bInvulnerable && !Damage.bDeathblow))
	{
		return 0.f;
	}

	float Amount = Damage.Amount;
	switch (Damage.Type)
	{
	case EHRDamageType::Physical:
		Amount *= 100.f / (100.f + FMath::Max(0.f, GetStat(EHRStat::Defense)));
		break;
	case EHRDamageType::Fire:
		Amount *= 1.f - FMath::Clamp(GetStat(EHRStat::FireResist), 0.f, 0.9f);
		break;
	case EHRDamageType::Frost:
		Amount *= 1.f - FMath::Clamp(GetStat(EHRStat::FrostResist), 0.f, 0.9f);
		break;
	case EHRDamageType::Blight:
		Amount *= 1.f - FMath::Clamp(GetStat(EHRStat::BlightResist), 0.f, 0.9f);
		break;
	case EHRDamageType::Fall:
		break;
	default:
		Amount *= 100.f / (100.f + 0.5f * FMath::Max(0.f, GetStat(EHRStat::Defense)));
		break;
	}
	Amount = FMath::Max(0.f, Amount * DamageTakenMultiplier);

	Health = FMath::Max(0.f, Health - Amount);

	if (Damage.PostureDamage > 0.f)
	{
		ApplyPostureDamage(Damage.PostureDamage);
	}

	// Lasting injuries make every fight a risk worth weighing.
	if (bCanBeInjured && Health > 0.f)
	{
		const float Fraction = Amount / FMath::Max(1.f, GetMaxHealth());
		const float Roll = FMath::FRand();
		if (Damage.Type == EHRDamageType::Fall && Fraction > 0.15f)
		{
			AddInjury(Fraction > 0.35f ? EHRInjury::BrokenArm : EHRInjury::SprainedAnkle);
		}
		else if (Damage.Type == EHRDamageType::Fire && Fraction > 0.08f && Roll < 0.25f)
		{
			AddInjury(EHRInjury::Burn);
		}
		else if (Damage.Type == EHRDamageType::Frost && Roll < 0.2f)
		{
			AddInjury(EHRInjury::Frostbite);
		}
		else if (Damage.bHeavy && Fraction > 0.18f && Roll < 0.3f)
		{
			AddInjury(FMath::RandBool() ? EHRInjury::DeepWound : EHRInjury::Concussion);
		}
	}

	FHRDamageInfo Applied = Damage;
	Applied.Amount = Amount;
	OnDamaged.Broadcast(this, Applied);
	OnHealthChanged.Broadcast(this, Health, GetMaxHealth());

	if (Health <= 0.f)
	{
		OnHealthDepleted.Broadcast(this, Damage.Source);
	}
	return Amount;
}

void UHRAttributeComponent::ApplyPostureDamage(float Amount)
{
	if (!IsAlive() || Amount <= 0.f)
	{
		return;
	}
	const float Max = GetMaxPosture();
	const bool bWasBroken = Posture >= Max;
	Posture = FMath::Min(Max, Posture + Amount);
	PostureDelayRemaining = PostureRegenDelay;
	if (!bWasBroken && Posture >= Max)
	{
		OnPostureBroken.Broadcast(this);
	}
}

void UHRAttributeComponent::Heal(float Amount)
{
	if (!IsAlive() || Amount <= 0.f)
	{
		return;
	}
	Health = FMath::Min(GetMaxHealth(), Health + Amount);
	OnHealthChanged.Broadcast(this, Health, GetMaxHealth());
}

bool UHRAttributeComponent::ConsumeStamina(float Amount)
{
	if (Stamina <= 0.f)
	{
		return false;
	}
	Stamina -= Amount;
	if (Stamina < 0.f)
	{
		// Overdrawing stamina is punished with a longer recovery delay.
		Stamina = 0.f;
		StaminaDelayRemaining = StaminaRegenDelay * 2.f;
	}
	else
	{
		StaminaDelayRemaining = StaminaRegenDelay;
	}
	return true;
}

void UHRAttributeComponent::RestoreStamina(float Amount)
{
	Stamina = FMath::Min(GetMaxStamina(), Stamina + Amount);
}

bool UHRAttributeComponent::ConsumeFocus(float Amount)
{
	if (Focus < Amount)
	{
		return false;
	}
	Focus -= Amount;
	return true;
}

void UHRAttributeComponent::RestoreFocus(float Amount)
{
	Focus = FMath::Min(GetMaxFocus(), Focus + Amount);
}

void UHRAttributeComponent::ResetPosture(float Fraction)
{
	Posture = GetMaxPosture() * FMath::Clamp(Fraction, 0.f, 1.f);
}

void UHRAttributeComponent::RefillAll()
{
	Health = GetMaxHealth();
	Stamina = GetMaxStamina();
	Focus = GetMaxFocus();
	Posture = 0.f;
	OnHealthChanged.Broadcast(this, Health, GetMaxHealth());
}

void UHRAttributeComponent::SetHealthFraction(float Fraction)
{
	Health = GetMaxHealth() * FMath::Clamp(Fraction, 0.f, 1.f);
	OnHealthChanged.Broadcast(this, Health, GetMaxHealth());
}

void UHRAttributeComponent::Kill(AActor* Killer)
{
	if (!IsAlive())
	{
		return;
	}
	Health = 0.f;
	OnHealthChanged.Broadcast(this, Health, GetMaxHealth());
	OnHealthDepleted.Broadcast(this, Killer);
}

void UHRAttributeComponent::AddInjury(EHRInjury Injury)
{
	if (Injury == EHRInjury::None || Injuries.Contains(Injury))
	{
		return;
	}
	Injuries.Add(Injury);
	RefreshInjuryModifiers();
	OnInjured.Broadcast(this, Injury);
}

void UHRAttributeComponent::CureAllInjuries()
{
	Injuries.Reset();
	RefreshInjuryModifiers();
}

void UHRAttributeComponent::SetInjuries(const TArray<EHRInjury>& InInjuries)
{
	Injuries = InInjuries;
	RefreshInjuryModifiers();
}

FText UHRAttributeComponent::GetInjuryName(EHRInjury Injury)
{
	switch (Injury)
	{
	case EHRInjury::DeepWound: return NSLOCTEXT("HR", "InjuryDeepWound", "Deep Wound (-15% max health)");
	case EHRInjury::BrokenArm: return NSLOCTEXT("HR", "InjuryBrokenArm", "Broken Arm (-30% damage)");
	case EHRInjury::SprainedAnkle: return NSLOCTEXT("HR", "InjurySprain", "Sprained Ankle (-25% speed)");
	case EHRInjury::Concussion: return NSLOCTEXT("HR", "InjuryConcussion", "Concussion (narrow parry window)");
	case EHRInjury::Frostbite: return NSLOCTEXT("HR", "InjuryFrostbite", "Frostbite (slow stamina)");
	case EHRInjury::Burn: return NSLOCTEXT("HR", "InjuryBurn", "Burn (-20% defense)");
	default: return FText::GetEmpty();
	}
}

void UHRAttributeComponent::GetInjuryModifiers(EHRInjury Injury, TArray<FHRStatModifier>& OutModifiers)
{
	switch (Injury)
	{
	case EHRInjury::DeepWound: OutModifiers.Add(FHRStatModifier(EHRStat::MaxHealth, 0.f, 0.85f)); break;
	case EHRInjury::BrokenArm:
		OutModifiers.Add(FHRStatModifier(EHRStat::PhysicalPower, -6.f, 0.7f));
		OutModifiers.Add(FHRStatModifier(EHRStat::PostureDamage, 0.f, 0.8f));
		break;
	case EHRInjury::SprainedAnkle: OutModifiers.Add(FHRStatModifier(EHRStat::MoveSpeed, 0.f, 0.75f)); break;
	case EHRInjury::Concussion:
		OutModifiers.Add(FHRStatModifier(EHRStat::ParryWindow, 0.f, 0.6f));
		OutModifiers.Add(FHRStatModifier(EHRStat::MaxFocus, 0.f, 0.7f));
		break;
	case EHRInjury::Frostbite: OutModifiers.Add(FHRStatModifier(EHRStat::StaminaRegen, 0.f, 0.7f)); break;
	case EHRInjury::Burn: OutModifiers.Add(FHRStatModifier(EHRStat::Defense, 0.f, 0.8f)); break;
	default: break;
	}
}

void UHRAttributeComponent::RefreshInjuryModifiers()
{
	TArray<FHRStatModifier> Modifiers;
	for (EHRInjury Injury : Injuries)
	{
		GetInjuryModifiers(Injury, Modifiers);
	}
	if (Modifiers.Num() > 0)
	{
		SetModifierSource(InjurySource, Modifiers);
	}
	else
	{
		RemoveModifierSource(InjurySource);
	}
}

void UHRAttributeComponent::MarkStatsDirty()
{
	bStatsDirty = true;
	ClampVitals();
}

void UHRAttributeComponent::RecalculateStats() const
{
	const int32 NumStats = static_cast<int32>(EHRStat::MAX);
	TArray<float> Additive;
	Additive.Init(0.f, NumStats);
	TArray<float> Multiplier;
	Multiplier.Init(1.f, NumStats);

	for (const TPair<FName, FHRModifierSet>& Pair : ModifierSources)
	{
		for (const FHRStatModifier& Modifier : Pair.Value.Modifiers)
		{
			const int32 Index = static_cast<int32>(Modifier.Stat);
			if (Index >= 0 && Index < NumStats)
			{
				Additive[Index] += Modifier.Additive;
				Multiplier[Index] *= Modifier.Multiplier;
			}
		}
	}

	CachedStats.SetNumZeroed(NumStats);
	for (int32 Index = 0; Index < NumStats; ++Index)
	{
		const float* Base = BaseStats.Find(static_cast<EHRStat>(Index));
		CachedStats[Index] = ((Base ? *Base : 0.f) + Additive[Index]) * Multiplier[Index];
	}
	bStatsDirty = false;
}

void UHRAttributeComponent::ClampVitals()
{
	Health = FMath::Min(Health, GetMaxHealth());
	Stamina = FMath::Min(Stamina, GetMaxStamina());
	Focus = FMath::Min(Focus, GetMaxFocus());
	Posture = FMath::Min(Posture, GetMaxPosture());
}
