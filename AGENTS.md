# AGENTS.md — rules every AI follows in this repo

ChatGPT/Codex, Claude, Gemini and GitHub Copilot all read this file. Role-specific notes live in `CLAUDE.md`, `GEMINI.md` and `.github/copilot-instructions.md`. If those ever disagree with this file, this file wins.

## Who you're working for

Riv is a **complete beginner** building their first game, on Windows, in **Unreal Engine 5.8**. Explain every step click-by-click. Name exact menus, buttons, variable names and default values. Never assume prior Unreal or C++ knowledge.

## The project

- **Game:** *Heavy Mettle* — space-western action RPG. Setting, names and story: `docs/design/WORLD.md`. Read it before writing anything player-facing.
- **Engine:** UE 5.8, project `HeavyMettle.uproject`. Do not change the engine version without Riv's say-so.
- **Architecture:** C++ systems in `Source/Hollowreach` (combat, attributes, inventory, crafting, stealth, survival, quests, dialogue, factions, saves, AI, bosses, companion, vehicles, procedural dungeon). Blueprints in `Content/HeavyMettle` are **children of those C++ classes** (`BP_HM_Player` → `AHRPlayerCharacter`, etc.).
- **Prefer Blueprints for new gameplay** so Riv can see and edit it. Touch C++ only to fix build errors, expose things to Blueprint (`UFUNCTION(BlueprintCallable)`, `UPROPERTY(EditAnywhere, BlueprintReadWrite)`), or when Blueprint genuinely can't do it — and say why.
- **Never change saved IDs** (`Quest.Embers`, `Item.IronBlade`, …). Rename display text only. Saves depend on IDs.
- `Archive/VeilOfAsh` is reference only. Don't compile it or restore a second game mode.

## How work happens

1. **One small, testable change at a time.** Each change must be playable on its own.
2. Every change ends with **three tests** Riv can do in Play-In-Editor, each with the expected result.
3. Every change gets a **commit message** and a line in `docs/daily-progress.md`.
4. Only tick `docs/STATUS.md` after Riv has played it. **Never claim something works that hasn't been run in Unreal.** Say "untested" plainly.
5. Work on a branch and open a pull request; don't push straight to `main`. Don't force-push.
6. Before finishing, run: `python Scripts/check_combined.py` and the rule test (`g++ -std=c++17 -ISource/Hollowreach Tests/campaign_rules_test.cpp -o crt && ./crt`). CI runs them too.

## Scope

The current target is the **15-minute first playable** in `ROADMAP.md` Milestone 1. Don't add new systems, regions or quests beyond the current milestone unless Riv asks.

## Assets

Free assets only (Fab free, Quixel, Epic samples, Mixamo, or Riv's own Blender models). Check the license before importing. Never commit third-party source files that the license doesn't allow redistributing; record what was used in `docs/assets/ASSETS.md`. Hook art into the C++ through **Project Settings → Game → Shared Free Asset Appearance** (`Source/SharedArt`).

## Platforms

Windows first, controller-friendly (Enhanced Input). PS5 is the lead design platform, but console builds need Sony/Epic approval — no platform-specific APIs in gameplay code. See `docs/setup/START_HERE_PS5_MULTIPLATFORM.md`.

## Never

- Commit API keys or secrets. They live in GitHub → Settings → Secrets.
- Delete Riv's work, rewrite history, or merge your own PR.
- Pretend. If you didn't run it, say so.
