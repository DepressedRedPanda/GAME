# Claude — role in GAME / Heavy Mettle

Read `AGENTS.md` and `docs/GAME_CONSOLIDATION.md` first; `AGENTS.md` overrides anything here. Riv's 2026-10-27 decisions (blended world, Hollowreach C++ compiled in, Blueprints on top, project ManOfMettle on UE 5.8) replace the older "Blueprint-only, no Hollowreach C++, Combat template" instructions, which are kept in `Legacy/attempt-two/CLAUDE.md`.

## Your job

1. **Implementation review** (pull requests and the daily lesson): architecture, naming, folder layout, bugs, Blueprint/C++ boundary. At most **3** optional refactors, each with the exact Blueprint or file and a commit message. Flag anything that renames saved IDs or adds a second GameMode/save path.
2. **UE 5.8 build fixes** in `Source/` — the C++ was written for 5.5 and has never been compiled. Smallest change per commit; one beginner-friendly sentence explaining each fix. Keep the single-runtime/single-save design in `docs/systems/hollowreach-veil-integration.md`.
3. **Expose C++ to Blueprint** when a lesson needs it, instead of rewriting systems.
4. **Combine the daily lesson** (`.github/prompts/synthesize.md`), flagging any AI that didn't report.

## Useful context
- Build target `ManOfMettleEditor`, Development Editor, Win64; script `Scripts/BuildEditor.ps1`.
- Checks: `python Scripts/check_combined.py`; saves: `docs/systems/SAVE_MIGRATION.md`; open validation work: `docs/systems/VALIDATION.md`. The earlier C++ test plan in `Legacy/The-Game/CLAUDE.md` still applies.
- Report compiled, PIE, packaged and imported-asset results separately. Untested means untested.

## Owned library integration
Read `docs/OWNED_LIBRARY_ASSET_PLAN.md` before adding external assets. Priority: Free Animation Library (mantles/vault), FREE Sword + Animation (mesh only — combat logic is the C++ combo system), Easy Crosshair (textures), Free Firearm Sounds. Ultra Components overlaps the C++ inventory/health — test it in isolation only, never wire in a second inventory. Hook art in through **Project Settings → Game → Shared Free Asset Appearance** (`Source/SharedArt`). Never upload third-party binaries until the license is checked; run `Scripts/audit_imported_assets.py` locally first.

## Platform mandate
PS5 is the lead design platform; build and test on Windows first with controller input via Enhanced Input (`docs/START_HERE_PS5_MULTIPLATFORM.md`). No hard-coded Windows paths or PlayStation-specific APIs in gameplay code. Console SDK work needs approved access and a source-built engine and stays out of this public repo; never claim a PS5 build exists.
