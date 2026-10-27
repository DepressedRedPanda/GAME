#pragma once
#include "CoreMinimal.h"
#include "AshTypes.generated.h"

UENUM(BlueprintType)
enum class EAshClass : uint8 { Vanguard, Seer, Shade };

UENUM(BlueprintType)
enum class EAshWeather : uint8 { Clear, Rain, Ashstorm };

UENUM(BlueprintType)
enum class EAshNodeKind : uint8
{
    Wood, Ore, Herb, Relic, Lore, Cipher, Shrine, Villager,
    CultEnvoy, FreeEnvoy, Farm, Forge, Gate, Shelter, Vehicle, Rune
};

UENUM(BlueprintType)
enum class EAshCompanionOrder : uint8 { Follow, Guard, Hunt };

USTRUCT(BlueprintType)
struct FAshStack
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Item = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Count = 0;
    FAshStack() = default;
    FAshStack(FName InItem, int32 InCount) : Item(InItem), Count(InCount) {}
};
