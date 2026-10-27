#include "Puzzles/HRPuzzleElement.h"

#include "Characters/HRPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRStatics.h"
#include "GameFramework/Pawn.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "World/HRPhysicsProp.h"

#define LOCTEXT_NAMESPACE "HRPuzzle"

AHRPuzzleElement::AHRPuzzleElement()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;

	Indicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Indicator"));
	Indicator->SetupAttachment(Mesh);
	Indicator->SetStaticMesh(HRGreybox::Cube());
	Indicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlateTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PlateTrigger"));
	PlateTrigger->SetupAttachment(Mesh);
	PlateTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PlateTrigger->SetBoxExtent(FVector(50.f, 50.f, 60.f));
	PlateTrigger->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Color = FLinearColor(0.4f, 0.38f, 0.35f);
}

void AHRPuzzleElement::BeginPlay()
{
	// Shape depends on element type.
	switch (ElementType)
	{
	case EHRPuzzleElementType::Lever:
		Mesh->SetRelativeScale3D(FVector(0.4f, 0.4f, 1.2f));
		Indicator->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.6f));
		Indicator->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
		break;
	case EHRPuzzleElementType::Dial:
		Mesh->SetStaticMesh(HRGreybox::Cylinder());
		Mesh->SetRelativeScale3D(FVector(1.f, 1.f, 1.6f));
		Indicator->SetRelativeScale3D(FVector(0.7f, 0.15f, 0.15f));
		Indicator->SetRelativeLocation(FVector(45.f, 0.f, 40.f));
		break;
	case EHRPuzzleElementType::PressurePlate:
		Mesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.1f));
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
		Indicator->SetVisibility(false);
		PlateTrigger->SetRelativeLocation(FVector(0.f, 0.f, 500.f));
		PlateTrigger->SetBoxExtent(FVector(45.f, 45.f, 400.f));
		break;
	case EHRPuzzleElementType::Brazier:
		Mesh->SetStaticMesh(HRGreybox::Cylinder());
		Mesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.f));
		Indicator->SetStaticMesh(HRGreybox::Sphere());
		Indicator->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.9f));
		Indicator->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
		break;
	case EHRPuzzleElementType::Glyph:
		Mesh->SetRelativeScale3D(FVector(0.3f, 1.4f, 2.f));
		Indicator->SetRelativeScale3D(FVector(1.1f, 0.6f, 0.3f));
		Indicator->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
		break;
	}
	Super::BeginPlay();
	SetActorTickEnabled(ElementType == EHRPuzzleElementType::PressurePlate);
	UpdateVisuals();
}

void AHRPuzzleElement::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bLocked)
	{
		return;
	}
	TArray<AActor*> Overlapping;
	PlateTrigger->GetOverlappingActors(Overlapping);
	bool bPressed = false;
	for (const AActor* Actor : Overlapping)
	{
		if (Cast<APawn>(Actor) || Cast<AHRPhysicsProp>(Actor))
		{
			bPressed = true;
			break;
		}
	}
	if (bPressed != (State == 1))
	{
		SetState(bPressed ? 1 : 0);
	}
}

bool AHRPuzzleElement::CanInteract(const AHRPlayerCharacter* Player) const
{
	if (bLocked)
	{
		return false;
	}
	return ElementType == EHRPuzzleElementType::Lever || ElementType == EHRPuzzleElementType::Dial || ElementType == EHRPuzzleElementType::Glyph;
}

FText AHRPuzzleElement::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	switch (ElementType)
	{
	case EHRPuzzleElementType::Lever: return LOCTEXT("Pull", "Pull lever");
	case EHRPuzzleElementType::Dial: return LOCTEXT("Turn", "Turn the stone");
	case EHRPuzzleElementType::Glyph: return LOCTEXT("Read", "Read the glyphs");
	default: return FText::GetEmpty();
	}
}

void AHRPuzzleElement::Interact(AHRPlayerCharacter* Player)
{
	switch (ElementType)
	{
	case EHRPuzzleElementType::Lever:
		SetState(State == 0 ? 1 : 0);
		break;
	case EHRPuzzleElementType::Dial:
		SetState((State + 1) % FMath::Max(1, NumStates));
		break;
	case EHRPuzzleElementType::Glyph:
	{
		const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
		if (!RequiredKnowledge.IsNone() && (!Narrative || !Narrative->HasKnowledge(RequiredKnowledge)))
		{
			UHRStatics::Notify(this, LOCTEXT("Unreadable", "The symbols mean nothing to you... yet."), FLinearColor::Gray);
			return;
		}
		UHRStatics::Notify(this, GlyphText, FLinearColor(0.85f, 0.8f, 0.6f), true);
		SetState(1);
		break;
	}
	default:
		break;
	}
}

void AHRPuzzleElement::ReactToElement(EHRDamageType Element, AActor* Source)
{
	if (!bLocked && ElementType == EHRPuzzleElementType::Brazier && Element == EHRDamageType::Fire && State == 0)
	{
		SetState(1);
	}
}

void AHRPuzzleElement::SetState(int32 NewState)
{
	if (State == NewState)
	{
		return;
	}
	State = NewState;
	UpdateVisuals();
	OnStateChanged.Broadcast(this, State);
}

void AHRPuzzleElement::UpdateVisuals()
{
	switch (ElementType)
	{
	case EHRPuzzleElementType::Lever:
		Indicator->SetRelativeRotation(FRotator(State == 0 ? 35.f : -35.f, 0.f, 0.f));
		HRGreybox::Tint(Indicator, State == 0 ? FLinearColor(0.5f, 0.2f, 0.1f) : FLinearColor(0.2f, 0.7f, 0.3f));
		break;
	case EHRPuzzleElementType::Dial:
		Mesh->SetRelativeRotation(FRotator(0.f, State * (360.f / FMath::Max(1, NumStates)), 0.f));
		HRGreybox::Tint(Indicator, FLinearColor(0.9f, 0.8f, 0.3f));
		break;
	case EHRPuzzleElementType::PressurePlate:
		SetColor(State == 0 ? Color : FLinearColor(0.3f, 0.7f, 0.35f));
		break;
	case EHRPuzzleElementType::Brazier:
		Indicator->SetVisibility(State != 0);
		HRGreybox::Tint(Indicator, FLinearColor(1.f, 0.5f, 0.1f));
		break;
	case EHRPuzzleElementType::Glyph:
		HRGreybox::Tint(Indicator, State == 0 ? FLinearColor(0.3f, 0.3f, 0.5f) : FLinearColor(0.6f, 0.8f, 1.f));
		break;
	}
}

#undef LOCTEXT_NAMESPACE
