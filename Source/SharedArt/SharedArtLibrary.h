#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SharedArtLibrary.generated.h"

class ACharacter;
class UMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
class UAnimMontage;

UCLASS()
class SHAREDART_API USharedArtLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Returns false without changing the existing character when no valid mesh is assigned.
    static bool ApplyCharacter(ACharacter* Character, FName Role, USceneComponent* Placeholder);
    static void SetCharacterScale(ACharacter* Character, FName Role, float Scale);
    static bool ApplyWeapon(ACharacter* Character, FName Role, UStaticMeshComponent* Weapon, FName ItemId);
    static UAnimMontage* FindMontage(FName Role, FName Action);

    UFUNCTION(BlueprintCallable, Category="Shared Art", meta=(WorldContext="WorldContextObject"))
    static void PlayCue(const UObject* WorldContextObject, FName Key, FVector Location);

    UFUNCTION(BlueprintCallable, Category="Shared Art")
    static bool ApplyProp(UStaticMeshComponent* Placeholder, FName Key);

    UFUNCTION(BlueprintCallable, Category="Shared Art")
    static bool ApplySurface(UMeshComponent* Component, FName Key);

    static void UpdateCombat(ACharacter* Character, FName State, int32 Combo, float Progress);
};
