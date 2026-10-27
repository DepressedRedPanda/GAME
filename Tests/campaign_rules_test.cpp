#include "Core/HRCampaignRules.h"
#include <cstdlib>
#include <iostream>

namespace
{
    int Checks = 0;
    void Check(bool Passed, const char* Description)
    {
        ++Checks;
        if (!Passed) { std::cerr << "FAIL: " << Description << '\n'; std::exit(1); }
    }
}

int main()
{
    using namespace HRCampaignRules;
    // Play the complete scavenging route; neither item type alone can open the boss gate.
    for (int Ciphers = 0; Ciphers <= 4; ++Ciphers)
        for (int Records = 0; Records <= 3; ++Records)
            Check(CanOpenMemoryGate(Ciphers, Records, false) == (Ciphers >= 3 && Records >= 2), "scavenging route boundary");

    int RuneProgress = 0;
    RuneProgress = AdvanceRune(RuneProgress, 2);
    Check(RuneProgress == 1, "second rune begins sequence");
    RuneProgress = AdvanceRune(RuneProgress, 3);
    Check(RuneProgress == 0, "wrong second press resets sequence");
    RuneProgress = AdvanceRune(RuneProgress, 2);
    RuneProgress = AdvanceRune(RuneProgress, 1);
    RuneProgress = AdvanceRune(RuneProgress, 3);
    Check(RuneProgress == 3, "2,1,3 solves sequence");
    Check(AdvanceRune(RuneProgress, 1) == 3, "solved circuit remains solved");
    Check(AdvanceRune(2, 3) == 3, "a restored two-step circuit finishes with the third rune");
    Check(!CanOpenMemoryGate(0, 1, true), "solved rune route still needs a cipher");
    Check(!CanOpenMemoryGate(1, 0, true), "solved rune route still needs a record");
    Check(CanOpenMemoryGate(1, 1, true), "knowledge route opens with one of each");
    Check(!CanOpenMemoryGate(1, 1, false), "one of each without knowledge stays locked");
    Check(!CanOpenMemoryGate(-1, 2, true), "invalid negative count never unlocks");

    // An imported old Warden victory does not skip the newly integrated chapter.
    Check(!CanResolveCampaign(false, true), "old Warden save must finish Pale Keeper chapter");
    Check(!CanResolveCampaign(true, false), "Pale Keeper alone cannot finish campaign");
    Check(CanResolveCampaign(true, true), "both chapters enable final outcome");

    // Import distinct progression sources once; overlap never doubles levels, points or items.
    Check(MergePersistentCount(12, 8) == 12, "higher main level preserved");
    Check(MergePersistentCount(4, 9) == 9, "higher legacy level preserved");
    Check(MergePersistentCount(5, 5) == 5, "same inventory history not doubled");
    Check(MergePersistentCount(-3, -1) == 0, "negative progress repaired");
    Check(ImportRemainingExperience(250, 40) == 290, "remaining XP enters one echo pool");
    Check(ImportRemainingExperience(2147483640, 100) == 2147483647, "currency import saturates safely");
    Check(ImportBranchAttribute(12, 5) == 15, "Might imports into shared Strength baseline");
    Check(ImportBranchAttribute(25, 3) == 25, "existing stronger attribute retained");
    Check(ShouldImportLegacy(false), "first session can import old slots");
    const bool UnifiedSlotWritten = true;
    Check(!ShouldImportLegacy(UnifiedSlotWritten), "subsequent load cannot reimport XP or inventory");
    std::cout << Checks << " campaign/migration policy checks passed\n";
}
