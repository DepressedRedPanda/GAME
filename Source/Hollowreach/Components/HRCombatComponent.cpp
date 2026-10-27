#include "Components/HRCombatComponent.h"
#include "SharedArtLibrary.h"

#include "Characters/HRCharacterBase.h"
#include "Characters/HRBossCharacter.h"
#include "Components/HRAttributeComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/HRInteractable.h"
#include "Systems/HRFactionSubsystem.h"
#include "TimerManager.h"

namespace
{
	constexpr int32 MaxFlowStacks = 5;
	constexpr float FlowDamagePerStack = 0.06f;
}

UHRCombatComponent::UHRCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Bare-handed / generic weapon used by anything without equipment.
	DefaultWeapon.BaseDamage = 12.f;
	DefaultWeapon.BasePosture = 10.f;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FHRAttackStep Step;
		Step.DamageMultiplier = Index == 2 ? 1.3f : 1.f;
		Step.PostureMultiplier = Index == 2 ? 1.4f : 1.f;
		DefaultWeapon.LightCombo.Add(Step);
	}
	DefaultWeapon.HeavyAttack.DamageMultiplier = 1.8f;
	DefaultWeapon.HeavyAttack.PostureMultiplier = 2.f;
	DefaultWeapon.HeavyAttack.StaminaCost = 24.f;
	DefaultWeapon.HeavyAttack.Windup = 0.5f;
	DefaultWeapon.HeavyAttack.Recovery = 0.5f;
	DefaultWeapon.HeavyAttack.LungeSpeed = 600.f;
}

void UHRCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Weapon.LightCombo.Num() == 0)
	{
		ResetWeaponToDefault();
	}
	if (UHRAttributeComponent* Attributes = GetAttributes())
	{
		Attributes->OnPostureBroken.AddDynamic(this, &UHRCombatComponent::HandlePostureBroken);
	}
}

void UHRCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Flow fades if you stop fighting.
	if (FlowStacks > 0 && State != EHRCombatState::Attacking && Now() - LastAttackEndTime > 2.5)
	{
		FlowStacks = 0;
	}

	// Drop attack tokens held by actors that died or vanished.
	TokenHolders.RemoveAll([](const TWeakObjectPtr<AActor>& Holder)
	{
		const AHRCharacterBase* Character = Cast<AHRCharacterBase>(Holder.Get());
		return !Character || Character->IsDead();
	});
}

// --- Helpers -----------------------------------------------------------------------------------

ACharacter* UHRCombatComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

UHRAttributeComponent* UHRCombatComponent::GetAttributes() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UHRAttributeComponent>() : nullptr;
}

double UHRCombatComponent::Now() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void UHRCombatComponent::SetState(EHRCombatState NewState)
{
	if (State == NewState)
	{
		return;
	}
	const EHRCombatState OldState = State;
	State = NewState;

	if (UHRAttributeComponent* Attributes = GetAttributes())
	{
		Attributes->SetGuarding(NewState == EHRCombatState::Blocking);
	}
	OnStateChanged.Broadcast(OldState, NewState);
}

void UHRCombatComponent::ClearTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTimer);
		World->GetTimerManager().ClearTimer(StateTimer);
	}
	Phase = EHRAttackPhase::None;
}

void UHRCombatComponent::EndStateTimer()
{
	if (State == EHRCombatState::Dead || State == EHRCombatState::Downed)
	{
		return;
	}
	if (State == EHRCombatState::PostureBroken)
	{
		// Sekiro: posture comes back half-full after a break.
		if (UHRAttributeComponent* Attributes = GetAttributes())
		{
			Attributes->ResetPosture(0.5f);
		}
	}
	SetState(bBlockHeld ? EHRCombatState::Blocking : EHRCombatState::Idle);
	if (State == EHRCombatState::Blocking)
	{
		// Re-entering a held guard does not count as a fresh deflect attempt.
		BlockStartTime = -100.0;
	}
}

bool UHRCombatComponent::CanAct() const
{
	return State == EHRCombatState::Idle || State == EHRCombatState::Blocking;
}

bool UHRCombatComponent::IsInRiposteWindow() const
{
	return Now() <= RiposteUntil;
}

float UHRCombatComponent::GetAttackProgress() const
{
	if (State != EHRCombatState::Attacking)
	{
		return 0.f;
	}
	const float Total = FMath::Max(0.01f, CurrentStep.GetTotalTime());
	return FMath::Clamp(static_cast<float>(Now() - AttackStartTime) / Total, 0.f, 1.f);
}

float UHRCombatComponent::GetParryWindow() const
{
	const UHRAttributeComponent* Attributes = GetAttributes();
	return Attributes ? FMath::Max(0.05f, Attributes->GetStat(EHRStat::ParryWindow)) : 0.18f;
}

// --- Weapon ------------------------------------------------------------------------------------

void UHRCombatComponent::SetWeapon(const FHRWeaponData& InWeapon, int32 UpgradeLevel)
{
	Weapon = InWeapon;
	if (Weapon.LightCombo.Num() == 0)
	{
		Weapon.LightCombo = DefaultWeapon.LightCombo;
	}
	WeaponUpgradeMultiplier = 1.f + 0.12f * FMath::Max(0, UpgradeLevel);
	ComboIndex = 0;
}

void UHRCombatComponent::ResetWeaponToDefault()
{
	SetWeapon(DefaultWeapon, 0);
}

// --- Offense -----------------------------------------------------------------------------------

bool UHRCombatComponent::RequestLightAttack()
{
	if (const AHRBossCharacter* Boss = Cast<AHRBossCharacter>(GetOwner())) { if (!Boss->IsCampaignUnlocked()) return false; }
	if (State == EHRCombatState::Attacking)
	{
		if (Phase == EHRAttackPhase::Recovery)
		{
			// On-beat input: late in the recovery window builds Flow and chains immediately.
			const float RecoveryElapsed = static_cast<float>(Now() - RecoveryStartTime);
			if (RecoveryElapsed >= CurrentStep.Recovery * 0.35f)
			{
				FlowStacks = FMath::Min(MaxFlowStacks, FlowStacks + 1);
			}
			ComboIndex = (ComboIndex + 1) % FMath::Max(1, Weapon.LightCombo.Num());
			ClearTimers();
			BeginAttackStep(Weapon.LightCombo[ComboIndex], false);
			return true;
		}
		// Too early: buffered, but mashing costs your Flow.
		if (bBufferedLight)
		{
			FlowStacks = 0;
		}
		bBufferedLight = true;
		return true;
	}

	if (!CanAct() || Weapon.LightCombo.Num() == 0)
	{
		return false;
	}

	if (Now() - LastAttackEndTime > ComboResetTime)
	{
		ComboIndex = 0;
	}
	else
	{
		ComboIndex = (ComboIndex + 1) % Weapon.LightCombo.Num();
	}
	BeginAttackStep(Weapon.LightCombo[ComboIndex], false);
	return true;
}

bool UHRCombatComponent::RequestHeavyAttack()
{
	if (const AHRBossCharacter* Boss = Cast<AHRBossCharacter>(GetOwner())) { if (!Boss->IsCampaignUnlocked()) return false; }
	if (State == EHRCombatState::Attacking)
	{
		if (Phase == EHRAttackPhase::Recovery)
		{
			ClearTimers();
			BeginAttackStep(Weapon.HeavyAttack, true);
			return true;
		}
		bBufferedHeavy = true;
		return true;
	}
	if (!CanAct())
	{
		return false;
	}
	BeginAttackStep(Weapon.HeavyAttack, true);
	return true;
}

bool UHRCombatComponent::PerformAttack(const FHRAttackStep& Step, bool bHeavy)
{
	if (const AHRBossCharacter* Boss = Cast<AHRBossCharacter>(GetOwner())) { if (!Boss->IsCampaignUnlocked()) return false; }
	if (!CanAct())
	{
		return false;
	}
	BeginAttackStep(Step, bHeavy);
	return true;
}

void UHRCombatComponent::BeginAttackStep(const FHRAttackStep& Step, bool bHeavy)
{
	UHRAttributeComponent* Attributes = GetAttributes();
	if (Attributes && !Attributes->ConsumeStamina(Step.StaminaCost))
	{
		// Out of stamina: the swing fails and you are briefly winded.
		SetState(EHRCombatState::Idle);
		return;
	}

	bBlockHeld = false;
	CurrentStep = Step;
	bCurrentHeavy = bHeavy;
	bBufferedLight = false;
	bBufferedHeavy = false;
	HitThisSwing.Reset();
	AttackStartTime = Now();
	Phase = EHRAttackPhase::Windup;
	SetState(EHRCombatState::Attacking);
	FaceTarget();

	if (ACharacter* Character = GetOwnerCharacter())
	{
		UAnimMontage* VisualMontage = Step.Montage;
		if (!VisualMontage)
		{
			const AHRCharacterBase* Combatant = Cast<AHRCharacterBase>(Character);
			const FName Action = bHeavy ? FName(TEXT("Heavy")) : FName(*FString::Printf(TEXT("Light%d"), ComboIndex));
			if (Combatant) VisualMontage = USharedArtLibrary::FindMontage(Combatant->SharedAppearanceRole, Action);
		}
		if (VisualMontage && Character->GetMesh() && Character->GetMesh()->GetAnimInstance())
		{
			Character->GetMesh()->GetAnimInstance()->Montage_Play(VisualMontage);
		}
	}

	OnAttackStarted.Broadcast(ComboIndex, Step.bUnblockable);
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UHRCombatComponent::HandleAttackActive, FMath::Max(0.01f, Step.Windup), false);
}

void UHRCombatComponent::HandleAttackActive()
{
	Phase = EHRAttackPhase::Active;
	if (ACharacter* Character = GetOwnerCharacter())
	{
		if (CurrentStep.LungeSpeed > 0.f && Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsMovingOnGround())
		{
			Character->LaunchCharacter(Character->GetActorForwardVector() * CurrentStep.LungeSpeed, true, false);
		}
	}
	PerformHitSweep();
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UHRCombatComponent::HandleAttackRecovery, FMath::Max(0.01f, CurrentStep.ActiveTime), false);
}

void UHRCombatComponent::HandleAttackRecovery()
{
	// Second sweep at the end of the swing arc catches targets that moved into it.
	PerformHitSweep();
	Phase = EHRAttackPhase::Recovery;
	RecoveryStartTime = Now();

	if (bBufferedHeavy)
	{
		BeginAttackStep(Weapon.HeavyAttack, true);
		return;
	}
	if (bBufferedLight && Weapon.LightCombo.Num() > 0)
	{
		ComboIndex = (ComboIndex + 1) % Weapon.LightCombo.Num();
		BeginAttackStep(Weapon.LightCombo[ComboIndex], false);
		return;
	}
	GetWorld()->GetTimerManager().SetTimer(PhaseTimer, this, &UHRCombatComponent::HandleAttackFinished, FMath::Max(0.01f, CurrentStep.Recovery), false);
}

void UHRCombatComponent::HandleAttackFinished()
{
	Phase = EHRAttackPhase::None;
	LastAttackEndTime = Now();
	bCurrentHeavy = false;
	SetState(EHRCombatState::Idle);
}

void UHRCombatComponent::FaceTarget()
{
	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	AActor* Target = CurrentTarget.Get();
	if (!Target)
	{
		// Soft lock: snap to the closest hostile in a forward cone.
		float BestScore = 0.f;
		TArray<FOverlapResult> Overlaps;
		FCollisionObjectQueryParams ObjectParams(ECC_Pawn);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(HRSoftLock), false, Character);
		GetWorld()->OverlapMultiByObjectType(Overlaps, Character->GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(400.f), Params);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AHRCharacterBase* Other = Cast<AHRCharacterBase>(Overlap.GetActor());
			if (!Other || Other->IsDead() || !UHRFactionSubsystem::AreHostile(Character, Other))
			{
				continue;
			}
			const FVector ToOther = (Other->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
			const float Score = FVector::DotProduct(Character->GetActorForwardVector(), ToOther);
			if (Score > 0.3f && Score > BestScore)
			{
				BestScore = Score;
				Target = Other;
			}
		}
	}

	if (Target)
	{
		FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
		ToTarget.Z = 0.f;
		if (!ToTarget.IsNearlyZero())
		{
			Character->SetActorRotation(ToTarget.Rotation());
		}
	}
}

FHRDamageInfo UHRCombatComponent::BuildDamage(AActor* Target) const
{
	const AHRCharacterBase* OwnerCharacter = Cast<AHRCharacterBase>(GetOwner());
	const UHRAttributeComponent* Attributes = GetAttributes();

	const bool bPhysical = Weapon.DamageType == EHRDamageType::Physical;
	const float Power = Attributes ? Attributes->GetStat(bPhysical ? EHRStat::PhysicalPower : EHRStat::ArcanePower) : 0.f;
	const float Scaling = OwnerCharacter ? OwnerCharacter->GetAttributeScaling(Weapon.ScalingAttribute) * Weapon.ScalingFactor : 0.f;

	bool bBackstab = false;
	const float Positional = ComputePositionalMultiplier(Target, bBackstab);
	const float Flow = 1.f + FlowDamagePerStack * FlowStacks;
	const bool bRiposte = IsInRiposteWindow();
	const float CritChance = (Attributes ? Attributes->GetStat(EHRStat::CritChance) : 0.05f) + (bBackstab ? 0.25f : 0.f);
	const bool bCritical = FMath::FRand() < CritChance;

	FHRDamageInfo Damage;
	Damage.Source = GetOwner();
	Damage.Type = Weapon.DamageType;
	Damage.bHeavy = bCurrentHeavy;
	Damage.bUnblockable = CurrentStep.bUnblockable;
	Damage.bCritical = bCritical;
	Damage.Amount = (Weapon.BaseDamage * WeaponUpgradeMultiplier * (1.f + Scaling) + Power)
		* CurrentStep.DamageMultiplier * Positional * Flow * (bRiposte ? 1.5f : 1.f) * (bCritical ? 1.6f : 1.f) * OutgoingDamageMultiplier;

	const float PostureStat = Attributes ? Attributes->GetStat(EHRStat::PostureDamage) : 1.f;
	Damage.PostureDamage = Weapon.BasePosture * CurrentStep.PostureMultiplier * PostureStat * (bBackstab ? 1.5f : 1.f) * (bRiposte ? 1.6f : 1.f);
	if (Target)
	{
		Damage.HitLocation = Target->GetActorLocation();
	}
	return Damage;
}

float UHRCombatComponent::ComputePositionalMultiplier(const AActor* Target, bool& bOutBackstab) const
{
	bOutBackstab = false;
	const AActor* Owner = GetOwner();
	if (!Target || !Owner)
	{
		return 1.f;
	}

	float Multiplier = 1.f;
	const FVector TargetToOwner = (Owner->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();

	// Backstab: attacking from the rear cone.
	if (FVector::DotProduct(Target->GetActorForwardVector(), TargetToOwner) < -0.5f)
	{
		bOutBackstab = true;
		Multiplier *= 1.5f;
	}

	// High ground.
	if (Owner->GetActorLocation().Z - Target->GetActorLocation().Z > 80.f)
	{
		Multiplier *= 1.15f;
	}

	// Flanking: the target is focused on someone else.
	if (const UHRCombatComponent* TargetCombat = Target->FindComponentByClass<UHRCombatComponent>())
	{
		const AActor* TheirTarget = TargetCombat->GetCurrentTarget();
		if (TheirTarget && TheirTarget != Owner)
		{
			Multiplier *= 1.1f;
		}
	}
	return Multiplier;
}

void UHRCombatComponent::PerformHitSweep()
{
	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	const FVector Forward = Character->GetActorForwardVector();
	const FVector Start = Character->GetActorLocation() + Forward * 30.f;
	const FVector End = Character->GetActorLocation() + Forward * CurrentStep.Reach;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRMeleeSweep), false, Character);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(CurrentStep.Radius), Params);

	if (bDebugDraw)
	{
		DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f + CurrentStep.Radius, CurrentStep.Radius,
			FRotationMatrix::MakeFromZ(End - Start).ToQuat(), CurrentStep.bUnblockable ? FColor::Red : FColor::Orange, false, 0.3f);
	}

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == Character)
		{
			continue;
		}
		const bool bAlreadyHit = HitThisSwing.ContainsByPredicate([HitActor](const TWeakObjectPtr<AActor>& Ptr) { return Ptr.Get() == HitActor; });
		if (bAlreadyHit)
		{
			continue;
		}
		HitThisSwing.Add(HitActor);

		if (UHRCombatComponent* Victim = HitActor->FindComponentByClass<UHRCombatComponent>())
		{
			if (UHRFactionSubsystem::AreFriendly(Character, HitActor))
			{
				continue;
			}
			const FHRDamageInfo Damage = BuildDamage(HitActor);
			const EHRHitResponse Response = Victim->ReceiveHit(Damage);
			OnHitLanded.Broadcast(HitActor, Response, Damage.Amount);
			if (Response != EHRHitResponse::Ignored && Response != EHRHitResponse::Evaded)
			{
				USharedArtLibrary::PlayCue(this, Response == EHRHitResponse::Deflected ? TEXT("Combat.Deflect") : TEXT("Combat.Hit"), Hit.ImpactPoint);
			}
			UHRFactionSubsystem::NotifyAttack(Character, HitActor);

			// Hits land with a little hit-stop for weight.
			if (Response == EHRHitResponse::Hit || Response == EHRHitResponse::Deathblow || Response == EHRHitResponse::Killed)
			{
				Character->CustomTimeDilation = 0.05f;
				TWeakObjectPtr<ACharacter> WeakCharacter = Character;
				FTimerHandle HitStop;
				GetWorld()->GetTimerManager().SetTimer(HitStop, FTimerDelegate::CreateLambda([WeakCharacter]()
				{
					if (WeakCharacter.IsValid())
					{
						WeakCharacter->CustomTimeDilation = 1.f;
					}
				}), 0.06f, false);
			}
		}
		else
		{
			if (UPrimitiveComponent* Component = Hit.GetComponent())
			{
				if (Component->IsSimulatingPhysics())
				{
					Component->AddImpulseAtLocation(Forward * PhysicsImpulse * (bCurrentHeavy ? 2.f : 1.f), Hit.ImpactPoint);
				}
			}
			if (IHRElementalReactive* Reactive = Cast<IHRElementalReactive>(HitActor))
			{
				Reactive->ReactToElement(Weapon.DamageType, Character);
			}
		}
	}
}

// --- Defense -----------------------------------------------------------------------------------

void UHRCombatComponent::StartBlock()
{
	bBlockHeld = true;
	if (State == EHRCombatState::Attacking && Phase == EHRAttackPhase::Recovery)
	{
		// Deflect-cancel out of recovery.
		ClearTimers();
		LastAttackEndTime = Now();
		SetState(EHRCombatState::Idle);
	}
	if (State != EHRCombatState::Idle)
	{
		return;
	}
	BlockStartTime = Now();
	SetState(EHRCombatState::Blocking);
}

void UHRCombatComponent::StopBlock()
{
	bBlockHeld = false;
	if (State == EHRCombatState::Blocking)
	{
		SetState(EHRCombatState::Idle);
	}
}

bool UHRCombatComponent::RequestDodge(FVector WorldDirection)
{
	const bool bCanCancel = State == EHRCombatState::Attacking && Phase == EHRAttackPhase::Recovery;
	if (!CanAct() && !bCanCancel)
	{
		return false;
	}
	UHRAttributeComponent* Attributes = GetAttributes();
	if (Attributes && !Attributes->ConsumeStamina(DodgeStaminaCost))
	{
		return false;
	}
	ACharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return false;
	}

	ClearTimers();
	FVector Direction = WorldDirection;
	Direction.Z = 0.f;
	Direction = Direction.IsNearlyZero() ? -Character->GetActorForwardVector() : Direction.GetSafeNormal();

	Character->LaunchCharacter(Direction * DodgeSpeed, true, false);
	DodgeStartTime = Now();
	SetState(EHRCombatState::Dodging);
	GrantInvulnerability(IFrameDuration);
	GetWorld()->GetTimerManager().SetTimer(StateTimer, this, &UHRCombatComponent::EndStateTimer, DodgeDuration, false);
	return true;
}

void UHRCombatComponent::GrantInvulnerability(float Duration)
{
	bIFrames = true;
	TWeakObjectPtr<UHRCombatComponent> WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(IFrameTimer, FTimerDelegate::CreateLambda([WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->bIFrames = false;
		}
	}), FMath::Max(0.01f, Duration), false);
}

bool UHRCombatComponent::BeginBusy(float Duration)
{
	if (!CanAct())
	{
		return false;
	}
	ClearTimers();
	SetState(EHRCombatState::Busy);
	GetWorld()->GetTimerManager().SetTimer(StateTimer, this, &UHRCombatComponent::EndStateTimer, FMath::Max(0.01f, Duration), false);
	return true;
}

void UHRCombatComponent::CancelActions()
{
	ClearTimers();
	bBufferedLight = false;
	bBufferedHeavy = false;
	bCurrentHeavy = false;
	if (State == EHRCombatState::Attacking || State == EHRCombatState::Busy || State == EHRCombatState::Blocking)
	{
		SetState(EHRCombatState::Idle);
	}
}

EHRHitResponse UHRCombatComponent::ReceiveHit(const FHRDamageInfo& InDamage)
{
	if (const AHRBossCharacter* Boss = Cast<AHRBossCharacter>(GetOwner()))
	{
		if (!Boss->IsCampaignUnlocked()) return EHRHitResponse::Ignored;
	}
	UHRAttributeComponent* Attributes = GetAttributes();
	AActor* Owner = GetOwner();
	if (!Attributes || !Owner || !Attributes->IsAlive() || State == EHRCombatState::Dead)
	{
		return EHRHitResponse::Ignored;
	}

	AActor* Attacker = InDamage.Source;
	FHRDamageInfo Damage = InDamage;
	EHRHitResponse Response = EHRHitResponse::Hit;

	// Executions: takedowns and strikes against a broken posture.
	if (Damage.bDeathblow || State == EHRCombatState::PostureBroken)
	{
		Damage.bDeathblow = true;
		Damage.Amount = FMath::Max(Damage.Amount * 2.f, Attributes->GetMaxHealth() * DeathblowHealthFraction + 1.f);
		Damage.PostureDamage = 0.f;
		ClearTimers();
		Attributes->ResetPosture();
		Attributes->ApplyDamage(Damage);
		if (Attributes->IsAlive())
		{
			SetState(EHRCombatState::Idle);
		}
		OnHitReceived.Broadcast(Attacker, EHRHitResponse::Deathblow);
		return EHRHitResponse::Deathblow;
	}

	if (bIFrames)
	{
		if (State == EHRCombatState::Dodging && Now() - DodgeStartTime <= PerfectDodgeWindow + 0.05)
		{
			OnPerfectDodge.Broadcast();
		}
		OnHitReceived.Broadcast(Attacker, EHRHitResponse::Evaded);
		return EHRHitResponse::Evaded;
	}

	bool bFacingAttacker = true;
	if (Attacker)
	{
		const FVector ToAttacker = (Attacker->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
		bFacingAttacker = FVector::DotProduct(Owner->GetActorForwardVector(), ToAttacker) > 0.2f;
	}

	if (State == EHRCombatState::Blocking && bFacingAttacker && !Damage.bUnblockable)
	{
		if (Now() - BlockStartTime <= GetParryWindow())
		{
			// Deflect: tiny posture cost to us, heavy posture cost to the attacker.
			Attributes->ApplyPostureDamage(Damage.PostureDamage * 0.2f);
			if (Attacker)
			{
				if (UHRCombatComponent* AttackerCombat = Attacker->FindComponentByClass<UHRCombatComponent>())
				{
					AttackerCombat->NotifyDeflectedBy(Owner, Damage);
				}
			}
			RiposteUntil = Now() + RiposteWindow;
			OnDeflectSuccess.Broadcast();
			OnHitReceived.Broadcast(Attacker, EHRHitResponse::Deflected);
			return EHRHitResponse::Deflected;
		}

		// Regular block: stamina, posture and a little chip damage.
		const bool bHeldGuard = Attributes->ConsumeStamina(Damage.Amount * BlockStaminaMultiplier);
		Attributes->ApplyPostureDamage(Damage.PostureDamage * 0.75f);
		FHRDamageInfo Chip = Damage;
		Chip.Amount *= Damage.bHeavy ? 0.25f : 0.1f;
		Chip.PostureDamage = 0.f;
		Attributes->ApplyDamage(Chip);

		if (State == EHRCombatState::PostureBroken)
		{
			Response = EHRHitResponse::GuardBroken;
		}
		else if (!bHeldGuard || Attributes->IsExhausted())
		{
			EnterStagger(GuardBreakDuration);
			Response = EHRHitResponse::GuardBroken;
		}
		else
		{
			Response = EHRHitResponse::Blocked;
		}
		OnHitReceived.Broadcast(Attacker, Response);
		return Response;
	}

	// Clean hit.
	Attributes->ApplyDamage(Damage);
	if (!Attributes->IsAlive())
	{
		OnHitReceived.Broadcast(Attacker, EHRHitResponse::Killed);
		return EHRHitResponse::Killed;
	}

	if (State != EHRCombatState::PostureBroken)
	{
		const bool bHyperArmor = State == EHRCombatState::Attacking && bCurrentHeavy && bHyperArmorOnHeavy;
		if (!bHyperArmor && Damage.Amount >= PoiseThreshold)
		{
			EnterStagger(Damage.bHeavy ? StaggerDuration * 1.6f : StaggerDuration);
		}
	}
	OnHitReceived.Broadcast(Attacker, Response);
	return Response;
}

void UHRCombatComponent::NotifyDeflectedBy(AActor* Defender, const FHRDamageInfo& Damage)
{
	if (UHRAttributeComponent* Attributes = GetAttributes())
	{
		Attributes->ApplyPostureDamage(Damage.PostureDamage * 0.6f + 6.f);
	}
	if (bRecoilWhenDeflected && State == EHRCombatState::Attacking)
	{
		EnterStagger(0.3f);
	}
}

void UHRCombatComponent::EnterStagger(float Duration)
{
	if (State == EHRCombatState::Dead || State == EHRCombatState::Downed || State == EHRCombatState::PostureBroken)
	{
		return;
	}
	ClearTimers();
	bBufferedLight = false;
	bBufferedHeavy = false;
	SetState(EHRCombatState::Staggered);
	GetWorld()->GetTimerManager().SetTimer(StateTimer, this, &UHRCombatComponent::EndStateTimer, FMath::Max(0.05f, Duration), false);
}

void UHRCombatComponent::EnterPostureBroken()
{
	if (State == EHRCombatState::Dead || State == EHRCombatState::Downed)
	{
		return;
	}
	ClearTimers();
	SetState(EHRCombatState::PostureBroken);
	OnPostureBroken.Broadcast();
	GetWorld()->GetTimerManager().SetTimer(StateTimer, this, &UHRCombatComponent::EndStateTimer, PostureBrokenDuration, false);
}

void UHRCombatComponent::HandlePostureBroken(UHRAttributeComponent* Attributes)
{
	EnterPostureBroken();
}

void UHRCombatComponent::SetDead()
{
	ClearTimers();
	TokenHolders.Reset();
	SetState(EHRCombatState::Dead);
}

void UHRCombatComponent::SetDowned(bool bDowned)
{
	ClearTimers();
	SetState(bDowned ? EHRCombatState::Downed : EHRCombatState::Idle);
}

void UHRCombatComponent::ResetCombatState()
{
	ClearTimers();
	bIFrames = false;
	bBlockHeld = false;
	FlowStacks = 0;
	ComboIndex = 0;
	TokenHolders.Reset();
	CurrentTarget = nullptr;
	SetState(EHRCombatState::Idle);
}

// --- Stealth takedowns -------------------------------------------------------------------------

AActor* UHRCombatComponent::FindTakedownTarget() const
{
	const ACharacter* Character = GetOwnerCharacter();
	if (!Character || !CanAct())
	{
		return nullptr;
	}

	const bool bFalling = Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling();
	const float Range = bFalling ? TakedownRange * 2.f : TakedownRange;

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRTakedown), false, Character);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Character->GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Range), Params);

	AActor* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AHRCharacterBase* Victim = Cast<AHRCharacterBase>(Overlap.GetActor());
		if (!Victim || Victim->IsDead() || !UHRFactionSubsystem::AreHostile(Character, Victim))
		{
			continue;
		}
		const UHRCombatComponent* VictimCombat = Victim->GetCombat();
		if (!VictimCombat || !VictimCombat->bCanBeTakenDown)
		{
			continue;
		}

		const EHRAwareness Awareness = Victim->GetAwarenessTowards(Character);
		if (Awareness == EHRAwareness::Alerted)
		{
			continue;
		}

		const FVector VictimToMe = (Character->GetActorLocation() - Victim->GetActorLocation()).GetSafeNormal2D();
		const bool bBehind = FVector::DotProduct(Victim->GetActorForwardVector(), VictimToMe) < -0.25f;
		const bool bInFrontOfMe = FVector::DotProduct(Character->GetActorForwardVector(), -VictimToMe) > 0.4f;
		const bool bFromAbove = bFalling && Character->GetActorLocation().Z > Victim->GetActorLocation().Z + 100.f;

		if (!(bFromAbove || (bInFrontOfMe && (bBehind || Awareness == EHRAwareness::Unaware))))
		{
			continue;
		}

		const float Distance = FVector::Dist(Character->GetActorLocation(), Victim->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Victim;
		}
	}
	return Best;
}

bool UHRCombatComponent::TryStealthTakedown()
{
	AActor* Victim = FindTakedownTarget();
	ACharacter* Character = GetOwnerCharacter();
	if (!Victim || !Character)
	{
		return false;
	}

	FVector ToVictim = Victim->GetActorLocation() - Character->GetActorLocation();
	ToVictim.Z = 0.f;
	Character->SetActorRotation(ToVictim.Rotation());

	FHRDamageInfo Damage;
	Damage.Source = Character;
	Damage.bDeathblow = true;
	Damage.Amount = Weapon.BaseDamage * 3.f;
	if (UHRCombatComponent* VictimCombat = Victim->FindComponentByClass<UHRCombatComponent>())
	{
		VictimCombat->ReceiveHit(Damage);
	}

	BeginBusy(0.7f);
	GrantInvulnerability(0.7f);
	OnTakedown.Broadcast();
	OnHitLanded.Broadcast(Victim, EHRHitResponse::Deathblow, Damage.Amount);
	return true;
}

// --- Attack tokens -----------------------------------------------------------------------------

bool UHRCombatComponent::RequestAttackToken(AActor* Requester)
{
	if (!Requester)
	{
		return false;
	}
	const bool bHasToken = TokenHolders.ContainsByPredicate([Requester](const TWeakObjectPtr<AActor>& Ptr) { return Ptr.Get() == Requester; });
	if (bHasToken)
	{
		return true;
	}
	if (TokenHolders.Num() >= MaxAttackTokens)
	{
		return false;
	}
	TokenHolders.Add(Requester);
	return true;
}

void UHRCombatComponent::ReleaseAttackToken(AActor* Requester)
{
	TokenHolders.RemoveAll([Requester](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid() || Ptr.Get() == Requester; });
}
