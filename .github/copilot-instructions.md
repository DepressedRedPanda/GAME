# GitHub Copilot — instructions for this repo

Follow `AGENTS.md` in the repo root; it is the shared rulebook for every AI working here.

Copilot's role:
- **In the editor:** small completions in C++ (`Source/Hollowreach`) and Python editor scripts (`Scripts/`). Match existing style: `AHR`/`UHR` class prefixes, `FName` content IDs, UE 5.8 APIs, no platform-specific calls in gameplay code.
- **Pull request review:** check that each PR is one small playable change, includes three PIE tests and a `docs/daily-progress.md` line, doesn't change saved IDs, and doesn't commit secrets or unlicensed assets.
- **Coding agent (issues assigned to Copilot):** keep to one issue, open a draft PR, and list what was not tested in Unreal.

Riv is a beginner: comments and PR descriptions should be plain English.
