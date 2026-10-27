#include "Core/HRStatics.h"

#include "Characters/HRPlayerCharacter.h"
#include "Game/HRPlayerController.h"
#include "Kismet/GameplayStatics.h"

AHRPlayerCharacter* UHRStatics::GetHRPlayer(const UObject* WorldContextObject)
{
	if (AHRPlayerController* PC = GetHRPlayerController(WorldContextObject))
	{
		if (AHRPlayerCharacter* Player = Cast<AHRPlayerCharacter>(PC->GetPawn()))
		{
			return Player;
		}
		// While driving a vehicle the controller possesses the vehicle; the character is still the player.
		return PC->GetPlayerCharacter();
	}
	return nullptr;
}

AHRPlayerController* UHRStatics::GetHRPlayerController(const UObject* WorldContextObject)
{
	return Cast<AHRPlayerController>(UGameplayStatics::GetPlayerController(WorldContextObject, 0));
}

void UHRStatics::Notify(const UObject* WorldContextObject, const FText& Message, const FLinearColor& Color, bool bLarge)
{
	if (AHRPlayerController* PC = GetHRPlayerController(WorldContextObject))
	{
		PC->AddNotification(Message, Color, bLarge ? 6.f : 4.f, bLarge);
	}
}

void UHRStatics::K2_Notify(const UObject* WorldContextObject, const FText& Message, FLinearColor Color, bool bLarge)
{
	Notify(WorldContextObject, Message, Color, bLarge);
}

int32 UHRStatics::HashName(FName Name, int32 Seed)
{
	return static_cast<int32>(HashCombine(GetTypeHash(Name.ToString()), GetTypeHash(Seed)) & 0x7fffffff);
}
