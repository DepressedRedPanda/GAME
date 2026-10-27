#include "World/HRStealthZone.h"
#include "SharedArtLibrary.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"

AHRStealthZone::AHRStealthZone()
{
	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	Volume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Volume->SetGenerateOverlapEvents(true);
	RootComponent = Volume;

	Grass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grass"));
	Grass->SetupAttachment(Volume);
	Grass->SetStaticMesh(HRGreybox::Cube());
	Grass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Grass->SetCanEverAffectNavigation(false);

	SetZoneSize(FVector(600.f, 600.f, 110.f));
}

void AHRStealthZone::SetZoneSize(const FVector& SizeCm)
{
	Volume->SetBoxExtent(SizeCm * 0.5f);
	Grass->SetRelativeScale3D(FVector(SizeCm.X / 100.f, SizeCm.Y / 100.f, SizeCm.Z / 100.f * 0.9f));
}

void AHRStealthZone::BeginPlay()
{
	Super::BeginPlay();
	HRGreybox::Tint(Grass, FLinearColor(0.12f, 0.35f, 0.08f));
	USharedArtLibrary::ApplyProp(Grass, TEXT("Stealth.Grass"));
}
