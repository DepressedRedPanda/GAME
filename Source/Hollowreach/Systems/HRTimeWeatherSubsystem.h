#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/HRTypes.h"
#include "HRTimeWeatherSubsystem.generated.h"

class ADirectionalLight;
class AExponentialHeightFog;
class ASkyLight;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHRHourSignature, int32, Hour);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHRWeatherSignature, EHRWeather, Weather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHRTimeEventSignature);

/**
 * Day-night cycle and Markov-chain weather. Drives the sun, fog and sky light, and feeds
 * gameplay: light level (stealth), sight range (AI), noise masking (rain), temperature
 * (survival), and the blood moon every seventh night (hordes).
 */
UCLASS()
class HOLLOWREACH_API UHRTimeWeatherSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UHRTimeWeatherSubsystem, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	void RegisterEnvironment(ADirectionalLight* InSun, AExponentialHeightFog* InFog, ASkyLight* InSkyLight);

	UFUNCTION(BlueprintPure, Category="Time") float GetTimeOfDay() const { return TimeOfDay; }
	UFUNCTION(BlueprintPure, Category="Time") int32 GetDay() const { return Day; }
	UFUNCTION(BlueprintPure, Category="Time") bool IsNight() const { return TimeOfDay < 6.f || TimeOfDay >= 20.f; }
	UFUNCTION(BlueprintPure, Category="Time") bool IsBloodMoon() const { return IsNight() && Day % 7 == 0; }
	UFUNCTION(BlueprintCallable, Category="Time") void SetTimeOfDay(float Hours);
	void RestoreClock(int32 InDay, float Hours, float WeatherHours);
	float GetWeatherHoursRemaining() const { return HoursUntilWeatherChange; }
	UFUNCTION(BlueprintCallable, Category="Time") void AdvanceHours(float Hours);
	UFUNCTION(BlueprintPure, Category="Time") float GetTotalHours() const { return Day * 24.f + TimeOfDay; }
	float RealSecondsToGameHours(float Seconds) const { return Seconds * 24.f / FMath::Max(1.f, DayLengthMinutes * 60.f); }

	UFUNCTION(BlueprintPure, Category="Weather") EHRWeather GetWeather() const { return Weather; }
	UFUNCTION(BlueprintCallable, Category="Weather") void SetWeather(EHRWeather NewWeather);

	/** 0 = pitch dark, 1 = bright noon. */
	UFUNCTION(BlueprintPure, Category="Environment") float GetLightLevel() const;
	/** Multiplier on how far AI can see. */
	UFUNCTION(BlueprintPure, Category="Environment") float GetSightMultiplier() const;
	/** Multiplier on how far noises carry (rain and storms mask footsteps). */
	UFUNCTION(BlueprintPure, Category="Environment") float GetNoiseMultiplier() const;
	/** Degrees C at a location: weather, night and altitude all matter. */
	UFUNCTION(BlueprintPure, Category="Environment") float GetTemperatureAt(const FVector& Location) const;

	FText GetTimeText() const;

	UPROPERTY(BlueprintAssignable, Category="Time") FHRHourSignature OnHourChanged;
	UPROPERTY(BlueprintAssignable, Category="Time") FHRTimeEventSignature OnNightfall;
	UPROPERTY(BlueprintAssignable, Category="Time") FHRTimeEventSignature OnDawn;
	UPROPERTY(BlueprintAssignable, Category="Weather") FHRWeatherSignature OnWeatherChanged;

	/** Real-time minutes per in-game day. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Time") float DayLengthMinutes = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") float MinWeatherHours = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather") float MaxWeatherHours = 6.f;

private:
	void UpdateEnvironment(float DeltaTime);
	void RollNextWeather();

	float TimeOfDay = 8.f;
	int32 Day = 1;
	EHRWeather Weather = EHRWeather::Clear;
	float HoursUntilWeatherChange = 4.f;
	float FogDensity = 0.02f;

	TWeakObjectPtr<ADirectionalLight> Sun;
	TWeakObjectPtr<AExponentialHeightFog> Fog;
	TWeakObjectPtr<ASkyLight> SkyLight;
};
