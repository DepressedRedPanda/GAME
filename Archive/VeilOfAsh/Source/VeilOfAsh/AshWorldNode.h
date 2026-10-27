#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AshTypes.h"
#include "AshWorldNode.generated.h"
class UStaticMeshComponent;
class AAshHero;
UCLASS()
class VEILOFASH_API AAshWorldNode : public AActor
{
    GENERATED_BODY()
public:
    AAshWorldNode();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Shape;
    UPROPERTY(ReplicatedUsing=OnRep_Kind, EditAnywhere, BlueprintReadOnly) EAshNodeKind Kind = EAshNodeKind::Wood;
    UFUNCTION() void OnRep_Kind();
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly) int32 Stock = 1;
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly) FString Label;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    void Configure(EAshNodeKind Type, int32 Amount = 1);
    void Interact(AAshHero* Hero);
    FString Describe() const;
private:
    void ApplySharedAppearance();
};
