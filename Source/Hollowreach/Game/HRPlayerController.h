#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/HRTypes.h"
#include "HRPlayerController.generated.h"

class UHRInputSet;
class AHRPlayerCharacter;
class AHRCheckpoint;
class AHRNPCCharacter;

USTRUCT()
struct FHRNotification
{
	GENERATED_BODY()

	FText Text;
	FLinearColor Color = FLinearColor::White;
	double Time = 0.0;
	float Duration = 4.f;
	bool bLarge = false;
};

/** One line of a text menu: rest menu, journal, shop or dialogue. */
USTRUCT()
struct FHRMenuEntry
{
	GENERATED_BODY()

	FText Label;
	FText Detail;
	bool bEnabled = true;
	FName Action;
	FName Id;
	FGuid Guid;
	int32 Payload = 0;
};

UENUM()
enum class EHRRestTab : uint8 { LevelUp, Skills, Craft, Upgrade, Travel, MAX UMETA(Hidden) };

UENUM()
enum class EHRJournalTab : uint8 { Quests, Inventory, Codex, Character, MAX UMETA(Hidden) };

/**
 * Owns the input set, the UI mode and every text menu. Menus are driven by the number keys
 * (1-8 select, 9 next page), Tab switches tabs, Backspace closes. The HUD only draws what
 * BuildMenu returns, so swapping in UMG later only replaces the drawing.
 * Also hosts console cheats for testing (HRGive, HREchoes, HRTime, HRWeather, HRUnlockAll...).
 */
UCLASS()
class HOLLOWREACH_API AHRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHRPlayerController();

	virtual void OnPossess(APawn* InPawn) override;

	UHRInputSet* GetInputSet();
	void EnsureInputMapping();
	AHRPlayerCharacter* GetPlayerCharacter() const { return PlayerCharacter.Get(); }

	// --- UI state ---
	EHRUIMode GetUIMode() const { return UIMode; }
	void SetUIMode(EHRUIMode NewMode);
	void OpenRestMenu(AHRCheckpoint* Checkpoint);
	void OpenShop(AHRNPCCharacter* Merchant);
	void ToggleJournal();
	void ShowEnding(FName EndingId);
	void CloseMenu();
	void NextMenuTab();
	void HandleMenuHotkey(int32 Index);

	/** Everything the HUD needs to draw the current menu. */
	void BuildMenu(FText& OutTitle, TArray<FText>& OutTabs, int32& OutActiveTab, TArray<FHRMenuEntry>& OutEntries, FText& OutFooter) const;
	int32 GetMenuPage() const { return MenuPage; }
	static constexpr int32 EntriesPerPage = 8;

	// --- Notifications ---
	void AddNotification(const FText& Text, const FLinearColor& Color, float Duration, bool bLarge);
	const TArray<FHRNotification>& GetNotifications() const { return Notifications; }

	// --- Console cheats ---
	UFUNCTION(Exec) void HRGive(FName ItemId, int32 Quantity = 1);
	UFUNCTION(Exec) void HREchoes(int32 Amount);
	UFUNCTION(Exec) void HRTime(float Hour);
	UFUNCTION(Exec) void HRWeather(int32 WeatherIndex);
	UFUNCTION(Exec) void HRSkillPoints(int32 Amount);
	UFUNCTION(Exec) void HRUnlockAll();
	UFUNCTION(Exec) void HRKnow(FName KnowledgeId);
	UFUNCTION(Exec) void HRGod();
	UFUNCTION(Exec) void HRSave();
	UFUNCTION(Exec) void HRNewGame();

private:
	void BuildRestEntries(TArray<FHRMenuEntry>& Out) const;
	void BuildJournalEntries(TArray<FHRMenuEntry>& Out) const;
	void BuildShopEntries(TArray<FHRMenuEntry>& Out) const;
	void BuildDialogueEntries(TArray<FHRMenuEntry>& Out, FText& OutFooter) const;
	void ExecuteEntry(const FHRMenuEntry& Entry);
	void GetAllEntries(TArray<FHRMenuEntry>& Out, FText& OutFooter) const;

	UPROPERTY() TObjectPtr<UHRInputSet> InputSet;

	TWeakObjectPtr<AHRPlayerCharacter> PlayerCharacter;
	TWeakObjectPtr<AHRCheckpoint> RestCheckpoint;
	TWeakObjectPtr<AHRNPCCharacter> ShopNPC;

	EHRUIMode UIMode = EHRUIMode::Gameplay;
	EHRRestTab RestTab = EHRRestTab::LevelUp;
	EHRJournalTab JournalTab = EHRJournalTab::Quests;
	int32 MenuPage = 0;
	bool bShopSellMode = false;
	FName ShownEnding;
	bool bGodMode = false;

	TArray<FHRNotification> Notifications;
};
