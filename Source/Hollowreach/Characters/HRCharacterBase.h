#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/HRTypes.h"
#include "HRCharacterBase.generated.h"

class UHRAttributeComponent;
class UHRCombatComponent;
class UHRAbilityComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Shared base for every combatant: player, enemies, bosses, companions and NPCs.
 * Owns attributes, combat and abilities, and a greybox body (capsule + head + blade) that
 * animates procedurally from combat state so the game reads well before real art exists.
 */
UCLASS(Abstract)
class HOLLOWREACH_API AHRCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AHRCharacterBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;

	UFUNCTION(BlueprintPure, Category="Character") UHRAttributeComponent* GetAttributes() const { return Attributes; }
	UFUNCTION(BlueprintPure, Category="Character") UHRCombatComponent* GetCombat() const { return Combat; }
	UFUNCTION(BlueprintPure, Category="Character") UHRAbilityComponent* GetAbilities() const { return Abilities; }
	UFUNCTION(BlueprintPure, Category="Character") bool IsDead() const { return bIsDead; }
	UFUNCTION(BlueprintPure, Category="Character") FText GetDisplayName() const { return DisplayName; }

	/** How aware this character is of Other. Non-AI characters are always fully aware. */
	virtual EHRAwareness GetAwarenessTowards(const AActor* Other) const { return EHRAwareness::Alerted; }
	/** Weapon damage bonus from an attribute (players scale with stats; NPCs don't). */
	virtual float GetAttributeScaling(EHRAttribute Attribute) const { return 0.f; }

	virtual void Die(AActor* Killer);
	virtual void ReviveCharacter(float HealthFraction);

	// --- Greybox visuals ---
	UFUNCTION(BlueprintCallable, Category="Character|Visual") void SetVisualColor(FLinearColor Color);
	UFUNCTION(BlueprintCallable, Category="Character|Visual") void ResetVisualColor();
	UFUNCTION(BlueprintCallable, Category="Character|Visual") void SetVisualScale(float Scale);
	UFUNCTION(BlueprintCallable, Category="Character|Visual") void SetGreyboxVisible(bool bVisible);
	void RefreshWeaponAppearance(FName ItemId);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character") FName FactionId = TEXT("Neutral");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character|Visual") FLinearColor BodyColor = FLinearColor(0.6f, 0.6f, 0.6f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character|Visual") bool bUseGreyboxVisuals = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character|Visual") FName SharedAppearanceRole = TEXT("Enemy");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character|Movement") float BaseWalkSpeed = 420.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character|Movement") float SprintMultiplier = 1.65f;
	/** AI uses this to creep while suspicious. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character|Movement") float MovementSpeedScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character|Movement") float FallDamageThreshold = 1150.f;

	UFUNCTION(BlueprintImplementableEvent, Category="Character", meta=(DisplayName="On Death"))
	void K2_OnDeath(AActor* Killer);

protected:
	UFUNCTION() virtual void HandleHealthDepleted(UHRAttributeComponent* InAttributes, AActor* Killer);
	UFUNCTION() virtual void HandleCombatStateChanged(EHRCombatState OldState, EHRCombatState NewState);

	virtual float GetMovementMultiplier() const;
	virtual bool WantsToSprint() const { return false; }
	void UpdateMovementSpeed();
	void UpdateGreyboxAnimation(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRAttributeComponent> Attributes;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRCombatComponent> Combat;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UHRAbilityComponent> Abilities;

	UPROPERTY(VisibleAnywhere, Category="Components|Greybox") TObjectPtr<USceneComponent> VisualRoot;
	UPROPERTY(VisibleAnywhere, Category="Components|Greybox") TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, Category="Components|Greybox") TObjectPtr<UStaticMeshComponent> HeadMesh;
	UPROPERTY(VisibleAnywhere, Category="Components|Greybox") TObjectPtr<USceneComponent> WeaponPivot;
	UPROPERTY(VisibleAnywhere, Category="Components|Greybox") TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	bool bIsDead = false;
	float LastFallingSpeed = 0.f;
	FRotator WeaponRotation = FRotator::ZeroRotator;
};
