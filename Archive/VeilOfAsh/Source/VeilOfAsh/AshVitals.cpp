#include "AshVitals.h"
#include "AshHero.h"
#include "AshProgression.h"
#include "AshWorldState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UAshVitals::UAshVitals()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}
void UAshVitals::BeginPlay() { Super::BeginPlay(); }
void UAshVitals::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(UAshVitals, Health);
    DOREPLIFETIME(UAshVitals, MaxHealth);
    DOREPLIFETIME(UAshVitals, Stamina);
    DOREPLIFETIME(UAshVitals, Focus);
    DOREPLIFETIME(UAshVitals, Hunger);
    DOREPLIFETIME(UAshVitals, Thirst);
    DOREPLIFETIME(UAshVitals, Infection);
    DOREPLIFETIME(UAshVitals, Injury);
    DOREPLIFETIME(UAshVitals, bDead);
}
void UAshVitals::TickComponent(float DT, ELevelTick TickType, FActorComponentTickFunction* TickFn)
{
    Super::TickComponent(DT, TickType, TickFn);
    if (!GetOwner() || !GetOwner()->HasAuthority() || bDead) return;
    const AAshHero* Hero=Cast<AAshHero>(GetOwner());
    const int32 Guile=Hero ? Hero->Progression->Guile : 0;
    Stamina = FMath::Min(100.f, Stamina + (19.f+Guile*2.f) * DT);
    Focus = FMath::Min(100.f, Focus + 2.5f * DT);
    Hunger = FMath::Max(0.f, Hunger - 0.065f * DT);
    Thirst = FMath::Max(0.f, Thirst - 0.115f * DT);
    if (AAshWorldState* World=GetWorld()->GetGameState<AAshWorldState>())
        if (World->Weather==EAshWeather::Ashstorm) Infection=FMath::Min(100.f,Infection+0.09f*DT);
    if (Hunger <= 0.f || Thirst <= 0.f || Infection > 90.f) Health -= 1.5f * DT;
    if (Health <= 0.f) { Health = 0.f; bDead = true; }
}
bool UAshVitals::SpendStamina(float N)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || bDead || Stamina < N) return false;
    Stamina -= N;
    return true;
}
bool UAshVitals::SpendFocus(float N)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || bDead || Focus < N) return false;
    Focus -= N; return true;
}
void UAshVitals::Heal(float N)
{
    if (GetOwner() && GetOwner()->HasAuthority()) Health = FMath::Clamp(Health + N, 0.f, MaxHealth);
}
void UAshVitals::StartParry(float Now) { if (GetOwner() && GetOwner()->HasAuthority() && SpendStamina(12.f)) ParryUntil = Now + .26f; }
void UAshVitals::StartDodge(float Now) { if (GetOwner() && GetOwner()->HasAuthority() && SpendStamina(21.f)) DodgeUntil = Now + .37f; }
bool UAshVitals::ReceiveHit(float Damage, float Now)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || bDead || Now < DodgeUntil) return false;
    if (Now <= ParryUntil && ParryUntil > 0.f) { Focus = FMath::Min(100.f, Focus + 12.f); ParryUntil = 0.f; return true; }
    if (const AAshHero* Hero=Cast<AAshHero>(GetOwner()))
    {
        if (Hero->bGuarding && SpendStamina(14.f)) Damage *= .35f;
    }
    Health = FMath::Max(0.f, Health - Damage * (1.f + Injury * 0.005f));
    Injury = FMath::Min(100.f, Injury + Damage * .12f);
    if (Health <= 0.f) bDead = true;
    return false;
}
void UAshVitals::RestoreAtShrine()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    bDead = false; Health = MaxHealth; Stamina = 100; Focus = 100;
    Hunger = FMath::Max(Hunger, 50.f); Thirst = FMath::Max(Thirst, 50.f);
    Infection = FMath::Max(0.f, Infection - 30.f); Injury = FMath::Max(0.f, Injury - 30.f);
}
