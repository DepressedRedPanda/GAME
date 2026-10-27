#include "Components/HRTraversalComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/HRGrapplePoint.h"

UHRTraversalComponent::UHRTraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHRTraversalComponent::BeginPlay()
{
	Super::BeginPlay();
	Progression = GetOwner()->FindComponentByClass<UHRProgressionComponent>();
	if (const ACharacter* Character = GetCharacter())
	{
		DefaultGravityScale = Character->GetCharacterMovement()->GravityScale;
		DefaultAirControl = Character->GetCharacterMovement()->AirControl;
	}
	RefreshUnlocks();
}

ACharacter* UHRTraversalComponent::GetCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

bool UHRTraversalComponent::HasAbility(uint8 Ability) const
{
	return Progression.IsValid() && Progression->HasTraversal(static_cast<EHRTraversalAbility>(Ability));
}

void UHRTraversalComponent::RefreshUnlocks()
{
	if (ACharacter* Character = GetCharacter())
	{
		Character->JumpMaxCount = HasAbility(static_cast<uint8>(EHRTraversalAbility::DoubleJump)) ? 2 : 1;
	}
}

void UHRTraversalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return;
	}
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	WallRunCooldown = FMath::Max(0.f, WallRunCooldown - DeltaTime);

	if (bMantling)
	{
		TickMantle(DeltaTime);
		return;
	}

	if (!Movement->IsFalling())
	{
		if (bWallRunning)
		{
			StopWallRun();
		}
		if (bGliding)
		{
			StopGlide();
		}
		return;
	}

	if (bWallRunning)
	{
		TickWallRun(DeltaTime);
		return;
	}

	// Assassin's Creed-style: holding forward into a ledge while airborne grabs it.
	AutoMantleTimer -= DeltaTime;
	if (AutoMantleTimer <= 0.f && !Character->GetLastMovementInputVector().IsNearlyZero())
	{
		AutoMantleTimer = 0.1f;
		if (TryMantle())
		{
			return;
		}
	}

	// Wall run when sprinting alongside a wall.
	if (bWantsSprint && WallRunCooldown <= 0.f && HasAbility(static_cast<uint8>(EHRTraversalAbility::WallRun))
		&& Character->GetVelocity().Size2D() >= WallRunMinSpeed)
	{
		FVector Normal;
		if (FindWall(Normal))
		{
			StartWallRun(Normal);
			return;
		}
	}

	// Glide once falling.
	if (bGlideHeld && !bGliding && Character->GetVelocity().Z < 0.f && HasAbility(static_cast<uint8>(EHRTraversalAbility::Glide)))
	{
		StartGlide();
	}
	if (bGliding)
	{
		FVector Velocity = Movement->Velocity;
		Velocity.Z = FMath::Max(Velocity.Z, -GlideMaxFallSpeed);
		Movement->Velocity = Velocity;
	}
}

bool UHRTraversalComponent::HandleJumpPressed(const FVector& ViewDirection)
{
	ACharacter* Character = GetCharacter();
	if (!Character || bMantling)
	{
		return bMantling;
	}
	bGlideHeld = true;
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

	if (bWallRunning)
	{
		// Wall jump: kick off away from the wall.
		const FVector Launch = WallNormal * 650.f + FVector(0.f, 0.f, 750.f) + Character->GetActorForwardVector() * 250.f;
		StopWallRun();
		WallRunCooldown = 0.25f;
		Character->LaunchCharacter(Launch, true, true);
		return true;
	}

	if (TryMantle())
	{
		return true;
	}

	if (Movement->IsFalling() && HasAbility(static_cast<uint8>(EHRTraversalAbility::Grapple)))
	{
		if (AHRGrapplePoint* Point = FindGrappleTarget(ViewDirection))
		{
			const FVector ToPoint = Point->GetActorLocation() - Character->GetActorLocation();
			Character->LaunchCharacter(ToPoint.GetSafeNormal() * GrappleSpeed + FVector(0.f, 0.f, 350.f), true, true);
			return true;
		}
	}
	return false;
}

void UHRTraversalComponent::HandleJumpReleased()
{
	bGlideHeld = false;
	if (bGliding)
	{
		StopGlide();
	}
}

bool UHRTraversalComponent::TryMantle()
{
	ACharacter* Character = GetCharacter();
	if (!Character || bMantling)
	{
		return false;
	}
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Location = Character->GetActorLocation();
	const FVector Forward = Character->GetActorForwardVector();
	const float FeetZ = Location.Z - HalfHeight;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRMantle), false, Character);
	UWorld* World = GetWorld();

	// 1. Is there a wall in front?
	FHitResult WallHit;
	const FVector WallStart = FVector(Location.X, Location.Y, FeetZ + MinMantleHeight);
	if (!World->LineTraceSingleByChannel(WallHit, WallStart, WallStart + Forward * (Radius + MantleReach), ECC_Visibility, Params)
		|| FMath::Abs(WallHit.ImpactNormal.Z) > 0.3f)
	{
		return false;
	}

	// 2. Is there a walkable ledge on top within reach?
	const FVector LedgeProbe = WallHit.ImpactPoint - WallHit.ImpactNormal * (Radius * 0.75f);
	const FVector DownStart(LedgeProbe.X, LedgeProbe.Y, FeetZ + MaxMantleHeight + 20.f);
	const FVector DownEnd(LedgeProbe.X, LedgeProbe.Y, FeetZ + MinMantleHeight);
	FHitResult LedgeHit;
	if (!World->LineTraceSingleByChannel(LedgeHit, DownStart, DownEnd, ECC_Visibility, Params)
		|| LedgeHit.bStartPenetrating || LedgeHit.ImpactNormal.Z < 0.7f)
	{
		return false;
	}

	// 3. Does the capsule fit on top?
	const FVector Target = LedgeHit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 3.f);
	if (World->OverlapBlockingTestByChannel(Target, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params))
	{
		return false;
	}

	StopWallRun();
	StopGlide();
	bMantling = true;
	MantleAlpha = 0.f;
	MantleStart = Location;
	MantleEnd = Target;
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	return true;
}

void UHRTraversalComponent::TickMantle(float DeltaTime)
{
	ACharacter* Character = GetCharacter();
	MantleAlpha = FMath::Min(1.f, MantleAlpha + DeltaTime / FMath::Max(0.05f, MantleDuration));

	// Rise first, then move over the edge.
	const float Vertical = FMath::InterpEaseOut(0.f, 1.f, FMath::Min(1.f, MantleAlpha * 1.6f), 2.f);
	const float Horizontal = FMath::InterpEaseIn(0.f, 1.f, MantleAlpha, 2.f);
	FVector NewLocation = FMath::Lerp(MantleStart, MantleEnd, Horizontal);
	NewLocation.Z = FMath::Lerp(MantleStart.Z, MantleEnd.Z, Vertical);
	Character->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

	if (MantleAlpha >= 1.f)
	{
		bMantling = false;
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

bool UHRTraversalComponent::FindWall(FVector& OutNormal) const
{
	const ACharacter* Character = GetCharacter();
	const FVector Location = Character->GetActorLocation();
	const FVector Right = Character->GetActorRightVector();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRWallRun), false, Character);

	for (const float Side : { 1.f, -1.f })
	{
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Location, Location + Right * Side * 85.f, ECC_Visibility, Params)
			&& FMath::Abs(Hit.ImpactNormal.Z) < 0.2f)
		{
			OutNormal = Hit.ImpactNormal;
			return true;
		}
	}
	return false;
}

void UHRTraversalComponent::StartWallRun(const FVector& Normal)
{
	ACharacter* Character = GetCharacter();
	bWallRunning = true;
	WallNormal = Normal;
	WallRunTime = 0.f;
	Character->GetCharacterMovement()->GravityScale = WallRunGravity;
	FVector Velocity = Character->GetCharacterMovement()->Velocity;
	Velocity.Z = FMath::Max(Velocity.Z, 150.f);
	Character->GetCharacterMovement()->Velocity = Velocity;
}

void UHRTraversalComponent::TickWallRun(float DeltaTime)
{
	ACharacter* Character = GetCharacter();
	WallRunTime += DeltaTime;

	FVector Normal;
	if (WallRunTime > WallRunMaxTime || !bWantsSprint || !FindWall(Normal))
	{
		StopWallRun();
		WallRunCooldown = 0.4f;
		return;
	}
	WallNormal = Normal;

	// Run along the wall in the direction we are facing.
	FVector Along = FVector::CrossProduct(WallNormal, FVector::UpVector);
	if (FVector::DotProduct(Along, Character->GetActorForwardVector()) < 0.f)
	{
		Along = -Along;
	}
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const float VerticalSpeed = FMath::Max(Movement->Velocity.Z, -100.f);
	Movement->Velocity = Along * WallRunSpeed - WallNormal * 50.f + FVector(0.f, 0.f, VerticalSpeed);
}

void UHRTraversalComponent::StopWallRun()
{
	if (!bWallRunning)
	{
		return;
	}
	bWallRunning = false;
	if (ACharacter* Character = GetCharacter())
	{
		Character->GetCharacterMovement()->GravityScale = DefaultGravityScale;
	}
}

void UHRTraversalComponent::StartGlide()
{
	ACharacter* Character = GetCharacter();
	bGliding = true;
	Character->GetCharacterMovement()->GravityScale = GlideGravity;
	Character->GetCharacterMovement()->AirControl = 0.9f;
}

void UHRTraversalComponent::StopGlide()
{
	if (!bGliding)
	{
		return;
	}
	bGliding = false;
	if (ACharacter* Character = GetCharacter())
	{
		Character->GetCharacterMovement()->GravityScale = DefaultGravityScale;
		Character->GetCharacterMovement()->AirControl = DefaultAirControl;
	}
}

AHRGrapplePoint* UHRTraversalComponent::FindGrappleTarget(const FVector& ViewDirection) const
{
	const ACharacter* Character = GetCharacter();
	if (!Character)
	{
		return nullptr;
	}
	const FVector Location = Character->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRGrapple), false, Character);

	AHRGrapplePoint* Best = nullptr;
	float BestDot = 0.85f;
	for (TActorIterator<AHRGrapplePoint> It(GetWorld()); It; ++It)
	{
		const FVector ToPoint = It->GetActorLocation() - Location;
		if (ToPoint.SizeSquared() > FMath::Square(GrappleRange))
		{
			continue;
		}
		const float Dot = FVector::DotProduct(ViewDirection.GetSafeNormal(), ToPoint.GetSafeNormal());
		if (Dot <= BestDot)
		{
			continue;
		}
		FHitResult Hit;
		Params.AddIgnoredActor(*It);
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Location, It->GetActorLocation(), ECC_Visibility, Params);
		if (!bBlocked)
		{
			BestDot = Dot;
			Best = *It;
		}
	}
	return Best;
}
