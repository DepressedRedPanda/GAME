# verify_project.py - simple checks for project compatibility with scripts and plugins
import unreal

ASSETS = unreal.EditorAssetLibrary
PLUGINS = unreal.Plugins

required_plugins = ["EditorScriptingUtilities", "PythonEditorScriptPlugin"]
missing = []
for p in required_plugins:
    plugin = PLUGINS.find_plugin(p)
    if not plugin or not plugin.is_enabled():
        missing.append(p)

if missing:
    unreal.log_warning("Missing editor plugins: " + ", ".join(missing))
else:
    unreal.log("Required editor plugins enabled.")

# Check engine version compatibility
engine_version = unreal.SystemLibrary.get_engine_version()
unreal.log("Engine version: " + engine_version)

# Verify folders
if not ASSETS.does_directory_exist("/Game/HeavyMettle"):
    unreal.log_warning("/Game/HeavyMettle content folder missing. Run build_dustfall.py to generate.")
else:
    unreal.log("/Game/HeavyMettle folder exists.")
