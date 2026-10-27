#pragma once

#include "CoreMinimal.h"
#include "AI/HRAIController.h"
#include "HRCompanionController.generated.h"

class AHRCompanionCharacter;

/** Companion brain: follow/hold, defend the leader, sneak when they sneak, revive them when down. */
UCLASS()
class HOLLOWREACH_API AHRCompanionController : public AHRAIController
{
	GENERATED_BODY()

protected:
	virtual void TickBehavior(float DeltaSeconds) override;

private:
	AHRCompanionCharacter* GetCompanion() const;
	AActor* FindThreat(const AActor* Leader, float Radius) const;

	double ReviveStartTime = -1.0;
};
