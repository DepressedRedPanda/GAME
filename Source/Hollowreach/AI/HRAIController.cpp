#include "AI/HRAIController.h"

#include "Characters/HRCharacterBase.h"
#include "Characters/HREnemyCharacter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/HRStealthComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Systems/HRFactionSubsystem.h"
#include "Systems/HRTimeWeatherSubsystem.h"
#include "TimerManager.h"

namespace
{
	constexpr float SuspiciousThreshold = 25.f;
	constexpr float SearchingThreshold = 70.f;
	constexpr float AlertThreshold = 100.f;
}

AHRAIController::AHRAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 3500.f;
	SightConfig->LoseSightRadius = 3800.f;
	SightConfig->PeripheralVisionAngleDegrees = 70.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(2.f);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 2500.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(1.f);

	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(UAISense_Sight::StaticClass());
	SetPerceptionComponent(*Perception);
}

void AHRAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AHRAIController::HandlePerception);
	if (AHRCharacterBase* HRCharacter = GetHRPawn())
	{
		HRCharacter->GetAttributes()->OnDamaged.AddUniqueDynamic(this, &AHRAIController::HandlePawnDamaged);
	}
	ResetAI();
}

double AHRAIController::Now() const
{
	return GetWorld()->GetTimeSeconds();
}

AHRCharacterBase* AHRAIController::GetHRPawn() const
{
	return Cast<AHRCharacterBase>(GetPawn());
}

AHREnemyCharacter* AHRAIController::GetEnemyPawn() const
{
	return Cast<AHREnemyCharacter>(GetPawn());
}

void AHRAIController::ResetAI()
{
	ReleaseToken();
	StopAllMovement();
	Target = nullptr;
	AwarenessValue = 0.f;
	PendingComboHits = 0;
	VisibleActors.Reset();
	SetAwareness(EHRAwareness::Unaware);
	SetCombatFacing(false);
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AHRAIController::HandlePawnDeath()
{
	ReleaseToken();
	StopAllMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
}

// --- Perception --------------------------------------------------------------------------------

bool AHRAIController::IsValidHostile(const AActor* Actor) const
{
	const AHRCharacterBase* HRCharacter = Cast<AHRCharacterBase>(Actor);
	return HRCharacter && !HRCharacter->IsDead() && UHRFactionSubsystem::AreHostile(GetPawn(), HRCharacter);
}

void AHRAIController::HandlePerception(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !GetPawn())
	{
		return;
	}

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			VisibleActors.AddUnique(Actor);
		}
		else
		{
			VisibleActors.RemoveAll([Actor](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Actor; });
			if (Actor == Target.Get())
			{
				LastKnownLocation = Stimulus.StimulusLocation;
			}
		}
		return;
	}

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
	{
		// Only noises made by things we'd fight are interesting.
		if (!IsValidHostile(Actor) && !IsValidHostile(Actor->GetInstigator()))
		{
			return;
		}
		if (Awareness == EHRAwareness::Alerted)
		{
			if (Actor == Target.Get())
			{
				LastKnownLocation = Stimulus.StimulusLocation;
			}
			return;
		}
		LastKnownLocation = Stimulus.StimulusLocation;
		AwarenessValue = FMath::Min(AlertThreshold - 5.f, FMath::Max(AwarenessValue, SuspiciousThreshold) + 35.f * Stimulus.Strength);
		StateStartTime = Now();
	}
}

void AHRAIController::HandlePawnDamaged(UHRAttributeComponent* Attributes, const FHRDamageInfo& Damage)
{
	if (Damage.Source && Damage.Source != GetPawn() && IsValidHostile(Damage.Source))
	{
		AlertTo(Damage.Source);
	}
	else if (Damage.Source && Damage.Source != GetPawn())
	{
		// Neutral actors that are attacked fight back (and the faction remembers).
		if (Cast<AHRCharacterBase>(Damage.Source))
		{
			AlertTo(Damage.Source);
		}
	}
}

void AHRAIController::AlertTo(AActor* InTarget)
{
	if (!InTarget || !GetHRPawn() || GetHRPawn()->IsDead())
	{
		return;
	}
	Target = InTarget;
	LastKnownLocation = InTarget->GetActorLocation();
	LastSeenTime = Now();
	AwarenessValue = AlertThreshold;
	SetAwareness(EHRAwareness::Alerted);
}

void AHRAIController::LoseTrackOf(AActor* Actor)
{
	if (Target.Get() != Actor)
	{
		return;
	}
	ReleaseToken();
	Target = nullptr;
	AwarenessValue = SearchingThreshold;
	SetAwareness(EHRAwareness::Searching);
}

void AHRAIController::SetAwareness(EHRAwareness NewAwareness)
{
	if (NewAwareness == Awareness)
	{
		return;
	}
	const EHRAwareness Old = Awareness;
	Awareness = NewAwareness;
	StateStartTime = Now();

	AHRCharacterBase* HRCharacter = GetHRPawn();
	if (HRCharacter)
	{
		HRCharacter->MovementSpeedScale = NewAwareness == EHRAwareness::Suspicious ? 0.45f : (NewAwareness == EHRAwareness::Searching ? 0.8f : 1.f);
	}
	SetCombatFacing(NewAwareness == EHRAwareness::Alerted);

	if (NewAwareness == EHRAwareness::Alerted)
	{
		AlertAllies();
	}
	else
	{
		ReleaseToken();
		ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (AHREnemyCharacter* Enemy = GetEnemyPawn())
	{
		Enemy->OnAwarenessChanged(Old, NewAwareness);
	}
	OnAwarenessChanged.Broadcast(Old, NewAwareness);
}

void AHRAIController::AlertAllies()
{
	const AHRCharacterBase* Self = GetHRPawn();
	AActor* CurrentTarget = Target.Get();
	if (!Self || !CurrentTarget)
	{
		return;
	}
	for (TActorIterator<AHRAIController> It(GetWorld()); It; ++It)
	{
		AHRAIController* Other = *It;
		const AHRCharacterBase* OtherPawn = Other->GetHRPawn();
		if (Other == this || !OtherPawn || OtherPawn->IsDead() || Other->Awareness == EHRAwareness::Alerted)
		{
			continue;
		}
		if (OtherPawn->FactionId == Self->FactionId && FVector::Dist(OtherPawn->GetActorLocation(), Self->GetActorLocation()) < AllyAlertRadius
			&& UHRFactionSubsystem::AreHostile(OtherPawn, CurrentTarget))
		{
			Other->AlertTo(CurrentTarget);
		}
	}
}

// --- Tick --------------------------------------------------------------------------------------

void AHRAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AHRCharacterBase* HRCharacter = GetHRPawn();
	if (!HRCharacter || HRCharacter->IsDead())
	{
		return;
	}
	TickBehavior(DeltaSeconds);
	TickDirectSteering();
}

void AHRAIController::TickBehavior(float DeltaSeconds)
{
	TickAwareness(DeltaSeconds);
	switch (Awareness)
	{
	case EHRAwareness::Unaware: TickPatrol(DeltaSeconds); break;
	case EHRAwareness::Suspicious: TickSuspicious(DeltaSeconds); break;
	case EHRAwareness::Searching: TickSearch(DeltaSeconds); break;
	case EHRAwareness::Alerted: TickCombat(DeltaSeconds); break;
	}
}

void AHRAIController::TickAwareness(float DeltaSeconds)
{
	const AHRCharacterBase* Self = GetHRPawn();
	const AHREnemyCharacter* Enemy = GetEnemyPawn();
	const UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>();
	const float SightRange = (Enemy ? Enemy->SightRange : 2000.f) * (TimeWeather ? TimeWeather->GetSightMultiplier() : 1.f);

	// Pick the closest visible hostile.
	AActor* Best = nullptr;
	float BestDistance = SightRange;
	VisibleActors.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });
	for (const TWeakObjectPtr<AActor>& Ptr : VisibleActors)
	{
		AActor* Actor = Ptr.Get();
		if (!IsValidHostile(Actor))
		{
			continue;
		}
		if (const AHRPlayerCharacter* Player = Cast<AHRPlayerCharacter>(Actor))
		{
			if (Player->IsDowned())
			{
				continue;
			}
		}
		const float Distance = FVector::Dist(Actor->GetActorLocation(), Self->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Actor;
		}
	}

	if (Best)
	{
		const UHRStealthComponent* Stealth = Best->FindComponentByClass<UHRStealthComponent>();
		const float Visibility = Stealth ? Stealth->GetVisibility() : 1.f;
		const float DistanceFactor = 1.f - 0.75f * FMath::Clamp(BestDistance / FMath::Max(1.f, SightRange), 0.f, 1.f);
		float Gain = AwarenessGainRate * Visibility * DistanceFactor;
		if (BestDistance < 350.f)
		{
			Gain *= 3.f;
		}
		if (Awareness >= EHRAwareness::Searching)
		{
			Gain *= 2.f;
		}
		AwarenessValue = FMath::Min(AlertThreshold, AwarenessValue + Gain * DeltaSeconds);
		LastKnownLocation = Best->GetActorLocation();
		LastSeenTime = Now();
		if (Awareness != EHRAwareness::Alerted || !Target.IsValid())
		{
			Target = Best;
		}
	}
	else if (Awareness == EHRAwareness::Alerted)
	{
		// Keep chasing the last known position for a while; combat code handles the pursuit.
		if (Now() - LastSeenTime > LoseTargetTime)
		{
			ReleaseToken();
			AwarenessValue = SearchingThreshold + 10.f;
			SetAwareness(EHRAwareness::Searching);
		}
		return;
	}
	else
	{
		const float Decay = Awareness == EHRAwareness::Searching && Now() - StateStartTime < SearchDuration ? 0.f : AwarenessDecayRate;
		AwarenessValue = FMath::Max(0.f, AwarenessValue - Decay * DeltaSeconds);
	}

	if (Awareness == EHRAwareness::Alerted)
	{
		return;
	}
	if (AwarenessValue >= AlertThreshold && Target.IsValid())
	{
		SetAwareness(EHRAwareness::Alerted);
	}
	else if (AwarenessValue >= SearchingThreshold)
	{
		SetAwareness(EHRAwareness::Searching);
	}
	else if (AwarenessValue >= SuspiciousThreshold)
	{
		SetAwareness(EHRAwareness::Suspicious);
	}
	else if (Awareness != EHRAwareness::Unaware)
	{
		SetAwareness(EHRAwareness::Unaware);
		Target = nullptr;
	}
}

void AHRAIController::TickPatrol(float DeltaSeconds)
{
	const AHREnemyCharacter* Enemy = GetEnemyPawn();
	const AHRCharacterBase* Self = GetHRPawn();
	if (!Self || (Enemy && Enemy->bStationary))
	{
		if (Enemy && FVector::Dist2D(Self->GetActorLocation(), Enemy->GetHomeLocation()) > 150.f && Now() >= NextPatrolMoveTime)
		{
			NextPatrolMoveTime = Now() + 2.0;
			MoveToPoint(Enemy->GetHomeLocation(), 50.f);
		}
		return;
	}
	if (Now() < NextPatrolMoveTime)
	{
		return;
	}

	const bool bArrived = FVector::Dist2D(Self->GetActorLocation(), PatrolDestination) < 120.f;
	const bool bIdle = GetMoveStatus() == EPathFollowingStatus::Idle && !bDirectSteering;
	if (!bArrived && !bIdle && !PatrolDestination.IsZero())
	{
		return;
	}

	if (Enemy && Enemy->PatrolPoints.Num() > 0)
	{
		PatrolIndex = (PatrolIndex + 1) % Enemy->PatrolPoints.Num();
		PatrolDestination = Enemy->GetSpawnTransform().TransformPosition(Enemy->PatrolPoints[PatrolIndex]);
	}
	else
	{
		const FVector Home = Enemy ? Enemy->GetHomeLocation() : Self->GetActorLocation();
		const float Radius = Enemy ? Enemy->WanderRadius : 500.f;
		const FVector2D Offset = FMath::RandPointInCircle(Radius);
		PatrolDestination = Home + FVector(Offset.X, Offset.Y, 0.f);
	}
	NextPatrolMoveTime = Now() + FMath::FRandRange(2.0, 5.0);
	MoveToPoint(PatrolDestination, 80.f);
}

void AHRAIController::TickSuspicious(float DeltaSeconds)
{
	// Freeze and look first, then creep toward the disturbance.
	SetFocalPoint(LastKnownLocation, EAIFocusPriority::Gameplay);
	if (Now() - StateStartTime > 1.5 && Now() >= NextDecisionTime)
	{
		NextDecisionTime = Now() + 1.0;
		MoveToPoint(LastKnownLocation, 150.f);
	}
}

void AHRAIController::TickSearch(float DeltaSeconds)
{
	const AHRCharacterBase* Self = GetHRPawn();
	if (Now() < NextDecisionTime)
	{
		return;
	}
	NextDecisionTime = Now() + FMath::FRandRange(1.5, 3.0);
	ClearFocus(EAIFocusPriority::Gameplay);

	if (FVector::Dist2D(Self->GetActorLocation(), LastKnownLocation) > 250.f && Now() - StateStartTime < 4.0)
	{
		MoveToPoint(LastKnownLocation, 100.f);
	}
	else
	{
		// Sweep around the last known position.
		const FVector2D Offset = FMath::RandPointInCircle(700.f);
		MoveToPoint(LastKnownLocation + FVector(Offset.X, Offset.Y, 0.f), 100.f);
	}
}

bool AHRAIController::IsTargetIncapacitated() const
{
	const AHRPlayerCharacter* Player = Cast<AHRPlayerCharacter>(Target.Get());
	return Player && Player->IsDowned();
}

void AHRAIController::TickCombat(float DeltaSeconds)
{
	AHRCharacterBase* Self = GetHRPawn();
	AHREnemyCharacter* Enemy = GetEnemyPawn();
	AActor* CurrentTarget = Target.Get();
	if (!Self || !CurrentTarget || !IsValidHostile(CurrentTarget))
	{
		ReleaseToken();
		Target = nullptr;
		AwarenessValue = SearchingThreshold;
		SetAwareness(EHRAwareness::Searching);
		return;
	}

	UHRCombatComponent* Combat = Self->GetCombat();
	UHRCombatComponent* TargetCombat = CurrentTarget->FindComponentByClass<UHRCombatComponent>();
	Combat->SetCurrentTarget(CurrentTarget);
	SetFocus(CurrentTarget, EAIFocusPriority::Gameplay);

	const FVector ToTarget = CurrentTarget->GetActorLocation() - Self->GetActorLocation();
	const float Distance = ToTarget.Size2D();

	// Don't stab someone who's already down; circle and wait.
	if (IsTargetIncapacitated())
	{
		ReleaseToken();
		if (Distance < 500.f)
		{
			MoveToPoint(Self->GetActorLocation() - ToTarget.GetSafeNormal2D() * 300.f, 50.f);
		}
		return;
	}

	// Chain queued combo hits on the beat of each recovery.
	if (Combat->IsAttacking())
	{
		if (PendingComboHits > 0 && Combat->IsInRecoveryPhase() && Combat->RequestLightAttack())
		{
			--PendingComboHits;
		}
		return;
	}
	if (!Combat->CanAct())
	{
		PendingComboHits = 0;
		return;
	}

	const float Skill = Enemy ? Enemy->SkillLevel : 0.3f;
	const float Aggression = Enemy ? Enemy->Aggression : 0.5f;
	const float AttackRange = Enemy ? Enemy->AttackRange : 190.f;

	// Defensive reactions to an incoming swing.
	if (TargetCombat && TargetCombat->IsAttacking() && Distance < 380.f && Now() >= NextReactionTime)
	{
		NextReactionTime = Now() + 0.7;
		const float Roll = FMath::FRand();
		if (Roll < Skill * 0.75f)
		{
			if (TargetCombat->IsCurrentAttackPerilous() || FMath::FRand() < 0.35f)
			{
				const FVector Side = FVector::CrossProduct(ToTarget.GetSafeNormal2D(), FVector::UpVector) * (FMath::RandBool() ? 1.f : -1.f);
				Combat->RequestDodge(Side - ToTarget.GetSafeNormal2D() * 0.5f);
			}
			else
			{
				Combat->StartBlock();
				TWeakObjectPtr<UHRCombatComponent> WeakCombat = Combat;
				GetWorldTimerManager().SetTimer(BlockReleaseTimer, FTimerDelegate::CreateLambda([WeakCombat]()
				{
					if (WeakCombat.IsValid())
					{
						WeakCombat->StopBlock();
					}
				}), 0.6f, false);
			}
			return;
		}
	}

	// Ranged enemies kite and shoot.
	if (Enemy && Enemy->bRanged)
	{
		if (Distance < Enemy->PreferredRange * 0.5f)
		{
			MoveToPoint(Self->GetActorLocation() - ToTarget.GetSafeNormal2D() * 400.f, 50.f);
		}
		else if (Distance > Enemy->PreferredRange * 1.2f)
		{
			MoveToPoint(CurrentTarget->GetActorLocation(), Enemy->PreferredRange);
		}
		else if (Now() >= NextRangedTime && LineOfSightTo(CurrentTarget))
		{
			StopAllMovement();
			NextRangedTime = Now() + FMath::FRandRange(1.6f, 2.8f) * (1.5f - Aggression);
			Enemy->FireProjectileAt(CurrentTarget);
		}
		return;
	}

	// Attack tokens keep group fights readable: the rest circle and wait their turn.
	if (!bHasToken)
	{
		bHasToken = !TargetCombat || TargetCombat->RequestAttackToken(Self);
		if (bHasToken)
		{
			TokenAcquiredTime = Now();
		}
	}
	if (!bHasToken)
	{
		if (Now() >= NextDecisionTime)
		{
			NextDecisionTime = Now() + 0.6;
			if (FMath::FRand() < 0.15f)
			{
				StrafeSign *= -1.f;
			}
			const FVector FromTarget = (-ToTarget).GetSafeNormal2D().RotateAngleAxis(25.f * StrafeSign, FVector::UpVector);
			MoveToPoint(CurrentTarget->GetActorLocation() + FromTarget * 480.f, 60.f);
		}
		return;
	}

	// Rotate tokens so everyone gets a turn.
	if (Now() - TokenAcquiredTime > 6.0 && PendingComboHits == 0)
	{
		ReleaseToken();
		NextDecisionTime = Now() + 1.5;
		return;
	}

	if (Distance > AttackRange)
	{
		if (Now() >= NextDecisionTime || bDirectSteering)
		{
			NextDecisionTime = Now() + 0.25;
			MoveToPoint(CurrentTarget->GetActorLocation(), AttackRange * 0.6f);
		}
		return;
	}

	StopAllMovement();
	if (Now() < NextAttackTime)
	{
		return;
	}

	const float Roll = FMath::FRand();
	const float Perilous = Enemy ? Enemy->PerilousChance : 0.f;
	if (Enemy && Roll < Perilous)
	{
		Combat->PerformAttack(Enemy->PerilousAttack, true);
	}
	else if (Roll < Perilous + 0.65f)
	{
		const int32 MaxHits = Enemy ? Enemy->MaxComboHits : 3;
		PendingComboHits = FMath::RandRange(1, MaxHits) - 1;
		Combat->RequestLightAttack();
	}
	else
	{
		Combat->RequestHeavyAttack();
	}
	NextAttackTime = Now() + FMath::FRandRange(0.8f, 2.2f) * (1.4f - Aggression);
}

// --- Movement ----------------------------------------------------------------------------------

void AHRAIController::MoveToPoint(const FVector& Destination, float Acceptance)
{
	const EPathFollowingRequestResult::Type Result = MoveToLocation(Destination, Acceptance, true, true, false, true);
	if (Result == EPathFollowingRequestResult::Failed)
	{
		// No navmesh (or no path): steer straight at it.
		bDirectSteering = true;
		SteerDestination = Destination;
		SteerAcceptance = Acceptance;
	}
	else
	{
		bDirectSteering = false;
	}
}

void AHRAIController::TickDirectSteering()
{
	APawn* ControlledPawn = GetPawn();
	if (!bDirectSteering || !ControlledPawn)
	{
		return;
	}
	FVector Direction = SteerDestination - ControlledPawn->GetActorLocation();
	Direction.Z = 0.f;
	if (Direction.Size() <= SteerAcceptance)
	{
		bDirectSteering = false;
		return;
	}
	ControlledPawn->AddMovementInput(Direction.GetSafeNormal(), 1.f);
}

void AHRAIController::StopAllMovement()
{
	bDirectSteering = false;
	StopMovement();
}

void AHRAIController::ReleaseToken()
{
	if (!bHasToken)
	{
		return;
	}
	bHasToken = false;
	if (AActor* CurrentTarget = Target.Get())
	{
		if (UHRCombatComponent* TargetCombat = CurrentTarget->FindComponentByClass<UHRCombatComponent>())
		{
			TargetCombat->ReleaseAttackToken(GetPawn());
		}
	}
}

void AHRAIController::SetCombatFacing(bool bFacingTarget)
{
	ACharacter* HRCharacter = Cast<ACharacter>(GetPawn());
	if (!HRCharacter)
	{
		return;
	}
	HRCharacter->GetCharacterMovement()->bOrientRotationToMovement = !bFacingTarget;
	HRCharacter->GetCharacterMovement()->bUseControllerDesiredRotation = bFacingTarget;
}
