"""Create editable Blueprint *definitions* for Heavy Mettle in Unreal Editor 5.8.

GAME repo layout: the C++ module (Hollowreach) supplies the working systems
(player, combat, inventory, quests, saves...). The Blueprints made here are
CHILDREN of those C++ classes, so you tune and extend everything in Blueprint
without touching C++. If the C++ module is not built yet, each Blueprint falls
back to a plain engine parent so the script still runs.

The original standalone component scaffolds from attempt-two (Stats, Inventory,
Combat, QuestTracker) duplicate C++ components, so they are only created when
CREATE_STANDALONE_SCAFFOLDS = True.

These assets compile but deliberately contain NO functional game graph.
This makes their exact status visible and avoids claiming feature-complete gameplay.
You can modify all names, variables, and function graphs inside Blueprint Editor.
"""
import unreal

ROOT = "/Game/HeavyMettle/Blueprints"
LIB = unreal.BlueprintEditorLibrary
ASSETS = unreal.EditorAssetLibrary

if not ASSETS.does_directory_exist(ROOT):
    ASSETS.make_directory(ROOT)

CREATE_STANDALONE_SCAFFOLDS = False


def cpp_parent(class_name, fallback):
    """Return /Script/Hollowreach.<class_name> if the C++ module is loaded, else fallback."""
    try:
        cls = unreal.load_class(None, "/Script/Hollowreach." + class_name)
        if cls:
            return cls
    except Exception:
        pass
    unreal.log_warning("C++ class " + class_name + " not loaded; using " + fallback.__name__)
    return fallback


# Blueprint layer on top of the C++ systems (always created)
LAYERED = [
    ("BP_HM_GameMode", cpp_parent("HRGameMode", unreal.GameModeBase), [], []),
    ("BP_HM_Player", cpp_parent("HRPlayerCharacter", unreal.Character),
     [("Ammo", "int"), ("FireRate", "real"), ("IsFiring", "bool"), ("Damage", "real")],
     ["StartFiring", "StopFiring", "FireOnce", "Reload"]),
    ("BP_HM_Bandit", cpp_parent("HREnemyCharacter", unreal.Character),
     [("Alerted", "bool"), ("KnowsPlayerLocation", "bool")],
     ["OnSight", "Die"]),
    ("BP_HM_Villager", cpp_parent("HRNPCCharacter", unreal.Character),
     [("NPCName", "string"), ("QuestToOffer", "string")],
     ["OfferQuest"]),
    ("BP_HM_Companion", cpp_parent("HRCompanionCharacter", unreal.Character), [], []),
    ("BP_HM_Guardian", cpp_parent("HRBossCharacter", unreal.Character), [], []),
]

# (asset, parent, variables with types, empty function graphs)
STANDALONE = [
    ("BP_HM_Stats", unreal.ActorComponent,
     [("Health", "real"), ("MaxHealth", "real"), ("Stamina", "real"),
      ("MaxStamina", "real"), ("Battery", "real"), ("XP", "int"),
      ("Level", "int"), ("Chits", "int"), ("SpeedMultiplier", "real"),
      ("DamageMultiplier", "real")],
     ["TakeDamage", "AddExperience", "SpendAttributePoint", "RestoreBattery"]),
    ("BP_HM_Inventory", unreal.ActorComponent,
     [("Scrap", "int"), ("Cloth", "int"), ("Powder", "int"),
      ("Medgel", "int"), ("BatteryCells", "int"), ("Ammo", "int"),
      ("HealingWraps", "int"), ("Grenades", "int")],
     ["AddItem", "RemoveItem", "HasIngredients", "CraftAmmo",
      "CraftHealingWrap", "CraftGrenade"]),
    ("BP_HM_Combat", unreal.ActorComponent,
     [("LockedOn", "bool"), ("CurrentCombo", "string"),
      ("ComboWindowOpen", "bool"), ("BlockHeld", "bool"),
      ("MaxLockRangeCm", "real"), ("Magazine", "int")],
     ["TryLockOn", "ClearTarget", "InputLight", "InputHeavy", "InputBlock",
      "TryParry", "FirePistol", "Reload"]),
    ("BP_HM_QuestTracker", unreal.ActorComponent,
     [("QuestID", "string"), ("QuestStage", "int"),
      ("ReturnedCells", "bool"), ("BanditsHelped", "bool")],
     ["AcceptMissingCells", "SetQuestStage", "CompleteMissingCells"]),
    ("BP_HM_QuestGiver", unreal.Actor,
     [("NPCName", "string"), ("QuestToOffer", "string"),
      ("CanInteract", "bool")],
     ["Interact", "OfferQuest"]),
    ("BP_HM_CraftingBench", unreal.Actor,
     [("BenchID", "string"), ("IsAvailable", "bool")],
     ["Interact", "CraftRequestedItem"]),
    ("BP_HM_LootPickup", unreal.Actor,
     [("ItemID", "string"), ("Quantity", "int")],
     ["Interact", "AwardLoot"]),
    ("BP_HM_Bandit", unreal.Character,
     [("CurrentHP", "real"), ("Alerted", "bool"),
      ("KnowsPlayerLocation", "bool")],
     ["OnSight", "ReceiveDamage", "Die"]),
]
DEFINITIONS = LAYERED + [d for d in STANDALONE
                         if CREATE_STANDALONE_SCAFFOLDS or d[0] in
                         ("BP_HM_QuestGiver", "BP_HM_CraftingBench", "BP_HM_LootPickup")]
created = []
compiled = []
errors = []
for asset_name, parent, variables, functions in DEFINITIONS:
    path = ROOT + "/" + asset_name
    bp = unreal.load_asset(path)
    if not bp:
        bp = LIB.create_blueprint_asset_with_parent(path, parent)
        if not bp:
            errors.append("Cannot create " + path)
            continue
        created.append(asset_name)
        for var_name, var_type in variables:
            try:
                pin_type = LIB.get_basic_type_by_name(unreal.Name(var_type))
                if not LIB.add_member_variable(bp, unreal.Name(var_name), pin_type):
                    errors.append(asset_name + ": variable " + var_name + " rejected")
            except Exception as exc:
                errors.append(asset_name + ": variable " + var_name + ": " + str(exc))
        for function in functions:
            try:
                LIB.add_function_graph(bp, function)
            except Exception as exc:
                errors.append(asset_name + ": function " + function + ": " + str(exc))
    try:
        good = LIB.compile_blueprint(bp)
        ASSETS.save_loaded_asset(bp)
        if good:
            compiled.append(asset_name)
        else:
            errors.append(asset_name + ": compilation not confirmed")
    except Exception as exc:
        errors.append(asset_name + ": compilation failed " + str(exc))

unreal.log("HEAVY METTLE SCAFFOLDS created: " + ", ".join(created))
unreal.log("Blueprint compilation confirmed: " + ", ".join(compiled))
unreal.log_warning(
    "Scaffold event graphs do not implement the gameplay! See docs/BLUEPRINT_BUILD.md"
)
for error in errors:
    unreal.log_error("HM scaffold: " + error)
if errors:
    raise RuntimeError("Some scaffolds did not compile; inspect Output Log.")
