#include "World/HRLootContainer.h"

#include "AI/HRAIController.h"
#include "Characters/HREnemyCharacter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRStatics.h"
#include "Engine/World.h"
#include "Game/HRGameMode.h"
#include "Items/HRLootLibrary.h"
#include "Systems/HRContentSubsystem.h"

#define LOCTEXT_NAMESPACE "HRLoot"

AHRLootContainer::AHRLootContainer()
{
	Mesh->SetRelativeScale3D(FVector(1.f, 0.65f, 0.55f));
	Color = FLinearColor(0.35f, 0.2f, 0.08f);

	Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lid"));
	Lid->SetupAttachment(Mesh);
	Lid->SetStaticMesh(HRGreybox::Cube());
	Lid->SetRelativeLocation(FVector(0.f, -50.f, 60.f));
	Lid->SetRelativeScale3D(FVector(1.05f, 1.05f, 0.2f));
	Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHRLootContainer::BeginPlay()
{
	Super::BeginPlay();
	if (bIsLootBag)
	{
		Lid->SetVisibility(false);
		return;
	}
	HRGreybox::Tint(Lid, Color * 0.8f);
	if (IsOpened())
	{
		SetOpenedVisuals();
	}
}

bool AHRLootContainer::IsOpened() const
{
	return !bIsLootBag && GetPersistentFlag(TEXT("Opened")) > 0;
}

void AHRLootContainer::SetOpenedVisuals()
{
	Lid->SetRelativeRotation(FRotator(0.f, 0.f, -100.f));
}

bool AHRLootContainer::CanInteract(const AHRPlayerCharacter* Player) const
{
	return !IsOpened();
}

FText AHRLootContainer::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	if (bIsLootBag)
	{
		return LOCTEXT("Scavenge", "Scavenge");
	}
	if (!RequiredKeyItem.IsNone() && Player && !Player->GetInventory()->HasItem(RequiredKeyItem))
	{
		return LOCTEXT("Locked", "Locked");
	}
	return LOCTEXT("Open", "Open");
}

void AHRLootContainer::Interact(AHRPlayerCharacter* Player)
{
	if (!RequiredKeyItem.IsNone())
	{
		if (!Player->GetInventory()->HasItem(RequiredKeyItem))
		{
			UHRStatics::Notify(this, LOCTEXT("NeedKey", "It's locked. Perhaps a key lies somewhere."), FLinearColor::Gray);
			return;
		}
		if (bConsumeKey)
		{
			Player->GetInventory()->RemoveItem(RequiredKeyItem, 1);
		}
	}

	const AHRGameMode* GameMode = GetWorld()->GetAuthGameMode<AHRGameMode>();
	const int32 WorldSeed = GameMode ? GameMode->WorldSeed : 0;
	const int32 Seed = UHRStatics::HashName(PersistentId, WorldSeed);

	if (!bIsLootBag && MimicChance > 0.f && FRandomStream(Seed + 7).FRand() < MimicChance)
	{
		SetPersistentFlag(TEXT("Opened"), 1);
		SpawnMimic(Player);
		return;
	}

	TArray<FHRItemInstance> Items = PrerolledItems;
	int32 Echoes = PrerolledEchoes;
	if (!bIsLootBag)
	{
		const float Luck = Player->GetAttributes()->GetStat(EHRStat::LootLuck);
		UHRLootLibrary::RollLoot(this, LootTableId, Seed, Luck, Items, Echoes);
	}

	for (const FHRItemInstance& Item : Items)
	{
		Player->GetInventory()->AddItemInstance(Item, true);
	}
	if (Echoes > 0)
	{
		Player->GetProgression()->AddEchoes(Echoes, true);
	}
	if (Items.Num() == 0 && Echoes == 0)
	{
		UHRStatics::Notify(this, LOCTEXT("Empty", "Empty."), FLinearColor::Gray);
	}

	if (bIsLootBag)
	{
		Destroy();
		return;
	}
	SetPersistentFlag(TEXT("Opened"), 1);
	SetOpenedVisuals();
}

void AHRLootContainer::SpawnMimic(AHRPlayerCharacter* Player)
{
	const FTransform SpawnTransform(GetActorRotation(), GetActorLocation() + FVector(0.f, 0.f, 100.f));
	AHREnemyCharacter* Mimic = GetWorld()->SpawnActorDeferred<AHREnemyCharacter>(AHREnemyCharacter::StaticClass(), SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Mimic)
	{
		return;
	}
	Mimic->DisplayName = LOCTEXT("Mimic", "Mimic");
	Mimic->EnemyTypeId = TEXT("Enemy.Mimic");
	Mimic->BodyColor = Color;
	Mimic->LootTableId = TEXT("Loot.Chest.Rare");
	Mimic->EchoReward = 400;
	Mimic->bRespawnsOnRest = false;
	Mimic->bTemporary = true;
	Mimic->SkillLevel = 0.4f;
	Mimic->Aggression = 0.9f;
	Mimic->GetAttributes()->SetBaseStat(EHRStat::MaxHealth, 320.f);
	Mimic->GetCombat()->OutgoingDamageMultiplier = 1.8f;
	Mimic->FinishSpawning(SpawnTransform);

	UHRStatics::Notify(this, LOCTEXT("MimicReveal", "The chest bares its teeth!"), FLinearColor(1.f, 0.3f, 0.3f), true);
	if (AHRAIController* AI = Mimic->GetHRController())
	{
		AI->AlertTo(Player);
	}
	Destroy();
}

AHRLootContainer* AHRLootContainer::SpawnLootBag(UWorld* World, const FVector& Location, const TArray<FHRItemInstance>& Items, int32 Echoes)
{
	if (!World)
	{
		return nullptr;
	}
	const FTransform SpawnTransform(FRotator::ZeroRotator, Location + FVector(0.f, 0.f, 15.f));
	AHRLootContainer* Bag = World->SpawnActorDeferred<AHRLootContainer>(AHRLootContainer::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Bag)
	{
		return nullptr;
	}
	Bag->bIsLootBag = true;
	Bag->PrerolledItems = Items;
	Bag->PrerolledEchoes = Echoes;
	Bag->Color = FLinearColor(0.55f, 0.45f, 0.2f);
	Bag->GetMesh()->SetStaticMesh(HRGreybox::Sphere());
	Bag->GetMesh()->SetRelativeScale3D(FVector(0.35f));
	Bag->GetMesh()->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Bag->FinishSpawning(SpawnTransform);
	Bag->SetLifeSpan(300.f);
	return Bag;
}

#undef LOCTEXT_NAMESPACE
