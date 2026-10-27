#include "World/HRCheckpoint.h"
#include "SharedArtLibrary.h"

#include "Characters/HRPlayerCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRStatics.h"
#include "Game/HRGameMode.h"
#include "Narrative/HRQuestSubsystem.h"

#define LOCTEXT_NAMESPACE "HRCheckpoint"

AHRCheckpoint::AHRCheckpoint()
{
	Mesh->SetStaticMesh(HRGreybox::Cylinder());
	Mesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.5f));
	Color = FLinearColor(0.25f, 0.22f, 0.2f);
	Tags.Add(TEXT("HeatSource"));

	Flame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Mesh);
	Flame->SetStaticMesh(HRGreybox::Sphere());
	// Children inherit the pedestal's squashed scale, so these are compensated.
	Flame->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	Flame->SetRelativeScale3D(FVector(0.33f, 0.33f, 1.8f));
	Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Mesh);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 400.f));
	Light->SetLightColor(FLinearColor(1.f, 0.55f, 0.2f));
	Light->SetAttenuationRadius(1400.f);
	Light->SetIntensity(6000.f);
}

void AHRCheckpoint::BeginPlay()
{
	Super::BeginPlay();
	HRGreybox::Tint(Flame, FLinearColor(1.f, 0.45f, 0.1f));
	if (bStartsDiscovered && !IsDiscovered())
	{
		Discover(false);
	}
	if (AHRGameMode* GameMode = GetWorld()->GetAuthGameMode<AHRGameMode>())
	{
		GameMode->RegisterCheckpoint(this);
	}
	RefreshVisuals();
}

bool AHRCheckpoint::IsDiscovered() const
{
	return GetPersistentFlag(TEXT("Discovered")) > 0;
}

void AHRCheckpoint::Discover(bool bNotify)
{
	SetPersistentFlag(TEXT("Discovered"), 1);
	if (bNotify)
	{
		UHRStatics::Notify(this, FText::Format(LOCTEXT("Kindled", "SHRINE KINDLED: {0}"), DisplayName), FLinearColor(1.f, 0.6f, 0.25f), true);
		USharedArtLibrary::PlayCue(this, TEXT("Checkpoint.Discover"), GetActorLocation());
	}
	if (UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this))
	{
		Quests->NotifyEvent(EHRObjectiveType::Interact, PersistentId, 1);
	}
	RefreshVisuals();
}

void AHRCheckpoint::RefreshVisuals()
{
	const bool bLit = IsDiscovered();
	Flame->SetVisibility(bLit);
	Light->SetIntensity(bLit ? 6000.f : 300.f);
}

FText AHRCheckpoint::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	return IsDiscovered()
		? FText::Format(LOCTEXT("Rest", "Rest at {0}"), DisplayName)
		: FText::Format(LOCTEXT("Kindle", "Kindle {0}"), DisplayName);
}

void AHRCheckpoint::Interact(AHRPlayerCharacter* Player)
{
	if (!IsDiscovered())
	{
		Discover(true);
	}
	Player->BeginRest(this);
}

FTransform AHRCheckpoint::GetRespawnTransform() const
{
	const FVector Location = GetActorLocation() + GetActorForwardVector() * 180.f + FVector(0.f, 0.f, 120.f);
	return FTransform((-GetActorForwardVector()).Rotation(), Location);
}

#undef LOCTEXT_NAMESPACE
