#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HRStatics.generated.h"

class AHRPlayerCharacter;
class AHRPlayerController;

/** Fetches a GameInstance subsystem from any world context. */
template <typename T>
T* HRGetSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject || !GEngine)
	{
		return nullptr;
	}
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<T>();
		}
	}
	return nullptr;
}

UCLASS()
class HOLLOWREACH_API UHRStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Hollowreach", meta=(WorldContext="WorldContextObject"))
	static AHRPlayerCharacter* GetHRPlayer(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Hollowreach", meta=(WorldContext="WorldContextObject"))
	static AHRPlayerController* GetHRPlayerController(const UObject* WorldContextObject);

	/** Pushes a line onto the HUD notification feed. */
	static void Notify(const UObject* WorldContextObject, const FText& Message, const FLinearColor& Color = FLinearColor::White, bool bLarge = false);

	UFUNCTION(BlueprintCallable, Category="Hollowreach", meta=(WorldContext="WorldContextObject", DisplayName="Notify"))
	static void K2_Notify(const UObject* WorldContextObject, const FText& Message, FLinearColor Color, bool bLarge);

	/** Deterministic hash of a name, used for seeded per-actor randomness. */
	static int32 HashName(FName Name, int32 Seed);
};
