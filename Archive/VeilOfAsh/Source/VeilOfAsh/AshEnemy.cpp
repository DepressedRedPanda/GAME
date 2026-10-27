#include "AshEnemy.h"
#include "SharedArtLibrary.h"
#include "AshHero.h"
#include "AshVitals.h"
#include "AshProgression.h"
#include "AshWorldState.h"
#include "AshWorldNode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

AAshEnemy::AAshEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    GetCapsuleComponent()->InitCapsuleSize(43.f, 90.f);
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    Body->SetupAttachment(GetRootComponent());
    Body->SetMobility(EComponentMobility::Movable);
    Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")));
    Body->SetRelativeScale3D(FVector(0.8,0.8,1.8));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->MaxWalkSpeed = 230.f;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
void AAshEnemy::SetBoss(bool bIsBoss)
{
    if (!HasAuthority()) return;
    bBoss = bIsBoss; MaxHP = bBoss ? 450.f : 55.f;
    HP = MaxHP; Poise = bBoss ? 220.f : 60.f;
    OnRep_Boss();
}
void AAshEnemy::OnRep_Boss()
{
    Body->SetRelativeScale3D(bBoss ? FVector(2.5f,2.5f,3.f) : FVector(.8f,.8f,1.8f));
    GetCapsuleComponent()->SetCapsuleSize(bBoss ? 80.f : 43.f, bBoss ? 155.f : 90.f);
    GetCharacterMovement()->MaxWalkSpeed = bBoss ? 170.f : 230.f;
	USharedArtLibrary::ApplyCharacter(this, bBoss ? TEXT("Boss") : TEXT("Enemy"), Body);
}
void AAshEnemy::BeginPlay() { Super::BeginPlay(); SpawnPoint = GetActorLocation(); USharedArtLibrary::ApplyCharacter(this, bBoss ? TEXT("Boss") : TEXT("Enemy"), Body); }
void AAshEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(AAshEnemy,bBoss); DOREPLIFETIME(AAshEnemy,HP);
    DOREPLIFETIME(AAshEnemy,MaxHP); DOREPLIFETIME(AAshEnemy,Poise);
    DOREPLIFETIME(AAshEnemy,bTelegraph); DOREPLIFETIME(AAshEnemy,bStaggered);
}
void AAshEnemy::Tick(float DT)
{
    Super::Tick(DT);
    if (!HasAuthority() || HP <= 0.f) return;
    const float Now = GetWorld()->GetTimeSeconds();
    if (bStaggered && Now >= StaggerUntil)
    {
        bStaggered = false; Poise = bBoss ? 140.f : 48.f;
    }
    if (bStaggered) return;
    AAshHero* Closest = nullptr;
    float BestDistance = 100000.f;
    for (TActorIterator<AAshHero> It(GetWorld()); It; ++It)
    {
        AAshHero* H = *It;
        if (!H || H->Vitals->bDead) continue;
        const float D = FVector::Dist2D(H->GetActorLocation(),GetActorLocation());
        const AAshWorldState* State=GetWorld()->GetGameState<AAshWorldState>();
        const float Darkness=(State && (State->TimeOfDay<6.f || State->TimeOfDay>19.f))?.78f:1.f;
        const float Storm=(State && State->Weather==EAshWeather::Ashstorm)?.65f:1.f;
        const float SkillStealth=FMath::Clamp(1.f-H->Progression->Guile*.06f,.60f,1.f);
        const float VisibleDistance = (H->bIsCrouched ? 475.f : 1150.f)*Darkness*Storm*
            (H->bRevealed?.50f:1.f)*(H->Progression->HeroClass==EAshClass::Shade?.85f:1.f)*SkillStealth;
        if (D < VisibleDistance && D < BestDistance) { Closest = H; BestDistance = D; }
    }
    TargetHero = Closest;
    if (bTelegraph)
    {
        if (Now >= StrikeMoment)
        {
            bTelegraph = false;
            AAshHero* Victim = TargetHero.Get();
            if (Victim && FVector::Dist2D(Victim->GetActorLocation(),GetActorLocation()) <= (bBoss ? 285.f : 200.f))
            {
                const bool Parried = Victim->Vitals->ReceiveHit(bBoss ? 28.f : 16.f, Now);
                if (Parried) ReceiveCounter(Victim);
                else if (Victim->Vitals->bDead) Victim->OnDeath();
            }
        }
        return;
    }
    if (Closest)
    {
        const FVector Direction = (Closest->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        SetActorRotation(Direction.Rotation());
        if (BestDistance > (bBoss ? 245.f : 160.f)) AddMovementInput(Direction,1.f);
        if (BestDistance < (bBoss ? 270.f : 185.f) && Now >= AttackClock)
        {
            bTelegraph = true;
            StrikeMoment = Now + (bBoss ? 1.05f : .72f);
            AttackClock = Now + (bBoss ? 2.5f : 1.7f);
        }
    }
    else if (FVector::Dist2D(GetActorLocation(),SpawnPoint) > 100.f)
    {
        AddMovementInput((SpawnPoint - GetActorLocation()).GetSafeNormal2D(),.5f);
    }
}
void AAshEnemy::ReceiveStrike(float Damage,float Stagger,AAshHero* Source)
{
    if (!HasAuthority() || HP <= 0.f) return;
    if (bBoss)
    {
        if (AAshWorldState* State=GetWorld()->GetGameState<AAshWorldState>())
        {
            if (!State->bGateOpen)
            {
                if (Source) Source->ClientNotice(TEXT("The Keeper is shielded. Unlock the three-cipher gate first."));
                return;
            }
        }
    }
    HP = FMath::Max(0.f,HP-Damage);
    Poise -= Stagger;
    if (Poise <= 0.f)
    {
        bStaggered = true; bTelegraph = false;
        StaggerUntil = GetWorld()->GetTimeSeconds()+ (bBoss ? 1.5f : 2.1f);
    }
    if (HP <= 0.f) Die(Source);
}
void AAshEnemy::ReceiveCounter(AAshHero* Source)
{
    if (!HasAuthority()) return;
    bStaggered = true;
    bTelegraph = false;
    StaggerUntil = GetWorld()->GetTimeSeconds() + (bBoss ? 1.25f : 2.2f);
    Poise = 0.f;
    if (Source) Source->ClientNotice(TEXT("Perfect parry! Enemy posture broken."));
}
void AAshEnemy::Die(AAshHero* Killer)
{
    if (Killer) Killer->AwardKill(bBoss ? 325 : 45);
    if (bBoss)
    {
        if (AAshWorldState* S = GetWorld()->GetGameState<AAshWorldState>()) S->bBossDefeated = true;
    }
    else if (FMath::RandRange(0,99)<60)
    {
        const FVector DropPos = GetActorLocation()+FVector(0,0,40);
        if (AAshWorldNode* Drop = GetWorld()->SpawnActor<AAshWorldNode>(AAshWorldNode::StaticClass(),DropPos,FRotator::ZeroRotator))
        {
            Drop->Configure(FMath::RandBool()?EAshNodeKind::Herb:EAshNodeKind::Ore,1);
        }
    }
    Destroy();
}
