#include "World/HRPhysicsProp.h"
#include "SharedArtLibrary.h"

#include "Characters/HRCharacterBase.h"
#include "Components/HRCombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"

AHRPhysicsProp::AHRPhysicsProp()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetStaticMesh(HRGreybox::Cube());
	Mesh->SetRelativeScale3D(FVector(0.8f));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->SetMassOverrideInKg(NAME_None, 60.f, true);
	RootComponent = Mesh;
}

void AHRPhysicsProp::MakeBarrel()
{
	bExplosive = true;
	Mesh->SetStaticMesh(HRGreybox::Cylinder());
	Mesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.9f));
}

void AHRPhysicsProp::BeginPlay()
{
	Super::BeginPlay();
	HRGreybox::Tint(Mesh, bExplosive ? FLinearColor(0.7f, 0.1f, 0.05f) : FLinearColor(0.45f, 0.3f, 0.15f));
	USharedArtLibrary::ApplyProp(Mesh, bExplosive ? TEXT("Physics.Barrel") : TEXT("Physics.Crate"));
	Mesh->OnComponentHit.AddDynamic(this, &AHRPhysicsProp::HandleHit);
}

void AHRPhysicsProp::ReactToElement(EHRDamageType Element, AActor* Source)
{
	if (bExplosive && (Element == EHRDamageType::Fire || Element == EHRDamageType::Lightning))
	{
		Explode(Source);
	}
}

void AHRPhysicsProp::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const float Speed = Mesh->GetPhysicsLinearVelocity().Size();
	AHRCharacterBase* Character = Cast<AHRCharacterBase>(OtherActor);
	if (!Character || Speed < ImpactDamageSpeed || Now - LastImpactTime < 0.5)
	{
		return;
	}
	LastImpactTime = Now;

	FHRDamageInfo Damage;
	Damage.Amount = Speed * 0.03f;
	Damage.PostureDamage = Speed * 0.03f;
	Damage.bHeavy = true;
	Damage.Source = GetInstigator() ? Cast<AActor>(GetInstigator()) : nullptr;
	if (UHRCombatComponent* Combat = Character->GetCombat())
	{
		Combat->ReceiveHit(Damage);
	}
}

void AHRPhysicsProp::Explode(AActor* Source)
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;
	UWorld* World = GetWorld();
	const FVector Origin = GetActorLocation();

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRExplosion), false, this);
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(ExplosionRadius), Params);

	TSet<AActor*> Damaged;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Damaged.Contains(Actor))
		{
			continue;
		}
		Damaged.Add(Actor);
		const FVector Direction = (Actor->GetActorLocation() - Origin).GetSafeNormal();
		const float Falloff = 1.f - FMath::Clamp(FVector::Dist(Actor->GetActorLocation(), Origin) / ExplosionRadius, 0.f, 0.8f);

		if (AHRCharacterBase* Character = Cast<AHRCharacterBase>(Actor))
		{
			FHRDamageInfo Damage;
			Damage.Amount = ExplosionDamage * Falloff;
			Damage.PostureDamage = 60.f * Falloff;
			Damage.Type = EHRDamageType::Fire;
			Damage.bHeavy = true;
			Damage.bUnblockable = true;
			Damage.Source = Source;
			Character->GetCombat()->ReceiveHit(Damage);
			Character->LaunchCharacter(Direction * 700.f * Falloff + FVector(0.f, 0.f, 400.f * Falloff), true, true);
		}
		else if (AHRPhysicsProp* Prop = Cast<AHRPhysicsProp>(Actor))
		{
			// Chain reaction with a beat of delay so it reads.
			if (Prop->bExplosive)
			{
				TWeakObjectPtr<AHRPhysicsProp> WeakProp = Prop;
				TWeakObjectPtr<AActor> WeakSource = Source;
				FTimerHandle Chain;
				World->GetTimerManager().SetTimer(Chain, FTimerDelegate::CreateLambda([WeakProp, WeakSource]()
				{
					if (WeakProp.IsValid())
					{
						WeakProp->Explode(WeakSource.Get());
					}
				}), 0.15f, false);
			}
			Prop->GetMesh()->AddImpulse(Direction * 1500.f * Falloff, NAME_None, true);
		}
		else if (UPrimitiveComponent* Component = Overlap.GetComponent())
		{
			if (Component->IsSimulatingPhysics())
			{
				Component->AddImpulse(Direction * 1500.f * Falloff, NAME_None, true);
			}
		}
	}
	UAISense_Hearing::ReportNoiseEvent(World, Origin, 1.f, Source, 4000.f);
	USharedArtLibrary::PlayCue(this, TEXT("Physics.Explosion"), Origin);
	Destroy();
}
