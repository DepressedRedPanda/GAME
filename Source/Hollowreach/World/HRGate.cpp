#include "World/HRGate.h"
#include "SharedArtLibrary.h"

#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"

AHRGate::AHRGate()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	Door->SetupAttachment(Root);
	Door->SetStaticMesh(HRGreybox::Cube());
	Door->SetMobility(EComponentMobility::Movable);
	Door->SetCollisionProfileName(TEXT("BlockAll"));
	SetGateSize(FVector(400.f, 40.f, 400.f));
}

void AHRGate::SetGateSize(const FVector& SizeCm)
{
	Height = SizeCm.Z;
	Door->SetRelativeScale3D(SizeCm / 100.f);
	Door->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f - Offset));
}

void AHRGate::BeginPlay()
{
	Super::BeginPlay();
	HRGreybox::Tint(Door, bIsFogGate ? FLinearColor(0.85f, 0.85f, 0.9f) : Color);
	// Fog gates retain their existing visibility behavior; only solid gates receive cosmetic meshes.
	if (!bIsFogGate) USharedArtLibrary::ApplyProp(Door, TEXT("Gate"));
	if (bStartsOpen)
	{
		bOpen = true;
		Offset = bIsFogGate ? 0.f : Height - 20.f;
		Door->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f - Offset));
		if (bIsFogGate)
		{
			Door->SetVisibility(false);
			Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AHRGate::Open()
{
	bOpen = true;
	if (bIsFogGate)
	{
		Door->SetVisibility(false);
		Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AHRGate::Close()
{
	bOpen = false;
	if (bIsFogGate)
	{
		Door->SetVisibility(true);
		Door->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

void AHRGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bIsFogGate)
	{
		return;
	}
	// Sinks into the ground when open.
	const float Target = bOpen ? Height - 20.f : 0.f;
	Offset = FMath::FInterpConstantTo(Offset, Target, DeltaSeconds, OpenSpeed);
	Door->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f - Offset));
}
