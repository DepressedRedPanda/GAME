#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HRTraversalComponent.generated.h"

class ACharacter;
class AHRGrapplePoint;
class UHRProgressionComponent;

/**
 * Parkour and platforming: ledge mantling, wall running + wall jumps, grappling to anchor
 * points, and gliding. Everything except mantling is unlocked through progression, so new
 * movement opens new routes through the world (Metroidvania-style gating).
 */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRTraversalComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Called when jump is pressed. Returns true if a traversal move consumed the input. */
	bool HandleJumpPressed(const FVector& ViewDirection);
	void HandleJumpReleased();

	UFUNCTION(BlueprintCallable, Category="Traversal") bool TryMantle();
	UFUNCTION(BlueprintPure, Category="Traversal") bool IsMantling() const { return bMantling; }
	UFUNCTION(BlueprintPure, Category="Traversal") bool IsWallRunning() const { return bWallRunning; }
	UFUNCTION(BlueprintPure, Category="Traversal") bool IsGliding() const { return bGliding; }
	UFUNCTION(BlueprintPure, Category="Traversal") AHRGrapplePoint* FindGrappleTarget(const FVector& ViewDirection) const;

	void SetWantsSprint(bool bInWantsSprint) { bWantsSprint = bInWantsSprint; }
	void RefreshUnlocks();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Mantle") float MinMantleHeight = 45.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Mantle") float MaxMantleHeight = 230.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Mantle") float MantleReach = 75.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Mantle") float MantleDuration = 0.38f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|WallRun") float WallRunMinSpeed = 350.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|WallRun") float WallRunSpeed = 750.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|WallRun") float WallRunMaxTime = 1.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|WallRun") float WallRunGravity = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Grapple") float GrappleRange = 2200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Grapple") float GrappleSpeed = 2100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Glide") float GlideGravity = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal|Glide") float GlideMaxFallSpeed = 220.f;

private:
	ACharacter* GetCharacter() const;
	bool HasAbility(uint8 Ability) const;
	bool FindWall(FVector& OutNormal) const;
	void StartWallRun(const FVector& Normal);
	void StopWallRun();
	void StartGlide();
	void StopGlide();
	void TickMantle(float DeltaTime);
	void TickWallRun(float DeltaTime);

	TWeakObjectPtr<UHRProgressionComponent> Progression;

	bool bMantling = false;
	FVector MantleStart;
	FVector MantleEnd;
	float MantleAlpha = 0.f;

	bool bWallRunning = false;
	FVector WallNormal;
	float WallRunTime = 0.f;
	float WallRunCooldown = 0.f;

	bool bGlideHeld = false;
	bool bGliding = false;
	bool bWantsSprint = false;
	float AutoMantleTimer = 0.f;

	float DefaultGravityScale = 1.f;
	float DefaultAirControl = 0.35f;
};
