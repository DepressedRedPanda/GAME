# Free asset plan for the combined UE 5.5 project

The project is one integrated campaign; see INTEGRATION.md and SAVE_MIGRATION.md.

No third-party packs have been downloaded, acquired or imported by this integration. The following free listings/built-in content are a candidate catalog, checked on 2026-10-12. Engine package versions and actual license terms must be recorded when each pack is acquired. See the machine-readable [manifest](free_asset_manifest.json).

## First imports

1. Choose Greystone **or** Kwang for Player, not both at once. Keep the included skeleton, adapt/create an Animation Blueprint, and validate movement before adding attacks.
2. Assign one sword to Weapon.Default; verify the configured hand socket and transform. Wire one Player.Light0 montage, then heavy/combo variants. Existing combat timings remain authoritative.
3. Import Medieval Dungeon and Soul: Cave. Assign a gate or resource mesh and selected surface materials; author one small area before redoing generated world geometry.
4. Add Black Alder, foliage and berries in a limited area; check frame rate, collision and stealth/gathering behavior.
5. Add one Niagara explosion/fire impact, then sounds from Starter Content. Expand only after each addition works.

## Catalog

| Free asset | Intended use / profile | Work remaining |
| --- | --- | --- |
+| [Greystone](https://www.fab.com/listings/122fd7bf-6f12-4304-a930-cccbbacdaebc) | Player or Enemy | model + animation Blueprint + attack source. |
| [Kwang](https://www.fab.com/listings/f4c67e92-b976-4b5b-ab9f-4c25b010f6f3) | Player or Companion | alternative sword character. |
| [Serath](https://www.fab.com/listings/522b6160-15ab-492b-a2b0-c09f9bb5f6e6) | Companion or Enemy | alternative fantasy fighter. |
| [Sevarog](https://www.fab.com/listings/a4882b5e-cfad-4830-a3dd-46a6c31a79b2) | PaleKeeper | boss model and animation source. |
| [Gideon](https://www.fab.com/listings/51935254-f70f-400a-8ca5-91a3e1b83e3b) | NPC or Enemy | mage model and ability-effect source. |
| [Animation Starter Pack](https://www.fab.com/listings/98ff449d-79db-4f54-9303-75486c4fb9d9) | Characters / Montages | legacy mannequin animation source; retargeting required for other skeletons. |
| [Game Animation Sample](https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016) | Characters | choose a 5.5-or-earlier sample; locomotion/traversal source, not a replacement controller. |
| [Medieval Dungeon (Infuse Studio)](https://www.fab.com/listings/c13bd0dc-ac4d-4595-b284-f81386b2e6ef) | Gate / HRLootContainer / Dungeon.Floor / Dungeon.Wall | modular geometry and prop/material source; full level authoring remains. |
| [Soul: Cave](https://www.fab.com/listings/75f42402-40bb-4a1b-b557-18e2c9604273) | Stone / Ruin / Water / Resource.IronOre | cave/rock/water material and prop source. |
| [European Black Alder](https://www.fab.com/listings/9de7ce19-5813-42d2-a5f0-5e6447006f72) | Authored forest / decorative scenery | older tree collection; do not substitute newer experimental Megaplants version. |
| [Temperate Vegetation: Foliage Collection](https://www.fab.com/listings/6a5ae8db-d80f-4b23-b276-87da390cfe56) | Stealth.Grass / authored foliage | grass and ground vegetation; preserve gameplay stealth volumes. |
| [Temperate Vegetation: Wild Berries](https://www.fab.com/listings/b6307eff-f99f-48ae-bc3e-4d15b56eda11) | Resource.Herb | resource-node appearance; does not add a new food item or harvesting rules. |
| [Temperate Vegetation: Fern Collection](https://www.fab.com/listings/b778bd8f-524c-42b6-b60c-4caac59029c1) | Authored foliage / Resource.Herb | forest groundcover and alternative herb visuals. |
| [temperate Vegetation: Spruce Forest](https://www.fab.com/listings/f8044501-17a2-498f-b198-5f1bc71ee87a) | Authored forest | colder forest alternative; importing does not replace generated trees automatically. |
| [temperate Vegetation: Meadow Flowers](https://www.fab.com/listings/e648b1bd-11f6-4895-bd43-15e399c80297) | Authored foliage / Resource.Herb | plant variety for fields and gathering. |
| [Medieval weapon axe and shield Set](https://www.fab.com/listings/0842eb1b-c49a-4d3c-9a57-a081944427b6) | Weapon.Default / Weapon.Item.IronBlade | weapon models; shield appearance does not implement a new shield mechanic. |
| [Realistic Fire & Explosion VFX Vol.1 - FREE Starter Pack](https://www.fab.com/listings/2adf4e4e-5f1d-4543-9e8c-192fe3c9a8ed) | Physics.Explosion / Projectile.Fire.Impact / Checkpoint.Discover | free 10-effect sampler; verify a 5.5 package is available. |
| [Unreal Starter Content](https://dev.epicgames.com/documentation/en-us/unreal-engine/starter-content-in-unreal-engine?application_version=5.1) | SurfaceMaterials / Sounds / Props | built-in basic materials, sounds and props; legacy particles are not Niagara systems. |

## Import procedure

Acquire each pack on Riv's own Epic/Fab account. Select a package compatible with UE 5.5 or earlier. If it is a complete sample project, open a copy in the supported version and migrate only the required assets plus dependencies into the combined project's Content folder. Do not overwrite our player controller/GameMode with a sample framework. Raw FBX/texture/WAV imports may be possible where newer-only Unreal packages are not, but need their own material/skeleton setup.

Do not fabricate asset object paths. After import, use the editor's actual asset references in Project Settings > Game > Shared Free Asset Appearance. Retarget animation sequences only when the skeleton differs; check the resulting feet, hands, weapons and root motion. Imported Animation Blueprints may cast to a different game's character type and require adaptation.

SharedArt's mesh overlays retain original hidden collision geometry. Align them deliberately, particularly doors, chests and physics crates. Skeletal cosmetics retain the character capsule. Terrain material slots do not automatically replace the world's cube instances with a full dungeon pack. Free-tree and foliage packs can be placed manually now; converting the generator to an art-aware mesh palette is a separate feature.

Avoid dependencies on a paid full edition behind a free sampler. Do not treat a personal-use audio demo as approved shipping audio. Keep acquisition/license information in the log below. Preserve asset packages privately according to their actual collaboration/distribution terms; no raw asset redistribution is performed by this change.

## Import log (currently empty)

For each pack record: actual acquired package/version, license, engine used to import, source folders, configured profiles, skeleton/Animation Blueprint, montage slots, dependencies, tested gameplay, frame-rate effect, and screenshots if helpful. Mark the manifest integrated only after import and verification; the current catalog is not an import claim.
