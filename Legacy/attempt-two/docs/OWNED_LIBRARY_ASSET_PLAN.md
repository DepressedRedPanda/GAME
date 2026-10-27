# Heavy Mettle — YOUR EXISTING FAB LIBRARY ASSET INTEGRATION

Last reviewed: 2026-10-17. Source: user-supplied screenshot of Epic Games Launcher > Library (not a verified list of installed assets).

## Engine choice — important correction

The screenshot shows **Unreal Engine 5.5.4** installed, NOT 5.8.3. The prebuilt **Third Person > Combat Variant** relied on by this repository was first introduced in UE **5.6**, so it WILL NOT be available in 5.5.4. Official introduction: https://forums.unrealengine.com/t/unreal-engine-5-6-released/2538952 and https://www.unrealengine.com/news/updated-game-templates-for-unreal-engine-5-6available-now

**Recommended path:** install Unreal Engine **5.8.3** alongside 5.5.4 (do not uninstall old engine or convert original older projects). Use UE5.8.3 Combat template as baseline. An asset being owned in your Epic library does NOT mean it supports 5.8.3; verify the Fab asset's "Supported Engine Versions" dropdown and test in a SEPARATE throwaway 5.8.3 project before adding to HeavyMettle. An asset restricted to 5.6 may need a vendor update or manual retargeting. Do not downgrade assets from a newer engine.

**Alternate 5.5.4 path:** create plain Third Person Blueprint template; build combat, targeting and enemy AI yourself (consider existing assets only after version checks). `place_combat_enemies.py` looks for `BP_CombatEnemy` from UE 5.6+ Combat template and cannot work unchanged under ordinary 5.5.4 Third Person.

## 1. Use now (subject to the exact engine-version dropdown)

| Seen in Library | Official listing | Role | Integration caveat |
| --- | --- | --- | --- |
| Free Animation Library | https://www.fab.com/listings/481ef75b-892b-424f-a213-f1cc058c9c19 | **Top priority**: 6 ledge climbs, 2 mantles, 1 vault, 7 counter/attack moves, crouch/roll/cover | Animation skeleton may differ. Retarget with UE IK Retargeter to Manny/Quinn before adjusting AnimBP |
| FREE Sword + Animation | https://www.fab.com/listings/32408edf-a782-41e8-8002-208ba200347d | Player's holstered sword proxy/model | Listing describes a golden sword plus dance emote, NOT an entire sword-combat move set. Use sword mesh; own combos from template/animations |
| Free Fantasy Weapon Sample | https://www.fab.com/listings/d5be0dc9-1a41-4be2-a63a-5ed436f3445d | Enemy swords, salvage blades, bandit props | 12 weapons in static and skeletal forms, with used/clean materials; choose 1–2 |
| Easy Crosshair – Free Pack | https://www.fab.com/listings/de40757a-69f1-4d2d-87e7-141d3bfca085 | Third-person shooting crosshair, hit and kill indicators | 20 **textures**, not an auto-aim or shooter Blueprint; listing states compatible with all UE5 |
| Medieval Dungeon | https://www.fab.com/listings/c13bd0dc-ac4d-4595-b284-f81386b2e6ef | Relay vaults, stone arches, ruined tunnels | Modular meshes and props; dress small portions of graybox, don't import/ship giant demo map |
| Good SKY | https://www.fab.com/listings/6eb8de95-710e-45cf-a029-e48e709aef03 | Alien frontier daylight/sunset, clouds and storms | Pack offers 20+ presets; replace or disable generator's own `HM_LateSun`, `HM_SkyFill`, `HM_SkyAir` if two sky rigs compete. Developer says active development paused |
| Animation Starter Pack | https://www.fab.com/listings/98ff449d-79db-4f54-9303-75486c4fb9d9 | Standard movement and aiming animations | 62 legacy mannequin animations; IK-retarget to UE5 mannequin before using; Epic UE-only content |

The screenshot also shows **Free Firearm Sounds** (small download), which should be used for the pistol if its asset details and license/engine package are confirmed; no exact official product URL was independently verified yet.

## 2. Evaluate in a throwaway project first (framework collision risk)

| Seen in Library | Listing | What to test / why it is not a blind import |
| --- | --- | --- |
| Ultra Components – Inventory | https://www.fab.com/listings/fb6389d0-dcb2-4fe2-9170-f2881b5c2e92 | Contains list/slot/spatial inventory plus ShooterCharacter, Weapon, Magazine and Bullet components. Strong candidate for both ammo and inventory, but evaluate its UE5.8.3-supported version, external interface and input assumptions. **Do not simultaneously run BP_HM_Inventory's manual item balances**; choose one source of truth and adapt our wrapper |
| RPG/Soulslike Stats System | https://www.fab.com/listings/924bce12-5249-4e27-83b2-cb7369859758 | Attribute->derived-stat system with demo equipment/inventory; useful for health/stamina/battery/XP only if supported on 5.8.3. Disable its example inventory so Ultra (if approved) remains item source of truth. Avoid two HP / damage systems at once |
| UAF H2H Combat | https://www.fab.com/listings/43af38bd-047f-4bf6-8785-23a38374b5e5 | **Built for UE 5.6**, bare fists and evasion, and input is not automatically bound. Keep optional as animation/reference; Epic Combat base remains the melee controller |
| Free Animations Pack | https://www.fab.com/listings/9e93d929-36e8-45f9-b925-325db1b9385d | Additional social/fighting emotes, selective retargeting after baseline |
| Free Bone Snapper | https://www.fab.com/listings/ce093e32-ca2e-43eb-8715-392c22c39fdb | Editor retargeting helper. Engine plugin: use only when specifically supported for 5.8.3; not a runtime dependency |

The following were also visible but are **not needed for the first playable loop**: Unreal's Game Animation Sample (large, ~5.4 GB screenshot), Electric Dreams Env (PCG jungle demo, not western), Paragon Serath/Sevarog/Gideon/Greystone/Kwang (stylized fantasy hero models), temperate vegetation sets and European Black Alder (wrong climate), MetaHumans (great later for facial realism, but expensive rigging/cloth task), Medieval axe/shield, Flashlight System V1, realistic explosion FX, Quixel Bridge/Fab Plugin, spatial audio spline tools, Multiplayer Ragdoll, Ultimate Weapon Loadout (another competing combat framework), and the universal health pack (another competing HP system). These are not rejected permanently.

## 3. Concrete integration order

1. Keep `HeavyMettle` project created locally using UE **5.8.3 Third Person > Combat**. Commit template .uassets only after your local project loads and Plays.
2. Run existing `Scripts/build_dustfall.py`, add NavMesh volume, run `Scripts/place_combat_enemies.py`, then press Play. If this fails, **stop** and fix before importing third-party assets.
3. Add the **Easy Crosshair**, **FREE Sword + Animation**, **Free Fantasy Weapon Sample**, and **Free Animation Library** to a throwaway 5.8.3 Third Person project, then record UE startup log, asset paths and retarget results.
4. Add a sword mesh to the Combat character using one holster socket on spine and one hand socket. Use Free Animation Library's 2 mantle and 1 vault animations to prototype *120/220cm* collision-safe traversals. No runtime C++.
5. Add Free Firearm Sounds after verification and implement first TPS shooting in the Combat character. A crosshair texture alone does not shoot.
6. Once baseline is stable, evaluate **Ultra Components – Inventory** in isolation. If valid, make it authoritative for ammunition, salvage, medkits and grenades. Do not copy the existing empty `BP_HM_Inventory` logic into a second overlapping system.
7. Evaluate **RPG/Soulslike Stats System** in isolation. If valid, adapt to Health, Stamina, Battery, Speed and Damage rather than introducing spells, mana, or its demo RPG classes. Decide which code handles damage and level-up once.
8. Add **Medieval Dungeon** stone meshes and a *single* sky system for dressing Dustfall. Make a full game build only after the graybox is stable.
9. Use `Scripts/audit_imported_assets.py` after importing content. It reports actual asset paths found inside your local Unreal project, which cannot be read from a screenshot of the Launcher.
10. Commit locally generated/retargeted `.uasset/.umap` only when license and redistribution rights allow; avoid uploading third-party marketplace source assets into this **public** GitHub repository without checking the exact license. When in doubt, keep them in local/private project storage and document dependency/install steps.

## 4. What we have vs. what is not in GitHub

The user demonstrably **owns or has listed** the assets in the Epic Games Launcher Library screenshot. They have **not** been imported into the Heavy Mettle local Unreal project by ChatGPT, and cannot be used by a running UE game until installed and mapped to Blueprint components/animation graphs. This repo contains only editor scripts, design text, and metadata (not licensed binaries).

Before committing a binary asset:
- Confirm project target UE 5.8.3 and Fab listed version availability.
- Confirm source asset license and Fab NoAI restrictions where relevant; avoid sending raw prohibited assets to external AI services. As of the listing check, **Free Animation Library** and **Easy Crosshair – Free Pack** both display 'Allows usage with AI: No' on Fab; review the applicable license before supplying source files to automated coding agents. This is not a claim that ordinary UE game development with those assets is prohibited. Do not interpret "owned" as permission to publicly redistribute asset source.
- Keep retarget chain reproducible. Do not embed a locked binary plugin into a public repo.
- Check gameplay while moving/aiming and when AI attacks. Avoid two components controlling the same character attributes or input events.

### Evidence / scope

- Epic's Combat template was introduced in UE5.6 (see link above).
- Fab marketplace features and intended uses are sourced from linked listings.
- The screenshot only shows asset names and launcher controls, not supported engine versions, licensing details, local download completion or shader/Blueprint compilation.
