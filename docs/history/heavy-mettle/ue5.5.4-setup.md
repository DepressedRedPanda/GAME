# UE5.5.4 Setup & Short Guide (Windows)

1) Install:
- Epic Games Launcher → sign in
- In Launcher: Library → Add Versions → choose 5.5.4 and install
- Install Visual Studio 2022 with the "Game development with C++" workload (needed for some editor tooling)
- Install Git and Git LFS: https://git-lfs.github.com/ and run `git lfs install`

2) Create project from Third Person template
- Launch Unreal Editor (5.5.4) → New Project → Games → Third Person
- Choose Blueprint project, include Starter Content
- Project Name: HeavyMettle
- Location: your local repo folder (or create then copy the Scripts/ docs/ into it)

3) Copy scripts and docs
- Copy the repo Scripts/ and docs/ into the project root so you can run Tools → Execute Python Script → Scripts/build_dustfall.py

4) Generate the Graybox Level
- In Editor: Tools → Execute Python Script → Scripts/build_dustfall.py
- Save the map in Content/HeavyMettle/Maps/L_Dustfall

5) Run verify scripts
- Tools → Execute Python Script → Scripts/verify_project.py
- This will run simple checks for expected plugins and settings

6) Track assets with Git LFS (local)
- In your local project folder run:
  git lfs install
  git lfs track "*.uasset" "*.umap" "*.fbx" "*.wav" "*.png" "*.jpg"
- Confirm tracked patterns in .gitattributes

7) Commit initial project state
- Commit only small text-based files first (README, scripts, docs).
- After you generate assets and import content, commit .uasset/.umap files (they will be stored using Git LFS)

