#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AshPhysicsCrate.generated.h"
class UStaticMeshComponent;
/** Physics playground prop: can be shoved, used for cover, or placed on platforms. */
UCLASS()
class VEILOFASH_API AAshPhysicsCrate : public AActor
{
    GENERATED_BODY()
public:
    AAshPhysicsCrate();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shape;
};
