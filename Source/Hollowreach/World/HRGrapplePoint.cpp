#include "World/HRGrapplePoint.h"

#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"

AHRGrapplePoint::AHRGrapplePoint()
{
	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetStaticMesh(HRGreybox::Sphere());
	Marker->SetRelativeScale3D(FVector(0.4f));
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = Marker;
}

void AHRGrapplePoint::BeginPlay()
{
	Super::BeginPlay();
	HRGreybox::Tint(Marker, FLinearColor(0.2f, 0.9f, 1.f));
}
