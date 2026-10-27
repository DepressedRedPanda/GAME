#include "Characters/HRPlayerCharacter.h"

#include "AI/HRAIController.h"
#include "Camera/CameraComponent.h"
#include "Characters/HRCompanionCharacter.h"
#include "Components/HRAbilityComponent.h"
#include "Components/HRAttributeComponent.h"
#include "Components/HRCombatComponent.h"
#include "Components/HRCraftingComponent.h"
#include "Components/HRInteractionComponent.h"
#include "Components/HRInventoryComponent.h"
#include "Components/HRProgressionComponent.h"
#include "Components/HRStealthComponent.h"
#include "Components/HRSurvivalComponent.h"
#include "Components/HRTraversalComponent.h"
#include "Core/HRInputSet.h"
#include "Core/HRStatics.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "Game/HRGameMode.h"
#include "Game/HRPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationInvokerComponent.h"
#include "Systems/HRFactionSubsystem.h"
#include "Systems/HRSaveSubsystem.h"
#include "World/HRCheckpoint.h"
#include "World/HRBuiltShelter.h"
#include "Narrative/HRNarrativeSubsystem.h"

#define LOCTEXT_NAMESPACE "HRPlayer"

AHRPlayerCharacter::AHRPlayerCharacter()
{
	SharedAppearanceRole = TEXT("Player");
	FactionId = TEXT("Player");
	DisplayName = LOCTEXT("PlayerName", "The Ashen Wanderer");
	BodyColor = FLinearColor(0.15f, 0.3f, 0.65f);
	AutoPossessPlayer = EAutoReceiveInput::Disabled;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 380.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 14.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Inventory = CreateDefaultSubobject<UHRInventoryComponent>(TEXT("Inventory"));
	Progression = CreateDefaultSubobject<UHRProgressionComponent>(TEXT("Progression"));
	Crafting = CreateDefaultSubobject<UHRCraftingComponent>(TEXT("Crafting"));
	Survival = CreateDefaultSubobject<UHRSurvivalComponent>(TEXT("Survival"));
	Stealth = CreateDefaultSubobject<UHRStealthComponent>(TEXT("Stealth"));
	Interaction = CreateDefaultSubobject<UHRInteractionComponent>(TEXT("Interaction"));
	Traversal = CreateDefaultSubobject<UHRTraversalComponent>(TEXT("Traversal"));
	NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
	NavInvoker->SetGenerationRadii(6000.f, 8000.f);

	Inventory->StartingItems = {
		FHRItemAmount(TEXT("Item.Herb"), 3),
		FHRItemAmount(TEXT("Item.Bread"), 2),
		FHRItemAmount(TEXT("Item.Waterskin"), 2),
		FHRItemAmount(TEXT("Item.Bandage"), 1),
		FHRItemAmount(TEXT("Item.Firebomb"), 2),
	};
	Inventory->StartingEquipment = { TEXT("Item.IronBlade"), TEXT("Item.LeatherCoat") };
}

void AHRPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	Attributes->bCanBeInjured = true;
	Inventory->InitializeStartingLoadout();
	Traversal->RefreshUnlocks();

	Combat->OnPerfectDodge.AddDynamic(this, &AHRPlayerCharacter::HandlePerfectDodge);
	Combat->OnDeflectSuccess.AddDynamic(this, &AHRPlayerCharacter::HandleDeflect);
	Combat->OnAttackStarted.AddDynamic(this, &AHRPlayerCharacter::HandleAttackStarted);

	// Restore a loaded save (if any) now that every component is live.
	Attributes->RefillAll();
	ResurrectionCharges = GetMaxResurrectionCharges();
	if (UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(this))
	{
		Saves->ApplyPendingPlayerState(this);
	}
}

void AHRPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement->IsFalling())
	{
		LastGroundedTime = GetWorld()->GetTimeSeconds();
	}

	// Sprinting drains stamina; running dry stops the sprint until the key is pressed again.
	if (IsSprinting() && Movement->IsMovingOnGround() && GetVelocity().Size2D() > 100.f)
	{
		Attributes->ConsumeStamina(SprintStaminaPerSecond * DeltaSeconds);
		if (Attributes->IsExhausted())
		{
			bWantsSprint = false;
		}
	}
	Traversal->SetWantsSprint(IsSprinting());
	Traversal->RefreshUnlocks();

	UpdateLockOn(DeltaSeconds);
}

// --- Input -------------------------------------------------------------------------------------

void AHRPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AHRPlayerController* PC = Cast<AHRPlayerController>(GetController());
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!PC || !Input)
	{
		return;
	}
	PC->EnsureInputMapping();
	const UHRInputSet* Set = PC->GetInputSet();

	Input->BindAction(Set->Move, ETriggerEvent::Triggered, this, &AHRPlayerCharacter::OnMove);
	Input->BindAction(Set->Look, ETriggerEvent::Triggered, this, &AHRPlayerCharacter::OnLook);
	Input->BindAction(Set->Jump, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnJumpPressed);
	Input->BindAction(Set->Jump, ETriggerEvent::Completed, this, &AHRPlayerCharacter::OnJumpReleased);
	Input->BindAction(Set->Sprint, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnSprintPressed);
	Input->BindAction(Set->Sprint, ETriggerEvent::Completed, this, &AHRPlayerCharacter::OnSprintReleased);
	Input->BindAction(Set->Crouch, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnCrouch);
	Input->BindAction(Set->Dodge, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnDodge);
	Input->BindAction(Set->LightAttack, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnLightAttack);
	Input->BindAction(Set->HeavyAttack, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnHeavyAttack);
	Input->BindAction(Set->Block, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnBlockPressed);
	Input->BindAction(Set->Block, ETriggerEvent::Completed, this, &AHRPlayerCharacter::OnBlockReleased);
	Input->BindAction(Set->Interact, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnInteract);
	Input->BindAction(Set->LockOn, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnLockOn);
	Input->BindAction(Set->UseItem, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnUseItem);
	Input->BindAction(Set->CycleItem, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnCycleItem);
	Input->BindAction(Set->CompanionCommand, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnCompanionCommand);
	Input->BindAction(Set->Journal, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnJournal);
	Input->BindAction(Set->MenuTab, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnMenuTab);
	Input->BindAction(Set->MenuBack, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnMenuBack);
	Input->BindAction(Set->BuildShelter, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnBuildShelter);
	Input->BindAction(Set->QuickSave, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnQuickSave);
	Input->BindAction(Set->QuickLoad, ETriggerEvent::Started, this, &AHRPlayerCharacter::OnQuickLoad);

	using FHotkeyHandler = void (AHRPlayerCharacter::*)();
	const FHotkeyHandler Handlers[UHRInputSet::NumHotkeys] = {
		&AHRPlayerCharacter::OnHotkey1, &AHRPlayerCharacter::OnHotkey2, &AHRPlayerCharacter::OnHotkey3,
		&AHRPlayerCharacter::OnHotkey4, &AHRPlayerCharacter::OnHotkey5, &AHRPlayerCharacter::OnHotkey6,
		&AHRPlayerCharacter::OnHotkey7, &AHRPlayerCharacter::OnHotkey8, &AHRPlayerCharacter::OnHotkey9,
	};
	for (int32 Index = 0; Index < UHRInputSet::NumHotkeys && Index < Set->Hotkeys.Num(); ++Index)
	{
		Input->BindAction(Set->Hotkeys[Index], ETriggerEvent::Started, this, Handlers[Index]);
	}
}

bool AHRPlayerCharacter::IsInMenu() const
{
	const AHRPlayerController* PC = Cast<AHRPlayerController>(GetController());
	return PC && PC->GetUIMode() != EHRUIMode::Gameplay;
}

bool AHRPlayerCharacter::CanControl() const
{
	return !IsInMenu() && !bIsDead && !bDowned;
}

FVector AHRPlayerCharacter::GetMoveInputWorldDirection() const
{
	return GetLastMovementInputVector();
}

void AHRPlayerCharacter::OnBuildShelter()
{
	if (CanControl() && Combat->CanAct()) AHRBuiltShelter::BuildFor(this);
}

void AHRPlayerCharacter::OnQuickSave()
{
	if (!CanControl()) return;
	if (UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(this)) Saves->SaveProgress(this);
}

void AHRPlayerCharacter::OnQuickLoad()
{
	if (!CanControl() || GetWorld()->GetNetMode() != NM_Standalone) return;
	if (const UHRSaveSubsystem* Saves = HRGetSubsystem<UHRSaveSubsystem>(this))
	{
		if (Saves->HasSave()) UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	}
}

void AHRPlayerCharacter::OnMove(const FInputActionValue& Value)
{
	if (!CanControl() || !Controller)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AHRPlayerCharacter::OnLook(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void AHRPlayerCharacter::OnJumpPressed()
{
	if (!CanControl() || !Combat->CanAct())
	{
		return;
	}
	const FVector View = Controller ? Controller->GetControlRotation().Vector() : GetActorForwardVector();
	if (Traversal->HandleJumpPressed(View))
	{
		return;
	}

	// Coyote time: a jump just after running off a ledge still counts.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement->IsFalling() && JumpCurrentCount == 0 && GetWorld()->GetTimeSeconds() - LastGroundedTime < 0.18)
	{
		LaunchCharacter(FVector(0.f, 0.f, Movement->JumpZVelocity), false, true);
		JumpCurrentCount = 1;
		return;
	}
	Jump();
}

void AHRPlayerCharacter::OnJumpReleased()
{
	StopJumping();
	Traversal->HandleJumpReleased();
}

void AHRPlayerCharacter::OnSprintPressed()
{
	bWantsSprint = true;
	if (bIsCrouched)
	{
		UnCrouch();
	}
}

void AHRPlayerCharacter::OnSprintReleased()
{
	bWantsSprint = false;
}

void AHRPlayerCharacter::OnCrouch()
{
	if (!CanControl())
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		bWantsSprint = false;
		Crouch();
	}
}

void AHRPlayerCharacter::OnDodge()
{
	if (!CanControl())
	{
		return;
	}
	if (Inventory->GetEncumbrance() > 1.25f)
	{
		UHRStatics::Notify(this, LOCTEXT("TooHeavy", "Too heavily burdened to dodge."), FLinearColor::Gray);
		return;
	}
	Combat->RequestDodge(GetMoveInputWorldDirection());
}

void AHRPlayerCharacter::OnLightAttack()
{
	if (!CanControl())
	{
		return;
	}
	if (Combat->TryStealthTakedown())
	{
		Stealth->MakeNoise(0.1f);
		return;
	}
	Combat->RequestLightAttack();
}

void AHRPlayerCharacter::OnHeavyAttack()
{
	if (CanControl())
	{
		Combat->RequestHeavyAttack();
	}
}

void AHRPlayerCharacter::OnBlockPressed()
{
	if (CanControl())
	{
		Combat->StartBlock();
	}
}

void AHRPlayerCharacter::OnBlockReleased()
{
	Combat->StopBlock();
}

void AHRPlayerCharacter::OnInteract()
{
	if (bDowned)
	{
		Resurrect(false);
		return;
	}
	if (CanControl())
	{
		Interaction->Interact();
	}
}

void AHRPlayerCharacter::OnLockOn()
{
	if (CanControl())
	{
		ToggleLockOn();
	}
}

void AHRPlayerCharacter::OnUseItem()
{
	if (!CanControl())
	{
		return;
	}
	if (Abilities->IsTransformed())
	{
		UHRStatics::Notify(this, LOCTEXT("BeastNoItems", "Beasts cannot use items."), FLinearColor::Gray);
		return;
	}
	Inventory->UseQuickItem();
}

void AHRPlayerCharacter::OnCycleItem()
{
	Inventory->CycleQuickItem();
}

void AHRPlayerCharacter::OnCompanionCommand()
{
	if (AHRCompanionCharacter* Ally = Companion.Get())
	{
		Ally->CycleCommand();
	}
	else
	{
		UHRStatics::Notify(this, LOCTEXT("NoCompanion", "You travel alone."), FLinearColor::Gray);
	}
}

void AHRPlayerCharacter::OnJournal()
{
	if (AHRPlayerController* PC = Cast<AHRPlayerController>(GetController()))
	{
		PC->ToggleJournal();
	}
}

void AHRPlayerCharacter::OnMenuTab()
{
	if (AHRPlayerController* PC = Cast<AHRPlayerController>(GetController()))
	{
		PC->NextMenuTab();
	}
}

void AHRPlayerCharacter::OnMenuBack()
{
	if (AHRPlayerController* PC = Cast<AHRPlayerController>(GetController()))
	{
		PC->CloseMenu();
	}
}

void AHRPlayerCharacter::HandleHotkey(int32 Index)
{
	AHRPlayerController* PC = Cast<AHRPlayerController>(GetController());
	if (PC && PC->GetUIMode() != EHRUIMode::Gameplay)
	{
		PC->HandleMenuHotkey(Index);
		return;
	}
	if (CanControl() && Index < UHRAbilityComponent::NumSlots)
	{
		Abilities->ActivateSlot(Index);
	}
}

// --- Combat feedback ---------------------------------------------------------------------------

void AHRPlayerCharacter::HandlePerfectDodge()
{
	// Brief world slow-motion while the player keeps moving at full speed.
	constexpr float Dilation = 0.35f;
	UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
	CustomTimeDilation = 1.f / Dilation;
	UHRStatics::Notify(this, LOCTEXT("PerfectDodge", "Perfect dodge"), FLinearColor(0.7f, 0.9f, 1.f));

	TWeakObjectPtr<AHRPlayerCharacter> WeakThis = this;
	GetWorldTimerManager().SetTimer(SlowMoTimer, FTimerDelegate::CreateLambda([WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			UGameplayStatics::SetGlobalTimeDilation(WeakThis.Get(), 1.f);
			WeakThis->CustomTimeDilation = 1.f;
		}
	}), 0.8f * Dilation, false);
}

void AHRPlayerCharacter::HandleDeflect()
{
	Stealth->MakeNoise(0.7f);
}

void AHRPlayerCharacter::HandleAttackStarted(int32 ComboIndex, bool bPerilous)
{
	Stealth->MakeNoise(0.6f);
}

// --- Lock-on -----------------------------------------------------------------------------------

void AHRPlayerCharacter::ToggleLockOn()
{
	if (LockTarget.IsValid())
	{
		LockTarget = nullptr;
	}
	else
	{
		LockTarget = FindLockTarget();
	}
	Combat->SetCurrentTarget(LockTarget.Get());
	const bool bLocked = LockTarget.IsValid();
	GetCharacterMovement()->bOrientRotationToMovement = !bLocked;
	GetCharacterMovement()->bUseControllerDesiredRotation = bLocked;
}

AActor* AHRPlayerCharacter::FindLockTarget() const
{
	if (!Controller)
	{
		return nullptr;
	}
	const FVector ViewDirection = Controller->GetControlRotation().Vector();
	AActor* Best = nullptr;
	float BestScore = -1.f;
	for (TActorIterator<AHRCharacterBase> It(GetWorld()); It; ++It)
	{
		AHRCharacterBase* Candidate = *It;
		if (Candidate == this || Candidate->IsDead() || UHRFactionSubsystem::AreFriendly(this, Candidate))
		{
			continue;
		}
		const FVector ToCandidate = Candidate->GetActorLocation() - GetActorLocation();
		const float Distance = ToCandidate.Size();
		if (Distance > LockOnRange)
		{
			continue;
		}
		const float Dot = FVector::DotProduct(ViewDirection.GetSafeNormal2D(), ToCandidate.GetSafeNormal2D());
		if (Dot < 0.5f)
		{
			continue;
		}
		const float Score = Dot * 2.f - Distance / LockOnRange + (UHRFactionSubsystem::AreHostile(this, Candidate) ? 1.f : 0.f);
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}
	return Best;
}

void AHRPlayerCharacter::UpdateLockOn(float DeltaSeconds)
{
	if (!LockTarget.IsValid())
	{
		return;
	}
	const AHRCharacterBase* Target = Cast<AHRCharacterBase>(LockTarget.Get());
	if (!Target || Target->IsDead() || FVector::Dist(Target->GetActorLocation(), GetActorLocation()) > LockOnRange * 1.25f)
	{
		ToggleLockOn();
		return;
	}
	if (Controller)
	{
		FRotator Desired = (Target->GetActorLocation() - GetActorLocation()).Rotation();
		Desired.Pitch = FMath::Clamp(Desired.Pitch - 12.f, -45.f, 20.f);
		Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), Desired, DeltaSeconds, 8.f));
	}
}

// --- Death & resurrection ----------------------------------------------------------------------

int32 AHRPlayerCharacter::GetMaxResurrectionCharges() const
{
	return 1 + (Progression->HasSkill(TEXT("Skill.SecondWind")) ? 1 : 0);
}

float AHRPlayerCharacter::GetDownedTimeRemaining() const
{
	return bDowned ? FMath::Max(0.f, static_cast<float>(DownedUntil - GetWorld()->GetTimeSeconds())) : 0.f;
}

void AHRPlayerCharacter::HandleHealthDepleted(UHRAttributeComponent* InAttributes, AActor* Killer)
{
	if (bIsDead)
	{
		return;
	}
	if (!bDowned && ResurrectionCharges > 0)
	{
		bDowned = true;
		DownedBy = Killer;
		DownedUntil = GetWorld()->GetTimeSeconds() + DownedWindow;
		Combat->SetDowned(true);
		Abilities->DeactivateAll();
		GetCharacterMovement()->StopMovementImmediately();
		UHRStatics::Notify(this, LOCTEXT("Downed", "You fall... [E] to rise again"), FLinearColor(0.9f, 0.3f, 0.3f), true);

		TWeakObjectPtr<AHRPlayerCharacter> WeakThis = this;
		GetWorldTimerManager().SetTimer(DownedTimer, FTimerDelegate::CreateLambda([WeakThis]()
		{
			if (WeakThis.IsValid() && WeakThis->bDowned)
			{
				WeakThis->bDowned = false;
				WeakThis->Die(WeakThis->DownedBy.Get());
			}
		}), DownedWindow, false);
		return;
	}
	bDowned = false;
	Die(Killer);
}

void AHRPlayerCharacter::Resurrect(bool bFree)
{
	if (!bDowned || bIsDead)
	{
		return;
	}
	if (!bFree)
	{
		if (ResurrectionCharges <= 0)
		{
			return;
		}
		--ResurrectionCharges;
	}
	bDowned = false;
	KillsSinceResurrection = 0;
	GetWorldTimerManager().ClearTimer(DownedTimer);
	Attributes->SetHealthFraction(0.5f);
	Attributes->ResetPosture();
	Combat->SetDowned(false);
	Combat->GrantInvulnerability(2.f);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// Enemies think you're dead: a chance to slip away or strike back unseen.
	for (TActorIterator<AHRAIController> It(GetWorld()); It; ++It)
	{
		It->LoseTrackOf(this);
	}
	UHRStatics::Notify(this, bFree ? LOCTEXT("Revived", "Your companion drags you back to your feet.") : LOCTEXT("Resurrected", "Resurrection."), FLinearColor(1.f, 0.4f, 0.3f), true);
}

void AHRPlayerCharacter::NotifyKilledEnemy()
{
	if (UHRNarrativeSubsystem* Narrative = HRGetSubsystem<UHRNarrativeSubsystem>(this)) Narrative->AddFlag(TEXT("Stats.Kills"));
	if (ResurrectionCharges >= GetMaxResurrectionCharges())
	{
		return;
	}
	if (++KillsSinceResurrection >= KillsToRestoreResurrection)
	{
		KillsSinceResurrection = 0;
		++ResurrectionCharges;
		UHRStatics::Notify(this, LOCTEXT("ResRestored", "Resurrection restored."), FLinearColor(1.f, 0.4f, 0.3f));
	}
}

void AHRPlayerCharacter::Die(AActor* Killer)
{
	if (bIsDead)
	{
		return;
	}
	if (LockTarget.IsValid())
	{
		ToggleLockOn();
	}
	Super::Die(Killer);
	bDowned = false;
	if (AHRGameMode* GameMode = GetWorld()->GetAuthGameMode<AHRGameMode>())
	{
		GameMode->HandlePlayerDeath(this, Killer);
	}
}

void AHRPlayerCharacter::RespawnAt(const FTransform& SpawnTransform)
{
	ReviveCharacter(1.f);
	bDowned = false;
	TeleportTo(SpawnTransform.GetLocation(), SpawnTransform.Rotator());
	if (Controller)
	{
		Controller->SetControlRotation(SpawnTransform.Rotator());
	}
	ResurrectionCharges = GetMaxResurrectionCharges();
	Inventory->RefillFlasks();
	Survival->RestoreAll();
}

// --- Rest --------------------------------------------------------------------------------------

void AHRPlayerCharacter::BeginRest(AHRCheckpoint* Checkpoint)
{
	if (!Checkpoint)
	{
		return;
	}
	bResting = true;
	if (LockTarget.IsValid())
	{
		ToggleLockOn();
	}
	Abilities->DeactivateAll();
	Combat->ResetCombatState();
	Attributes->CureAllInjuries();
	Attributes->RefillAll();
	Inventory->RefillFlasks();
	Survival->RestoreAll();
	ResurrectionCharges = GetMaxResurrectionCharges();
	Crafting->SetStation(EHRCraftStation::Campfire, Checkpoint->bHasForge);

	if (AHRGameMode* GameMode = GetWorld()->GetAuthGameMode<AHRGameMode>())
	{
		GameMode->OnPlayerRested(this, Checkpoint);
	}
	if (AHRPlayerController* PC = Cast<AHRPlayerController>(GetController()))
	{
		PC->OpenRestMenu(Checkpoint);
	}
}

void AHRPlayerCharacter::EndRest()
{
	bResting = false;
	Crafting->SetStation(EHRCraftStation::None, false);
}

void AHRPlayerCharacter::SetSpiritSight(bool bEnabled)
{
	bSpiritSight = bEnabled;
	FPostProcessSettings& Settings = FollowCamera->PostProcessSettings;
	Settings.bOverride_ColorSaturation = bEnabled;
	Settings.ColorSaturation = FVector4(0.25f, 0.25f, 0.35f, 1.f);
	Settings.bOverride_SceneColorTint = bEnabled;
	Settings.SceneColorTint = FLinearColor(0.75f, 0.9f, 1.2f);
	FollowCamera->PostProcessBlendWeight = bEnabled ? 1.f : 0.f;
}

// --- Stats -------------------------------------------------------------------------------------

float AHRPlayerCharacter::GetAttributeScaling(EHRAttribute Attribute) const
{
	return Progression->GetAttributeScaling(Attribute);
}

float AHRPlayerCharacter::GetMovementMultiplier() const
{
	float Multiplier = Super::GetMovementMultiplier();
	if (Inventory->GetEncumbrance() > 1.f)
	{
		Multiplier *= 0.6f;
	}
	if (bResting || bDowned)
	{
		Multiplier = 0.f;
	}
	return Multiplier;
}

#undef LOCTEXT_NAMESPACE
