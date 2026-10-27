#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AshBlock.generated.h"
class UStaticMeshComponent;

/** Simple replicated collision block, so the entire generated arena needs no assets. */
UCLASS()
class VEILOFASH_API AAshBlock : public AActor
{
    GENERATED_BODY()
public:
    AAshBlock();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shape;
    UPROPERTY(ReplicatedUsing=OnRep_BlockScale) FVector BlockScale = FVector::OneVector;
    UFUNCTION() void OnRep_BlockScale();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    void ConfigureScale(const FVector& Scale);
};
