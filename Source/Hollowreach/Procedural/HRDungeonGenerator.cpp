#include "Procedural/HRDungeonGenerator.h"
#include "SharedArtLibrary.h"

#include "Characters/HRBossCharacter.h"
#include "Characters/HREnemyCharacter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRStatics.h"
#include "Puzzles/HRPuzzleController.h"
#include "Puzzles/HRPuzzleElement.h"
#include "TimerManager.h"
#include "World/HRCheckpoint.h"
#include "World/HRLootContainer.h"
#include "World/HRPhysicsProp.h"

#define LOCTEXT_NAMESPACE "HRDungeon"

namespace
{
	const FName TrialSource(TEXT("TrialModifiers"));
	const FIntPoint Directions[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
}

// --- Portal ------------------------------------------------------------------------------------

AHRTrialPortal::AHRTrialPortal()
{
	Mesh->SetStaticMesh(HRGreybox::Cylinder());
	Mesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 0.1f));
	Color = FLinearColor(0.5f, 0.3f, 1.f);
}

FText AHRTrialPortal::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	return LOCTEXT("Descend", "Descend deeper into the Trial");
}

void AHRTrialPortal::Interact(AHRPlayerCharacter* Player)
{
	AHRDungeonGenerator* Dungeon = Generator.Get();
	if (!Dungeon)
	{
		return;
	}
	const int32 NewSeed = Dungeon->Seed * 31 + 7919 * (Dungeon->Depth + 1);
	Player->TeleportTo(Dungeon->GetEntranceLocation(), FRotator::ZeroRotator);
	Dungeon->Regenerate(NewSeed, true); // Destroys this portal.
}

// --- Generator ---------------------------------------------------------------------------------

AHRDungeonGenerator::AHRDungeonGenerator()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Floors = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Floors"));
	Floors->SetupAttachment(RootComponent);
	Floors->SetStaticMesh(HRGreybox::Cube());
	Floors->SetMobility(EComponentMobility::Movable);
	Floors->SetCollisionProfileName(TEXT("BlockAll"));

	Walls = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Walls"));
	Walls->SetupAttachment(RootComponent);
	Walls->SetStaticMesh(HRGreybox::Cube());
	Walls->SetMobility(EComponentMobility::Movable);
	Walls->SetCollisionProfileName(TEXT("BlockAll"));

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetupAttachment(RootComponent);
	Bounds->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Bounds->SetGenerateOverlapEvents(true);

	auto AddModifier = [this](const TCHAR* Name, TArray<FHRStatModifier> PlayerMods, float EnemyScale, int32 Extra, float Mimic)
	{
		FHRRunModifier Modifier;
		Modifier.Name = FText::FromString(Name);
		Modifier.PlayerModifiers = PlayerMods;
		Modifier.EnemyScale = EnemyScale;
		Modifier.ExtraEnemiesPerRoom = Extra;
		Modifier.MimicChance = Mimic;
		PossibleModifiers.Add(Modifier);
	};
	AddModifier(TEXT("Frenzy: the Hollow hit harder"), {}, 1.35f, 0, 0.1f);
	AddModifier(TEXT("Glass Cannon: you hit harder, and break easier"), { FHRStatModifier(EHRStat::PhysicalPower, 0.f, 1.4f), FHRStatModifier(EHRStat::Defense, 0.f, 0.5f) }, 1.f, 0, 0.1f);
	AddModifier(TEXT("Swarm: more of them"), {}, 0.85f, 1, 0.1f);
	AddModifier(TEXT("Shadowed Halls: easier to stay unseen"), { FHRStatModifier(EHRStat::Stealth, 0.3f) }, 1.f, 0, 0.1f);
	AddModifier(TEXT("Greed: chests may bite"), { FHRStatModifier(EHRStat::LootLuck, 0.5f) }, 1.f, 0, 0.45f);
	AddModifier(TEXT("Iron Skin: tougher but slower"), { FHRStatModifier(EHRStat::Defense, 25.f), FHRStatModifier(EHRStat::MoveSpeed, 0.f, 0.9f) }, 1.f, 0, 0.1f);
}

void AHRDungeonGenerator::BeginPlay()
{
	Super::BeginPlay();
	HRGreybox::Tint(Floors, FLinearColor(0.12f, 0.11f, 0.1f));
	HRGreybox::Tint(Walls, FLinearColor(0.22f, 0.2f, 0.24f));
	USharedArtLibrary::ApplySurface(Floors, TEXT("Dungeon.Floor"));
	USharedArtLibrary::ApplySurface(Walls, TEXT("Dungeon.Wall"));
	Bounds->OnComponentBeginOverlap.AddDynamic(this, &AHRDungeonGenerator::HandleBoundsBegin);
	Bounds->OnComponentEndOverlap.AddDynamic(this, &AHRDungeonGenerator::HandleBoundsEnd);
	if (bGenerateOnBeginPlay)
	{
		Generate();
	}
}

FVector AHRDungeonGenerator::RoomCenter(const FHRDungeonRoom& Room) const
{
	return FVector(Room.Cell.X * RoomSize, Room.Cell.Y * RoomSize, 0.f);
}

FVector AHRDungeonGenerator::GetEntranceLocation() const
{
	return GetActorTransform().TransformPosition(FVector(-RoomSize * 0.5f - 400.f, 0.f, 120.f));
}

void AHRDungeonGenerator::Clear()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Reset();
	Floors->ClearInstances();
	Walls->ClearInstances();
	Rooms.Reset();
	CellToRoom.Reset();
	ActiveModifiers.Reset();
	GetWorldTimerManager().ClearTimer(BossCheckTimer);
	Boss = nullptr;
	bPortalSpawned = false;
}

void AHRDungeonGenerator::Regenerate(int32 NewSeed, bool bDeeper)
{
	Clear();
	Seed = NewSeed;
	if (bDeeper)
	{
		++Depth;
	}
	Generate();
	UHRStatics::Notify(this, FText::Format(LOCTEXT("Depth", "TRIAL OF ECHOES - Depth {0}"), Depth), FLinearColor(0.7f, 0.5f, 1.f), true);
	UHRStatics::Notify(this, GetModifierSummary(), FLinearColor(0.8f, 0.7f, 1.f));
}

void AHRDungeonGenerator::Generate()
{
	FRandomStream Stream(Seed);
	BuildLayout(Stream);
	BuildGeometry();

	// Roll two different run modifiers.
	if (PossibleModifiers.Num() > 0)
	{
		const int32 First = Stream.RandRange(0, PossibleModifiers.Num() - 1);
		ActiveModifiers.Add(First);
		if (PossibleModifiers.Num() > 1)
		{
			int32 Second = Stream.RandRange(0, PossibleModifiers.Num() - 2);
			Second += Second >= First ? 1 : 0;
			ActiveModifiers.Add(Second);
		}
	}
	PopulateRooms(Stream);
	GetWorldTimerManager().SetTimer(BossCheckTimer, this, &AHRDungeonGenerator::CheckBoss, 1.f, true);
}

void AHRDungeonGenerator::BuildLayout(FRandomStream& Stream)
{
	auto AddRoom = [this](const FIntPoint& Cell)
	{
		FHRDungeonRoom Room;
		Room.Cell = Cell;
		const int32 Index = Rooms.Add(Room);
		CellToRoom.Add(Cell, Index);
		return Index;
	};
	auto Connect = [this](int32 A, int32 B, int32 Direction)
	{
		Rooms[A].Connections |= 1 << Direction;
		Rooms[B].Connections |= 1 << (Direction ^ 1);
	};

	AddRoom(FIntPoint(0, 0));
	FIntPoint Current(0, 0);
	for (int32 Guard = 0; Rooms.Num() < RoomCount && Guard < 5000; ++Guard)
	{
		// Occasionally branch from an existing room instead of continuing the walk.
		if (Stream.FRand() < 0.3f)
		{
			Current = Rooms[Stream.RandRange(0, Rooms.Num() - 1)].Cell;
		}
		const int32 Direction = Stream.RandRange(0, 3);
		const FIntPoint Next = Current + Directions[Direction];
		if (Next.X < 0)
		{
			continue; // Keep the entrance side open.
		}
		const int32 CurrentIndex = CellToRoom.FindChecked(Current);
		if (const int32* Existing = CellToRoom.Find(Next))
		{
			if (!(Rooms[CurrentIndex].Connections & (1 << Direction)) && Stream.FRand() < LoopChance)
			{
				Connect(CurrentIndex, *Existing, Direction);
			}
		}
		else
		{
			Connect(CurrentIndex, AddRoom(Next), Direction);
		}
		Current = Next;
	}

	// Distances from the entrance (BFS).
	for (FHRDungeonRoom& Room : Rooms)
	{
		Room.Distance = -1;
	}
	TArray<int32> Queue = { 0 };
	Rooms[0].Distance = 0;
	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const FHRDungeonRoom Room = Rooms[Queue[Head]];
		for (int32 Direction = 0; Direction < 4; ++Direction)
		{
			if (!(Room.Connections & (1 << Direction)))
			{
				continue;
			}
			const int32 Neighbor = CellToRoom.FindChecked(Room.Cell + Directions[Direction]);
			if (Rooms[Neighbor].Distance < 0)
			{
				Rooms[Neighbor].Distance = Room.Distance + 1;
				Queue.Add(Neighbor);
			}
		}
	}

	// Room roles.
	int32 BossIndex = 0;
	for (int32 Index = 0; Index < Rooms.Num(); ++Index)
	{
		if (Rooms[Index].Distance > Rooms[BossIndex].Distance)
		{
			BossIndex = Index;
		}
	}
	bool bPuzzlePlaced = false;
	for (int32 Index = 0; Index < Rooms.Num(); ++Index)
	{
		FHRDungeonRoom& Room = Rooms[Index];
		const int32 Doors = FMath::CountBits(Room.Connections);
		if (Index == 0)
		{
			Room.Type = EHRRoomType::Start;
		}
		else if (Index == BossIndex)
		{
			Room.Type = EHRRoomType::Boss;
		}
		else if (Doors == 1)
		{
			Room.Type = bPuzzlePlaced ? EHRRoomType::Treasure : EHRRoomType::Puzzle;
			bPuzzlePlaced = true;
		}
		else
		{
			Room.Type = EHRRoomType::Combat;
		}
	}
}

void AHRDungeonGenerator::AddWall(const FVector& Center, bool bAlongX, float Length)
{
	const FVector Scale = bAlongX
		? FVector(Length / 100.f, WallThickness / 100.f, WallHeight / 100.f)
		: FVector(WallThickness / 100.f, Length / 100.f, WallHeight / 100.f);
	Walls->AddInstance(FTransform(FRotator::ZeroRotator, Center, Scale));
}

void AHRDungeonGenerator::BuildGeometry()
{
	FVector Min(TNumericLimits<float>::Max());
	FVector Max(-TNumericLimits<float>::Max());

	for (const FHRDungeonRoom& Room : Rooms)
	{
		const FVector Center = RoomCenter(Room);
		Min = Min.ComponentMin(Center);
		Max = Max.ComponentMax(Center);
		Floors->AddInstance(FTransform(FRotator::ZeroRotator, Center - FVector(0.f, 0.f, 25.f), FVector(RoomSize / 100.f, RoomSize / 100.f, 0.5f)));

		for (int32 Direction = 0; Direction < 4; ++Direction)
		{
			const FIntPoint NeighborCell = Room.Cell + Directions[Direction];
			const bool bHasNeighbor = CellToRoom.Contains(NeighborCell);
			const bool bNegative = Direction == 1 || Direction == 3;
			// Shared walls are built once, by the room on the negative side.
			if (bNegative && bHasNeighbor)
			{
				continue;
			}
			const bool bConnected = (Room.Connections & (1 << Direction)) != 0 || (Room.Type == EHRRoomType::Start && Direction == 1);
			const bool bAlongX = Direction >= 2; // Walls on the Y edges run along X.
			const FVector Offset = FVector(Directions[Direction].X, Directions[Direction].Y, 0.f) * RoomSize * 0.5f;
			const FVector EdgeCenter = Center + Offset + FVector(0.f, 0.f, WallHeight * 0.5f);
			if (!bConnected)
			{
				AddWall(EdgeCenter, bAlongX, RoomSize + WallThickness);
				continue;
			}
			const float Segment = (RoomSize - DoorWidth) * 0.5f;
			const FVector Tangent = bAlongX ? FVector(1.f, 0.f, 0.f) : FVector(0.f, 1.f, 0.f);
			AddWall(EdgeCenter + Tangent * (DoorWidth * 0.5f + Segment * 0.5f), bAlongX, Segment + WallThickness);
			AddWall(EdgeCenter - Tangent * (DoorWidth * 0.5f + Segment * 0.5f), bAlongX, Segment + WallThickness);
		}
	}

	const FVector Extent = (Max - Min) * 0.5f + FVector(RoomSize * 0.5f, RoomSize * 0.5f, 0.f);
	Bounds->SetBoxExtent(FVector(Extent.X, Extent.Y, WallHeight));
	Bounds->SetRelativeLocation((Max + Min) * 0.5f + FVector(0.f, 0.f, WallHeight * 0.5f));
}

AActor* AHRDungeonGenerator::SpawnTracked(UClass* Class, const FVector& Location, const FRotator& Rotation)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* Actor = GetWorld()->SpawnActor<AActor>(Class, GetActorTransform().TransformPosition(Location), Rotation, Params);
	if (Actor)
	{
		SpawnedActors.Add(Actor);
	}
	return Actor;
}

void AHRDungeonGenerator::SpawnEnemy(const FVector& Location, FRandomStream& Stream, bool bElite)
{
	AHREnemyCharacter* Enemy = Cast<AHREnemyCharacter>(SpawnTracked(AHREnemyCharacter::StaticClass(), Location + FVector(0.f, 0.f, 100.f),
		FRotator(0.f, Stream.FRandRange(0.f, 360.f), 0.f)));
	if (!Enemy)
	{
		return;
	}
	float Scale = 1.f + 0.2f * (Depth - 1);
	for (const int32 Index : ActiveModifiers)
	{
		Scale *= PossibleModifiers[Index].EnemyScale;
	}
	Enemy->EnemyTypeId = TEXT("Enemy.TrialHollow");
	Enemy->DisplayName = bElite ? LOCTEXT("Elite", "Echo Knight") : LOCTEXT("Wraith", "Trial Wraith");
	Enemy->LootTableId = bElite ? FName(TEXT("Loot.Chest.Common")) : FName(TEXT("Loot.Hollow"));
	Enemy->SkillLevel = FMath::Clamp(0.25f + 0.07f * Depth + (bElite ? 0.2f : 0.f), 0.f, 0.85f);
	Enemy->Aggression = 0.6f;
	Enemy->PerilousChance = bElite ? 0.2f : 0.08f;
	Enemy->WanderRadius = 300.f;
	if (bElite)
	{
		Enemy->SetVisualScale(1.25f);
		Enemy->GetCombat()->PoiseThreshold = 15.f;
		Scale *= 1.8f;
	}
	Enemy->ApplyDifficultyScale(Scale);
}

void AHRDungeonGenerator::SpawnBoss(const FVector& Location)
{
	const FTransform SpawnTransform(FRotator(0.f, 180.f, 0.f), GetActorTransform().TransformPosition(Location + FVector(0.f, 0.f, 150.f)));
	AHRBossCharacter* Guardian = GetWorld()->SpawnActorDeferred<AHRBossCharacter>(AHRBossCharacter::StaticClass(), SpawnTransform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Guardian)
	{
		return;
	}
	Guardian->BossId = FName(*FString::Printf(TEXT("Boss.Trial.%d"), Seed));
	Guardian->DisplayName = FText::Format(LOCTEXT("Guardian", "Guardian of Depth {0}"), Depth);
	Guardian->EnemyTypeId = TEXT("Enemy.TrialGuardian");
	Guardian->EchoReward = 800 * Depth;
	Guardian->SkillPointReward = 1;

	FHRBossPhase First;
	First.SkillLevel = 0.45f;
	FHRBossPhase Second;
	Second.PhaseLine = LOCTEXT("GuardianPhase", "The Guardian splits into furious echoes!");
	Second.Modifiers = { FHRStatModifier(EHRStat::PhysicalPower, 10.f), FHRStatModifier(EHRStat::MoveSpeed, 0.f, 1.2f) };
	Second.SkillLevel = 0.6f;
	Second.Aggression = 0.85f;
	Second.PerilousChance = 0.3f;
	Second.Color = FLinearColor(0.6f, 0.15f, 0.7f);
	Guardian->Phases = { First, Second };
	Guardian->Hints = {
		LOCTEXT("GuardianHint1", "Its thrust glows red before it lunges. Dodge sideways, never block."),
		LOCTEXT("GuardianHint2", "Deflect its combos: its posture cracks faster than its health falls.")
	};
	Guardian->FinishSpawning(SpawnTransform);
	Guardian->ApplyDifficultyScale(0.6f + 0.25f * Depth);
	SpawnedActors.Add(Guardian);
	Boss = Guardian;
}

void AHRDungeonGenerator::PopulateRooms(FRandomStream& Stream)
{
	int32 ExtraEnemies = 0;
	float MimicChance = 0.1f;
	for (const int32 Index : ActiveModifiers)
	{
		ExtraEnemies += PossibleModifiers[Index].ExtraEnemiesPerRoom;
		MimicChance = FMath::Max(MimicChance, PossibleModifiers[Index].MimicChance);
	}

	int32 ChestIndex = 0;
	for (const FHRDungeonRoom& Room : Rooms)
	{
		const FVector Center = RoomCenter(Room);
		auto RandomPoint = [&Stream, this, &Center]()
		{
			return Center + FVector(Stream.FRandRange(-0.3f, 0.3f) * RoomSize, Stream.FRandRange(-0.3f, 0.3f) * RoomSize, 0.f);
		};

		switch (Room.Type)
		{
		case EHRRoomType::Start:
		{
			// The shrine keeps a fixed id so it stays kindled across regenerations.
			const FTransform SpawnTransform(FRotator::ZeroRotator, GetActorTransform().TransformPosition(Center + FVector(0.f, RoomSize * 0.3f, 25.f)));
			AHRCheckpoint* Shrine = GetWorld()->SpawnActorDeferred<AHRCheckpoint>(AHRCheckpoint::StaticClass(), SpawnTransform);
			if (Shrine)
			{
				Shrine->PersistentId = TEXT("Shrine.Trial");
				Shrine->DisplayName = LOCTEXT("TrialShrine", "Threshold of Echoes");
				Shrine->bHasForge = true;
				Shrine->FinishSpawning(SpawnTransform);
				SpawnedActors.Add(Shrine);
			}
			break;
		}
		case EHRRoomType::Combat:
		{
			const int32 Count = FMath::Min(5, 1 + Depth / 2 + (Room.Distance > 3 ? 1 : 0) + ExtraEnemies);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				SpawnEnemy(RandomPoint(), Stream, Stream.FRand() < 0.15f);
			}
			const int32 Props = Stream.RandRange(0, 2);
			for (int32 Index = 0; Index < Props; ++Index)
			{
				if (AHRPhysicsProp* Prop = Cast<AHRPhysicsProp>(SpawnTracked(AHRPhysicsProp::StaticClass(), RandomPoint() + FVector(0.f, 0.f, 60.f))))
				{
					if (Stream.FRand() < 0.4f)
					{
						Prop->MakeBarrel();
					}
				}
			}
			break;
		}
		case EHRRoomType::Treasure:
		{
			const FTransform SpawnTransform(FRotator::ZeroRotator, GetActorTransform().TransformPosition(Center + FVector(0.f, 0.f, 30.f)));
			AHRLootContainer* Chest = GetWorld()->SpawnActorDeferred<AHRLootContainer>(AHRLootContainer::StaticClass(), SpawnTransform);
			if (Chest)
			{
				Chest->PersistentId = FName(*FString::Printf(TEXT("Trial.%d.Chest%d"), Seed, ChestIndex++));
				Chest->LootTableId = Depth >= 3 ? FName(TEXT("Loot.Boss")) : FName(TEXT("Loot.Chest.Rare"));
				Chest->MimicChance = MimicChance;
				Chest->FinishSpawning(SpawnTransform);
				SpawnedActors.Add(Chest);
			}
			break;
		}
		case EHRRoomType::Puzzle:
		{
			// Three levers that must be pulled in the right order. The glyph tells the order,
			// if you can read the Old Tongue. Otherwise: experiment.
			TArray<int32> Order = { 0, 1, 2 };
			for (int32 Index = Order.Num() - 1; Index > 0; --Index)
			{
				Order.Swap(Index, Stream.RandRange(0, Index));
			}
			static const TCHAR* Names[3] = { TEXT("left"), TEXT("middle"), TEXT("right") };
			TArray<AHRPuzzleElement*> Levers;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const FTransform SpawnTransform(FRotator::ZeroRotator, GetActorTransform().TransformPosition(Center + FVector(-300.f, (Index - 1) * 300.f, 60.f)));
				if (AHRPuzzleElement* Lever = GetWorld()->SpawnActorDeferred<AHRPuzzleElement>(AHRPuzzleElement::StaticClass(), SpawnTransform))
				{
					Lever->ElementType = EHRPuzzleElementType::Lever;
					Lever->FinishSpawning(SpawnTransform);
					SpawnedActors.Add(Lever);
					Levers.Add(Lever);
				}
			}
			if (Levers.Num() < 3)
			{
				break;
			}

			// The controller binds to its elements in BeginPlay, so fill it before it finishes spawning.
			const FTransform ControllerTransform(FRotator::ZeroRotator, GetActorTransform().TransformPosition(Center));
			if (AHRPuzzleController* Controller = GetWorld()->SpawnActorDeferred<AHRPuzzleController>(AHRPuzzleController::StaticClass(), ControllerTransform))
			{
				for (const int32 LeverIndex : Order)
				{
					Controller->Elements.Add(Levers[LeverIndex]);
					Controller->RequiredStates.Add(1);
				}
				Controller->bOrderedSequence = true;
				Controller->PuzzleId = FName(*FString::Printf(TEXT("Trial.%d.Puzzle"), Seed));
				Controller->RewardEchoes = 400 * Depth;
				Controller->SolvedText = LOCTEXT("TrialPuzzle", "The levers lock in place. Echoes pour from the walls.");
				Controller->FinishSpawning(ControllerTransform);
				SpawnedActors.Add(Controller);
			}

			const FTransform GlyphTransform(FRotator::ZeroRotator, GetActorTransform().TransformPosition(Center + FVector(300.f, 0.f, 100.f)));
			if (AHRPuzzleElement* Glyph = GetWorld()->SpawnActorDeferred<AHRPuzzleElement>(AHRPuzzleElement::StaticClass(), GlyphTransform))
			{
				Glyph->ElementType = EHRPuzzleElementType::Glyph;
				Glyph->RequiredKnowledge = TEXT("Lore.OldTongue");
				Glyph->GlyphText = FText::Format(LOCTEXT("TrialGlyph", "\"First the {0}, then the {1}, last the {2}.\""),
					FText::FromString(Names[Order[0]]), FText::FromString(Names[Order[1]]), FText::FromString(Names[Order[2]]));
				Glyph->FinishSpawning(GlyphTransform);
				SpawnedActors.Add(Glyph);
			}
			break;
		}
		case EHRRoomType::Boss:
			SpawnBoss(Center);
			break;
		}
	}
}

void AHRDungeonGenerator::CheckBoss()
{
	const AHRBossCharacter* Guardian = Boss.Get();
	if (bPortalSpawned || !Guardian || !Guardian->IsDead())
	{
		return;
	}
	bPortalSpawned = true;
	const FVector Location = GetActorTransform().InverseTransformPosition(Guardian->GetSpawnTransform().GetLocation()) - FVector(0.f, 0.f, 140.f);
	if (AHRTrialPortal* Portal = Cast<AHRTrialPortal>(SpawnTracked(AHRTrialPortal::StaticClass(), Location)))
	{
		Portal->Generator = this;
	}
	UHRStatics::Notify(this, LOCTEXT("PortalOpen", "A portal opens deeper into the Trial."), FLinearColor(0.7f, 0.5f, 1.f), true);
}

FText AHRDungeonGenerator::GetModifierSummary() const
{
	FString Summary;
	for (const int32 Index : ActiveModifiers)
	{
		if (!Summary.IsEmpty())
		{
			Summary += TEXT("   |   ");
		}
		Summary += PossibleModifiers[Index].Name.ToString();
	}
	return FText::FromString(Summary);
}

void AHRDungeonGenerator::HandleBoundsBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AHRPlayerCharacter* Player = Cast<AHRPlayerCharacter>(OtherActor);
	if (!Player || OtherComp != Player->GetRootComponent())
	{
		return;
	}
	TArray<FHRStatModifier> Modifiers;
	for (const int32 Index : ActiveModifiers)
	{
		Modifiers.Append(PossibleModifiers[Index].PlayerModifiers);
	}
	Player->GetAttributes()->SetModifierSource(TrialSource, Modifiers);
	UHRStatics::Notify(this, FText::Format(LOCTEXT("Enter", "Trial of Echoes - Depth {0}: {1}"), Depth, GetModifierSummary()), FLinearColor(0.75f, 0.6f, 1.f));
}

void AHRDungeonGenerator::HandleBoundsEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AHRPlayerCharacter* Player = Cast<AHRPlayerCharacter>(OtherActor);
	if (Player && OtherComp == Player->GetRootComponent())
	{
		Player->GetAttributes()->RemoveModifierSource(TrialSource);
	}
}

#undef LOCTEXT_NAMESPACE
