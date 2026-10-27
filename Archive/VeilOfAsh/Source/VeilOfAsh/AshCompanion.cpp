#include "AshCompanion.h"
#include "AshHero.h"
#include "AshEnemy.h"
#include "AshVitals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
AAshCompanion::AAshCompanion()
{
    PrimaryActorTick.bCanEverTick=true;
    bReplicates=true;
    SetReplicateMovement(true);
    Shape=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugAlly"));
    RootComponent=Shape;
    Shape->SetMobility(EComponentMobility::Movable);
    Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Shape->SetWorldScale3D(FVector(.65f));
    Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AAshCompanion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(AAshCompanion,Leader);
}
void AAshCompanion::Tick(float DT)
{
    Super::Tick(DT);
    if (!HasAuthority() || !IsValid(Leader) || !Leader->Vitals || Leader->Vitals->bDead) return;
    const FVector Desired=Leader->GetActorLocation()-Leader->GetActorForwardVector()*160.f+
        Leader->GetActorRightVector()*120.f+FVector(0,0,70);
    const FVector Delta=Desired-GetActorLocation();
    if (Delta.SizeSquared()>FMath::Square(25.f))
        SetActorLocation(GetActorLocation()+Delta.GetClampedToMaxSize(370.f*DT),false);
    const float Now=GetWorld()->GetTimeSeconds();
    if (Now-LastAssist<2.6f) return;
    LastAssist=Now;
    if (Leader->CompanionMode==0) Leader->Vitals->Focus=FMath::Min(100.f,Leader->Vitals->Focus+5.f);
    if (Leader->CompanionMode==1) Leader->Vitals->Heal(5.f);
    if (Leader->CompanionMode==2)
    {
        for (TActorIterator<AAshEnemy> It(GetWorld());It;++It)
            if (FVector::Dist2D(GetActorLocation(),It->GetActorLocation())<530.f)
            { It->ReceiveStrike(14.f,22.f,Leader); break; }
    }
}
