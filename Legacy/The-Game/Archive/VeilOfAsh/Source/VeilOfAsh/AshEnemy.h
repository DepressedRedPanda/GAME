#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AshEnemy.generated.h"
class UStaticMeshComponent;
class AAshHero;
UCLASS()
class VEILOFASH_API AAshEnemy : public ACharacter
{
    GENERATED_BODY()
public:
    AAshEnemy();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Boss, BlueprintReadOnly) bool bBoss = false;
    UFUNCTION() void OnRep_Boss();
    UPROPERTY(Replicated, BlueprintReadOnly) float HP = 55.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float MaxHP = 55.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float Poise = 60.f;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bTelegraph = false;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bStaggered = false;
    void SetBoss(bool bIsBoss);
    void ReceiveStrike(float Damage, float Stagger, AAshHero* Source);
    void ReceiveCounter(AAshHero* Source);
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    FVector SpawnPoint;
    float AttackClock = 0.f;
    float StrikeMoment = 0.f;
    float StaggerUntil = 0.f;
    float RegainTime = 0.f;
    TWeakObjectPtr<AAshHero> TargetHero;
    void Die(AAshHero* Killer);
};
