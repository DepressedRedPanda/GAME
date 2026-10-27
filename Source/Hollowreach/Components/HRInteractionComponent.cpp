#include "Components/HRInteractionComponent.h"

#include "Characters/HRPlayerCharacter.h"
#include "Core/HRInteractable.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

UHRInteractionComponent::UHRInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

AHRPlayerCharacter* UHRInteractionComponent::GetPlayer() const
{
	return Cast<AHRPlayerCharacter>(GetOwner());
}

void UHRInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	FindFocus();
}

void UHRInteractionComponent::FindFocus()
{
	AHRPlayerCharacter* Player = GetPlayer();
	if (!Player)
	{
		return;
	}

	const FVector Origin = Player->GetActorLocation() + Player->GetActorForwardVector() * 80.f;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HRInteract), false, Player);
	FCollisionObjectQueryParams ObjectParams(FCollisionObjectQueryParams::AllObjects);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius), Params);

	AActor* Best = nullptr;
	float BestScore = -TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		const IHRInteractable* Interactable = Cast<IHRInteractable>(Actor);
		if (!Interactable || !Interactable->CanInteract(Player))
		{
			continue;
		}
		const FVector ToActor = Actor->GetActorLocation() - Player->GetActorLocation();
		const float Facing = FVector::DotProduct(Player->GetActorForwardVector(), ToActor.GetSafeNormal2D());
		const float Score = Facing * 2.f - ToActor.Size() / Radius;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Actor;
		}
	}
	Focused = Best;
}

FText UHRInteractionComponent::GetPrompt() const
{
	if (const IHRInteractable* Interactable = Cast<IHRInteractable>(Focused.Get()))
	{
		return Interactable->GetInteractionPrompt(GetPlayer());
	}
	return FText::GetEmpty();
}

bool UHRInteractionComponent::Interact()
{
	AHRPlayerCharacter* Player = GetPlayer();
	FindFocus();
	IHRInteractable* Interactable = Cast<IHRInteractable>(Focused.Get());
	if (!Player || !Interactable || !Interactable->CanInteract(Player))
	{
		return false;
	}
	Interactable->Interact(Player);
	return true;
}
