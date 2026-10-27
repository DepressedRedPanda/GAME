# Heavy Mettle

A third-person, Blueprint-driven Unreal Engine 5.5.4 game prototype.

Overview
- Title: Heavy Mettle
- Genre: Third-person shooter with melee combo system, climbing, crafting, XP/progression
- Setting: Spaghetti western in space — an android child lost on an alien planet, blending dark-age living with cyberpunk tech.
- Player: Android (modeled as human-like kid) w/ pistol on hip and sword on back, long flowing jacket with cloth physics.
- Implementation: Unreal Engine 5.5.4 (Blueprints-first, minimal/no C++). Use free assets from Epic/Quixel/Marketplace where possible. Use Git LFS for large binary assets.

Repository: https://github.com/DepressedRedPanda/Heavy-Mettle

## Start here (Windows / Unreal Engine 5.5.4)

**Brand new to Unreal?** Follow the click-by-click first playable & Windows packaging guide in docs/ue5.5.4-setup.md before importing marketplace assets or trying to open this repository locally.

## Getting started: create the project locally and test the first playable slice

This repository contains generator scripts and documentation to produce an editable graybox world using Unreal Editor Python tools. It does not ship large .uasset files — those are tracked with Git LFS and must be generated or imported locally.

1. Install Unreal Engine **5.5.4** (Epic Games Launcher → Library → Add Versions → 5.5.4) and Visual Studio 2022 with "Game development with C++" (for optional native tools).
2. Clone or download this repo to a folder on your machine.
3. In the Epic Games Launcher, create a new project:
   - Games → Third Person
   - Project Type: Blueprint
   - Starter Content: Include
   - Project Name: HeavyMettle
   - Location: point to the folder where you cloned this repo (or create inside and copy files later)
4. Copy the `Scripts/` and `docs/` folders from this repo into your new Unreal project folder (Content is generated when you run the scripts).
5. Open the project in Unreal Editor (5.5.4). Enable the **Editor Scripting Utilities** and **Python Editor Script Plugin** if needed.
6. In the Editor: Tools → Execute Python Script → Scripts/build_dustfall.py. This creates `/Game/HeavyMettle/Maps/L_Dustfall` using built-in engine primitives.
7. Play the level: press Play. You should see a graybox village and a PlayerStart.

> Note: This repository favors using the standard Third Person Blueprint template (not the newer Combat variant). All AI and combat Blueprints are written as editable scaffolds in Scripts/create_blueprint_scaffolds.py and the design docs. You will implement aiming and shooting in Blueprint following the steps in docs/first-playable.md.

## Project state
See `docs/feature-log.md` and `DESIGN.md` for feature plans and status. The `Scripts/` folder contains Editor-only Python tools to generate the initial map and blueprint scaffolds.

## Git LFS and large assets
This repository tracks Unreal assets using Git LFS. Before committing .uasset/.umap files locally, run:

```bash
git lfs install
# track patterns (already present in .gitattributes):
git lfs track "*.uasset" "*.umap" "*.fbx" "*.wav" "*.png" "*.jpg"
```

Do not commit API keys or other secrets into the repo. Use GitHub Secrets for CI/ACTIONS keys.
