# Veil of Ash: Hollowreach

One integrated action RPG campaign for **Unreal Engine 5.5**, with **5.5.4** as the recommended build baseline. Open `Hollowreach.uproject`.

The player explores Emberfall, the Memory Reliquary and Ashen Peak in one generated world. Decoding Veil of Ash's cipher/rune gate leads to the Pale Keeper; defeating it unlocks the Hollow Warden and the shared First Flame/Choir Engine finale. Hollowreach's original side quests, investigation, traversal, companion, survival, vehicle and procedural dungeon remain in that world.

**One player, inventory, level, Echoes currency, skill-point pool, world clock, quest journal and versioned save format.** There is no alternative Veil game mode. Original Veil gameplay source is retained under `Archive/VeilOfAsh` for reference. Its small runtime module now preserves only the old save schema for import.

## Status

Source consistency and portable campaign/migration policy tests have passed. **Unreal C++/UHT compilation, Play In Editor, actual save-file import and asset loading have not been run here.** The repository contains source and placeholder geometry, not a certified executable. The engine is unavailable in this workspace.

- [Integration and campaign rules](Docs/INTEGRATION.md)
- [Unified saves and legacy import](Docs/SAVE_MIGRATION.md)
- [Free asset catalog/import plan](Docs/FREE_ASSETS.md)
- [Validation status](Docs/VALIDATION.md)
- [Claude's build/playtest assignment](CLAUDE.md)
- [Original Hollowreach design](Docs/DESIGN.md)

## Getting started

1. Install UE 5.5, preferably 5.5.4, and a supported C++ toolchain.
2. Generate project files for `Hollowreach.uproject`; build `HollowreachEditor`, Development Editor / Win64. On Windows: `Scripts/BuildEditor.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.5'`.
3. Create and save one empty level at `Content/Maps/World`, use `HRGameMode`, and set it as the startup map.
4. Press Play after a successful build. The world is generated at runtime.
5. Run `python Scripts/check_combined.py` for source checks. `Tests/campaign_rules_test.cpp` tests the engine-independent rules. The authored Unreal test is `Hollowreach.Unified.SaveRoundTrip`.

Old test maps with an AshGameMode override must use HRGameMode. No binary map or imported asset pack is fabricated by this change.

## Main route

1. Kindle the Ember Hearth and talk to Elder Maren.
2. Go northeast to the Memory Reliquary (around X=17500, Y=15500 cm). Choose an origin at one of its three memorials if desired.
3. Open the memory gate with **3 cipher seals + 2 lost records**, or solve **SECOND → FIRST → THIRD** and bring **1 cipher + 1 record**.
4. Defeat the Pale Keeper; its death breaks the Warden's ward.
5. Climb Ashen Peak and defeat the Hollow Warden.
6. Return to the First Flame altar. Choose an original Hollowreach outcome or the Freefolk, Choir or Unbound outcome. Faction choices use the same reputation ledger.

Side quests, the Trial dungeon, vehicles, parkour and Kestrel remain available throughout the campaign.

## Controls

| Action | Keyboard / mouse |
| --- | --- |
| Move / look | WASD / mouse |
| Jump / traversal | Space |
| Sprint / crouch | Left Shift / C |
| Dodge / lock-on | Left Ctrl / middle mouse or T |
| Light / heavy / block-deflect | Left mouse / R / right mouse |
| Interact / journal | E / J |
| Abilities | 1–4; Void Pulse and Spectral Form are granted into available slots |
| Quick item / cycle item | Q / X |
| Companion order | G |
| Build shelter | B (5 Wood + 2 Iron Ore) |
| Save / reload current saved map | F5 / F9 |
| Menus: choose / page / tab / close | 1–8 / 9 / Tab / Backspace |

Existing gamepad mappings remain. Class origins and the shared skill tree use game interactions/menus; original F1/F2/F3 class keys and a separate Veil input scheme are retired.

## Saves and art

The canonical slot is `Hollowreach_Unified_Slot0`. Compatible old `Hollowreach_Slot0` and `VeilOfAsh_Save1` files are imported only when the canonical slot is absent. Original files are left untouched. See the migration document for exact mappings and limits.

Shared Free Asset Appearance settings connect imported characters, weapons, props, materials, montages, Niagara cues and sounds to the game. All defaults are empty and fall back to placeholder art. The catalog lists candidates; **no third-party packs have been acquired or imported**.

The unified runtime is a single-player prototype. Veil's old replication code remains in the archive; production co-op requires an authority/replication pass across the unified systems.
