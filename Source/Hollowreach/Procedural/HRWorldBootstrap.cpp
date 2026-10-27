#include "Procedural/HRWorldBootstrap.h"
#include "SharedArtLibrary.h"

#include "Characters/HRBossCharacter.h"
#include "Characters/HRCompanionCharacter.h"
#include "Characters/HREnemyCharacter.h"
#include "Characters/HRNPCCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Core/HRGreybox.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"
#include "Procedural/HRDungeonGenerator.h"
#include "Puzzles/HRPuzzleController.h"
#include "Puzzles/HRPuzzleElement.h"
#include "Systems/HRTimeWeatherSubsystem.h"
#include "Vehicles/HRHoverVehicle.h"
#include "World/HRCheckpoint.h"
#include "World/HRDiscoverables.h"
#include "World/HREndingAltar.h"
#include "World/HRGate.h"
#include "World/HRGrapplePoint.h"
#include "World/HRLootContainer.h"
#include "World/HRMovingPlatform.h"
#include "World/HRPhysicsProp.h"
#include "World/HRResourceNode.h"
#include "World/HRStealthZone.h"

#define LOCTEXT_NAMESPACE "HRWorld"

namespace
{
	/** Spawns an actor deferred so it can be configured before BeginPlay. */
	template <typename T, typename FConfigure>
	T* SpawnConfigured(UWorld* World, const FVector& Location, const FRotator& Rotation, FConfigure&& Configure)
	{
		const FTransform Transform(Rotation, Location);
		T* Actor = World->SpawnActorDeferred<T>(T::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Actor)
		{
			Configure(Actor);
			Actor->FinishSpawning(Transform);
		}
		return Actor;
	}

	FText Txt(const TCHAR* String)
	{
		return FText::FromString(String);
	}

	// Region anchors (cm). The village sits at the origin.
	const FVector ForestCenter(9000.f, 0.f, 0.f);
	const FVector CampCenter(2500.f, 9500.f, 0.f);
	const FVector RuinsCenter(-9500.f, 1500.f, 0.f);
	const FVector BellTowerCenter(-4500.f, -6000.f, 0.f);
	const FVector SpireCenter(6000.f, -7500.f, 0.f);
	const FVector MountainCenter(-3000.f, -21000.f, 0.f);
	const FVector TrialOrigin(21000.f, -1500.f, 0.f);

	// Mountain: stacked terraces.
	constexpr int32 TerraceCount = 6;
	constexpr float TerraceHalf0 = 6000.f;
	constexpr float TerraceStep = 900.f;
	constexpr float TerraceRise = 500.f;
}

AHRWorldBootstrap::AHRWorldBootstrap()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	for (uint8 Index = 0; Index < static_cast<uint8>(EHRPalette::MAX); ++Index)
	{
		UInstancedStaticMeshComponent* Palette = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Palette%d"), Index));
		Palette->SetupAttachment(RootComponent);
		Palette->SetStaticMesh(HRGreybox::Cube());
		Palette->SetMobility(EComponentMobility::Movable);
		Palette->SetCollisionProfileName(TEXT("BlockAll"));
		Palettes.Add(Palette);
	}
	// Foliage canopies and water don't block.
	Palettes[static_cast<uint8>(EHRPalette::Foliage)]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Palettes[static_cast<uint8>(EHRPalette::Water)]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AHRWorldBootstrap::BuildWorld(int32 InSeed)
{
	if (bBuilt)
	{
		return;
	}
	bBuilt = true;
	Seed = InSeed;
	SetActorLocation(FVector::ZeroVector);

	const FLinearColor Colors[] = {
		FLinearColor(0.16f, 0.2f, 0.1f),   // Ground
		FLinearColor(0.32f, 0.32f, 0.33f), // Stone
		FLinearColor(0.3f, 0.2f, 0.1f),    // Wood
		FLinearColor(0.85f, 0.87f, 0.9f),  // Snow
		FLinearColor(0.2f, 0.22f, 0.25f),  // Ruin
		FLinearColor(0.45f, 0.15f, 0.08f), // Ember
		FLinearColor(0.08f, 0.22f, 0.07f), // Foliage
		FLinearColor(0.1f, 0.25f, 0.4f),   // Water
	};
	for (int32 Index = 0; Index < Palettes.Num(); ++Index)
	{
		HRGreybox::Tint(Palettes[Index], Colors[Index]);
		USharedArtLibrary::ApplySurface(Palettes[Index], FName(*StaticEnum<EHRPalette>()->GetNameStringByValue(Index)));
	}

	FRandomStream Stream(Seed);
	BuildEnvironment();
	BuildTerrain();
	BuildVillage();
	BuildForest(Stream);
	BuildAshbornCamp(Stream);
	BuildRuins(Stream);
	BuildBellTower();
	BuildSpire();
	BuildMountain();
	BuildVeilChapter();
	BuildTrial();
	BuildScatter(Stream);
}

// --- Helpers -----------------------------------------------------------------------------------

void AHRWorldBootstrap::Block(const FVector& Center, const FVector& Size, EHRPalette Palette, const FRotator& Rotation)
{
	Palettes[static_cast<uint8>(Palette)]->AddInstance(FTransform(Rotation, Center, Size / 100.f));
}

void AHRWorldBootstrap::Ramp(const FVector& Base, const FVector& Direction, float Run, float Rise, float Width, EHRPalette Palette)
{
	const float Length = FMath::Sqrt(Run * Run + Rise * Rise);
	const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(Rise, Run));
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
	const FVector Mid = Base + Direction * Run * 0.5f + FVector(0.f, 0.f, Rise * 0.5f - 25.f);
	Block(Mid, FVector(Length + 40.f, Width, 50.f), Palette, FRotator(Pitch, Yaw, 0.f));
}

void AHRWorldBootstrap::Tree(const FVector& Base, float Height)
{
	Block(Base + FVector(0.f, 0.f, Height * 0.5f), FVector(70.f, 70.f, Height), EHRPalette::Wood);
	Block(Base + FVector(0.f, 0.f, Height + 150.f), FVector(420.f, 420.f, 380.f), EHRPalette::Foliage, FRotator(0.f, 45.f, 0.f));
}

AHREnemyCharacter* AHRWorldBootstrap::SpawnEnemy(const FVector& Location, EHREnemyPreset Preset, float Yaw)
{
	return SpawnConfigured<AHREnemyCharacter>(GetWorld(), Location + FVector(0.f, 0.f, 100.f), FRotator(0.f, Yaw, 0.f), [Preset](AHREnemyCharacter* Enemy)
	{
		UHRAttributeComponent* Attributes = Enemy->GetAttributes();
		UHRCombatComponent* Combat = Enemy->GetCombat();
		switch (Preset)
		{
		case EHREnemyPreset::Hollow:
			Enemy->DisplayName = LOCTEXT("Hollow", "Hollow");
			Attributes->SetBaseStat(EHRStat::MaxHealth, 90.f);
			break;
		case EHREnemyPreset::HollowArcher:
			Enemy->DisplayName = LOCTEXT("HollowArcher", "Hollow Bowman");
			Enemy->bRanged = true;
			Enemy->BodyColor = FLinearColor(0.4f, 0.2f, 0.12f);
			Attributes->SetBaseStat(EHRStat::MaxHealth, 70.f);
			break;
		case EHREnemyPreset::HollowKnight:
			Enemy->DisplayName = LOCTEXT("HollowKnight", "Hollow Knight");
			Enemy->EnemyTypeId = TEXT("Enemy.HollowKnight");
			Enemy->SkillLevel = 0.55f;
			Enemy->PerilousChance = 0.2f;
			Enemy->EchoReward = 220;
			Enemy->LootTableId = TEXT("Loot.Chest.Common");
			Enemy->BodyColor = FLinearColor(0.3f, 0.3f, 0.35f);
			Attributes->SetBaseStat(EHRStat::MaxHealth, 260.f);
			Attributes->SetBaseStat(EHRStat::MaxPosture, 160.f);
			Attributes->SetBaseStat(EHRStat::Defense, 30.f);
			Combat->PoiseThreshold = 18.f;
			Combat->DefaultWeapon.BaseDamage = 22.f;
			Enemy->SetVisualScale(1.2f);
			break;
		case EHREnemyPreset::AshbornRaider:
			Enemy->DisplayName = LOCTEXT("Raider", "Ashborn Raider");
			Enemy->FactionId = TEXT("Ashborn");
			Enemy->EnemyTypeId = TEXT("Enemy.Ashborn");
			Enemy->LootTableId = TEXT("Loot.Ashborn");
			Enemy->BodyColor = FLinearColor(0.55f, 0.3f, 0.1f);
			Enemy->SkillLevel = 0.4f;
			Enemy->EchoReward = 80;
			Attributes->SetBaseStat(EHRStat::MaxHealth, 120.f);
			break;
		case EHREnemyPreset::AshbornArcher:
			Enemy->DisplayName = LOCTEXT("AshArcher", "Ashborn Archer");
			Enemy->FactionId = TEXT("Ashborn");
			Enemy->EnemyTypeId = TEXT("Enemy.Ashborn");
			Enemy->LootTableId = TEXT("Loot.Ashborn");
			Enemy->BodyColor = FLinearColor(0.5f, 0.35f, 0.15f);
			Enemy->bRanged = true;
			Enemy->bStationary = true;
			Enemy->EchoReward = 80;
			Attributes->SetBaseStat(EHRStat::MaxHealth, 90.f);
			break;
		case EHREnemyPreset::AshbornCaptain:
			Enemy->DisplayName = LOCTEXT("Captain", "Captain Varn");
			Enemy->FactionId = TEXT("Ashborn");
			Enemy->EnemyTypeId = TEXT("Enemy.AshbornCaptain");
			Enemy->LootTableId = TEXT("Loot.Captain");
			Enemy->BodyColor = FLinearColor(0.7f, 0.25f, 0.05f);
			Enemy->SkillLevel = 0.65f;
			Enemy->Aggression = 0.7f;
			Enemy->PerilousChance = 0.25f;
			Enemy->MaxComboHits = 4;
			Enemy->EchoReward = 600;
			Enemy->bRespawnsOnRest = false;
			Attributes->SetBaseStat(EHRStat::MaxHealth, 420.f);
			Attributes->SetBaseStat(EHRStat::MaxPosture, 200.f);
			Combat->PoiseThreshold = 20.f;
			Combat->DefaultWeapon.BaseDamage = 26.f;
			Enemy->SetVisualScale(1.25f);
			break;
		case EHREnemyPreset::FrostHollow:
			Enemy->DisplayName = LOCTEXT("FrostHollow", "Frostbitten Hollow");
			Enemy->BodyColor = FLinearColor(0.55f, 0.65f, 0.75f);
			Enemy->SkillLevel = 0.4f;
			Enemy->EchoReward = 120;
			Attributes->SetBaseStat(EHRStat::MaxHealth, 150.f);
			Combat->DefaultWeapon.DamageType = EHRDamageType::Frost;
			break;
		}
	});
}

// --- Environment -------------------------------------------------------------------------------

void AHRWorldBootstrap::BuildEnvironment()
{
	UWorld* World = GetWorld();
	ADirectionalLight* Sun = nullptr;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		Sun = *It;
		break;
	}
	if (!Sun)
	{
		Sun = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 5000.f), FRotator(-45.f, 35.f, 0.f));
		if (Sun)
		{
			Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			if (UDirectionalLightComponent* SunLight = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
			{
				SunLight->SetAtmosphereSunLight(true);
				SunLight->SetIntensity(8.f);
			}
		}
		World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator);
	}

	ASkyLight* Sky = nullptr;
	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		Sky = *It;
		break;
	}
	if (!Sky)
	{
		Sky = World->SpawnActor<ASkyLight>(FVector(0.f, 0.f, 2000.f), FRotator::ZeroRotator);
		if (Sky)
		{
			USkyLightComponent* SkyComponent = Sky->GetLightComponent();
			SkyComponent->SetMobility(EComponentMobility::Movable);
			SkyComponent->bRealTimeCapture = true;
			SkyComponent->SetIntensity(1.f);
			SkyComponent->RecaptureSky();
		}
	}

	AExponentialHeightFog* Fog = nullptr;
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		Fog = *It;
		break;
	}
	if (!Fog)
	{
		Fog = World->SpawnActor<AExponentialHeightFog>(FVector(0.f, 0.f, 0.f), FRotator::ZeroRotator);
		if (Fog)
		{
			Fog->GetComponent()->SetFogDensity(0.02f);
			Fog->GetComponent()->SetVolumetricFog(true);
		}
	}

	if (UHRTimeWeatherSubsystem* TimeWeather = World->GetSubsystem<UHRTimeWeatherSubsystem>())
	{
		TimeWeather->RegisterEnvironment(Sun, Fog, Sky);
	}
}

void AHRWorldBootstrap::BuildTerrain()
{
	// A 600m x 600m plain. Regions add their own relief.
	Block(FVector(0.f, 0.f, -50.f), FVector(60000.f, 60000.f, 100.f), EHRPalette::Ground);

	// A lake between the village and the ruins (decorative, shallow basin rim).
	Block(FVector(-4500.f, 2500.f, 2.f), FVector(3000.f, 2200.f, 4.f), EHRPalette::Water);

	// Rolling hills to break sightlines (stealth needs cover).
	const FVector Hills[] = {
		FVector(4000.f, 4000.f, 0.f), FVector(-6000.f, -2000.f, 0.f), FVector(12000.f, -6000.f, 0.f),
		FVector(-12000.f, 8000.f, 0.f), FVector(6000.f, 14000.f, 0.f), FVector(15000.f, 8000.f, 0.f)
	};
	for (const FVector& Hill : Hills)
	{
		Block(Hill + FVector(0.f, 0.f, 100.f), FVector(2400.f, 1800.f, 200.f), EHRPalette::Ground, FRotator(0.f, 20.f, 0.f));
		Block(Hill + FVector(0.f, 0.f, 250.f), FVector(1400.f, 1000.f, 200.f), EHRPalette::Ground, FRotator(0.f, 20.f, 0.f));
	}
}

// --- Village hub -------------------------------------------------------------------------------

void AHRWorldBootstrap::BuildVillage()
{
	UWorld* World = GetWorld();

	// Houses around the square.
	for (int32 Index = 0; Index < 7; ++Index)
	{
		const float Angle = Index * (2.f * PI / 7.f) + 0.3f;
		const FVector Center(FMath::Cos(Angle) * 2200.f, FMath::Sin(Angle) * 2200.f, 0.f);
		const float Yaw = FMath::RadiansToDegrees(Angle);
		Block(Center + FVector(0.f, 0.f, 200.f), FVector(600.f, 500.f, 400.f), EHRPalette::Wood, FRotator(0.f, Yaw, 0.f));
		Block(Center + FVector(0.f, 0.f, 450.f), FVector(680.f, 580.f, 100.f), EHRPalette::Stone, FRotator(0.f, Yaw, 0.f));
	}
	// Bell tower stump in the square (the bell is gone).
	Block(FVector(-300.f, 300.f, 300.f), FVector(250.f, 250.f, 600.f), EHRPalette::Stone);

	SpawnConfigured<AHRCheckpoint>(World, FVector(600.f, 0.f, 25.f), FRotator::ZeroRotator, [](AHRCheckpoint* Shrine)
	{
		Shrine->PersistentId = TEXT("Shrine.Hearth");
		Shrine->DisplayName = LOCTEXT("Hearth", "Ember Hearth");
		Shrine->bHasForge = true;
	});

	SpawnConfigured<AHRNPCCharacter>(World, FVector(1100.f, 650.f, 100.f), FRotator(0.f, 200.f, 0.f), [](AHRNPCCharacter* Npc)
	{
		Npc->NpcId = TEXT("NPC.Elder");
		Npc->DisplayName = LOCTEXT("Elder", "Elder Maren");
		Npc->DialogueId = TEXT("Dialogue.Elder");
		Npc->FactionId = TEXT("Villagers");
	});

	SpawnConfigured<AHRNPCCharacter>(World, FVector(1100.f, -750.f, 100.f), FRotator(0.f, 160.f, 0.f), [](AHRNPCCharacter* Npc)
	{
		Npc->NpcId = TEXT("NPC.Merchant");
		Npc->DisplayName = LOCTEXT("Merchant", "Odo the Peddler");
		Npc->DialogueId = TEXT("Dialogue.Merchant");
		Npc->FactionId = TEXT("Villagers");
		Npc->bIsMerchant = true;
		Npc->BodyColor = FLinearColor(0.6f, 0.5f, 0.2f);
		Npc->Stock = {
			FHRItemAmount(TEXT("Item.Herb"), 10), FHRItemAmount(TEXT("Item.Bread"), 8), FHRItemAmount(TEXT("Item.Waterskin"), 8),
			FHRItemAmount(TEXT("Item.Bandage"), 5), FHRItemAmount(TEXT("Item.Cloth"), 10), FHRItemAmount(TEXT("Item.Oil"), 6),
			FHRItemAmount(TEXT("Item.Firebomb"), 4), FHRItemAmount(TEXT("Item.HollowAsh"), 5), FHRItemAmount(TEXT("Item.FurCloak"), 1),
			FHRItemAmount(TEXT("Item.TwinFangs"), 1), FHRItemAmount(TEXT("Item.LeatherHood"), 1), FHRItemAmount(TEXT("Item.Moonstone"), 1),
		};
	});

	SpawnConfigured<AHRNPCCharacter>(World, FVector(-1000.f, 950.f, 100.f), FRotator(0.f, -30.f, 0.f), [](AHRNPCCharacter* Npc)
	{
		Npc->NpcId = TEXT("NPC.Envoy");
		Npc->DisplayName = LOCTEXT("Envoy", "Sable of the Ashborn");
		Npc->DialogueId = TEXT("Dialogue.Envoy");
		Npc->FactionId = TEXT("Neutral");
		Npc->BodyColor = FLinearColor(0.6f, 0.3f, 0.12f);
	});

	SpawnConfigured<AHRCompanionCharacter>(World, FVector(300.f, 1000.f, 100.f), FRotator(0.f, -90.f, 0.f), [](AHRCompanionCharacter* Companion)
	{
		Companion->CompanionId = TEXT("Companion.Kestrel");
		Companion->DialogueId = TEXT("Dialogue.Kestrel");
	});

	SpawnConfigured<AHRHoverVehicle>(World, FVector(-1400.f, -700.f, 150.f), FRotator(0.f, -60.f, 0.f), [](AHRHoverVehicle*) {});

	// The village well never runs dry.
	SpawnConfigured<AHRResourceNode>(World, FVector(250.f, -1000.f, 40.f), FRotator::ZeroRotator, [](AHRResourceNode* Node)
	{
		Node->ItemId = TEXT("Item.Waterskin");
		Node->DisplayName = LOCTEXT("Well", "fresh water");
		Node->MinQuantity = 1;
		Node->MaxQuantity = 1;
		Node->RespawnHours = 1.f;
		Node->Color = FLinearColor(0.2f, 0.4f, 0.8f);
		Node->GetMesh()->SetStaticMesh(HRGreybox::Cylinder());
		Node->GetMesh()->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.8f));
	});

	SpawnConfigured<AHRLandmark>(World, FVector(0.f, 0.f, 0.f), FRotator::ZeroRotator, [](AHRLandmark* Landmark)
	{
		Landmark->LandmarkId = TEXT("Landmark.Emberfall");
		Landmark->DisplayName = LOCTEXT("Emberfall", "EMBERFALL");
		Landmark->EchoReward = 0;
		Landmark->KnowledgeId = TEXT("Lore.Emberfall");
		Landmark->SetExtent(FVector(2600.f, 2600.f, 800.f));
	});

	// A practice crate stack and barrels for learning physics and fire.
	for (int32 Index = 0; Index < 3; ++Index)
	{
		SpawnConfigured<AHRPhysicsProp>(World, FVector(-600.f + Index * 90.f, -1300.f, 60.f + Index * 10.f), FRotator::ZeroRotator, [](AHRPhysicsProp*) {});
	}
}

// --- Whispering Wood (stealth, gathering, first combat) ----------------------------------------

void AHRWorldBootstrap::BuildForest(FRandomStream& Stream)
{
	UWorld* World = GetWorld();
	for (int32 Index = 0; Index < 70; ++Index)
	{
		const FVector Offset(Stream.FRandRange(-4500.f, 4500.f), Stream.FRandRange(-4500.f, 4500.f), 0.f);
		Tree(ForestCenter + Offset, Stream.FRandRange(500.f, 900.f));
	}
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const FVector Offset(Stream.FRandRange(-4000.f, 4000.f), Stream.FRandRange(-4000.f, 4000.f), 55.f);
		SpawnConfigured<AHRStealthZone>(World, ForestCenter + Offset, FRotator(0.f, Stream.FRandRange(0.f, 90.f), 0.f), [&Stream](AHRStealthZone* Zone)
		{
			Zone->SetZoneSize(FVector(Stream.FRandRange(500.f, 1100.f), Stream.FRandRange(500.f, 1100.f), 110.f));
		});
	}

	for (int32 Index = 0; Index < 10; ++Index)
	{
		const bool bHerb = Index % 3 != 2;
		const FVector Offset(Stream.FRandRange(-4000.f, 4000.f), Stream.FRandRange(-4000.f, 4000.f), 30.f);
		SpawnConfigured<AHRResourceNode>(World, ForestCenter + Offset, FRotator::ZeroRotator, [bHerb](AHRResourceNode* Node)
		{
			Node->ItemId = bHerb ? FName(TEXT("Item.Herb")) : FName(TEXT("Item.Mushroom"));
			Node->DisplayName = bHerb ? LOCTEXT("Herbs", "herbs") : LOCTEXT("Mushrooms", "mushrooms");
			Node->BonusItemId = TEXT("Item.Herb");
			Node->GetMesh()->SetStaticMesh(HRGreybox::Sphere());
			Node->GetMesh()->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.4f));
			Node->Color = bHerb ? FLinearColor(0.2f, 0.7f, 0.2f) : FLinearColor(0.7f, 0.5f, 0.3f);
		});
	}
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const FVector Offset(Stream.FRandRange(-4000.f, 4000.f), Stream.FRandRange(-4000.f, 4000.f), 60.f);
		SpawnConfigured<AHRResourceNode>(World, ForestCenter + Offset, FRotator::ZeroRotator, [](AHRResourceNode* Node)
		{
			Node->ItemId = TEXT("Item.Wood");
			Node->DisplayName = LOCTEXT("Deadwood", "deadwood");
			Node->BonusItemId = TEXT("Item.Leather");
			Node->BonusChance = 0.2f;
			Node->GetMesh()->SetRelativeScale3D(FVector(1.6f, 0.5f, 0.5f));
			Node->Color = FLinearColor(0.35f, 0.22f, 0.1f);
		});
	}

	SpawnEnemy(ForestCenter + FVector(-1500.f, 1200.f, 0.f), EHREnemyPreset::Hollow, 30.f);
	SpawnEnemy(ForestCenter + FVector(800.f, -1600.f, 0.f), EHREnemyPreset::Hollow, 120.f);
	SpawnEnemy(ForestCenter + FVector(2200.f, 1800.f, 0.f), EHREnemyPreset::Hollow, 200.f);
	SpawnEnemy(ForestCenter + FVector(3000.f, -500.f, 0.f), EHREnemyPreset::HollowArcher, 180.f);
	if (AHREnemyCharacter* Patroller = SpawnEnemy(ForestCenter + FVector(0.f, 0.f, 0.f), EHREnemyPreset::Hollow))
	{
		Patroller->PatrolPoints = { FVector(0.f, 0.f, 0.f), FVector(2500.f, 0.f, 0.f), FVector(2500.f, 2500.f, 0.f), FVector(0.f, 2500.f, 0.f) };
	}

	// Knowledge-based progression: the Old Tongue primer unlocks glyphs and Spirit Sight.
	SpawnConfigured<AHRCollectible>(World, ForestCenter + FVector(3800.f, 3600.f, 120.f), FRotator::ZeroRotator, [](AHRCollectible* Page)
	{
		Page->PersistentId = TEXT("Collectible.OldTonguePrimer");
		Page->DisplayName = LOCTEXT("Primer", "a weathered primer");
		Page->Type = EHRCollectibleType::LoreFragment;
		Page->KnowledgeId = TEXT("Lore.OldTongue");
	});
	SpawnConfigured<AHRCollectible>(World, ForestCenter + FVector(-3600.f, -3300.f, 120.f), FRotator::ZeroRotator, [](AHRCollectible* Page)
	{
		Page->PersistentId = TEXT("Collectible.HerbalistNotes");
		Page->DisplayName = LOCTEXT("Herbalist", "a herbalist's notes");
		Page->Type = EHRCollectibleType::LoreFragment;
		Page->KnowledgeId = TEXT("Lore.Alchemy");
	});
	SpawnConfigured<AHRCollectible>(World, ForestCenter + FVector(4200.f, -4100.f, 120.f), FRotator::ZeroRotator, [](AHRCollectible* Cache)
	{
		Cache->PersistentId = TEXT("Collectible.ForestCache");
		Cache->DisplayName = LOCTEXT("ForestCache", "a buried cache");
		Cache->Type = EHRCollectibleType::EchoCache;
		Cache->Amount = 400;
		Cache->bHiddenUntilRevealed = true;
	});

	SpawnConfigured<AHRClue>(World, ForestCenter + FVector(-2200.f, -2600.f, 60.f), FRotator::ZeroRotator, [](AHRClue* Clue)
	{
		Clue->ClueId = TEXT("Clue.BellRope");
		Clue->PersistentId = TEXT("Clue.BellRope");
		Clue->DisplayName = LOCTEXT("Rope", "a length of bell rope");
	});

	SpawnConfigured<AHRLandmark>(World, ForestCenter, FRotator::ZeroRotator, [](AHRLandmark* Landmark)
	{
		Landmark->LandmarkId = TEXT("Landmark.Wood");
		Landmark->DisplayName = LOCTEXT("Wood", "THE WHISPERING WOOD");
		Landmark->KnowledgeId = TEXT("Lore.Whispers");
		Landmark->SetExtent(FVector(4500.f, 4500.f, 1000.f));
	});
}

// --- Ashborn encampment (faction politics, physics, ranged enemies) ----------------------------

void AHRWorldBootstrap::BuildAshbornCamp(FRandomStream& Stream)
{
	UWorld* World = GetWorld();

	// Palisade ring with two gaps.
	const int32 Segments = 20;
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		if (Index == 0 || Index == 10)
		{
			continue;
		}
		const float Angle = Index * 2.f * PI / Segments;
		const FVector Position = CampCenter + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 1600.f + FVector(0.f, 0.f, 175.f);
		Block(Position, FVector(80.f, 520.f, 350.f), EHRPalette::Wood, FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f));
	}
	// Tents.
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const float Angle = Index * PI * 0.5f + 0.4f;
		Block(CampCenter + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 800.f + FVector(0.f, 0.f, 150.f), FVector(500.f, 400.f, 300.f), EHRPalette::Ember,
			FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f));
	}
	// Watchtower with mantle-able steps (high ground).
	const FVector Tower = CampCenter + FVector(-700.f, -700.f, 0.f);
	Block(Tower + FVector(0.f, 0.f, 300.f), FVector(400.f, 400.f, 600.f), EHRPalette::Wood);
	Block(Tower + FVector(300.f, 0.f, 100.f), FVector(200.f, 300.f, 200.f), EHRPalette::Wood);
	Block(Tower + FVector(300.f, 0.f, 300.f), FVector(200.f, 120.f, 40.f), EHRPalette::Wood);
	Block(Tower + FVector(0.f, 300.f, 400.f), FVector(300.f, 200.f, 40.f), EHRPalette::Wood);

	SpawnEnemy(CampCenter + FVector(400.f, 200.f, 0.f), EHREnemyPreset::AshbornRaider, 0.f);
	SpawnEnemy(CampCenter + FVector(-300.f, 500.f, 0.f), EHREnemyPreset::AshbornRaider, 90.f);
	if (AHREnemyCharacter* Guard = SpawnEnemy(CampCenter + FVector(1700.f, 0.f, 0.f), EHREnemyPreset::AshbornRaider, 0.f))
	{
		Guard->PatrolPoints = { FVector(0.f, 0.f, 0.f), FVector(0.f, 1500.f, 0.f) };
	}
	SpawnEnemy(CampCenter + FVector(-1700.f, 0.f, 0.f), EHREnemyPreset::AshbornRaider, 180.f);
	SpawnEnemy(Tower + FVector(0.f, 0.f, 620.f), EHREnemyPreset::AshbornArcher, 45.f);
	SpawnEnemy(CampCenter + FVector(0.f, -200.f, 0.f), EHREnemyPreset::AshbornCaptain, 0.f);

	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FVector Offset(Stream.FRandRange(-1100.f, 1100.f), Stream.FRandRange(-1100.f, 1100.f), 60.f);
		const bool bBarrel = Index < 3;
		SpawnConfigured<AHRPhysicsProp>(World, CampCenter + Offset, FRotator::ZeroRotator, [bBarrel](AHRPhysicsProp* Prop)
		{
			if (bBarrel)
			{
				Prop->MakeBarrel();
			}
		});
	}

	SpawnConfigured<AHRLootContainer>(World, CampCenter + FVector(900.f, 900.f, 30.f), FRotator(0.f, 45.f, 0.f), [](AHRLootContainer* Chest)
	{
		Chest->PersistentId = TEXT("Chest.AshbornStores");
		Chest->LootTableId = TEXT("Loot.Chest.Rare");
	});

	// Tall grass outside the walls for a stealthy approach.
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const float Angle = Index * PI / 3.f;
		SpawnConfigured<AHRStealthZone>(World, CampCenter + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 2400.f + FVector(0.f, 0.f, 55.f),
			FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f), [](AHRStealthZone* Zone) { Zone->SetZoneSize(FVector(700.f, 1200.f, 110.f)); });
	}

	SpawnConfigured<AHRLandmark>(World, CampCenter, FRotator::ZeroRotator, [](AHRLandmark* Landmark)
	{
		Landmark->LandmarkId = TEXT("Landmark.Camp");
		Landmark->DisplayName = LOCTEXT("Camp", "ASHBORN ENCAMPMENT");
		Landmark->SetExtent(FVector(2600.f, 2600.f, 800.f));
	});
}

// --- Sunken Ruins (environmental puzzles) ------------------------------------------------------

void AHRWorldBootstrap::BuildRuins(FRandomStream& Stream)
{
	UWorld* World = GetWorld();
	const FVector C = RuinsCenter;

	// Broken outer walls and pillars.
	for (int32 Index = 0; Index < 14; ++Index)
	{
		const FVector Offset(Stream.FRandRange(-3500.f, 3500.f), Stream.FRandRange(-3500.f, 3500.f), 0.f);
		const float Height = Stream.FRandRange(150.f, 700.f);
		Block(C + Offset + FVector(0.f, 0.f, Height * 0.5f), FVector(Stream.FRandRange(200.f, 900.f), 120.f, Height), EHRPalette::Ruin, FRotator(0.f, Stream.FRandRange(0.f, 180.f), 0.f));
	}

	// Three sealed vaults along the west side, each opened by a different puzzle.
	auto Vault = [this, World](const FVector& Center) -> AHRGate*
	{
		Block(Center + FVector(0.f, 450.f, 250.f), FVector(1000.f, 100.f, 500.f), EHRPalette::Ruin);
		Block(Center + FVector(0.f, -450.f, 250.f), FVector(1000.f, 100.f, 500.f), EHRPalette::Ruin);
		Block(Center + FVector(-450.f, 0.f, 250.f), FVector(100.f, 1000.f, 500.f), EHRPalette::Ruin);
		Block(Center + FVector(0.f, 0.f, 530.f), FVector(1000.f, 1000.f, 60.f), EHRPalette::Ruin);
		return SpawnConfigured<AHRGate>(World, Center + FVector(450.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f), [](AHRGate* Gate)
		{
			Gate->SetGateSize(FVector(800.f, 80.f, 500.f));
			Gate->Color = FLinearColor(0.25f, 0.25f, 0.3f);
		});
	};

	// Vault A: three dials. The glyph tells the combination if you know the Old Tongue.
	const FVector VaultA = C + FVector(-2500.f, -2500.f, 0.f);
	AHRGate* GateA = Vault(VaultA);
	FRandomStream PuzzleStream(Seed + 101);
	TArray<int32> Combination = { PuzzleStream.RandRange(1, 3), PuzzleStream.RandRange(0, 3), PuzzleStream.RandRange(1, 3) };
	TArray<AHRPuzzleElement*> Dials;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Dials.Add(SpawnConfigured<AHRPuzzleElement>(World, VaultA + FVector(1000.f, (Index - 1) * 350.f, 80.f), FRotator::ZeroRotator, [](AHRPuzzleElement* Dial)
		{
			Dial->ElementType = EHRPuzzleElementType::Dial;
			Dial->NumStates = 4;
		}));
	}
	static const TCHAR* Symbols[4] = { TEXT("the Sun"), TEXT("the Moon"), TEXT("the Flame"), TEXT("the Eye") };
	SpawnConfigured<AHRPuzzleElement>(World, VaultA + FVector(1500.f, 0.f, 100.f), FRotator(0.f, 180.f, 0.f), [&Combination](AHRPuzzleElement* Glyph)
	{
		Glyph->ElementType = EHRPuzzleElementType::Glyph;
		Glyph->RequiredKnowledge = TEXT("Lore.OldTongue");
		Glyph->GlyphText = FText::Format(LOCTEXT("RuinsGlyph", "\"Turn the stones to face {0}, {1} and {2}.\" (each turn advances Sun > Moon > Flame > Eye)"),
			FText::FromString(Symbols[Combination[0]]), FText::FromString(Symbols[Combination[1]]), FText::FromString(Symbols[Combination[2]]));
	});
	SpawnConfigured<AHRPuzzleController>(World, VaultA, FRotator::ZeroRotator, [&Dials, &Combination, GateA](AHRPuzzleController* Puzzle)
	{
		Puzzle->PuzzleId = TEXT("Puzzle.RuinsDials");
		Puzzle->Elements = TArray<TObjectPtr<AHRPuzzleElement>>(Dials);
		Puzzle->RequiredStates = Combination;
		Puzzle->GatesToOpen.Add(GateA);
		Puzzle->RewardKnowledge = TEXT("Lore.FirstFlame");
		Puzzle->SolvedText = LOCTEXT("DialsSolved", "The stones grind into place and the vault exhales.");
	});
	SpawnConfigured<AHRCollectible>(World, VaultA + FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator, [](AHRCollectible* Relic)
	{
		Relic->PersistentId = TEXT("Collectible.RuinsRelic");
		Relic->DisplayName = LOCTEXT("Relic", "a Warden's relic");
		Relic->Type = EHRCollectibleType::SkillRelic;
		Relic->Amount = 2;
	});
	SpawnConfigured<AHRLootContainer>(World, VaultA + FVector(-250.f, 250.f, 30.f), FRotator::ZeroRotator, [](AHRLootContainer* Chest)
	{
		Chest->PersistentId = TEXT("Chest.RuinsVault");
		Chest->LootTableId = TEXT("Loot.Chest.Rare");
		Chest->RequiredKeyItem = TEXT("Item.RuinsKey");
	});

	// Vault B: two braziers that only fire can light (Firebolt, firebomb, burning barrel).
	const FVector VaultB = C + FVector(-2500.f, 0.f, 0.f);
	AHRGate* GateB = Vault(VaultB);
	TArray<AHRPuzzleElement*> Braziers;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Braziers.Add(SpawnConfigured<AHRPuzzleElement>(World, VaultB + FVector(800.f, Index == 0 ? -700.f : 700.f, 50.f), FRotator::ZeroRotator, [](AHRPuzzleElement* Brazier)
		{
			Brazier->ElementType = EHRPuzzleElementType::Brazier;
		}));
	}
	SpawnConfigured<AHRPhysicsProp>(World, VaultB + FVector(1300.f, -500.f, 60.f), FRotator::ZeroRotator, [](AHRPhysicsProp* Barrel) { Barrel->MakeBarrel(); });
	SpawnConfigured<AHRPuzzleController>(World, VaultB, FRotator::ZeroRotator, [&Braziers, GateB](AHRPuzzleController* Puzzle)
	{
		Puzzle->PuzzleId = TEXT("Puzzle.RuinsBraziers");
		Puzzle->Elements = TArray<TObjectPtr<AHRPuzzleElement>>(Braziers);
		Puzzle->RequiredStates = { 1, 1 };
		Puzzle->GatesToOpen.Add(GateB);
		Puzzle->SolvedText = LOCTEXT("BraziersSolved", "Both flames roar and the seal melts away.");
	});
	SpawnConfigured<AHRCollectible>(World, VaultB + FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator, [](AHRCollectible* Relic)
	{
		Relic->PersistentId = TEXT("Collectible.GlideRelic");
		Relic->DisplayName = LOCTEXT("Feather", "an ashen feather");
		Relic->Type = EHRCollectibleType::TraversalRelic;
		Relic->Traversal = EHRTraversalAbility::Glide;
	});

	// Vault C: a pressure plate that needs something heavy left on it.
	const FVector VaultC = C + FVector(-2500.f, 2500.f, 0.f);
	AHRGate* GateC = Vault(VaultC);
	AHRPuzzleElement* Plate = SpawnConfigured<AHRPuzzleElement>(World, VaultC + FVector(1100.f, 0.f, 5.f), FRotator::ZeroRotator, [](AHRPuzzleElement* Element)
	{
		Element->ElementType = EHRPuzzleElementType::PressurePlate;
	});
	SpawnConfigured<AHRPhysicsProp>(World, VaultC + FVector(1900.f, 600.f, 60.f), FRotator::ZeroRotator, [](AHRPhysicsProp*) {});
	SpawnConfigured<AHRPuzzleController>(World, VaultC, FRotator::ZeroRotator, [Plate, GateC](AHRPuzzleController* Puzzle)
	{
		Puzzle->PuzzleId = TEXT("Puzzle.RuinsPlate");
		Puzzle->Elements.Add(Plate);
		Puzzle->RequiredStates = { 1 };
		Puzzle->GatesToOpen.Add(GateC);
		Puzzle->SolvedText = LOCTEXT("PlateSolved", "The plate sinks and stays down. A door rumbles open.");
	});
	SpawnConfigured<AHRCollectible>(World, VaultC + FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator, [](AHRCollectible* VigorSeed)
	{
		VigorSeed->PersistentId = TEXT("Collectible.RuinsSeed");
		VigorSeed->DisplayName = LOCTEXT("SeedName", "a vigor seed");
		VigorSeed->Type = EHRCollectibleType::VigorSeed;
	});

	SpawnEnemy(C + FVector(500.f, 800.f, 0.f), EHREnemyPreset::Hollow, 0.f);
	SpawnEnemy(C + FVector(-600.f, -900.f, 0.f), EHREnemyPreset::Hollow, 90.f);
	SpawnEnemy(C + FVector(1500.f, 2000.f, 0.f), EHREnemyPreset::HollowArcher, 200.f);
	SpawnEnemy(C + FVector(-800.f, 200.f, 0.f), EHREnemyPreset::HollowKnight, 0.f);

	SpawnConfigured<AHRClue>(World, C + FVector(1200.f, -1800.f, 60.f), FRotator::ZeroRotator, [](AHRClue* Clue)
	{
		Clue->ClueId = TEXT("Clue.BellLedger");
		Clue->PersistentId = TEXT("Clue.BellLedger");
		Clue->DisplayName = LOCTEXT("Ledger", "a waterlogged ledger");
	});

	SpawnConfigured<AHRLandmark>(World, C, FRotator::ZeroRotator, [](AHRLandmark* Landmark)
	{
		Landmark->LandmarkId = TEXT("Landmark.Ruins");
		Landmark->DisplayName = LOCTEXT("Ruins", "THE SUNKEN RUINS");
		Landmark->SetExtent(FVector(4000.f, 4000.f, 1000.f));
	});
}

// --- The Drowned Bell tower (investigation, vertical climb) ------------------------------------

void AHRWorldBootstrap::BuildBellTower()
{
	UWorld* World = GetWorld();
	const FVector C = BellTowerCenter;
	const float TowerHeight = 2400.f;
	Block(C + FVector(0.f, 0.f, TowerHeight * 0.5f), FVector(600.f, 600.f, TowerHeight), EHRPalette::Stone);

	// A spiral of ledges, each within mantle reach of the last.
	const int32 Steps = 14;
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		const float Angle = Index * (PI / 3.f);
		const float Z = 160.f * (Index + 1);
		const FVector Offset(FMath::Cos(Angle) * 450.f, FMath::Sin(Angle) * 450.f, Z);
		Block(C + Offset, FVector(260.f, 260.f, 40.f), EHRPalette::Wood, FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f));
	}
	const float TopZ = TowerHeight;
	SpawnConfigured<AHRClue>(World, C + FVector(0.f, 0.f, TopZ + 60.f), FRotator::ZeroRotator, [](AHRClue* Clue)
	{
		Clue->ClueId = TEXT("Clue.BellClapper");
		Clue->PersistentId = TEXT("Clue.BellClapper");
		Clue->DisplayName = LOCTEXT("Clapper", "the bell's clapper");
	});
	SpawnConfigured<AHRCollectible>(World, C + FVector(200.f, 200.f, TopZ + 100.f), FRotator::ZeroRotator, [](AHRCollectible* Secret)
	{
		Secret->PersistentId = TEXT("Collectible.TrueName");
		Secret->DisplayName = LOCTEXT("TrueName", "a name scratched into the bell frame");
		Secret->Type = EHRCollectibleType::LoreFragment;
		Secret->KnowledgeId = TEXT("Lore.TrueName");
		Secret->bHiddenUntilRevealed = true;
	});
	SpawnEnemy(C + FVector(900.f, 0.f, 0.f), EHREnemyPreset::Hollow, 0.f);

	SpawnConfigured<AHRLandmark>(World, C, FRotator::ZeroRotator, [](AHRLandmark* Landmark)
	{
		Landmark->LandmarkId = TEXT("Landmark.BellTower");
		Landmark->DisplayName = LOCTEXT("BellTower", "THE DROWNED BELL");
		Landmark->SetExtent(FVector(1500.f, 1500.f, 2000.f));
	});
}

// --- Skyreach Spire (platforming, parkour, grapple) --------------------------------------------

void AHRWorldBootstrap::BuildSpire()
{
	UWorld* World = GetWorld();
	const FVector C = SpireCenter;

	// Section 1: mantle steps up to a ledge (no unlocks needed).
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const float Height = 180.f * (Index + 1);
		Block(C + FVector(Index * 350.f, 0.f, Height * 0.5f), FVector(300.f, 400.f, Height), EHRPalette::Stone);
	}
	const FVector Ledge1 = C + FVector(1850.f, 0.f, 900.f);
	Block(Ledge1 - FVector(0.f, 0.f, 25.f), FVector(600.f, 600.f, 50.f), EHRPalette::Stone);
	Block(C + FVector(1850.f, 0.f, 437.f), FVector(150.f, 150.f, 875.f), EHRPalette::Stone);
	SpawnConfigured<AHRCollectible>(World, Ledge1 + FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, [](AHRCollectible* Relic)
	{
		Relic->PersistentId = TEXT("Collectible.DoubleJumpRelic");
		Relic->DisplayName = LOCTEXT("Boots", "the Skyreach anklets");
		Relic->Type = EHRCollectibleType::TraversalRelic;
		Relic->Traversal = EHRTraversalAbility::DoubleJump;
	});

	// Section 2: moving and crumbling platforms.
	SpawnConfigured<AHRMovingPlatform>(World, Ledge1 + FVector(600.f, 0.f, -20.f), FRotator::ZeroRotator, [](AHRMovingPlatform* Platform)
	{
		Platform->EndOffset = FVector(900.f, 0.f, 300.f);
		Platform->Period = 5.f;
	});
	const FVector Ledge2 = Ledge1 + FVector(2000.f, 0.f, 300.f);
	Block(Ledge2 - FVector(0.f, 0.f, 25.f), FVector(500.f, 500.f, 50.f), EHRPalette::Stone);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		SpawnConfigured<AHRMovingPlatform>(World, Ledge2 + FVector(0.f, 450.f + Index * 380.f, Index * 60.f), FRotator::ZeroRotator, [](AHRMovingPlatform* Platform)
		{
			Platform->bCrumbles = true;
			Platform->SetPlatformSize(FVector(220.f, 220.f, 40.f));
		});
	}
	const FVector Ledge3 = Ledge2 + FVector(0.f, 2000.f, 260.f);
	Block(Ledge3 - FVector(0.f, 0.f, 25.f), FVector(600.f, 600.f, 50.f), EHRPalette::Stone);

	// Section 3: a wall-run corridor and grapple anchors up to the summit.
	Block(Ledge3 + FVector(-1300.f, 300.f, 300.f), FVector(1800.f, 60.f, 800.f), EHRPalette::Stone);
	Block(Ledge3 + FVector(-2500.f, 0.f, -25.f), FVector(500.f, 500.f, 50.f), EHRPalette::Stone);
	SpawnConfigured<AHRGrapplePoint>(World, Ledge3 + FVector(-2500.f, 0.f, 900.f), FRotator::ZeroRotator, [](AHRGrapplePoint*) {});
	SpawnConfigured<AHRGrapplePoint>(World, Ledge3 + FVector(-2500.f, -1200.f, 1500.f), FRotator::ZeroRotator, [](AHRGrapplePoint*) {});
	const FVector Summit = Ledge3 + FVector(-2500.f, -1800.f, 1300.f);
	Block(Summit - FVector(0.f, 0.f, 25.f), FVector(700.f, 700.f, 50.f), EHRPalette::Snow);
	SpawnConfigured<AHRCollectible>(World, Summit + FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, [](AHRCollectible* VigorSeed)
	{
		VigorSeed->PersistentId = TEXT("Collectible.SpireSeed");
		VigorSeed->DisplayName = LOCTEXT("SpireSeed", "a vigor seed");
		VigorSeed->Type = EHRCollectibleType::VigorSeed;
	});
	SpawnConfigured<AHRCollectible>(World, Summit + FVector(200.f, 0.f, 100.f), FRotator::ZeroRotator, [](AHRCollectible* Cache)
	{
		Cache->PersistentId = TEXT("Collectible.SpireCache");
		Cache->DisplayName = LOCTEXT("SpireCache", "a pilgrim's offering");
		Cache->Type = EHRCollectibleType::EchoCache;
		Cache->Amount = 1200;
	});

	SpawnConfigured<AHRLandmark>(World, C + FVector(1500.f, 1000.f, 0.f), FRotator::ZeroRotator, [](AHRLandmark* Landmark)
	{
		Landmark->LandmarkId = TEXT("Landmark.Spire");
		Landmark->DisplayName = LOCTEXT("Spire", "SKYREACH SPIRE");
		Landmark->SetExtent(FVector(3000.f, 3000.f, 3000.f));
	});
}

// --- Ashen Peak (survival cold, boss arena, ending) --------------------------------------------

void AHRWorldBootstrap::BuildMountain()
{
	UWorld* World = GetWorld();
	const FVector C = MountainCenter;

	// Directions the ramps climb from, rotating around the mountain. Ramp 0 faces the village.
	const FVector Sides[4] = { FVector(0.f, 1.f, 0.f), FVector(1.f, 0.f, 0.f), FVector(0.f, -1.f, 0.f), FVector(-1.f, 0.f, 0.f) };

	for (int32 Terrace = 0; Terrace < TerraceCount; ++Terrace)
	{
		const float Half = TerraceHalf0 - Terrace * TerraceStep;
		const float Top = (Terrace + 1) * TerraceRise;
		const EHRPalette Palette = Top > 1500.f ? EHRPalette::Snow : EHRPalette::Stone;
		Block(C + FVector(0.f, 0.f, Top * 0.5f), FVector(Half * 2.f, Half * 2.f, Top), Palette);

		// Ramp up to this terrace from the level below.
		const FVector Side = Sides[Terrace % 4];
		const float BelowZ = Terrace * TerraceRise;
		const float Run = TerraceStep - 20.f;
		const FVector Base = C + Side * (Half + Run) + FVector(0.f, 0.f, BelowZ);
		Ramp(Base, -Side, Run, TerraceRise, 600.f, Palette);
	}

	const float SummitZ = TerraceCount * TerraceRise;
	const float SummitHalf = TerraceHalf0 - (TerraceCount - 1) * TerraceStep;

	// The last shrine before the boss, one terrace down.
	const float ShrineTerraceHalf = SummitHalf + TerraceStep;
	const FVector LastSide = Sides[(TerraceCount - 1) % 4];
	const FVector ShrineLocation = C + LastSide * (ShrineTerraceHalf - 300.f) + FVector(0.f, 0.f, SummitZ - TerraceRise + 25.f)
		+ FVector::CrossProduct(LastSide, FVector::UpVector) * 900.f;
	SpawnConfigured<AHRCheckpoint>(World, ShrineLocation, FRotator::ZeroRotator, [](AHRCheckpoint* Shrine)
	{
		Shrine->PersistentId = TEXT("Shrine.Peak");
		Shrine->DisplayName = LOCTEXT("PeakShrine", "Frostbitten Shrine");
	});

	// Arena walls with an entrance where the final ramp arrives.
	const float WallHeight = 450.f;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector Side = Sides[Index];
		const FVector Tangent = FVector::CrossProduct(Side, FVector::UpVector);
		const FVector WallCenter = C + Side * (SummitHalf - 40.f) + FVector(0.f, 0.f, SummitZ + WallHeight * 0.5f);
		const bool bEntrance = Side.Equals(LastSide);
		const FVector Size = FMath::Abs(Side.X) > 0.5f ? FVector(80.f, SummitHalf * 2.f, WallHeight) : FVector(SummitHalf * 2.f, 80.f, WallHeight);
		if (!bEntrance)
		{
			Block(WallCenter, Size, EHRPalette::Ruin);
			continue;
		}
		const float SegmentLength = SummitHalf - 400.f;
		const FVector SegmentSize = FMath::Abs(Side.X) > 0.5f ? FVector(80.f, SegmentLength, WallHeight) : FVector(SegmentLength, 80.f, WallHeight);
		Block(WallCenter + Tangent * (400.f + SegmentLength * 0.5f), SegmentSize, EHRPalette::Ruin);
		Block(WallCenter - Tangent * (400.f + SegmentLength * 0.5f), SegmentSize, EHRPalette::Ruin);
	}

	AHRGate* FogGate = SpawnConfigured<AHRGate>(World, C + LastSide * (SummitHalf - 40.f) + FVector(0.f, 0.f, SummitZ), FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(LastSide.Y, LastSide.X)) + 90.f, 0.f),
		[](AHRGate* Gate)
		{
			Gate->bIsFogGate = true;
			Gate->bStartsOpen = true;
			Gate->SetGateSize(FVector(820.f, 60.f, 450.f));
		});

	SpawnConfigured<AHRBossCharacter>(World, C + FVector(0.f, 0.f, SummitZ + 150.f), FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(LastSide.Y, LastSide.X)), 0.f), [FogGate](AHRBossCharacter* Boss)
	{
		Boss->BossId = TEXT("Boss.Warden");
		Boss->RequiredCampaignFlag = TEXT("Boss.PaleKeeper.Defeated");
		Boss->DisplayName = LOCTEXT("Warden", "THE HOLLOW WARDEN");
		Boss->EnemyTypeId = TEXT("Enemy.Warden");
		Boss->RewardItem = TEXT("Item.WardenSigil");
		Boss->SkillPointReward = 2;
		Boss->ArenaGates.Add(FogGate);

		FHRBossPhase Oath;
		Oath.SkillLevel = 0.5f;
		Oath.Aggression = 0.55f;
		Oath.PerilousChance = 0.15f;
		Oath.Color = FLinearColor(0.35f, 0.3f, 0.4f);
		FHRBossPhase Fury;
		Fury.PhaseLine = LOCTEXT("WardenPhase2", "\"The oath... still... binds me!\"");
		Fury.Modifiers = { FHRStatModifier(EHRStat::PhysicalPower, 8.f), FHRStatModifier(EHRStat::MoveSpeed, 0.f, 1.15f) };
		Fury.SkillLevel = 0.6f;
		Fury.Aggression = 0.7f;
		Fury.PerilousChance = 0.25f;
		Fury.Color = FLinearColor(0.5f, 0.15f, 0.2f);
		FHRBossPhase Hollow;
		Hollow.PhaseLine = LOCTEXT("WardenPhase3", "The Warden's armor splits. Something hollow and burning pours out.");
		Hollow.Modifiers = { FHRStatModifier(EHRStat::PhysicalPower, 15.f), FHRStatModifier(EHRStat::MoveSpeed, 0.f, 1.3f), FHRStatModifier(EHRStat::Defense, -10.f) };
		Hollow.SkillLevel = 0.7f;
		Hollow.Aggression = 0.9f;
		Hollow.PerilousChance = 0.35f;
		Hollow.Color = FLinearColor(0.8f, 0.3f, 0.05f);
		Boss->Phases = { Oath, Fury, Hollow };
		Boss->Hints = {
			LOCTEXT("WardenHint1", "His posture breaks before his body does. Deflect, don't dodge, when he strings blows together."),
			LOCTEXT("WardenHint2", "When his blade glows red: sidestep the thrust, then punish the long recovery."),
			LOCTEXT("WardenHint3", "Fire unsettles what's inside him. Firebolts and firebombs stagger his final form."),
			LOCTEXT("WardenHint4", "You can sneak up the last ramp crouched; a takedown before he notices counts as a deathblow."),
		};
		Boss->GetCombat()->DefaultWeapon.BaseDamage = 30.f;
		Boss->GetCombat()->DefaultWeapon.BasePosture = 22.f;
	});

	SpawnConfigured<AHREndingAltar>(World, C - LastSide * (SummitHalf - 500.f) + FVector(0.f, 0.f, SummitZ + 50.f), FRotator::ZeroRotator, [](AHREndingAltar*) {});

	// Guards along the climb.
	for (int32 Terrace = 0; Terrace < TerraceCount - 1; ++Terrace)
	{
		const float Half = TerraceHalf0 - (Terrace + 1) * TerraceStep;
		const FVector Side = Sides[(Terrace + 1) % 4];
		const FVector Location = C + Side * (Half + TerraceStep * 0.5f) + FVector(0.f, 0.f, (Terrace + 1) * TerraceRise)
			+ FVector::CrossProduct(Side, FVector::UpVector) * 1200.f;
		SpawnEnemy(Location, Terrace >= 2 ? EHREnemyPreset::FrostHollow : EHREnemyPreset::Hollow, 0.f);
	}

	// Ore veins on the slopes.
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const FVector Side = Sides[Index % 4];
		const float Half = TerraceHalf0 - (Index % 3) * TerraceStep;
		const FVector Location = C + Side * (Half - 300.f) + FVector::CrossProduct(Side, FVector::UpVector) * (Index * 500.f - 1500.f)
			+ FVector(0.f, 0.f, ((Index % 3) + 1) * TerraceRise + 40.f);
		SpawnConfigured<AHRResourceNode>(World, Location, FRotator(0.f, Index * 40.f, 20.f), [](AHRResourceNode* Node)
		{
			Node->ItemId = TEXT("Item.IronOre");
			Node->DisplayName = LOCTEXT("Ore", "an iron vein");
			Node->BonusItemId = TEXT("Item.Moonstone");
			Node->BonusChance = 0.08f;
			Node->RespawnHours = 48.f;
			Node->Color = FLinearColor(0.4f, 0.35f, 0.3f);
		});
	}

	SpawnConfigured<AHRLandmark>(World, C + FVector(0.f, 0.f, 0.f), FRotator::ZeroRotator, [](AHRLandmark* Landmark)
	{
		Landmark->LandmarkId = TEXT("Landmark.Peak");
		Landmark->DisplayName = LOCTEXT("Peak", "ASHEN PEAK");
		Landmark->SetExtent(FVector(7000.f, 7000.f, 4000.f));
	});
}

// --- Trial of Echoes (procedural dungeon) ------------------------------------------------------

void AHRWorldBootstrap::BuildTrial()
{
	UWorld* World = GetWorld();
	SpawnConfigured<AHRDungeonGenerator>(World, TrialOrigin, FRotator::ZeroRotator, [this](AHRDungeonGenerator* Dungeon)
	{
		Dungeon->Seed = Seed ^ 0x5eed;
	});
	SpawnConfigured<AHRLandmark>(World, TrialOrigin + FVector(-1500.f, 0.f, 0.f), FRotator::ZeroRotator, [](AHRLandmark* Landmark)
	{
		Landmark->LandmarkId = TEXT("Landmark.Trial");
		Landmark->DisplayName = LOCTEXT("Trial", "THE TRIAL OF ECHOES");
		Landmark->KnowledgeId = TEXT("Lore.Trial");
		Landmark->StartsQuest = TEXT("Quest.Trial");
		Landmark->SetExtent(FVector(800.f, 1500.f, 800.f));
	});
	// A road marker leading east.
	for (int32 Index = 1; Index < 8; ++Index)
	{
		Block(FVector(Index * 2400.f + 2000.f, -1500.f, 150.f), FVector(80.f, 80.f, 300.f), EHRPalette::Stone);
	}
}

// --- Seeded scatter ----------------------------------------------------------------------------

void AHRWorldBootstrap::BuildScatter(FRandomStream& Stream)
{
	UWorld* World = GetWorld();
	auto RandomWildPoint = [&Stream]()
	{
		// Anywhere outside the village and away from the mountain's footprint.
		for (int32 Attempt = 0; Attempt < 20; ++Attempt)
		{
			const FVector Point(Stream.FRandRange(-25000.f, 25000.f), Stream.FRandRange(-12000.f, 25000.f), 0.f);
			if (Point.Size2D() > 3500.f && FVector::Dist2D(Point, MountainCenter) > TerraceHalf0 + 1500.f && FVector::Dist2D(Point, TrialOrigin) > 6000.f)
			{
				return Point;
			}
		}
		return FVector(15000.f, 15000.f, 0.f);
	};

	// Rocks as cover.
	for (int32 Index = 0; Index < 45; ++Index)
	{
		const FVector Point = RandomWildPoint();
		const float Size = Stream.FRandRange(150.f, 500.f);
		Block(Point + FVector(0.f, 0.f, Size * 0.4f), FVector(Size, Size * 0.8f, Size), EHRPalette::Stone, FRotator(Stream.FRandRange(-10.f, 10.f), Stream.FRandRange(0.f, 360.f), 0.f));
	}

	// Chests, some locked, some not what they seem.
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const FVector Point = RandomWildPoint() + FVector(0.f, 0.f, 30.f);
		SpawnConfigured<AHRLootContainer>(World, Point, FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f), [Index](AHRLootContainer* Chest)
		{
			Chest->PersistentId = FName(*FString::Printf(TEXT("Chest.Wild%d"), Index));
			Chest->LootTableId = Index % 4 == 0 ? FName(TEXT("Loot.Chest.Rare")) : FName(TEXT("Loot.Chest.Common"));
			Chest->MimicChance = 0.12f;
		});
	}

	// Wandering patrols.
	for (int32 Index = 0; Index < 8; ++Index)
	{
		const EHREnemyPreset Preset = Index % 4 == 3 ? EHREnemyPreset::HollowArcher : EHREnemyPreset::Hollow;
		SpawnEnemy(RandomWildPoint(), Preset, Stream.FRandRange(0.f, 360.f));
	}

	// Ore and herbs across the land.
	for (int32 Index = 0; Index < 14; ++Index)
	{
		const bool bOre = Index % 2 == 0;
		SpawnConfigured<AHRResourceNode>(World, RandomWildPoint() + FVector(0.f, 0.f, 40.f), FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f), [bOre](AHRResourceNode* Node)
		{
			Node->ItemId = bOre ? FName(TEXT("Item.IronOre")) : FName(TEXT("Item.Herb"));
			Node->DisplayName = bOre ? LOCTEXT("OreScatter", "an iron vein") : LOCTEXT("HerbScatter", "herbs");
			Node->Color = bOre ? FLinearColor(0.4f, 0.35f, 0.3f) : FLinearColor(0.2f, 0.7f, 0.2f);
			if (!bOre)
			{
				Node->GetMesh()->SetStaticMesh(HRGreybox::Sphere());
				Node->GetMesh()->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.4f));
			}
		});
	}

	// Secrets for the thorough: hidden caches and relics with no markers.
	for (int32 Index = 0; Index < 5; ++Index)
	{
		SpawnConfigured<AHRCollectible>(World, RandomWildPoint() + FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, [Index](AHRCollectible* Secret)
		{
			Secret->PersistentId = FName(*FString::Printf(TEXT("Collectible.Wild%d"), Index));
			Secret->DisplayName = Index == 0 ? LOCTEXT("WildRelic", "a forgotten relic") : LOCTEXT("WildCache", "a hidden cache");
			Secret->Type = Index == 0 ? EHRCollectibleType::SkillRelic : EHRCollectibleType::EchoCache;
			Secret->Amount = Index == 0 ? 1 : 300;
			Secret->bHiddenUntilRevealed = true;
		});
	}
}

#undef LOCTEXT_NAMESPACE
