#include "Characters/HRBossCharacter.h"
#include "AI/HRAIController.h"

#include "Characters/HRPlayerCharacter.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Core/HRStatics.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Narrative/HRQuestSubsystem.h"
#include "World/HRGate.h"

#define LOCTEXT_NAMESPACE "HRBoss"

namespace
{
	const FName PhaseSource(TEXT("BossPhase"));
}

AHRBossCharacter::AHRBossCharacter()
{
	SharedAppearanceRole = TEXT("Boss");
	bRespawnsOnRest = false;
	BodyColor = FLinearColor(0.35f, 0.1f, 0.45f);
	EchoReward = 2500;
	LootTableId = TEXT("Loot.Boss");
	SkillLevel = 0.55f;
	Aggression = 0.65f;
	PerilousChance = 0.2f;
	MaxComboHits = 4;
	SightRange = 3000.f;

	Combat->bCanBeTakenDown = true; // A takedown still lands one deathblow.
	Combat->bRecoilWhenDeflected = false;
	Combat->bHyperArmorOnHeavy = true;
	Combat->PoiseThreshold = 30.f;
	Combat->MaxAttackTokens = 3;
	Combat->PostureBrokenDuration = 2.f;

	Attributes->SetBaseStat(EHRStat::MaxHealth, 900.f);
	Attributes->SetBaseStat(EHRStat::MaxPosture, 260.f);
	Attributes->SetBaseStat(EHRStat::PostureRegen, 10.f);
	Attributes->SetBaseStat(EHRStat::Defense, 25.f);
}

void AHRBossCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetVisualScale(1.4f);

	if (IsDefeated())
	{
		bIsDead = true;
		Dormant();
		SetArenaSealed(false);
		return;
	}
	EnterPhase(0);
}

void AHRBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bIsDead && !bFightActive && IsCampaignUnlocked())
	{
		const AHRAIController* AI = GetHRController();
		if (AI && AI->GetAwareness() == EHRAwareness::Alerted) OnAwarenessChanged(EHRAwareness::Unaware, EHRAwareness::Alerted);
	}
}

bool AHRBossCharacter::IsCampaignUnlocked() const
{
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	return RequiredCampaignFlag.IsNone() || (Narrative && Narrative->GetFlag(RequiredCampaignFlag) > 0);
}

FName AHRBossCharacter::GetDefeatedFlag() const
{
	return FName(*FString::Printf(TEXT("%s.Defeated"), *BossId.ToString()));
}

bool AHRBossCharacter::IsDefeated() const
{
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	return Narrative && Narrative->GetFlag(GetDefeatedFlag()) > 0;
}

void AHRBossCharacter::EnterPhase(int32 NewPhase)
{
	PhaseIndex = NewPhase;
	if (!Phases.IsValidIndex(NewPhase))
	{
		return;
	}
	const FHRBossPhase& Phase = Phases[NewPhase];
	Attributes->SetModifierSource(PhaseSource, Phase.Modifiers);
	SkillLevel = Phase.SkillLevel;
	Aggression = Phase.Aggression;
	PerilousChance = Phase.PerilousChance;
	BodyColor = Phase.Color;
	ResetVisualColor();
}

void AHRBossCharacter::OnAwarenessChanged(EHRAwareness OldAwareness, EHRAwareness NewAwareness)
{
	if (!IsCampaignUnlocked()) return;
	if (NewAwareness == EHRAwareness::Alerted && !bFightActive && !bIsDead)
	{
		bFightActive = true;
		SetArenaSealed(true);
		UHRStatics::Notify(this, DisplayName, FLinearColor(0.8f, 0.6f, 1.f), true);
	}
}

void AHRBossCharacter::SetArenaSealed(bool bSealed)
{
	for (AHRGate* Gate : ArenaGates)
	{
		if (Gate)
		{
			bSealed ? Gate->Close() : Gate->Open();
		}
	}
}

void AHRBossCharacter::HandleHealthDepleted(UHRAttributeComponent* InAttributes, AActor* Killer)
{
	if (!IsCampaignUnlocked()) { Attributes->RefillAll(); return; }
	if (bTransitioning)
	{
		return;
	}
	if (PhaseIndex < GetPhaseCount() - 1)
	{
		// One deathblow down, more to go.
		bTransitioning = true;
		EnterPhase(PhaseIndex + 1);
		Attributes->RefillAll();
		Combat->ResetCombatState();
		Combat->GrantInvulnerability(2.f);
		Combat->BeginBusy(1.5f);
		if (Phases.IsValidIndex(PhaseIndex) && !Phases[PhaseIndex].PhaseLine.IsEmpty())
		{
			UHRStatics::Notify(this, Phases[PhaseIndex].PhaseLine, FLinearColor(0.8f, 0.5f, 1.f), true);
		}
		bTransitioning = false;
		return;
	}
	Die(Killer);
}

void AHRBossCharacter::Die(AActor* Killer)
{
	if (!IsCampaignUnlocked()) return;
	if (bIsDead)
	{
		return;
	}
	bFightActive = false;
	SetArenaSealed(false);

	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->SetFlag(GetDefeatedFlag(), 1);
	}
	if (UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this))
	{
		Quests->NotifyKill(BossId);
	}
	if (AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(this))
	{
		Player->GetProgression()->GrantSkillPoints(SkillPointReward);
		if (!RewardItem.IsNone())
		{
			Player->GetInventory()->AddItem(RewardItem, 1, true);
		}
	}
	UHRStatics::Notify(this, FText::Format(LOCTEXT("Felled", "{0} FELLED"), DisplayName), FLinearColor(1.f, 0.8f, 0.3f), true);

	Super::Die(Killer);
}

void AHRBossCharacter::ResetEnemy()
{
	if (IsDefeated())
	{
		return;
	}
	bFightActive = false;
	SetArenaSealed(false);
	Super::ResetEnemy();
	EnterPhase(0);
}

void AHRBossCharacter::OnPlayerDefeated()
{
	if (!bFightActive)
	{
		return;
	}
	UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	if (!Narrative || Hints.Num() == 0)
	{
		return;
	}
	// Each defeat reveals the next hint.
	const FName DeathFlag(*FString::Printf(TEXT("%s.Deaths"), *BossId.ToString()));
	Narrative->AddFlag(DeathFlag, 1);
	const int32 Deaths = Narrative->GetFlag(DeathFlag);
	const FText& Hint = Hints[FMath::Min(Deaths - 1, Hints.Num() - 1)];
	UHRStatics::Notify(this, FText::Format(LOCTEXT("Hint", "A memory of defeat: {0}"), Hint), FLinearColor(0.7f, 0.7f, 1.f), true);
}

#undef LOCTEXT_NAMESPACE
