# GAME — Heavy Mettle

Unified Unreal game project combining previous prototypes into one canonical GAME repo.

One **Unreal Engine 5.8** project (`ManOfMettle.uproject`) built from all three earlier attempts at the same game:

| Came from | What it brought |
| --- | --- |
| **attempt-two** | The space-western setting, creative direction, quests, balance data, UE 5.8 editor scripts, PS5 plan, asset plan |
| **Heavy-Mettle** | First-playable shooting lesson, UE 5.5.4 setup, daily AI lesson pipeline |
| **The-Game** (Hollowreach / Veil of Ash) | ~130 C++ files of working systems: combat, stealth, survival, crafting, quests, factions, saves, bosses, companion, vehicles, procedural dungeon |

**The game:** an amnesiac android takes mercenary jobs on a dusty alien frontier, and slowly discovers the ruins under it are where he came from. Setting: [docs/WORLD.md](docs/WORLD.md).

**How it's built:** the C++ (`Source/Hollowreach`) supplies the systems. You build on top of them with **Blueprints** in `Content/HeavyMettle`, so day-to-day work stays in the Blueprint editor. Plan and decision log: [docs/GAME_CONSOLIDATION.md](docs/GAME_CONSOLIDATION.md).

## Honest status

Nothing here has been opened in Unreal 5.8 yet. The C++ passes the repo's consistency checks and 43 rule tests, but it was written for 5.5 and **has never been compiled** — expect build errors the first time. Only things actually played go in [docs/STATUS.md](docs/STATUS.md).

## Start here

1. [docs/START_HERE.md](docs/START_HERE.md) — install, bring in your local ManOfMettle project, build, open.
2. [docs/FIRST_PLAYABLE_SHOOTING.md](docs/FIRST_PLAYABLE_SHOOTING.md) — the first lesson.
3. [docs/ai/AI_WORKFLOW.md](docs/ai/AI_WORKFLOW.md) — how ChatGPT, Claude, Gemini and Copilot share the work.

## Repository map

```
ManOfMettle.uproject     open this in Unreal 5.8
Source/                  C++: Hollowreach (systems), SharedArt (art hooks), VeilOfAsh (old-save reader)
Content/HeavyMettle/     your Blueprints, maps, imported assets (Git LFS)
Config/  Scripts/  Tests/  data/
docs/                    WORLD, STATUS, START_HERE, design docs, systems/, ai/, daily-lessons/
Legacy/                  the three original repos, verbatim (see Legacy/SOURCE_COMMITS.txt)
AGENTS.md                shared rules every AI follows
CLAUDE.md  GEMINI.md  .github/copilot-instructions.md   per-AI roles
```

The original repos `Heavy-Mettle`, `attempt-two` and `The-Game` are untouched. Archive them only after GAME builds and plays (see the cutover list in the consolidation plan).
