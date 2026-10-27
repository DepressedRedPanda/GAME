# Heavy Mettle

A third-person, blueprint-driven Unreal Engine 5 game prototype.

Overview
- Title: Heavy Mettle
- Genre: Third-person shooter with melee combo system, climbing, crafting, XP/progression
- Setting: Spaghetti western in space — an android child lost on an alien planet, blending dark-age living with cyberpunk tech.
- Player: Android (modeled as human-like kid) w/ pistol on hip and sword on back, long flowing jacket with cloth physics.
- Implementation: Unreal Engine 5 (Blueprints-first, minimal/no C++). Use free assets from Epic/Quixel/Marketplace where possible. Use Git LFS for large binary assets.

Repository: https://github.com/DepressedRedPanda/attempt-two

## Start here (Windows / Unreal Engine 5.8)

**Brand new to Unreal?** Follow [the click-by-click first playable & Windows packaging guide](docs/START_HERE_WINDOWS_58.md) before importing marketplace assets or trying to open this repository's text-only `.uproject`. This guide uses Epic's locally generated Third Person Combat template and our map-generation scripts.

## Getting started: navigate and fight in a graybox

You must have **Unreal Engine 5.8.3** installed on a Windows computer. Epic's official **Third Person > Combat** variant supplies the *binary Blueprint* player, attack combos, health, and enemy AI assets that GitHub text files cannot manufacture on their own.

1. In Epic Games Launcher install Unreal Engine **5.8.3**.
2. In UE Project Browser select **Games > Third Person**, set **Variant = Combat**, **Blueprint**, project name **HeavyMettle**, and create a project somewhere on your computer (not nested inside this Git repository).
3. Download this repository ZIP or `git clone https://github.com/DepressedRedPanda/attempt-two.git`.
4. In the generated Combat project, copy the repository's `Scripts/` folder. You can also copy the docs and data for reference. **Keep the Combat template's `Content/`, `Config/` and `.uproject`**; do not replace its gameplay assets with the text-only repository shell.
5. Enable **Python Editor Script Plugin** and **Editor Scripting Utilities** in the editor, then restart.
6. Use **Tools > Execute Python Script > Scripts/build_dustfall.py**. This generates the editable village and badlands level `/Game/HeavyMettle/Maps/L_Dustfall`. It uses built-in meshes, so no marketplace content is required.
7. Use **Tools > Execute Python Script > Scripts/place_combat_enemies.py**. It discovers Epic's `BP_CombatEnemy` and places three existing combat NPCs near the Old Relay.
8. Add a **Nav Mesh Bounds Volume** that covers the Old Relay combat area. Press **P** in the viewport to check the navigable green floor, then rebuild navigation if needed.
9. Open `L_Dustfall`. In **World Settings > GameMode Override**, choose the same GameMode as Epic's Combat demonstration level (needed if the template uses a per-map override). Press **Play**. Test walking, jumping, attacks, AI pathfinding, and targeting.
10. Optionally run **Scripts/create_blueprint_scaffolds.py** to generate eight editable Blueprint definitions with variables and empty function graphs. The scaffolds **do not** implement the new custom gameplay.
11. Run **Scripts/verify_project.py** to inspect prerequisites in the Unreal Output Log; this doesn't replace playing the game yourself.
12. After the local assets are generated, commit the `Content/HeavyMettle` assets and the original Combat template assets into the game project repository with Git LFS. Until then, a fresh clone of this repository is **not independently playable**.

## Feature status (as of October 17, 2026)

| Feature | Actual state |
| --- | --- |
| Sci-fi western world and mercenary quest design | Written and committed |
| Village graybox, badlands, elevated cover, editor material generation | Unreal Editor generator code committed; **not run in UE here** |
| Player, melee combo/health, enemy base AI | Provided by Epic Combat template **once locally generated and verified** |
| Three enemy placements | Script committed; local UE/navmesh verification needed |
| Custom third-person pistol and shooting | Blueprint instructions only, not implemented |
| Lock-on limited to 3.5m | Blueprint instructions only, not implemented |
| Light/heavy sword branching, block and parry | Blueprint instructions only, not implemented |
| Climbing/mantling | Elevation geometry scripted; gameplay Blueprint not implemented |
| Crafting, XP, battery, dialogue and factional quests | Balance and exact Blueprint design written; not implemented |
| Android appearance, holstered sword/pistol, Chaos Cloth coat | Art and rigging task, not yet delivered |
| Windows executable | **Not built or tested** |

See `docs/BLUEPRINT_BUILD.md` for exact node/logic instructions, `docs/CREATIVE_DIRECTION.md` for story and scope, `docs/QUESTS.md` for missions, `docs/ASSETS.md` for free content and license policy, `ROADMAP.md` for milestone planning, and `DESIGN.md` for alternate design notes.

## Collaboration

- `CLAUDE.md` provides instructions for Claude when connected.
- `GEMINI.md` provides instructions for Gemini when connected.
- `.github/copilot-instructions.md` provides repository-level context for GitHub Copilot when it is enabled.
- These files are instructions, **not proof that any of these AI systems has joined or tested the game**.
- Prefer small pull requests, editor-tested `.uasset/.umap` files, and reproducible tests.

## Why no C++?

Heavy Mettle is designed around Epic's existing Blueprint Combat template. The build scripts use Python **only in Unreal Editor** to automate art and content assembly; packaged gameplay uses Blueprints, not runtime Python. That avoids a C++ toolchain as the first launch obstacle, but a functioning UE5 editor is still required to create the actual playable assets.


## Assets you already own in Epic Games Launcher

The user provided a screenshot of their Fab/Unreal Library showing **Free Animation Library**, **FREE Sword + Animation**, **Free Fantasy Weapon Sample**, **Easy Crosshair**, **Ultra Components – Inventory**, **RPG/Soulslike Stats System**, **Good SKY**, **Medieval Dungeon**, and other sample packs. See **[docs/OWNED_LIBRARY_ASSET_PLAN.md](docs/OWNED_LIBRARY_ASSET_PLAN.md)** for vetted links, order, and compatibility/redistribution caveats. The screenshot shows Unreal 5.5.4 installed, but the existing project target remains UE5.8.3 because the ready-made Combat template is available in 5.6+. Verify exact Fab Supported Engine Versions before any import. Running `Scripts/audit_imported_assets.py` inside your local Unreal project writes a report of likely imported assets at `Saved/HeavyMettleImportAudit.csv`—it does not download or wire gameplay. **No third-party asset binaries have been imported by ChatGPT.**


## Lead platform strategy — PS5-first (2026-10-17)

**Read [docs/START_HERE_PS5_MULTIPLATFORM.md](docs/START_HERE_PS5_MULTIPLATFORM.md) before creating the game.** Heavy Mettle is designed for the PS5 controller and TV experience from day one, with Windows used for editor testing and early .exe packaging. Console versions require official Sony/Microsoft/Nintendo developer approval, Epic console access, platform-specific SDKs and an authorized *source build* of Unreal Engine; the free Launcher-installed UE5.8 cannot package for PS5. Gameplay can remain Blueprint-first. Later Xbox/Steam Deck/Nintendo/other ports are goals, not currently working builds. Never upload proprietary SDKs or other restricted materials to this public repository.
