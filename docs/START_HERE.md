# Start here — Windows, Unreal Engine 5.8

Click-by-click, for a complete beginner. Do one numbered section at a time. If something goes red, **stop**, copy the first error, and paste it to ChatGPT/Claude/Gemini along with the step number.

## 1. Install the tools (once)

1. **Unreal Engine 5.8** — Epic Games Launcher → Unreal Engine → Library → the **+** next to Engine Versions → pick the newest **5.8.x** → Install. You can keep 5.5.4 installed too.
2. **Visual Studio** (needed because this project has C++) — install the version Epic lists for 5.8 (Visual Studio 2022 or newer, Community edition is free). In the installer tick **Game development with C++**, and on the right make sure **Unreal Engine installer** and a **Windows SDK** are ticked.
3. **Git** and **Git LFS** — install Git for Windows, then open *Git Bash* and run `git lfs install` once.
4. **GitHub Desktop** (optional, easier than the command line).

## 2. Get the project

1. Clone `https://github.com/DepressedRedPanda/GAME` (GitHub Desktop → File → Clone repository). Put it somewhere short like `C:\Dev\GAME` — long paths cause C++ build errors.
2. In that folder you should see `ManOfMettle.uproject`, `Source`, `Content`, `Config`, `Scripts`, `docs`.

### If you already have a local ManOfMettle project

1. **Back it up first:** copy the whole `C:\UnrealProjects\ManOfMettle` folder somewhere safe (e.g. `ManOfMettle_backup_2026-10-27`).
2. Copy its **`Content`** folder into the GAME clone, merging with the `Content` folder already there. Say **no** to replacing any file that already exists.
3. **Don't** copy its `.uproject` or `Config` over GAME's — GAME's `ManOfMettle.uproject` is what adds the C++ systems.
4. Note whether your local project was the **Combat** or **standard** Third Person template (Combat projects have a `Variant_Combat` folder in Content). Tell the AIs which one; Combat-only instructions don't apply to a standard project.
5. Commit in GitHub Desktop: *"Add local ManOfMettle content"*. The `.uasset` files go through Git LFS automatically.

## 3. Build the C++ the first time

1. Right-click `ManOfMettle.uproject` → **Generate Visual Studio project files**. (If that menu item is missing: Epic Launcher → 5.8 → Launch once, then try again.)
2. Open the new `ManOfMettle.sln` in Visual Studio.
3. Top toolbar: set **Development Editor** and **Win64**. Right-click **HeavyMettle** in Solution Explorer → **Set as Startup Project**.
4. Menu **Build → Build Solution**. The first build takes a while.
5. **Expected:** errors. This code was written for 5.5 and has never been built. Copy the *first* error from the **Error List** (or the first `error C` line in Output) and give it to Claude or ChatGPT with: *"UE 5.8 build error in the GAME repo, smallest fix please."* Fix one, build again, repeat. Commit after each fix.

   Alternative without opening Visual Studio: PowerShell in the repo folder →
   `.\Scripts\BuildEditor.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'`

## 4. Open the editor

1. When the build says **Build succeeded**, press **F5** in Visual Studio (or double-click `ManOfMettle.uproject`).
2. Enable plugins if asked (Python Editor Script Plugin, Editor Scripting Utilities) and restart.
3. Make one empty level: File → New Level → **Empty Level** → save as `Content/Maps/World`.
4. Window → **World Settings** → GameMode Override → **HRGameMode**. Press **Play**. The C++ builds a graybox world at runtime.
5. Tick the first boxes in [docs/STATUS.md](../STATUS.md) that actually worked.

## 5. Make the Blueprint layer

1. Tools → **Execute Python Script** → `Scripts/create_blueprint_scaffolds.py`.
2. It creates `BP_HM_Player`, `BP_HM_Bandit`, `BP_HM_Villager`, `BP_HM_Companion`, `BP_HM_Guardian`, `BP_HM_GameMode` in `Content/HeavyMettle/Blueprints`. Each is a **child of a C++ class**, so it already has the C++ behavior — you add to it.
3. In World Settings switch the GameMode Override to **BP_HM_GameMode**, and in BP_HM_GameMode set Default Pawn Class to **BP_HM_Player**.
4. Optional: `Scripts/build_dustfall.py` builds the Cinder's Rest graybox map.

## 6. Commit

GitHub Desktop → write a short summary like *"Build fixes for UE 5.8: HRCombatComponent"* → Commit → Push. `.uasset`/`.umap` files go through Git LFS automatically.

## Platforms

Windows first. PS5 and consoles need Sony/Epic developer approval and a source-built engine — see [START_HERE_PS5_MULTIPLATFORM.md](START_HERE_PS5_MULTIPLATFORM.md). The older guide for the Combat template is kept as [START_HERE_WINDOWS_58.md](START_HERE_WINDOWS_58.md).
