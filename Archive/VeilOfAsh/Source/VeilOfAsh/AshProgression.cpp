#include "AshProgression.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UAshProgression::UAshProgression() { SetIsReplicatedByDefault(true); }
void UAshProgression::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(UAshProgression, Level); DOREPLIFETIME(UAshProgression, XP);
    DOREPLIFETIME(UAshProgression, SkillPoints); DOREPLIFETIME(UAshProgression, Might);
    DOREPLIFETIME(UAshProgression, Guile); DOREPLIFETIME(UAshProgression, Insight);
    DOREPLIFETIME(UAshProgression, FreefolkReputation); DOREPLIFETIME(UAshProgression, ChoirReputation);
    DOREPLIFETIME(UAshProgression, HeroClass);
}
void UAshProgression::GainXP(int32 Amount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0) return;
    XP += Amount;
    while (XP >= Level * 75)
    {
        XP -= Level * 75;
        Level++;
        SkillPoints++;
    }
}
void UAshProgression::ChooseClass(EAshClass NewClass)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Level > 1) return;
    HeroClass = NewClass;
}
bool UAshProgression::BuySkill(int32 Branch)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || SkillPoints < 1) return false;
    if (Branch == 0) Might++; else if (Branch == 1) Guile++; else if (Branch == 2) Insight++;
    else return false;
    SkillPoints--; return true;
}
