#include "AshGameMode.h"
#include "AshHero.h"
#include "AshHUD.h"
#include "AshEnemy.h"
#include "AshWorldNode.h"
#include "AshWorldState.h"
#include "AshBlock.h"
#include "AshPhysicsCrate.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AAshGameMode::AAshGameMode()
{
    PrimaryActorTick.bCanEverTick = false;
    DefaultPawnClass = AAshHero::StaticClass();
    HUDClass = AAshHUD::StaticClass();
    GameStateClass = AAshWorldState::StaticClass();
}
void AAshGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (AAshWorldState* State = GetGameState<AAshWorldState>()) State->Seed = WorldSeed;
    BuildTestWorld();
    // Works even in a new blank level with no PlayerStart. Otherwise GameMode auto-spawns it.
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (!PC->GetPawn())
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            if (AAshHero* Pawn = GetWorld()->SpawnActor<AAshHero>(AAshHero::StaticClass(), FVector(0,0,170), FRotator::ZeroRotator, Params)) PC->Possess(Pawn);
        }
        else if (AAshHero* Pawn = Cast<AAshHero>(PC->GetPawn()))
        {
            if (Pawn->GetActorLocation().Z < 110.f) Pawn->SetActorLocation(FVector(0,0,170));
        }
    }
}
void AAshGameMode::Tick(float DT) { Super::Tick(DT); }
void AAshGameMode::PutBlock(const FVector& Where, const FVector& Scale, bool bSolid)
{
    FTransform T(FRotator::ZeroRotator, Where, Scale);
    if (AAshBlock* Block = GetWorld()->SpawnActorDeferred<AAshBlock>(AAshBlock::StaticClass(), T))
    {
        Block->SetReplicates(true);
        Block->FinishSpawning(T);
        Block->ConfigureScale(Scale);
        Block->Shape->SetCollisionEnabled(bSolid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }
}
void AAshGameMode::PutNode(EAshNodeKind Type, const FVector& Where, int32 Stock)
{
    if (AAshWorldNode* Node = GetWorld()->SpawnActor<AAshWorldNode>(AAshWorldNode::StaticClass(), Where, FRotator::ZeroRotator))
        Node->Configure(Type, Stock);
}
void AAshGameMode::BuildTestWorld()
{
    if (!HasAuthority()) return;
    // An 88m square collision-ground world. Use World Partition when replacing it with a production world.
    PutBlock(FVector(0,0,-80), FVector(88,88,1.5));
    FRandomStream R(WorldSeed);
    for (int32 i=0; i<42; ++i)
    {
        const FVector Where(R.FRandRange(-3800,3800), R.FRandRange(-3800,3800), 70);
        if (Where.Size2D() < 620 || FVector::Dist2D(Where,FVector(2800,2800,0)) < 480) continue;
        const FVector Scale(R.FRandRange(.5,1.6),R.FRandRange(.5,1.6),R.FRandRange(1.0,2.0));
        PutBlock(Where,Scale);
    }
    // Elevated ruins form a traversal route; player jump and mantle can reach the top.
    PutBlock(FVector(-800,800,0),FVector(3,3,.5));
    PutBlock(FVector(-950,610,80),FVector(1,1,1));
    PutBlock(FVector(-950,800,155),FVector(1,1,1));
    // Two complementary but non-exclusive routes to the encounter.
    PutNode(EAshNodeKind::Shrine, FVector(0,-420,100));
    PutNode(EAshNodeKind::Forge, FVector(-350,200,95));
    PutNode(EAshNodeKind::Villager, FVector(430,250,100));
    PutNode(EAshNodeKind::Farm, FVector(-640,-220,100));
    PutNode(EAshNodeKind::Vehicle, FVector(300,-450,100));
    PutNode(EAshNodeKind::CultEnvoy, FVector(450,650,100));
    PutNode(EAshNodeKind::FreeEnvoy, FVector(-450,600,100));
    PutNode(EAshNodeKind::Shelter, FVector(100,950,100));
    PutNode(EAshNodeKind::Gate, FVector(1840,1830,100));
    PutNode(EAshNodeKind::Rune, FVector(-1650,600,100),1);
    PutNode(EAshNodeKind::Rune, FVector(1200,-1100,100),2);
    PutNode(EAshNodeKind::Rune, FVector(1200,1250,100),3);
    PutNode(EAshNodeKind::Cipher,FVector(-2250,2100,100));
    PutNode(EAshNodeKind::Cipher,FVector(2420,-2100,100));
    PutNode(EAshNodeKind::Cipher,FVector(-2400,-2350,100));
    PutNode(EAshNodeKind::Cipher,FVector(550,2650,100)); // optional alternate path
    PutNode(EAshNodeKind::Lore,FVector(-900,930,250));
    PutNode(EAshNodeKind::Lore,FVector(1800,-1650,100));
    PutNode(EAshNodeKind::Lore,FVector(-1300,-2300,100));
    for(int32 i=0; i<28; ++i)
    {
        FVector P(R.FRandRange(-3000,3000),R.FRandRange(-3000,3000),105);
        if (P.Size2D()<530) continue;
        const int32 Roll=R.RandRange(0,5);
        const EAshNodeKind Type=Roll<2 ? EAshNodeKind::Wood : (Roll<4 ? EAshNodeKind::Herb : (Roll==4 ? EAshNodeKind::Ore : EAshNodeKind::Relic));
        PutNode(Type,P,R.RandRange(1,3));
    }
    for(int32 i=0; i<EnemyPopulation; ++i)
    {
        FVector P(R.FRandRange(-3100,3100),R.FRandRange(-3100,3100),150);
        if (P.Size2D()<950 || FVector::Dist2D(P,FVector(2800,2800,0))<650) continue;
        GetWorld()->SpawnActor<AAshEnemy>(AAshEnemy::StaticClass(),P,FRotator::ZeroRotator);
    }
    if (AAshEnemy* Boss = GetWorld()->SpawnActor<AAshEnemy>(AAshEnemy::StaticClass(),FVector(2800,2800,175),FRotator::ZeroRotator)) Boss->SetBoss(true);
    // Physics interactions, independent of the quest route.
    for (int32 i=0;i<6;++i)
        GetWorld()->SpawnActor<AAshPhysicsCrate>(AAshPhysicsCrate::StaticClass(),
            FVector(150.f+i*125.f,-850.f,160.f), FRotator::ZeroRotator);
    // Meshes are visible without starter content / authored assets.
    if (ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,3000),FRotator(-53,18,0)))
    { Sun->SetReplicates(true); Sun->GetLightComponent()->SetIntensity(2.5f); }
    const FVector Lights[] = {FVector(0,0,1000),FVector(-2100,2000,1000),FVector(2800,2800,1100),FVector(2200,-2200,1100)};
    for (const FVector& P: Lights)
        if (APointLight* L = GetWorld()->SpawnActor<APointLight>(P,FRotator::ZeroRotator))
        { L->SetReplicates(true); L->GetLightComponent()->SetIntensity(90000.f); L->GetLightComponent()->SetAttenuationRadius(4000.f); }
}
