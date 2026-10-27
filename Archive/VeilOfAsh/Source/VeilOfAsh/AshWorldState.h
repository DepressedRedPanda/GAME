#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AshTypes.h"
#include "AshWorldState.generated.h"

UCLASS()
class VEILOFASH_API AAshWorldState : public AGameStateBase
{
    GENERATED_BODY()
public:
    AAshWorldState();
    virtual void Tick(float DT) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(Replicated, BlueprintReadOnly) float TimeOfDay = 8.f;
    UPROPERTY(Replicated, BlueprintReadOnly) EAshWeather Weather = EAshWeather::Clear;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Seed = 1729;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 DiscoveryCount = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bGateOpen = false;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 RuneProgress = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bRuneSolved = false;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bBossDefeated = false;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Ending = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 BuiltCount = 0;
private:
    float WeatherClock = 0.f;
};
