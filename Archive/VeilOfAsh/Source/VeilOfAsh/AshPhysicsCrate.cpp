#include "AshPhysicsCrate.h"
#include "SharedArtLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
AAshPhysicsCrate::AAshPhysicsCrate()
{
    bReplicates=true;
    SetReplicateMovement(true);
    Shape=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicsBody"));
    RootComponent=Shape;
    Shape->SetMobility(EComponentMobility::Movable);
    Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Shape->SetCollisionProfileName(TEXT("PhysicsActor"));
    Shape->SetSimulatePhysics(true);
    Shape->SetMassOverrideInKg(NAME_None,42.f);
    Shape->SetLinearDamping(1.1f);
}
void AAshPhysicsCrate::BeginPlay()
{
    Super::BeginPlay();
    USharedArtLibrary::ApplyProp(Shape, TEXT("Physics.Crate"));
}
