#include "Abilities/HRVeilAbilities.h"
#include "Characters/HRCharacterBase.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Systems/HRFactionSubsystem.h"
#include "SharedArtLibrary.h"

UHRAbility_VoidPulse::UHRAbility_VoidPulse()
{
    AbilityId = TEXT("Ability.VoidPulse");
    DisplayName = NSLOCTEXT("Veil", "VoidPulse", "Void Pulse");
    Description = NSLOCTEXT("Veil", "VoidPulseDesc", "A close-range arcane pulse that damages and staggers hostile enemies.");
    FocusCost = 25.f;
    Cooldown = 1.8f;
    CastTime = 0.2f;
}

void UHRAbility_VoidPulse::Activate(AHRCharacterBase* Owner)
{
    if (!Owner) return;
    Super::Activate(Owner);
    TArray<FOverlapResult> Hits;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(VoidPulse), false, Owner);
    Owner->GetWorld()->OverlapMultiByObjectType(Hits, Owner->GetActorLocation(), FQuat::Identity, Objects, FCollisionShape::MakeSphere(430.f), Params);
    TSet<AActor*> Seen;
    for (const FOverlapResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (!Target || Target == Owner || Seen.Contains(Target) || !UHRFactionSubsystem::AreHostile(Owner, Target)) continue;
        Seen.Add(Target);
        if (UHRCombatComponent* Combat = Target->FindComponentByClass<UHRCombatComponent>())
        {
            FHRDamageInfo Damage;
            Damage.Amount = 20.f + Owner->GetAttributes()->GetStat(EHRStat::ArcanePower) * 1.3f;
            Damage.PostureDamage = 45.f;
            Damage.Type = EHRDamageType::Arcane;
            Damage.Source = Owner;
            Combat->ReceiveHit(Damage);
        }
    }
    USharedArtLibrary::PlayCue(Owner, TEXT("Ability.VoidPulse"), Owner->GetActorLocation());
}

UHRAbility_SpectralForm::UHRAbility_SpectralForm()
{
    AbilityId = TEXT("Ability.SpectralForm");
    DisplayName = NSLOCTEXT("Veil", "SpectralForm", "Spectral Form");
    Description = NSLOCTEXT("Veil", "SpectralDesc", "Toggle reduced detection, draining focus until dismissed or exhausted.");
    FocusCost = 20.f;
    FocusPerSecond = 3.2f;
    Cooldown = 1.f;
    bIsToggle = true;
}

void UHRAbility_SpectralForm::Activate(AHRCharacterBase* Owner)
{
    if (!Owner) return;
    Super::Activate(Owner);
    Owner->GetAttributes()->SetModifierSource(TEXT("SpectralForm"), { FHRStatModifier(EHRStat::Stealth, 0.35f) });
    USharedArtLibrary::PlayCue(Owner, TEXT("Ability.SpectralForm"), Owner->GetActorLocation());
}

void UHRAbility_SpectralForm::Deactivate(AHRCharacterBase* Owner)
{
    if (Owner) Owner->GetAttributes()->RemoveModifierSource(TEXT("SpectralForm"));
    Super::Deactivate(Owner);
}
