#include "Abilities/HRAbilities.h"

#include "Characters/HRCharacterBase.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Core/HRInteractable.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Systems/HRFactionSubsystem.h"
#include "World/HRProjectile.h"

#define LOCTEXT_NAMESPACE "HRAbilities"

namespace
{
	FVector GetAimDirection(const AHRCharacterBase* Owner)
	{
		if (const UHRCombatComponent* Combat = Owner->GetCombat())
		{
			if (const AActor* Target = Combat->GetCurrentTarget())
			{
				return (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
			}
		}
		if (Owner->IsPlayerControlled())
		{
			FVector Aim = Owner->GetBaseAimRotation().Vector();
			// Keep spells roughly level so they don't fly into the sky.
			Aim.Z = FMath::Clamp(Aim.Z, -0.3f, 0.3f);
			return Aim.GetSafeNormal();
		}
		return Owner->GetActorForwardVector();
	}
}

// --- Firebolt ----------------------------------------------------------------------------------

UHRAbility_Firebolt::UHRAbility_Firebolt()
{
	AbilityId = TEXT("Ability.Firebolt");
	DisplayName = LOCTEXT("FireboltName", "Firebolt");
	Description = LOCTEXT("FireboltDesc", "Hurl a bolt of flame. Ignites braziers and volatile barrels.");
	FocusCost = 12.f;
	Cooldown = 1.2f;
	CastTime = 0.35f;
}

void UHRAbility_Firebolt::Activate(AHRCharacterBase* Owner)
{
	Super::Activate(Owner);
	const UHRAttributeComponent* Attributes = Owner->GetAttributes();

	FHRDamageInfo Damage;
	Damage.Source = Owner;
	Damage.Type = EHRDamageType::Fire;
	Damage.Amount = 22.f + (Attributes ? Attributes->GetStat(EHRStat::ArcanePower) * 1.3f : 0.f);
	Damage.PostureDamage = 14.f;

	const FVector Direction = GetAimDirection(Owner);
	const FVector Start = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 70.f + FVector(0.f, 0.f, 40.f);
	AHRProjectile::Launch(Owner, Start, Direction, Damage, 2600.f, 0.f);
}

// --- Kinetic Push ------------------------------------------------------------------------------

UHRAbility_KineticPush::UHRAbility_KineticPush()
{
	AbilityId = TEXT("Ability.KineticPush");
	DisplayName = LOCTEXT("PushName", "Kinetic Push");
	Description = LOCTEXT("PushDesc", "A wave of force that hurls objects and staggers foes.");
	FocusCost = 18.f;
	Cooldown = 3.f;
}

void UHRAbility_KineticPush::Activate(AHRCharacterBase* Owner)
{
	Super::Activate(Owner);
	UWorld* World = Owner->GetWorld();
	const FVector Origin = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector();

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRKineticPush), false, Owner);
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Range), Params);

	TSet<AActor*> Processed;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		UPrimitiveComponent* Component = Overlap.GetComponent();
		if (!Actor || !Component)
		{
			continue;
		}
		const FVector ToActor = (Actor->GetActorLocation() - Origin);
		const FVector Direction = ToActor.GetSafeNormal2D();
		if (FVector::DotProduct(Forward, Direction) < 0.35f)
		{
			continue; // Cone in front only.
		}
		const float Falloff = 1.f - FMath::Clamp(ToActor.Size() / Range, 0.f, 0.7f);

		if (Component->IsSimulatingPhysics())
		{
			Component->AddImpulse((Direction + FVector(0.f, 0.f, 0.35f)) * Force * Falloff, NAME_None, true);
		}
		if (Processed.Contains(Actor))
		{
			continue;
		}
		Processed.Add(Actor);

		if (AHRCharacterBase* Character = Cast<AHRCharacterBase>(Actor))
		{
			if (Character->IsDead() || UHRFactionSubsystem::AreFriendly(Owner, Character))
			{
				continue;
			}
			Character->LaunchCharacter((Direction * 900.f + FVector(0.f, 0.f, 350.f)) * Falloff, true, true);
			if (UHRAttributeComponent* Attributes = Character->GetAttributes())
			{
				Attributes->ApplyPostureDamage(30.f * Falloff);
			}
			if (UHRCombatComponent* Combat = Character->GetCombat())
			{
				Combat->EnterStagger(0.8f);
			}
			UHRFactionSubsystem::NotifyAttack(Owner, Character);
		}
		if (IHRElementalReactive* Reactive = Cast<IHRElementalReactive>(Actor))
		{
			Reactive->ReactToElement(EHRDamageType::Arcane, Owner);
		}
	}
	UAISense_Hearing::ReportNoiseEvent(World, Origin, 0.8f, Owner, 1500.f);
}

// --- Blink -------------------------------------------------------------------------------------

UHRAbility_Blink::UHRAbility_Blink()
{
	AbilityId = TEXT("Ability.Blink");
	DisplayName = LOCTEXT("BlinkName", "Blink");
	Description = LOCTEXT("BlinkDesc", "Step through space. Passes through enemies, not walls.");
	FocusCost = 10.f;
	Cooldown = 1.5f;
	CastTime = 0.1f;
}

void UHRAbility_Blink::Activate(AHRCharacterBase* Owner)
{
	Super::Activate(Owner);
	UWorld* World = Owner->GetWorld();
	const UCapsuleComponent* Capsule = Owner->GetCapsuleComponent();

	FVector Direction = Owner->GetLastMovementInputVector();
	Direction.Z = 0.f;
	if (Direction.IsNearlyZero())
	{
		Direction = Owner->GetActorForwardVector();
	}
	Direction.Normalize();

	const FVector Start = Owner->GetActorLocation();
	const FVector End = Start + Direction * Distance;

	// Sweep only against world geometry so enemies can be passed through.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRBlink), false, Owner);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
	FVector Target = End;
	if (World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_WorldStatic, Shape, Params))
	{
		Target = Hit.Location - Direction * 10.f;
	}
	Owner->TeleportTo(Target, Owner->GetActorRotation());
	if (UHRCombatComponent* Combat = Owner->GetCombat())
	{
		Combat->GrantInvulnerability(0.25f);
	}
}

// --- Ward --------------------------------------------------------------------------------------

UHRAbility_Ward::UHRAbility_Ward()
{
	AbilityId = TEXT("Ability.Ward");
	DisplayName = LOCTEXT("WardName", "Ember Ward");
	Description = LOCTEXT("WardDesc", "Wreathe yourself in protective embers for 10 seconds.");
	FocusCost = 20.f;
	Cooldown = 15.f;
	CastTime = 0.5f;
}

void UHRAbility_Ward::Activate(AHRCharacterBase* Owner)
{
	Super::Activate(Owner);
	if (UHRAttributeComponent* Attributes = Owner->GetAttributes())
	{
		TArray<FHRStatModifier> Modifiers;
		Modifiers.Add(FHRStatModifier(EHRStat::Defense, 40.f));
		Modifiers.Add(FHRStatModifier(EHRStat::FireResist, 0.25f));
		Modifiers.Add(FHRStatModifier(EHRStat::FrostResist, 0.25f));
		Modifiers.Add(FHRStatModifier(EHRStat::PostureRegen, 0.f, 1.5f));
		Attributes->AddTimedModifiers(AbilityId, Modifiers, 10.f);
	}
}

// --- Shadow Veil -------------------------------------------------------------------------------

UHRAbility_ShadowVeil::UHRAbility_ShadowVeil()
{
	AbilityId = TEXT("Ability.ShadowVeil");
	DisplayName = LOCTEXT("VeilName", "Shadow Veil");
	Description = LOCTEXT("VeilDesc", "Fade into shadow for 8 seconds. Enemies struggle to see you.");
	FocusCost = 22.f;
	Cooldown = 20.f;
}

void UHRAbility_ShadowVeil::Activate(AHRCharacterBase* Owner)
{
	Super::Activate(Owner);
	if (UHRAttributeComponent* Attributes = Owner->GetAttributes())
	{
		TArray<FHRStatModifier> Modifiers;
		Modifiers.Add(FHRStatModifier(EHRStat::Stealth, 0.75f));
		Attributes->AddTimedModifiers(AbilityId, Modifiers, 8.f);
	}
}

// --- Spirit Sight ------------------------------------------------------------------------------

UHRAbility_SpiritSight::UHRAbility_SpiritSight()
{
	AbilityId = TEXT("Ability.SpiritSight");
	DisplayName = LOCTEXT("SightName", "Spirit Sight");
	Description = LOCTEXT("SightDesc", "See what is hidden: secrets, clues, and enemies through walls. Drains focus.");
	FocusCost = 5.f;
	Cooldown = 0.5f;
	CastTime = 0.f;
	bIsToggle = true;
	FocusPerSecond = 3.f;
}

void UHRAbility_SpiritSight::Activate(AHRCharacterBase* Owner)
{
	Super::Activate(Owner);
	if (AHRPlayerCharacter* Player = Cast<AHRPlayerCharacter>(Owner))
	{
		Player->SetSpiritSight(true);
	}
}

void UHRAbility_SpiritSight::Deactivate(AHRCharacterBase* Owner)
{
	Super::Deactivate(Owner);
	if (AHRPlayerCharacter* Player = Cast<AHRPlayerCharacter>(Owner))
	{
		Player->SetSpiritSight(false);
	}
}

// --- Beast Form --------------------------------------------------------------------------------

UHRAbility_BeastForm::UHRAbility_BeastForm()
{
	AbilityId = TEXT("Ability.BeastForm");
	DisplayName = LOCTEXT("BeastName", "Beast Form");
	Description = LOCTEXT("BeastDesc", "Transform into a hulking beast: stronger, tougher, louder. Drains focus.");
	FocusCost = 20.f;
	Cooldown = 10.f;
	CastTime = 0.8f;
	bIsToggle = true;
	FocusPerSecond = 4.f;
	bBlocksOtherAbilities = true;
}

void UHRAbility_BeastForm::Activate(AHRCharacterBase* Owner)
{
	Super::Activate(Owner);
	if (UHRAttributeComponent* Attributes = Owner->GetAttributes())
	{
		TArray<FHRStatModifier> Modifiers;
		Modifiers.Add(FHRStatModifier(EHRStat::PhysicalPower, 12.f, 1.4f));
		Modifiers.Add(FHRStatModifier(EHRStat::MaxPosture, 0.f, 1.5f));
		Modifiers.Add(FHRStatModifier(EHRStat::PostureDamage, 0.f, 1.5f));
		Modifiers.Add(FHRStatModifier(EHRStat::Defense, 30.f));
		Modifiers.Add(FHRStatModifier(EHRStat::MoveSpeed, 0.f, 1.15f));
		Modifiers.Add(FHRStatModifier(EHRStat::Stealth, -0.6f));
		Attributes->SetModifierSource(AbilityId, Modifiers);
	}
	if (UHRCombatComponent* Combat = Owner->GetCombat())
	{
		Combat->PoiseThreshold = 25.f;
	}
	Owner->SetVisualScale(1.35f);
	Owner->SetVisualColor(FLinearColor(0.45f, 0.08f, 0.05f));
}

void UHRAbility_BeastForm::Deactivate(AHRCharacterBase* Owner)
{
	Super::Deactivate(Owner);
	if (UHRAttributeComponent* Attributes = Owner->GetAttributes())
	{
		Attributes->RemoveModifierSource(AbilityId);
	}
	if (UHRCombatComponent* Combat = Owner->GetCombat())
	{
		Combat->PoiseThreshold = 0.f;
	}
	Owner->SetVisualScale(1.f);
	Owner->ResetVisualColor();
}

#undef LOCTEXT_NAMESPACE
