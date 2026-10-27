# GAME / Heavy Mettle — shared rules for every AI agent

Read `docs/GAME_CONSOLIDATION.md`, `docs/CREATIVE_DIRECTION.md`, `docs/QUESTS.md`, `docs/BLUEPRINT_BUILD.md`, and `data/balance.json` before modifying gameplay. Target **Unreal Engine 5.8.4**, local project name **ManOfMettle**, and eventual GitHub repo name **GAME**.

Project invariants:
- One human-looking android protagonist; grounded alien-frontier space western, not fantasy Hollowreach gameplay.
- One playable Blueprint-driven campaign, one player controller, one GameMode, one coherent save/progression chain.
- Preserve the user's existing working local Unreal Third Person project. Do not overwrite its Content/Config/.uproject; verify whether it is standard or Combat variant before using Combat-specific instructions.
- Editor Python may generate maps/scaffolds; text files do not by themselves make packaged playable Blueprint assets.
- Only make small reversible PRs. One agent owns each active feature branch; others review it. No unreviewed concurrent writes to the same Blueprint.
- Never claim a feature works until the Unreal Editor Blueprint compile and Play In Editor both pass. Report **design / scripted / generated / compiled / PIE-tested / packaged** separately.
- Preserve old repositories, code, documentation and histories. Legacy fantasy C++ is an archive/reference, not an active gameplay dependency.
- Commit real Unreal binary assets through Git LFS. Do not ignore .uasset/.umap. Respect Fab redistribution licensing and never commit secrets or console SDKs.
- Workflow: create GitHub Issue with exact acceptance criteria -> implement one narrow feature branch -> review with a different AI -> human tests PIE -> merge PR.
- Optimize for the 15-minute Missing Cells playable slice before adding large systems.

Role division: ChatGPT = integration planner, player experience and scope owner; Claude = narrowly scoped implementation/refactors; Gemini = second-opinion design/compatibility and independent tests; GitHub Copilot = in-editor/IDE assistance, boilerplate and PR review. Every proposed change needs evidence, not AI agreement. Unreal 5.8 experimental MCP integration can be tested in a sandbox with tightly scoped permissions, not assumed connected or production-safe.
