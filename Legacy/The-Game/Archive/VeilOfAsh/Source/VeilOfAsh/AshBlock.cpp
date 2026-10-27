#include "AshBlock.h"
#include "SharedArtLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
AAshBlock::AAshBlock()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true);
    Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerrainBlock"));
    RootComponent = Shape;
    Shape->SetMobility(EComponentMobility::Movable);
    Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Shape->SetCollisionProfileName(TEXT("BlockAll"));
}

void AAshBlock::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(AAshBlock,BlockScale);
}
void AAshBlock::BeginPlay()
{
    Super::BeginPlay();
    USharedArtLibrary::ApplySurface(Shape, TEXT("Stone"));
}
void AAshBlock::ConfigureScale(const FVector& Scale)
{
    if (!HasAuthority()) return;
    BlockScale=Scale;
    OnRep_BlockScale();
}
void AAshBlock::OnRep_BlockScale() { SetActorScale3D(BlockScale); }
