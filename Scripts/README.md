# Scripts

## Run inside the Unreal Editor (Tools → Execute Python Script)
Needs the Python Editor Script Plugin and Editor Scripting Utilities (enabled in `HeavyMettle.uproject`).

| Script | What it does |
| --- | --- |
| `create_blueprint_scaffolds.py` | Makes the Blueprint layer: `BP_HM_Player`, `BP_HM_Bandit`, `BP_HM_Villager`, `BP_HM_Companion`, `BP_HM_Guardian`, `BP_HM_GameMode` as children of the C++ classes, plus quest-giver, crafting bench and loot pickup actors. Set `CREATE_STANDALONE_SCAFFOLDS = True` to also get attempt-two's original Blueprint-only components. |
| `build_dustfall.py` | Builds the Cinder's Rest / Old Relay graybox map at `/Game/HeavyMettle/Maps/L_Dustfall` from engine shapes. |
| `place_combat_enemies.py` | Places Epic Combat-template enemies near the Old Relay (only if that template's content is present). |
| `place_bandit_placeholders.py` | Places three `BP_HM_Bandit` near the Old Relay (from Heavy-Mettle). |
| `verify_project.py` | Prints a prerequisites report to the Output Log. |
| `audit_imported_assets.py` | Lists imported asset packs so licenses can be checked. |
| `legacy/` | The UE 5.5.4 versions of the map and verify scripts from the Heavy-Mettle repo. |

## Run on your PC or in GitHub (no Unreal needed)
| Script | What it does |
| --- | --- |
| `BuildEditor.ps1` | Builds `HeavyMettleEditor` from PowerShell: `.\Scripts\BuildEditor.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'` |
| `check_combined.py` | C++/project consistency checks (also run by CI) |
| `static_smoke.py` | Same checks, older entry point |
| `verify_git_lfs.sh` | Confirms Git LFS is installed and tracking Unreal files |
