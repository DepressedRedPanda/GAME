# Heavy Mettle — Step-by-step Windows first-playable and packaging guide (Unreal Engine 5.8)

> Beginner-safe instructions; written 2026-10-17. Unreal Engine 5.8 is installed on the user's Windows PC. Actual game level generator scripts are currently untested in Unreal Editor, and custom systems/packaged output have NOT yet been built by ChatGPT. Any step that errors must be debugged from Output Log rather than assumed to work.

## 1. Make a truly functioning template first
1. Launch Epic Games Launcher -> Unreal Engine -> Library -> Launch 5.8.
2. In the Project Browser choose **Games -> Third Person**.
3. In the template settings choose **Variant: Combat**, **Project Type: Blueprint**, target **Desktop**, project name **HeavyMettle** (no spaces). Set location, e.g. `Documents\Unreal Projects`; click **Create**. Unreal writes a new `HeavyMettle.uproject` and the required binary assets locally.
4. Wait for project to open and shaders to compile.
5. Click green **Play** triangle. Test W/A/S/D, mouse and Space and template combat controls shown in the sample; press Esc/Stop to exit.
6. File -> Save All. **STOP** if this base Combat template does not move/fight successfully. Record screen/error first.

## 2. Download the source scripts from GitHub, not its .uproject
1. Open https://github.com/DepressedRedPanda/attempt-two -> green **Code** -> **Download ZIP**.
2. Open Windows File Explorer -> Downloads. Right-click ZIP -> **Extract All** -> **Extract**.
3. Open resulting `attempt-two-main` directory. Find folder **Scripts**.
4. Open a second Explorer window (Win+E) and navigate to `Documents\Unreal Projects\HeavyMettle` (your actual local project path may differ). Confirm you see **Content**, **Config**, and **HeavyMettle.uproject**.
5. **Copy ONLY the Scripts folder** from the ZIP into that local HeavyMettle project folder; you should get `...\HeavyMettle\Scripts\build_dustfall.py`.
6. **DO NOT replace/overwrite** the local `HeavyMettle.uproject`, `Config`, or `Content` with same-named items from GitHub. The checked-in .uproject is a text-only shell and is NOT an engine-generated Combat-template project.

## 3. Generate Dustfall level
1. In your running project choose **Edit -> Plugins**.
2. Search **Python Editor Script Plugin** -> Enable. Search **Editor Scripting Utilities** -> Enable if available. Click **Restart Now**.
3. Choose **Tools -> Execute Python Script** (depending on UE UI, it can be under Tools -> Python or File -> Execute Python Script). Use the file chooser to find `HeavyMettle\Scripts\build_dustfall.py` and click Open.
4. Allow it to finish. To inspect progress/errors choose **Window -> Developer Tools -> Output Log** or use the **Output Log** bottom tab if present. Read the first red error/traceback if any.
5. Open Content Drawer -> Content -> HeavyMettle -> Maps -> **L_Dustfall** (double-click). Save All.
6. If map generation succeeded but no character spawns, open **Window -> World Settings**. Find **GameMode Override** and choose the same GameMode used by the original Epic Combat sample map; you can inspect the original sample map's World Settings and copy its exact choice. Test Play and Esc again.
7. The generator builds a graybox village, cover and relay area. It does NOT implement pistol, crafting, quests, climbing or a cloth coat.

## 4. Enemy placement (optional for first standalone)
1. Run **Scripts/place_combat_enemies.py** through the same Execute Python Script menu.
2. This depends on the Epic Combat enemy Blueprint name `BP_CombatEnemy`. If the asset has a different name in your local UE5.8 template, the script may stop with a clear error; report it and do not keep rerunning it.
3. To support enemy AI, select **Window -> Place Actors** (or use the Place Actors panel). Search **Nav Mesh Bounds Volume**. Drag it into the level. Select it; in **Details -> Transform**, resize it so it covers the enemy fight area at the Old Relay, including the floor. Press **P** to show green navigable ground. Rebuild navigation as required, save level, Play-test.
4. Optionally run **Scripts/verify_project.py** from the same menu, then read Output Log; note that this script checks asset presence, not runtime gameplay. **Do not run `create_blueprint_scaffolds.py` yet unless you want empty experimental classes—it does not add functional gameplay.**

## 5. Set map and compile/Play-test
1. In the editor choose **Edit -> Project Settings -> Maps & Modes**.
2. Under Default Maps choose **Game Default Map = L_Dustfall**. Optionally set Editor Startup Map to L_Dustfall too.
3. Under Default Modes make sure the correct Epic Combat GameMode is selected as needed (or use matching per-level GameMode override). Otherwise the wrong Pawn can spawn.
4. Return to L_Dustfall -> File -> Save All -> Play. Confirm you can walk and see the new village. If enemies are placed, check their AI behavior. For a fuller test use the arrow beside Play -> **Standalone Game** (this is not the packaged .exe).
5. For any Blueprint you personally edit: double-click the Blueprint asset -> click **Compile** at top -> fix red errors -> **Save**. No C++/Visual Studio project should need compiling for our pure Blueprint runtime prototype.

## 6. Create Windows standalone executable
1. Editor -> **Edit -> Project Settings -> Maps & Modes**: Game Default Map should be L_Dustfall. If not, correct it and Save All.
2. In same Project Settings choose **Packaging** -> **Build Configuration: Development** for first test. Close Settings.
3. Editor top toolbar -> **Platforms -> Windows -> Package Project** (UE may present an x64 architecture selection; choose Windows x64). Choose a new folder, e.g. `Desktop\HeavyMettleBuild` as the packaging output, **not** the source project directory.
4. Wait for message `Packaging Complete` or check Output Log / Message Log -> Packaging Results. If failure, stop and capture exact red messages.
5. Open output folder in Windows File Explorer. Open subfolder(s) until you see **HeavyMettle.exe**. Double-click to run. **Keep the full output folder together**, including Paks/data and other support files; an .exe by itself is not enough.
6. Test keyboard/mouse, FPS/controls, that L_Dustfall actually loads, and that Esc/Alt+F4 exits. Development build may also accept ~ then `quit`.
7. This Windows executable is a **playable prototype**, not the completed Heavy Mettle RPG. Shipping build configuration comes later after functional systems and exit UI work.

## 7. If something fails
- **Missing project / module compile dialog**: You likely opened the GitHub text shell .uproject rather than the true Unreal-generated Combat project. Open the correct file from your local project folder.
- **Script crash**: Window -> Output Log, copy first red traceback plus 10 surrounding lines. Report exact script filename and UE version.
- **Character absent or spectator camera**: set the same GameMode as original Combat map; verify a PlayerStart actor exists.
- **Enemies stationary**: check navmesh covers floor and press P for green surfaces. Also check enemy Blueprint version/name.
- **Black packaged screen**: make sure Game Default Map is set to L_Dustfall; Save All and repackage.
- **Package failed**: use Packaging Results & Output Log and share the last red fatal messages; check Windows packaging target SDK only if log explicitly complains.
- **Fab marketplace assets**: do not add these until vanilla Combat template and Dustfall play. See docs/OWNED_LIBRARY_ASSET_PLAN.md for safe version/asset integration. Do not upload licensed third-party content into the public repo without appropriate permission.

## Official UE5.8 docs
- Third Person Combat template: https://dev.epicgames.com/documentation/unreal-engine/third-person-template-in-unreal-engine
- Packaging: https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project
- Changing startup map: https://dev.epicgames.com/documentation/unreal-engine/changing-the-default-level-of-an-unreal-engine-project
- Using the Platforms dropdown: https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-platforms-dropdown-in-unreal-editor
- Editor Python: https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python
