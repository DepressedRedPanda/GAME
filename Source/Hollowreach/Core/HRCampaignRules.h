#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

// Gameplay/migration policy independent of Unreal so it can be tested without the editor.
namespace HRCampaignRules
{
    inline bool CanOpenMemoryGate(int Ciphers, int Records, bool RuneSolved)
    {
        return (Ciphers >= 3 && Records >= 2) || (RuneSolved && Ciphers >= 1 && Records >= 1);
    }

    inline int AdvanceRune(int Progress, int Rune)
    {
        constexpr int Sequence[] = { 2, 1, 3 };
        if (Progress >= 3) return 3;
        Progress = std::clamp(Progress, 0, 2);
        return Rune == Sequence[Progress] ? Progress + 1 : 0;
    }

    inline bool CanResolveCampaign(bool PaleKeeperDefeated, bool WardenDefeated)
    {
        return PaleKeeperDefeated && WardenDefeated;
    }

    inline int MergePersistentCount(int Primary, int Legacy)
    {
        return std::max({0, Primary, Legacy});
    }

    inline int ImportRemainingExperience(int Echoes, int RemainingXP)
    {
        const std::int64_t Total = static_cast<std::int64_t>(std::max(0, Echoes)) + std::max(0, RemainingXP);
        return static_cast<int>(std::min(Total, static_cast<std::int64_t>(std::numeric_limits<int>::max())));
    }

    inline int ImportBranchAttribute(int Primary, int Branch)
    {
        return std::max(std::clamp(Primary, 10, 1000), 10 + std::clamp(Branch, 0, 990));
    }

    inline bool ShouldImportLegacy(bool UnifiedSlotExists)
    {
        return !UnifiedSlotExists;
    }
}
