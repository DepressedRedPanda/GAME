#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HRStealthZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** Tall grass, reeds or deep shadow. Being inside one (especially crouched) hides you. */
UCLASS()
class HOLLOWREACH_API AHRStealthZone : public AActor
{
	GENERATED_BODY()

public:
	AHRStealthZone();
	virtual void BeginPlay() override;

	void SetZoneSize(const FVector& SizeCm);

protected:
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UBoxComponent> Volume;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Grass;
};
