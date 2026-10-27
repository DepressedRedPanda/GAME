#include "Systems/HRTimeWeatherSubsystem.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "HRTimeWeather"

bool UHRTimeWeatherSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UHRTimeWeatherSubsystem::RegisterEnvironment(ADirectionalLight* InSun, AExponentialHeightFog* InFog, ASkyLight* InSkyLight)
{
	Sun = InSun;
	Fog = InFog;
	SkyLight = InSkyLight;
	UpdateEnvironment(1.f);
}

void UHRTimeWeatherSubsystem::Tick(float DeltaTime)
{
	const float Hours = RealSecondsToGameHours(DeltaTime);
	AdvanceHours(Hours);

	HoursUntilWeatherChange -= Hours;
	if (HoursUntilWeatherChange <= 0.f)
	{
		RollNextWeather();
	}
	UpdateEnvironment(DeltaTime);
}

void UHRTimeWeatherSubsystem::RestoreClock(int32 InDay, float Hours, float WeatherHours)
{
	Day = FMath::Max(1, InDay);
	TimeOfDay = FMath::Clamp(Hours, 0.f, 23.999f);
	HoursUntilWeatherChange = FMath::Max(0.f, WeatherHours);
}

void UHRTimeWeatherSubsystem::AdvanceHours(float Hours)
{
	const int32 OldHour = FMath::FloorToInt(TimeOfDay);
	const bool bWasNight = IsNight();

	TimeOfDay += Hours;
	while (TimeOfDay >= 24.f)
	{
		TimeOfDay -= 24.f;
		++Day;
	}

	if (FMath::FloorToInt(TimeOfDay) != OldHour)
	{
		OnHourChanged.Broadcast(FMath::FloorToInt(TimeOfDay));
	}
	if (!bWasNight && IsNight())
	{
		OnNightfall.Broadcast();
	}
	else if (bWasNight && !IsNight())
	{
		OnDawn.Broadcast();
	}
}

void UHRTimeWeatherSubsystem::SetTimeOfDay(float Hours)
{
	const float Target = FMath::Fmod(FMath::Max(0.f, Hours), 24.f);
	float Delta = Target - TimeOfDay;
	if (Delta < 0.f)
	{
		Delta += 24.f;
	}
	AdvanceHours(Delta);
	UpdateEnvironment(1.f);
}

void UHRTimeWeatherSubsystem::SetWeather(EHRWeather NewWeather)
{
	if (Weather == NewWeather)
	{
		return;
	}
	Weather = NewWeather;
	HoursUntilWeatherChange = FMath::FRandRange(MinWeatherHours, MaxWeatherHours);
	OnWeatherChanged.Broadcast(Weather);
}

void UHRTimeWeatherSubsystem::RollNextWeather()
{
	// Markov chain: weather tends to evolve gradually rather than jump around.
	struct FTransition
	{
		EHRWeather To;
		float Weight;
	};
	TArray<FTransition> Options;
	switch (Weather)
	{
	case EHRWeather::Clear: Options = { { EHRWeather::Clear, 4.f }, { EHRWeather::Overcast, 3.f }, { EHRWeather::Fog, 1.f } }; break;
	case EHRWeather::Overcast: Options = { { EHRWeather::Clear, 2.f }, { EHRWeather::Rain, 3.f }, { EHRWeather::Snow, 1.f }, { EHRWeather::Ashfall, 0.5f } }; break;
	case EHRWeather::Rain: Options = { { EHRWeather::Overcast, 3.f }, { EHRWeather::Storm, 1.5f }, { EHRWeather::Fog, 1.f } }; break;
	case EHRWeather::Storm: Options = { { EHRWeather::Rain, 3.f }, { EHRWeather::Overcast, 1.f } }; break;
	case EHRWeather::Fog: Options = { { EHRWeather::Clear, 2.f }, { EHRWeather::Overcast, 2.f } }; break;
	case EHRWeather::Snow: Options = { { EHRWeather::Overcast, 3.f }, { EHRWeather::Snow, 1.f } }; break;
	case EHRWeather::Ashfall: Options = { { EHRWeather::Overcast, 2.f }, { EHRWeather::Fog, 1.f } }; break;
	default: Options = { { EHRWeather::Clear, 1.f } }; break;
	}

	float Total = 0.f;
	for (const FTransition& Option : Options)
	{
		Total += Option.Weight;
	}
	float Pick = FMath::FRandRange(0.f, Total);
	for (const FTransition& Option : Options)
	{
		Pick -= Option.Weight;
		if (Pick <= 0.f)
		{
			SetWeather(Option.To);
			break;
		}
	}
	HoursUntilWeatherChange = FMath::FRandRange(MinWeatherHours, MaxWeatherHours);
}

float UHRTimeWeatherSubsystem::GetLightLevel() const
{
	// Sun elevation curve: 0 at 6:00 and 20:00, 1 at 13:00.
	float Daylight = 0.f;
	if (!IsNight())
	{
		Daylight = FMath::Sin(PI * (TimeOfDay - 6.f) / 14.f);
	}
	const float MoonLight = IsBloodMoon() ? 0.3f : 0.12f;
	float Light = FMath::Max(Daylight, MoonLight);

	switch (Weather)
	{
	case EHRWeather::Overcast: Light *= 0.8f; break;
	case EHRWeather::Rain: Light *= 0.7f; break;
	case EHRWeather::Storm: Light *= 0.5f; break;
	case EHRWeather::Fog: Light *= 0.75f; break;
	case EHRWeather::Ashfall: Light *= 0.6f; break;
	default: break;
	}
	return FMath::Clamp(Light, 0.f, 1.f);
}

float UHRTimeWeatherSubsystem::GetSightMultiplier() const
{
	float Multiplier = IsNight() ? 0.6f : 1.f;
	switch (Weather)
	{
	case EHRWeather::Fog: Multiplier *= 0.45f; break;
	case EHRWeather::Storm: Multiplier *= 0.6f; break;
	case EHRWeather::Rain:
	case EHRWeather::Snow: Multiplier *= 0.8f; break;
	case EHRWeather::Ashfall: Multiplier *= 0.65f; break;
	default: break;
	}
	return Multiplier;
}

float UHRTimeWeatherSubsystem::GetNoiseMultiplier() const
{
	switch (Weather)
	{
	case EHRWeather::Rain: return 0.7f;
	case EHRWeather::Storm: return 0.45f;
	case EHRWeather::Snow: return 0.85f;
	default: return 1.f;
	}
}

float UHRTimeWeatherSubsystem::GetTemperatureAt(const FVector& Location) const
{
	float Temperature = 16.f;
	Temperature -= IsNight() ? 9.f : 0.f;
	switch (Weather)
	{
	case EHRWeather::Rain: Temperature -= 4.f; break;
	case EHRWeather::Storm: Temperature -= 7.f; break;
	case EHRWeather::Snow: Temperature -= 15.f; break;
	case EHRWeather::Fog: Temperature -= 2.f; break;
	case EHRWeather::Ashfall: Temperature += 4.f; break;
	default: break;
	}
	// Roughly -6.5C per 1000m; the greybox mountain compresses that to make altitude matter.
	Temperature -= FMath::Max(0.f, Location.Z) / 100.f * 0.6f;
	return Temperature;
}

void UHRTimeWeatherSubsystem::UpdateEnvironment(float DeltaTime)
{
	if (ADirectionalLight* SunActor = Sun.Get())
	{
		// 6:00 sunrise on the horizon, 12:00 overhead, 18:00 sunset.
		const float Pitch = -((TimeOfDay - 6.f) / 24.f) * 360.f;
		SunActor->SetActorRotation(FRotator(Pitch, 35.f, 0.f));
		if (UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(SunActor->GetLightComponent()))
		{
			const float Daylight = IsNight() ? 0.f : FMath::Sin(PI * (TimeOfDay - 6.f) / 14.f);
			Light->SetIntensity(FMath::Max(0.05f, Daylight) * 8.f * (Weather == EHRWeather::Storm ? 0.4f : 1.f));
			const FLinearColor Color = IsBloodMoon() ? FLinearColor(1.f, 0.35f, 0.3f) : FLinearColor::LerpUsingHSV(FLinearColor(1.f, 0.6f, 0.4f), FLinearColor::White, FMath::Clamp(Daylight * 1.5f, 0.f, 1.f));
			Light->SetLightColor(Color);
		}
	}

	if (AExponentialHeightFog* FogActor = Fog.Get())
	{
		float Target = 0.015f;
		switch (Weather)
		{
		case EHRWeather::Fog: Target = 0.12f; break;
		case EHRWeather::Rain: Target = 0.04f; break;
		case EHRWeather::Storm: Target = 0.06f; break;
		case EHRWeather::Snow: Target = 0.05f; break;
		case EHRWeather::Ashfall: Target = 0.08f; break;
		default: break;
		}
		FogDensity = FMath::FInterpTo(FogDensity, Target, DeltaTime, 0.3f);
		if (UExponentialHeightFogComponent* FogComponent = FogActor->GetComponent())
		{
			FogComponent->SetFogDensity(FogDensity);
		}
	}
}

FText UHRTimeWeatherSubsystem::GetTimeText() const
{
	const int32 Hour = FMath::FloorToInt(TimeOfDay);
	const int32 Minute = FMath::FloorToInt((TimeOfDay - Hour) * 60.f);
	return FText::FromString(FString::Printf(TEXT("Day %d  %02d:%02d  %s%s"), Day, Hour, Minute,
		*UEnum::GetDisplayValueAsText(Weather).ToString(), IsBloodMoon() ? TEXT("  [BLOOD MOON]") : TEXT("")));
}

#undef LOCTEXT_NAMESPACE
