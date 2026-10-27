#include "World/HRInteractableActor.h"
#include "SharedArtLibrary.h"

#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Narrative/HRNarrativeSubsystem.h"

AHRInteractableActor::AHRInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetStaticMesh(HRGreybox::Cube());
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetMobility(EComponentMobility::Movable);
	RootComponent = Mesh;
}

void AHRInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	if (PersistentId.IsNone())
	{
		PersistentId = GetFName();
	}
	Material = HRGreybox::Tint(Mesh, Color);
	USharedArtLibrary::ApplyProp(Mesh, SharedAppearanceKey.IsNone() ? GetClass()->GetFName() : SharedAppearanceKey);
}

void AHRInteractableActor::SetColor(FLinearColor InColor)
{
	Color = InColor;
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

FName AHRInteractableActor::MakeFlag(const TCHAR* Suffix) const
{
	return FName(*FString::Printf(TEXT("%s.%s"), *PersistentId.ToString(), Suffix));
}

int32 AHRInteractableActor::GetPersistentFlag(const TCHAR* Suffix) const
{
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	return Narrative ? Narrative->GetFlag(MakeFlag(Suffix)) : 0;
}

void AHRInteractableActor::SetPersistentFlag(const TCHAR* Suffix, int32 Value)
{
	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->SetFlag(MakeFlag(Suffix), Value);
	}
}
