#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SharedArtAnimInstance.generated.h"

/** Optional common parent for authored animation Blueprints; not an animation graph by itself. */
UCLASS(Transient, Blueprintable)
class SHAREDART_API USharedArtAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly, Category="Movement") float Speed = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Movement") float Direction = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Movement") bool bFalling = false;
    UPROPERTY(BlueprintReadOnly, Category="Movement") bool bCrouched = false;
    UPROPERTY(BlueprintReadOnly, Category="Combat") FName CombatState = TEXT("Idle");
    UPROPERTY(BlueprintReadOnly, Category="Combat") int32 ComboIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category="Combat") float AttackProgress = 0.f;
};
