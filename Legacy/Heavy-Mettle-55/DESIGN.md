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
  - Climbing: simple climb system based on traces and "Climbable" tag volumes. Two climb heights: chest (approx 1m) and head (approx 1.8m).

2) Weapons
- Pistol (range weapon)
  - BP_Gun (Actor)
  - Ammo crafting, reload, spread, recoil
- Sword (melee)
  - BP_Sword (Actor attached to socket when sheathed)
  - Melee combo system with light/heavy inputs

3) Melee Combo System
- Inputs: LightAttack, HeavyAttack, Block, Parry
- State machine (BP_MeleeState)
  - Basic windowed combo system: pressing Light/Heavy within a combo window advances combos

4) Lock-on & Camera
- Limited lock-on system: when player presses LockOn, find target actors with tag "Lockable" within radius (default 3–5m). If multiple, pick closest or let player cycle.

5) Combat & AI
- BP_Enemy (AIController + Behavior Tree)

6) Crafting & Inventory
- Simple crafting menu: craft ammo, bandages, grenades

7) XP & Progression
- XP gain from quests, kills, and craft milestones

8) Level & World Design
- Use modular blocks and props to make semi-random layouts with multiple elevations and climbable ledges at chest/head heights

Blueprint Class Plan (suggested names)
- BP_PlayerCharacter (Character)
- BP_PlayerController (PlayerController)
- BP_Gun (Actor)
- BP_MeleeWeapon (Actor) / BP_Sword
- BP_Enemy (Character)
- BP_InventoryComponent (ActorComponent)
- BP_CraftingUI (Widget Blueprint)
- BP_XPComponent (ActorComponent)
- BP_LockOnComponent (ActorComponent)

Animation & Blender notes
- Use Mixamo for initial animations and retarget into Unreal's Skeleton
- For jacket cloth, use Chaos Cloth in UE and paint weights

