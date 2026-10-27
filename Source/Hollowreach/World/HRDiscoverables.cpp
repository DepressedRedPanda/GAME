#include "World/HRDiscoverables.h"

#include "Characters/HRPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Components/HRTraversalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRStatics.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Narrative/HRQuestSubsystem.h"

#define LOCTEXT_NAMESPACE "HRDiscoverables"

// --- Collectible -------------------------------------------------------------------------------

AHRCollectible::AHRCollectible()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh->SetStaticMesh(HRGreybox::Cone());
	Mesh->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.45f));
	Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Color = FLinearColor(0.9f, 0.85f, 0.4f);
}

void AHRCollectible::BeginPlay()
{
	Super::BeginPlay();
	BaseZ = GetActorLocation().Z;
	if (GetPersistentFlag(TEXT("Collected")) > 0)
	{
		Destroy();
	}
}

bool AHRCollectible::IsRevealed() const
{
	if (!bHiddenUntilRevealed)
	{
		return true;
	}
	const AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(this);
	return Player && (Player->IsSpiritSightActive() || FVector::Dist(Player->GetActorLocation(), GetActorLocation()) < 220.f);
}

void AHRCollectible::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AddActorWorldRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));
	FVector Location = GetActorLocation();
	Location.Z = BaseZ + FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f) * 10.f;
	SetActorLocation(Location);

	const bool bNowRevealed = IsRevealed();
	if (bNowRevealed != bRevealed)
	{
		bRevealed = bNowRevealed;
		SetActorHiddenInGame(!bRevealed);
	}
}

bool AHRCollectible::CanInteract(const AHRPlayerCharacter* Player) const
{
	return IsRevealed();
}

FText AHRCollectible::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	return FText::Format(LOCTEXT("Take", "Take {0}"), DisplayName);
}

void AHRCollectible::Interact(AHRPlayerCharacter* Player)
{
	UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	switch (Type)
	{
	case EHRCollectibleType::LoreFragment:
		break;
	case EHRCollectibleType::SkillRelic:
		Player->GetProgression()->GrantSkillPoints(Amount);
		UHRStatics::Notify(this, FText::Format(LOCTEXT("Relic", "+{0} skill point(s)"), Amount), FLinearColor(0.6f, 0.8f, 1.f), true);
		break;
	case EHRCollectibleType::VigorSeed:
		Player->GetInventory()->MaxFlaskCharges += Amount;
		Player->GetInventory()->RefillFlasks();
		UHRStatics::Notify(this, LOCTEXT("Seed", "Your Ember Gourd can hold another draught."), FLinearColor(1.f, 0.6f, 0.3f), true);
		break;
	case EHRCollectibleType::EchoCache:
		Player->GetProgression()->AddEchoes(Amount, true);
		break;
	case EHRCollectibleType::TraversalRelic:
		Player->GetProgression()->GrantTraversal(Traversal, true);
		Player->GetTraversal()->RefreshUnlocks();
		break;
	}
	if (Narrative && !KnowledgeId.IsNone())
	{
		Narrative->AddKnowledge(KnowledgeId);
	}
	if (UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this))
	{
		Quests->NotifyEvent(EHRObjectiveType::Interact, PersistentId, 1);
	}
	SetPersistentFlag(TEXT("Collected"), 1);
	if (Narrative)
	{
		Narrative->AddFlag(TEXT("Stats.Collectibles"), 1);
	}
	Destroy();
}

// --- Clue --------------------------------------------------------------------------------------

AHRClue::AHRClue()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	Mesh->SetStaticMesh(HRGreybox::Sphere());
	Mesh->SetRelativeScale3D(FVector(0.25f));
	Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Color = FLinearColor(0.3f, 0.9f, 1.f);
}

void AHRClue::BeginPlay()
{
	Super::BeginPlay();
	if (ClueId.IsNone())
	{
		ClueId = PersistentId;
	}
}

bool AHRClue::IsExamined() const
{
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	return Narrative && Narrative->HasClue(ClueId);
}

void AHRClue::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bool bVisible = !IsExamined();
	if (bVisible && bRequiresSpiritSight)
	{
		const AHRPlayerCharacter* Player = UHRStatics::GetHRPlayer(this);
		bVisible = Player && Player->IsSpiritSightActive();
	}
	SetActorHiddenInGame(!bVisible);
}

bool AHRClue::CanInteract(const AHRPlayerCharacter* Player) const
{
	if (IsExamined())
	{
		return false;
	}
	return !bRequiresSpiritSight || (Player && Player->IsSpiritSightActive());
}

FText AHRClue::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	return FText::Format(LOCTEXT("Examine", "Examine {0}"), DisplayName);
}

void AHRClue::Interact(AHRPlayerCharacter* Player)
{
	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->AddClue(ClueId);
	}
}

// --- Landmark ----------------------------------------------------------------------------------

AHRLandmark::AHRLandmark()
{
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetBoxExtent(FVector(1500.f, 1500.f, 800.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetGenerateOverlapEvents(true);
	RootComponent = Trigger;
}

void AHRLandmark::BeginPlay()
{
	Super::BeginPlay();
	if (LandmarkId.IsNone())
	{
		LandmarkId = GetFName();
	}
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AHRLandmark::HandleOverlap);
}

void AHRLandmark::SetExtent(const FVector& Extent)
{
	Trigger->SetBoxExtent(Extent);
}

void AHRLandmark::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AHRPlayerCharacter* Player = Cast<AHRPlayerCharacter>(OtherActor);
	UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	if (!Player || !Narrative)
	{
		return;
	}
	const FName Flag(*FString::Printf(TEXT("%s.Discovered"), *LandmarkId.ToString()));
	if (Narrative->GetFlag(Flag) > 0)
	{
		return;
	}
	Narrative->SetFlag(Flag, 1);
	Narrative->AddFlag(TEXT("Stats.Landmarks"), 1);
	UHRStatics::Notify(this, DisplayName, FLinearColor(0.95f, 0.9f, 0.75f), true);
	Player->GetProgression()->AddEchoes(EchoReward, true);
	if (!KnowledgeId.IsNone())
	{
		Narrative->AddKnowledge(KnowledgeId);
	}
	if (UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this))
	{
		if (!StartsQuest.IsNone())
		{
			Quests->StartQuest(StartsQuest);
		}
		Quests->NotifyReach(LandmarkId);
	}
}

#undef LOCTEXT_NAMESPACE
