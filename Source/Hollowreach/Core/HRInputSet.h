#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HRInputSet.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * All Enhanced Input actions and the default keyboard/mouse + gamepad mapping, created in code
 * so the project is playable without authoring input assets. Designers can replace any action
 * with an asset later; the bindings only reference these pointers.
 */
UCLASS()
class HOLLOWREACH_API UHRInputSet : public UObject
{
	GENERATED_BODY()

public:
	void Build();

	static constexpr int32 NumHotkeys = 9;

	UPROPERTY() TObjectPtr<UInputMappingContext> Context;

	UPROPERTY() TObjectPtr<UInputAction> Move;
	UPROPERTY() TObjectPtr<UInputAction> Look;
	UPROPERTY() TObjectPtr<UInputAction> Jump;
	UPROPERTY() TObjectPtr<UInputAction> Sprint;
	UPROPERTY() TObjectPtr<UInputAction> Crouch;
	UPROPERTY() TObjectPtr<UInputAction> Dodge;
	UPROPERTY() TObjectPtr<UInputAction> LightAttack;
	UPROPERTY() TObjectPtr<UInputAction> HeavyAttack;
	UPROPERTY() TObjectPtr<UInputAction> Block;
	UPROPERTY() TObjectPtr<UInputAction> Interact;
	UPROPERTY() TObjectPtr<UInputAction> LockOn;
	UPROPERTY() TObjectPtr<UInputAction> UseItem;
	UPROPERTY() TObjectPtr<UInputAction> CycleItem;
	UPROPERTY() TObjectPtr<UInputAction> CompanionCommand;
	UPROPERTY() TObjectPtr<UInputAction> Journal;
	UPROPERTY() TObjectPtr<UInputAction> MenuTab;
	UPROPERTY() TObjectPtr<UInputAction> MenuBack;
	UPROPERTY() TObjectPtr<UInputAction> BuildShelter;
	UPROPERTY() TObjectPtr<UInputAction> QuickSave;
	UPROPERTY() TObjectPtr<UInputAction> QuickLoad;

	/** Number keys 1-9: ability slots in gameplay, menu selection in menus. */
	UPROPERTY() TArray<TObjectPtr<UInputAction>> Hotkeys;

private:
	UInputAction* MakeAction(const TCHAR* Name, bool bAxis2D);
};
