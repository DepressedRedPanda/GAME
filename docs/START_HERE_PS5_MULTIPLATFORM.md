# HEAVY METTLE — PS5-FIRST, MULTIPLATFORM BEGINNER GUIDE
Updated 2026-10-17. Unreal Engine 5.8 (Blueprint-first). **PS5 is the lead design target**; Windows is the accessible editor/debug/package target. Xbox Series X|S, Switch 2, Linux/Steam Deck, and macOS are potential ports depending on performance, approvals, and resources. Mobile and older consoles require separate feasibility decisions.

## Honest distinction
- **Can do right now:** run UE 5.8's Games > Third Person > Combat **Blueprint** template on Windows; use existing Scripts/ to generate the Dustfall prototype (local engine scripts are **not yet verified**); test with keyboard/gamepad; package a Windows .exe once it works.
- **Cannot do with the free Launcher installation alone:** select PS5 and produce a runnable game for a retail PS5. Sony must authorize development, Epic must grant restricted PS5 integration, and console packaging requires the authorized PS5 SDK and a **source build** of Unreal Engine. This is a build toolchain requirement, not a requirement to hand-write Heavy Mettle gameplay in C++.
- **Important:** A normal store-bought PS5 isn't a substitute for official dev/test hardware; a Windows .exe cannot be installed on it. Obtain hardware/program rights by the approved Sony route. No console binaries, platform libraries, confidential docs, SDKs or protected cert material should be uploaded to this public repository.

## Track A — Windows editor and baseline gameplay (start here)
1. Open Epic Games Launcher > Unreal Engine > Library > Launch **5.8**.
2. In the Project Browser choose **Games > Third Person**, Variant **Combat**, Project Type **Blueprint**, Target **Desktop**, Name **HeavyMettle** (no space). Save in a local folder and click Create.
3. Wait for editor to load, use the toolbar green Play arrow; test WASD, mouse, jump and baseline Combat features; press Esc/Stop, then File > Save All.
4. Open https://github.com/DepressedRedPanda/attempt-two > Code > Download ZIP; Extract All; locate `Scripts` folder. **Copy ONLY Scripts** into your locally generated `HeavyMettle` project root. Never overwrite that local working Combat project's .uproject, Config or Content from this repository's text-only shell.
5. Editor > Edit > Plugins > enable **Python Editor Script Plugin** (and Editor Scripting Utilities if present) > Restart.
6. Editor > Tools > Execute Python Script (menu placement can vary) > local `Scripts/build_dustfall.py`. Read Window > Output Log for error/traceback. Do not assume this untested script runs without adjustments.
7. Content Drawer > `Content/HeavyMettle/Maps/L_Dustfall` > double-click. In World Settings set the same GameMode Override as Epic's Combat sample if Pawn isn't spawning. Save All and Play.
8. When the village loads, run `Scripts/place_combat_enemies.py` (looks for Epic's `BP_CombatEnemy`; if its name differs, adjust script). Add Nav Mesh Bounds Volume over Old Relay; press P to inspect green navigation. Test enemies in Play In Editor.
9. Set Edit > Project Settings > Maps & Modes > Game Default Map = `L_Dustfall`; ensure Combat GameMode is active; Save All.
10. Choose Play arrow dropdown > Standalone Game to test as an external editor-run window.
11. Choose Edit > Project Settings > Packaging > Build Configuration **Development**, then top toolbar **Platforms > Windows > Package Project** (x64). Choose a new output folder. Wait for Packaging Complete; run `HeavyMettle.exe` from the generated folder. Keep the complete packaged output intact. If packaging fails, open Output Log/Packaging Results and fix first red relevant error before repeating.

**Done when**: play in template, play in L_Dustfall, and then double-click a packaged Windows game with controller/keyboard working. Packaging DOES NOT mean all the planned custom quest/combat/art features exist.

## Track B — Controller-first, PS5-style game design in Blueprint
Do this **before writing custom game mechanics**, regardless of whether Sony approves console development yet.

### B1. Test a controller on Windows
1. Connect a compatible gamepad by USB. If using a DualSense, Windows/UE may require a compatible input driver or controller middleware. Do not assume it works just because Windows makes a connection sound.
2. Press Win+R > type `joy.cpl` > press Enter. Look for a connected controller; choose Properties to inspect stick/button activity. **This only verifies Windows recognition, not Unreal mapping**.
3. Return to Unreal, click Play, see whether left stick moves, right stick looks, and expected actions work. If they do not, **stop and troubleshoot input mapping/device support**, not combat Blueprints.
4. Prefer a working generic/XInput-compatible controller for first basic Windows controller testing; later verify native DualSense behavior with authorized PlayStation platform tools on PS5 development hardware.

### B2. Map abstract actions instead of hard-coding PlayStation-specific keys
1. Open Content Drawer (Ctrl+Space). In the search box type `IMC_` and find the Input Mapping Context actually used by your Third Person Combat character. Do not guess its filename; use template's existing input setup and check the player Blueprint/BeginPlay if needed.
2. **Duplicate** the working IMC and name it `IMC_HM_Gameplay`, but keep existing movement mappings. If the Combat character references its original IMC, assign the duplicate in the character Blueprint and test immediately.
3. Use Enhanced Input **Input Actions** (search `IA_` for existing template actions) and create any missing actions via Content Drawer > Add > Input > Input Action:
   - `IA_HM_Aim`, `IA_HM_FireHeavy`, `IA_HM_LightAttack`, `IA_HM_BlockParry`, `IA_HM_Dodge`, `IA_HM_Interact`, `IA_HM_WeaponSwitch`, `IA_HM_Reload`, `IA_HM_LockOn`, `IA_HM_Pause`.
   - Set each new pressed action to Boolean. Move/Look are Axis 2D.
4. Open `IMC_HM_Gameplay`; add bindings to actual **Gamepad** keys. Suggested initial PS5-style layout (subject to game design testing):
   - Left Stick Move / Right Stick Look.
   - Cross (bottom face button) Jump.
   - Circle (right face button) Dodge.
   - Triangle (top face button) Interact.
   - Square (left face button) Reload.
   - R1 Light sword attack.
   - R2 Heavy sword attack **if not aiming** / Fire pistol **while aiming**.
   - L1 Block/parry.
   - L2 Aim pistol.
   - R3 Lock on nearby enemy only (<=350cm, line of sight).
   - L3 Sprint, D-pad left/right weapon switch, Options Pause.
5. **These mappings alone do not make gameplay**: create Blueprint event handlers for each new Input Action in the player, routing R2 to fire when aiming and to heavy attack otherwise. Preserve the Combat template's working attack logic until new handlers are tested. Eliminate input conflicts before continuing.
6. Compile Blueprint, Save, Play; test every button. Never let menu navigation require a mouse.
7. For controller prompts and cross-platform buttons later, consider **Common UI** and platform-specific controller glyph/data assets instead of hard-coded Triangle/Cross images. Review Epic's Common UI Quickstart.

### B3. Console-safe choices from day one
- Menus navigable with D-pad or left stick, buttons for Confirm/Back, no mouse-only interactions.
- HUD/text legible on a living room TV from several feet away; no essential corner content cropped by display area.
- Aim for smooth frame time and a 60fps PS5 performance mode as a design goal, **not a guaranteed result**. Avoid huge high-poly demo environment scenes or expensive cloth simulation everywhere.
- Gameplay remains single-player for first vertical slice; add networking only if later requested, as it radically increases cost and complexity.
- Do not tie saves to a hard-coded `C:\...` Windows file. Use UE SaveGame APIs and later authorized PS5 save integration.
- Use platform-neutral Input Actions. Don't hard-code PSN achievements, PS5-only controllers, or proprietary console APIs into baseline character Blueprints.
- Use fallback sounds, meshes, and animations where third-party Fab content has restricted engines/licenses. Follow docs/OWNED_LIBRARY_ASSET_PLAN.md.

## Track C — Official PlayStation access (separate approval process)
1. On your PC open a browser, visit **https://partners.playstation.net/**. Sign up / apply as a PlayStation partner/developer. Individuals and small studios may apply; acceptance is Sony's decision.
2. Have a **real** project concept and a working Windows video/build to describe: *Heavy Mettle — Blueprint-first third-person sci-fi spaghetti western; lost android; frontier village; local mercenary jobs*. State your real experience, available resources, intended audience, target platforms and stage honestly. Save your application confirmation.
3. Wait for Sony to review. If approved, follow the official partner portal's NDA, development agreement and hardware/SDK instructions. Use only Sony-supplied private documentation; do NOT copy it to this public GitHub repository.
4. Once Sony **developer approval** is verified, open **https://dev.epicgames.com/documentation/unreal-engine/development-for-playstation-5-in-unreal-engine** and follow its official request at **https://forms.unrealengine.com/s/form-console-access-request** to obtain Epic's restricted Unreal PS5 access.
5. Configure the **approved source-built Unreal engine**, required platform SDK, and officially authorized test hardware **exactly per the private current Sony/Epic documents**. These steps cannot responsibly be reproduced publicly because they depend on approved versions and NDA-protected details.
6. Build/deploy the PlayStation configuration to official PS5 dev/test equipment (not ordinary retail PS5), verify controls, saves, perf, trophies/requirements as applicable, and resolve errors.
7. Once the game is ready, follow Sony's submission and certification process to release through the PlayStation Store. PS5 release is not automatic after developer approval.

Official references:
- Sony partner portal: https://partners.playstation.net/
- Epic PS5 access: https://dev.epicgames.com/documentation/unreal-engine/development-for-playstation-5-in-unreal-engine
- Epic console access request: https://forms.unrealengine.com/s/form-console-access-request
- Sony program hardware information: https://sonyinteractive.com/en/news/blog/complimentary-development-hardware/

## Track D — Other targets
| Target | Route | Caveat |
|---|---|---|
| Windows PC | UE 5.8 Launcher packaging on Windows | First independently playable test |
| PS5 | Sony PlayStation Partners -> Epic approved PS5 Unreal access -> source engine/SDK/devkit -> certification | **Primary console** |
| Xbox Series X/S | ID@Xbox / Microsoft developer onboarding -> Epic's Xbox console access -> SDK/source engine/certification | Controller-first gameplay carries over, but build does not |
| Nintendo Switch 2 | Nintendo Developer Portal and platform request -> Epic Switch 2 access -> licensed tools/test hardware | Aggressive performance/content optimization likely |
| Steam Deck/Linux | Test Linux packaging and controller interfaces, Proton/Steam Deck performance, platform storefront requirements | Need hardware testing |
| macOS | Mac development/build setup and platform testing | PC packages cannot be relabeled as macOS |
| iOS/Android | Dedicated touch controls, UI/performance adaptation, mobile publishing credentials | **Later phase**, no commitment of parity |
| Older PS4/Xbox One/Switch | Only after authorized access, platform feasibility, performance budget and SDK support checked | No promise every prior-generation device can run this game |

**Do not claim "released on all platforms" until each target is actually approved, built, tested and certified.** Prioritize Windows-first functional testing and PS5-first **design**, which is not the same as having a PS5 binary today.

## Milestones / acceptance
- [ ] User created local UE 5.8 Blueprint Combat template; screenshot of running Play In Editor.
- [ ] Controller is recognized, movement and combat work in local template.
- [ ] Dustfall script generated `.umap`, map loads and character can traverse it without errors.
- [ ] Basic melee enemy encounters tested in Dustfall.
- [ ] Windows Development .exe packaged and runs independently of editor.
- [ ] Custom combat, pistol, crafting, first mercenary quest and save verified and committed as appropriate.
- [ ] Sony developer status approved (external process, only after user has proof).
- [ ] Epic PS5 Unreal access granted (external process).
- [ ] Licensed PS5 SDK and dev/test environment set up (private tools).
- [ ] PS5 build deployed/tested on authorized hardware.
- [ ] PS5 certification/submission complete; then Xbox/Nintendo etc individually.

## Technical documentation
- https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project
- https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine
- https://dev.epicgames.com/documentation/en-us/unreal-engine/common-ui-quickstart-guide-for-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/xbox-development-in-unreal-engine
- https://developer.microsoft.com/en-us/games/articles/2023/11/publishing-pathways-id-at-xbox/
- https://developer.nintendo.com/register
- https://dev.epicgames.com/documentation/en-us/unreal-engine/nintendo-switch-2
