#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AshTypes.h"
#include "AshSave.generated.h"
UCLASS()
class VEILOFASH_API UAshSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FVector Position = FVector(0,0,120);
    UPROPERTY() FVector Checkpoint = FVector(0,0,120);
    UPROPERTY() TArray<FAshStack> Items;
    UPROPERTY() int32 Level = 1;
    UPROPERTY() int32 XP = 0;
    UPROPERTY() int32 SkillPoints = 0;
    UPROPERTY() int32 Might = 0;
    UPROPERTY() int32 Guile = 0;
    UPROPERTY() int32 Insight = 0;
    UPROPERTY() int32 WeaponTier = 1;
    UPROPERTY() int32 FreefolkReputation = 0;
    UPROPERTY() int32 ChoirReputation = 0;
    UPROPERTY() int32 HeroClass = 0;
    UPROPERTY() float Health = 100;
    UPROPERTY() float Hunger = 100;
    UPROPERTY() float Thirst = 100;
    UPROPERTY() float Infection = 0;
    UPROPERTY() int32 Kills = 0;
};
