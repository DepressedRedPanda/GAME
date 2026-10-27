#include "Core/HRInputSet.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"

UInputAction* UHRInputSet::MakeAction(const TCHAR* Name, bool bAxis2D)
{
	UInputAction* Action = NewObject<UInputAction>(this, FName(Name));
	Action->ValueType = bAxis2D ? EInputActionValueType::Axis2D : EInputActionValueType::Boolean;
	return Action;
}

void UHRInputSet::Build()
{
	if (Context)
	{
		return;
	}

	Context = NewObject<UInputMappingContext>(this, TEXT("IMC_Hollowreach"));
	BuildShelter = MakeAction(TEXT("BuildShelter"), false);
	QuickSave = MakeAction(TEXT("QuickSave"), false);
	QuickLoad = MakeAction(TEXT("QuickLoad"), false);

	Move = MakeAction(TEXT("IA_Move"), true);
	Look = MakeAction(TEXT("IA_Look"), true);
	Jump = MakeAction(TEXT("IA_Jump"), false);
	Sprint = MakeAction(TEXT("IA_Sprint"), false);
	Crouch = MakeAction(TEXT("IA_Crouch"), false);
	Dodge = MakeAction(TEXT("IA_Dodge"), false);
	LightAttack = MakeAction(TEXT("IA_LightAttack"), false);
	HeavyAttack = MakeAction(TEXT("IA_HeavyAttack"), false);
	Block = MakeAction(TEXT("IA_Block"), false);
	Interact = MakeAction(TEXT("IA_Interact"), false);
	LockOn = MakeAction(TEXT("IA_LockOn"), false);
	UseItem = MakeAction(TEXT("IA_UseItem"), false);
	CycleItem = MakeAction(TEXT("IA_CycleItem"), false);
	CompanionCommand = MakeAction(TEXT("IA_CompanionCommand"), false);
	Journal = MakeAction(TEXT("IA_Journal"), false);
	MenuTab = MakeAction(TEXT("IA_MenuTab"), false);
	MenuBack = MakeAction(TEXT("IA_MenuBack"), false);

	// --- Movement: WASD as a 2D axis (X = right, Y = forward) ---
	{
		FEnhancedActionKeyMapping& W = Context->MapKey(Move, EKeys::W);
		W.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));

		FEnhancedActionKeyMapping& S = Context->MapKey(Move, EKeys::S);
		S.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		S.Modifiers.Add(NewObject<UInputModifierNegate>(Context));

		FEnhancedActionKeyMapping& A = Context->MapKey(Move, EKeys::A);
		A.Modifiers.Add(NewObject<UInputModifierNegate>(Context));

		Context->MapKey(Move, EKeys::D);

		FEnhancedActionKeyMapping& Stick = Context->MapKey(Move, EKeys::Gamepad_Left2D);
		Stick.Modifiers.Add(NewObject<UInputModifierDeadZone>(Context));
	}

	// --- Camera ---
	{
		Context->MapKey(Look, EKeys::Mouse2D);

		FEnhancedActionKeyMapping& Stick = Context->MapKey(Look, EKeys::Gamepad_Right2D);
		Stick.Modifiers.Add(NewObject<UInputModifierDeadZone>(Context));
		UInputModifierScalar* Scale = NewObject<UInputModifierScalar>(Context);
		Scale->Scalar = FVector(2.5);
		Stick.Modifiers.Add(Scale);
	}

	// --- Actions (keyboard/mouse first, then gamepad) ---
	Context->MapKey(Jump, EKeys::SpaceBar);
	Context->MapKey(BuildShelter, EKeys::B);
	Context->MapKey(QuickSave, EKeys::F5);
	Context->MapKey(QuickLoad, EKeys::F9);
	Context->MapKey(Jump, EKeys::Gamepad_FaceButton_Bottom);

	Context->MapKey(Sprint, EKeys::LeftShift);
	Context->MapKey(Sprint, EKeys::Gamepad_LeftThumbstick);

	Context->MapKey(Crouch, EKeys::C);
	Context->MapKey(Crouch, EKeys::Gamepad_DPad_Left);

	Context->MapKey(Dodge, EKeys::LeftControl);
	Context->MapKey(Dodge, EKeys::Gamepad_FaceButton_Right);

	Context->MapKey(LightAttack, EKeys::LeftMouseButton);
	Context->MapKey(LightAttack, EKeys::Gamepad_RightShoulder);

	Context->MapKey(HeavyAttack, EKeys::R);
	Context->MapKey(HeavyAttack, EKeys::Gamepad_RightTrigger);

	Context->MapKey(Block, EKeys::RightMouseButton);
	Context->MapKey(Block, EKeys::Gamepad_LeftShoulder);

	Context->MapKey(Interact, EKeys::E);
	Context->MapKey(Interact, EKeys::Gamepad_FaceButton_Left);

	Context->MapKey(LockOn, EKeys::MiddleMouseButton);
	Context->MapKey(LockOn, EKeys::T);
	Context->MapKey(LockOn, EKeys::Gamepad_RightThumbstick);

	Context->MapKey(UseItem, EKeys::Q);
	Context->MapKey(UseItem, EKeys::Gamepad_DPad_Down);

	Context->MapKey(CycleItem, EKeys::X);
	Context->MapKey(CycleItem, EKeys::Gamepad_DPad_Right);

	Context->MapKey(CompanionCommand, EKeys::G);
	Context->MapKey(CompanionCommand, EKeys::Gamepad_DPad_Up);

	Context->MapKey(Journal, EKeys::J);
	Context->MapKey(Journal, EKeys::Gamepad_Special_Right);

	Context->MapKey(MenuTab, EKeys::Tab);
	Context->MapKey(MenuTab, EKeys::Gamepad_Special_Left);

	Context->MapKey(MenuBack, EKeys::BackSpace);

	const FKey NumberKeys[NumHotkeys] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
		EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
	};
	for (int32 Index = 0; Index < NumHotkeys; ++Index)
	{
		UInputAction* Hotkey = MakeAction(*FString::Printf(TEXT("IA_Hotkey%d"), Index + 1), false);
		Context->MapKey(Hotkey, NumberKeys[Index]);
		Hotkeys.Add(Hotkey);
	}
	// Gamepad: Y casts ability slot 1, left trigger casts slot 2.
	Context->MapKey(Hotkeys[0], EKeys::Gamepad_FaceButton_Top);
	Context->MapKey(Hotkeys[1], EKeys::Gamepad_LeftTrigger);
}
