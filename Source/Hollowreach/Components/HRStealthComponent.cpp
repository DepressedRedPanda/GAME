#include "Components/HRStealthComponent.h"

#include "Components/HRAttributeComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Systems/HRTimeWeatherSubsystem.h"
#include "World/HRStealthZone.h"

UHRStealthComponent::UHRStealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UHRStealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateVisibility();

	// Footsteps.
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Character->GetCharacterMovement() || !Character->GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}
	const float Speed = Character->GetVelocity().Size2D();
	if (Speed < 50.f)
	{
		return;
	}
	FootstepTimer -= DeltaTime;
	if (FootstepTimer > 0.f)
	{
		return;
	}
	FootstepTimer = FootstepInterval;

	float Loudness = Character->bIsCrouched ? 0.12f : (Speed > 600.f ? 0.9f : 0.4f);
	if (bInStealthZone)
	{
		Loudness *= 0.7f;
	}
	MakeNoise(Loudness);
}

void UHRStealthComponent::UpdateVisibility()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	TArray<AActor*> Zones;
	Character->GetOverlappingActors(Zones, AHRStealthZone::StaticClass());
	bInStealthZone = Zones.Num() > 0;

	const UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>();
	const float Light = TimeWeather ? TimeWeather->GetLightLevel() : 1.f;

	float Value = 0.35f + 0.65f * Light;
	if (Character->bIsCrouched)
	{
		Value *= 0.55f;
	}
	if (bInStealthZone)
	{
		Value *= Character->bIsCrouched ? 0.25f : 0.6f;
	}
	const float Speed = Character->GetVelocity().Size2D();
	Value *= Speed > 600.f ? 1.25f : (Speed > 50.f ? 1.f : 0.85f);

	if (const UHRAttributeComponent* Attributes = Character->FindComponentByClass<UHRAttributeComponent>())
	{
		Value *= FMath::Clamp(1.f - Attributes->GetStat(EHRStat::Stealth), 0.05f, 2.f);
	}
	Visibility = FMath::Clamp(Value, 0.02f, 1.f);
}

void UHRStealthComponent::MakeNoise(float Loudness)
{
	const UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>();
	const float Mask = TimeWeather ? TimeWeather->GetNoiseMultiplier() : 1.f;
	const UHRAttributeComponent* Attributes = GetOwner()->FindComponentByClass<UHRAttributeComponent>();
	const float StealthFactor = Attributes ? FMath::Clamp(1.f - Attributes->GetStat(EHRStat::Stealth) * 0.5f, 0.2f, 2.f) : 1.f;
	const float Final = FMath::Clamp(Loudness * Mask * StealthFactor, 0.f, 1.f);
	if (Final > 0.01f)
	{
		UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetOwner()->GetActorLocation(), Final, GetOwner(), MaxNoiseRange * Final);
	}
}
