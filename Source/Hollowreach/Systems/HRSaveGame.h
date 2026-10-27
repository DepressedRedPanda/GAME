#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/HRTypes.h"
#include "Items/HRItemTypes.h"
#include "Narrative/HRNarrativeTypes.h"
#include "Systems/HRUnifiedSaveTypes.h"
#include "HRSaveGame.generated.h"

/** Everything that persists between sessions. World objects persist through narrative flags. */
UCLASS()
class HOLLOWREACH_API UHRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 2;
	UPROPERTY() int32 SaveVersion = CurrentVersion;
	UPROPERTY() FDateTime Timestamp;

	// Player
	UPROPERTY() FName LastCheckpoint;
	UPROPERTY() int32 Level = 1;
	UPROPERTY() int32 Echoes = 0;
	UPROPERTY() int32 SkillPoints = 0;
	UPROPERTY() EHRStartingClass StartingClass = EHRStartingClass::Unchosen;
	UPROPERTY() bool bHasVitals = false;
	UPROPERTY() bool bHasPlayerTransform = false;
	UPROPERTY() FTransform PlayerTransform;
	UPROPERTY() float HealthFraction = 1.f;
	UPROPERTY() float StaminaFraction = 1.f;
	UPROPERTY() float FocusFraction = 1.f;
	UPROPERTY() TMap<EHRAttribute, int32> Attributes;
	UPROPERTY() TArray<FName> Skills;
	UPROPERTY() TArray<EHRTraversalAbility> Traversal;
	UPROPERTY() TArray<FName> AbilitySlots;
	UPROPERTY() TArray<FHRItemInstance> Items;
	UPROPERTY() TMap<EHREquipSlot, FGuid> Equipped;
	UPROPERTY() FName QuickItem;
	UPROPERTY() int32 MaxFlasks = 4;
	UPROPERTY() TArray<EHRInjury> Injuries;
	UPROPERTY() float Food = 100.f;
	UPROPERTY() float Water = 100.f;
	UPROPERTY() float Warmth = 80.f;
	UPROPERTY() float Fatigue = 0.f;
	UPROPERTY() float Infection = 0.f;
	UPROPERTY() int32 TotalKills = 0;

	// World
	UPROPERTY() TMap<FName, int32> Flags;
	UPROPERTY() TArray<FName> Knowledge;
	UPROPERTY() TArray<FName> Clues;
	UPROPERTY() TArray<FName> Deductions;
	UPROPERTY() FName Ending;
	UPROPERTY() TMap<FName, float> Reputation;
	UPROPERTY() TArray<FHRQuestProgress> Quests;
	UPROPERTY() float TimeOfDay = 8.f;
	UPROPERTY() int32 WorldDay = 1;
	UPROPERTY() float HoursUntilWeatherChange = 4.f;
	UPROPERTY() EHRWeather Weather = EHRWeather::Clear;
	UPROPERTY() int32 WorldSeed = 0;
	UPROPERTY() int32 DeathCount = 0;
	UPROPERTY() TArray<FHRPlacedShelter> Shelters;
	UPROPERTY() bool bHasCompanionState = false;
	UPROPERTY() int32 CompanionCommand = 0;
	UPROPERTY() FVector CompanionHoldLocation = FVector::ZeroVector;
	UPROPERTY() int32 CompanionRevives = 1;
	UPROPERTY() bool bImportedHollowreach = false;
	UPROPERTY() bool bImportedVeil = false;
	UPROPERTY() bool bFreshStart = false;
	UPROPERTY() FName LegacyHollowreachEnding;
	UPROPERTY() TMap<FName, int32> LegacyUnmappedItems;

	// Lost echoes waiting at the place you died.
	UPROPERTY() bool bHasDroppedEchoes = false;
	UPROPERTY() FVector DroppedEchoesLocation = FVector::ZeroVector;
	UPROPERTY() int32 DroppedEchoesAmount = 0;
};
