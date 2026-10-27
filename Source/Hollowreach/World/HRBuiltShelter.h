#pragma once

#include "CoreMinimal.h"
#include "World/HRCheckpoint.h"
#include "HRBuiltShelter.generated.h"

UCLASS()
class HOLLOWREACH_API AHRBuiltShelter : public AHRCheckpoint
{
    GENERATED_BODY()
public:
    AHRBuiltShelter();
    static bool BuildFor(AHRPlayerCharacter* Player);
};
