#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AshTypes.h"
#include "AshHero.generated.h"
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UAshVitals;
class UAshInventory;
class UAshProgression;
class AAshEnemy;
class AAshWorldNode;
class AAshCompanion;

UCLASS()
class VEILOFASH_API AAshHero : public ACharacter
{
    GENERATED_BODY()
public:
    AAshHero();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UAshVitals> Vitals;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UAshInventory> Inventory;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UAshProgression> Progression;
    UPROPERTY(Replicated, BlueprintReadOnly) FVector Checkpoint = FVector(0,0,120);
    UPROPERTY(Replicated, BlueprintReadOnly) bool bGuarding = false;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bRevealed = false; // spectral form: reduced enemy perception while focus lasts
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Deaths = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bPermanentDeath = false;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 CompanionMode = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) float VehicleBoostUntil = 0.f;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Kills = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) TObjectPtr<AAshCompanion> Companion;
    UPROPERTY(BlueprintReadOnly) FString LastMessage;
    UFUNCTION(Client, Reliable) void ClientNotice(const FString& Text);
    UFUNCTION(Server, Reliable) void ServerAction(int32 ActionCode);
    UFUNCTION(Server, Reliable) void ServerInteract(AActor* Target);
    UFUNCTION(Server, Reliable) void ServerBuild();
    UFUNCTION(Server, Reliable) void ServerSkill(int32 Branch);
    UFUNCTION(Server, Reliable) void ServerClass(int32 ClassIndex);
    UFUNCTION(Server, Reliable) void ServerEnding(int32 Option);
    UFUNCTION(Server, Reliable) void ServerSave(bool bLoad);
    void DeliverHit(AAshEnemy* Enemy, float Damage, bool bHeavy);
    void SetCheckpoint(const FVector& Place);
    void AwardKill(int32 Experience);
    void OnDeath();
    AAshWorldNode* FindInteractable(float Range = 340.f) const;
    FString NearbyName() const;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Boom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Figure;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Lantern;
    bool bSprinting = false;
    float LastAttack = -20.f;
    float LastDodge = -20.f;
    float LastSpell = -20.f;
    int32 Combo = 0;
    bool bDeathHandled = false;
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void SprintStart();
    void SprintEnd();
    void CrouchToggle();
    void Attack();
    void Heavy();
    void Parry();
    void Dodge();
    void GuardStart();
    void GuardEnd();
    void Spell();
    void Interact();
    void Build();
    void CraftRation();
    void CraftTonic();
    void CraftBlade();
    void UseRation();
    void UseTonic();
    void UpgradeMight();
    void UpgradeGuile();
    void UpgradeInsight();
    void ChooseVanguard();
    void ChooseSeer();
    void ChooseShade();
    void CompanionOrder();
    void EndingOne();
    void EndingTwo();
    void EndingThree();
    void Save();
    void Load();
    void Respawn();
    void Spectral();
    void Mantle();
};
