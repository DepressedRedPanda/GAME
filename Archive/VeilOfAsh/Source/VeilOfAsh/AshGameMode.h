#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AshTypes.h"
#include "AshGameMode.generated.h"
class UStaticMesh;
class AAshBlock;
class AAshWorldNode;
UCLASS()
class VEILOFASH_API AAshGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AAshGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DT) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 WorldSeed = 1729;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EnemyPopulation = 18;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllowPVP = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIronSoulPermadeath = false;
private:
    TArray<TObjectPtr<AAshBlock>> GeneratedGeometry;
    TObjectPtr<UStaticMesh> CubeMesh;
    TObjectPtr<UStaticMesh> SphereMesh;
    void BuildTestWorld();
    void PutBlock(const FVector& Where, const FVector& Scale, bool bSolid = true);
    void PutNode(EAshNodeKind Type, const FVector& Where, int32 Stock=1);
};
