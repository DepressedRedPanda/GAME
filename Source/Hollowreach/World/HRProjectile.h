#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/HRTypes.h"
#include "HRProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/** Spells, arrows and thrown bombs. Damages combatants and triggers elemental reactions. */
UCLASS()
class HOLLOWREACH_API AHRProjectile : public AActor
{
	GENERATED_BODY()

public:
	AHRProjectile();

	virtual void BeginPlay() override;

	static AHRProjectile* Launch(AActor* Shooter, const FVector& Location, const FVector& Direction, const FHRDamageInfo& InDamage, float Speed, float GravityScale);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile") FHRDamageInfo Damage;
	/** Thrown bombs splash in a radius on impact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile") float SplashRadius = 0.f;

protected:
	UFUNCTION() void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	void ApplyTo(AActor* Target, const FVector& Direction);

	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UProjectileMovementComponent> Movement;

	bool bHasHit = false;
};
