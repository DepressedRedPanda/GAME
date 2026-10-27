#include "AshInventory.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UAshInventory::UAshInventory() { SetIsReplicatedByDefault(true); }
void UAshInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(UAshInventory, Items);
    DOREPLIFETIME(UAshInventory, WeaponTier);
}
int32 UAshInventory::Count(FName Item) const
{
    for (const FAshStack& Stack : Items) if (Stack.Item == Item) return Stack.Count;
    return 0;
}
bool UAshInventory::Add(FName Item, int32 Amount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Item.IsNone() || Amount <= 0) return false;
    for (FAshStack& Stack : Items) if (Stack.Item == Item) { Stack.Count += Amount; return true; }
    Items.Add(FAshStack(Item, Amount)); return true;
}
bool UAshInventory::Consume(FName Item, int32 Amount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0 || Count(Item) < Amount) return false;
    for (int32 i=0; i<Items.Num(); ++i) if (Items[i].Item == Item)
    {
        Items[i].Count -= Amount;
        if (Items[i].Count == 0) Items.RemoveAt(i);
        return true;
    }
    return false;
}
bool UAshInventory::Craft(FName Recipe)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    if (Recipe == "Ration" && Count("Herb") >= 2 && Count("Wood") >= 1)
    {
        Consume("Herb", 2); Consume("Wood", 1); Add("Ration", 1); return true;
    }
    if (Recipe == "Tonic" && Count("Herb") >= 3)
    {
        Consume("Herb", 3); Add("Tonic", 1); return true;
    }
    if (Recipe == "Blade" && Count("Ore") >= 3 && Count("Relic") >= 1 && WeaponTier < 5)
    {
        Consume("Ore", 3); Consume("Relic", 1); ++WeaponTier; return true;
    }
    return false;
}
FString UAshInventory::Describe() const
{
    FString Result;
    for (const FAshStack& Stack : Items)
    {
        if (!Result.IsEmpty()) Result += TEXT("  |  ");
        Result += FString::Printf(TEXT("%s:%d"), *Stack.Item.ToString(), Stack.Count);
    }
    return Result.IsEmpty() ? TEXT("Empty") : Result;
}
