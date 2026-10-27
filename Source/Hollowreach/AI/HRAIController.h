#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Core/HRTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "HRAIController.generated.h"

class AHRCharacterBase;
class AHREnemyCharacter;
class UHRAttributeComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRAwarenessChangedSignature, EHRAwareness, OldAwareness, EHRAwareness, NewAwareness);

/**
 * Hand-written AI (no behaviour-tree assets needed) built around a stealth-game awareness meter:
 *
 *   Unaware   -> patrols or wanders.
 *   Suspicious-> stops, looks at the disturbance, creeps toward it.
 *   Searching -> goes to the last known position and sweeps the area.
 *   Alerted   -> fights: approaches, circles when it lacks an attack token, attacks in combos,
 *                blocks/deflects/dodges based on skill, uses perilous attacks, alerts allies.
 *
 * Sight gain scales with the target's stealth visibility, distance, weather and darkness.
 * Movement falls back to direct steering when no navmesh path exists.
 */
UCLASS()
class HOLLOWREACH_API AHRAIController : public AAIController
{
	GENERATED_BODY()

public:
	AHRAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category="AI") EHRAwareness GetAwareness() const { return Awareness; }
	UFUNCTION(BlueprintPure, Category="AI") float GetAwarenessValue() const { return AwarenessValue; }
	UFUNCTION(BlueprintPure, Category="AI") AActor* GetTargetActor() const { return Target.Get(); }

	/** Forces combat against a target (ally shouts, being hit, hordes). */
	void AlertTo(AActor* InTarget);
	/** The target "died" (feigned): drop to searching. */
	void LoseTrackOf(AActor* Actor);
	void ResetAI();
	void HandlePawnDeath();

	UPROPERTY(BlueprintAssignable, Category="AI") FHRAwarenessChangedSignature OnAwarenessChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI") float AwarenessGainRate = 65.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI") float AwarenessDecayRate = 9.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI") float LoseTargetTime = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI") float SearchDuration = 14.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI") float AllyAlertRadius = 1600.f;

protected:
	UFUNCTION() void HandlePerception(AActor* Actor, FAIStimulus Stimulus);
	UFUNCTION() void HandlePawnDamaged(UHRAttributeComponent* Attributes, const FHRDamageInfo& Damage);

	/** Override for different brains (companions). */
	virtual void TickBehavior(float DeltaSeconds);

	void TickAwareness(float DeltaSeconds);
	void TickPatrol(float DeltaSeconds);
	void TickSuspicious(float DeltaSeconds);
	void TickSearch(float DeltaSeconds);
	void TickCombat(float DeltaSeconds);
	void TickDirectSteering();

	void SetAwareness(EHRAwareness NewAwareness);
	void AlertAllies();
	void MoveToPoint(const FVector& Destination, float Acceptance);
	void StopAllMovement();
	void ReleaseToken();
	void SetCombatFacing(bool bFacingTarget);
	bool IsValidHostile(const AActor* Actor) const;
	bool IsTargetIncapacitated() const;

	AHRCharacterBase* GetHRPawn() const;
	AHREnemyCharacter* GetEnemyPawn() const;
	double Now() const;

	UPROPERTY(VisibleAnywhere, Category="AI") TObjectPtr<UAIPerceptionComponent> Perception;
	UPROPERTY() TObjectPtr<UAISenseConfig_Sight> SightConfig;
	UPROPERTY() TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	EHRAwareness Awareness = EHRAwareness::Unaware;
	float AwarenessValue = 0.f;
	TWeakObjectPtr<AActor> Target;
	TArray<TWeakObjectPtr<AActor>> VisibleActors;
	FVector LastKnownLocation = FVector::ZeroVector;
	double LastSeenTime = -100.0;
	double StateStartTime = 0.0;

	// Patrol
	int32 PatrolIndex = 0;
	double NextPatrolMoveTime = 0.0;
	FVector PatrolDestination = FVector::ZeroVector;

	// Combat
	bool bHasToken = false;
	double TokenAcquiredTime = 0.0;
	double NextDecisionTime = 0.0;
	double NextAttackTime = 0.0;
	double NextReactionTime = 0.0;
	double NextRangedTime = 0.0;
	int32 PendingComboHits = 0;
	float StrafeSign = 1.f;
	FTimerHandle BlockReleaseTimer;

	// Movement fallback
	bool bDirectSteering = false;
	FVector SteerDestination = FVector::ZeroVector;
	float SteerAcceptance = 50.f;
};
