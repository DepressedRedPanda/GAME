#include "Abilities/HRAbility.h"

#include "Characters/HRCharacterBase.h"

bool UHRAbility::CanActivate(const AHRCharacterBase* Owner, FText& OutReason) const
{
	if (!Owner || Owner->IsDead())
	{
		return false;
	}
	return true;
}

void UHRAbility::Activate(AHRCharacterBase* Owner)
{
	if (bIsToggle)
	{
		bActive = true;
	}
	K2_OnActivated(Owner);
}

void UHRAbility::Deactivate(AHRCharacterBase* Owner)
{
	bActive = false;
	K2_OnDeactivated(Owner);
}

UWorld* UHRAbility::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}
