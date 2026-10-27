#pragma once

#include "CoreMinimal.h"
#include "World/HRInteractableActor.h"
#include "Core/HRTypes.h"
#include "HRDiscoverables.generated.h"

class UBoxComponent;

UENUM(BlueprintType)
enum class EHRCollectibleType : uint8
{
	LoreFragment,
	SkillRelic,
	VigorSeed,
	EchoCache,
	TraversalRelic
};

/**
 * Hidden collectibles. Some only appear under Spirit Sight or when you're right next to them,
 * rewarding players who explore every corner without a map marker.
 */
UCLASS()
class HOLLOWREACH_API AHRCollectible : public AHRInteractableActor
{
	GENERATED_BODY()

public:
	AHRCollectible();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override;
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collectible") EHRCollectibleType Type = EHRCollectibleType::LoreFragment;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collectible") FName KnowledgeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collectible") int32 Amount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collectible") EHRTraversalAbility Traversal = EHRTraversalAbility::DoubleJump;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collectible") bool bHiddenUntilRevealed = false;

private:
	bool IsRevealed() const;
	bool bRevealed = true;
	float BaseZ = 0.f;
};

/** Investigation clue. Examine it to note a clue; collect enough to deduce the truth. */
UCLASS()
class HOLLOWREACH_API AHRClue : public AHRInteractableActor
{
	GENERATED_BODY()

public:
	AHRClue();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override;
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override;
	virtual void Interact(AHRPlayerCharacter* Player) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clue") FName ClueId;
	/** Only visible with Spirit Sight (detective vision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clue") bool bRequiresSpiritSight = false;

private:
	bool IsExamined() const;
};

/** A named place. Walking into it the first time is a discovery: echoes and sometimes knowledge. */
UCLASS()
class HOLLOWREACH_API AHRLandmark : public AActor
{
	GENERATED_BODY()

public:
	AHRLandmark();

	virtual void BeginPlay() override;

	UFUNCTION() void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void SetExtent(const FVector& Extent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark") FName LandmarkId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark") int32 EchoReward = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark") FName KnowledgeId;
	/** Optional quest that begins when this place is first found. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Landmark") FName StartsQuest;

protected:
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UBoxComponent> Trigger;
};
