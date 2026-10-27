#include "World/HREndingAltar.h"
#include "Core/HRCampaignRules.h"

#include "Components/StaticMeshComponent.h"
#include "Core/HRGreybox.h"
#include "Core/HRStatics.h"
#include "Narrative/HRNarrativeSubsystem.h"

AHREndingAltar::AHREndingAltar()
{
	Mesh->SetRelativeScale3D(FVector(2.f, 1.2f, 1.f));
	Color = FLinearColor(0.8f, 0.7f, 0.5f);
}

bool AHREndingAltar::CanInteract(const AHRPlayerCharacter* Player) const
{
	const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
	return Narrative && Narrative->GetFlag(RequiredFlag) > 0 && Narrative->GetActiveEnding().IsNone()
		&& HRCampaignRules::CanResolveCampaign(Narrative->GetFlag(TEXT("Boss.PaleKeeper.Defeated")) > 0, Narrative->GetFlag(TEXT("Boss.Warden.Defeated")) > 0);
}

FText AHREndingAltar::GetInteractionPrompt(const AHRPlayerCharacter* Player) const
{
	return NSLOCTEXT("HR", "AltarPrompt", "Approach the First Flame");
}

void AHREndingAltar::Interact(AHRPlayerCharacter* Player)
{
	if (!CanInteract(Player)) return;
	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
	{
		Narrative->StartDialogue(DialogueId, this, TEXT("Altar"), Player);
	}
}
