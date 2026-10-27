#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HRHUD.generated.h"

class AHRPlayerCharacter;
class AHRPlayerController;
class UFont;

/**
 * Canvas HUD so everything is visible with zero UI assets: vitals, posture, survival, abilities,
 * quest tracker, compass-free exploration, enemy awareness markers, perilous warnings, boss bars,
 * notifications and all text menus. Replace with UMG widgets reading the same data later.
 */
UCLASS()
class HOLLOWREACH_API AHRHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawVitals(const AHRPlayerCharacter* Player);
	void DrawPosture(const AHRPlayerCharacter* Player);
	void DrawSurvival(const AHRPlayerCharacter* Player);
	void DrawAbilities(const AHRPlayerCharacter* Player);
	void DrawWorldInfo(const AHRPlayerCharacter* Player);
	void DrawPrompts(const AHRPlayerCharacter* Player);
	void DrawWorldMarkers(const AHRPlayerCharacter* Player);
	void DrawBossBar(const AHRPlayerCharacter* Player);
	void DrawCompanion(const AHRPlayerCharacter* Player);
	void DrawNotifications(const AHRPlayerController* PC);
	void DrawMenu(const AHRPlayerController* PC);
	void DrawCenterMessage(const FString& Text, const FLinearColor& Color, float Scale, float YFraction);

	void Bar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color, const FLinearColor& Background = FLinearColor(0.f, 0.f, 0.f, 0.6f));
	void Text(const FString& String, float X, float Y, const FLinearColor& Color, float Scale = 1.f, UFont* Font = nullptr);
	float TextWidth(const FString& String, float Scale = 1.f, UFont* Font = nullptr);
};
