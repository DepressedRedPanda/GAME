#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AshTypes.h"
#include "AshProgression.generated.h"

UCLASS(ClassGroup=(Ash), meta=(BlueprintSpawnableComponent))
class VEILOFASH_API UAshProgression : public UActorComponent
{
    GENERATED_BODY()
public:
    UAshProgression();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Level = 1;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 XP = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 SkillPoints = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Might = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Guile = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Insight = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 FreefolkReputation = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ChoirReputation = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) EAshClass HeroClass = EAshClass::Vanguard;
    void GainXP(int32 Amount);
    void ChooseClass(EAshClass NewClass);
    bool BuySkill(int32 Branch); // 0 Might, 1 Guile, 2 Insight
};
