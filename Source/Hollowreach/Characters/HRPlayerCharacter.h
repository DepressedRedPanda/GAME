#pragma once

#include "CoreMinimal.h"
#include "Characters/HRCharacterBase.h"
#include "HRPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UNavigationInvokerComponent;
class UHRInventoryComponent;
class UHRProgressionComponent;
class UHRCraftingComponent;
class UHRSurvivalComponent;
class UHRStealthComponent;
class UHRInteractionComponent;
class UHRTraversalComponent;
class AHRCheckpoint;
class AHRCompanionCharacter;
struct FInputActionValue;

UCLASS()
class HOLLOWREACH_API AHRPlayerCharacter : public AHRCharacterBase
{
	GENERATED_BODY()

public:
	AHRPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual float GetAttributeScaling(EHRAttribute Attribute) const override;
	virtual void Die(AActor* Killer) override;

	UFUNCTION(BlueprintPure, Category="Player") UHRInventoryComponent* GetInventory() const { return Inventory; }
	UFUNCTION(BlueprintPure, Category="Player") UHRProgressionComponent* GetProgression() const { return Progression; }
	UFUNCTION(BlueprintPure, Category="Player") UHRCraftingComponent* GetCrafting() const { return Crafting; }
	UFUNCTION(BlueprintPure, Category="Player") UHRSurvivalComponent* GetSurvival() const { return Survival; }
	UFUNCTION(BlueprintPure, Category="Player") UHRStealthComponent* GetStealth() const { return Stealth; }
	UFUNCTION(BlueprintPure, Category="Player") UHRInteractionComponent* GetInteraction() const { return Interaction; }
	UFUNCTION(BlueprintPure, Category="Player") UHRTraversalComponent* GetTraversal() const { return Traversal; }

	// --- Lock-on ---
	UFUNCTION(BlueprintPure, Category="Player") AActor* GetLockTarget() const { return LockTarget.Get(); }
	void ToggleLockOn();

	// --- Death & resurrection (Sekiro) ---
	UFUNCTION(BlueprintPure, Category="Player") bool IsDowned() const { return bDowned; }
	UFUNCTION(BlueprintPure, Category="Player") int32 GetResurrectionCharges() const { return ResurrectionCharges; }
	UFUNCTION(BlueprintPure, Category="Player") int32 GetMaxResurrectionCharges() const;
	UFUNCTION(BlueprintPure, Category="Player") float GetDownedTimeRemaining() const;
	/** bFree: revived by a companion instead of spending a charge. */
	UFUNCTION(BlueprintCallable, Category="Player") void Resurrect(bool bFree);
	void SetResurrectionCharges(int32 Charges) { ResurrectionCharges = Charges; }
	void RespawnAt(const FTransform& SpawnTransform);

	// --- Rest ---
	void BeginRest(AHRCheckpoint* Checkpoint);
	void EndRest();
	UFUNCTION(BlueprintPure, Category="Player") bool IsResting() const { return bResting; }

	// --- Spirit sight ---
	void SetSpiritSight(bool bEnabled);
	UFUNCTION(BlueprintPure, Category="Player") bool IsSpiritSightActive() const { return bSpiritSight; }

	// --- Companion ---
	void SetCompanion(AHRCompanionCharacter* InCompanion) { Companion = InCompanion; }
	UFUNCTION(BlueprintPure, Category="Player") AHRCompanionCharacter* GetCompanion() const { return Companion.Get(); }

	UFUNCTION(BlueprintPure, Category="Player") bool IsSprinting() const { return bWantsSprint && !bIsCrouched; }
	UFUNCTION(BlueprintPure, Category="Player") int32 GetKillsSinceResurrection() const { return KillsSinceResurrection; }
	void NotifyKilledEnemy();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player") float SprintStaminaPerSecond = 9.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player") float DownedWindow = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player") int32 KillsToRestoreResurrection = 6;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player") float LockOnRange = 2200.f;

protected:
	virtual void HandleHealthDepleted(UHRAttributeComponent* InAttributes, AActor* Killer) override;
	virtual float GetMovementMultiplier() const override;
	virtual bool WantsToSprint() const override { return IsSprinting(); }

	// Input
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpPressed();
	void OnJumpReleased();
	void OnSprintPressed();
	void OnSprintReleased();
	void OnCrouch();
	void OnDodge();
	void OnLightAttack();
	void OnHeavyAttack();
	void OnBlockPressed();
	void OnBlockReleased();
	void OnInteract();
	void OnLockOn();
	void OnUseItem();
	void OnCycleItem();
	void OnCompanionCommand();
	void OnJournal();
	void OnMenuTab();
	void OnMenuBack();
	void OnBuildShelter();
	void OnQuickSave();
	void OnQuickLoad();
	void HandleHotkey(int32 Index);
	void OnHotkey1() { HandleHotkey(0); }
	void OnHotkey2() { HandleHotkey(1); }
	void OnHotkey3() { HandleHotkey(2); }
	void OnHotkey4() { HandleHotkey(3); }
	void OnHotkey5() { HandleHotkey(4); }
	void OnHotkey6() { HandleHotkey(5); }
	void OnHotkey7() { HandleHotkey(6); }
	void OnHotkey8() { HandleHotkey(7); }
	void OnHotkey9() { HandleHotkey(8); }

	bool IsInMenu() const;
	bool CanControl() const;
	FVector GetMoveInputWorldDirection() const;

	UFUNCTION() void HandlePerfectDodge();
	UFUNCTION() void HandleDeflect();
	UFUNCTION() void HandleAttackStarted(int32 ComboIndex, bool bPerilous);

	void UpdateLockOn(float DeltaSeconds);
	AActor* FindLockTarget() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRInventoryComponent> Inventory;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRProgressionComponent> Progression;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRCraftingComponent> Crafting;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRSurvivalComponent> Survival;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRStealthComponent> Stealth;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRInteractionComponent> Interaction;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRTraversalComponent> Traversal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UNavigationInvokerComponent> NavInvoker;

private:
	TWeakObjectPtr<AActor> LockTarget;
	TWeakObjectPtr<AHRCompanionCharacter> Companion;

	bool bWantsSprint = false;
	bool bDowned = false;
	double DownedUntil = 0.0;
	TWeakObjectPtr<AActor> DownedBy;
	int32 ResurrectionCharges = 1;
	int32 KillsSinceResurrection = 0;
	bool bResting = false;
	bool bSpiritSight = false;
	double LastGroundedTime = 0.0;
	FTimerHandle DownedTimer;
	FTimerHandle SlowMoTimer;
};
