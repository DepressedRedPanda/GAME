#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/HRTypes.h"
#include "HRInteractable.generated.h"

class AHRPlayerCharacter;

UINTERFACE(MinimalAPI)
class UHRInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Anything the player can use with the Interact key: shrines, chests, levers, NPCs, vehicles... */
class HOLLOWREACH_API IHRInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanInteract(const AHRPlayerCharacter* Player) const { return true; }
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const = 0;
	virtual void Interact(AHRPlayerCharacter* Player) = 0;
};

UINTERFACE(MinimalAPI)
class UHRElementalReactive : public UInterface
{
	GENERATED_BODY()
};

/** Environmental interaction: braziers that light, barrels that explode, ice that melts... */
class HOLLOWREACH_API IHRElementalReactive
{
	GENERATED_BODY()

public:
	virtual void ReactToElement(EHRDamageType Element, AActor* Source) = 0;
};
