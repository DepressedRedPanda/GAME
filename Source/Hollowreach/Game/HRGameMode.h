#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HRGameMode.generated.h"

class AHRPlayerCharacter;
class AHREnemyCharacter;
class AHRCheckpoint;
class AHRDroppedEchoes;
class AHRWorldBootstrap;
class UHRSaveGame;

/**
 * Rules of the world: Souls-style death (drop your echoes, respawn at the last shrine, enemies
 * return), resting, fast travel, blood-moon hordes, and booting the greybox world.
 */
UCLASS()
class HOLLOWREACH_API AHRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHRGameMode();

	virtual void StartPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	void RegisterEnemy(AHREnemyCharacter* Enemy);
	void RegisterCheckpoint(AHRCheckpoint* Checkpoint);
	void RespawnEnemies();

	void HandlePlayerDeath(AHRPlayerCharacter* Player, AActor* Killer);
	void OnPlayerRested(AHRPlayerCharacter* Player, AHRCheckpoint* Checkpoint);
	void FastTravel(AHRPlayerCharacter* Player, AHRCheckpoint* Destination);
	void ClearDroppedEchoes() { DroppedEchoes = nullptr; }

	AHRCheckpoint* FindCheckpoint(FName CheckpointId) const;
	TArray<AHRCheckpoint*> GetDiscoveredCheckpoints() const;
	FName GetLastCheckpointId() const { return LastCheckpointId; }
	int32 GetDeathCount() const { return DeathCount; }

	void WriteToSave(UHRSaveGame& Save) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") bool bBuildGreyboxWorld = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") bool bAutoLoadSave = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") int32 WorldSeed = 20261012;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") TSubclassOf<AHRWorldBootstrap> BootstrapClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") int32 HordeSize = 6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") float RespawnDelay = 4.f;

private:
	UFUNCTION() void HandleNightfall();
	UFUNCTION() void HandleDawn();
	void RespawnPlayer();
	void SpawnDroppedEchoes(const FVector& Location, int32 Amount);
	AHRCheckpoint* GetRespawnCheckpoint() const;

	TArray<TWeakObjectPtr<AHREnemyCharacter>> Enemies;
	TArray<TWeakObjectPtr<AHREnemyCharacter>> Horde;
	TArray<TWeakObjectPtr<AHRCheckpoint>> Checkpoints;
	TWeakObjectPtr<AHRDroppedEchoes> DroppedEchoes;
	TWeakObjectPtr<AHRPlayerCharacter> DeadPlayer;
	FName LastCheckpointId;
	int32 DeathCount = 0;
	bool bTeleportToCheckpointOnStart = false;
	FTimerHandle RespawnTimer;
};
