#include "AshHUD.h"
#include "AshHero.h"
#include "AshVitals.h"
#include "AshProgression.h"
#include "AshInventory.h"
#include "AshWorldState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AAshHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!PlayerOwner || !Canvas || !GEngine) return;
    const AAshHero* H=Cast<AAshHero>(PlayerOwner->GetPawn());
    if (!H || !H->Vitals || !H->Inventory || !H->Progression) return;
    const AAshWorldState* S=GetWorld()->GetGameState<AAshWorldState>();
    const UFont* Font=GEngine->GetSmallFont();
    float Y=18.f;
    auto Line=[&](const FString& Str, const FLinearColor& Color)
    {
        DrawText(Str,Color,20.f,Y,const_cast<UFont*>(Font),1.2f,false); Y+=21.f;
    };
    Line(TEXT("VEIL OF ASH // SYSTEMS PROTOTYPE"), FLinearColor(.95f,.82f,.5f));
    Line(FString::Printf(TEXT("HP %.0f/%.0f | ST %.0f | FOCUS %.0f | HUNGER %.0f | THIRST %.0f"),
        H->Vitals->Health,H->Vitals->MaxHealth,H->Vitals->Stamina,H->Vitals->Focus,H->Vitals->Hunger,H->Vitals->Thirst),FLinearColor::White);
    Line(FString::Printf(TEXT("Injury %.0f | Infection %.0f | Weapon +%d | Kills %d | Deaths %d"),
        H->Vitals->Injury,H->Vitals->Infection,H->Inventory->WeaponTier,H->Kills,H->Deaths),FLinearColor::White);
    Line(FString::Printf(TEXT("Level %d XP %d/%d | SP %d | Might %d Guile %d Insight %d"),
        H->Progression->Level,H->Progression->XP,H->Progression->Level*75,H->Progression->SkillPoints,
        H->Progression->Might,H->Progression->Guile,H->Progression->Insight),FLinearColor::White);
    if (S)
    {
        const FString Weather=S->Weather==EAshWeather::Clear?TEXT("CLEAR"):(S->Weather==EAshWeather::Rain?TEXT("RAIN"):TEXT("ASH STORM"));
        Line(FString::Printf(TEXT("World: %02d:00 | %s | Clues %d | Gate %s | Boss %s"),
            FMath::FloorToInt(S->TimeOfDay),*Weather,S->DiscoveryCount,S->bGateOpen?TEXT("OPEN"):TEXT("LOCKED"),
            S->bBossDefeated?TEXT("DEFEATED"):TEXT("ALIVE")),FLinearColor(.65f,.85f,1.f));
    }
    if (S) Line(FString::Printf(TEXT("RUNE PUZZLE: %d/3 completed | Alternate gate route %s"),S->RuneProgress,
        S->bRuneSolved?TEXT("UNLOCKED"):TEXT("LOCKED")),FLinearColor(.75f,.7f,1.f));
    Line(FString::Printf(TEXT("Quest: Ciphers %d/3 | Lore %d/2 | Reputation Freefolk %d Choir %d"),
        H->Inventory->Count(TEXT("Cipher")),H->Inventory->Count(TEXT("Knowledge")),
        H->Progression->FreefolkReputation,H->Progression->ChoirReputation),FLinearColor(.65f,1.f,.65f));
    Line(FString::Printf(TEXT("Position: X %.0f Y %.0f | Nearby [E]: %s"),
        H->GetActorLocation().X,H->GetActorLocation().Y,*H->NearbyName()),FLinearColor::Yellow);
    Line(FString::Printf(TEXT("Bag: %s"),*H->Inventory->Describe()),FLinearColor::White);
    Y+=10;
    Line(TEXT("WASD Move | Mouse Look | Space Jump | Shift Sprint | Ctrl Crouch"),FLinearColor(.72f,.72f,.72f));
    Line(TEXT("LMB Attack | RMB Heavy | Q Parry | Alt Dodge | V Guard | F Void Pulse | G Spectral | T Mantle"),FLinearColor(.72f,.72f,.72f));
    Line(TEXT("E Interact | B Build | 1/2/3 Craft ration/tonic/blade | 4/5 Consume"),FLinearColor(.72f,.72f,.72f));
    Line(TEXT("6/7/8 Skills | F1/F2/F3 Class | C Ally orders | F5 Save F9 Load R Respawn"),FLinearColor(.72f,.72f,.72f));
    Line(TEXT("After boss: F6 Freefolk | F7 Choir | F8 Destroy engine"),FLinearColor(.72f,.72f,.72f));
    Y+=10;
    if (H->Vitals->bDead) Line(TEXT("YOU DIED. PRESS R TO RESURRECT AT SHRINE."),FLinearColor::Red);
    if (!H->LastMessage.IsEmpty()) Line(H->LastMessage,FLinearColor::Yellow);
}
