#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HRMovingPlatform.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

/** Platforming: a block that travels between two points, optionally crumbling when stood on. */
UCLASS()
class HOLLOWREACH_API AHRMovingPlatform : public AActor
{
	GENERATED_BODY()

public:
	AHRMovingPlatform();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetPlatformSize(const FVector& SizeCm);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Platform", meta=(MakeEditWidget=true)) FVector EndOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Platform") float Period = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Platform") float PhaseOffset = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Platform") bool bCrumbles = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Platform") float CrumbleDelay = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Platform") float RespawnDelay = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Platform") FLinearColor Color = FLinearColor(0.45f, 0.4f, 0.35f);

protected:
	UFUNCTION() void HandleStep(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void SetSolid(bool bSolid);

	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Platform;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UBoxComponent> StepTrigger;

	FVector StartLocation;
	bool bCrumbling = false;
	FTimerHandle CrumbleTimer;
};
