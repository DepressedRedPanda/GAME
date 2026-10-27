#include "AI/HRCompanionController.h"

#include "Characters/HRCompanionCharacter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRCombatComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Systems/HRFactionSubsystem.h"

AHRCompanionCharacter* AHRCompanionController::GetCompanion() const
{
	return Cast<AHRCompanionCharacter>(GetPawn());
}

AActor* AHRCompanionController::FindThreat(const AActor* Leader, float Radius) const
{
	const AHRCompanionCharacter* Companion = GetCompanion();

	// First priority: whatever the leader is fighting.
	if (const AHRCharacterBase* LeaderCharacter = Cast<AHRCharacterBase>(Leader))
	{
		AActor* LeaderTarget = LeaderCharacter->GetCombat()->GetCurrentTarget();
		if (IsValidHostile(LeaderTarget))
		{
			return LeaderTarget;
		}
	}

	// Then anything hostile and alert near the leader.
	AActor* Best = nullptr;
	float BestDistance = Radius;
	for (TActorIterator<AHRCharacterBase> It(GetWorld()); It; ++It)
	{
		AHRCharacterBase* Candidate = *It;
		if (Candidate == Companion || !IsValidHostile(Candidate) || Candidate->GetAwarenessTowards(Leader) != EHRAwareness::Alerted)
		{
			continue;
		}
		const float Distance = FVector::Dist(Candidate->GetActorLocation(), Leader->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Candidate;
		}
	}
	return Best;
}

void AHRCompanionController::TickBehavior(float DeltaSeconds)
{
	AHRCompanionCharacter* Companion = GetCompanion();
	AHRPlayerCharacter* Leader = Companion ? Companion->GetLeader() : nullptr;
	if (!Companion || Companion->IsKnockedOut())
	{
		StopAllMovement();
		return;
	}
	if (!Companion->IsRecruited() || !Leader)
	{
		// Not in the party yet: behave like any villager.
		Super::TickBehavior(DeltaSeconds);
		return;
	}

	// Mirror the leader's stealth.
	if (Leader->bIsCrouched && !Companion->bIsCrouched)
	{
		Companion->Crouch();
	}
	else if (!Leader->bIsCrouched && Companion->bIsCrouched)
	{
		Companion->UnCrouch();
	}

	const float LeaderDistance = FVector::Dist(Companion->GetActorLocation(), Leader->GetActorLocation());

	// Revive a downed leader.
	if (Leader->IsDowned() && Companion->CanRevivePlayer())
	{
		if (LeaderDistance > 160.f)
		{
			ReviveStartTime = -1.0;
			MoveToPoint(Leader->GetActorLocation(), 120.f);
		}
		else
		{
			StopAllMovement();
			if (ReviveStartTime < 0.0)
			{
				ReviveStartTime = Now();
			}
			else if (Now() - ReviveStartTime > 1.5)
			{
				Companion->ConsumeRevive();
				Leader->Resurrect(true);
				ReviveStartTime = -1.0;
			}
		}
		return;
	}
	ReviveStartTime = -1.0;

	// Teleport to catch up when hopelessly far behind.
	if (LeaderDistance > 4000.f)
	{
		const FVector Behind = Leader->GetActorLocation() - Leader->GetActorForwardVector() * 250.f;
		Companion->TeleportTo(Behind, Leader->GetActorRotation());
		return;
	}

	const EHRCompanionCommand Command = Companion->GetCommand();
	const float EngageRadius = Command == EHRCompanionCommand::Aggressive ? 2000.f : 1100.f;
	AActor* Threat = Command == EHRCompanionCommand::Passive ? nullptr : FindThreat(Leader, EngageRadius);

	if (Command == EHRCompanionCommand::Hold && Threat && FVector::Dist(Threat->GetActorLocation(), Companion->GetHoldLocation()) > 900.f)
	{
		Threat = nullptr;
	}

	if (Threat)
	{
		if (Target.Get() != Threat)
		{
			ReleaseToken();
			Target = Threat;
		}
		Awareness = EHRAwareness::Alerted;
		LastSeenTime = Now();
		SetCombatFacing(true);
		TickCombat(DeltaSeconds);
		return;
	}

	// No fight: follow or hold.
	if (Awareness == EHRAwareness::Alerted)
	{
		ReleaseToken();
		Target = nullptr;
		Awareness = EHRAwareness::Unaware;
		SetCombatFacing(false);
		ClearFocus(EAIFocusPriority::Gameplay);
	}

	const FVector Goal = Command == EHRCompanionCommand::Hold
		? Companion->GetHoldLocation()
		: Leader->GetActorLocation() - Leader->GetActorForwardVector() * 180.f + Leader->GetActorRightVector() * 120.f;
	if (FVector::Dist2D(Companion->GetActorLocation(), Goal) > 250.f && Now() >= NextDecisionTime)
	{
		NextDecisionTime = Now() + 0.4;
		Companion->MovementSpeedScale = LeaderDistance > 900.f ? 1.6f : 1.f;
		MoveToPoint(Goal, 120.f);
	}
}
