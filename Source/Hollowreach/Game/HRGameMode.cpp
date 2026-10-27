#include "Game/HRGameMode.h"

#include "AI/HRAIController.h"
#include "Characters/HRBossCharacter.h"
#include "Characters/HREnemyCharacter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRProgressionComponent.h"
#include "Core/HRStatics.h"
#include "EngineUtils.h"
#include "Game/HRHUD.h"
#include "Game/HRPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Narrative/HRQuestSubsystem.h"
#include "Procedural/HRWorldBootstrap.h"
#include "Systems/HRSaveGame.h"
#include "Systems/HRSaveSubsystem.h"
#include "Systems/HRTimeWeatherSubsystem.h"
#include "World/HRCheckpoint.h"
#include "World/HRDroppedEchoes.h"

#define LOCTEXT_NAMESPACE "HRGameMode"

AHRGameMode::AHRGameMode()
{
	DefaultPawnClass = AHRPlayerCharacter::StaticClass();
	PlayerControllerClass = AHRPlayerController::StaticClass();
	HUDClass = AHRHUD::StaticClass();
	BootstrapClass = AHRWorldBootstrap::StaticClass();
}

AActor* AHRGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (AActor* Start = Super::ChoosePlayerStart_Implementation(Player))
	{
		return Start;
	}
	// Empty level: make a start so the greybox world can be played from a blank map.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0.f, -300.f, 200.f), FRotator::ZeroRotator, Params);
}

void AHRGameMode::StartPlay()
{
	// 1. Restore world state first so every actor reads it in BeginPlay.
	if (UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(this))
	{
		if (bAutoLoadSave)
		{
			if (const UHRSaveGame* Save = Saves->LoadWorldState())
			{
				LastCheckpointId = Save->LastCheckpoint;
				DeathCount = Save->DeathCount;
				WorldSeed = Save->WorldSeed != 0 ? Save->WorldSeed : WorldSeed;
				bTeleportToCheckpointOnStart = Save->bHasPlayerTransform || !LastCheckpointId.IsNone();
				if (Save->bHasDroppedEchoes)
				{
					SpawnDroppedEchoes(Save->DroppedEchoesLocation, Save->DroppedEchoesAmount);
				}
			}
		}
	}

	// 2. Build the greybox world if the level doesn't provide one.
	if (bBuildGreyboxWorld && BootstrapClass)
	{
		AHRWorldBootstrap* Bootstrap = nullptr;
		for (TActorIterator<AHRWorldBootstrap> It(GetWorld()); It; ++It)
		{
			Bootstrap = *It;
			break;
		}
		if (!Bootstrap)
		{
			Bootstrap = GetWorld()->SpawnActor<AHRWorldBootstrap>(BootstrapClass);
		}
		if (Bootstrap)
		{
			Bootstrap->BuildWorld(WorldSeed);
		}
	}

	Super::StartPlay();
	if (UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(this)) Saves->RestoreShelters(GetWorld());
	if (UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this)) Quests->StartQuest(TEXT("Quest.Embers"));

	if (UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>())
	{
		TimeWeather->OnNightfall.AddDynamic(this, &AHRGameMode::HandleNightfall);
		TimeWeather->OnDawn.AddDynamic(this, &AHRGameMode::HandleDawn);
	}

	// 3. Put a returning player back at their shrine.
	if (bTeleportToCheckpointOnStart)
	{
		TWeakObjectPtr<AHRGameMode> WeakThis = this;
		GetWorldTimerManager().SetTimerForNextTick([WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(WeakThis.Get());
			const UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(WeakThis.Get());
			const UHRSaveGame* Save = Saves ? Saves->GetLoadedSave() : nullptr;
			if (Player && Save && Save->bHasPlayerTransform)
			{
				Player->SetActorTransform(Save->PlayerTransform);
			}
			else if (AHRCheckpoint* Checkpoint = WeakThis->GetRespawnCheckpoint())
			{
				if (Player)
				{
					Player->SetActorTransform(Checkpoint->GetRespawnTransform());
				}
			}
			UHRStatics::Notify(WeakThis.Get(), LOCTEXT("Welcome", "The embers remember you."), FLinearColor(1.f, 0.7f, 0.4f), true);
		});
	}
	else
	{
		if (UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this))
		{
			Quests->StartQuest(TEXT("Quest.Embers"));
		}
		UHRStatics::Notify(this, LOCTEXT("Intro", "VEIL OF ASH: HOLLOWREACH - Find the ember shrine. Press J for your journal."), FLinearColor(1.f, 0.8f, 0.5f), true);
	}
	// Flush an imported snapshot after the earlier startup-teleport callback has run.
	if (const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		if (Narrative->GetFlag(TEXT("Campaign.ImportedLegacy")) == 1)
		{
			TWeakObjectPtr<AHRGameMode> WeakMode = this;
			GetWorldTimerManager().SetTimerForNextTick([WeakMode]()
			{
				if (!WeakMode.IsValid()) return;
				UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(WeakMode.Get());
				UHRNarrativeSubsystem* State = HRGetSubsystem<UHRNarrativeSubsystem>(WeakMode.Get());
				AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(WeakMode.Get());
				if (State && Saves && Player)
				{
					State->SetFlag(TEXT("Campaign.ImportedLegacy"), 2);
					if (!Saves->SaveProgress(Player)) State->SetFlag(TEXT("Campaign.ImportedLegacy"), 1);
				}
			});
		}
	}

}

void AHRGameMode::RegisterEnemy(AHREnemyCharacter* Enemy)
{
	if (Enemy)
	{
		Enemies.AddUnique(Enemy);
	}
}

void AHRGameMode::RegisterCheckpoint(AHRCheckpoint* Checkpoint)
{
	if (Checkpoint)
	{
		Checkpoints.AddUnique(Checkpoint);
	}
}

AHRCheckpoint* AHRGameMode::FindCheckpoint(FName CheckpointId) const
{
	for (const TWeakObjectPtr<AHRCheckpoint>& Checkpoint : Checkpoints)
	{
		if (Checkpoint.IsValid() && Checkpoint->PersistentId == CheckpointId)
		{
			return Checkpoint.Get();
		}
	}
	return nullptr;
}

TArray<AHRCheckpoint*> AHRGameMode::GetDiscoveredCheckpoints() const
{
	TArray<AHRCheckpoint*> Result;
	for (const TWeakObjectPtr<AHRCheckpoint>& Checkpoint : Checkpoints)
	{
		if (Checkpoint.IsValid() && Checkpoint->IsDiscovered())
		{
			Result.Add(Checkpoint.Get());
		}
	}
	return Result;
}

AHRCheckpoint* AHRGameMode::GetRespawnCheckpoint() const
{
	if (AHRCheckpoint* Last = FindCheckpoint(LastCheckpointId))
	{
		return Last;
	}
	const TArray<AHRCheckpoint*> Discovered = GetDiscoveredCheckpoints();
	return Discovered.Num() > 0 ? Discovered[0] : nullptr;
}

void AHRGameMode::RespawnEnemies()
{
	Enemies.RemoveAll([](const TWeakObjectPtr<AHREnemyCharacter>& Enemy) { return !Enemy.IsValid(); });
	for (const TWeakObjectPtr<AHREnemyCharacter>& Enemy : Enemies)
	{
		if (AHRBossCharacter* Boss = Cast<AHRBossCharacter>(Enemy.Get()))
		{
			if (!Boss->IsDefeated())
			{
				Boss->ResetEnemy();
			}
			continue;
		}
		if (Enemy->bRespawnsOnRest || !Enemy->IsDead())
		{
			Enemy->ResetEnemy();
		}
	}
}

void AHRGameMode::HandlePlayerDeath(AHRPlayerCharacter* Player, AActor* Killer)
{
	if (!Player)
	{
		return;
	}
	++DeathCount;
	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->AddFlag(TEXT("Stats.Deaths"), 1);
	}

	// Souls rule: unspent echoes stay where you fell. Die again first and they're gone.
	if (AHRDroppedEchoes* Previous = DroppedEchoes.Get())
	{
		UHRStatics::Notify(this, LOCTEXT("EchoesLost", "Your previous echoes fade forever."), FLinearColor(0.6f, 0.6f, 0.6f));
		Previous->Destroy();
		DroppedEchoes = nullptr;
	}
	const int32 Carried = Player->GetProgression()->GetEchoes();
	if (Carried > 0)
	{
		SpawnDroppedEchoes(Player->GetActorLocation(), Carried);
		Player->GetProgression()->SetEchoes(0);
	}

	// Bosses teach you something every time they win.
	for (const TWeakObjectPtr<AHREnemyCharacter>& Enemy : Enemies)
	{
		if (AHRBossCharacter* Boss = Cast<AHRBossCharacter>(Enemy.Get()))
		{
			Boss->OnPlayerDefeated();
		}
	}

	if (AHRPlayerController* PC = Cast<AHRPlayerController>(Player->GetController()))
	{
		PC->SetUIMode(EHRUIMode::Dead);
	}
	DeadPlayer = Player;
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &AHRGameMode::RespawnPlayer, RespawnDelay, false);
}

void AHRGameMode::RespawnPlayer()
{
	AHRPlayerCharacter* Player = DeadPlayer.Get();
	if (!Player)
	{
		return;
	}
	const AHRCheckpoint* Checkpoint = GetRespawnCheckpoint();
	const FTransform SpawnTransform = Checkpoint ? Checkpoint->GetRespawnTransform() : FTransform(FRotator::ZeroRotator, FVector(0.f, -300.f, 250.f));
	Player->RespawnAt(SpawnTransform);
	RespawnEnemies();
	for (const TWeakObjectPtr<AHREnemyCharacter>& Enemy : Horde)
	{
		if (Enemy.IsValid())
		{
			Enemy->Destroy();
		}
	}
	Horde.Reset();

	if (AHRPlayerController* PC = Cast<AHRPlayerController>(Player->GetController()))
	{
		PC->SetUIMode(EHRUIMode::Gameplay);
	}
}

void AHRGameMode::SpawnDroppedEchoes(const FVector& Location, int32 Amount)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AHRDroppedEchoes* Drop = GetWorld()->SpawnActor<AHRDroppedEchoes>(AHRDroppedEchoes::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Drop)
	{
		Drop->Amount = Amount;
		DroppedEchoes = Drop;
	}
}

void AHRGameMode::OnPlayerRested(AHRPlayerCharacter* Player, AHRCheckpoint* Checkpoint)
{
	if (!Player || !Checkpoint)
	{
		return;
	}
	LastCheckpointId = Checkpoint->PersistentId;
	RespawnEnemies();
	if (UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(this))
	{
		Saves->SaveProgress(Player);
	}
}

void AHRGameMode::FastTravel(AHRPlayerCharacter* Player, AHRCheckpoint* Destination)
{
	if (!Player || !Destination || !Destination->IsDiscovered())
	{
		return;
	}
	Player->EndRest();
	Player->RespawnAt(Destination->GetRespawnTransform());
	LastCheckpointId = Destination->PersistentId;
	if (AHRPlayerController* PC = Cast<AHRPlayerController>(Player->GetController()))
	{
		PC->SetUIMode(EHRUIMode::Gameplay);
	}
	UHRStatics::Notify(this, FText::Format(LOCTEXT("Travelled", "Travelled to {0}"), Destination->DisplayName), FLinearColor(1.f, 0.7f, 0.4f));
}

void AHRGameMode::WriteToSave(UHRSaveGame& Save) const
{
	Save.LastCheckpoint = LastCheckpointId;
	Save.WorldSeed = WorldSeed;
	Save.DeathCount = DeathCount;
	if (const AHRDroppedEchoes* Drop = DroppedEchoes.Get())
	{
		Save.bHasDroppedEchoes = true;
		Save.DroppedEchoesLocation = Drop->GetActorLocation();
		Save.DroppedEchoesAmount = Drop->Amount;
	}
}

void AHRGameMode::HandleNightfall()
{
	const UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>();
	AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(this);
	if (!TimeWeather || !TimeWeather->IsBloodMoon() || !Player)
	{
		return;
	}

	// Every seventh night the Hollow come in force (7 Days to Die).
	UHRStatics::Notify(this, LOCTEXT("BloodMoon", "THE BLOOD MOON RISES. They are coming."), FLinearColor(1.f, 0.2f, 0.2f), true);
	for (int32 Index = 0; Index < HordeSize; ++Index)
	{
		const float Angle = (2.f * PI * Index) / HordeSize + FMath::FRandRange(-0.3f, 0.3f);
		const FVector Offset(FMath::Cos(Angle) * 2800.f, FMath::Sin(Angle) * 2800.f, 0.f);
		FVector Location = Player->GetActorLocation() + Offset;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Location + FVector(0.f, 0.f, 5000.f), Location - FVector(0.f, 0.f, 5000.f), ECC_Visibility))
		{
			Location = Hit.ImpactPoint + FVector(0.f, 0.f, 100.f);
		}

		const FTransform SpawnTransform((-Offset).Rotation(), Location);
		AHREnemyCharacter* Enemy = GetWorld()->SpawnActorDeferred<AHREnemyCharacter>(AHREnemyCharacter::StaticClass(), SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Enemy)
		{
			continue;
		}
		Enemy->bTemporary = true;
		Enemy->bRespawnsOnRest = false;
		Enemy->DisplayName = LOCTEXT("Frenzied", "Frenzied Hollow");
		Enemy->BodyColor = FLinearColor(0.6f, 0.05f, 0.05f);
		Enemy->Aggression = 0.85f;
		Enemy->EchoReward = 90;
		Enemy->FinishSpawning(SpawnTransform);
		Enemy->ApplyDifficultyScale(1.2f + 0.05f * TimeWeather->GetDay());
		if (AHRAIController* AI = Enemy->GetHRController())
		{
			AI->AlertTo(Player);
		}
		Horde.Add(Enemy);
	}
}

void AHRGameMode::HandleDawn()
{
	if (Horde.Num() == 0)
	{
		return;
	}
	for (const TWeakObjectPtr<AHREnemyCharacter>& Enemy : Horde)
	{
		if (Enemy.IsValid() && !Enemy->IsDead())
		{
			Enemy->Destroy();
		}
	}
	Horde.Reset();
	UHRStatics::Notify(this, LOCTEXT("Dawn", "Dawn breaks. The horde scatters."), FLinearColor(1.f, 0.9f, 0.6f), true);
}

#undef LOCTEXT_NAMESPACE
