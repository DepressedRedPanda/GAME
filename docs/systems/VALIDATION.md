# Full campaign integration validation — 2026-10-12

| Check | Result | Evidence / limit |
| --- | --- | --- |
| Runtime architecture/source consistency | PASS | Three build modules, one primary game module, one configured GameMode; Veil module limited to old save schema. Target linkage/dependency/header/type/brace checks passed: 64 .cpp, 68 .h, 126 reflected types. |
| Portable campaign/migration policy tests | PASS | g++ C++17, -Wall -Wextra -Werror; 43 checks for both gate routes, rune reset/resume, both-boss finale, conservative progression/count mapping, overflow and one-time import selection. |
| Original source retention | PASS | Original Hollowreach source retained; former Veil gameplay preserved under Archive/VeilOfAsh. Duplicate legacy pawn/progression/GameMode no longer compiled. |
| Patch whitespace / Python syntax | PASS | git diff --check and Python script compilation. |
| Unreal UHT/C++ editor build | NOT RUN | Unreal Editor/UnrealBuildTool unavailable in this workspace. |
| Unreal save round-trip automation | AUTHORED, NOT RUN | Hollowreach.Unified.SaveRoundTrip covers canonical data serialization through actual Unreal APIs. |
| Actual legacy disk migration | NOT RUN | Requires real old .sav fixtures and the engine; policy checks alone do not verify deserialization or writes. |
| PIE campaign / save-reload gameplay | NOT RUN | Requires a built editor target and saved World map. |
| Imported art / performance / cooking | NOT RUN | 18 asset candidates; no third-party packs imported. |
| Multiplayer | NOT IMPLEMENTED / NOT RUN | Legacy replication preserved as archived source; unified runtime needs an authority pass. |

The source checker is heuristic, not a C++ parser or engine build. Portable tests exercise the actual engine-independent rule functions used by production code, not the whole Unreal game. No claim is made that the combined game currently compiles or runs.

## Claude's acceptance pass

Build the UE 5.5.4 editor target, run automation, and complete the fresh route: Ember Hearth → Elder → Memory Gate → Pale Keeper → Warden → final altar. Test both gate routes, early boss damage/attacks, fall/direct-death locks, all five original endings and three Veil outcomes, one-time envoy pledges and one-time origin selection.

Test disk import for each legacy combination and repeat-load idempotence. Verify startup relocation occurs before conversion is saved. Validate unsupported data blocks writes and New Game cannot re-import old slots.

Save/reload after records/ciphers, partial rune entry, gate opening, each boss, origin/skills, resource harvesting/regrowth, shelter building/resting, Kestrel command/revive consumption, infection, vitals and world calendar. Verify F5/F9 reload and no duplicate quest rewards. Then integrate one free character/weapon and a small environment, checking skeletons, montage slots, collision, frame rate and cooking.
