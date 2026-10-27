"""Audit assets already imported into the local Unreal Editor project.

This script does NOT download any assets from the Epic Games Launcher account.
Run from Tools > Execute Python Script AFTER importing selected Fab packs.
Writes Saved/HeavyMettleImportAudit.csv and logs matches to Output Log.
Reports candidate asset paths based on names; it does not prove a pack's license,
UE version compatibility, or that the content is wired into gameplay.
"""
import csv
import os
import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
prefix = unreal.Name("/Game")
all_asset_data = registry.get_assets_by_path(prefix, recursive=True)
keywords = {
    "sword": ["sword", "blade", "katana", "scabbard", "scimitar"],
    "firearm": ["pistol", "gun", "bullet", "magazine", "reload", "firearm", "muzzle"],
    "climbing": ["mantle", "vault", "ledge", "climb", "parkour"],
    "melee": ["attack", "combo", "parry", "block", "counter"],
    "crosshair": ["crosshair", "reticle", "hitmarker"],
    "crafting": ["inventory", "component", "item", "craft", "loot"],
    "environment": ["dungeon", "medieval", "castle", "ruin", "stone", "sky", "cloud"],
    "sound": ["firearmsound", "gunshot", "shot", "reload", "impact"],
}
rows = []
for asset_data in all_asset_data:
    try:
        object_path = str(asset_data.get_soft_object_path())
    except Exception:
        object_path = str(asset_data.package_name)
    if object_path.startswith("/Game/HeavyMettle/"):
        continue
    name = str(asset_data.asset_name)
    low = (name + " " + object_path).lower()
    groups = [label for label, hints in keywords.items() if any(h in low for h in hints)]
    if not groups:
        continue
    try:
        class_name = str(asset_data.asset_class_path.asset_name)
    except Exception:
        class_name = "Unknown"
    rows.append({
        "asset_path": object_path,
        "asset_name": name,
        "type": class_name,
        "candidate_uses": "|".join(groups),
        "confirmed_fab_pack": "NO — source must be verified manually",
        "confirmed_compatible": "NO — open/compile locally",
        "integration_state": "NOT_VERIFIED",
    })

rows.sort(key=lambda x: (x["candidate_uses"], x["asset_path"]))
output_path = os.path.join(unreal.Paths.project_saved_dir(), "HeavyMettleImportAudit.csv")
os.makedirs(os.path.dirname(output_path), exist_ok=True)
with open(output_path, "w", newline="", encoding="utf-8-sig") as stream:
    writer = csv.DictWriter(stream, fieldnames=[
        "asset_path", "asset_name", "type", "candidate_uses", "confirmed_fab_pack",
        "confirmed_compatible", "integration_state",
    ])
    writer.writeheader()
    writer.writerows(rows)
unreal.log("HEAVY METTLE: scanned {} local assets; found {} candidate assets.".format(
    len(all_asset_data), len(rows)))
unreal.log("HEAVY METTLE: wrote " + output_path)
unreal.log_warning("Candidates are not proof of license, import source or Blueprints working in-game.")
