#pragma once

#include "CoreMinimal.h"
#include "Characters/HRCharacterBase.h"
#include "Core/HRInteractable.h"
#include "HRCompanionCharacter.generated.h"

class AHRPlayerCharacter;

UENUM(BlueprintType)
enum class EHRCompanionCommand : uint8
{
	Follow,
	Hold,
	Aggressive,
	Passive
};

/**
 * Recruitable ally. Fights beside you, holds positions, sneaks when you sneak, and can drag
 * you back up when you fall. Approval (changed by your dialogue choices) decides whether
 * they stay. Knocked out instead of killed; they get back up after a while or when helped.
 */
UCLASS()
class HOLLOWREACH_API AHRCompanionCharacter : public AHRCharacterBase, public IHRInteractable
{
	GENERATED_BODY()

public:
	AHRCompanionCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	int32 GetRevivesRemaining() const { return RevivesRemaining; }

	// IHRInteractable
	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override;
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	void Recruit(AHRPlayerCharacter* Player, bool bRestoreFromSave = false);
	void Dismiss();
	void CycleCommand();
	void ModifyApproval(int32 Delta);

	UFUNCTION(BlueprintPure, Category="Companion") bool IsRecruited() const { return bRecruited; }
	UFUNCTION(BlueprintPure, Category="Companion") bool IsKnockedOut() const { return bKnockedOut; }
	UFUNCTION(BlueprintPure, Category="Companion") EHRCompanionCommand GetCommand() const { return Command; }
	UFUNCTION(BlueprintPure, Category="Companion") int32 GetApproval() const;
	UFUNCTION(BlueprintPure, Category="Companion") bool CanRevivePlayer() const { return RevivesRemaining > 0; }
	AHRPlayerCharacter* GetLeader() const { return Leader.Get(); }
	FVector GetHoldLocation() const { return HoldLocation; }
	void ConsumeRevive() { RevivesRemaining = FMath::Max(0, RevivesRemaining - 1); }
	void RefreshRevives() { RevivesRemaining = MaxRevives; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Companion") FName CompanionId = TEXT("Companion.Kestrel");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Companion") FName DialogueId = TEXT("Dialogue.Kestrel");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Companion") int32 MaxRevives = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Companion") int32 LeaveBelowApproval = -40;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Companion") float KnockoutDuration = 20.f;

protected:
	virtual void HandleHealthDepleted(UHRAttributeComponent* InAttributes, AActor* Killer) override;
	void WakeUp(float HealthFraction);

	TWeakObjectPtr<AHRPlayerCharacter> Leader;
	bool bRecruited = false;
	bool bKnockedOut = false;
	EHRCompanionCommand Command = EHRCompanionCommand::Follow;
	FVector HoldLocation;
	int32 RevivesRemaining = 1;
	FTimerHandle KnockoutTimer;
	float AssistTimer = 0.f;
};
