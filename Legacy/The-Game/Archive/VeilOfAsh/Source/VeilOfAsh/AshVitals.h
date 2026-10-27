#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AshVitals.generated.h"

UCLASS(ClassGroup=(Ash), meta=(BlueprintSpawnableComponent))
class VEILOFASH_API UAshVitals : public UActorComponent
{
    GENERATED_BODY()
public:
    UAshVitals();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(Replicated, BlueprintReadOnly) float Health = 100.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float MaxHealth = 100.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float Stamina = 100.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float Focus = 60.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float Hunger = 100.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float Thirst = 100.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float Infection = 0.f;
    UPROPERTY(Replicated, BlueprintReadOnly) float Injury = 0.f;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bDead = false;
    bool SpendStamina(float Amount);
    bool SpendFocus(float Amount);
    void Heal(float Amount);
    // Returns true if parried. Dodge immunity and parry only valid on server.
    bool ReceiveHit(float Damage, float Now);
    void StartParry(float Now);
    void StartDodge(float Now);
    void RestoreAtShrine();
    bool IsDodging(float Now) const { return Now < DodgeUntil; }
    float DodgeExpiry() const { return DodgeUntil; }
private:
    float ParryUntil = 0.f;
    float DodgeUntil = 0.f;
};
