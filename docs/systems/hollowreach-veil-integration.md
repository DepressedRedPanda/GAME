# One campaign: Veil of Ash / Hollowreach

## Runtime architecture

There is one HRGameMode, AHRPlayerCharacter, controller, inventory, progression component, survival component, world clock, narrative/faction ledger and save subsystem. Hollowreach is the primary module. SharedArt supplies optional cosmetics. The remaining VeilOfAsh module exists only to deserialize /Script/VeilOfAsh.AshSave; it contains no GameMode, pawn, inventory or progression implementation.

The old Veil gameplay source is preserved in Archive/VeilOfAsh. It is not compiled or selectable. This preserves the earlier work without running duplicate systems.

## Connected campaign

Quest.Embers is now: Kindle → Elder → VeilGate → PaleKeeper → Warden → Altar. The Memory Reliquary is a region of the same generated world, around (17500,15500,0) cm. No travel to another campaign is used.

The gate preserves both original routes: 3 ciphers + 2 records, or a solved 2→1→3 rune sequence + 1 cipher + 1 record. Rune progress and pickup flags are persistent. The Pale Keeper cannot attack or receive normal combat damage until the gate opens. Boss death guards also prevent fall/direct death callbacks from recording an early victory. The Warden requires the Pale Keeper's defeated flag.

The final altar and ending resolver require both bosses. The five original Hollowreach outcomes remain; three explicit choices carry Veil's Freefolk, Choir and Unbound outcomes into that same finale. Faction trust is measured in the shared reputation ledger, and each envoy's pledge is recorded once.

## Progression and mechanics

| Veil system | Unified implementation |
| --- | --- |
| XP and automatic levels | Remaining XP imports into Echoes; one shared level and shrine-based attribute progression |
| Might / Guile / Insight | Legacy points import into Strength / Dexterity / Arcane; future tiered nodes use the existing Blade / Shadow / Arcane trees and shared skill points |
| Vanguard / Seer / Shade | One persisted origin choice on HRProgressionComponent; permanent modifier source, no alternative pawn |
| Void pulse | Granted ability using shared focus/cooldown/combat damage and faction filtering |
| Spectral form | Granted toggle using shared focus upkeep and stealth stats |
| Hunger / thirst / injury | Existing survival and attribute/injury systems |
| Infection | Added to shared survival, ashfall exposure, tonic treatment, resting and saves |
| Shelter | Built checkpoint costing 5 Wood + 2 Iron Ore, saved with a stable checkpoint ID and transform |
| Follow / Guard / Hunt companion | Kestrel's Follow restores focus, Hold supports healing, Aggressive uses its existing combat AI; orders/revives persist |
| Numeric blade tier | Imports into the existing Iron Blade's capped upgrade level |
| Weather / shrine / death | Existing world clock, checkpoint and resurrection systems |
| Transport / farming analogues | Existing hover vehicle and renewable resource nodes |
| Replicated prototype | Archived source; the merged runtime still needs a multiplayer authority pass |

These are deliberate rule mappings, not a claim that both old balance curves can remain unchanged. One progression model replaces conflicting implementations.

## Shared art

Project Settings > Game > Shared Free Asset Appearance has optional references for character roles Player, Enemy, Boss, PaleKeeper, Companion and NPC. Weapon models use Weapon.<item ID> and Weapon.Default; the chosen character profile supplies the actual hand socket. Montages use <role>.Light0/Light1/Light2/Heavy. Attack-step montages take precedence. A matching skeleton and an authored montage slot/Animation Blueprint are still required.

Props preserve existing hidden collision/physics geometry. Resources use Resource.<canonical item suffix>, such as Resource.Herb, Resource.Wood and Resource.IronOre. Veil chapter nodes use Veil.Node.Cipher, Veil.Node.Lore, Veil.Node.Rune, etc. Shelters use Shelter, solid gates Gate, and Kestrel remains a skeletal character role.

Terrain material keys are Ground, Stone, Wood, Snow, Ruin, Ember, Foliage, Water, Dungeon.Floor and Dungeon.Wall. Detailed dungeon/tree placement still requires level or generator authoring; these material slots do not replace all cube geometry automatically.

Cue keys include Combat.Hit, Combat.Deflect, Physics.Explosion, Checkpoint.Discover, Projectile.<damage type>.Impact, Ability.VoidPulse and Ability.SpectralForm. No imported art/animation graphs or persistent VFX loops are included. Empty settings retain placeholders.

## Engine and source history

UE 5.5 / recommended 5.5.4 matches the larger Hollowreach codebase. Veil was source-only, so no saved 5.8 art packages need downgrading. Source compatibility still requires a real build. Older asset packages can generally migrate forward; newer-only packages cannot generally open in older editors.

Original source refs: Veil main 816c45466adb80985f4b62f94fbd899fd5fb98a4; Claude Hollowreach 107f539531e0b6f6c6a4b3c8774c27aa2b8bb037. The earlier two-mode integration commit is retained in Git history. The current PR supersedes its architecture.

See SAVE_MIGRATION.md and VALIDATION.md for concrete behavior and remaining runtime checks.
