#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Kismet/GameplayStatics.h"
#include "Systems/HRSaveGame.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHRUnifiedSaveRoundTrip, "Hollowreach.Unified.SaveRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHRUnifiedSaveRoundTrip::RunTest(const FString& Parameters)
{
    UHRSaveGame* Save = NewObject<UHRSaveGame>();
    Save->Level = 12;
    Save->Echoes = 290;
    Save->StartingClass = EHRStartingClass::Shade;
    Save->Infection = 63.f;
    Save->WorldDay = 7;
    Save->bHasVitals = true;
    Save->HealthFraction = 0.45f;
    Save->Attributes.Add(EHRAttribute::Strength, 15);
    Save->Skills.Add(TEXT("Skill.Veil.Might1"));
    FHRItemInstance Cipher;
    Cipher.InstanceId = FGuid::NewGuid();
    Cipher.ItemId = TEXT("Item.VeilCipher");
    Cipher.Quantity = 3;
    Save->Items.Add(Cipher);
    Save->bHasPlayerTransform = true;
    Save->PlayerTransform.SetLocation(FVector(17500.f, 15500.f, 180.f));
    Save->Flags.Add(TEXT("Veil.RuneProgress"), 2);
    Save->Flags.Add(TEXT("Veil.GateOpen"), 1);
    Save->Flags.Add(TEXT("Boss.PaleKeeper.Defeated"), 1);
    Save->Flags.Add(TEXT("Veil.Record1.Collected"), 1);
    Save->bHasCompanionState = true;
    Save->CompanionCommand = 1;
    Save->CompanionRevives = 0;
    FHRPlacedShelter Shelter;
    Shelter.CheckpointId = TEXT("Shelter.Test");
    Shelter.Transform.SetLocation(FVector(100.f, 200.f, 72.f));
    Save->Shelters.Add(Shelter);
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Serialize canonical save"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
    UHRSaveGame* Loaded = Cast<UHRSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Deserialize canonical save"), Loaded)) return false;
    TestEqual(TEXT("Schema version"), Loaded->SaveVersion, UHRSaveGame::CurrentVersion);
    TestEqual(TEXT("One level"), Loaded->Level, 12);
    TestEqual(TEXT("One echo pool"), Loaded->Echoes, 290);
    if (TestEqual(TEXT("Shared inventory count"), Loaded->Items.Num(), 1)) TestEqual(TEXT("Cipher stack"), Loaded->Items[0].Quantity, 3);
    TestEqual(TEXT("Shared Strength"), Loaded->Attributes.FindRef(EHRAttribute::Strength), 15);
    TestTrue(TEXT("Shared skill tree"), Loaded->Skills.Contains(TEXT("Skill.Veil.Might1")));
    TestEqual(TEXT("Health fraction"), Loaded->HealthFraction, 0.45f);
    TestTrue(TEXT("Origin preserved"), Loaded->StartingClass == EHRStartingClass::Shade);
    TestEqual(TEXT("Infection"), Loaded->Infection, 63.f);
    TestEqual(TEXT("Blood moon calendar"), Loaded->WorldDay, 7);
    TestEqual(TEXT("Rune progress"), Loaded->Flags.FindRef(TEXT("Veil.RuneProgress")), 2);
    TestEqual(TEXT("Keeper victory"), Loaded->Flags.FindRef(TEXT("Boss.PaleKeeper.Defeated")), 1);
    TestEqual(TEXT("Collected record"), Loaded->Flags.FindRef(TEXT("Veil.Record1.Collected")), 1);
    TestEqual(TEXT("Built shelter"), Loaded->Shelters.Num(), 1);
    TestTrue(TEXT("Player transform"), Loaded->PlayerTransform.GetLocation().Equals(Save->PlayerTransform.GetLocation()));
    TestEqual(TEXT("Companion command"), Loaded->CompanionCommand, 1);
    TestEqual(TEXT("Used companion revive remains used"), Loaded->CompanionRevives, 0);
    return true;
}
#endif
