#include "Characters/HRCompanionCharacter.h"

#include "AI/HRCompanionController.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Core/HRStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Systems/HRSaveSubsystem.h"
#include "Systems/HRSaveGame.h"

#define LOCTEXT_NAMESPACE "HRCompanion"

AHRCompanionCharacter::AHRCompanionCharacter()
{
	SharedAppearanceRole = TEXT("Companion");
	AIControllerClass = AHRCompanionController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	FactionId = TEXT("Villagers");
	DisplayName = LOCTEXT("KestrelName", "Kestrel");
	BodyColor = FLinearColor(0.15f, 0.5f, 0.25f);
	BaseWalkSpeed = 440.f;

	Attributes->SetBaseStat(EHRStat::MaxHealth, 260.f);
	Attributes->SetBaseStat(EHRStat::PhysicalPower, 8.f);
	Combat->DefaultWeapon.BaseDamage = 16.f;
}

void AHRCompanionCharacter::BeginPlay()
{
	Super::BeginPlay();
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	const FName RecruitedFlag(*FString::Printf(TEXT("%s.Recruited"), *CompanionId.ToString()));
	if (Narrative && Narrative->GetFlag(RecruitedFlag) > 0)
	{
		// Restored from a save: rejoin the player next frame once they exist.
		TWeakObjectPtr<AHRCompanionCharacter> WeakThis = this;
		GetWorldTimerManager().SetTimerForNextTick([WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				if (AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(WeakThis.Get()))
				{
					WeakThis->Recruit(Player, true);
				}
			}
		});
	}
}

bool AHRCompanionCharacter::CanInteract(const AHRPlayerCharacter* Player) const
{
	return !bIsDead;
}

FText AHRCompanionCharacter::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	if (bKnockedOut)
	{
		return FText::Format(LOCTEXT("HelpUp", "Help {0} up"), DisplayName);
	}
	return FText::Format(LOCTEXT("Talk", "Talk to {0}"), DisplayName);
}

void AHRCompanionCharacter::Interact(AHRPlayerCharacter* Player)
{
	if (bKnockedOut)
	{
		WakeUp(0.4f);
		return;
	}
	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->StartDialogue(DialogueId, this, CompanionId, Player);
	}
}

void AHRCompanionCharacter::Recruit(AHRPlayerCharacter* Player, bool bRestoreFromSave)
{
	if (!Player)
	{
		return;
	}
	Leader = Player;
	bRecruited = true;
	FactionId = TEXT("Player");
	Command = EHRCompanionCommand::Follow;
	RevivesRemaining = MaxRevives;
	if (const UHRSaveSubsystem* Saves = bRestoreFromSave ? HRGetSubsystem<UHRSaveSubsystem>(this) : nullptr)
	{
		const UHRSaveGame* Save = Saves->GetLoadedSave();
		if (Save && Save->bHasCompanionState)
		{
			Command = static_cast<EHRCompanionCommand>(FMath::Clamp(Save->CompanionCommand, 0, 3));
			HoldLocation = Save->CompanionHoldLocation;
			RevivesRemaining = FMath::Clamp(Save->CompanionRevives, 0, MaxRevives);
		}
	}
	Player->SetCompanion(this);

	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->SetFlag(FName(*FString::Printf(TEXT("%s.Recruited"), *CompanionId.ToString())), 1);
	}
}

void AHRCompanionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AHRPlayerCharacter* Player = Leader.Get();
	if (!bRecruited || bKnockedOut || !Player || Player->IsDead() || FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) > FMath::Square(600.f)) return;
	AssistTimer += DeltaSeconds;
	if (AssistTimer < 2.6f) return;
	AssistTimer = 0.f;
	if (Command == EHRCompanionCommand::Follow) Player->GetAttributes()->RestoreFocus(5.f);
	else if (Command == EHRCompanionCommand::Hold) Player->GetAttributes()->Heal(5.f);
}

void AHRCompanionCharacter::Dismiss()
{
	if (AHRPlayerCharacter* Player = Leader.Get())
	{
		Player->SetCompanion(nullptr);
	}
	bRecruited = false;
	Leader = nullptr;
	FactionId = TEXT("Villagers");
	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->SetFlag(FName(*FString::Printf(TEXT("%s.Recruited"), *CompanionId.ToString())), 0);
	}
	UHRStatics::Notify(this, FText::Format(LOCTEXT("Left", "{0} has left your side."), DisplayName), FLinearColor(0.8f, 0.5f, 0.5f), true);
}

void AHRCompanionCharacter::CycleCommand()
{
	if (!bRecruited)
	{
		return;
	}
	Command = static_cast<EHRCompanionCommand>((static_cast<uint8>(Command) + 1) % 4);
	if (Command == EHRCompanionCommand::Hold)
	{
		HoldLocation = GetActorLocation();
	}
	UHRStatics::Notify(this, FText::Format(LOCTEXT("Command", "{0}: {1}"), DisplayName, UEnum::GetDisplayValueAsText(Command)), FLinearColor(0.5f, 0.9f, 0.6f));
}

int32 AHRCompanionCharacter::GetApproval() const
{
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	return Narrative ? Narrative->GetFlag(FName(*FString::Printf(TEXT("%s.Approval"), *CompanionId.ToString()))) : 0;
}

void AHRCompanionCharacter::ModifyApproval(int32 Delta)
{
	UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	if (!Narrative)
	{
		return;
	}
	Narrative->AddFlag(FName(*FString::Printf(TEXT("%s.Approval"), *CompanionId.ToString())), Delta);
	UHRStatics::Notify(this, FText::Format(Delta >= 0 ? LOCTEXT("Approves", "{0} approves.") : LOCTEXT("Disapproves", "{0} disapproves."), DisplayName),
		Delta >= 0 ? FLinearColor(0.5f, 1.f, 0.5f) : FLinearColor(1.f, 0.5f, 0.4f));
	if (bRecruited && GetApproval() <= LeaveBelowApproval)
	{
		Dismiss();
	}
}

void AHRCompanionCharacter::HandleHealthDepleted(UHRAttributeComponent* InAttributes, AActor* Killer)
{
	if (bKnockedOut)
	{
		return;
	}
	bKnockedOut = true;
	Combat->SetDowned(true);
	GetCharacterMovement()->StopMovementImmediately();
	UHRStatics::Notify(this, FText::Format(LOCTEXT("KnockedOut", "{0} is down!"), DisplayName), FLinearColor(1.f, 0.6f, 0.3f));

	TWeakObjectPtr<AHRCompanionCharacter> WeakThis = this;
	GetWorldTimerManager().SetTimer(KnockoutTimer, FTimerDelegate::CreateLambda([WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->WakeUp(0.3f);
		}
	}), KnockoutDuration, false);
}

void AHRCompanionCharacter::WakeUp(float HealthFraction)
{
	if (!bKnockedOut)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(KnockoutTimer);
	bKnockedOut = false;
	Attributes->SetHealthFraction(HealthFraction);
	Attributes->ResetPosture();
	Combat->SetDowned(false);
	Combat->GrantInvulnerability(1.5f);
}

#undef LOCTEXT_NAMESPACE
