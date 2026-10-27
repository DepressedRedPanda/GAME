#include "World/HRDroppedEchoes.h"

#include "Characters/HRPlayerCharacter.h"
#include "Components/HRProgressionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRStatics.h"
#include "Game/HRGameMode.h"

AHRDroppedEchoes::AHRDroppedEchoes()
{
	Mesh->SetStaticMesh(HRGreybox::Sphere());
	Mesh->SetRelativeScale3D(FVector(0.5f));
	Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Color = FLinearColor(0.3f, 1.f, 0.6f);
}

FText AHRDroppedEchoes::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	return FText::Format(NSLOCTEXT("HR", "Reclaim", "Reclaim lost echoes ({0})"), Amount);
}

void AHRDroppedEchoes::Interact(AHRPlayerCharacter* Player)
{
	Player->GetProgression()->AddEchoes(Amount, true);
	UHRStatics::Notify(this, NSLOCTEXT("HR", "Reclaimed", "ECHOES RECLAIMED"), FLinearColor(0.4f, 1.f, 0.7f), true);
	if (AHRGameMode* GameMode = GetWorld()->GetAuthGameMode<AHRGameMode>())
	{
		GameMode->ClearDroppedEchoes();
	}
	Destroy();
}
