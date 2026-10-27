# Heavy Mettle — Design

High-level goals
- Blueprint-first third-person shooter with melee depth.
- Simple, approachable systems so you can iterate in-editor.
- Modular levels with verticality (chest-height / head-height climbable geometry).
- Mix of low-tech 'dark age' settlements and advanced tech devices.

Core Systems

1) Player & Movement
- BP_PlayerCharacter (inherits from Character)
  - Components: Mesh, Camera Boom, Camera, Capsule, Jacket Cloth (Chaos Cloth), Weapon Sockets (hip, back), Stamina, Health, Battery/Energy
  - Movement: walk/run/sprint, crouch, jump
  - Climbing: simple climb system based on traces and "Climbable" tag volumes. Two climb heights: chest (approx 1m) and head (approx 1.8m). If trace hits climbable and ledge height within thresholds -> enter climb montage and move to top.

2) Weapons
- Pistol (range weapon)
  - BP_Gun (Blueprint Actor)
  - Ammo crafting, reload, spread, recoil
- Sword (melee)
  - BP_Sword (Actor attached to socket when sheathed)
  - Melee combo system with light/heavy inputs

3) Melee Combo System
- Inputs: LightAttack, HeavyAttack, Block, Parry
- State machine (BP_MeleeState)
  - Basic windowed combo system: pressing Light/Heavy within a combo window advances combos
  - Example Combos:
    - L -> L: Quick two-hit
    - L -> H: Finisher with stagger
    - H -> L: Heavy opener -> quick follow
- Block and Parry:
  - Block reduces incoming damage while held; stamina drains
  - Parry is a timed tap while blocking within a small window: successful parry staggers enemy and triggers a counter-attack window

4) Lock-on & Camera
- Limited lock-on system: when player presses LockOn, find target actors with tag "Lockable" within radius (default 3–5m). If multiple, pick closest or let player cycle.
- While locked, camera orbits target and aim assists; outside lock-on, free-aim third-person camera

5) Combat & AI
- BP_Enemy (AIController + Behavior Tree)
  - States: Idle, Patrol, Aggro, MeleeAttack, RangedAttack, Flee
  - For ranged enemies, use simple projectile or hitscan

6) Crafting & Inventory
- Simple crafting menu: craft ammo, bandages, grenades
- BP_Inventory stores items and resources. Crafting costs resources; crafting can be performed at workbenches or anywhere.

7) XP & Progression
- XP gain from quests, kills, and craft milestones
- Level-up grants points to increase: Health, Stamina, Battery (mana/tech), Speed, Damage

8) Level & World Design
- Use modular blocks and props to make semi-random layouts with multiple elevations and climbable ledges at chest/head heights
- Encounters built around verticality: snipers on high ledges, melee in narrow alleys

Blueprint Class Plan (suggested names)
- BP_PlayerCharacter (Character)
- BP_PlayerController (PlayerController)
- BP_Gun (Actor)
- BP_MeleeWeapon (Actor) / BP_Sword
- BP_Enemy (Character)
- BP_Interactable_Climbable (Actor)
- BP_InventoryComponent (ActorComponent)
- BP_CraftingUI (Widget Blueprint)
- BP_XPComponent (ActorComponent)
- BP_LockOnComponent (ActorComponent)

Example Blueprint logic (Melee combo - textual)
- Variables: CurrentComboIndex (int), ComboTimer (float), IsInCombo (bool)
- On LightAttack input:
  - If !IsInCombo: Start combo -> Play montage section "Light_1"; CurrentComboIndex=1; IsInCombo=true; Start ComboTimer
  - Else if CurrentComboIndex == 1 and ComboTimer < Window: Play montage "Light_2"; CurrentComboIndex=2; Reset ComboTimer
  - Else if CurrentComboIndex == 2 ...
- On combo timer expire: IsInCombo=false; CurrentComboIndex=0

Climbing (textual blueprint)
- On Jump/Climb input: LineTraceForward from character eye to maxDistance
  - If hit actor has tag "Climbable" and hitNormal roughly vertical and ledge height within range:
    - Disable movement, play climb montage, move character to top via Launch or SetActorLocation over time

Lock-on (textual blueprint)
- On LockOn input:
  - SphereOverlapActors with radius R
  - Filter for actors implementing ILockable or tag "Lockable"
  - If found: set CurrentTarget, set bIsLocked=true, damp player's camera to face target
  - If none found: bIsLocked=false

Performance & Asset Notes
- Keep Blueprint logic modular to allow later conversion to C++ for perf-critical parts.
- Use Epic/Quixel free meshes where possible; track uasset files with Git LFS.

Art & Assets
- Character: humanoid android model (similar to human kid)
- Jacket: setup cloth (Chaos Cloth)
- Weapons: pistol + blade; holster/back sheathe sockets
- Environment: low-poly modular buildings + sci-fi props

QA & Playtesting
- Create small test levels for systems in isolation (CombatArena, ClimbCourse, CraftingBench)

