#include "AshStructure.h"
#include "SharedArtLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
AAshStructure::AAshStructure()
{
    bReplicates = true;
    SetReplicateMovement(true);
    Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShelterMesh"));
    RootComponent = Shape;
    Shape->SetMobility(EComponentMobility::Movable);
    Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Shape->SetWorldScale3D(FVector(2.7f,2.7f,1.4f));
    Shape->SetCollisionProfileName(TEXT("BlockAll"));
}
void AAshStructure::BeginPlay()
{
    Super::BeginPlay();
    USharedArtLibrary::ApplyProp(Shape, TEXT("Shelter"));
}
