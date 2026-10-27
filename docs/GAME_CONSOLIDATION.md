# GAME — consolidation plan and decision log (UE 5.8)

Started by ChatGPT, melded with Claude's combined build on 2026-10-27. Both AIs follow this file and `AGENTS.md`.

## Decision log

| Date | Decision (by Riv) | Replaces |
| --- | --- | --- |
| 2026-10-27 | One repo **GAME**, public | "private GAME repo" |
| 2026-10-27 | **Blend both worlds**: Heavy Mettle's space-western frontier over Hollowreach/Veil of Ash's ancient ruins. Hollowreach "magic" = old Hollow tech on Battery. See `docs/WORLD.md` | "Hollowreach fantasy is not canon; no fantasy lore or magic" |
| 2026-10-27 | **Compile Hollowreach's C++ in** as the game's systems; Blueprints built on top as child classes | "C++ is reference only; Blueprint-only runtime" |
| 2026-10-27 | Fresh-start history in GAME; originals preserved verbatim under `Legacy/` with source commit IDs; original repos stay untouched | "git subtree with full histories" |
| 2026-10-27 | Unreal project name **ManOfMettle**, engine **UE 5.8** (5.8.4 hotfix or newer 5.8.x) | Heavy Mettle 5.5.4 / 5.8.3 / Hollowreach 5.5 targets |

## The game

**Heavy Mettle** — a human-looking amnesiac android taking morally complicated jobs in a lived-in alien frontier, slowly discovering the ruins beneath it are where he came from. One campaign, one player, one GameMode, one save chain.

## Source ledger (all preserved)

Exact source commits: `Legacy/SOURCE_COMMITS.txt`. Every original file is kept verbatim under `Legacy/`; the active tree is built from them.

1. **`attempt-two`** → `Legacy/attempt-two/` — primary design and tools. Active copies: `DESIGN.md`, `data/`, `docs/` (creative direction, quests, Blueprint build, asset plans, PS5 and Windows guides), editor scripts in `Scripts/`.
2. **`Heavy-Mettle`** → `Legacy/Heavy-Mettle-55/` — 5.5.4 setup, first-playable shooting lesson (active copy: `docs/FIRST_PLAYABLE_SHOOTING.md`), script variants (active copies in `Scripts/legacy/`), daily AI pipeline (`docs/ai/daily-pipeline-original.md`).
3. **`The-Game`** → `Legacy/The-Game/` — README, docs, design maps, CLAUDE handoff, Gemini audit workflow and the archived Veil of Ash gameplay source (never compiled). Its live code (`Source/`, `Config/`, `Tests/`) is the active project; system docs are copied to `docs/systems/`.

## Layout

```text
GAME/
  ManOfMettle.uproject     # UE 5.8, modules Hollowreach + SharedArt + VeilOfAsh
  Source/                  # C++ systems (compiled) and ManOfMettle(Editor).Target.cs
  Config/
  Content/HeavyMettle/     # Blueprints (children of the C++ classes), maps, rights-cleared assets (LFS)
  Scripts/                 # editor Python, build and check scripts
  Tests/                   # engine-free rule tests
  data/                    # balance and tuning
  docs/                    # WORLD, STATUS, START_HERE, design, systems/, ai/, daily-lessons/
  Legacy/                  # the three original repos, verbatim
  .github/                 # AI instructions, prompts, workflows, templates
  AGENTS.md CLAUDE.md GEMINI.md README.md ROADMAP.md
```

## Bringing in the local ManOfMettle project

Back up `C:\UnrealProjects\ManOfMettle` first. Then follow `docs/START_HERE.md`: copy its `Content/` (and any maps/Blueprints Riv made) into the GAME clone, keep GAME's `ManOfMettle.uproject`, `Config/` and `Source/` (the repo's `.uproject` adds the C++ modules), build, open. Audit whether the local template was **Combat** or **standard** Third Person; Combat-specific instructions don't apply to a standard project, and the C++ already provides combat and enemies.

## Milestones (A–F kept from ChatGPT's plan; 0 added for the C++)

- **0 — It builds:** `ManOfMettleEditor` compiles in UE 5.8; fix errors in small commits; editor opens; runtime graybox plays with `HRGameMode`.
- **A:** move/jump in a real map; checkpoint commit with real `.uasset`/`.umap` via Git LFS; generate the Blueprint layer.
- **B:** run and adapt `Scripts/build_dustfall.py`; play `L_Dustfall` (Cinder's Rest / Old Relay).
- **C:** free-aim pistol and damage test in `BP_HM_Player`, then one hostile enemy.
- **D:** sword melee (C++ combo/deflect), 350 cm line-of-sight lock-on, defensive move.
- **E:** 15-minute *Missing Cells* job with Oren Bale, two moral outcomes, save and reload.
- **F:** art/audio, optimization, Windows packaged build, then console exploration with licensed access.

The C++ already contains crafting, factions, companion, procedural dungeons and full RPG progression. Leave them in place but **don't build on them until the slice runs end to end**, and never add a competing framework.

## Legacy and C++ policy

- The C++ is the one runtime. Fix it for UE 5.8; extend it through Blueprint children and exposed functions.
- Change fiction (display text, art, VFX) to match `docs/WORLD.md`; **never change saved IDs**.
- Don't compile `Legacy/`. Don't add a second GameMode, inventory or save system. Old save import (Hollowreach/Veil slots) stays as written.

## Asset and version control policy

- Track `*.uasset` and `*.umap` with Git LFS; **never ignore them**.
- Ignore `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Saved/`.
- Editor Python is not packaged runtime scripting.
- No API keys, unreleasable SDKs, restricted Fab packages or caches in git.
- CI (`.github/workflows/checks.yml`) checks project/C++ consistency, rule tests, script syntax and secrets. **Green CI is not proof of UE compilation, PIE, packaged launch or platform compatibility.**

## Cutover status

1. ✅ `DepressedRedPanda/GAME` created (public).
2. ✅ All three trees preserved under `Legacy/` with source commit IDs. (Full histories remain in the original repos.)
3. ✅ attempt-two tools/docs/data promoted to active paths; Hollowreach C++ promoted to the active project; overlapping docs resolved by hand.
4. ⬜ Copy the local ManOfMettle `Content/` into GAME; install Git LFS before the first binary commit.
5. ⬜ First successful UE 5.8 editor build and PIE smoke test; record in `docs/STATUS.md`.
6. ⬜ Archive the three source repos only after step 5 and a clean clone/LFS checkout of GAME. Never delete them.
