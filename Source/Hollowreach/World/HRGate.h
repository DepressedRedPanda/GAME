#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HRGate.generated.h"

class UStaticMeshComponent;

/** Door, portcullis or boss fog wall that slides open and closed. */
UCLASS()
class HOLLOWREACH_API AHRGate : public AActor
{
	GENERATED_BODY()

public:
	AHRGate();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="Gate") void Open();
	UFUNCTION(BlueprintCallable, Category="Gate") void Close();
	UFUNCTION(BlueprintPure, Category="Gate") bool IsOpen() const { return bOpen; }

	void SetGateSize(const FVector& SizeCm);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gate") bool bStartsOpen = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gate") float OpenSpeed = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gate") FLinearColor Color = FLinearColor(0.3f, 0.3f, 0.32f);
	/** Fog gates vanish when open instead of sliding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gate") bool bIsFogGate = false;

protected:
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category="Components") TObjectPtr<UStaticMeshComponent> Door;

	bool bOpen = false;
	float Height = 400.f;
	float Offset = 0.f;
};
