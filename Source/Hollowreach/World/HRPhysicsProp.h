#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/HRInteractable.h"
#include "HRPhysicsProp.generated.h"

class UStaticMeshComponent;

/**
 * Physics-simulated crate or barrel. Can be shoved by attacks and Kinetic Push, weighs down
 * pressure plates, and hurts whatever it slams into. Volatile barrels explode when burned,
 * chaining into other barrels.
 */
UCLASS()
class HOLLOWREACH_API AHRPhysicsProp : public AActor, public IHRElementalReactive
{
	GENERATED_BODY()

public:
	AHRPhysicsProp();

	virtual void BeginPlay() override;
	virtual void ReactToElement(EHRDamageType Element, AActor* Source) override;

	UFUNCTION(BlueprintCallable, Category="Prop") void Explode(AActor* Source);
	void MakeBarrel();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop") bool bExplosive = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop") float ExplosionRadius = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop") float ExplosionDamage = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop") float ImpactDamageSpeed = 900.f;

	UStaticMeshComponent* GetMesh() const { return Mesh; }

protected:
	UFUNCTION() void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Mesh;

	bool bExploded = false;
	double LastImpactTime = 0.0;
};
