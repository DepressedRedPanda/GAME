#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HRStealthComponent.generated.h"

/**
 * How visible and how loud the player is. AI awareness gain is scaled by Visibility; footsteps
 * and combat emit hearing stimuli scaled by Loudness. Darkness, crouching, tall grass, fog/rain,
 * gear and Shadow Veil all reduce visibility.
 */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRStealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRStealthComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="Stealth") float GetVisibility() const { return Visibility; }
	UFUNCTION(BlueprintPure, Category="Stealth") bool IsInCover() const { return bInStealthZone; }

	/** Emits a noise the AI can hear. Loudness 0..1. */
	UFUNCTION(BlueprintCallable, Category="Stealth") void MakeNoise(float Loudness);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth") float FootstepInterval = 0.45f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stealth") float MaxNoiseRange = 1800.f;

private:
	void UpdateVisibility();

	float Visibility = 1.f;
	bool bInStealthZone = false;
	float FootstepTimer = 0.f;
};
