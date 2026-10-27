#include "Components/HRAbilityComponent.h"

#include "Abilities/HRAbility.h"
#include "Characters/HRCharacterBase.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Core/HRStatics.h"
#include "Systems/HRContentSubsystem.h"

#define LOCTEXT_NAMESPACE "HRAbilityComponent"

UHRAbilityComponent::UHRAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SlotIds.SetNum(NumSlots);
}

AHRCharacterBase* UHRAbilityComponent::GetOwnerCharacter() const
{
	return Cast<AHRCharacterBase>(GetOwner());
}

void UHRAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AHRCharacterBase* Owner = GetOwnerCharacter();
	UHRAttributeComponent* Attributes = Owner ? Owner->GetAttributes() : nullptr;
	if (!Owner || !Attributes)
	{
		return;
	}

	for (UHRAbility* Ability : Abilities)
	{
		if (!Ability || !Ability->IsActive())
		{
			continue;
		}
		if (Owner->IsDead() || !Attributes->ConsumeFocus(Ability->FocusPerSecond * DeltaTime))
		{
			Ability->Deactivate(Owner);
			CooldownEnds.Add(Ability->AbilityId, GetWorld()->GetTimeSeconds() + Ability->Cooldown);
			continue;
		}
		Ability->TickActive(Owner, DeltaTime);
	}
}

UHRAbility* UHRAbilityComponent::GrantAbility(TSubclassOf<UHRAbility> AbilityClass)
{
	if (!AbilityClass)
	{
		return nullptr;
	}
	const UHRAbility* Defaults = AbilityClass->GetDefaultObject<UHRAbility>();
	if (UHRAbility* Existing = FindAbility(Defaults->AbilityId))
	{
		return Existing;
	}

	UHRAbility* Ability = NewObject<UHRAbility>(this, AbilityClass);
	Abilities.Add(Ability);

	// Auto-slot into the first free slot.
	for (FName& SlotId : SlotIds)
	{
		if (SlotId.IsNone())
		{
			SlotId = Ability->AbilityId;
			break;
		}
	}
	return Ability;
}

UHRAbility* UHRAbilityComponent::GrantAbilityById(FName AbilityId)
{
	const UHRContentSubsystem* Content = UHRContentSubsystem::Get(this);
	return Content ? GrantAbility(Content->FindAbilityClass(AbilityId)) : nullptr;
}

bool UHRAbilityComponent::HasAbility(FName AbilityId) const
{
	return FindAbility(AbilityId) != nullptr;
}

UHRAbility* UHRAbilityComponent::FindAbility(FName AbilityId) const
{
	for (UHRAbility* Ability : Abilities)
	{
		if (Ability && Ability->AbilityId == AbilityId)
		{
			return Ability;
		}
	}
	return nullptr;
}

UHRAbility* UHRAbilityComponent::GetSlotAbility(int32 Slot) const
{
	return SlotIds.IsValidIndex(Slot) ? FindAbility(SlotIds[Slot]) : nullptr;
}

bool UHRAbilityComponent::ActivateSlot(int32 Slot)
{
	if (!SlotIds.IsValidIndex(Slot) || SlotIds[Slot].IsNone())
	{
		UHRStatics::Notify(this, LOCTEXT("EmptySlot", "No ability in that slot. Learn one at a shrine."), FLinearColor::Gray);
		return false;
	}
	return ActivateAbility(SlotIds[Slot]);
}

bool UHRAbilityComponent::ActivateAbility(FName AbilityId)
{
	AHRCharacterBase* Owner = GetOwnerCharacter();
	UHRAbility* Ability = FindAbility(AbilityId);
	if (!Owner || !Ability)
	{
		return false;
	}

	// Toggles switch off on a second press.
	if (Ability->bIsToggle && Ability->IsActive())
	{
		Ability->Deactivate(Owner);
		CooldownEnds.Add(AbilityId, GetWorld()->GetTimeSeconds() + Ability->Cooldown);
		return true;
	}

	if (GetCooldownRemaining(AbilityId) > 0.f)
	{
		return false;
	}
	for (UHRAbility* Other : Abilities)
	{
		if (Other && Other != Ability && Other->IsActive() && Other->bBlocksOtherAbilities)
		{
			UHRStatics::Notify(this, LOCTEXT("Transformed", "Cannot do that while transformed."), FLinearColor::Gray);
			return false;
		}
	}

	FText Reason;
	if (!Ability->CanActivate(Owner, Reason))
	{
		if (!Reason.IsEmpty())
		{
			UHRStatics::Notify(this, Reason, FLinearColor::Gray);
		}
		return false;
	}

	UHRAttributeComponent* Attributes = Owner->GetAttributes();
	if (Attributes && Attributes->GetFocus() < Ability->FocusCost)
	{
		UHRStatics::Notify(this, LOCTEXT("NoFocus", "Not enough focus."), FLinearColor(0.4f, 0.6f, 1.f));
		return false;
	}

	if (UHRCombatComponent* Combat = Owner->GetCombat())
	{
		if (Ability->CastTime > 0.f && !Combat->BeginBusy(Ability->CastTime))
		{
			return false;
		}
	}

	if (Attributes)
	{
		Attributes->ConsumeFocus(Ability->FocusCost);
	}
	Ability->Activate(Owner);
	if (!Ability->bIsToggle)
	{
		CooldownEnds.Add(AbilityId, GetWorld()->GetTimeSeconds() + Ability->Cooldown);
	}
	return true;
}

void UHRAbilityComponent::DeactivateAll()
{
	AHRCharacterBase* Owner = GetOwnerCharacter();
	for (UHRAbility* Ability : Abilities)
	{
		if (Ability && Ability->IsActive())
		{
			Ability->Deactivate(Owner);
		}
	}
}

float UHRAbilityComponent::GetCooldownRemaining(FName AbilityId) const
{
	const double* End = CooldownEnds.Find(AbilityId);
	if (!End || !GetWorld())
	{
		return 0.f;
	}
	return FMath::Max(0.f, static_cast<float>(*End - GetWorld()->GetTimeSeconds()));
}

bool UHRAbilityComponent::IsTransformed() const
{
	for (const UHRAbility* Ability : Abilities)
	{
		if (Ability && Ability->IsActive() && Ability->bBlocksOtherAbilities)
		{
			return true;
		}
	}
	return false;
}

TArray<FName> UHRAbilityComponent::GetGrantedIds() const
{
	TArray<FName> Ids;
	for (const UHRAbility* Ability : Abilities)
	{
		if (Ability)
		{
			Ids.Add(Ability->AbilityId);
		}
	}
	return Ids;
}

void UHRAbilityComponent::RestoreSlots(const TArray<FName>& InSlots)
{
	for (int32 Index = 0; Index < NumSlots; ++Index)
	{
		SlotIds[Index] = InSlots.IsValidIndex(Index) && HasAbility(InSlots[Index]) ? InSlots[Index] : SlotIds[Index];
	}
}

#undef LOCTEXT_NAMESPACE
