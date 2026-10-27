# Roadmap

Hollowreach is built piece by piece. Phase 1 put every system in place and wired them together in a playable
greybox world. Each later phase deepens one area without rewriting the others.

## Phase 1: Systems foundation (this commit)

- Every gameplay system from the design brief, in C++, as reusable components and subsystems.
- A runtime-built greybox open world, procedural dungeon, canvas HUD and code-defined input.
- Built-in content: 39 items, 10 recipes, 6 loot tables, 18 skills, 4 factions, 6 quests, 5 dialogues, 5 endings.

### Known gaps

- **Not compiled yet.** The first build will need small API fixes.
- No animation: the greybox blade swings procedurally from combat state. `FHRAttackStep::Montage` is ready for real montages.
- Canvas HUD only; menus are numbered text lists.
- Single-player only. Co-op is covered by the AI companion for now.
- No audio.

## Phase 2: Make it feel good

- Build and fix; add automated tests for loot rolls, quest progression and save/load.
- Character models, animation blueprints and montages per attack step; hit reactions; camera shake.
- Niagara effects for fire, deflect sparks, posture breaks, rain and snow.
- Sound: footsteps (they already feed the AI), weapon impacts, ambience per region and weather.
- Tune combat numbers by playtesting the Warden and the Trial guardians.

## Phase 3: Real world

- Replace greybox regions with real levels using World Partition; hand-place the same gameplay actors.
- Landscape terrain for Ashen Peak and the plains; foliage that feeds `AHRStealthZone`.
- Behavior Trees / StateTree if the hand-written AI outgrows itself.

## Phase 4: Presentation and UI

- UMG widgets reading the same data the HUD draws (menus already go through `AHRPlayerController::BuildMenu`).
- Dialogue camera, journal map, skill-tree screen, inventory grid.

## Phase 5: Online co-op

- Replicate attributes, combat state and inventory; server-authoritative combat and loot.
- Co-op missions built on the companion quest structure; the Trial as a co-op roguelite (Nightreign-style).

## Phase 6: More game

- More regions, factions and quests per region; more bosses with unique move sets.
- More transformations and spells; weapon arts per weapon.
- New Game+, harder Trial depths with leaderboards, photo mode.
