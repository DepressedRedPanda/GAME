#include "World/HRResourceNode.h"

#include "Characters/HRPlayerCharacter.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRStealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Systems/HRContentSubsystem.h"
#include "Systems/HRTimeWeatherSubsystem.h"

AHRResourceNode::AHRResourceNode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 2.f;
	Color = FLinearColor(0.2f, 0.6f, 0.2f);
}

void AHRResourceNode::BeginPlay()
{
	if (PersistentId.IsNone())
	{
		const FVector Place = GetActorLocation();
		PersistentId = FName(*FString::Printf(TEXT("Resource.%s.%d.%d.%d"), *ItemId.ToString(), FMath::RoundToInt(Place.X), FMath::RoundToInt(Place.Y), FMath::RoundToInt(Place.Z)));
	}
	if (SharedAppearanceKey.IsNone())
	{
		FString Resource = ItemId.ToString();
		Resource.RemoveFromStart(TEXT("Item."));
		SharedAppearanceKey = FName(*(TEXT("Resource.") + Resource));
	}
	Super::BeginPlay();
	if (GetPersistentFlag(TEXT("Depleted")) > 0)
	{
		DepletedAtHours = GetPersistentFlag(TEXT("DepletedMilliHours")) / 1000.f;
		SetDepleted(true);
	}
}

FText AHRResourceNode::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	return FText::Format(NSLOCTEXT("HR", "Gather", "Gather {0}"), DisplayName);
}

void AHRResourceNode::Interact(AHRPlayerCharacter* Player)
{
	if (bDepleted)
	{
		return;
	}
	Player->GetInventory()->AddItem(ItemId, FMath::RandRange(MinQuantity, FMath::Max(MinQuantity, MaxQuantity)), true);
	if (!BonusItemId.IsNone() && FMath::FRand() < BonusChance)
	{
		Player->GetInventory()->AddItem(BonusItemId, 1, true);
	}
	// Mining and chopping are loud.
	Player->GetStealth()->MakeNoise(0.5f);

	if (RespawnHours > 0.f)
	{
		const UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>();
		DepletedAtHours = TimeWeather ? TimeWeather->GetTotalHours() : 0.f;
		SetDepleted(true);
	}
}

void AHRResourceNode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bDepleted)
	{
		return;
	}
	const UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>();
	if (TimeWeather && TimeWeather->GetTotalHours() - DepletedAtHours >= RespawnHours)
	{
		SetDepleted(false);
	}
}

void AHRResourceNode::SetDepleted(bool bInDepleted)
{
	if (bDepleted == bInDepleted) return;
	bDepleted = bInDepleted;
	SetPersistentFlag(TEXT("Depleted"), bDepleted ? 1 : 0);
	if (bDepleted) SetPersistentFlag(TEXT("DepletedMilliHours"), FMath::RoundToInt(DepletedAtHours * 1000.f));
	SetColor(bDepleted ? Color * 0.3f : Color);
	Mesh->SetRelativeScale3D(Mesh->GetRelativeScale3D() * (bDepleted ? 0.6f : 1.f / 0.6f));
}
