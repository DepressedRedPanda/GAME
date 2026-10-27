"""Inspect a local UE 5.8 editor session for first-playable Heavy Mettle assets.

Read the UE Output Log after running. This reports the local machine's real state;
it does not certify combat mechanics that were never implemented.
"""
import unreal

ASSETS = unreal.EditorAssetLibrary
LEVEL = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MAP = "/Game/HeavyMettle/Maps/L_Dustfall"
ROOT = "/Game/HeavyMettle/Blueprints"
critical = []
warnings = []

if ASSETS.does_asset_exist(MAP):
    unreal.log("[PASS] Heavy Mettle Dustfall level asset exists")
    if not LEVEL.load_level(MAP):
        critical.append("Level exists but could not be opened")
    else:
        labels = [a.get_actor_label() for a in ACTORS.get_all_level_actors()]
        if any(lab == "HM_PlayerStart" for lab in labels):
            unreal.log("[PASS] PlayerStart is in the Dustfall level")
        else:
            critical.append("No Heavy Mettle PlayerStart")
        if any(lab.startswith("HM_BountyBoard_") for lab in labels):
            unreal.log("[PASS] Bounty board graybox found")
        else:
            warnings.append("Quest board proxy absent")
        bandits = sum(1 for lab in labels if lab.startswith("HM_RelayBandit_"))
        if bandits == 3:
            unreal.log("[PASS] Three Combat-template enemy actors placed")
        else:
            warnings.append("Expected 3 enemies in Old Relay, found " + str(bandits))
else:
    critical.append("Missing L_Dustfall. Run Scripts/build_dustfall.py.")

template_enemy = [
    p for p in ASSETS.list_assets("/Game", recursive=True, include_folder=False)
    if str(p).rsplit("/", 1)[-1].split(".")[0] == "BP_CombatEnemy"
]
if template_enemy:
    unreal.log("[PASS] Epic or compatible Combat enemy Blueprint available")
else:
    critical.append("BP_CombatEnemy missing. Open a Combat variant template.")

for bp_name in ("BP_HM_Stats", "BP_HM_Inventory", "BP_HM_Combat",
                "BP_HM_QuestTracker", "BP_HM_QuestGiver", "BP_HM_CraftingBench",
                "BP_HM_LootPickup", "BP_HM_Bandit"):
    if ASSETS.does_asset_exist(ROOT + "/" + bp_name):
        unreal.log("[PASS] Editable Blueprint scaffold: " + bp_name)
    else:
        warnings.append("Blueprint scaffold missing: " + bp_name)

if warnings:
    for value in warnings:
        unreal.log_warning("[WARNING] " + value)
if critical:
    for value in critical:
        unreal.log_error("[FAILED] " + value)
    raise RuntimeError("Heavy Mettle first-playable prerequisites missing")
unreal.log("HEAVY METTLE: asset presence verified. Manual Play-In-Editor test STILL REQUIRED.")
