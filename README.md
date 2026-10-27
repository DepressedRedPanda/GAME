# GAME — Heavy Mettle

One Unreal Engine 5.8 project that combines all three earlier attempts at the same game:

| Came from | What it brought |
| --- | --- |
| **Heavy-Mettle** | Blueprint-first plan for UE 5.5.4, first-playable shooting lesson, daily AI lesson pipeline |
| **attempt-two** | The space-western setting, creative direction, quests, balance data, UE 5.8 editor scripts, PS5 plan, asset plan |
| **The-Game** (Hollowreach / Veil of Ash) | ~130 C++ files of working systems: combat, stealth, survival, crafting, quests, factions, saves, bosses, companion, vehicles, procedural dungeon |

**The game:** an amnesiac android takes mercenary jobs on a dusty alien frontier, and slowly discovers the ruins under it are where he came from. Full setting: [docs/design/WORLD.md](docs/design/WORLD.md).

**How it's built:** the C++ module (`Source/Hollowreach`) supplies the systems. You build on top of them with **Blueprints** in `Content/HeavyMettle` — day-to-day work stays in the Blueprint editor.

## Honest status

Nothing here has been opened in Unreal 5.8 yet. The C++ passes the repo's own consistency checks and 43 rule tests, but it was written for 5.5 and **has never been compiled**. Expect build errors the first time — that's the first job. See [docs/STATUS.md](docs/STATUS.md) for the "works in PIE" checklist; nothing goes on it until it's actually been played.

## Start here

1. Follow **[docs/setup/START_HERE.md](docs/setup/START_HERE.md)** — install UE 5.8 and Visual Studio, clone, build, open.
2. Then the first lesson: [docs/setup/first-playable-shooting.md](docs/setup/first-playable-shooting.md).
3. Working with the AIs (ChatGPT, Claude, Gemini, Copilot): [docs/ai/AI_WORKFLOW.md](docs/ai/AI_WORKFLOW.md).

## Repository map

```
HeavyMettle.uproject     open this in Unreal 5.8
Source/
  Hollowreach/           main C++ game module (systems)
  SharedArt/             C++ hooks for plugging in free art/animation
  VeilOfAsh/             only reads old Veil of Ash save files
  HeavyMettle*.Target.cs build targets
Content/HeavyMettle/     your Blueprints, maps, imported assets (Git LFS)
Config/                  engine, game and input settings
Scripts/                 editor Python tools + build/check scripts
Tests/                   engine-free rule tests
data/balance.json        tuning numbers from attempt-two
docs/
  design/                WORLD.md (setting), quests, creative direction, systems maps
  setup/                 install + beginner lessons
  systems/               how the C++ systems fit together, saves, validation
  assets/                free asset plans and manifest
  ai/                    AI workflow and prompts
  history/               original READMEs/roadmaps/instructions from all three repos
Archive/VeilOfAsh/       original Veil of Ash source, kept for reference (not compiled)
AGENTS.md  CLAUDE.md  GEMINI.md  .github/copilot-instructions.md   AI instructions
```

## Old repos

`Heavy-Mettle`, `attempt-two` and `The-Game` were left untouched. Once you're happy with this repo you can archive them on GitHub (Settings → Archive this repository) so nobody works in the wrong place.
