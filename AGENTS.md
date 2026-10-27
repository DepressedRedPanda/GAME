# GAME / Heavy Mettle — shared rules for every AI agent

ChatGPT/Codex, Claude, Gemini and GitHub Copilot all read this file. Role notes live in `CLAUDE.md`, `GEMINI.md` and `.github/copilot-instructions.md`; if they disagree with this file, this file wins. The full plan and decision log is `docs/GAME_CONSOLIDATION.md`.

Before changing gameplay, read: `docs/WORLD.md` (setting), `docs/CREATIVE_DIRECTION.md`, `docs/QUESTS.md`, `docs/BLUEPRINT_BUILD.md`, `data/balance.json`, `docs/STATUS.md`.

## Who you're working for

Riv is a **complete beginner**, on Windows. Explain every step click-by-click: exact menus, buttons, node and variable names, default values. Never assume prior Unreal or C++ knowledge.

## Decisions Riv made (2026-10-27) — these override older notes

1. **One game, blended world.** Heavy Mettle's space-western frontier (Cinder's Rest, the android, mercenary jobs) sits on top of Hollowreach/Veil of Ash's ancient ruins. Hollowreach's "magic" is old **Hollow tech** powered by **Battery** — no spells, no gods. Names and story: `docs/WORLD.md`.
2. **Hollowreach's C++ is compiled in** as the game's systems (`Source/Hollowreach`, plus `SharedArt` and the save-only `VeilOfAsh` module). It is no longer reference-only.
3. **Blueprints on top.** New gameplay is built as Blueprint children of the C++ classes in `Content/HeavyMettle` (`BP_HM_Player` → `AHRPlayerCharacter`, etc.) so Riv can see and edit it.
4. Repo **GAME** is public. Unreal project is **ManOfMettle** (`ManOfMettle.uproject`), **Unreal Engine 5.8** (5.8.4 or newer 5.8.x hotfix).

## Project invariants

- One human-looking android protagonist. One campaign, one player controller, one GameMode (`HRGameMode` or a Blueprint child of it), one save/progression chain (`HRSaveSubsystem`).
- Touch C++ only to fix build errors, expose things to Blueprint (`UFUNCTION(BlueprintCallable)`, `UPROPERTY(EditAnywhere, BlueprintReadWrite)`), or when Blueprint truly can't do it — and say why.
- **Never rename saved IDs** (`Quest.Embers`, `Item.IronBlade` …). Rename display text only.
- `Legacy/` holds the three original repos verbatim (commits in `Legacy/SOURCE_COMMITS.txt`). Read-only reference; never compile `Legacy/The-Game/Archive/VeilOfAsh` or restore a second game mode.
- Preserve Riv's existing local Unreal project. Never overwrite its `Content/`; copy it into the repo deliberately (see `docs/START_HERE.md`).
- Editor Python may generate maps and scaffolds; text alone does not make playable assets.
- Commit real `.uasset`/`.umap` through Git LFS. Respect Fab licenses. Never commit secrets or console SDKs.

## How work happens

1. **One small, reversible, playable change at a time** on its own branch → pull request. One agent owns a branch; a *different* AI reviews it. No two agents editing the same Blueprint at once.
2. Each change comes with **three Play-In-Editor tests** (expected result for each), a commit message, and a line in `docs/daily-progress.md`.
3. Report status as **design / scripted / generated / compiled / PIE-tested / packaged** — separately. Only tick `docs/STATUS.md` after Riv has played it. Never claim something works that wasn't run.
4. Before finishing: `python Scripts/check_combined.py` and the rule test (`g++ -std=c++17 -ISource/Hollowreach Tests/campaign_rules_test.cpp -o crt && ./crt`). CI runs both. **Green CI is not proof of an Unreal build, PIE or packaging.**
5. Evidence over AI agreement.

## Scope

Current target: the **15-minute *Missing Cells* playable slice** (`ROADMAP.md`). The C++ already contains crafting, factions, companion, procedural dungeon and more — leave them **switched on but untouched** until the slice runs end to end. Don't add new systems, regions or quests unless Riv asks.

## Roles

| AI | Owns |
| --- | --- |
| **ChatGPT** | Integration planner, player experience and scope owner; builds the daily beginner lesson |
| **Claude** | Implementation review (architecture, naming, bugs, ≤3 refactors), UE 5.8 build fixes, narrowly scoped implementation; writes the combined daily lesson |
| **Gemini** | Game feel, balance numbers, free-asset picks, second-opinion design/compatibility and independent test ideas |
| **GitHub Copilot** | In-editor help, boilerplate, PR checklist review, small assigned issues |

Workflow: GitHub Issue with acceptance criteria → one narrow branch → review by a different AI → Riv tests in PIE → merge. Details and automation: `docs/ai/AI_WORKFLOW.md`. Unreal 5.8's experimental MCP integration may be tried in a sandbox with tight permissions only.

## Platforms

Windows first, controller-friendly via Enhanced Input. PS5 is the lead design platform but console builds need Sony/Epic approval and a source-built engine; no platform-specific APIs in gameplay code (`docs/START_HERE_PS5_MULTIPLATFORM.md`).
