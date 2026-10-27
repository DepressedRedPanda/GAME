#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/HRTypes.h"
#include "HRFactionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRReputationChangedSignature, FName, FactionId, float, NewReputation);

/**
 * Reputation with each faction (-100..100) and static stances between factions. Attitude drives
 * AI hostility, merchant prices and dialogue options. Helping one faction ripples to its allies
 * and rivals. Attacking someone who wasn't hostile costs you with their faction.
 */
UCLASS()
class HOLLOWREACH_API UHRFactionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category="Factions") float GetReputation(FName FactionId) const;
	UFUNCTION(BlueprintCallable, Category="Factions") void ModifyReputation(FName FactionId, float Delta, bool bRipple = true, bool bNotify = true);
	UFUNCTION(BlueprintCallable, Category="Factions") void SetReputation(FName FactionId, float Value);
	UFUNCTION(BlueprintPure, Category="Factions") EHRAttitude GetAttitudeBetween(FName A, FName B) const;
	UFUNCTION(BlueprintPure, Category="Factions") static FText GetRankText(float InReputation);

	static FName GetFactionOf(const AActor* Actor);
	static EHRAttitude GetAttitude(const AActor* A, const AActor* B);
	static bool AreHostile(const AActor* A, const AActor* B) { return GetAttitude(A, B) == EHRAttitude::Hostile; }
	static bool AreFriendly(const AActor* A, const AActor* B) { return GetAttitude(A, B) == EHRAttitude::Friendly; }
	/** Called whenever someone strikes someone else; the player pays for attacking non-hostiles. */
	static void NotifyAttack(const AActor* Attacker, const AActor* Victim);

	const TMap<FName, float>& GetAllReputation() const { return Reputation; }
	void RestoreReputation(const TMap<FName, float>& InReputation);

	UPROPERTY(BlueprintAssignable, Category="Factions") FHRReputationChangedSignature OnReputationChanged;

	static const FName PlayerFaction;

private:
	UPROPERTY() TMap<FName, float> Reputation;
};
