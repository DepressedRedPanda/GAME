#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/HRItemTypes.h"
#include "HRPuzzleController.generated.h"

class AHRPuzzleElement;
class AHRGate;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHRPuzzleSolvedSignature);

/**
 * Watches puzzle elements and solves when every element matches its required state.
 * In sequence mode, elements must reach their required state in array order; a mistake resets
 * them (Simon-says / rhythm puzzles). Solving opens gates and pays out rewards, permanently.
 */
UCLASS()
class HOLLOWREACH_API AHRPuzzleController : public AActor
{
	GENERATED_BODY()

public:
	AHRPuzzleController();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="Puzzle") bool IsSolved() const { return bSolved; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") FName PuzzleId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") TArray<TObjectPtr<AHRPuzzleElement>> Elements;
	/** Parallel to Elements. Elements beyond this array's length are ignored (e.g. hint glyphs). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") TArray<int32> RequiredStates;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") bool bOrderedSequence = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") TArray<TObjectPtr<AHRGate>> GatesToOpen;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") int32 RewardEchoes = 300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") FName RewardKnowledge;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") FText SolvedText;

	UPROPERTY(BlueprintAssignable, Category="Puzzle") FHRPuzzleSolvedSignature OnSolved;

protected:
	UFUNCTION() void HandleElementChanged(AHRPuzzleElement* Element, int32 NewState);
	void Solve(bool bFromSave);
	FName GetSolvedFlag() const;

	bool bSolved = false;
	int32 SequenceProgress = 0;
	bool bResetting = false;
};
