# Canonical save and migration

## One write path

The runtime uses UHRSaveGame schema version 2 and one canonical slot: Hollowreach_Unified_Slot0. F5, shrine rest, imported-save finalization and final-ending selection write through HRSaveSubsystem. F9 reloads the saved current map and reads that same slot. The original Veil save writer is archived.

The canonical schema contains shared level/currency/skill points, attributes, origin, skills, abilities, inventory/equipment, vitals, injuries, survival/infection, knowledge, clues, quests, faction reputations, boss/gate/rune/pickup flags, ending, world day/weather, checkpoint/player transform, shelters and companion order/hold/revive state. Resources persist depletion time through stable location/item IDs and narrative flags.

This is campaign/player/world progression persistence. It does not yet serialize every loose physics prop, moving platform, living enemy position, vehicle pose or temporary effect.

## First-load precedence

1. If the canonical slot exists, read only it; never import the old files again.
2. Otherwise read compatible Hollowreach_Slot0 and VeilOfAsh_Save1 files.
3. Keep Hollowreach's richer world/quest/inventory state as the starting point when present, then map Veil's fields.
4. Finalize the imported result into the canonical slot after the player's startup relocation. Original files are not written or deleted.
5. An unreadable old file, unreadable canonical file or unsupported newer schema blocks unified writes rather than silently hiding old progress.

Real .sav reading, engine-header compatibility and disk behavior remain untested here. The import code retains the original Veil save class path and field types, but that is not a runtime compatibility guarantee.

## Field mappings

| Legacy field | Canonical treatment |
| --- | --- |
| Level / skill points | Larger nonnegative value when both sources exist; Veil-only points retained exactly |
| Remaining Veil XP | Added once to the Echoes pool, saturating at the integer limit |
| Might / Guile / Insight | Strength / Dexterity / Arcane become max(existing, 10 + old branch points) |
| HeroClass | Vanguard / Seer / Shade origin when no origin is already selected |
| Wood / Ore / Herb | Item.Wood / Item.IronOre / Item.Herb |
| Relic / Cipher / Knowledge | Item.AncientAlloy / Item.VeilCipher / Item.VeilRecord |
| Ration / Tonic | Item.VeilRation / Item.VeilTonic |
| Overlapping stack counts | Larger count rather than summing both histories |
| Unknown items | Preserved in LegacyUnmappedItems metadata for a later explicit mapping |
| WeaponTier | Existing Iron Blade upgrade, clamped to its supported cap |
| Freefolk / Choir reputation | Corresponding entries in the shared faction ledger |
| Hunger / thirst / infection / kills | Shared survival and persistent kill count |
| Veil-only health | Safe relative health fraction; dead legacy snapshots resume alive |
| Old Veil coordinates | Resume at the new Memory Shrine; old arena coordinates do not identify the new world |
| Old Hollowreach ending | Kept as historical metadata; the combined finale requires the newly added Pale Keeper chapter |
| Completed old main quest | Reopened at VeilGate; reward marker prevents paying its original completion reward twice |

Old Veil saves did not record gate/rune/boss/world-pickup/companion state. Those facts cannot be reconstructed from a character-only file and are not invented. Imported owned cipher/record counts suppress corresponding new pickups to prevent duplication.

## New Game

HRNewGame writes a fresh-start marker to the canonical slot, resets live narrative/quests/reputation, and reloads the current saved map. The marker prevents untouched old slots from re-importing after the reset. On write failure the current session is retained. New Game needs a saved map for reload.

## Verification required in Unreal

Test legacy fixtures for Hollowreach only, Veil only, both, neither, unreadable data and a future schema. Compare original file bytes before/after. Load twice and confirm no repeated XP/items/rewards. Save/reload half-completed runes, each collected pickup, both boss flags, shelters, resource depletion, origin/skills, vitals, companion state and endings. Run Hollowreach.Unified.SaveRoundTrip and actual disk/gameplay tests; memory serialization alone is insufficient.
