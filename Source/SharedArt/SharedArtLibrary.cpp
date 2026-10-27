#include "SharedArtLibrary.h"
#include "SharedArtSettings.h"
#include "SharedArtAnimInstance.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInterface.h"

namespace
{
    const FName CosmeticTag(TEXT("SharedArtCosmetic"));
}

bool USharedArtLibrary::ApplyCharacter(ACharacter* Character, FName Role, USceneComponent* Placeholder)
{
    if (!IsValid(Character) || !Character->GetMesh()) return false;
    const FSharedCharacterLook* Look = GetDefault<USharedArtSettings>()->Characters.Find(Role);
    if (!Look || Look->Mesh.IsNull()) return false;
    USkeletalMesh* Mesh = Look->Mesh.LoadSynchronous();
    if (!Mesh) return false;

    USkeletalMeshComponent* Visual = Character->GetMesh();
    Visual->SetSkeletalMesh(Mesh);
    Visual->SetRelativeTransform(Look->RelativeTransform);
    // Movement/damage still use the original capsule; no extra cosmetic collision.
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetCanEverAffectNavigation(false);
    if (!Look->AnimationBlueprint.IsNull())
    {
        if (UClass* AnimationClass = Look->AnimationBlueprint.LoadSynchronous()) Visual->SetAnimInstanceClass(AnimationClass);
    }
    Visual->SetVisibility(true);
    Visual->SetHiddenInGame(false);
    if (Placeholder) Placeholder->SetVisibility(false, true);
    return true;
}

void USharedArtLibrary::SetCharacterScale(ACharacter* Character, FName Role, float Scale)
{
    if (!Character || !Character->GetMesh()) return;
    const FSharedCharacterLook* Look = GetDefault<USharedArtSettings>()->Characters.Find(Role);
    if (Look && !Look->Mesh.IsNull()) Character->GetMesh()->SetRelativeScale3D(Look->RelativeTransform.GetScale3D() * Scale);
}

bool USharedArtLibrary::ApplyWeapon(ACharacter* Character, FName Role, UStaticMeshComponent* Weapon, FName ItemId)
{
    if (!Character || !Weapon || !Character->GetMesh() || !Character->GetMesh()->GetSkeletalMeshAsset()) return false;
    const USharedArtSettings* Settings = GetDefault<USharedArtSettings>();
    const FSharedCharacterLook* CharacterLook = Settings->Characters.Find(Role);
    const FSharedPropLook* Look = ItemId.IsNone() ? nullptr : Settings->Props.Find(FName(*(FString(TEXT("Weapon.")) + ItemId.ToString())));
    if (!Look) Look = Settings->Props.Find(TEXT("Weapon.Default"));
    if (!Look || Look->Mesh.IsNull()) return false;
    UStaticMesh* Mesh = Look->Mesh.LoadSynchronous();
    if (!Mesh) return false;
    const FName Socket = CharacterLook ? CharacterLook->WeaponSocket : FName(TEXT("hand_r"));
    if (!Character->GetMesh()->DoesSocketExist(Socket)) return false;
    Weapon->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
    Weapon->SetStaticMesh(Mesh);
    Weapon->EmptyOverrideMaterials();
    Weapon->SetRelativeTransform(Look->RelativeTransform);
    Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (UMaterialInterface* Material = Look->Material.LoadSynchronous()) Weapon->SetMaterial(0, Material);
    Weapon->SetVisibility(true);
    return true;
}

UAnimMontage* USharedArtLibrary::FindMontage(FName Role, FName Action)
{
    const auto& References = GetDefault<USharedArtSettings>()->Montages;
    const TSoftObjectPtr<UAnimMontage>* Reference = References.Find(FName(*(Role.ToString() + TEXT(".") + Action.ToString())));
    return Reference ? Reference->LoadSynchronous() : nullptr;
}

void USharedArtLibrary::PlayCue(const UObject* WorldContextObject, FName Key, FVector Location)
{
    if (!GEngine || !WorldContextObject) return;
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (!World || World->GetNetMode() == NM_DedicatedServer) return;
    const USharedArtSettings* Settings = GetDefault<USharedArtSettings>();
    if (const TSoftObjectPtr<UNiagaraSystem>* Reference = Settings->Effects.Find(Key))
    {
        if (UNiagaraSystem* System = Reference->LoadSynchronous()) UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Location);
    }
    if (const TSoftObjectPtr<USoundBase>* Reference = Settings->Sounds.Find(Key))
    {
        if (USoundBase* Sound = Reference->LoadSynchronous()) UGameplayStatics::PlaySoundAtLocation(World, Sound, Location);
    }
}

bool USharedArtLibrary::ApplySurface(UMeshComponent* Component, FName Key)
{
    if (!Component) return false;
    const TSoftObjectPtr<UMaterialInterface>* Reference = GetDefault<USharedArtSettings>()->SurfaceMaterials.Find(Key);
    if (!Reference || Reference->IsNull()) return false;
    UMaterialInterface* Material = Reference->LoadSynchronous();
    if (!Material) return false;
    for (int32 Slot = 0; Slot < FMath::Max(1, Component->GetNumMaterials()); ++Slot) Component->SetMaterial(Slot, Material);
    return true;
}

bool USharedArtLibrary::ApplyProp(UStaticMeshComponent* Placeholder, FName Key)
{
    if (!Placeholder || !Placeholder->GetOwner()) return false;
    const FSharedPropLook* Look = GetDefault<USharedArtSettings>()->Props.Find(Key);
    if (!Look) return false;
    UStaticMesh* Mesh = Look->Mesh.IsNull() ? nullptr : Look->Mesh.LoadSynchronous();
    UMaterialInterface* Material = Look->Material.IsNull() ? nullptr : Look->Material.LoadSynchronous();
    if (!Mesh)
    {
        if (Material) Placeholder->SetMaterial(0, Material);
        return Material != nullptr;
    }

    AActor* Owner = Placeholder->GetOwner();
    UStaticMeshComponent* Cosmetic = nullptr;
    TArray<UStaticMeshComponent*> Components;
    Owner->GetComponents<UStaticMeshComponent>(Components);
    for (UStaticMeshComponent* Component : Components)
    {
        if (Component->GetAttachParent() == Placeholder && Component->ComponentHasTag(CosmeticTag))
        {
            Cosmetic = Component;
            break;
        }
    }
    if (!Cosmetic)
    {
        Cosmetic = NewObject<UStaticMeshComponent>(Owner);
        Owner->AddInstanceComponent(Cosmetic);
        Cosmetic->ComponentTags.Add(CosmeticTag);
        Cosmetic->SetMobility(Placeholder->Mobility);
        Cosmetic->SetupAttachment(Placeholder);
        Cosmetic->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Cosmetic->SetGenerateOverlapEvents(false);
        Cosmetic->SetCanEverAffectNavigation(false);
        Cosmetic->RegisterComponent();
    }
    Cosmetic->SetStaticMesh(Mesh);
    // Clear an old profile's material overrides before applying the next one.
    Cosmetic->EmptyOverrideMaterials();
    FTransform Transform = Look->RelativeTransform;
    if (Look->bFitToPlaceholder && Placeholder->GetStaticMesh())
    {
        const FVector Target = Placeholder->GetStaticMesh()->GetBoundingBox().GetExtent();
        const FVector Extent = Mesh->GetBoundingBox().GetExtent();
        const double Fit = FMath::Min3(Target.X / FMath::Max(Extent.X, 1.0), Target.Y / FMath::Max(Extent.Y, 1.0), Target.Z / FMath::Max(Extent.Z, 1.0));
        Transform.SetScale3D(Transform.GetScale3D() * Fit);
    }
    Cosmetic->SetRelativeTransform(Transform);
    if (Material)
    {
        for (int32 Slot = 0; Slot < FMath::Max(1, Cosmetic->GetNumMaterials()); ++Slot) Cosmetic->SetMaterial(Slot, Material);
    }
    // Hide only the placeholder; leave its gameplay collision and physics intact.
    Placeholder->SetVisibility(false, false);
    Cosmetic->SetVisibility(true, false);
    return true;
}

void USharedArtLibrary::UpdateCombat(ACharacter* Character, FName State, int32 Combo, float Progress)
{
    if (!Character || !Character->GetMesh()) return;
    if (USharedArtAnimInstance* Instance = Cast<USharedArtAnimInstance>(Character->GetMesh()->GetAnimInstance()))
    {
        Instance->CombatState = State;
        Instance->ComboIndex = Combo;
        Instance->AttackProgress = FMath::Clamp(Progress, 0.f, 1.f);
    }
}
