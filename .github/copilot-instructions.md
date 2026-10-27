# GitHub Copilot — GAME / Heavy Mettle

Follow [AGENTS.md](../AGENTS.md) and [docs/GAME_CONSOLIDATION.md](../docs/GAME_CONSOLIDATION.md). Riv's 2026-10-27 decisions replace the older Blueprint-only notes (kept in `Legacy/attempt-two/.github/copilot-instructions.md`).

- Unreal Engine 5.8, project `ManOfMettle.uproject`. C++ systems in `Source/Hollowreach` (prefixes `AHR`/`UHR`, `FName` content IDs); gameplay additions as Blueprint children in `Content/HeavyMettle`.
- Space-western frontier over ancient Hollow tech (`docs/WORLD.md`). No spells — abilities run on Battery.
- Melee lock-on max 350 cm with line of sight; beyond that, free third-person aiming.
- Light/heavy combos, block/parry, pistol and sword body sockets, Chaos Cloth duster after gameplay works.
- `data/balance.json` is the tuning reference; update it when gameplay numbers change.
- Never rename saved IDs. Never add a second GameMode, inventory or save system.
- Commit `.uasset`/`.umap` via Git LFS; check Fab licenses; never commit secrets or console SDKs.
- PS5 lead design platform, Windows first; platform-neutral Enhanced Input, gamepad-first UI.

**PR review:** check each PR is one small playable change, lists three PIE tests and a `docs/daily-progress.md` line, and states plainly what was not compiled or tested in Unreal.
**Coding agent:** one issue per draft PR; list what wasn't tested.

Riv is a beginner — plain-English comments and PR descriptions.
