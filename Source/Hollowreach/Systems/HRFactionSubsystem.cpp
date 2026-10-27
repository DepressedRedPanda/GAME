#include "Systems/HRFactionSubsystem.h"

#include "Characters/HRCharacterBase.h"
#include "Core/HRStatics.h"
#include "Systems/HRContentSubsystem.h"
#include "Vehicles/HRHoverVehicle.h"

#define LOCTEXT_NAMESPACE "HRFactions"

const FName UHRFactionSubsystem::PlayerFaction(TEXT("Player"));

namespace
{
	constexpr float HostileThreshold = -25.f;
	constexpr float FriendlyThreshold = 25.f;

	EHRAttitude AttitudeFromValue(float Value)
	{
		if (Value <= HostileThreshold)
		{
			return EHRAttitude::Hostile;
		}
		return Value >= FriendlyThreshold ? EHRAttitude::Friendly : EHRAttitude::Neutral;
	}
}

void UHRFactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	const UHRContentSubsystem* Content = Collection.InitializeDependency<UHRContentSubsystem>();
	Super::Initialize(Collection);

	if (Content)
	{
		for (const TPair<FName, FHRFactionDefinition>& Pair : Content->Factions)
		{
			Reputation.Add(Pair.Key, Pair.Value.StartingReputation);
		}
	}
}

float UHRFactionSubsystem::GetReputation(FName FactionId) const
{
	const float* Value = Reputation.Find(FactionId);
	return Value ? *Value : 0.f;
}

void UHRFactionSubsystem::SetReputation(FName FactionId, float Value)
{
	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRFactionDefinition* Definition = Content ? Content->FindFaction(FactionId) : nullptr;
	const float Max = Definition ? Definition->MaxReputation : 100.f;
	Reputation.Add(FactionId, FMath::Clamp(Value, -100.f, Max));
	OnReputationChanged.Broadcast(FactionId, GetReputation(FactionId));
}

void UHRFactionSubsystem::ModifyReputation(FName FactionId, float Delta, bool bRipple, bool bNotify)
{
	if (FactionId.IsNone() || FactionId == PlayerFaction || FMath::IsNearlyZero(Delta))
	{
		return;
	}
	const EHRAttitude Before = AttitudeFromValue(GetReputation(FactionId));
	SetReputation(FactionId, GetReputation(FactionId) + Delta);
	const EHRAttitude After = AttitudeFromValue(GetReputation(FactionId));

	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	const FHRFactionDefinition* Definition = Content ? Content->FindFaction(FactionId) : nullptr;

	if (bNotify && Definition)
	{
		UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("RepChanged", "{0}: {1}{2} ({3})"),
			Definition->DisplayName, FText::FromString(Delta > 0.f ? TEXT("+") : TEXT("")), FText::AsNumber(FMath::RoundToInt(Delta)), GetRankText(GetReputation(FactionId))),
			Delta > 0.f ? FLinearColor(0.6f, 1.f, 0.6f) : FLinearColor(1.f, 0.6f, 0.5f));
		if (Before != After)
		{
			UHRStatics::Notify(GetGameInstance(), FText::Format(LOCTEXT("StanceChanged", "The {0} now regard you as {1}."), Definition->DisplayName, UEnum::GetDisplayValueAsText(After)),
				FLinearColor(1.f, 0.9f, 0.5f), true);
		}
	}

	// Politics: allies share a portion of the sentiment, enemies resent it.
	if (bRipple && Definition)
	{
		for (const TPair<FName, float>& Relation : Definition->Relations)
		{
			if (Relation.Key == PlayerFaction)
			{
				continue;
			}
			if (Relation.Value >= 50.f)
			{
				ModifyReputation(Relation.Key, Delta * 0.4f, false, false);
			}
			else if (Relation.Value <= -50.f)
			{
				ModifyReputation(Relation.Key, -Delta * 0.25f, false, false);
			}
		}
	}
}

EHRAttitude UHRFactionSubsystem::GetAttitudeBetween(FName A, FName B) const
{
	if (A == B && !A.IsNone())
	{
		return EHRAttitude::Friendly;
	}
	if (A.IsNone() || B.IsNone() || A == TEXT("Neutral") || B == TEXT("Neutral"))
	{
		return EHRAttitude::Neutral;
	}
	if (A == PlayerFaction)
	{
		return AttitudeFromValue(GetReputation(B));
	}
	if (B == PlayerFaction)
	{
		return AttitudeFromValue(GetReputation(A));
	}

	const UHRContentSubsystem* Content = GetGameInstance()->GetSubsystem<UHRContentSubsystem>();
	if (const FHRFactionDefinition* Definition = Content ? Content->FindFaction(A) : nullptr)
	{
		if (const float* Relation = Definition->Relations.Find(B))
		{
			return AttitudeFromValue(*Relation);
		}
	}
	if (const FHRFactionDefinition* Definition = Content ? Content->FindFaction(B) : nullptr)
	{
		if (const float* Relation = Definition->Relations.Find(A))
		{
			return AttitudeFromValue(*Relation);
		}
	}
	return EHRAttitude::Neutral;
}

FText UHRFactionSubsystem::GetRankText(float InReputation)
{
	const float Value = InReputation;
	if (Value <= -75.f) { return LOCTEXT("Hated", "Hated"); }
	if (Value <= -25.f) { return LOCTEXT("Hostile", "Hostile"); }
	if (Value < -5.f) { return LOCTEXT("Wary", "Wary"); }
	if (Value < 25.f) { return LOCTEXT("Neutral", "Neutral"); }
	if (Value < 50.f) { return LOCTEXT("Friendly", "Friendly"); }
	if (Value < 80.f) { return LOCTEXT("Honored", "Honored"); }
	return LOCTEXT("Exalted", "Exalted");
}

FName UHRFactionSubsystem::GetFactionOf(const AActor* Actor)
{
	if (const AHRCharacterBase* Character = Cast<AHRCharacterBase>(Actor))
	{
		return Character->FactionId;
	}
	if (Cast<AHRHoverVehicle>(Actor))
	{
		return PlayerFaction;
	}
	return NAME_None;
}

EHRAttitude UHRFactionSubsystem::GetAttitude(const AActor* A, const AActor* B)
{
	if (!A || !B)
	{
		return EHRAttitude::Neutral;
	}
	if (A == B)
	{
		return EHRAttitude::Friendly;
	}
	const UHRFactionSubsystem* Factions = HRGetSubsystem<UHRFactionSubsystem>(A);
	return Factions ? Factions->GetAttitudeBetween(GetFactionOf(A), GetFactionOf(B)) : EHRAttitude::Neutral;
}

void UHRFactionSubsystem::NotifyAttack(const AActor* Attacker, const AActor* Victim)
{
	if (GetFactionOf(Attacker) != PlayerFaction)
	{
		return;
	}
	const FName VictimFaction = GetFactionOf(Victim);
	UHRFactionSubsystem* Factions = HRGetSubsystem<UHRFactionSubsystem>(Attacker);
	if (!Factions || VictimFaction.IsNone() || VictimFaction == PlayerFaction)
	{
		return;
	}
	if (Factions->GetAttitudeBetween(PlayerFaction, VictimFaction) != EHRAttitude::Hostile)
	{
		Factions->ModifyReputation(VictimFaction, -15.f, true, true);
	}
}

void UHRFactionSubsystem::RestoreReputation(const TMap<FName, float>& InReputation)
{
	for (const TPair<FName, float>& Pair : InReputation)
	{
		Reputation.Add(Pair.Key, Pair.Value);
	}
}

#undef LOCTEXT_NAMESPACE
