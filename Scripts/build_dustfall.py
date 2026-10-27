"""Build a repeatable Heavy Mettle exploration graybox in Unreal Engine 5.8.

Run from Tools > Execute Python Script in a Third Person *Combat* template project.
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
    raise RuntimeError("Built-in UE Cube mesh not found. Open the project inside Unreal 5.8.")
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

def block_building(name, x, y, width=780, depth=610, height=500, front=-1):
    """Hollow, enterable shop with a 240cm street-facing doorway."""
    thick = 48
    primitive(name + "_floor", x, y, 4, width, depth, 18, "stone")
    for side in (-1, 1):
        wall_y = y + side * (depth / 2 - thick / 2)
        if side == front:
            wing = (width - 240) / 2
            for k in (-1, 1):
                px = x + k * (width / 2 - wing / 2)
                primitive(name + "_frontWing_" + str(k),
                          px, wall_y, height / 2, wing, thick, height, "plaster")
            primitive(name + "_header", x, wall_y, height - 45,
                      240, thick, 90, "wood")
        else:
            primitive(name + "_rear", x, wall_y, height / 2,
                      width, thick, height, "plaster")
    for k in (-1, 1):
        primitive(name + "_side_" + str(k),
                  x + k * (width / 2 - thick / 2), y, height / 2,
                  thick, depth, height, "plaster")
    primitive(name + "_roof", x, y, height + 17, width + 120, depth + 100, 34, "wood")
    for k in (-1, 1):
        primitive(name + "_solarRail_" + str(k),
                  x - 120, y + k * 130, height + 42, 270, 18, 25, "iron")
    primitive(name + "_solarPanel", x - 120, y,
              height + 60, 280, 280, 12, "cyan")
    world_text(name + "_sign", name.replace("_", " ").upper(),
               x - width / 2 + 70, y + (front * (depth / 2 + 18)), 310,
               0 if front < 0 else 180)

def raised_platform(prefix, x, y, z, w=600, d=550):
    primitive(prefix + "_deck", x, y, z, w, d, 30, "wood")
    for dx in (-w / 2 + 40, w / 2 - 40):
        for dy in (-d / 2 + 40, d / 2 - 40):
            primitive(prefix + "_leg_" + str(dx) + "_" + str(dy),
                      x + dx, y + dy, z / 2, 42, 42, z, "wood")
    # Primitive stairs: navigable by regular jump; dedicated mantling remains a TODO.
    for step in range(1, 6):
        primitive(prefix + "_step_" + str(step), x - w / 2 - 360 + step * 94,
                  y, step * (z / 5) / 2,
                  94, 230, step * (z / 5), "stone")

# Main ground and the ring of surrounding badlands.
primitive("RustFlats_main", 0, 0, -120, 13000, 8900, 230, "sand")
primitive("RustFlats_east", 8000, 0, -135, 6000, 10000, 230, "sand")
primitive("CinderRoad", -400, 0, 3, 11000, 720, 12, "road")
primitive("NorthTrack", 900, 2400, 4, 720, 4900, 10, "road")
primitive("CentralWell_base", 0, 0, 38, 330, 330, 76, "stone", "cylinder")
primitive("CentralWell_water", 0, 0, 81, 190, 190, 14, "cyan", "cylinder")
primitive("CentralWell_cap", 0, 0, 120, 310, 310, 19, "wood", "cylinder")

# Eight individually identifiable village structures.
names = ["FREIGHT_OFFICE", "CINDER_TAVERN", "MEDIC", "SALVAGE_SHOP",
         "GUILD_HALL", "SMITHY", "LODGINGS", "WATCH_POST"]
for row, yy in enumerate((1440, -1440)):
    for col, xx in enumerate((-2750, -1140, 1120, 2660)):
        name = names[row * 4 + col]
        block_building(name, xx, yy, width=710 + 40 * (col % 2),
                       depth=640, height=470 + 35 * col, front=-1 if row == 0 else 1)

# Enterable central kiosk / bounty board.
primitive("BountyBoard_postL", -3550, -385, 115, 28, 28, 230, "wood")
primitive("BountyBoard_postR", -3120, -385, 115, 28, 28, 230, "wood")
primitive("BountyBoard_panel", -3340, -385, 207, 470, 25, 250, "wood")
world_text("JobBoard", "MERCENARY JOBS", -3540, -410, 286)
world_text("QuestMarker_Oren", "OREN: MISSING CELLS [QUEST BLUEPRINT TODO]",
           -3030, 1010, 160)

# Market booths and waist/head-high waist-cover around combat lane.
for k, xx in enumerate((-1970, -240, 1900)):
    primitive("MarketCounter_" + str(k), xx, -550, 60, 360, 140, 120, "wood")
    primitive("MarketShade_" + str(k), xx, -600, 270, 480, 370, 25, "cloth")
    for dx in (-210, 210):
        primitive("MarketPole_" + str(k) + "_" + str(dx),
                  xx + dx, -600, 140, 26, 26, 260, "wood")

# Uneven climbable/jumpable height tiers. Combat template can jump; formal climbing remains open.
raised_platform("South_Overlook", -1800, -3000, 240)
raised_platform("North_Scrapyard", 1900, 3050, 310)
raised_platform("East_Patrol_Post", 5100, -1400, 400)
for n in range(30):
    x = RNG.uniform(-3800, 6700)
    y = RNG.choice((-1, 1)) * RNG.uniform(600, 3900)
    if abs(x) < 3000 and 750 < abs(y) < 2200:
        continue  # avoid building interiors
    h = RNG.choice((90, 125, 180, 225))
    primitive("Cover_" + str(n), x, y, h / 2, RNG.randint(120, 265),
              RNG.randint(100, 220), h, RNG.choice(("stone", "iron", "wood")),
              yaw=RNG.choice((0, 15, -25, 45)))

# Perimeter, scattered cable posts, dust farm solar towers.
for i in range(18):
    xx = -5300 + i * 610
    yy = -4100 if i % 2 else 4040
    primitive("PerimeterTower_" + str(i), xx, yy, 150, 72, 72, 300, "wood")
    primitive("TowerLight_" + str(i), xx, yy, 312, 95, 95, 40, "copper")
for i in range(10):
    xx = RNG.uniform(-4800, 5500)
    yy = RNG.choice((-1, 1)) * RNG.uniform(3100, 4050)
    primitive("Scrap_" + str(i), xx, yy, 75,
              RNG.uniform(80, 220), RNG.uniform(90, 180), 150, "iron",
              yaw=RNG.uniform(-40, 40))

# Old Relay bandit yard, southeast of the village.
primitive("OldRelay_foundation", 7800, -1600, 65, 2150, 1650, 140, "stone")
primitive("OldRelay_antenna", 8300, -2100, 560, 55, 55, 980, "iron", "cylinder")
for dx in (-650, 650):
    primitive("Relay_silo_" + str(dx), 7800 + dx, -1500, 220,
              240, 240, 440, "copper", "cylinder")
for i in range(7):
    xx = 6900 + (i % 3) * 420
    yy = -2400 + (i // 3) * 440
    primitive("BanditBarricade_" + str(i), xx, yy, 105, 280, 70, 210, "red")
world_text("RelayMarker", "OLD RELAY — ENEMY AI / QUEST TO DO", 6840, -800, 245)
primitive("PowerCellsLoot", 7580, -1420, 90, 200, 140, 180, "cyan")

# Starter position and lighting. No packaged assets required beyond UE engine meshes.
actor_of(unreal.PlayerStart, "PlayerStart", -4500, 0, 145)
sun = actor_of(unreal.DirectionalLight, "LateSun", 0, 0, 3000, -28, -42)
try:
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if light:
        light.set_editor_property("intensity", 5.0)
        light.set_editor_property("use_temperature", True)
        light.set_editor_property("temperature", 4300.0)
except Exception as exc:
    unreal.log_warning("Could not tune sunlight: " + str(exc))
for cls, name in ((unreal.SkyLight, "SkyFill"),
                  (unreal.SkyAtmosphere, "SkyAir"),
                  (unreal.ExponentialHeightFog, "DustHaze")):
    try:
        actor_of(cls, name, 0, 0, 1600)
    except Exception as exc:
        unreal.log_warning(name + " unavailable: " + str(exc))
try:
    fog = next(a for a in ACTORS.get_all_level_actors()
               if a.get_actor_label() == "HM_DustHaze")
    component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    component.set_editor_property("fog_density", 0.008)
except Exception:
    pass

if not LEVEL.save_current_level():
    raise RuntimeError("Level generated but save failed. Save /Game/HeavyMettle/Maps/L_Dustfall manually.")
unreal.log("HEAVY METTLE: created/updated " + MAP + " with " + str(COUNT)
           + " actors. Open the map and click Play. World layout only; Heavy Mettle gameplay graphs remain TODO.")
