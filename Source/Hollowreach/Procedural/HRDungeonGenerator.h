#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/HRInteractableActor.h"
#include "Core/HRTypes.h"
#include "HRDungeonGenerator.generated.h"

class UInstancedStaticMeshComponent;
class UBoxComponent;
class AHRDungeonGenerator;

UENUM(BlueprintType)
enum class EHRRoomType : uint8
{
	Start,
	Combat,
	Treasure,
	Puzzle,
	Boss
};

/** One cell of the generated layout. */
struct FHRDungeonRoom
{
	FIntPoint Cell = FIntPoint::ZeroValue;
	uint8 Connections = 0; // Bits: 0=+X, 1=-X, 2=+Y, 3=-Y
	int32 Distance = 0;
	EHRRoomType Type = EHRRoomType::Combat;
};

/** A run modifier ("curse" or "blessing") applied to one generated trial. */
USTRUCT(BlueprintType)
struct FHRRunModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") TArray<FHRStatModifier> PlayerModifiers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") float EnemyScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") int32 ExtraEnemiesPerRoom = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") float MimicChance = 0.1f;
};

/** Leaves a cleared trial and generates a deeper, harder one (roguelite loop). */
UCLASS()
class HOLLOWREACH_API AHRTrialPortal : public AHRInteractableActor
{
	GENERATED_BODY()

public:
	AHRTrialPortal();
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	TWeakObjectPtr<AHRDungeonGenerator> Generator;
};

/**
 * Procedural "Trial of Echoes": a seeded random-walk room graph with loops, dead-end treasure
 * rooms, a boss in the furthest room and a shrine at the entrance. Each generation rolls run
 * modifiers. Clear the boss and step through the portal to descend deeper with a new seed
 * (Binding of Isaac / Nightreign-style repeated challenges).
 */
UCLASS()
class HOLLOWREACH_API AHRDungeonGenerator : public AActor
{
	GENERATED_BODY()

public:
	AHRDungeonGenerator();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Dungeon") void Generate();
	UFUNCTION(BlueprintCallable, Category="Dungeon") void Regenerate(int32 NewSeed, bool bDeeper);
	UFUNCTION(BlueprintCallable, Category="Dungeon") void Clear();
	FVector GetEntranceLocation() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") int32 Seed = 1337;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") int32 Depth = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") int32 RoomCount = 11;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") float RoomSize = 1800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") float WallHeight = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") float WallThickness = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") float DoorWidth = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") float LoopChance = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") bool bGenerateOnBeginPlay = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon") TArray<FHRRunModifier> PossibleModifiers;

	UFUNCTION(BlueprintPure, Category="Dungeon") FText GetModifierSummary() const;

protected:
	void BuildLayout(FRandomStream& Stream);
	void BuildGeometry();
	void PopulateRooms(FRandomStream& Stream);
	void AddWall(const FVector& Center, bool bAlongX, float Length);
	FVector RoomCenter(const FHRDungeonRoom& Room) const;
	void SpawnEnemy(const FVector& Location, FRandomStream& Stream, bool bElite);
	void SpawnBoss(const FVector& Location);
	AActor* SpawnTracked(UClass* Class, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	void CheckBoss();

	UFUNCTION() void HandleBoundsBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION() void HandleBoundsEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UInstancedStaticMeshComponent> Floors;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UInstancedStaticMeshComponent> Walls;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UBoxComponent> Bounds;

	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> SpawnedActors;

	TArray<FHRDungeonRoom> Rooms;
	TMap<FIntPoint, int32> CellToRoom;
	TArray<int32> ActiveModifiers;
	FTimerHandle BossCheckTimer;
	TWeakObjectPtr<class AHRBossCharacter> Boss;
	bool bPortalSpawned = false;
};
