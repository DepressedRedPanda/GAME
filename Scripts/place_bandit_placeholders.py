# Place simple enemy placeholders in generated map (editor script)
import unreal

ROOT = "/Game/HeavyMettle/Blueprints"
ASSETS = unreal.EditorAssetLibrary
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Try to spawn BP_HM_Bandit instances near the Old Relay marker
try:
    bp = unreal.load_asset(ROOT + "/BP_HM_Bandit")
    if not bp:
        unreal.log_warning("BP_HM_Bandit not found. Run create_blueprint_scaffolds.py first.")
    else:
        for i in range(3):
            x = 6900 + i * 200
            y = -2400 + (i // 3) * 440
            actor = ACTORS.spawn_actor_from_class(bp.generated_class, unreal.Vector(x, y, 100))
            actor.set_actor_label("HM_Bandit_" + str(i))
except Exception as e:
    unreal.log_warning("Place enemies script skipped: " + str(e))
