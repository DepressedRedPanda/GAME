#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/HRTypes.h"
#include "Items/HRItemTypes.h"
#include "HRCombatComponent.generated.h"

class ACharacter;
class UHRAttributeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRCombatStateChangedSignature, EHRCombatState, OldState, EHRCombatState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRAttackStartedSignature, int32, ComboIndex, bool, bPerilous);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHRHitLandedSignature, AActor*, Target, EHRHitResponse, Response, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRHitReceivedSignature, AActor*, Attacker, EHRHitResponse, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHRCombatEventSignature);

enum class EHRAttackPhase : uint8 { None, Windup, Active, Recovery };

/**
 * Melee combat for players, enemies, companions and bosses.
 *
 *  Offense: light combo chains, heavy attacks, perilous (unblockable) attacks, lunges, soft lock.
 *  Rhythm:  pressing the next attack late in the recovery of the previous one ("on the beat")
 *           builds Flow stacks (+damage). Mashing early resets Flow.
 *  Defense: block (stamina + posture), deflect (block pressed inside the parry window) which
 *           punishes the attacker's posture and opens a riposte window, dodge with i-frames;
 *           a dodge right as a hit lands is a perfect dodge.
 *  Posture: filling an enemy's posture breaks it; the next hit is a deathblow.
 *  Stealth: takedowns on unaware or unsuspecting enemies from behind or above.
 *  Tactics: backstab, high-ground and flanking multipliers; attack tokens limit how many
 *           enemies may swing at one target at once.
 */
UCLASS(ClassGroup=(Hollowreach), meta=(BlueprintSpawnableComponent))
class HOLLOWREACH_API UHRCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHRCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Actions ---
	UFUNCTION(BlueprintCallable, Category="Combat") bool RequestLightAttack();
	UFUNCTION(BlueprintCallable, Category="Combat") bool RequestHeavyAttack();
	/** Starts an arbitrary attack (bosses and AI use their own move sets). */
	UFUNCTION(BlueprintCallable, Category="Combat") bool PerformAttack(const FHRAttackStep& Step, bool bHeavy);
	UFUNCTION(BlueprintCallable, Category="Combat") void StartBlock();
	UFUNCTION(BlueprintCallable, Category="Combat") void StopBlock();
	UFUNCTION(BlueprintCallable, Category="Combat") bool RequestDodge(FVector WorldDirection);
	UFUNCTION(BlueprintCallable, Category="Combat") bool TryStealthTakedown();
	/** Locks the owner in a non-combat action (drinking, casting). */
	UFUNCTION(BlueprintCallable, Category="Combat") bool BeginBusy(float Duration);
	UFUNCTION(BlueprintCallable, Category="Combat") void GrantInvulnerability(float Duration);
	UFUNCTION(BlueprintCallable, Category="Combat") void CancelActions();

	// --- Receiving ---
	UFUNCTION(BlueprintCallable, Category="Combat") EHRHitResponse ReceiveHit(const FHRDamageInfo& Damage);
	/** Called on the attacker when its blow was deflected. */
	void NotifyDeflectedBy(AActor* Defender, const FHRDamageInfo& Damage);

	UFUNCTION(BlueprintCallable, Category="Combat") void EnterStagger(float Duration);
	UFUNCTION(BlueprintCallable, Category="Combat") void EnterPostureBroken();
	UFUNCTION(BlueprintCallable, Category="Combat") void SetDead();
	UFUNCTION(BlueprintCallable, Category="Combat") void SetDowned(bool bDowned);
	UFUNCTION(BlueprintCallable, Category="Combat") void ResetCombatState();

	// --- Queries ---
	UFUNCTION(BlueprintPure, Category="Combat") EHRCombatState GetState() const { return State; }
	UFUNCTION(BlueprintPure, Category="Combat") bool CanAct() const;
	UFUNCTION(BlueprintPure, Category="Combat") bool IsAttacking() const { return State == EHRCombatState::Attacking; }
	UFUNCTION(BlueprintPure, Category="Combat") bool IsBlocking() const { return State == EHRCombatState::Blocking; }
	UFUNCTION(BlueprintPure, Category="Combat") bool IsInvulnerable() const { return bIFrames; }
	UFUNCTION(BlueprintPure, Category="Combat") bool IsCurrentAttackPerilous() const { return IsAttacking() && CurrentStep.bUnblockable; }
	UFUNCTION(BlueprintPure, Category="Combat") bool IsInRiposteWindow() const;
	UFUNCTION(BlueprintPure, Category="Combat") float GetAttackProgress() const;
	UFUNCTION(BlueprintPure, Category="Combat") bool IsHeavyAttack() const { return bCurrentHeavy; }
	UFUNCTION(BlueprintPure, Category="Combat") int32 GetComboIndex() const { return ComboIndex; }
	UFUNCTION(BlueprintPure, Category="Combat") int32 GetFlowStacks() const { return FlowStacks; }
	UFUNCTION(BlueprintPure, Category="Combat") float GetParryWindow() const;
	UFUNCTION(BlueprintPure, Category="Combat") AActor* FindTakedownTarget() const;
	bool IsInRecoveryPhase() const { return Phase == EHRAttackPhase::Recovery; }

	UFUNCTION(BlueprintCallable, Category="Combat") void SetCurrentTarget(AActor* Target) { CurrentTarget = Target; }
	UFUNCTION(BlueprintPure, Category="Combat") AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	// --- Weapon ---
	UFUNCTION(BlueprintCallable, Category="Combat") void SetWeapon(const FHRWeaponData& InWeapon, int32 UpgradeLevel);
	UFUNCTION(BlueprintCallable, Category="Combat") void ResetWeaponToDefault();
	const FHRWeaponData& GetWeapon() const { return Weapon; }

	// --- Attack tokens (combat director) ---
	bool RequestAttackToken(AActor* Requester);
	void ReleaseAttackToken(AActor* Requester);

	// --- Tuning ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") FHRWeaponData DefaultWeapon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float DodgeSpeed = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float DodgeStaminaCost = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float DodgeDuration = 0.45f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float IFrameDuration = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float PerfectDodgeWindow = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float BlockStaminaMultiplier = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float StaggerDuration = 0.45f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float GuardBreakDuration = 1.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float PostureBrokenDuration = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float RiposteWindow = 1.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float ComboResetTime = 0.6f;
	/** Fraction of max health a deathblow removes. Bosses with several phases still need one per phase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float DeathblowHealthFraction = 1.f;
	/** Hits below this damage do not stagger (poise). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float PoiseThreshold = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") bool bHyperArmorOnHeavy = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") bool bRecoilWhenDeflected = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") bool bCanBeTakenDown = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float TakedownRange = 230.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") int32 MaxAttackTokens = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float OutgoingDamageMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Tuning") float PhysicsImpulse = 60000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Debug") bool bDebugDraw = false;

	UPROPERTY(BlueprintAssignable, Category="Combat") FHRCombatStateChangedSignature OnStateChanged;
	UPROPERTY(BlueprintAssignable, Category="Combat") FHRAttackStartedSignature OnAttackStarted;
	UPROPERTY(BlueprintAssignable, Category="Combat") FHRHitLandedSignature OnHitLanded;
	UPROPERTY(BlueprintAssignable, Category="Combat") FHRHitReceivedSignature OnHitReceived;
	UPROPERTY(BlueprintAssignable, Category="Combat") FHRCombatEventSignature OnDeflectSuccess;
	UPROPERTY(BlueprintAssignable, Category="Combat") FHRCombatEventSignature OnPerfectDodge;
	UPROPERTY(BlueprintAssignable, Category="Combat") FHRCombatEventSignature OnPostureBroken;
	UPROPERTY(BlueprintAssignable, Category="Combat") FHRCombatEventSignature OnTakedown;

private:
	void SetState(EHRCombatState NewState);
	void BeginAttackStep(const FHRAttackStep& Step, bool bHeavy);
	void HandleAttackActive();
	void HandleAttackRecovery();
	void HandleAttackFinished();
	void PerformHitSweep();
	FHRDamageInfo BuildDamage(AActor* Target) const;
	float ComputePositionalMultiplier(const AActor* Target, bool& bOutBackstab) const;
	void FaceTarget();
	void ClearTimers();
	void EndStateTimer();

	UFUNCTION() void HandlePostureBroken(UHRAttributeComponent* Attributes);

	ACharacter* GetOwnerCharacter() const;
	UHRAttributeComponent* GetAttributes() const;
	double Now() const;

	EHRCombatState State = EHRCombatState::Idle;
	EHRAttackPhase Phase = EHRAttackPhase::None;

	UPROPERTY(Transient) FHRWeaponData Weapon;
	float WeaponUpgradeMultiplier = 1.f;

	FHRAttackStep CurrentStep;
	bool bCurrentHeavy = false;
	int32 ComboIndex = 0;
	double AttackStartTime = 0.0;
	double RecoveryStartTime = 0.0;
	double LastAttackEndTime = -100.0;
	bool bBufferedLight = false;
	bool bBufferedHeavy = false;
	int32 FlowStacks = 0;

	double BlockStartTime = -100.0;
	double RiposteUntil = -100.0;
	double DodgeStartTime = -100.0;
	bool bIFrames = false;
	bool bBlockHeld = false;

	TArray<TWeakObjectPtr<AActor>> HitThisSwing;
	TArray<TWeakObjectPtr<AActor>> TokenHolders;
	TWeakObjectPtr<AActor> CurrentTarget;

	FTimerHandle PhaseTimer;
	FTimerHandle StateTimer;
	FTimerHandle IFrameTimer;
};
