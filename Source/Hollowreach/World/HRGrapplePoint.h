#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HRGrapplePoint.generated.h"

class UStaticMeshComponent;

/** Anchor for the grappling hook. Jump in mid-air while looking at one to swing toward it. */
UCLASS()
class HOLLOWREACH_API AHRGrapplePoint : public AActor
{
	GENERATED_BODY()

public:
	AHRGrapplePoint();
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Marker;
};
