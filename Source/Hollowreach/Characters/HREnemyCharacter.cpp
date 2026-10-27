#include "Characters/HREnemyCharacter.h"

#include "AI/HRAIController.h"
#include "Characters/HRCompanionCharacter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Core/HRStatics.h"
#include "Game/HRGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Items/HRLootLibrary.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Narrative/HRQuestSubsystem.h"
#include "World/HRLootContainer.h"
#include "World/HRProjectile.h"

AHREnemyCharacter::AHREnemyCharacter()
{
	AIControllerClass = AHRAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	FactionId = TEXT("Hollow");
	DisplayName = NSLOCTEXT("HR", "HollowName", "Hollow");
	BodyColor = FLinearColor(0.45f, 0.12f, 0.1f);
	BaseWalkSpeed = 380.f;

	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	// Perilous thrust: long windup, must be dodged (Sekiro's red kanji attack).
	PerilousAttack.DamageMultiplier = 2.2f;
	PerilousAttack.PostureMultiplier = 2.5f;
	PerilousAttack.StaminaCost = 0.f;
	PerilousAttack.Windup = 0.75f;
	PerilousAttack.ActiveTime = 0.2f;
	PerilousAttack.Recovery = 0.7f;
	PerilousAttack.Reach = 260.f;
	PerilousAttack.LungeSpeed = 900.f;
	PerilousAttack.bUnblockable = true;
}

void AHREnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnTransform = GetActorTransform();
	if (AHRGameMode* GameMode = GetWorld()->GetAuthGameMode<AHRGameMode>())
	{
		if (!bTemporary)
		{
			GameMode->RegisterEnemy(this);
		}
	}
}

AHRAIController* AHREnemyCharacter::GetHRController() const
{
	return Cast<AHRAIController>(GetController());
}

EHRAwareness AHREnemyCharacter::GetAwarenessTowards(const AActor* Other) const
{
	const AHRAIController* AI = GetHRController();
	if (!AI)
	{
		return EHRAwareness::Unaware;
	}
	if (AI->GetAwareness() == EHRAwareness::Alerted && AI->GetTargetActor() != Other)
	{
		// Busy fighting someone else: you're only a suspicion.
		return EHRAwareness::Suspicious;
	}
	return AI->GetAwareness();
}

void AHREnemyCharacter::ApplyDifficultyScale(float Scale)
{
	Attributes->ScaleBaseStat(EHRStat::MaxHealth, Scale);
	Attributes->ScaleBaseStat(EHRStat::MaxPosture, FMath::Sqrt(Scale));
	Combat->OutgoingDamageMultiplier *= FMath::Sqrt(Scale);
	EchoReward = FMath::RoundToInt(EchoReward * Scale);
	Attributes->RefillAll();
}

void AHREnemyCharacter::Die(AActor* Killer)
{
	if (bIsDead)
	{
		return;
	}
	Super::Die(Killer);

	// Companion kills still reward the player.
	AActor* Credit = Killer;
	if (Cast<AHRCompanionCharacter>(Killer))
	{
		Credit = UHRStatics::GetHRPlayer(this);
	}
	if (AHRPlayerCharacter* Player = Cast<AHRPlayerCharacter>(Credit))
	{
		Player->GetProgression()->AddEchoes(EchoReward, false);
		Player->NotifyKilledEnemy();
		DropLoot(Player);
	}

	if (UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this))
	{
		Quests->NotifyKill(EnemyTypeId);
	}
	if (AHRAIController* AI = GetHRController())
	{
		AI->HandlePawnDeath();
	}

	TWeakObjectPtr<AHREnemyCharacter> WeakThis = this;
	GetWorldTimerManager().SetTimer(DormantTimer, FTimerDelegate::CreateLambda([WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			if (WeakThis->bTemporary)
			{
				WeakThis->Destroy();
			}
			else
			{
				WeakThis->Dormant();
			}
		}
	}), 5.f, false);
}

void AHREnemyCharacter::DropLoot(AActor* Killer)
{
	if (LootTableId.IsNone())
	{
		return;
	}
	const UHRAttributeComponent* KillerAttributes = Killer ? Killer->FindComponentByClass<UHRAttributeComponent>() : nullptr;
	const float Luck = KillerAttributes ? KillerAttributes->GetStat(EHRStat::LootLuck) : 0.f;

	TArray<FHRItemInstance> Items;
	int32 Echoes = 0;
	UHRLootLibrary::RollLoot(this, LootTableId, FMath::Rand(), Luck, Items, Echoes);
	if (Items.Num() == 0 && Echoes == 0)
	{
		return;
	}
	AHRLootContainer::SpawnLootBag(GetWorld(), GetActorLocation() - FVector(0.f, 0.f, 70.f), Items, Echoes);
}

void AHREnemyCharacter::Dormant()
{
	bDormant = true;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	if (AHRAIController* AI = GetHRController())
	{
		AI->SetActorTickEnabled(false);
		AI->StopMovement();
	}
}

void AHREnemyCharacter::ResetEnemy()
{
	GetWorldTimerManager().ClearTimer(DormantTimer);
	bDormant = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	TeleportTo(SpawnTransform.GetLocation(), SpawnTransform.Rotator(), false, true);
	ReviveCharacter(1.f);
	if (AHRAIController* AI = GetHRController())
	{
		AI->SetActorTickEnabled(true);
		AI->ResetAI();
	}
}

void AHREnemyCharacter::FireProjectileAt(AActor* Target)
{
	if (!Target || !Combat->BeginBusy(0.6f))
	{
		return;
	}
	FHRDamageInfo Damage;
	Damage.Source = this;
	Damage.Amount = Combat->GetWeapon().BaseDamage * Combat->OutgoingDamageMultiplier;
	Damage.PostureDamage = Damage.Amount * 0.5f;
	Damage.Type = Combat->GetWeapon().DamageType;

	const FVector Start = GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0.f, 0.f, 50.f);
	// Lead the target slightly.
	const FVector Aim = Target->GetActorLocation() + Target->GetVelocity() * 0.25f - Start;
	AHRProjectile::Launch(this, Start, Aim.GetSafeNormal(), Damage, 2000.f, 0.05f);
}
