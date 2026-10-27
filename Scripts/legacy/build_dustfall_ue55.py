"""Build a repeatable Heavy Mettle exploration graybox in Unreal Engine using Editor Python.

Run from Tools > Execute Python Script in a Third Person (Blueprint) template project.
Editor-only generator: NO runtime Python dependency. Produces Content/HeavyMettle/Maps/L_Dustfall.umap.
It does not create executable combat/quest/crafting Blueprints.
"""
import math
import random

import unreal

ROOT = "/Game/HeavyMettle"
MAP = ROOT + "/Maps/L_Dustfall"
MATERIALS = ROOT + "/Materials"
SEED = 47117
RNG = random.Random(SEED)

LEVEL = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ASSETS = unreal.EditorAssetLibrary
MESHES = {
    "cube": unreal.load_asset("/Engine/BasicShapes/Cube.Cube"),
    "cylinder": unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder"),
    "sphere": unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere"),
}
if not MESHES["cube"]:
    raise RuntimeError("Built-in UE Cube mesh not found. Open the project inside Unreal 5.5.4.")
for folder in (ROOT, ROOT + "/Maps", MATERIALS):
    if not ASSETS.does_directory_exist(folder):
        ASSETS.make_directory(folder)

def color_material(name, rgb):
    """Create a simple editable color material. Gray fallback if material API fails."""
    path = MATERIALS + "/" + name
    existing = unreal.load_asset(path)
    if existing:
        return existing
    try:
        factory = unreal.MaterialFactoryNew()
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, MATERIALS, unreal.Material, factory
        )
        if asset is None:
            raise RuntimeError("material factory returned None")
        node = unreal.MaterialEditingLibrary.create_material_expression(
            asset, unreal.MaterialExpressionConstant3Vector
        )
        node.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        unreal.MaterialEditingLibrary.connect_material_property(
            node, "", unreal.MaterialProperty.MP_BASE_COLOR
        )
        rough = unreal.MaterialEditingLibrary.create_material_expression(
            asset, unreal.MaterialExpressionConstant
        )
        rough.set_editor_property("r", 0.86)
        unreal.MaterialEditingLibrary.connect_material_property(
            rough, "", unreal.MaterialProperty.MP_ROUGHNESS
        )
        unreal.MaterialEditingLibrary.recompile_material(asset)
        ASSETS.save_loaded_asset(asset)
        return asset
    except Exception as exc:
        unreal.log_warning("HM material " + name + " skipped: " + str(exc))
        return None

COLORS = {
    "sand": color_material("M_HM_Sand", (0.41, 0.225, 0.15)),
    "road": color_material("M_HM_DustRoad", (0.22, 0.15, 0.13)),
    "stone": color_material("M_HM_Basalt", (0.15, 0.13, 0.15)),
    "plaster": color_material("M_HM_DustPlaster", (0.49, 0.37, 0.26)),
    "wood": color_material("M_HM_DarkTimber", (0.19, 0.105, 0.07)),
    "iron": color_material("M_HM_WornIron", (0.12, 0.18, 0.19)),
    "copper": color_material("M_HM_Copper", (0.48, 0.235, 0.09)),
    "cyan": color_material("M_HM_BatteryBlue", (0.025, 0.27, 0.43)),
    "red": color_material("M_HM_BanditRed", (0.48, 0.055, 0.04)),
    "cloth": color_material("M_HM_Cloth", (0.31, 0.25, 0.20)),
}

# Running repeatedly must not duplicate the level, spawn points, or actors.
if ASSETS.does_asset_exist(MAP):
    if not LEVEL.load_level(MAP):
        raise RuntimeError("Cannot load existing level " + MAP)
    for actor in list(ACTORS.get_all_level_actors()):
        if actor.get_actor_label().startswith("HM_"):
            ACTORS.destroy_actor(actor)
else:
    if not LEVEL.new_level(MAP, False):
        raise RuntimeError("Could not create " + MAP + "; check Unreal project permissions.")

COUNT = 0

def actor_of(cls, name, x, y, z, yaw=0.0, pitch=0.0, roll=0.0):
    global COUNT
    a = ACTORS.spawn_actor_from_class(
        cls, unreal.Vector(float(x), float(y), float(z)),
        unreal.Rotator(float(pitch), float(yaw), float(roll))
    )
    if a is None:
        raise RuntimeError("Could not place actor " + name)
    a.set_actor_label("HM_" + name)
    COUNT += 1
    return a

def primitive(name, x, y, z, sx, sy, sz, material="stone", kind="cube", yaw=0.0):
    mesh = MESHES.get(kind)
    if not mesh:
        mesh = MESHES["cube"]
    obj = actor_of(unreal.StaticMeshActor, name, x, y, z, yaw)
    component = obj.get_component_by_class(unreal.StaticMeshComponent)
    if not component:
        raise RuntimeError("Static mesh component missing for " + name)
    component.set_static_mesh(mesh)
    mat = COLORS.get(material)
    if mat:
        component.set_material(0, mat)
    obj.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    return obj

def world_text(label, message, x, y, z, yaw=0.0):
    try:
        a = actor_of(unreal.TextRenderActor, label, x, y, z, yaw)
        comp = a.get_component_by_class(unreal.TextRenderComponent)
        comp.set_text(message)
        comp.set_editor_property("world_size", 68.0)
        comp.set_text_render_color(unreal.Color(244, 218, 169, 255))
        return a
    except Exception as exc:
        unreal.log_warning("Text label skipped: " + label + " " + str(exc))

# (rest of generator preserved)
