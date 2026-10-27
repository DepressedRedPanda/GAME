"""Place Epic Combat-template AI opponents into L_Dustfall, if asset exists.

Requires the Third Person > Combat template content. Run after build_dustfall.py.
No AI Blueprint is copied from the internet or created by this script.
"""
import unreal

MAP = "/Game/HeavyMettle/Maps/L_Dustfall"
LEVEL = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ASSETS = unreal.EditorAssetLibrary

if not ASSETS.does_asset_exist(MAP):
    raise RuntimeError("First run Scripts/build_dustfall.py.")
if not LEVEL.load_level(MAP):
    raise RuntimeError("Could not load the Dustfall map.")

matches = []
for path in ASSETS.list_assets("/Game", recursive=True, include_folder=False):
    stem = str(path).rsplit("/", 1)[-1].split(".")[0]
    if stem == "BP_CombatEnemy":
        matches.append(str(path).split(".")[0])
if not matches:
    raise RuntimeError(
        "Epic BP_CombatEnemy not found. Create a Third Person > Combat template "
        "and copy its Content into this project. See README.md."
    )

# Prefer Epic template paths over third-party assets with matching names.
matches.sort(key=lambda p: (0 if "Variant_Combat" in p else 1, len(p)))
enemy_class = ASSETS.load_blueprint_class(matches[0])
if not enemy_class:
    raise RuntimeError("Cannot load compiled class for " + matches[0])

for a in list(ACTORS.get_all_level_actors()):
    if a.get_actor_label().startswith("HM_RelayBandit_"):
        ACTORS.destroy_actor(a)

locations = [(6920, -1650, 240), (7780, -1850, 240), (8420, -1210, 240)]
for i, (x, y, z) in enumerate(locations):
    a = ACTORS.spawn_actor_from_class(
        enemy_class, unreal.Vector(float(x), float(y), float(z)),
        unreal.Rotator(0, 180, 0)
    )
    if not a:
        raise RuntimeError("Enemy actor " + str(i) + " spawn failed")
    a.set_actor_label("HM_RelayBandit_" + str(i + 1))

if not LEVEL.save_current_level():
    raise RuntimeError("Could not save actors in L_Dustfall.")
unreal.log("Heavy Mettle: placed 3 Combat-template enemies from " + matches[0])
unreal.log_warning(
    "AI navigation requires a Nav Mesh Bounds Volume covering the relay and "
    "a rebuilt navmesh. Enable Show Navigation (P) in editor to inspect."
)
