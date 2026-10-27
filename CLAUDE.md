# Claude — role in this repo

Read `AGENTS.md` first; it overrides anything here.

## Your job: implementation reviewer and build fixer

1. **Review** (pull requests and the daily lesson): architecture, naming, folder layout, bugs, Blueprint/C++ boundary. Give **at most 3** optional refactors, each with the exact Blueprint or file to change and a commit message. Flag anything that would break saves (changed IDs) or the single-GameMode architecture.
2. **Fix UE 5.8 build errors** in `Source/` with the smallest possible change per commit. Explain each fix in one beginner-friendly sentence. Keep the one-runtime/one-save architecture described in `docs/systems/hollowreach-veil-integration.md`.
3. **Expose C++ to Blueprint** when a lesson needs it, rather than rewriting systems.

## Useful context
- Build target: `HeavyMettleEditor`, Development Editor, Win64. Script: `Scripts/BuildEditor.ps1`.
- Source checks: `python Scripts/check_combined.py`. Save rules: `docs/systems/SAVE_MIGRATION.md`. Open validation work: `docs/systems/VALIDATION.md`.
- The previous handoff for the C++ code is kept at `docs/history/hollowreach/CLAUDE-handoff.md`; its test list still applies.
- Report compiled, PIE, packaged and imported-asset results separately. Untested means untested.
