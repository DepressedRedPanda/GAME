#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AshCompanion.generated.h"
class UStaticMeshComponent;
class AAshHero;
UCLASS()
class VEILOFASH_API AAshCompanion : public AActor
{
    GENERATED_BODY()
public:
    AAshCompanion();
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(Replicated) TObjectPtr<AAshHero> Leader;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Shape;
private:
    float LastAssist=0.f;
};
