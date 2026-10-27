#include "World/HRMovingPlatform.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

AHRMovingPlatform::AHRMovingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));
	Platform->SetupAttachment(Root);
	Platform->SetStaticMesh(HRGreybox::Cube());
	Platform->SetMobility(EComponentMobility::Movable);
	Platform->SetCollisionProfileName(TEXT("BlockAll"));

	StepTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("StepTrigger"));
	StepTrigger->SetupAttachment(Root);
	StepTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	SetPlatformSize(FVector(300.f, 300.f, 40.f));
}

void AHRMovingPlatform::SetPlatformSize(const FVector& SizeCm)
{
	Platform->SetRelativeScale3D(SizeCm / 100.f);
	StepTrigger->SetBoxExtent(FVector(SizeCm.X * 0.5f, SizeCm.Y * 0.5f, 30.f));
	StepTrigger->SetRelativeLocation(FVector(0.f, 0.f, SizeCm.Z * 0.5f + 30.f));
}

void AHRMovingPlatform::BeginPlay()
{
	Super::BeginPlay();
	StartLocation = GetActorLocation();
	HRGreybox::Tint(Platform, bCrumbles ? FLinearColor(0.55f, 0.35f, 0.25f) : Color);
	StepTrigger->OnComponentBeginOverlap.AddDynamic(this, &AHRMovingPlatform::HandleStep);
	SetActorTickEnabled(!EndOffset.IsNearlyZero());
}

void AHRMovingPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Time = GetWorld()->GetTimeSeconds() + PhaseOffset;
	const float Alpha = 0.5f - 0.5f * FMath::Cos(2.f * PI * Time / FMath::Max(0.5f, Period));
	SetActorLocation(StartLocation + EndOffset * Alpha);
}

void AHRMovingPlatform::HandleStep(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bCrumbles || bCrumbling || !Cast<ACharacter>(OtherActor))
	{
		return;
	}
	bCrumbling = true;
	TWeakObjectPtr<AHRMovingPlatform> WeakThis = this;
	GetWorldTimerManager().SetTimer(CrumbleTimer, FTimerDelegate::CreateLambda([WeakThis]()
	{
		if (!WeakThis.IsValid())
		{
			return;
		}
		WeakThis->SetSolid(false);
		FTimerHandle Respawn;
		WeakThis->GetWorldTimerManager().SetTimer(Respawn, FTimerDelegate::CreateLambda([WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->SetSolid(true);
				WeakThis->bCrumbling = false;
			}
		}), WeakThis->RespawnDelay, false);
	}), CrumbleDelay, false);
}

void AHRMovingPlatform::SetSolid(bool bSolid)
{
	Platform->SetVisibility(bSolid);
	Platform->SetCollisionEnabled(bSolid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}
