#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Systems/HRUnifiedSaveTypes.h"
#include "HRSaveSubsystem.generated.h"

class UHRSaveGame;
class AHRPlayerCharacter;

/**
 * Saves at every rest. Loading happens in two halves: world state (flags, quests, factions,
 * time) before actors begin play so they can read it, then player state once the player exists.
 */
UCLASS()
class HOLLOWREACH_API UHRSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Save") bool SaveProgress(AHRPlayerCharacter* Player);
	UFUNCTION(BlueprintPure, Category="Save") bool HasSave() const;
	UFUNCTION(BlueprintCallable, Category="Save") bool DeleteSave();

	/** Loads the slot and restores world-level state. Returns the save for the game mode to read. */
	UHRSaveGame* LoadWorldState();
	void ApplyPendingPlayerState(AHRPlayerCharacter* Player);
	UHRSaveGame* GetLoadedSave() const { return LoadedSave; }
	void RestoreShelters(UWorld* World);
	void RememberShelter(FName Id, const FTransform& Transform);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Save") FString SlotName = TEXT("Hollowreach_Unified_Slot0");

private:
	UPROPERTY() TObjectPtr<UHRSaveGame> LoadedSave;
	bool bPlayerStatePending = false;
	bool bUnsupportedSaveVersion = false;
	UHRSaveGame* ImportLegacySaves();
	void ResetWorldState();
	UPROPERTY() TArray<FHRPlacedShelter> PlacedShelters;
};
