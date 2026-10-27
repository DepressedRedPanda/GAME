#include "AshWorldState.h"
#include "Net/UnrealNetwork.h"
AAshWorldState::AAshWorldState() { PrimaryActorTick.bCanEverTick = true; bReplicates = true; }
void AAshWorldState::Tick(float DT)
{
    Super::Tick(DT);
    if (!HasAuthority()) return;
    TimeOfDay = FMath::Fmod(TimeOfDay + DT / 45.f, 24.f);
    WeatherClock += DT;
    if (WeatherClock >= 120.f)
    {
        WeatherClock = 0.f;
        Weather = static_cast<EAshWeather>((static_cast<uint8>(Weather)+1)%3);
    }
}
void AAshWorldState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(AAshWorldState, TimeOfDay);
    DOREPLIFETIME(AAshWorldState, Weather);
    DOREPLIFETIME(AAshWorldState, Seed);
    DOREPLIFETIME(AAshWorldState, DiscoveryCount);
    DOREPLIFETIME(AAshWorldState, bGateOpen);
    DOREPLIFETIME(AAshWorldState, RuneProgress);
    DOREPLIFETIME(AAshWorldState, bRuneSolved);
    DOREPLIFETIME(AAshWorldState, bBossDefeated);
    DOREPLIFETIME(AAshWorldState, Ending);
    DOREPLIFETIME(AAshWorldState, BuiltCount);
}
