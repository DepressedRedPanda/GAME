#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AshTypes.h"
#include "AshInventory.generated.h"

UCLASS(ClassGroup=(Ash), meta=(BlueprintSpawnableComponent))
class VEILOFASH_API UAshInventory : public UActorComponent
{
    GENERATED_BODY()
public:
    UAshInventory();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(Replicated, BlueprintReadOnly) TArray<FAshStack> Items;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 WeaponTier = 1;
    int32 Count(FName Item) const;
    bool Add(FName Item, int32 Amount);
    bool Consume(FName Item, int32 Amount);
    bool Craft(FName Recipe); // Ration: Herb x2 + Wood x1; Tonic: Herb x3; Blade: Ore x3 + Relic x1
    FString Describe() const;
};
