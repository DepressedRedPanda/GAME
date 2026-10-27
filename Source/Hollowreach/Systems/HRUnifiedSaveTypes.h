#pragma once

#include "CoreMinimal.h"
#include "HRUnifiedSaveTypes.generated.h"

USTRUCT()
struct FHRPlacedShelter
{
    GENERATED_BODY()

    UPROPERTY() FName CheckpointId;
    UPROPERTY() FTransform Transform;
};
