# HEAVY METTLE — instructions for Gemini
Engine: Unreal 5.8.3. Genre: Blueprint-first solo third-person RPG/shooter, alien frontier spaghetti western. User wants editable Blueprints and no C++ compile problems. Repo is not yet a packaged game; use a real Unreal Editor to generate and test binary assets. Work from data/balance.json, docs/BLUEPRINT_BUILD.md, and docs/CREATIVE_DIRECTION.md. Start with Epic Third Person Combat variant; use UE primitives and rights-cleared free Fab art. Prefer small individually runnable branches and a 15-minute playable quest. Never assume you can reuse marketplace source content in GitHub without license verification. Verify graph compilation and Play in Editor before claiming feature implementation.


## Owned library integration
See docs/OWNED_LIBRARY_ASSET_PLAN.md and GitHub issues #7-8. The screenshots show Epic Launcher library, not an installed Unreal HeavyMettle Content folder. User currently shows engine 5.5.4 while this repo targets 5.8.3 with the 5.6+ Combat template. Evaluate compatibility in a temporary 5.8.3 project; prefer Free Animation Library, FREE Sword + Animation, Easy Crosshair, Free Firearm Sounds and then Ultra Components for ammo+inventory if no collision. Respect license and source redistribution limitations; do not assume asset source can be copied to this public GitHub repository.


## Platform mandate
Lead platform PS5; first development environment Windows UE5.8. For controller-first Enhanced Input, platform-neutral save/menu architecture, Sony/Epic access gates and Xbox/Nintendo future-port plan, follow docs/START_HERE_PS5_MULTIPLATFORM.md. Console packaging requires official developer authorization, platform SDK and source-built Unreal; all gameplay systems can still be Blueprint-first. No console builds or runtime footage have been verified yet.
