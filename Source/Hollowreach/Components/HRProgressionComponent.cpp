#include "Components/HRProgressionComponent.h"

#include "Components/HRAbilityComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Core/HRStatics.h"
#include "Narrative/HRNarrativeSubsystem.h"
#include "Systems/HRContentSubsystem.h"

#define LOCTEXT_NAMESPACE "HRProgression"

namespace
{
	const FName AttributeSource(TEXT("Attributes"));
	const FName SkillSource(TEXT("Skills"));

	/** Soft cap: points beyond the cap are worth 40%. */
	float Soft(int32 Points, int32 Cap)
	{
		return Points <= Cap ? static_cast<float>(Points) : Cap + (Points - Cap) * 0.4f;
	}
}

UHRProgressionComponent::UHRProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	for (uint8 Index = 0; Index < static_cast<uint8>(EHRAttribute::MAX); ++Index)
	{
		Attributes.Add(static_cast<EHRAttribute>(Index), 10);
	}
	Traversal.Add(EHRTraversalAbility::Mantle);
}

void UHRProgressionComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyAllEffects();
}

void UHRProgressionComponent::AddEchoes(int32 Amount, bool bNotify)
{
	if (Amount == 0)
	{
		return;
	}
	Echoes = FMath::Max(0, Echoes + Amount);
	OnEchoesChanged.Broadcast(Echoes);
	if (bNotify && Amount > 0)
	{
		UHRStatics::Notify(this, FText::Format(LOCTEXT("EchoesGained", "+{0} Echoes"), Amount), FLinearColor(0.5f, 1.f, 0.8f));
	}
}

bool UHRProgressionComponent::SpendEchoes(int32 Amount)
{
	if (Amount < 0 || Echoes < Amount)
	{
		return false;
	}
	Echoes -= Amount;
	OnEchoesChanged.Broadcast(Echoes);
	return true;
}

void UHRProgressionComponent::SetEchoes(int32 Amount)
{
	Echoes = FMath::Max(0, Amount);
	OnEchoesChanged.Broadcast(Echoes);
}

int32 UHRProgressionComponent::GetLevelUpCost() const
{
	return FMath::RoundToInt(90.f * FMath::Pow(1.1f, static_cast<float>(Level - 1)) + 25.f * Level);
}

bool UHRProgressionComponent::LevelUpAttribute(EHRAttribute Attribute)
{
	if (!SpendEchoes(GetLevelUpCost()))
	{
		return false;
	}
	++Level;
	Attributes.FindOrAdd(Attribute, 10) += 1;
	GrantSkillPoints(1);
	ApplyAttributeModifiers();
	OnLevelUp.Broadcast(Level);
	UHRStatics::Notify(this, FText::Format(LOCTEXT("LevelUp", "Level {0}: {1} increased"), Level, UEnum::GetDisplayValueAsText(Attribute)), FLinearColor(1.f, 0.85f, 0.4f));
	return true;
}

int32 UHRProgressionComponent::GetAttribute(EHRAttribute Attribute) const
{
	const int32* Value = Attributes.Find(Attribute);
	return Value ? *Value : 10;
}

float UHRProgressionComponent::GetAttributeScaling(EHRAttribute Attribute) const
{
	return Soft(GetAttribute(Attribute) - 10, 30) * 0.025f;
}

void UHRProgressionComponent::GrantSkillPoints(int32 Amount)
{
	SkillPoints += Amount;
}

bool UHRProgressionComponent::CanUnlockSkill(FName SkillId, FText& OutReason) const
{
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	const FHRSkillNode* Node = Content ? Content->FindSkill(SkillId) : nullptr;
	if (!Node)
	{
		OutReason = LOCTEXT("UnknownSkill", "Unknown skill");
		return false;
	}
	if (HasSkill(SkillId))
	{
		OutReason = LOCTEXT("Owned", "Learned");
		return false;
	}
	for (const FName& Prerequisite : Node->Prerequisites)
	{
		if (!HasSkill(Prerequisite))
		{
			const FHRSkillNode* Required = Content->FindSkill(Prerequisite);
			OutReason = FText::Format(LOCTEXT("NeedsPrereq", "Requires {0}"), Required ? Required->DisplayName : FText::FromName(Prerequisite));
			return false;
		}
	}
	if (Level < Node->RequiredLevel)
	{
		OutReason = FText::Format(LOCTEXT("NeedsLevel", "Requires level {0}"), Node->RequiredLevel);
		return false;
	}
	if (!Node->RequiredKnowledge.IsNone())
	{
		const UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this);
		if (!Narrative || !Narrative->HasKnowledge(Node->RequiredKnowledge))
		{
			OutReason = LOCTEXT("NeedsKnowledge", "Requires forgotten knowledge");
			return false;
		}
	}
	if (SkillPoints < Node->Cost)
	{
		OutReason = FText::Format(LOCTEXT("NeedsPoints", "Needs {0} skill point(s)"), Node->Cost);
		return false;
	}
	OutReason = LOCTEXT("Available", "Available");
	return true;
}

bool UHRProgressionComponent::UnlockSkill(FName SkillId)
{
	FText Reason;
	if (!CanUnlockSkill(SkillId, Reason))
	{
		UHRStatics::Notify(this, Reason, FLinearColor::Gray);
		return false;
	}
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	const FHRSkillNode* Node = Content->FindSkill(SkillId);

	SkillPoints -= Node->Cost;
	UnlockedSkills.Add(SkillId);
	ApplySkillModifiers();
	ApplyClassModifiers();

	if (!Node->GrantAbility.IsNone())
	{
		if (UHRAbilityComponent* Abilities = GetOwner()->FindComponentByClass<UHRAbilityComponent>())
		{
			Abilities->GrantAbilityById(Node->GrantAbility);
		}
	}
	if (Node->bGrantsTraversal)
	{
		GrantTraversal(Node->Traversal, false);
	}

	OnSkillUnlocked.Broadcast(SkillId);
	UHRStatics::Notify(this, FText::Format(LOCTEXT("SkillLearned", "Skill learned: {0}"), Node->DisplayName), FLinearColor(0.6f, 0.8f, 1.f));
	return true;
}

bool UHRProgressionComponent::HasTraversal(EHRTraversalAbility Ability) const
{
	return Traversal.Contains(Ability);
}

void UHRProgressionComponent::GrantTraversal(EHRTraversalAbility Ability, bool bNotify)
{
	if (Traversal.Contains(Ability))
	{
		return;
	}
	Traversal.Add(Ability);
	if (bNotify)
	{
		UHRStatics::Notify(this, FText::Format(LOCTEXT("TraversalGained", "New movement: {0}"), UEnum::GetDisplayValueAsText(Ability)), FLinearColor(0.6f, 1.f, 1.f), true);
	}
}

void UHRProgressionComponent::ApplyAllEffects()
{
	ApplyAttributeModifiers();
	ApplySkillModifiers();
	ApplyClassModifiers();

	if (UHRAbilityComponent* Abilities = GetOwner()->FindComponentByClass<UHRAbilityComponent>())
	{
		Abilities->GrantAbilityById(TEXT("Ability.VoidPulse"));
		Abilities->GrantAbilityById(TEXT("Ability.SpectralForm"));
		if (const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this))
		{
			for (const FName& SkillId : UnlockedSkills)
			{
				const FHRSkillNode* Node = Content->FindSkill(SkillId);
				if (Node && !Node->GrantAbility.IsNone() && !Abilities->HasAbility(Node->GrantAbility))
				{
					Abilities->GrantAbilityById(Node->GrantAbility);
				}
			}
		}
	}
}

void UHRProgressionComponent::RestoreState(int32 InLevel, int32 InEchoes, int32 InSkillPoints, const TMap<EHRAttribute, int32>& InAttributes,
	const TArray<FName>& InSkills, const TArray<EHRTraversalAbility>& InTraversal)
{
	Level = FMath::Max(1, InLevel);
	Echoes = FMath::Max(0, InEchoes);
	SkillPoints = FMath::Max(0, InSkillPoints);
	for (const TPair<EHRAttribute, int32>& Pair : InAttributes)
	{
		Attributes.Add(Pair.Key, Pair.Value);
	}
	UnlockedSkills = InSkills;
	Traversal = InTraversal;
	if (!Traversal.Contains(EHRTraversalAbility::Mantle))
	{
		Traversal.Add(EHRTraversalAbility::Mantle);
	}
	ApplyAllEffects();
	OnEchoesChanged.Broadcast(Echoes);
}

bool UHRProgressionComponent::ChooseStartingClass(EHRStartingClass InClass)
{
	if (StartingClass != EHRStartingClass::Unchosen || InClass == EHRStartingClass::Unchosen || static_cast<uint8>(InClass) > 3) return false;
	StartingClass = InClass;
	ApplyClassModifiers();
	UHRStatics::Notify(this, FText::Format(LOCTEXT("Origin", "Origin chosen: {0}"), UEnum::GetDisplayValueAsText(StartingClass)), FLinearColor(0.7f, 0.8f, 1.f));
	return true;
}

void UHRProgressionComponent::RestoreStartingClass(EHRStartingClass InClass)
{
	StartingClass = static_cast<uint8>(InClass) <= 3 ? InClass : EHRStartingClass::Unchosen;
	ApplyClassModifiers();
}

void UHRProgressionComponent::ApplyClassModifiers()
{
	UHRAttributeComponent* Stats = GetOwner() ? GetOwner()->FindComponentByClass<UHRAttributeComponent>() : nullptr;
	if (!Stats) return;
	TArray<FHRStatModifier> Modifiers;
	if (StartingClass == EHRStartingClass::Vanguard) Modifiers = { FHRStatModifier(EHRStat::MaxHealth, 30.f), FHRStatModifier(EHRStat::PhysicalPower, 4.f) };
	else if (StartingClass == EHRStartingClass::Seer) Modifiers = { FHRStatModifier(EHRStat::MaxFocus, 30.f), FHRStatModifier(EHRStat::ArcanePower, 5.f) };
	else if (StartingClass == EHRStartingClass::Shade) Modifiers = { FHRStatModifier(EHRStat::Stealth, 0.15f), FHRStatModifier(EHRStat::CritChance, 0.05f) };
	Stats->SetModifierSource(TEXT("OriginClass"), Modifiers);
}

void UHRProgressionComponent::ApplyAttributeModifiers()
{
	UHRAttributeComponent* Stats = GetOwner() ? GetOwner()->FindComponentByClass<UHRAttributeComponent>() : nullptr;
	if (!Stats)
	{
		return;
	}

	const int32 Vigor = GetAttribute(EHRAttribute::Vigor) - 10;
	const int32 Endurance = GetAttribute(EHRAttribute::Endurance) - 10;
	const int32 Strength = GetAttribute(EHRAttribute::Strength) - 10;
	const int32 Dexterity = GetAttribute(EHRAttribute::Dexterity) - 10;
	const int32 Arcane = GetAttribute(EHRAttribute::Arcane) - 10;
	const int32 Mind = GetAttribute(EHRAttribute::Mind) - 10;

	TArray<FHRStatModifier> Modifiers;
	Modifiers.Add(FHRStatModifier(EHRStat::MaxHealth, 22.f * Soft(Vigor, 30)));
	Modifiers.Add(FHRStatModifier(EHRStat::MaxStamina, 4.f * Soft(Endurance, 30)));
	Modifiers.Add(FHRStatModifier(EHRStat::CarryCapacity, 2.f * Endurance));
	Modifiers.Add(FHRStatModifier(EHRStat::MaxPosture, 1.5f * Soft(Endurance, 30)));
	Modifiers.Add(FHRStatModifier(EHRStat::PhysicalPower, 1.5f * Soft(Strength, 30) + 1.f * Soft(Dexterity, 30)));
	Modifiers.Add(FHRStatModifier(EHRStat::PostureDamage, 0.01f * Strength));
	Modifiers.Add(FHRStatModifier(EHRStat::CritChance, 0.004f * Dexterity));
	Modifiers.Add(FHRStatModifier(EHRStat::ParryWindow, 0.002f * Dexterity));
	Modifiers.Add(FHRStatModifier(EHRStat::ArcanePower, 2.5f * Soft(Arcane, 30)));
	Modifiers.Add(FHRStatModifier(EHRStat::FireResist, 0.005f * Arcane));
	Modifiers.Add(FHRStatModifier(EHRStat::FrostResist, 0.005f * Arcane));
	Modifiers.Add(FHRStatModifier(EHRStat::MaxFocus, 5.f * Soft(Mind, 30)));
	Modifiers.Add(FHRStatModifier(EHRStat::FocusRegen, 0.08f * Mind));
	Stats->SetModifierSource(AttributeSource, Modifiers);
}

void UHRProgressionComponent::ApplySkillModifiers()
{
	UHRAttributeComponent* Stats = GetOwner() ? GetOwner()->FindComponentByClass<UHRAttributeComponent>() : nullptr;
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	if (!Stats || !Content)
	{
		return;
	}
	TArray<FHRStatModifier> Modifiers;
	for (const FName& SkillId : UnlockedSkills)
	{
		if (const FHRSkillNode* Node = Content->FindSkill(SkillId))
		{
			Modifiers.Append(Node->Modifiers);
		}
	}
	Stats->SetModifierSource(SkillSource, Modifiers);
}

#undef LOCTEXT_NAMESPACE
