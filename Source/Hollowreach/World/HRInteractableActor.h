#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/HRInteractable.h"
#include "HRInteractableActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Base for placed interactables. Gives a greybox mesh, a tint, and persistence helpers:
 * state is stored as narrative flags named "<PersistentId>.<Suffix>" so it survives saves.
 */
UCLASS(Abstract)
class HOLLOWREACH_API AHRInteractableActor : public AActor, public IHRInteractable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") FName SharedAppearanceKey;
	AHRInteractableActor();

	virtual void BeginPlay() override;

	virtual bool CanInteract(const AHRPlayerCharacter* Player) const override { return true; }
	virtual FText GetInteractionPrompt(const AHRPlayerCharacter* Player) const override { return Prompt; }
	virtual void Interact(AHRPlayerCharacter* Player) override {}

	UFUNCTION(BlueprintCallable, Category="Interactable") void SetColor(FLinearColor InColor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interactable") FName PersistentId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interactable") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interactable") FText Prompt;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interactable") FLinearColor Color = FLinearColor(0.5f, 0.5f, 0.5f);

	UStaticMeshComponent* GetMesh() const { return Mesh; }

protected:
	FName MakeFlag(const TCHAR* Suffix) const;
	int32 GetPersistentFlag(const TCHAR* Suffix) const;
	void SetPersistentFlag(const TCHAR* Suffix, int32 Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components") TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
};
