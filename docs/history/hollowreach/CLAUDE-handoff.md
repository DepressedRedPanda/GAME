# Claude — full integration assignment

Riv clarified the goal: **one campaign, progression and save system, with no separate modes**. Continue from `integration/unified-ue55-free-assets` / PR #1. The two-mode approach has been replaced; do not restore it.

Open `Hollowreach.uproject`, UE 5.5 (recommended 5.5.4), build HollowreachEditor. Read README, Docs/INTEGRATION.md, Docs/SAVE_MIGRATION.md, Docs/FREE_ASSETS.md and Docs/VALIDATION.md.

## Current implementation

Hollowreach supplies the one player/controller, inventory, Echoes progression, skills, survival, narrative, factions, world clock and save writer. The main Quest.Embers now connects the memory gate → Pale Keeper → Warden → shared altar. Veil origins, Might/Guile/Insight skill nodes, Void Pulse, Spectral Form, infection, shelter building, envoy choices and its three endings are ported into those systems.

Source/VeilOfAsh contains only the legacy save schema and module glue. Archived gameplay is reference material, not a playable mode. SharedArt is still optional; no imported packs are present.

## Work to perform in Unreal

1. Run the combined source checker and portable C++ policy tests. Then run a real UE 5.5.4 UHT/editor-target build; fix observed errors with small, reviewable changes. Source checks do not prove engine compatibility.
2. Create one saved empty World map with HRGameMode. Test a fresh campaign without assets: gate routes, rune reset/resume, shielded bosses, both boss deaths, all original/new endings and side quests.
3. Run the authored automation test Hollowreach.Unified.SaveRoundTrip. Also test real disk fixtures for old Hollowreach-only, Veil-only and both saves. Verify source files remain unchanged, conversion writes the canonical slot once, repeat loads do not add XP/items, and malformed/newer saves block writes.
4. Test save/reload after picking up each cipher/record, entering two runes, opening the gate, killing each boss, choosing an origin, buying skills, harvesting/regrowth, building/resting at a shelter, commanding Kestrel and spending its revive. Verify health/focus/stamina, infection, day/calendar, player transform and endings survive.
5. Test HRNewGame clears live subsystem state and writes a fresh-start marker so old slots cannot be re-imported. Verify a failed reset does not reload a fresh world or erase current progress.
6. Import one free character (Greystone/Kwang) and one weapon first. Configure Player, Boss and PaleKeeper roles separately if desired. Match skeletons, hand sockets, montage slots and gameplay timing. Do not add damage from animation notifies on top of existing sweeps/timers.
7. Add Medieval Dungeon/Soul: Cave and foliage/effects incrementally. Record actual package versions, license, object paths and tests. Do not overwrite the player/GameMode with a downloaded framework. All asset packages must be suitable for 5.5 or migrated from older versions.
8. Update Docs/VALIDATION.md and the existing draft PR with actual evidence. Do not merge, force-push or change engine versions casually.

ChatGPT implemented this source integration and can review your commits in the shared PR. This handoff does not launch a Claude session or prove an Unreal build. Report compiled, PIE, standalone, import, multiplayer and cooking results separately.

No paid asset is required. Keep original source/save references available; preserve the single runtime/save architecture when fixing compatibility errors.
