# GAME — consolidation and migration plan (UE 5.8.4)

## Decision
One shipping game: **Heavy Mettle**, a human-looking amnesiac android taking morally complicated jobs in a lived-in alien frontier. One repository: **GAME** (target name; not yet created). One Unreal project: **ManOfMettle** (the local project name the creator has already chosen). One canonical gameplay runtime; **Blueprint-first**, Editor Python allowed, no imported legacy gameplay C++ by default.

Latest verified engine baseline: **Unreal Engine 5.8.4** (October 20, 2026 hotfix). Use 5.8.x serialization only after a project backup. Do not change versions per feature branch.

## Source ledger and preservation
1. `DepressedRedPanda/attempt-two` — primary design and tools. Preserve all existing scripts, data, docs, design, story, console plan, and issue history. Migrate tested components to the Unreal project only after PIE validation.
2. `DepressedRedPanda/Heavy-Mettle` — preserve its older 5.5 Third Person setup, first-playable tutorials, Script variations, and previous implementation notes as historical reference in `Legacy/Heavy-Mettle-55/`. Compare scripts before replacing newer variants.
3. `DepressedRedPanda/The-Game` — preserve Hollowreach and Veil of Ash architecture, C++ source, procedural systems, quests, saves, tests, and docs as **reference only** under `Legacy/The-Game/`, preferably with complete git history separately accessible. Its fantasy campaign is NOT Heavy Mettle canon; do not merge its GameMode, C++ modules, combat classes, input, saves or assets directly into the active project.

**Important:** This file is a migration specification, not proof that the three repository file trees or histories have already been imported. The original repositories must remain intact until preservation is independently verified. For history-safe migration, use git remote fetch and git subtree with explicit prefixes (or preserve immutable source commit references), avoiding accidental overwrites of shared paths.

## Canonical layout
```text
GAME/
  ManOfMettle.uproject
  Config/
  Content/                 # only local/editor-generated or rights-cleared UE assets
    HeavyMettle/
      Blueprints/
      Characters/
      Maps/
      UI/
  Scripts/                 # Unreal Editor Python tools
  data/                    # balance, quests, tuning
  docs/                    # active design, testing, beginner instructions
  Legacy/
    Heavy-Mettle-55/       # source/docs/scripts from first attempt
    The-Game/              # Hollowreach/Veil reference (not compiled)
  .github/
    copilot-instructions.md
    workflows/
  AGENTS.md
  CLAUDE.md
  GEMINI.md
  README.md
  .gitignore
  .gitattributes
```

## The first actual playable build
Keep local `C:\UnrealProjects\ManOfMettle` if it already exists; back it up before migration. Do **not** replace its `Content`, `Config`, `.uproject` or player character with a text-only repo shell. Audit whether the locally created Third Person template is *Combat variant* or standard, because instructions written for Combat are not automatically valid for standard Third Person. If standard, keep it working and build shooting/melee incrementally; do not pretend Combat enemies exist.

Milestone A: open editor, move/jump in existing map, create checkpoint commit with real .uasset/.umap via Git LFS.
Milestone B: run and adapt `Scripts/build_dustfall.py` against the real project; inspect resulting `L_Dustfall`; play in editor.
Milestone C: free-aim pistol and damage test, then one hostile enemy. Preserve control feel.
Milestone D: sword melee, short 350cm line-of-sight lock-on, defensive move.
Milestone E: first 15-minute *Missing Cells* mercenary job in Cinder's Rest / Old Relay with Oren Bale and two distinct moral outcomes, persistence and reload.
Milestone F: art/audio, optimization, Windows packaged build, then exploration of console ports with licensed platform access.

Don't implement crafting, factions, companion systems, procedural dungeons or multiple RPG progression frameworks until the playable slice runs end to end.

## Legacy feature adaptation policy
Examine Hollowreach's save schema, inventory, quests, dialog triggers, clock, reputation, traversal, procedural layouts and portable tests as reference for *design patterns*. Rewrite only the minimum needed for Heavy Mettle in Blueprint assets and test it. Avoid importing fantasy lore, magic, original game save names, unwanted engine modules, unsupported marketplace files or competing GameModes.

## Asset and version control policy
- Track `*.uasset` and `*.umap` with Git LFS; **never ignore them**.
- Ignore generated `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Saved/`.
- Editor Python is not packaged runtime scripting.
- No API keys, unreleasable SDKs, restricted Fab marketplace packages or massive caches in git.
- CI can lint text/scripts, check LFS pointers and file hygiene. **A green CI result is not proof of UE compilation, PIE, packaged launch or actual platform compatibility.**

## Cutover procedure (not yet executed)
1. Make a private `DepressedRedPanda/GAME` repository (or rename after backup) and ensure linked GitHub tools can access it.
2. Mirror/copy all three repository histories/trees into non-overlapping source namespaces with git subtree or recorded immutable refs; keep a provenance manifest with source commit IDs.
3. Promote `attempt-two` tools/docs/data to active directories, resolving overlapping docs manually.
4. Check out a backup of the locally working `ManOfMettle` Unreal project and add its real `.uproject`, `Config`, `Content` to GAME. Install/configure Git LFS before the first binary asset commit.
5. Standardize 5.8.4 in active guides; mark historical guidance by original version. Keep a test matrix and migrate feature-by-feature through PRs.
6. Archive source repositories only after content and history checks, successful UE editor smoke test and successful clone/LFS checkout from GAME. Never delete source repos to complete the merge.
