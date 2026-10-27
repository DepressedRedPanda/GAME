#pragma once

#include "CoreMinimal.h"
#include "World/HRInteractableActor.h"
#include "HRPuzzleElement.generated.h"

class AHRPuzzleElement;
class UBoxComponent;

UENUM(BlueprintType)
enum class EHRPuzzleElementType : uint8
{
	/** Toggles 0/1 on use. */
	Lever,
	/** Rotates through NumStates positions on use. */
	Dial,
	/** 1 while something heavy (a pawn or a physics prop) rests on it. */
	PressurePlate,
	/** Only lights with fire: a Firebolt, a firebomb, or a burning barrel. */
	Brazier,
	/** Ancient writing. Readable only with the right knowledge; reading it reveals a hint. */
	Glyph
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHRPuzzleElementSignature, AHRPuzzleElement*, Element, int32, NewState);

/** One piece of an environmental puzzle. A puzzle controller watches a set of these. */
UCLASS()
class HOLLOWREACH_API AHRPuzzleElement : public AHRInteractableActor, public IHRElementalReactive
{
	GENERATED_BODY()

public:
	AHRPuzzleElement();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override;
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;
	virtual void ReactToElement(EHRDamageType Element, AActor* Source) override;

	UFUNCTION(BlueprintCallable, Category="Puzzle") void SetState(int32 NewState);
	UFUNCTION(BlueprintPure, Category="Puzzle") int32 GetState() const { return State; }
	void SetLocked(bool bInLocked) { bLocked = bInLocked; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") EHRPuzzleElementType ElementType = EHRPuzzleElementType::Lever;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") int32 NumStates = 4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle") FName RequiredKnowledge;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Puzzle", meta=(MultiLine=true)) FText GlyphText;

	UPROPERTY(BlueprintAssignable, Category="Puzzle") FHRPuzzleElementSignature OnStateChanged;

protected:
	void UpdateVisuals();

	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Indicator;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UBoxComponent> PlateTrigger;

	int32 State = 0;
	bool bLocked = false;
};
