#include "AshWorldNode.h"
#include "SharedArtLibrary.h"
#include "AshHero.h"
#include "AshVitals.h"
#include "AshInventory.h"
#include "AshProgression.h"
#include "AshWorldState.h"
#include "AshCompanion.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

AAshWorldNode::AAshWorldNode()
{
    bReplicates = true;
    SetReplicateMovement(false);
    Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Primitive"));
    RootComponent = Shape;
    Shape->SetMobility(EComponentMobility::Movable);
    Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Shape->SetRelativeScale3D(FVector(1.1));
    Shape->SetCollisionProfileName(TEXT("BlockAll"));
}
void AAshWorldNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(AAshWorldNode,Kind);
    DOREPLIFETIME(AAshWorldNode,Stock);
    DOREPLIFETIME(AAshWorldNode,Label);
}
void AAshWorldNode::BeginPlay()
{
    Super::BeginPlay();
    ApplySharedAppearance();
}
void AAshWorldNode::Configure(EAshNodeKind Type,int32 Amount)
{
    if (!HasAuthority()) return;
    Kind = Type; Stock = FMath::Max(1,Amount);
    switch (Kind)
    {
    case EAshNodeKind::Wood: Label=TEXT("Timber"); break;
    case EAshNodeKind::Ore: Label=TEXT("Metal vein"); break;
    case EAshNodeKind::Herb: Label=TEXT("Wild herbs"); break;
    case EAshNodeKind::Relic: Label=TEXT("Ancient alloy"); break;
    case EAshNodeKind::Lore: Label=TEXT("Lost record"); break;
    case EAshNodeKind::Cipher: Label=TEXT("Sealed cipher"); break;
    case EAshNodeKind::Shrine: Label=TEXT("Memory shrine / checkpoint"); break;
    case EAshNodeKind::Villager: Label=TEXT("Wandering ally"); break;
    case EAshNodeKind::CultEnvoy: Label=TEXT("Choir envoy"); break;
    case EAshNodeKind::FreeEnvoy: Label=TEXT("Freefolk envoy"); break;
    case EAshNodeKind::Farm: Label=TEXT("Abandoned field"); break;
    case EAshNodeKind::Forge: Label=TEXT("Ancient forge"); break;
    case EAshNodeKind::Gate: Label=TEXT("Gate of three ciphers"); break;
    case EAshNodeKind::Shelter: Label=TEXT("Bedroll / safe camp"); break;
    case EAshNodeKind::Vehicle: Label=TEXT("Relic transport beacon"); break;
    case EAshNodeKind::Rune: Label=FString::Printf(TEXT("Ancient rune #%d"),Stock); break;
    default: Label=TEXT("Unknown"); break;
    }
    OnRep_Kind();
}
void AAshWorldNode::OnRep_Kind()
{
    Shape->SetRelativeScale3D((Kind==EAshNodeKind::Gate) ? FVector(2,2,3) : FVector(1));
    if (Kind == EAshNodeKind::Shrine || Kind == EAshNodeKind::Gate || Kind == EAshNodeKind::Vehicle)
    {
        Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    }
    ApplySharedAppearance();
}
void AAshWorldNode::ApplySharedAppearance()
{
	FName Appearance = FName(*(FString(TEXT("Veil.Node.")) + StaticEnum<EAshNodeKind>()->GetNameStringByValue(static_cast<int64>(Kind))));
	switch (Kind)
	{
	case EAshNodeKind::Wood: Appearance = TEXT("Resource.Wood"); break;
	case EAshNodeKind::Ore: Appearance = TEXT("Resource.Ore"); break;
	case EAshNodeKind::Herb: case EAshNodeKind::Farm: Appearance = TEXT("Resource.Herb"); break;
	case EAshNodeKind::Shrine: Appearance = TEXT("HRCheckpoint"); break;
	case EAshNodeKind::Gate: Appearance = TEXT("Gate"); break;
	default: break;
	}
	USharedArtLibrary::ApplyProp(Shape, Appearance);
}
FString AAshWorldNode::Describe() const { return Label.IsEmpty()?TEXT("Unknown object"):Label; }
void AAshWorldNode::Interact(AAshHero* Hero)
{
    if (!HasAuthority() || !IsValid(Hero) || Hero->Vitals->bDead) return;
    UAshInventory* Bag = Hero->Inventory;
    UAshProgression* P = Hero->Progression;
    AAshWorldState* World = GetWorld()->GetGameState<AAshWorldState>();
    switch(Kind)
    {
    case EAshNodeKind::Wood: Bag->Add(TEXT("Wood"),Stock); Hero->ClientNotice(TEXT("Collected timber.")); Destroy(); break;
    case EAshNodeKind::Ore: Bag->Add(TEXT("Ore"),Stock); Hero->ClientNotice(TEXT("Collected metal ore.")); Destroy(); break;
    case EAshNodeKind::Herb: Bag->Add(TEXT("Herb"),Stock); Hero->ClientNotice(TEXT("Collected medicinal herbs.")); Destroy(); break;
    case EAshNodeKind::Relic: Bag->Add(TEXT("Relic"),Stock); Hero->ClientNotice(TEXT("Recovered ancient alloy.")); Destroy(); break;
    case EAshNodeKind::Cipher: Bag->Add(TEXT("Cipher"),1); Hero->ClientNotice(TEXT("Obtained a cipher seal.")); Destroy(); break;
    case EAshNodeKind::Lore:
        Bag->Add(TEXT("Knowledge"),1);
        if (World) World->DiscoveryCount++;
        P->GainXP(25);
        Hero->ClientNotice(TEXT("Inscription decoded. Rune sequence: SECOND, FIRST, THIRD. +Knowledge and +XP."));
        Destroy(); break;
    case EAshNodeKind::Shrine:
        Hero->SetCheckpoint(GetActorLocation()+FVector(0,0,135));
        Hero->Vitals->RestoreAtShrine();
        Hero->ClientNotice(TEXT("Checkpoint bound; wounds and infection eased. Press F5 to save locally.")); break;
    case EAshNodeKind::Shelter:
        Hero->Vitals->Heal(28);
        Hero->Vitals->Thirst=FMath::Min(100.f,Hero->Vitals->Thirst+18.f);
        Hero->ClientNotice(TEXT("Rested at shelter.")); break;
    case EAshNodeKind::Farm:
        Bag->Add(TEXT("Herb"),2);
        Hero->ClientNotice(TEXT("Harvested herbs. This prototype farm regrows on a fresh session.")); break;
    case EAshNodeKind::Forge:
        Hero->ClientNotice(TEXT("Forge: press 3 for blade upgrade (3 Ore + 1 Relic).")); break;
    case EAshNodeKind::Villager:
        if (!IsValid(Hero->Companion))
        {
            Hero->Companion=GetWorld()->SpawnActor<AAshCompanion>(AAshCompanion::StaticClass(),
                GetActorLocation()+FVector(0,0,80),FRotator::ZeroRotator);
            if (Hero->Companion) Hero->Companion->Leader=Hero;
            Hero->ClientNotice(TEXT("Ally recruited! C switches Follow (focus), Guard (healing), Hunt (attacks)."));
        }
        else
        {
            Hero->CompanionMode=(Hero->CompanionMode+1)%3;
            Hero->ClientNotice(TEXT("Ally order changed. 0 Follow, 1 Guard, 2 Hunt."));
        }
        break;
    case EAshNodeKind::CultEnvoy:
        P->ChoirReputation += 1; P->FreefolkReputation -= 1;
        Hero->ClientNotice(TEXT("Choir reputation increased. An ending choice has consequences.")); break;
    case EAshNodeKind::FreeEnvoy:
        P->FreefolkReputation += 1; P->ChoirReputation -= 1;
        Hero->ClientNotice(TEXT("Freefolk reputation increased.")); break;
    case EAshNodeKind::Gate:
        if (!World) break;
        if (World->bGateOpen) { Hero->ClientNotice(TEXT("The gate is already unlocked.")); break; }
        if ((Bag->Count(TEXT("Cipher"))>=3 && Bag->Count(TEXT("Knowledge"))>=2) ||
            (World->bRuneSolved && Bag->Count(TEXT("Cipher"))>=1 && Bag->Count(TEXT("Knowledge"))>=1))
        {
            World->bGateOpen = true;
            Hero->ClientNotice(TEXT("The gate opens. The Pale Keeper is now vulnerable."));
            Destroy();
        }
        else Hero->ClientNotice(TEXT("Gate: collect 3 ciphers + 2 records OR solve runes + 1 cipher + 1 record."));
        break;
    case EAshNodeKind::Rune:
        if (!World || World->bRuneSolved) break;
        {
            constexpr int32 Sequence[3]={2,1,3};
            if (Stock==Sequence[FMath::Clamp(World->RuneProgress,0,2)])
            {
                ++World->RuneProgress;
                if (World->RuneProgress==3)
                { World->bRuneSolved=true; Hero->ClientNotice(TEXT("The runic circuit is solved. An alternate gate route is available!")); }
                else Hero->ClientNotice(FString::Printf(TEXT("Rune accepted: %d/3"),World->RuneProgress));
            }
            else { World->RuneProgress=0; Hero->ClientNotice(TEXT("Wrong rune. Sequence reset. Seek a lost record for the order.")); }
        }
        break;
    case EAshNodeKind::Vehicle:
        Hero->VehicleBoostUntil=GetWorld()->GetTimeSeconds()+25.f;
        Hero->ClientNotice(TEXT("Relic transport activated: movement boosted for 25 seconds (not vehicle physics).")); break;
    default: break;
    }
}
