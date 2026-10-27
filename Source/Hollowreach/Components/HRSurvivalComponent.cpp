#include "Components/HRSurvivalComponent.h"

#include "Components/HRAttributeComponent.h"
#include "Core/HRStatics.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Systems/HRTimeWeatherSubsystem.h"

#define LOCTEXT_NAMESPACE "HRSurvival"

namespace
{
	const FName SurvivalSource(TEXT("Survival"));
	const FName HeatSourceTag(TEXT("HeatSource"));
}

UHRSurvivalComponent::UHRSurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHRSurvivalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bEnabled)
	{
		return;
	}

	const UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>();
	const float GameHours = TimeWeather ? TimeWeather->RealSecondsToGameHours(DeltaTime) : DeltaTime / 60.f;

	HeatCheckTimer -= DeltaTime;
	if (HeatCheckTimer <= 0.f)
	{
		HeatCheckTimer = 1.f;
		UpdateHeatSources();
	}

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const bool bExerting = Character && Character->GetVelocity().Size2D() > 500.f;

	Food = FMath::Clamp(Food - FoodPerHour * GameHours, 0.f, 100.f);
	if (TimeWeather && TimeWeather->GetWeather() == EHRWeather::Ashfall) SetInfection(Infection + GameHours * 4.f);
	Water = FMath::Clamp(Water - WaterPerHour * GameHours * (bExerting ? 1.5f : 1.f), 0.f, 100.f);
	Fatigue = FMath::Clamp(Fatigue + FatiguePerHour * GameHours * (bExerting ? SprintFatigueMultiplier : 1.f), 0.f, 100.f);

	// Warmth drifts toward a target set by the ambient temperature and insulation.
	const UHRAttributeComponent* Attributes = GetOwner()->FindComponentByClass<UHRAttributeComponent>();
	const float Insulation = Attributes ? Attributes->GetStat(EHRStat::Insulation) : 0.f;
	AmbientTemperature = TimeWeather ? TimeWeather->GetTemperatureAt(GetOwner()->GetActorLocation()) : 15.f;
	const float EffectiveTemperature = AmbientTemperature + Insulation + (bNearHeat ? 25.f : 0.f);
	const float TargetWarmth = FMath::Clamp(50.f + EffectiveTemperature * 2.5f, 0.f, 100.f);
	Warmth = FMath::FInterpConstantTo(Warmth, TargetWarmth, GameHours, 30.f);

	EffectTimer -= DeltaTime;
	if (EffectTimer <= 0.f)
	{
		EffectTimer = 1.f;
		ApplyEffects(1.f);
	}
}

void UHRSurvivalComponent::UpdateHeatSources()
{
	bNearHeat = false;
	const FVector Location = GetOwner()->GetActorLocation();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(HeatSourceTag) && FVector::DistSquared(It->GetActorLocation(), Location) < FMath::Square(HeatSourceRadius))
		{
			bNearHeat = true;
			return;
		}
	}
}

void UHRSurvivalComponent::ApplyEffects(float Seconds)
{
	UHRAttributeComponent* Attributes = GetOwner()->FindComponentByClass<UHRAttributeComponent>();
	if (!Attributes || !Attributes->IsAlive())
	{
		return;
	}

	TArray<FHRStatModifier> Modifiers;
	if (Food < 25.f) { Modifiers.Add(FHRStatModifier(EHRStat::StaminaRegen, 0.f, 0.7f)); }
	if (Water < 25.f) { Modifiers.Add(FHRStatModifier(EHRStat::MaxStamina, 0.f, 0.8f)); }
	if (Warmth < 25.f) { Modifiers.Add(FHRStatModifier(EHRStat::MoveSpeed, 0.f, 0.85f)); }
	if (Fatigue > 75.f)
	{
		Modifiers.Add(FHRStatModifier(EHRStat::PostureRegen, 0.f, 0.6f));
		Modifiers.Add(FHRStatModifier(EHRStat::FocusRegen, 0.f, 0.5f));
	}
	if (Food > 80.f && Water > 80.f) { Modifiers.Add(FHRStatModifier(EHRStat::StaminaRegen, 0.f, 1.1f)); }
	if (Infection >= 50.f) Modifiers.Add(FHRStatModifier(EHRStat::StaminaRegen, 0.f, 0.7f));
	Attributes->SetModifierSource(SurvivalSource, Modifiers);

	// Starvation, dehydration and freezing hurt.
	float Drain = 0.f;
	if (Food <= 0.f) { Drain += 0.5f; }
	if (Water <= 0.f) { Drain += 0.8f; }
	if (Infection >= 90.f) Drain += 0.25f;
	if (Warmth <= 5.f)
	{
		Drain += 1.f;
		if (FMath::FRand() < 0.02f)
		{
			Attributes->AddInjury(EHRInjury::Frostbite);
		}
	}
	if (Drain > 0.f)
	{
		FHRDamageInfo Damage;
		Damage.Amount = Drain * Seconds;
		Damage.Type = EHRDamageType::Fall; // Ignores armor.
		Attributes->ApplyDamage(Damage);
	}
}

void UHRSurvivalComponent::Eat(float Amount)
{
	Food = FMath::Clamp(Food + Amount, 0.f, 100.f);
}

void UHRSurvivalComponent::Drink(float Amount)
{
	Water = FMath::Clamp(Water + Amount, 0.f, 100.f);
}

void UHRSurvivalComponent::AddWarmth(float Amount)
{
	Warmth = FMath::Clamp(Warmth + Amount, 0.f, 100.f);
}

void UHRSurvivalComponent::AddRest(float Amount)
{
	Fatigue = FMath::Clamp(Fatigue - Amount, 0.f, 100.f);
}

void UHRSurvivalComponent::RestoreAll()
{
	Fatigue = 0.f;
	TreatInfection(30.f);
	Warmth = FMath::Max(Warmth, 75.f);
	Food = FMath::Max(Food, 40.f);
	Water = FMath::Max(Water, 40.f);
}

void UHRSurvivalComponent::SetValues(float InFood, float InWater, float InWarmth, float InFatigue)
{
	Food = InFood;
	Water = InWater;
	Warmth = InWarmth;
	Fatigue = InFatigue;
}

#undef LOCTEXT_NAMESPACE
