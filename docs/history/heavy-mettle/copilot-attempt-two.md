# Copilot: Heavy Mettle
- Unreal Engine 5.8.3, Blueprint-only runtime project. Avoid introducing C++ modules/build rules.
- Base Epic Third Person **Combat** variant, not Lyra.
- Alien frontier village, ground-level mercenary quests; no magic.
- Melee proximity targeting max 350cm and line-of-sight; beyond that manual TPS aiming only.
- Light/heavy combo chains, block/parry, pistol and sword body sockets, Chaos Cloth jacket after gameplay.
- Built-in engine primitives for graybox; Fab content only with individual compatibility and license verification.
- Use data/balance.json as canonical tuning reference and update it when implementing gameplay.
- Commit generated .uasset/.umap under Git LFS; cannot edit or run them purely from GitHub text.
- State what was actually compiled/tested in Unreal Editor. Favor short testable PRs, not unverified bulk changes.


Owned Fab packs from user's launcher screenshot: see docs/OWNED_LIBRARY_ASSET_PLAN.md and issues #7 and #8. Use already-owned Free Animation Library for mantling and sword/crosshair/sound packs after 5.8.3 checks. Ultra Components has inventory+shooter components; do NOT combine overlapping inventory/health frameworks. Installed UE visible in screenshot is 5.5.4, which lacks the Combat variant; existing repo requires 5.8.3 unless intentionally redesigned. Never assert source assets are in repo; compile & PIE-test them locally, and check license before adding any third-party .uasset to this public repo.


Multiplatform requirement: PS5 is lead target; Windows is build and playable test platform until official console access. Read docs/START_HERE_PS5_MULTIPLATFORM.md. Use platform-neutral Enhanced Input actions, gamepad-first UMG/Common UI navigation, save abstraction and performance scalability. PS5 and Xbox require private authorized SDKs/source-built UE. Never commit restricted console tooling or assert a PS5 build exists without devkit testing.
