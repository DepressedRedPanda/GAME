#include "Systems/HRSaveSubsystem.h"

#include "Characters/HRPlayerCharacter.h"
#include "Characters/HRCompanionCharacter.h"
#include "Core/HRCampaignRules.h"
#include "World/HRBuiltShelter.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/HRAbilityComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Components/HRSurvivalComponent.h"
#include "Components/HRTraversalComponent.h"
#include "Core/HRStatics.h"
#include "Game/HRGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Narrative/HRQuestSubsystem.h"
#include "Systems/HRFactionSubsystem.h"
#include "Systems/HRSaveGame.h"
#include "Systems/HRTimeWeatherSubsystem.h"

bool UHRSaveSubsystem::HasSave() const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

bool UHRSaveSubsystem::DeleteSave()
{
	// A fresh-start marker prevents old slots from being re-imported after New Game.
	UHRSaveGame* Fresh = NewObject<UHRSaveGame>(this);
	Fresh->bFreshStart = true;
	if (!UGameplayStatics::SaveGameToSlot(Fresh, SlotName, 0)) return false;
	LoadedSave = nullptr;
	bPlayerStatePending = false;
	PlacedShelters.Reset();
	ResetWorldState();
	return true;
}

void UHRSaveSubsystem::ResetWorldState()
{
	if (UHRNarrativeSubsystem* Narrative = GetGameInstance()->GetSubsystem<UHRNarrativeSubsystem>())
	{
		Narrative->EndDialogue();
		Narrative->RestoreState({}, {}, {}, {}, NAME_None);
	}
	if (UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>()) Quests->RestoreState({});
	if (UHRFactionSubsystem* Factions = GetGameInstance()->GetSubsystem<UHRFactionSubsystem>()) Factions->RestoreReputation({});
	PlacedShelters.Reset();
}

bool UHRSaveSubsystem::SaveProgress(AHRPlayerCharacter* Player)
{
	if (!Player || Player->IsDead() || Player->IsDowned())
	{
		return false;
	}
	if (bUnsupportedSaveVersion)
	{
		UHRStatics::Notify(Player, NSLOCTEXT("HR", "UnsupportedSave", "An existing save cannot be read. Saving is paused to protect its progress."), FLinearColor(1.f, 0.3f, 0.3f));
		return false;
	}
	UHRSaveGame* Save = Cast<UHRSaveGame>(UGameplayStatics::CreateSaveGameObject(UHRSaveGame::StaticClass()));
	Save->Timestamp = FDateTime::Now();
	Save->Shelters = PlacedShelters;
	if (LoadedSave)
	{
		Save->bImportedHollowreach = LoadedSave->bImportedHollowreach;
		Save->bImportedVeil = LoadedSave->bImportedVeil;
		Save->LegacyUnmappedItems = LoadedSave->LegacyUnmappedItems;
		Save->LegacyHollowreachEnding = LoadedSave->LegacyHollowreachEnding;
	}

	// Player
	const UHRProgressionComponent* Progression = Player->GetProgression();
	Save->Level = Progression->GetLevel();
	Save->Echoes = Progression->GetEchoes();
	Save->SkillPoints = Progression->GetSkillPoints();
	Save->StartingClass = Progression->GetStartingClass();
	const UHRAttributeComponent* Stats = Player->GetAttributes();
	Save->bHasVitals = true;
	Save->bHasPlayerTransform = true;
	Save->PlayerTransform = Player->GetActorTransform();
	Save->HealthFraction = Stats->GetHealthPercent();
	Save->StaminaFraction = Stats->GetStamina() / FMath::Max(1.f, Stats->GetMaxStamina());
	Save->FocusFraction = Stats->GetFocus() / FMath::Max(1.f, Stats->GetMaxFocus());
	if (const AHRCompanionCharacter* Companion = Player->GetCompanion())
	{
		Save->bHasCompanionState = Companion->IsRecruited();
		Save->CompanionCommand = static_cast<int32>(Companion->GetCommand());
		Save->CompanionHoldLocation = Companion->GetHoldLocation();
		Save->CompanionRevives = Companion->GetRevivesRemaining();
	}
	Save->Attributes = Progression->GetAttributeMap();
	Save->Skills = Progression->GetUnlockedSkills();
	Save->Traversal = Progression->GetTraversal();
	Save->AbilitySlots = Player->GetAbilities()->GetSlotIds();

	const UHRInventoryComponent* Inventory = Player->GetInventory();
	Save->Items = Inventory->GetItems();
	Save->Equipped = Inventory->GetEquippedMap();
	Save->QuickItem = Inventory->GetQuickItemId();
	Save->MaxFlasks = Inventory->MaxFlaskCharges;
	Save->Injuries = Player->GetAttributes()->GetInjuries();

	const UHRSurvivalComponent* Survival = Player->GetSurvival();
	Save->Food = Survival->GetFood();
	Save->Water = Survival->GetWater();
	Save->Warmth = Survival->GetWarmth();
	Save->Fatigue = Survival->GetFatigue();
	Save->Infection = Survival->GetInfection();

	// World
	if (const UHRNarrativeSubsystem* Narrative = GetGameInstance()->GetSubsystem<UHRNarrativeSubsystem>())
	{
		Save->Flags = Narrative->GetAllFlags();
		Save->Knowledge = Narrative->GetKnowledgeList();
		Save->Clues = Narrative->GetClueList();
		Save->Deductions = Narrative->GetSolvedDeductions();
		Save->Ending = Narrative->GetActiveEnding();
		Save->TotalKills = Narrative->GetFlag(TEXT("Stats.Kills"));
	}
	if (const UHRFactionSubsystem* Factions = GetGameInstance()->GetSubsystem<UHRFactionSubsystem>())
	{
		Save->Reputation = Factions->GetAllReputation();
	}
	if (const UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>())
	{
		Save->Quests = Quests->GetAllProgress();
	}
	if (const UHRTimeWeatherSubsystem* TimeWeather = Player->GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>())
	{
		Save->TimeOfDay = TimeWeather->GetTimeOfDay();
		Save->WorldDay = TimeWeather->GetDay();
		Save->HoursUntilWeatherChange = TimeWeather->GetWeatherHoursRemaining();
		Save->Weather = TimeWeather->GetWeather();
	}
	if (const AHRGameMode* GameMode = Player->GetWorld()->GetAuthGameMode<AHRGameMode>())
	{
		GameMode->WriteToSave(*Save);
	}

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
	if (bSaved)
	{
		LoadedSave = Save;
		UHRStatics::Notify(Player, NSLOCTEXT("HR", "Saved", "Progress saved."), FLinearColor(0.6f, 0.6f, 0.6f));
	}
	return bSaved;
}

UHRSaveGame* UHRSaveSubsystem::LoadWorldState()
{
	const bool bUnifiedExists = HasSave();
	bUnsupportedSaveVersion = false;
	LoadedSave = HRCampaignRules::ShouldImportLegacy(bUnifiedExists) ? ImportLegacySaves() : Cast<UHRSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	bUnsupportedSaveVersion = bUnsupportedSaveVersion || (bUnifiedExists && (!LoadedSave || LoadedSave->SaveVersion > UHRSaveGame::CurrentVersion));
	if (bUnsupportedSaveVersion || (LoadedSave && LoadedSave->bFreshStart))
	{
		if (!bUnsupportedSaveVersion) ResetWorldState();
		LoadedSave = nullptr;
		bPlayerStatePending = false;
		PlacedShelters.Reset();
		return nullptr;
	}
	if (!LoadedSave)
	{
		ResetWorldState();
		bPlayerStatePending = false;
		return nullptr;
	}
	PlacedShelters = LoadedSave->Shelters;

	if (UHRNarrativeSubsystem* Narrative = GetGameInstance()->GetSubsystem<UHRNarrativeSubsystem>())
	{
		Narrative->RestoreState(LoadedSave->Flags, LoadedSave->Knowledge, LoadedSave->Clues, LoadedSave->Deductions, LoadedSave->Ending);
	}
	if (UHRFactionSubsystem* Factions = GetGameInstance()->GetSubsystem<UHRFactionSubsystem>())
	{
		Factions->RestoreReputation(LoadedSave->Reputation);
	}
	if (UHRQuestSubsystem* Quests = GetGameInstance()->GetSubsystem<UHRQuestSubsystem>())
	{
		Quests->RestoreState(LoadedSave->Quests);
	}
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		if (UHRTimeWeatherSubsystem* TimeWeather = World->GetSubsystem<UHRTimeWeatherSubsystem>())
		{
			TimeWeather->SetWeather(LoadedSave->Weather);
			TimeWeather->RestoreClock(LoadedSave->WorldDay, LoadedSave->TimeOfDay, LoadedSave->HoursUntilWeatherChange);
		}
	}
	bPlayerStatePending = true;
	return LoadedSave;
}

void UHRSaveSubsystem::ApplyPendingPlayerState(AHRPlayerCharacter* Player)
{
	if (!bPlayerStatePending || !LoadedSave || !Player)
	{
		return;
	}
	bPlayerStatePending = false;

	Player->GetProgression()->RestoreState(LoadedSave->Level, LoadedSave->Echoes, LoadedSave->SkillPoints, LoadedSave->Attributes, LoadedSave->Skills, LoadedSave->Traversal);
	Player->GetProgression()->RestoreStartingClass(LoadedSave->StartingClass);
	if (!LoadedSave->bHasVitals) Player->GetAttributes()->RefillAll();
	if (!LoadedSave->AbilitySlots.IsEmpty()) Player->GetAbilities()->RestoreSlots(LoadedSave->AbilitySlots);
	Player->GetInventory()->RestoreState(LoadedSave->Items, LoadedSave->Equipped, LoadedSave->QuickItem, LoadedSave->MaxFlasks);
	Player->GetAttributes()->SetInjuries(LoadedSave->Injuries);
	Player->GetSurvival()->SetValues(LoadedSave->Food, LoadedSave->Water, LoadedSave->Warmth, LoadedSave->Fatigue);
	Player->GetSurvival()->SetInfection(LoadedSave->Infection);
	if (LoadedSave->bHasVitals)
	{
		UHRAttributeComponent* Stats = Player->GetAttributes();
		Stats->RefillAll();
		Stats->SetHealthFraction(LoadedSave->HealthFraction);
		Stats->ConsumeStamina(Stats->GetMaxStamina() * (1.f - FMath::Clamp(LoadedSave->StaminaFraction, 0.f, 1.f)));
		Stats->ConsumeFocus(Stats->GetMaxFocus() * (1.f - FMath::Clamp(LoadedSave->FocusFraction, 0.f, 1.f)));
	}
	Player->GetTraversal()->RefreshUnlocks();
}

void UHRSaveSubsystem::RememberShelter(FName Id, const FTransform& Transform)
{
	if (PlacedShelters.ContainsByPredicate([Id](const FHRPlacedShelter& Shelter) { return Shelter.CheckpointId == Id; })) return;
	FHRPlacedShelter Shelter;
	Shelter.CheckpointId = Id;
	Shelter.Transform = Transform;
	PlacedShelters.Add(Shelter);
}

void UHRSaveSubsystem::RestoreShelters(UWorld* World)
{
	if (!World) return;
	for (const FHRPlacedShelter& Data : PlacedShelters)
	{
		AHRBuiltShelter* Shelter = World->SpawnActorDeferred<AHRBuiltShelter>(AHRBuiltShelter::StaticClass(), Data.Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Shelter) continue;
		Shelter->PersistentId = Data.CheckpointId;
		Shelter->FinishSpawning(Data.Transform);
	}
}
