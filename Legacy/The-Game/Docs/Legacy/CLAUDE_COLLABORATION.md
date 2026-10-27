# Message to give Claude Code after sharing GitHub repo

This is an original UE 5.8 C++ project, VeilOfAsh. Please inspect `README.md`, `FEATURE_MATRIX.md` and all classes in `Source/VeilOfAsh`.

First task: run a real Unreal Engine 5.8 editor-target build on a machine with UE installed, identify UHT and compiler errors, fix the smallest set of issues, and open a pull request. Do not replace the architecture wholesale or alter game rules as part of the compile-fix PR. Record exactly what you built and the full error/fix summary. Preserve server-authoritative action validation and replication.

Second task after first merge: build a self-contained inventory + equipment subsystem on a new branch using DataAssets/GameplayTags as appropriate. Avoid direct edits to `AshHero.cpp` if ChatGPT is concurrently working there. No copyright-protected characters, artwork, or game assets.

The starter is intentionally not certified as compile-tested. Treat code as an initial prototype requiring engineering verification.
