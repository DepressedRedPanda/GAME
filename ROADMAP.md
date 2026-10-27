# Roadmap

One small, playable step at a time. A milestone is done when its items are ticked in [docs/STATUS.md](docs/STATUS.md) after real Play-In-Editor testing. Milestone letters match [docs/GAME_CONSOLIDATION.md](docs/GAME_CONSOLIDATION.md).

## 0 — It builds (now)
- Build `ManOfMettleEditor` in UE 5.8; fix compile errors one small commit at a time.
- Open the editor; play the runtime graybox world with `HRGameMode`.

## A — Real project in the repo
- Copy your local ManOfMettle `Content/` in; first `.uasset`/`.umap` commit through Git LFS.
- Generate the Blueprint layer (`Scripts/create_blueprint_scaffolds.py`).

## B — Cinder's Rest graybox
- `Scripts/build_dustfall.py` → play `L_Dustfall` (town + Old Relay).

## C — Shooting
- Free-aim hitscan pistol in `BP_HM_Player` ([lesson](docs/FIRST_PLAYABLE_SHOOTING.md)), damage through the C++ attribute component; one hostile bandit.

## D — Melee
- Tune the C++ light/heavy combo, deflect/parry and 350 cm line-of-sight lock-on to `data/balance.json`.

## E — The 15-minute slice
- *The Missing Cells* with Oren Bale, two moral outcomes; crafting bench for ammo; save and reload.

## F — Make it a game
- First free character and weapon via SharedArt; animations; Chaos Cloth duster; sound; crosshair.
- Rename fantasy display text to the names in [docs/WORLD.md](docs/WORLD.md).
- Windows packaged build. Console exploration only with licensed access.

## After the slice (already in C++, switch on one at a time)
*A Thimble of Mercy* and *Nine Bells and a Body* · stealth in the Stalk Fields · Silt Jack's camp · Kestrel · dust skiff · Old Relay puzzles · Drowned Bell · the Spire · Memory Reliquary and the Pale Keeper · Ashen Peak and the Hollow Warden · endings · the Trial · UMG menus · World Partition landscape · online co-op.

The original roadmaps are in `Legacy/`.
