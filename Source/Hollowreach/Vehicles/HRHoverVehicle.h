#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Core/HRInteractable.h"
#include "HRHoverVehicle.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class AHRPlayerCharacter;
struct FInputActionValue;

/**
 * Physics hover-skiff for crossing the open world. Four spring "thrusters" keep it floating over
 * terrain; throttle, steering, lateral grip and a boost-hop are applied as forces. Ram enemies
 * at speed to bowl them over. Implemented without the Chaos Vehicles plugin so it works anywhere.
 */
UCLASS()
class HOLLOWREACH_API AHRHoverVehicle : public APawn, public IHRInteractable
{
	GENERATED_BODY()

public:
	AHRHoverVehicle();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override { return !Driver.IsValid(); }
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	void Enter(AHRPlayerCharacter* Player);
	void Exit();
	AHRPlayerCharacter* GetDriver() const { return Driver.Get(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle") float HoverHeight = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle") float SpringStrength = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle") float SpringDamping = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle") float ThrustAcceleration = 1800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle") float TurnRate = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle") float LateralGrip = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle") float BoostImpulse = 900.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle") float RamSpeed = 1100.f;

protected:
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnBoost();
	void OnExit();

	UFUNCTION() void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Canopy;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USceneComponent> Seat;

	TWeakObjectPtr<AHRPlayerCharacter> Driver;
	FVector2D MoveInput = FVector2D::ZeroVector;
	double LastBoostTime = -10.0;
	double LastRamTime = -10.0;
};
