# Blueprint implementation guide — HEAVY METTLE
Engine baseline: UE 5.8.3 **Third Person > Combat** variant (Blueprint).
This is an implementation specification, NOT a claim that these nodes are already wired. Editor Python creates map/scaffolds, while Gameplay Blueprints must be edited, compiled and PIE-tested in a running editor. Keep the original Combat template working during each integration.

## 0. Editor launch and acceptance
Start in a fresh Combat-variant project named HeavyMettle. In the project's generated Content folder preserve the original template assets. Run, in order, the repository scripts:
1. `build_dustfall.py` — creates colored graybox `L_Dustfall`.
2. `create_blueprint_scaffolds.py` — creates unconnected editable BP classes and empty function graphs.
3. `place_combat_enemies.py` — finds Epic's `BP_CombatEnemy` in the Combat template, placing 3 near the Old Relay.

Open `L_Dustfall`, inspect Project Settings > Maps & Modes and verify the current default GameMode spawns the *Combat* player (not DefaultPawn). In an empty map the default GameMode may differ from the sample map's per-level override. If necessary use World Settings > GameMode Override and select the exact game mode used by the Combat example map. Rebuild navmesh with a Nav Mesh Bounds Volume around the Old Relay; press P to view navigable green floor. PIE-test the player and enemies. Do not discard sample map until this is verified.

## 1. Character and camera
Duplicate the Combat character Blueprint to `BP_HM_Android`, and its associated GameMode to `BP_HM_GameMode`. Point GameMode default pawn at BP_HM_Android. In GameMode, retain Combat's existing input setup, camera, combo and enemy damage behaviors.
- Shoulder camera: spring arm 300–370cm, vertical offset 70cm; interpolate shoulder offset while aiming. Never move the camera into a wall.
- Movement: walk 300cm/s; sprint 550cm/s at stamina cost; camera-driven movement (current template supports third-person movement).
- Equip `BP_HM_Stats`, `BP_HM_Inventory`, `BP_HM_Combat` and `BP_HM_QuestTracker` as components. Their generated function graphs are EMPTY until you implement them. Initialize stat values to data/balance.json.
- The character is a synthetic human. Battery is an electrical resource; no magic abilities.

## 2. Short lock-on
Modify Combat template's targeting rather than introducing a competing lock-on manager.
- Input `IA_LockOn`: `Sphere Overlap Actors` radius 350cm, object type Pawn.
- Filter self, allies/dead actors and targets behind blocking geometry. Score each candidate: camera-forward dot above 0.3; favor central visible target and distance.
- Retain target only while distance <=350cm AND a line trace has clear line of sight; clear on death, teleport, or blocked view. Limit retarget updates to ~10/sec, not every tick for all AI.
- While locked, smoothly orient movement toward target, but preserve the Combat template's actual attack combo.
- Past 350cm, lock-on MUST stop: aim manually with camera crosshair and pistol trace. Automated projectile tracking is explicitly forbidden.

## 3. Third-person pistol
Start with a colored blockout `SM_PistolProxy` or primitive attached to the mannequin `thigh_r` socket, then replace with a rights-cleared model.
- `IA_Aim` held: turn camera into over-the-shoulder aim, show crosshair. `IA_Fire` pressed: check current mag >0 and fire cooldown; subtract 1, play muzzle VFX/SFX.
- Query line: `Start = FollowCamera WorldLocation`; `End = Start + GetForwardVector(CameraRotation) * 15000cm`; `LineTraceByChannel(Visibility)`. Trace collision should ignore player actor; don't fire from the camera through solid cover.
- From pistol muzzle trace toward camera hit point for actual obstruction; if muzzle-to-target path is blocked, hit the wall instead.
- Apply point damage on valid hit using `ApplyPointDamage`; set base to 20. Reserve a slot for 8 rounds; manual reload requires spare ammo. Range balancing to be tested.
- Avoid Aim Assist within 350cm where melee lock-on handles fighting. Equip/draw transitions must update visible gun from hip to right hand.

## 4. Sword melee and defense
Start by reusing Combat's functioning baseline combos and animation timing. Add sword socket `spine_03` (holstered), `hand_r` (drawn), and trail VFX after functional damage.
- `IA_Light`: Light, stamina -14, base damage 14. `IA_Heavy`: Heavy, stamina -28, damage 26.
- Store the last 3 button inputs during an approximately 0.35-sec attack window. Set combo routing from tables instead of hundreds of hard-coded branches: `LLL` = fast three-hit, `LLH` = overhead finisher, `LHL` = sweep, `HLL` = stagger follow-up, `HH` = charged finisher. Only enable transitions at Anim Montage notify windows.
- Detect blade hits with weapon collision or socket sweep during a montage AnimNotifyState. Track actors hit per swing, clear on new swing.
- `IA_Block` held: reduce frontal damage, drain 12 stamina/sec; break guard at zero. A press within 0.18sec before a valid hit is a parry; stagger enemy, briefly open riposte. A failed parry still risks damage. Enforce forward facing dot to prevent blocking backstabs.
- Shared attack / pistol state: guard, climb, reload and attack montages cannot trigger incompatible actions simultaneously.

## 5. Climbing without superhero traversal
For nearby chest-high (100–120cm) and head-high (180–220cm) surfaces:
- Forward capsule trace from player at waist height to detect a mantle obstacle.
- Second forward trace at top and downward trace for a valid standing area, then capsule sweep to verify safe landing.
- Play short chest-high or taller vault/mantle animation, updating position only after collision checks. Don't blindly SetActorLocation through geometry.
- Provide a visible falling path from elevated rooftops; no inaccessible random one-way jump traps.
- In blockout map, existing platforms and steps are traversable via base jumping. Actual mantle Blueprint and matching animations are a separate acceptance test.

## 6. Crafting (minimal resource loop)
Use `BP_HM_Inventory` with integers: Scrap, Powder, Cloth, Medgel, BatteryCells, Ammo, HealingWraps, Grenades.
- Ammo: 2 Scrap + 1 Powder -> 6 rounds.
- Healing wrap: 2 Cloth + 1 Medgel -> 1 wrap.
- Grenade: 3 Scrap + 2 Powder + 1 BatteryCell -> 1 grenade.
- Crafting function must compare all costs before subtracting ANY resource. On insufficient materials leave inventory unchanged. Apply heal 40 to a max of 100 base health, no overheal.
- Craft bench uses `BPI_Interactable` interface; `Interact` opens a UMG menu. Item lists and upgrade progress save through SaveGame.

## 7. Progression
XP threshold for next level: `100 + 75*(currentLevel-1)`. Keep overflow XP and award a stat point. Per point choose one:
- Health +10 maximum.
- Stamina +10 maximum.
- Battery +10 maximum.
- Movement speed +2% multiplicatively, soft cap.
- Melee and ballistic base damage +3%, soft cap.
Combat health UI, stamina bar, battery meter, XP and inventory should be read directly from the component data and updated through event dispatchers rather than every tick.

## 8. Quest and dialogue
Use `BP_HM_QuestTracker` state machine (enum recommended): NotAccepted -> TalkingToOren -> InvestigateTrack -> AtRelay -> ResolveShipment -> Report -> Complete.
- Dialogue offers Accept / Decline. Investigation interaction records footprint evidence, then shows a map waypoint.
- At the relay player can defeat raiders, sneak to the crate, or parley. Each route changes ending, faction flags, payout, and NPC follow-up dialogue. No unavoidable kill requirement.
- Save state in `SG_HM_Player`: Stats, Inventory, Quest flags, current map and safe respawn transform. Load should not duplicate mission rewards.
- Dialogues should be 1–3 sentences and distinct per NPC. No magical prophecies, no lore dump.

## 9. Weapons, animations, coat and appearance
Before buying content:
- Manny skeleton socket `Sword_Back` anchored near spine; `Pistol_Hip` anchored to upper right pelvis or thigh; gun combat socket `Weapon_R`; blade combat socket `Sword_R`.
- Use SkeletalMeshComponent or StaticMeshComponent attach to socket with `SnapToTargetNotIncludingScale`. Copy draw/holster position between visible in-hand and holstered meshes by toggling visibility.
- Flowing coat must be skinned to the same skeleton, cover torso/thighs, and be simulated with Chaos Cloth / Cloth Asset. Add collisions against legs/hips; test sprint, 90-degree turn, roll, sit, and sword draw. Avoid clipping and cloth-driven performance spikes.
- Prefer cheap mannequin/proxy assets during mechanics. A purchased coat or custom MetaHuman is **not** required for first PIE run.

## 10. Acceptance list (do not check off without testing)
- [ ] L_Dustfall opens without any C++ compile dialog.
- [ ] Combat character spawns, walks and jumps inside Dustfall.
- [ ] Three spawned Combat-template enemies navigate and attack in PIE.
- [ ] A successful melee combo causes enemy HP loss.
- [ ] Pistol shoulder aim, hit detection, ammo decrement and reload work.
- [ ] Lock-on stops past 350cm.
- [ ] Light/heavy sequences, block and parry can be reproduced.
- [ ] Both 120cm and 220cm mantle succeed without clipping.
- [ ] Craft all three recipes, refuse invalid crafts safely.
- [ ] Missing Cells quest completes through at least two different outcomes.
- [ ] XP/stat upgrade persists across save/reload.
- [ ] Sword and pistol appear in holsters; coat behaves in motion.
- [ ] Packaged Windows Shipping build launches without editor.

Avoid changing this checklist until actual PIE or packaged build evidence exists.