# Roadmap

One small, playable step at a time. A milestone is done when every item is ticked in [docs/STATUS.md](docs/STATUS.md).

## Milestone 0 — It builds (now)
- Build `HeavyMettleEditor` in UE 5.8; fix compile errors one small commit at a time.
- Open the editor, play the runtime graybox world with `HRGameMode`.
- Generate the Blueprint layer (`Scripts/create_blueprint_scaffolds.py`).

## Milestone 1 — First playable, 15 minutes (from attempt-two)
- Pistol: hitscan free-aim shooting in `BP_HM_Player` ([lesson](docs/setup/first-playable-shooting.md)), using the C++ combat/attribute components for damage.
- Sword: confirm the C++ light/heavy combo, deflect and lock-on (3.5 m) feel right; tune with `data/balance.json` numbers.
- Cinder's Rest graybox + the Old Relay; two enemy types; crafting bench; *The Missing Cells*; one save slot.
- Windows packaged build.

## Milestone 2 — Make it feel good
- First free character + weapon from Fab via SharedArt settings ([asset plan](docs/assets/OWNED_LIBRARY_ASSET_PLAN.md)); Mixamo or Free Animation Library animations; duster coat with Chaos Cloth.
- Hit reactions, camera shake, sounds (Free Firearm Sounds), crosshair.
- Rename fantasy display text to the space-western names in [WORLD.md](docs/design/WORLD.md).

## Milestone 3 — The frontier opens
- *A Thimble of Mercy*, *Nine Bells and a Body*; stealth in the Stalk Fields; Silt Jack's camp; Kestrel the companion; dust skiff.

## Milestone 4 — The Hollow
- Old Relay puzzles, the Drowned Bell, the Spire, Memory Reliquary, Pale Keeper, Ashen Peak, Hollow Warden, endings.

## Milestone 5 — Polish and platforms
- UMG menus replacing the canvas HUD; World Partition landscape; controller-first input; performance; PS5 dev-kit work once approved.

## Later
Online co-op (the C++ is single-player; Veil's old replication code is in `Archive/`), the Trial as a roguelite, New Game+.

The original roadmaps are kept in `docs/history/`.
