#include "World/HRProjectile.h"
#include "SharedArtLibrary.h"

#include "Components/HRCombatComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRInteractable.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Systems/HRFactionSubsystem.h"

AHRProjectile::AHRProjectile()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(14.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->SetCanEverAffectNavigation(false);
	RootComponent = Collision;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetStaticMesh(HRGreybox::Sphere());
	Visual->SetRelativeScale3D(FVector(0.3f));
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 2400.f;
	Movement->MaxSpeed = 4000.f;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 5.f;
}

void AHRProjectile::BeginPlay()
{
	Super::BeginPlay();
	FLinearColor Color = FLinearColor(0.9f, 0.9f, 0.9f);
	switch (Damage.Type)
	{
	case EHRDamageType::Fire: Color = FLinearColor(1.f, 0.4f, 0.05f); break;
	case EHRDamageType::Frost: Color = FLinearColor(0.5f, 0.8f, 1.f); break;
	case EHRDamageType::Lightning: Color = FLinearColor(0.9f, 0.9f, 0.3f); break;
	case EHRDamageType::Arcane: Color = FLinearColor(0.7f, 0.3f, 1.f); break;
	case EHRDamageType::Blight: Color = FLinearColor(0.4f, 0.7f, 0.1f); break;
	default: break;
	}
	HRGreybox::Tint(Visual, Color);
	Collision->OnComponentHit.AddDynamic(this, &AHRProjectile::HandleHit);
}

AHRProjectile* AHRProjectile::Launch(AActor* Shooter, const FVector& Location, const FVector& Direction, const FHRDamageInfo& InDamage, float Speed, float GravityScale)
{
	if (!Shooter || !Shooter->GetWorld())
	{
		return nullptr;
	}
	const FTransform SpawnTransform(Direction.Rotation(), Location);
	AHRProjectile* Projectile = Shooter->GetWorld()->SpawnActorDeferred<AHRProjectile>(AHRProjectile::StaticClass(), SpawnTransform, Shooter, Cast<APawn>(Shooter),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return nullptr;
	}
	Projectile->Damage = InDamage;
	Projectile->SplashRadius = GravityScale > 0.3f ? 300.f : 0.f;
	Projectile->Movement->InitialSpeed = Speed;
	Projectile->Movement->ProjectileGravityScale = GravityScale;
	Projectile->Collision->IgnoreActorWhenMoving(Shooter, true);
	Projectile->FinishSpawning(SpawnTransform);
	Projectile->Movement->Velocity = Direction.GetSafeNormal() * Speed;
	return Projectile;
}

void AHRProjectile::ApplyTo(AActor* Target, const FVector& Direction)
{
	if (!Target || Target == GetOwner())
	{
		return;
	}
	if (UHRCombatComponent* Combat = Target->FindComponentByClass<UHRCombatComponent>())
	{
		if (!UHRFactionSubsystem::AreFriendly(GetOwner(), Target))
		{
			Combat->ReceiveHit(Damage);
			UHRFactionSubsystem::NotifyAttack(GetOwner(), Target);
		}
	}
	if (IHRElementalReactive* Reactive = Cast<IHRElementalReactive>(Target))
	{
		Reactive->ReactToElement(Damage.Type, GetOwner());
	}
}

void AHRProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasHit)
	{
		return;
	}
	bHasHit = true;
	const FVector Direction = Movement->Velocity.GetSafeNormal();

	if (SplashRadius > 0.f)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionObjectQueryParams ObjectParams(FCollisionObjectQueryParams::AllDynamicObjects);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(HRSplash), false, this);
		GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(SplashRadius), Params);
		TSet<AActor*> Applied;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Actor = Overlap.GetActor();
			if (Actor && !Applied.Contains(Actor))
			{
				Applied.Add(Actor);
				ApplyTo(Actor, Direction);
			}
		}
	}
	else
	{
		ApplyTo(OtherActor, Direction);
	}

	if (OtherComp && OtherComp->IsSimulatingPhysics())
	{
		OtherComp->AddImpulseAtLocation(Direction * 30000.f, Hit.ImpactPoint);
	}
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 0.6f, GetOwner(), 1500.f);
	USharedArtLibrary::PlayCue(this, FName(*(FString(TEXT("Projectile.")) + StaticEnum<EHRDamageType>()->GetNameStringByValue(static_cast<int64>(Damage.Type)) + TEXT(".Impact"))), Hit.ImpactPoint);
	Destroy();
}
