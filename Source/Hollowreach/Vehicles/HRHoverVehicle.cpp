#include "Vehicles/HRHoverVehicle.h"

#include "Camera/CameraComponent.h"
#include "Characters/HRCharacterBase.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRInputSet.h"
#include "Core/HRStatics.h"
#include "EnhancedInputComponent.h"
#include "Game/HRPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Systems/HRFactionSubsystem.h"

AHRHoverVehicle::AHRHoverVehicle()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetStaticMesh(HRGreybox::Cube());
	Body->SetRelativeScale3D(FVector(3.2f, 1.7f, 0.45f));
	Body->SetCollisionProfileName(TEXT("PhysicsActor"));
	Body->SetSimulatePhysics(true);
	Body->SetMassOverrideInKg(NAME_None, 600.f, true);
	Body->SetLinearDamping(0.4f);
	Body->SetAngularDamping(3.f);
	Body->SetNotifyRigidBodyCollision(true);
	RootComponent = Body;

	Canopy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Canopy"));
	Canopy->SetupAttachment(Body);
	Canopy->SetStaticMesh(HRGreybox::Cube());
	Canopy->SetRelativeLocation(FVector(-15.f, 0.f, 80.f));
	Canopy->SetRelativeScale3D(FVector(0.35f, 0.6f, 0.9f));
	Canopy->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Seat = CreateDefaultSubobject<USceneComponent>(TEXT("Seat"));
	Seat->SetupAttachment(Body);
	Seat->SetRelativeLocation(FVector(-20.f, 0.f, 200.f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Body);
	CameraBoom->SetUsingAbsoluteScale(true);
	CameraBoom->TargetArmLength = 750.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 200.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 6.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	AutoPossessAI = EAutoPossessAI::Disabled;
}

void AHRHoverVehicle::BeginPlay()
{
	Super::BeginPlay();
	HRGreybox::Tint(Body, FLinearColor(0.5f, 0.45f, 0.3f));
	HRGreybox::Tint(Canopy, FLinearColor(0.2f, 0.25f, 0.3f));
	Body->OnComponentHit.AddDynamic(this, &AHRHoverVehicle::HandleHit);
}

FText AHRHoverVehicle::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	return NSLOCTEXT("HR", "Ride", "Ride the skiff");
}

void AHRHoverVehicle::Interact(AHRPlayerCharacter* Player)
{
	Enter(Player);
}

void AHRHoverVehicle::Enter(AHRPlayerCharacter* Player)
{
	AHRPlayerController* PC = Player ? Cast<AHRPlayerController>(Player->GetController()) : nullptr;
	if (!PC || Driver.IsValid())
	{
		return;
	}
	Driver = Player;
	Player->GetCharacterMovement()->DisableMovement();
	Player->SetActorEnableCollision(false);
	Player->SetActorHiddenInGame(true);
	Player->AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	PC->Possess(this);
	PC->SetControlRotation(GetActorRotation());
}

void AHRHoverVehicle::Exit()
{
	AHRPlayerCharacter* Player = Driver.Get();
	AHRPlayerController* PC = Cast<AHRPlayerController>(GetController());
	if (!Player || !PC)
	{
		return;
	}
	Driver = nullptr;
	MoveInput = FVector2D::ZeroVector;

	Player->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	const FVector ExitLocation = GetActorLocation() + GetActorRightVector() * 260.f + FVector(0.f, 0.f, 120.f);
	Player->TeleportTo(ExitLocation, FRotator(0.f, GetActorRotation().Yaw, 0.f));
	Player->SetActorEnableCollision(true);
	Player->SetActorHiddenInGame(false);
	Player->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	PC->Possess(Player);
}

void AHRHoverVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	AHRPlayerController* PC = Cast<AHRPlayerController>(GetController());
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!PC || !Input)
	{
		return;
	}
	PC->EnsureInputMapping();
	const UHRInputSet* Set = PC->GetInputSet();
	Input->BindAction(Set->Move, ETriggerEvent::Triggered, this, &AHRHoverVehicle::OnMove);
	Input->BindAction(Set->Move, ETriggerEvent::Completed, this, &AHRHoverVehicle::OnMove);
	Input->BindAction(Set->Look, ETriggerEvent::Triggered, this, &AHRHoverVehicle::OnLook);
	Input->BindAction(Set->Jump, ETriggerEvent::Started, this, &AHRHoverVehicle::OnBoost);
	Input->BindAction(Set->Interact, ETriggerEvent::Started, this, &AHRHoverVehicle::OnExit);
}

void AHRHoverVehicle::OnMove(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
}

void AHRHoverVehicle::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void AHRHoverVehicle::OnBoost()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastBoostTime < 1.5)
	{
		return;
	}
	LastBoostTime = Now;
	Body->AddImpulse((GetActorForwardVector() * 0.6f + FVector::UpVector) * BoostImpulse, NAME_None, true);
}

void AHRHoverVehicle::OnExit()
{
	Exit();
}

void AHRHoverVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Hover springs at the four corners.
	const FVector Extent = FVector(150.f, 80.f, 0.f);
	const FVector Corners[4] = {
		FVector(Extent.X, Extent.Y, 0.f), FVector(Extent.X, -Extent.Y, 0.f),
		FVector(-Extent.X, Extent.Y, 0.f), FVector(-Extent.X, -Extent.Y, 0.f)
	};
	const float Mass = Body->GetMass();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRHover), false, this);
	if (Driver.IsValid())
	{
		Params.AddIgnoredActor(Driver.Get());
	}

	int32 Grounded = 0;
	for (const FVector& Corner : Corners)
	{
		const FVector Start = GetActorTransform().TransformPositionNoScale(Corner);
		const FVector End = Start - FVector::UpVector * HoverHeight * 2.f;
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			++Grounded;
			const float Compression = 1.f - Hit.Distance / (HoverHeight * 2.f);
			const float PointVelocity = Body->GetPhysicsLinearVelocityAtPoint(Start).Z;
			const float Force = (Compression * SpringStrength * 980.f - PointVelocity * SpringDamping) * Mass * 0.25f;
			Body->AddForceAtLocation(FVector::UpVector * FMath::Max(0.f, Force), Start);
		}
	}

	if (!Driver.IsValid())
	{
		return;
	}

	// Steering and thrust only bite when near the ground.
	const float Control = Grounded > 0 ? 1.f : 0.25f;
	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();
	Body->AddForce(Forward * MoveInput.Y * ThrustAcceleration * Mass * Control);
	Body->AddTorqueInRadians(FVector::UpVector * MoveInput.X * TurnRate * Body->GetInertiaTensor().Z * Control * 4.f);

	// Lateral grip so it carves instead of sliding forever.
	const FVector Velocity = Body->GetPhysicsLinearVelocity();
	const FVector Right = GetActorRightVector();
	const float Lateral = FVector::DotProduct(Velocity, Right);
	Body->AddForce(-Right * Lateral * LateralGrip * Mass * Control);

	// Keep it upright.
	const FVector Up = GetActorUpVector();
	const FVector Correction = FVector::CrossProduct(Up, FVector::UpVector);
	Body->AddTorqueInRadians(Correction * Body->GetInertiaTensor().X * 30.f);
}

void AHRHoverVehicle::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AHRCharacterBase* Victim = Cast<AHRCharacterBase>(OtherActor);
	const double Now = GetWorld()->GetTimeSeconds();
	const float Speed = Body->GetPhysicsLinearVelocity().Size();
	if (!Victim || !Driver.IsValid() || Speed < RamSpeed || Now - LastRamTime < 0.4 || UHRFactionSubsystem::AreFriendly(Driver.Get(), Victim))
	{
		return;
	}
	LastRamTime = Now;
	FHRDamageInfo Damage;
	Damage.Amount = Speed * 0.04f;
	Damage.PostureDamage = Speed * 0.05f;
	Damage.bHeavy = true;
	Damage.Source = Driver.Get();
	Victim->GetCombat()->ReceiveHit(Damage);
	Victim->LaunchCharacter(Body->GetPhysicsLinearVelocity().GetSafeNormal() * 800.f + FVector(0.f, 0.f, 400.f), true, true);
	UHRFactionSubsystem::NotifyAttack(Driver.Get(), Victim);
}
