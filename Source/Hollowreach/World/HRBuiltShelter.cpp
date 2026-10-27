#include "World/HRBuiltShelter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRStatics.h"
#include "Engine/World.h"
#include "Systems/HRSaveSubsystem.h"

AHRBuiltShelter::AHRBuiltShelter()
{
    DisplayName = NSLOCTEXT("Veil", "Shelter", "Built Shelter");
    SharedAppearanceKey = TEXT("Shelter");
    bStartsDiscovered = true;
    Mesh->SetRelativeScale3D(FVector(2.7f, 2.7f, 1.4f));
}

bool AHRBuiltShelter::BuildFor(AHRPlayerCharacter* Player)
{
    if (!Player || Player->IsDead() || Player->IsDowned()) return false;
    UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(Player);
    UHRInventoryComponent* Inventory = Player->GetInventory();
    if (!Saves || Inventory->CountItem(TEXT("Item.Wood")) < 5 || Inventory->CountItem(TEXT("Item.IronOre")) < 2)
    {
        UHRStatics::Notify(Player, NSLOCTEXT("Veil", "ShelterCost", "Shelter needs 5 Wood and 2 Iron Ore."));
        return false;
    }
    UWorld* World = Player->GetWorld();
    FVector Place = Player->GetActorLocation() + Player->GetActorForwardVector() * 450.f;
    FHitResult Ground;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ShelterGround), false, Player);
    if (!World->LineTraceSingleByChannel(Ground, Place + FVector(0.f, 0.f, 600.f), Place - FVector(0.f, 0.f, 1600.f), ECC_Visibility, Params) || Ground.ImpactNormal.Z < 0.8f) return false;
    Place = Ground.ImpactPoint + FVector(0.f, 0.f, 72.f);
    if (World->OverlapBlockingTestByChannel(Place, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeBox(FVector(130.f, 130.f, 65.f)), Params)) return false;
    const FTransform Transform(Player->GetActorRotation(), Place);
    AHRBuiltShelter* Shelter = World->SpawnActorDeferred<AHRBuiltShelter>(StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
    if (!Shelter) return false;
    Shelter->PersistentId = FName(*(FString(TEXT("Shelter.")) + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    Shelter->FinishSpawning(Transform);
    if (!IsValid(Shelter)) return false;
    Inventory->RemoveItem(TEXT("Item.Wood"), 5);
    Inventory->RemoveItem(TEXT("Item.IronOre"), 2);
    Saves->RememberShelter(Shelter->PersistentId, Transform);
    UHRStatics::Notify(Player, NSLOCTEXT("Veil", "ShelterBuilt", "Shelter built. Rest here to bind the checkpoint and save."));
    return true;
}
