#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SharedArtSettings.generated.h"

class UAnimInstance;
class USkeletalMesh;
class UStaticMesh;
class UMaterialInterface;
class UAnimMontage;
class USoundBase;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct SHAREDART_API FSharedCharacterLook
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") TSoftClassPtr<UAnimInstance> AnimationBlueprint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") FName WeaponSocket = TEXT("hand_r");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") FTransform RelativeTransform = FTransform(FRotator(0.f, -90.f, 0.f), FVector(0.f, 0.f, -90.f));
};

USTRUCT(BlueprintType)
struct SHAREDART_API FSharedPropLook
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") TSoftObjectPtr<UMaterialInterface> Material;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") FTransform RelativeTransform = FTransform::Identity;
    // Fits the cosmetic uniformly inside the existing shape. Turn off for trees/tall props and tune the transform.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") bool bFitToPlaceholder = false;
};

/** Optional appearance profiles. Empty settings preserve both prototypes' original shapes and rules. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Shared Free Asset Appearance"))
class SHAREDART_API USharedArtSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return TEXT("Game"); }

    UPROPERTY(Config, EditAnywhere, Category="Characters") TMap<FName, FSharedCharacterLook> Characters;
    UPROPERTY(Config, EditAnywhere, Category="Props") TMap<FName, FSharedPropLook> Props;
    // Materials only: terrain collision and instanced-mesh dimensions remain unchanged.
    UPROPERTY(Config, EditAnywhere, Category="Environment") TMap<FName, TSoftObjectPtr<UMaterialInterface>> SurfaceMaterials;
    UPROPERTY(Config, EditAnywhere, Category="Combat") TMap<FName, TSoftObjectPtr<UAnimMontage>> Montages;
    UPROPERTY(Config, EditAnywhere, Category="Effects") TMap<FName, TSoftObjectPtr<UNiagaraSystem>> Effects;
    UPROPERTY(Config, EditAnywhere, Category="Audio") TMap<FName, TSoftObjectPtr<USoundBase>> Sounds;
};
