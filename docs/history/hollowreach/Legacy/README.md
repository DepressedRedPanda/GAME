# VEIL OF ASH — Unreal Engine 5.8 C++ systems prototype

This is a **source-code starter project**, not a precompiled executable or production game. It deliberately uses Engine BasicShapes instead of any authored art assets, and creates a small region of geometry, resource nodes and enemies **when Play starts**.

**Tested in this environment:** project structure, class references, configured inputs and C++ source static checks. **Not tested:** compiling or running inside Unreal Engine (Unreal Editor is not installed here). There may be engine-version or platform-specific issues to correct after the first real compile.

## Requirements

- Windows PC with **Unreal Engine 5.8**, installed through Epic Games Launcher. The project targets UE 5.8, but a newer compatible UE5 version can be tried by updating EngineAssociation after making a backup.
- **Visual Studio 2022** with the **Game development with C++** workload, Windows SDK and Unreal integration/toolchain.
- Keyboard and mouse for all mapped controls. Basic gamepad movement axes are present; complete gamepad action mappings aren't included.

## Getting started

1. Extract this zip, keeping `VeilOfAsh.uproject` next to `Config/` and `Source/`.
2. On Windows, right-click `VeilOfAsh.uproject` → **Generate Visual Studio project files** (or open the `.uproject` directly; Unreal can generate and build C++ project files).
3. Open the project in Unreal Editor 5.8 and accept the request to build the missing C++ module.
4. Create a **New Level → Basic** or **Empty** level, save it as `Content/Maps/L_Prototype`. Add a **Player Start** at `(0, 0, 180)` if desired. The GameMode has a fallback if none exists.
5. Select **World Settings → GameMode Override → AshGameMode** if the project defaults haven't applied. In **Project Settings → Maps & Modes**, set `L_Prototype` as both default game and editor startup map.
6. Press **Play**. The C++ GameMode generates the small blockout terrain, nodes, enemies, boss, lights and movable crates on server BeginPlay. They do **not** persist in the authored editor map.

If the editor appears black or you've opened the engine Entry map rather than a playable level, create the Basic level and press Play there. Most of the visible content is created only at runtime.

## Controls (keyboard + mouse)

| Input | Action |
|---|---|
| WASD + Mouse | Move and look |
| Space / Shift / Left Ctrl | Jump / sprint / crouch |
| Left / Right mouse button | Light combo / heavy strike |
| Q / Left Alt / V | Parry / dodge / hold guard |
| F / G / T | Void pulse / spectral form / short mantle |
| E | Interact with nearest object |
| B | Build a basic shelter (5 Wood + 2 Ore) |
| 1 / 2 / 3 | Craft ration / tonic / blade upgrade |
| 4 / 5 | Consume ration / tonic |
| 6 / 7 / 8 | Improve Might / Guile / Insight (with skill points) |
| F1 / F2 / F3 | Vanguard / Seer / Shade class (before leveling) |
| C | Change companion order: Follow / Guard / Hunt |
| F5 / F9 | Save / load **character progress** (standalone only) |
| R | Resurrect at checkpoint after death |
| F6 / F7 / F8 | Choose one of three endings **after** boss victory |

The screen-space debug HUD shows vitals, resources, objectives, coordinates, nearby objects, world state and last interaction. Key bindings live in `Config/DefaultInput.ini`.

## First-play route

1. Pick a class (F1/F2/F3), collect wood and herbs, find the forge and craft basic supplies.
2. Talk to the wandering ally near your starting area to recruit the spherical companion. Change their job with C.
3. Activate the memory shrine behind the starting area. It sets your respawn checkpoint.
4. Find **3 cipher seals and 2 lost records**, scattered across the world. Approach them and press E. The lost records reveal the runic sequence **SECOND → FIRST → THIRD**.
5. You can unlock the boss gate either with **3 ciphers + 2 records**, **or** solve the rune sequence and gather **1 cipher + 1 record**. This is a small nonlinear progression puzzle.
6. The larger cone-shaped enemy in the northeast is the boss. It is invulnerable until the gate puzzle is solved. Learn its attack telegraph and time a parry.
7. After winning, pick an ending (F6/F7/F8). Freefolk and Choir endings require a positive reputation with that envoy. The third ending does not.

Map orientation: starting camp at the center, major boss and gate in the **northeast (+X, +Y)**, scattered exploration nodes in all quadrants. The world measures approximately 88 meters on each side.

## Systems implemented as runnable prototype C++

- Player pawn with third-person camera, jumping, sprinting, crouching, short mantle, spectral mode.
- Server-authoritative actions and interaction range checks. Replicated pawn stats, inventory, enemies, companion, world-state flags and level-blockout actors. **Network play has not been tested.**
- Attacks, heavy combos, parries, timed dodge immunity, block, stamina/focus, stagger/poise and backstabs.
- Enemy perception reduced by crouch, class/Guile, nighttime and ashstorm weather; simple direct pursuit, attack wind-ups and boss.
- XP, three development branches, starting class, numerical weapon upgrades, inventory, loot drops, crafting and consumables.
- Hunger, thirst, injury, infection, regeneration, shrine rest, death and checkpoint resurrection, optional GameMode permadeath flag.
- Procedural placements seeded by GameMode `WorldSeed`; pickups, ruins, movable physics crates, building a shelter, simple harvest node.
- Knowledge-gated cipher/rune puzzle and two routes through the gate; faction reputation and three ending states.
- Recruitable companion (Follow adds Focus, Guard heals, Hunt attacks nearby enemies).
- Accelerated day/night clock and 3 weather types that affect enemy detection; ashstorm also affects infection.
- Minimal local character save/load in standalone mode. **Saving does not persist killed enemies or collected world nodes after restarting the session.**

## Explicitly simplified or not implemented yet

This is **not** a complete implementation of every feature listed in the design brief. The following have starter analogues rather than full systems:

- “Open world” is a **small generated test arena**, not Unreal World Partition streaming, scalable terrain or interconnected production levels.
- “Vehicle exploration” is a timed movement boost from a transport beacon; there is **no drivable vehicle** yet.
- “Farming” is a repeatable harvest interaction, not crop planting/soil/growth cycles.
- “Dungeon crawling” is outdoor ruins and encounters, not interior dungeon generation.
- “Equipment customization” is a numerical weapon tier, not a full equipment-slot/loadout system.
- “Dialogue” is short interaction notices and faction choice, not cinematic conversations or dialogue trees.
- “Companion tactical orders” are 3 abstract behaviors, not navigation, cover or pathfinding.
- “Co-op” has a replicated prototype structure and shared world-state, but **no tested co-op sessions**, multiplayer missions, server infrastructure or matchmaking.
- “Weather” affects numbers and detection but has no rain/ash VFX.
- “Save/load” does not serialize world objects, quest completion or active multiplayer sessions.
- No animation system, audio, menus, accessibility settings, input remapping UI, full equipment system, ability UI, balancing pass or automated gameplay tests yet.

## Architecture

`Source/VeilOfAsh/` houses:

- `AshGameMode` — game bootstrap and small procedural level.
- `AshHero` — player input, camera, server actions, ability combat and saves.
- `AshVitals` — regenerating and survival resources, damage, dodge/parry timing.
- `AshInventory` — item stacks, consumption, crafting, upgrades.
- `AshProgression` — levels, classes, skills, factions.
- `AshEnemy` — sight/detection, pursuit, wind-up attack, stagger, boss, drops.
- `AshWorldNode` — resource, clue, rune, shrine, gate, NPC and transport interactions.
- `AshCompanion` — recruitable helper and 3 assistance strategies.
- `AshWorldState` — replicated clocks, weather, puzzle and boss state.
- `AshStructure`, `AshBlock`, `AshPhysicsCrate` — blockout/build/physical geometry.
- `AshHUD` — engine Canvas debug HUD; replace with UMG when making a real game.
- `AshSave` — limited serialized character save state.

## Next engineering milestone

1. **Compile in UE5.8 and fix the first reported errors**; then test controls and the complete quest path in PIE.
2. Separate actions into **Gameplay Ability System** abilities (or reusable combat ability components), and replace generic mesh cones with an animation blueprint.
3. Set up World Partition + Data Layers + streaming, and move procedural placement to PCG/data assets.
4. Replace numeric weapons with DataAssets, equipment slots and visual modifiers.
5. Add formal quest/dialogue state graphs, persistent World Save, gameplay tags and item definitions.
6. Test listen-server PIE with two players, authoritative cooldowns, prediction, ownership, remote visuals and disconnect/rejoin.
7. Add a real traversal component, drivable vehicle pawn and enemy AI with perception/behavior trees/NavMesh.

**Version-control note:** Keep new experimental features on separate Git branches. For example, Claude can create `feature/equipment` while ChatGPT reviews the changes in a pull request. Don't let two agents edit `AshHero.cpp` simultaneously without coordinating.
