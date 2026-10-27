#include "Procedural/HRWorldBootstrap.h"
#include "Characters/HRBossCharacter.h"
#include "Components/HRCombatComponent.h"
#include "Engine/World.h"
#include "World/HRCheckpoint.h"
#include "World/HRGate.h"
#include "World/HRResourceNode.h"
#include "World/HRVeilNode.h"

namespace
{
    template <typename T, typename F>
    T* VeilSpawn(UWorld* World, const FVector& Place, F&& Configure)
    {
        const FTransform Transform(FRotator::ZeroRotator, Place);
        T* Actor = World->SpawnActorDeferred<T>(T::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Actor) { Configure(Actor); Actor->FinishSpawning(Transform); }
        return Actor;
    }
}

void AHRWorldBootstrap::BuildVeilChapter()
{
    UWorld* World = GetWorld();
    const FVector C(17500.f, 15500.f, 0.f);
    // A traversable chapter of the existing world: no level travel or alternate player/GameMode.
    Block(C + FVector(-2000.f, 3100.f, 225.f), FVector(80.f, 2800.f, 450.f), EHRPalette::Ruin);
    Block(C + FVector(2000.f, 3100.f, 225.f), FVector(80.f, 2800.f, 450.f), EHRPalette::Ruin);
    Block(C + FVector(0.f, 4500.f, 225.f), FVector(4080.f, 80.f, 450.f), EHRPalette::Ruin);
    Block(C + FVector(-1200.f, 1700.f, 225.f), FVector(1600.f, 80.f, 450.f), EHRPalette::Ruin);
    Block(C + FVector(1200.f, 1700.f, 225.f), FVector(1600.f, 80.f, 450.f), EHRPalette::Ruin);
    VeilSpawn<AHRCheckpoint>(World, C + FVector(0.f, -2000.f, 0.f), [](AHRCheckpoint* Shrine)
    {
        Shrine->PersistentId = TEXT("Shrine.Veil");
        Shrine->DisplayName = NSLOCTEXT("Veil", "Shrine", "Memory Shrine");
        Shrine->bHasForge = true;
        Shrine->bStartsDiscovered = true;
    });
    AHRGate* Gate = VeilSpawn<AHRGate>(World, C + FVector(0.f, 1700.f, 0.f), [](AHRGate* Door) { Door->SetGateSize(FVector(800.f, 50.f, 450.f)); });
    auto Node = [&](EHRVeilNodeKind Kind, FName Id, const TCHAR* Label, const FVector& Offset, int32 Rune = 1)
    {
        return VeilSpawn<AHRVeilNode>(World, C + Offset, [Kind, Id, Label, Rune, Gate](AHRVeilNode* Actor)
        {
            Actor->Kind = Kind;
            Actor->PersistentId = Id;
            Actor->DisplayName = FText::FromString(Label);
            Actor->RuneNumber = Rune;
            Actor->LinkedGate = Kind == EHRVeilNodeKind::Gate ? Gate : nullptr;
            Actor->SharedAppearanceKey = Kind == EHRVeilNodeKind::Record ? FName(TEXT("Veil.Node.Lore")) : FName(*(FString(TEXT("Veil.Node.")) + StaticEnum<EHRVeilNodeKind>()->GetNameStringByValue(static_cast<int64>(Kind))));
        });
    };
    Node(EHRVeilNodeKind::Cipher, TEXT("Veil.Cipher1"), TEXT("Cipher Seal"), FVector(-1400.f, -1000.f, 60.f));
    Node(EHRVeilNodeKind::Cipher, TEXT("Veil.Cipher2"), TEXT("Cipher Seal"), FVector(1400.f, -400.f, 60.f));
    Node(EHRVeilNodeKind::Cipher, TEXT("Veil.Cipher3"), TEXT("Cipher Seal"), FVector(-1400.f, 900.f, 60.f));
    Node(EHRVeilNodeKind::Record, TEXT("Veil.Record1"), TEXT("Lost Record"), FVector(-500.f, -2200.f, 60.f));
    Node(EHRVeilNodeKind::Record, TEXT("Veil.Record2"), TEXT("Lost Record"), FVector(1800.f, -1300.f, 60.f));
    for (int32 Index = 1; Index <= 3; ++Index) Node(EHRVeilNodeKind::Rune, FName(*FString::Printf(TEXT("Veil.Rune%d"), Index)), TEXT("Memory Rune"), FVector((Index - 2) * 600.f, 500.f, 60.f), Index);
    Node(EHRVeilNodeKind::Gate, TEXT("Veil.GateControl"), TEXT("Memory Gate"), FVector(0.f, 1250.f, 60.f));
    Node(EHRVeilNodeKind::FreefolkEnvoy, TEXT("Veil.FreefolkEnvoy"), TEXT("Freefolk Envoy"), FVector(-1000.f, -1700.f, 60.f));
    Node(EHRVeilNodeKind::ChoirEnvoy, TEXT("Veil.ChoirEnvoy"), TEXT("Choir Envoy"), FVector(1000.f, -1700.f, 60.f));
    Node(EHRVeilNodeKind::Vanguard, TEXT("Veil.OriginVanguard"), TEXT("Vanguard Origin"), FVector(-450.f, -2600.f, 60.f));
    Node(EHRVeilNodeKind::Seer, TEXT("Veil.OriginSeer"), TEXT("Seer Origin"), FVector(0.f, -2600.f, 60.f));
    Node(EHRVeilNodeKind::Shade, TEXT("Veil.OriginShade"), TEXT("Shade Origin"), FVector(450.f, -2600.f, 60.f));

    VeilSpawn<AHRResourceNode>(World, C + FVector(-1500.f, -2500.f, 40.f), [](AHRResourceNode* Resource)
    {
        Resource->ItemId = TEXT("Item.AncientAlloy");
        Resource->DisplayName = NSLOCTEXT("Veil", "Alloy", "Ancient Alloy Seam");
        Resource->RespawnHours = 48.f;
    });
    VeilSpawn<AHRBossCharacter>(World, C + FVector(0.f, 3500.f, 150.f), [Gate](AHRBossCharacter* Boss)
    {
        Boss->BossId = TEXT("Boss.PaleKeeper");
        Boss->SharedAppearanceRole = TEXT("PaleKeeper");
        Boss->EnemyTypeId = TEXT("Enemy.PaleKeeper");
        Boss->DisplayName = NSLOCTEXT("Veil", "Keeper", "THE PALE KEEPER");
        Boss->RequiredCampaignFlag = TEXT("Veil.GateOpen");
        Boss->RewardItem = TEXT("Item.AncientAlloy");
        Boss->SkillPointReward = 1;
        Boss->EchoReward = 325;
        Boss->ArenaGates.Add(Gate);
        Boss->Hints = { NSLOCTEXT("Veil", "KeeperHint", "Decode the memory gate. His ward and the Warden's oath are parts of the same binding.") };
        Boss->GetCombat()->DefaultWeapon.BaseDamage = 28.f;
    });
}
