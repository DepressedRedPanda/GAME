#include "Game/HRHUD.h"

#include "Abilities/HRAbility.h"
#include "AI/HRAIController.h"
#include "Characters/HRBossCharacter.h"
#include "Characters/HRCompanionCharacter.h"
#include "Characters/HRPlayerCharacter.h"
#include "Components/HRAbilityComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/HRInteractionComponent.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Components/HRStealthComponent.h"
#include "Components/HRSurvivalComponent.h"
#include "Components/HRTraversalComponent.h"
#include "Core/HRStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "Game/HRPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Narrative/HRQuestSubsystem.h"
#include "Systems/HRContentSubsystem.h"
#include "Systems/HRFactionSubsystem.h"
#include "Systems/HRTimeWeatherSubsystem.h"
#include "World/HRDiscoverables.h"

namespace
{
	const FLinearColor HealthColor(0.75f, 0.12f, 0.1f);
	const FLinearColor StaminaColor(0.2f, 0.7f, 0.3f);
	const FLinearColor FocusColor(0.25f, 0.45f, 1.f);
	const FLinearColor Dim(0.65f, 0.65f, 0.65f);
}

// --- Primitives --------------------------------------------------------------------------------

void AHRHUD::Bar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color, const FLinearColor& Background)
{
	DrawRect(Background, X - 2.f, Y - 2.f, Width + 4.f, Height + 4.f);
	DrawRect(Color, X, Y, Width * FMath::Clamp(Fraction, 0.f, 1.f), Height);
}

void AHRHUD::Text(const FString& String, float X, float Y, const FLinearColor& Color, float Scale, UFont* Font)
{
	DrawText(String, Color, X, Y, Font ? Font : GEngine->GetSmallFont(), Scale);
}

float AHRHUD::TextWidth(const FString& String, float Scale, UFont* Font)
{
	float Width = 0.f;
	float Height = 0.f;
	GetTextSize(String, Width, Height, Font ? Font : GEngine->GetSmallFont(), Scale);
	return Width;
}

void AHRHUD::DrawCenterMessage(const FString& String, const FLinearColor& Color, float Scale, float YFraction)
{
	UFont* Font = GEngine->GetLargeFont();
	const float Width = TextWidth(String, Scale, Font);
	Text(String, (Canvas->ClipX - Width) * 0.5f, Canvas->ClipY * YFraction, Color, Scale, Font);
}

// --- Main --------------------------------------------------------------------------------------

void AHRHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}

	const AHRPlayerController* PC = Cast<AHRPlayerController>(GetOwningPlayerController());
	const AHRPlayerCharacter* Player = PC ? PC->GetPlayerCharacter() : nullptr;
	if (!PC || !Player)
	{
		return;
	}

	if (PC->GetUIMode() == EHRUIMode::Dead)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
		DrawCenterMessage(TEXT("YOU DIED"), FLinearColor(0.7f, 0.05f, 0.05f), 3.f, 0.4f);
		DrawCenterMessage(TEXT("Your echoes remain where you fell."), Dim, 1.f, 0.52f);
		DrawNotifications(PC);
		return;
	}

	DrawWorldMarkers(Player);
	DrawVitals(Player);
	DrawPosture(Player);
	DrawSurvival(Player);
	DrawAbilities(Player);
	DrawWorldInfo(Player);
	DrawBossBar(Player);
	DrawCompanion(Player);

	if (PC->GetUIMode() == EHRUIMode::Gameplay)
	{
		DrawPrompts(Player);
		// Crosshair dot.
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.6f), Canvas->ClipX * 0.5f - 2.f, Canvas->ClipY * 0.5f - 2.f, 4.f, 4.f);
	}
	else
	{
		DrawMenu(PC);
	}
	DrawNotifications(PC);
}

void AHRHUD::DrawVitals(const AHRPlayerCharacter* Player)
{
	const UHRAttributeComponent* Attributes = Player->GetAttributes();
	const float X = 40.f;
	float Y = 36.f;

	// Bars grow with their maximum, like the Souls games.
	const float HealthWidth = FMath::Clamp(Attributes->GetMaxHealth() * 2.4f, 180.f, 620.f);
	const float StaminaWidth = FMath::Clamp(Attributes->GetMaxStamina() * 2.4f, 140.f, 520.f);
	const float FocusWidth = FMath::Clamp(Attributes->GetMaxFocus() * 3.f, 100.f, 420.f);

	Bar(X, Y, HealthWidth, 14.f, Attributes->GetHealthPercent(), HealthColor);
	Text(FString::Printf(TEXT("%.0f / %.0f"), Attributes->GetHealth(), Attributes->GetMaxHealth()), X + HealthWidth + 10.f, Y - 2.f, FLinearColor::White);
	Y += 22.f;
	Bar(X, Y, StaminaWidth, 9.f, Attributes->GetStamina() / FMath::Max(1.f, Attributes->GetMaxStamina()), StaminaColor);
	Y += 16.f;
	Bar(X, Y, FocusWidth, 9.f, Attributes->GetFocus() / FMath::Max(1.f, Attributes->GetMaxFocus()), FocusColor);
	Y += 20.f;

	const UHRProgressionComponent* Progression = Player->GetProgression();
	const UHRInventoryComponent* Inventory = Player->GetInventory();
	FString Line = FString::Printf(TEXT("Lv %d   Echoes %d   Gourd %d/%d   Resurrection %d/%d"),
		Progression->GetLevel(), Progression->GetEchoes(), Inventory->CountItem(Inventory->FlaskItemId), Inventory->MaxFlaskCharges,
		Player->GetResurrectionCharges(), Player->GetMaxResurrectionCharges());
	Text(Line, X, Y, FLinearColor::White);
	Y += 18.f;

	const int32 Flow = Player->GetCombat()->GetFlowStacks();
	if (Flow > 0)
	{
		Text(FString::Printf(TEXT("FLOW x%d"), Flow), X, Y, FLinearColor(0.4f, 0.9f, 1.f), 1.3f);
		Y += 20.f;
	}
	if (Player->GetAbilities()->IsTransformed())
	{
		Text(TEXT("BEAST FORM"), X, Y, FLinearColor(1.f, 0.3f, 0.2f), 1.2f);
		Y += 20.f;
	}
	if (Inventory->GetEncumbrance() > 1.f)
	{
		Text(TEXT("Overburdened"), X, Y, FLinearColor(1.f, 0.7f, 0.2f));
		Y += 16.f;
	}
	for (const EHRInjury Injury : Attributes->GetInjuries())
	{
		Text(UHRAttributeComponent::GetInjuryName(Injury).ToString(), X, Y, FLinearColor(1.f, 0.45f, 0.35f));
		Y += 16.f;
	}
}

void AHRHUD::DrawPosture(const AHRPlayerCharacter* Player)
{
	const UHRAttributeComponent* Attributes = Player->GetAttributes();
	const float Fraction = Attributes->GetPosture() / FMath::Max(1.f, Attributes->GetMaxPosture());
	if (Fraction <= 0.01f)
	{
		return;
	}
	const float Width = 320.f;
	const FLinearColor Color = FLinearColor::LerpUsingHSV(FLinearColor(1.f, 0.85f, 0.2f), FLinearColor(1.f, 0.15f, 0.05f), Fraction);
	// Sekiro-style: fills from the centre outward.
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY - 110.f;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), X - 2.f, Y - 2.f, Width + 4.f, 10.f);
	const float Filled = Width * Fraction;
	DrawRect(Color, X + (Width - Filled) * 0.5f, Y, Filled, 6.f);
}

void AHRHUD::DrawSurvival(const AHRPlayerCharacter* Player)
{
	const UHRSurvivalComponent* Survival = Player->GetSurvival();
	if (!Survival->bEnabled)
	{
		return;
	}
	const float X = 40.f;
	float Y = Canvas->ClipY - 120.f;
	struct FMeter
	{
		const TCHAR* Label;
		float Value;
		FLinearColor Color;
	};
	const FMeter Meters[] = {
		{ TEXT("Food"), Survival->GetFood() / 100.f, FLinearColor(0.8f, 0.55f, 0.2f) },
		{ TEXT("Water"), Survival->GetWater() / 100.f, FLinearColor(0.3f, 0.6f, 1.f) },
		{ TEXT("Warmth"), Survival->GetWarmth() / 100.f, FLinearColor(1.f, 0.4f, 0.2f) },
		{ TEXT("Rest"), 1.f - Survival->GetFatigue() / 100.f, FLinearColor(0.7f, 0.6f, 0.9f) },
	};
	for (const FMeter& Meter : Meters)
	{
		Text(Meter.Label, X, Y - 4.f, Meter.Value < 0.25f ? FLinearColor(1.f, 0.3f, 0.3f) : Dim);
		Bar(X + 60.f, Y, 120.f, 6.f, Meter.Value, Meter.Color);
		Y += 16.f;
	}
	Text(FString::Printf(TEXT("%.0f C%s"), Survival->GetAmbientTemperature(), Survival->IsNearHeat() ? TEXT("  (fire)") : TEXT("")), X, Y, Dim);
	Text(FString::Printf(TEXT("Ash infection: %.0f%%"), Survival->GetInfection()), X + 200.f, Y, Survival->GetInfection() >= 50.f ? FLinearColor(1.f, 0.3f, 0.3f) : Dim);
}

void AHRHUD::DrawAbilities(const AHRPlayerCharacter* Player)
{
	const UHRAbilityComponent* Abilities = Player->GetAbilities();
	const float X = 240.f;
	const float Y = Canvas->ClipY - 120.f;
	for (int32 Slot = 0; Slot < UHRAbilityComponent::NumSlots; ++Slot)
	{
		const UHRAbility* Ability = Abilities->GetSlotAbility(Slot);
		FString Label = FString::Printf(TEXT("[%d] "), Slot + 1);
		FLinearColor Color = Dim;
		if (Ability)
		{
			const float Cooldown = Abilities->GetCooldownRemaining(Ability->AbilityId);
			Label += Ability->DisplayName.ToString();
			if (Ability->IsActive())
			{
				Label += TEXT(" (active)");
				Color = FLinearColor(0.5f, 1.f, 1.f);
			}
			else if (Cooldown > 0.f)
			{
				Label += FString::Printf(TEXT(" %.1fs"), Cooldown);
			}
			else
			{
				Color = FLinearColor::White;
			}
		}
		else
		{
			Label += TEXT("-");
		}
		Text(Label, X, Y + Slot * 16.f, Color);
	}

	const UHRInventoryComponent* Inventory = Player->GetInventory();
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	const UHRItemDefinition* Quick = Content ? Content->FindItem(Inventory->GetQuickItemId()) : nullptr;
	Text(FString::Printf(TEXT("[Q] %s x%d   [X] cycle"), Quick ? *Quick->DisplayName.ToString() : TEXT("-"), Inventory->CountItem(Inventory->GetQuickItemId())),
		X, Y + 70.f, FLinearColor::White);
}

void AHRHUD::DrawWorldInfo(const AHRPlayerCharacter* Player)
{
	const float Right = Canvas->ClipX - 40.f;
	float Y = 36.f;

	if (const UHRTimeWeatherSubsystem* TimeWeather = GetWorld()->GetSubsystem<UHRTimeWeatherSubsystem>())
	{
		const FString Time = TimeWeather->GetTimeText().ToString();
		Text(Time, Right - TextWidth(Time), Y, TimeWeather->IsBloodMoon() ? FLinearColor(1.f, 0.3f, 0.3f) : FLinearColor::White);
		Y += 20.f;
	}

	// Stealth readout.
	const UHRStealthComponent* Stealth = Player->GetStealth();
	const float Visibility = Stealth->GetVisibility();
	const FString StealthText = FString::Printf(TEXT("Visibility %d%%%s"), FMath::RoundToInt(Visibility * 100.f), Stealth->IsInCover() ? TEXT(" (cover)") : TEXT(""));
	const FLinearColor StealthColor = FLinearColor::LerpUsingHSV(FLinearColor(0.4f, 0.8f, 1.f), FLinearColor(1.f, 0.5f, 0.2f), Visibility);
	Text(StealthText, Right - TextWidth(StealthText), Y, StealthColor);
	Y += 28.f;

	// Quest tracker: text only, no markers.
	const UHRQuestSubsystem* Quests = HRGetSubsystem<UHRQuestSubsystem>(this);
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	if (!Quests || !Content)
	{
		return;
	}
	const FName Tracked = Quests->GetTrackedQuest();
	const FHRQuestProgress* Progress = Quests->GetProgress(Tracked);
	const FHRQuestDefinition* Definition = Content->FindQuest(Tracked);
	if (!Progress || !Definition)
	{
		return;
	}
	const FString Title = Definition->Title.ToString();
	Text(Title, Right - TextWidth(Title, 1.1f), Y, FLinearColor(1.f, 0.85f, 0.5f), 1.1f);
	Y += 20.f;
	if (const FHRQuestStage* Stage = Definition->FindStage(Progress->CurrentStage))
	{
		for (int32 Index = 0; Index < Stage->Objectives.Num(); ++Index)
		{
			const FString Objective = Quests->GetObjectiveText(*Progress, Index).ToString();
			Text(Objective, Right - TextWidth(Objective), Y, Dim);
			Y += 16.f;
		}
	}
}

void AHRHUD::DrawPrompts(const AHRPlayerCharacter* Player)
{
	const float CenterX = Canvas->ClipX * 0.5f;
	float Y = Canvas->ClipY * 0.58f;

	if (Player->IsDowned())
	{
		DrawCenterMessage(FString::Printf(TEXT("[E] RESURRECT  (%d)   %.1fs"), Player->GetResurrectionCharges(), Player->GetDownedTimeRemaining()),
			FLinearColor(1.f, 0.3f, 0.25f), 1.4f, 0.45f);
		return;
	}

	if (Player->GetCombat()->FindTakedownTarget())
	{
		const FString Prompt(TEXT("[LMB] Takedown"));
		Text(Prompt, CenterX - TextWidth(Prompt, 1.2f) * 0.5f, Y, FLinearColor(1.f, 0.25f, 0.25f), 1.2f);
		Y += 24.f;
	}

	const FText Interaction = Player->GetInteraction()->GetPrompt();
	if (!Interaction.IsEmpty())
	{
		const FString Prompt = TEXT("[E] ") + Interaction.ToString();
		Text(Prompt, CenterX - TextWidth(Prompt, 1.2f) * 0.5f, Y, FLinearColor::White, 1.2f);
		Y += 24.f;
	}

	if (Player->GetCharacterMovement()->IsFalling() && Player->GetProgression()->HasTraversal(EHRTraversalAbility::Grapple) && Player->GetController())
	{
		if (Player->GetTraversal()->FindGrappleTarget(Player->GetController()->GetControlRotation().Vector()))
		{
			const FString Prompt = TEXT("[Space] Grapple");
			Text(Prompt, CenterX - TextWidth(Prompt) * 0.5f, Y, FLinearColor(0.3f, 0.9f, 1.f));
		}
	}
}

void AHRHUD::DrawWorldMarkers(const AHRPlayerCharacter* Player)
{
	const bool bSpiritSight = Player->IsSpiritSightActive();
	const float MaxDistance = bSpiritSight ? 4500.f : 2500.f;
	const AActor* LockTarget = Player->GetLockTarget();

	for (TActorIterator<AHRCharacterBase> It(GetWorld()); It; ++It)
	{
		const AHRCharacterBase* Other = *It;
		if (Other == Player || Other->IsDead() || Other->IsHidden())
		{
			continue;
		}
		const float Distance = FVector::Dist(Other->GetActorLocation(), Player->GetActorLocation());
		if (Distance > MaxDistance)
		{
			continue;
		}
		const FVector Screen = Project(Other->GetActorLocation() + FVector(0.f, 0.f, 130.f));
		if (Screen.Z <= 0.f)
		{
			continue;
		}

		const bool bHostile = UHRFactionSubsystem::AreHostile(Player, Other);
		const UHRCombatComponent* Combat = Other->GetCombat();
		const UHRAttributeComponent* Attributes = Other->GetAttributes();

		// Awareness indicator (stealth games).
		const EHRAwareness Awareness = Other->GetAwarenessTowards(Player);
		if (bHostile && Awareness != EHRAwareness::Unaware)
		{
			const TCHAR* Symbol = Awareness == EHRAwareness::Alerted ? TEXT("!") : (Awareness == EHRAwareness::Searching ? TEXT("?!") : TEXT("?"));
			const FLinearColor Color = Awareness == EHRAwareness::Alerted ? FLinearColor(1.f, 0.15f, 0.1f) : FLinearColor(1.f, 0.8f, 0.2f);
			Text(Symbol, Screen.X - 6.f, Screen.Y - 34.f, Color, 1.6f, GEngine->GetLargeFont());
		}

		// Perilous attack warning.
		if (Combat && Combat->IsCurrentAttackPerilous())
		{
			Text(TEXT("PERILOUS"), Screen.X - 40.f, Screen.Y - 60.f, FLinearColor(1.f, 0.f, 0.f), 1.3f, GEngine->GetMediumFont());
		}
		if (Combat && Combat->GetState() == EHRCombatState::PostureBroken)
		{
			Text(TEXT("DEATHBLOW"), Screen.X - 45.f, Screen.Y - 60.f, FLinearColor(1.f, 0.85f, 0.2f), 1.2f, GEngine->GetMediumFont());
		}

		// Health / posture bars for damaged or locked enemies.
		const bool bLocked = Other == LockTarget;
		if (Attributes && (bLocked || (bHostile && Attributes->GetHealthPercent() < 0.999f)) && !Cast<AHRBossCharacter>(Other))
		{
			Bar(Screen.X - 40.f, Screen.Y, 80.f, 5.f, Attributes->GetHealthPercent(), HealthColor);
			Bar(Screen.X - 40.f, Screen.Y + 9.f, 80.f, 3.f, Attributes->GetPosture() / FMath::Max(1.f, Attributes->GetMaxPosture()), FLinearColor(1.f, 0.75f, 0.2f));
		}
		if (bLocked)
		{
			Text(TEXT("<  >"), Screen.X - 16.f, Screen.Y + 60.f, FLinearColor::White, 1.2f);
		}
		if (bSpiritSight)
		{
			const FString Name = Other->GetDisplayName().ToString();
			Text(Name, Screen.X - TextWidth(Name) * 0.5f, Screen.Y + 16.f, bHostile ? FLinearColor(1.f, 0.4f, 0.4f) : FLinearColor(0.5f, 1.f, 0.6f));
		}
	}

	// Spirit sight also shows secrets.
	if (bSpiritSight)
	{
		for (TActorIterator<AHRCollectible> It(GetWorld()); It; ++It)
		{
			const FVector Screen = Project(It->GetActorLocation());
			if (Screen.Z > 0.f && FVector::Dist(It->GetActorLocation(), Player->GetActorLocation()) < MaxDistance)
			{
				Text(TEXT("*"), Screen.X, Screen.Y, FLinearColor(1.f, 0.9f, 0.4f), 1.5f, GEngine->GetLargeFont());
			}
		}
		for (TActorIterator<AHRClue> It(GetWorld()); It; ++It)
		{
			const FVector Screen = Project(It->GetActorLocation());
			if (!It->IsHidden() && Screen.Z > 0.f && FVector::Dist(It->GetActorLocation(), Player->GetActorLocation()) < MaxDistance)
			{
				Text(TEXT("o"), Screen.X, Screen.Y, FLinearColor(0.3f, 1.f, 1.f), 1.5f, GEngine->GetLargeFont());
			}
		}
	}
}

void AHRHUD::DrawBossBar(const AHRPlayerCharacter* Player)
{
	for (TActorIterator<AHRBossCharacter> It(GetWorld()); It; ++It)
	{
		const AHRBossCharacter* Boss = *It;
		if (!Boss->IsFightActive() || Boss->IsDead())
		{
			continue;
		}
		const UHRAttributeComponent* Attributes = Boss->GetAttributes();
		const float Width = Canvas->ClipX * 0.5f;
		const float X = (Canvas->ClipX - Width) * 0.5f;
		const float Y = Canvas->ClipY - 70.f;
		Text(Boss->GetDisplayName().ToString(), X, Y - 22.f, FLinearColor(0.95f, 0.9f, 0.8f), 1.2f);
		Bar(X, Y, Width, 10.f, Attributes->GetHealthPercent(), FLinearColor(0.6f, 0.08f, 0.1f));
		Bar(X, Y + 14.f, Width, 4.f, Attributes->GetPosture() / FMath::Max(1.f, Attributes->GetMaxPosture()), FLinearColor(1.f, 0.7f, 0.2f));
		// Deathblow pips: one per phase.
		for (int32 Phase = 0; Phase < Boss->GetPhaseCount(); ++Phase)
		{
			const bool bSpent = Phase < Boss->GetPhaseIndex();
			DrawRect(bSpent ? FLinearColor(0.2f, 0.2f, 0.2f) : FLinearColor(0.9f, 0.1f, 0.1f), X + Width + 12.f + Phase * 16.f, Y - 2.f, 10.f, 10.f);
		}
		break;
	}
}

void AHRHUD::DrawCompanion(const AHRPlayerCharacter* Player)
{
	const AHRCompanionCharacter* Companion = Player->GetCompanion();
	if (!Companion)
	{
		return;
	}
	const float X = 40.f;
	const float Y = Canvas->ClipY - 170.f;
	const FString Label = FString::Printf(TEXT("%s - %s  (approval %d)%s"), *Companion->GetDisplayName().ToString(),
		*UEnum::GetDisplayValueAsText(Companion->GetCommand()).ToString(), Companion->GetApproval(), Companion->IsKnockedOut() ? TEXT("  DOWN") : TEXT(""));
	Text(Label, X, Y, FLinearColor(0.5f, 1.f, 0.6f));
	Bar(X, Y + 18.f, 160.f, 5.f, Companion->GetAttributes()->GetHealthPercent(), StaminaColor);
}

void AHRHUD::DrawNotifications(const AHRPlayerController* PC)
{
	const double Now = GetWorld()->GetRealTimeSeconds();
	float Y = Canvas->ClipY * 0.35f;
	float LargeY = Canvas->ClipY * 0.18f;
	for (const FHRNotification& Notification : PC->GetNotifications())
	{
		const float Age = static_cast<float>(Now - Notification.Time);
		if (Age > Notification.Duration)
		{
			continue;
		}
		const float Alpha = FMath::Clamp((Notification.Duration - Age) / 0.75f, 0.f, 1.f);
		FLinearColor Color = Notification.Color;
		Color.A = Alpha;
		if (Notification.bLarge)
		{
			UFont* Font = GEngine->GetLargeFont();
			const FString String = Notification.Text.ToString();
			Text(String, (Canvas->ClipX - TextWidth(String, 1.2f, Font)) * 0.5f, LargeY, Color, 1.2f, Font);
			LargeY += 34.f;
		}
		else
		{
			Text(Notification.Text.ToString(), 40.f, Y, Color);
			Y += 18.f;
		}
	}
}

void AHRHUD::DrawMenu(const AHRPlayerController* PC)
{
	FText Title;
	TArray<FText> Tabs;
	int32 ActiveTab = 0;
	TArray<FHRMenuEntry> Entries;
	FText Footer;
	PC->BuildMenu(Title, Tabs, ActiveTab, Entries, Footer);

	const float Width = FMath::Min(1000.f, Canvas->ClipX - 80.f);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	float Y = Canvas->ClipY * 0.16f;
	DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.82f), X - 20.f, Y - 20.f, Width + 40.f, Canvas->ClipY * 0.7f);

	Text(Title.ToString(), X, Y, FLinearColor(1.f, 0.85f, 0.55f), 1.f, GEngine->GetLargeFont());
	Y += 40.f;

	if (Tabs.Num() > 0)
	{
		float TabX = X;
		for (int32 Index = 0; Index < Tabs.Num(); ++Index)
		{
			const FString Tab = Index == ActiveTab ? FString::Printf(TEXT("[%s]"), *Tabs[Index].ToString()) : Tabs[Index].ToString();
			Text(Tab, TabX, Y, Index == ActiveTab ? FLinearColor::White : Dim, 1.1f);
			TabX += TextWidth(Tab, 1.1f) + 24.f;
		}
		Y += 30.f;
	}

	// Dialogue: the spoken line comes before the choices.
	if (PC->GetUIMode() == EHRUIMode::Dialogue && !Footer.IsEmpty())
	{
		TArray<FString> Lines;
		Footer.ToString().ParseIntoArrayLines(Lines);
		for (const FString& Line : Lines)
		{
			Text(Line, X, Y, FLinearColor(0.95f, 0.92f, 0.85f), 1.15f);
			Y += 22.f;
		}
		if (const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this))
		{
			if (!Narrative->GetLastCheckResult().IsEmpty())
			{
				Text(Narrative->GetLastCheckResult().ToString(), X, Y, Dim);
				Y += 18.f;
			}
		}
		Y += 14.f;
	}

	const int32 Page = PC->GetMenuPage();
	const int32 PageCount = FMath::Max(1, FMath::DivideAndRoundUp(Entries.Num(), AHRPlayerController::EntriesPerPage));
	const int32 Start = FMath::Min(Page, PageCount - 1) * AHRPlayerController::EntriesPerPage;
	for (int32 Index = Start; Index < Entries.Num() && Index < Start + AHRPlayerController::EntriesPerPage; ++Index)
	{
		const FHRMenuEntry& Entry = Entries[Index];
		const bool bActionable = !Entry.Action.IsNone();
		const FString Label = bActionable ? FString::Printf(TEXT("%d. %s"), Index - Start + 1, *Entry.Label.ToString()) : Entry.Label.ToString();
		Text(Label, X, Y, Entry.bEnabled ? FLinearColor::White : FLinearColor(0.45f, 0.45f, 0.45f), 1.05f);
		Y += 19.f;
		if (!Entry.Detail.IsEmpty())
		{
			TArray<FString> Lines;
			Entry.Detail.ToString().ParseIntoArrayLines(Lines);
			for (const FString& Line : Lines)
			{
				Text(Line, X + 24.f, Y, Dim, 0.95f);
				Y += 16.f;
			}
		}
		Y += 6.f;
	}
	if (Entries.Num() == 0)
	{
		Text(TEXT("(nothing here yet)"), X, Y, Dim);
		Y += 20.f;
	}
	if (PageCount > 1)
	{
		Text(FString::Printf(TEXT("Page %d/%d  [9] next page"), Page + 1, PageCount), X, Y + 6.f, Dim);
		Y += 26.f;
	}

	if (PC->GetUIMode() != EHRUIMode::Dialogue && !Footer.IsEmpty())
	{
		TArray<FString> Lines;
		Footer.ToString().ParseIntoArrayLines(Lines, false);
		Y += 10.f;
		for (const FString& Line : Lines)
		{
			Text(Line, X, Y, FLinearColor(0.85f, 0.82f, 0.75f));
			Y += 18.f;
		}
	}
}
