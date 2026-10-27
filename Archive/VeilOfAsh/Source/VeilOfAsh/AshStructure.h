#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AshStructure.generated.h"
class UStaticMeshComponent;
UCLASS()
class VEILOFASH_API AAshStructure : public AActor
{
    GENERATED_BODY()
public:
    AAshStructure();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shape;
};
