#include "World/HRVeilNode.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Core/HRCampaignRules.h"
#include "Core/HRStatics.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Systems/HRFactionSubsystem.h"
#include "World/HRGate.h"

void AHRVeilNode::BeginPlay()
{
    Super::BeginPlay();
    if ((Kind == EHRVeilNodeKind::Cipher || Kind == EHRVeilNodeKind::Record) && GetPersistentFlag(TEXT("Collected")) > 0)
    {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
    }
    if (Kind == EHRVeilNodeKind::Gate && LinkedGate)
    {
        if (const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
        {
            if (Narrative->GetFlag(TEXT("Veil.GateOpen")) > 0) LinkedGate->Open();
        }
    }
}

bool AHRVeilNode::CanInteract(const AHRPlayerCharacter* Player) const
{
    return Player && !Player->IsDead() && GetPersistentFlag(TEXT("Collected")) == 0;
}

FText AHRVeilNode::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
    return FText::Format(NSLOCTEXT("Veil", "Interact", "Examine {0}"), DisplayName);
}

void AHRVeilNode::Interact(AHRPlayerCharacter* Player)
{
    if (!CanInteract(Player)) return;
    UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
    if (!Narrative) return;
    UHRInventoryComponent* Inventory = Player->GetInventory();
    const bool bPickup = Kind == EHRVeilNodeKind::Cipher || Kind == EHRVeilNodeKind::Record;
    if (bPickup)
    {
        const FName Item = Kind == EHRVeilNodeKind::Cipher ? FName(TEXT("Item.VeilCipher")) : FName(TEXT("Item.VeilRecord"));
        if (!Inventory->AddItem(Item, 1).IsValid()) return;
        SetPersistentFlag(TEXT("Collected"), 1);
        if (Kind == EHRVeilNodeKind::Record)
        {
            Narrative->AddKnowledge(TEXT("Lore.VeilRunes"));
            Player->GetProgression()->AddEchoes(25);
            UHRStatics::Notify(this, NSLOCTEXT("Veil", "Order", "The lost record reveals: SECOND, FIRST, THIRD."));
        }
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        return;
    }
    if (Kind == EHRVeilNodeKind::Rune)
    {
        const int32 Progress = HRCampaignRules::AdvanceRune(Narrative->GetFlag(TEXT("Veil.RuneProgress")), RuneNumber);
        Narrative->SetFlag(TEXT("Veil.RuneProgress"), Progress);
        if (Progress == 3) Narrative->SetFlag(TEXT("Veil.RuneSolved"), 1);
        UHRStatics::Notify(this, Progress == 3 ? NSLOCTEXT("Veil", "Solved", "The runic circuit is solved.") : (Progress == 0 ? NSLOCTEXT("Veil", "Wrong", "Wrong rune. The sequence resets.") : NSLOCTEXT("Veil", "Accepted", "Rune accepted.")));
        return;
    }
    if (Kind == EHRVeilNodeKind::Gate)
    {
        if (HRCampaignRules::CanOpenMemoryGate(Inventory->CountItem(TEXT("Item.VeilCipher")), Inventory->CountItem(TEXT("Item.VeilRecord")), Narrative->GetFlag(TEXT("Veil.RuneSolved")) > 0))
        {
            Narrative->SetFlag(TEXT("Veil.GateOpen"), 1);
            if (LinkedGate) LinkedGate->Open();
            UHRStatics::Notify(this, NSLOCTEXT("Veil", "GateOpen", "The memory gate opens. The Pale Keeper's shield falls."));
        }
        else UHRStatics::Notify(this, NSLOCTEXT("Veil", "GateLocked", "Find 3 ciphers + 2 records, or solve the runes and bring 1 cipher + 1 record."));
        return;
    }
    if (Kind == EHRVeilNodeKind::FreefolkEnvoy || Kind == EHRVeilNodeKind::ChoirEnvoy)
    {
        // Each envoy grants allegiance once, preventing reputation farming by repeated interaction.
        if (GetPersistentFlag(TEXT("Pledged")) > 0) return;
        if (UHRFactionSubsystem* Factions = HRGetSubsystem<UHRFactionSubsystem>(this))
        {
            const bool bFreefolk = Kind == EHRVeilNodeKind::FreefolkEnvoy;
            Factions->ModifyReputation(bFreefolk ? TEXT("Freefolk") : TEXT("Choir"), 10.f);
            Factions->ModifyReputation(bFreefolk ? TEXT("Choir") : TEXT("Freefolk"), -10.f);
            SetPersistentFlag(TEXT("Pledged"), 1);
            Narrative->AddKnowledge(bFreefolk ? TEXT("Lore.Freefolk") : TEXT("Lore.Choir"));
        }
        return;
    }
    const EHRStartingClass Origin = Kind == EHRVeilNodeKind::Vanguard ? EHRStartingClass::Vanguard : (Kind == EHRVeilNodeKind::Seer ? EHRStartingClass::Seer : EHRStartingClass::Shade);
    if (!Player->GetProgression()->ChooseStartingClass(Origin)) UHRStatics::Notify(this, NSLOCTEXT("Veil", "ClassLocked", "Your origin has already been chosen."));
}
