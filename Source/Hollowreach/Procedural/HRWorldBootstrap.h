#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/HRTypes.h"
#include "HRWorldBootstrap.generated.h"

class UInstancedStaticMeshComponent;
class AHREnemyCharacter;

UENUM()
enum class EHRPalette : uint8
{
	Ground,
	Stone,
	Wood,
	Snow,
	Ruin,
	Ember,
	Foliage,
	Water,
	MAX UMETA(Hidden)
};

UENUM()
enum class EHREnemyPreset : uint8
{
	Hollow,
	HollowArcher,
	HollowKnight,
	AshbornRaider,
	AshbornArcher,
	AshbornCaptain,
	FrostHollow
};

/**
 * Builds the greybox open world at runtime so the whole game is playable from an empty level:
 * lighting and sky, terrain, a village hub, a forest, a bandit camp, puzzle ruins, a bell tower
 * mystery, a parkour spire, a mountain with the Warden's arena, the procedural Trial, and
 * seeded scatter (resources, chests, patrols, secrets). Each region exercises a set of systems.
 * Replace region by region with real levels; the gameplay actors work the same when hand-placed.
 */
UCLASS()
class HOLLOWREACH_API AHRWorldBootstrap : public AActor
{
	GENERATED_BODY()

public:
	AHRWorldBootstrap();

	void BuildWorld(int32 InSeed);

protected:
	void BuildEnvironment();
	void BuildTerrain();
	void BuildVillage();
	void BuildForest(FRandomStream& Stream);
	void BuildAshbornCamp(FRandomStream& Stream);
	void BuildRuins(FRandomStream& Stream);
	void BuildBellTower();
	void BuildSpire();
	void BuildMountain();
	void BuildTrial();
	void BuildVeilChapter();
	void BuildScatter(FRandomStream& Stream);

	/** Adds a greybox block. Size is in centimetres, Center is the block's centre. */
	void Block(const FVector& Center, const FVector& Size, EHRPalette Palette, const FRotator& Rotation = FRotator::ZeroRotator);
	/** A ramp rising Rise cm over Run cm towards Direction (unit, horizontal), starting at Base. */
	void Ramp(const FVector& Base, const FVector& Direction, float Run, float Rise, float Width, EHRPalette Palette);
	void Tree(const FVector& Base, float Height);

	AHREnemyCharacter* SpawnEnemy(const FVector& Location, EHREnemyPreset Preset, float Yaw = 0.f);

	UPROPERTY(VisibleAnywhere, Category="Components") TArray<TObjectPtr<UInstancedStaticMeshComponent>> Palettes;

	int32 Seed = 0;
	bool bBuilt = false;
};
